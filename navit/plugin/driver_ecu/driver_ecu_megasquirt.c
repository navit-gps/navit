/**
 * Navit, a modular navigation system.
 *
 * MegaSquirt serial backend for ECU fuel monitoring.
 */

#include "driver_ecu_megasquirt.h"

#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "callback.h"
#include "debug.h"
#include "event.h"

struct event_timeout;

#define MS_DEFAULT_DEVICE "/dev/ttyUSB0"
#define MS_REALTIME_BUF_SIZE 128
#define MS_OFFSET_RPM 2
#define MS_OFFSET_PW1 4
#define MS_N_CYL_DEFAULT 4

struct driver_ecu_megasquirt {
    struct ecu_config config;
    struct ecu_fuel_state *fuel;
    int fd;
    struct event_timeout *poll_timeout;
    int n_cyl;
    int initialized;
};

static int ms_open_serial(const char *device) {
    int fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        dbg(lvl_warning, "Driver ECU MegaSquirt: Cannot open %s: %s", device, strerror(errno));
        return -1;
    }

    struct termios tio;
    if (tcgetattr(fd, &tio) < 0) {
        dbg(lvl_warning, "Driver ECU MegaSquirt: tcgetattr failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    cfmakeraw(&tio);
    cfsetispeed(&tio, B115200);
    cfsetospeed(&tio, B115200);
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~CSTOPB;
    tio.c_cflag &= ~PARENB;

    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        dbg(lvl_warning, "Driver ECU MegaSquirt: tcsetattr failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

static int ms_write_line(int fd, const char *cmd) {
    ssize_t len = (ssize_t)strlen(cmd);
    ssize_t written = write(fd, cmd, len);
    if (written != len) {
        dbg(lvl_warning, "Driver ECU MegaSquirt: write '%s' failed: %s", cmd, strerror(errno));
        return 0;
    }
    return 1;
}

static int ms_read_response(int fd, char *buf, size_t buflen) {
    ssize_t n = read(fd, buf, buflen - 1);
    if (n <= 0) {
        return 0;
    }
    buf[n] = '\0';
    return (int)n;
}

static int ms_read_realtime_block(int fd, unsigned char *buf, size_t buflen) {
    ssize_t n = read(fd, buf, buflen);
    if (n <= 0) {
        return 0;
    }
    return (int)n;
}

static void ms_detect_version(int fd) {
    char resp[64];

    if (!ms_write_line(fd, "V\r")) {
        return;
    }
    if (!ms_read_response(fd, resp, sizeof(resp))) {
        dbg(lvl_debug, "Driver ECU MegaSquirt: No response to version query");
        return;
    }
    if (strstr(resp, "MS")) {
        dbg(lvl_info, "Driver ECU MegaSquirt: Detected firmware: %s", resp);
    } else {
        dbg(lvl_debug, "Driver ECU MegaSquirt: Unexpected version response: %s", resp);
    }
}

static double ms_injector_to_fuel_rate(double pw_ms, unsigned int rpm, int n_cyl, int injector_flow_cc_min) {
    if (pw_ms <= 0.0 || rpm == 0 || n_cyl <= 0 || injector_flow_cc_min <= 0) {
        return 0.0;
    }

    double rate = pw_ms * (double)rpm * (double)n_cyl * (double)injector_flow_cc_min / 2000000.0;

    if (rate < 0.0 || rate > 600.0) {
        return 0.0;
    }
    return rate;
}

static void ms_poll(struct driver_ecu_megasquirt *ctx);

static void ms_poll_timeout(struct event_timeout *to, void *data) {
    struct driver_ecu_megasquirt *ctx = (struct driver_ecu_megasquirt *)data;
    (void)to;
    ms_poll(ctx);
}

static int ms_apply_realtime_block(struct driver_ecu_megasquirt *ctx, const unsigned char *buf, int n) {
    if (n < (MS_OFFSET_PW1 + 2)) {
        dbg(lvl_debug, "Driver ECU MegaSquirt: Short realtime block (%d bytes)", n);
        return 0;
    }

    unsigned int rpm = ((unsigned int)buf[MS_OFFSET_RPM] << 8) | (unsigned int)buf[MS_OFFSET_RPM + 1];
    unsigned int pw_raw = ((unsigned int)buf[MS_OFFSET_PW1] << 8) | (unsigned int)buf[MS_OFFSET_PW1 + 1];
    double pw_ms = (double)pw_raw / 1000.0;

    if (!(rpm > 200 && rpm < 12000 && pw_ms > 0.0 && pw_ms < 50.0)) {
        return 0;
    }

    int flow_cc = ctx->config.fuel_injector_flow_cc_min;
    if (flow_cc <= 0) {
        dbg(lvl_debug, "Driver ECU MegaSquirt: Injector flow not configured, skipping update");
        return 0;
    }

    double rate_l_h = ms_injector_to_fuel_rate(pw_ms, rpm, ctx->n_cyl, flow_cc);
    if (rate_l_h <= 0.0) {
        return 0;
    }

    ctx->fuel->fuel_rate_l_h = rate_l_h;
    dbg(lvl_debug, "Driver ECU MegaSquirt: rpm=%u pw=%.3f ms n_cyl=%d flow=%d cc/min -> fuel_rate=%.2f L/h", rpm, pw_ms,
        MS_N_CYL_DEFAULT, flow_cc, rate_l_h);
    return 1;
}

static void ms_poll(struct driver_ecu_megasquirt *ctx) {
    unsigned char buf[MS_REALTIME_BUF_SIZE];
    int n;

    if (!ctx || ctx->fd < 0 || !ctx->fuel) {
        return;
    }

    if (!ms_write_line(ctx->fd, "A\r")) {
        dbg(lvl_debug, "Driver ECU MegaSquirt: Failed to send realtime command");
        goto schedule_next;
    }

    n = ms_read_realtime_block(ctx->fd, buf, sizeof(buf));
    (void)ms_apply_realtime_block(ctx, buf, n);

schedule_next:
    ctx->poll_timeout = event_add_timeout(3000, 1, callback_new_1(callback_cast(ms_poll_timeout), ctx));
}

struct driver_ecu_megasquirt *driver_ecu_megasquirt_start(const struct ecu_config *config,
                                                          struct ecu_fuel_state *fuel) {
    struct driver_ecu_megasquirt *ctx;
    const char *device = MS_DEFAULT_DEVICE;

    if (!config || !fuel) {
        return NULL;
    }

    if (!config->fuel_megasquirt_available || config->fuel_injector_flow_cc_min <= 0) {
        return NULL;
    }

    ctx = g_new0(struct driver_ecu_megasquirt, 1);
    ctx->config = *config;
    ctx->fuel = fuel;
    ctx->n_cyl = MS_N_CYL_DEFAULT;
    ctx->fd = ms_open_serial(device);
    if (ctx->fd < 0) {
        g_free(ctx);
        return NULL;
    }

    ms_detect_version(ctx->fd);

    ctx->initialized = 1;
    ctx->poll_timeout = event_add_timeout(3000, 1, callback_new_1(callback_cast(ms_poll_timeout), ctx));
    dbg(lvl_info, "Driver ECU MegaSquirt: Backend started on %s", device);

    return ctx;
}

void driver_ecu_megasquirt_stop(struct driver_ecu_megasquirt *ctx) {
    if (!ctx)
        return;

    if (ctx->poll_timeout) {
        event_remove_timeout(ctx->poll_timeout);
        ctx->poll_timeout = NULL;
    }

    if (ctx->fd >= 0) {
        close(ctx->fd);
        ctx->fd = -1;
    }

    g_free(ctx);
}
