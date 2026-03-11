/**
 * Navit, a modular navigation system.
 *
 * MegaSquirt serial backend for Driver Break plugin.
 *
 * This backend connects to a MegaSquirt ECU on a serial port (default
 * /dev/ttyUSB0) and periodically requests realtime data using the classic
 * MS2/Extra "A" command. From the returned binary block it extracts RPM and
 * injector pulse width and converts these into an approximate fuel rate
 * (L/h) using the configured injector flow rate and cylinder count.
 *
 * The intent is to provide a practical, working baseline that can be tuned
 * per firmware; if the data are not plausible, the backend will skip updates
 * rather than corrupting the adaptive fuel learning.
 */

#include "driver_break_megasquirt.h"
#include "callback.h"
#include "debug.h"
#include "driver_break.h"
#include "event.h"
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* Default serial device and protocol parameters */
#define MS_DEFAULT_DEVICE "/dev/ttyUSB0"
#define MS_BAUD 115200
#define MS_REALTIME_BUF_SIZE 128

/* Offsets into the realtime data block for RPM and PW1.
 * These are based on common MS2/MS3 layouts; they may be adapted per firmware.
 */
#define MS_OFFSET_RPM 2
#define MS_OFFSET_PW1 4

/* Default number of cylinders if not otherwise known (inline-4). */
#define MS_N_CYL_DEFAULT 4

struct driver_break_megasquirt {
    struct driver_break_priv *priv;
    int fd;
    struct event_timeout *poll_timeout;
    int n_cyl;
    int initialized;
};

static int ms_open_serial(const char *device) {
    int fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        dbg(lvl_warning, "Driver Break MegaSquirt: Cannot open %s: %s", device, strerror(errno));
        return -1;
    }

    struct termios tio;
    if (tcgetattr(fd, &tio) < 0) {
        dbg(lvl_warning, "Driver Break MegaSquirt: tcgetattr failed: %s", strerror(errno));
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
        dbg(lvl_warning, "Driver Break MegaSquirt: tcsetattr failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

static int ms_write_line(int fd, const char *cmd) {
    ssize_t len = (ssize_t)strlen(cmd);
    ssize_t written = write(fd, cmd, len);
    if (written != len) {
        dbg(lvl_warning, "Driver Break MegaSquirt: write '%s' failed: %s", cmd, strerror(errno));
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

/* Read a single realtime data block after sending the 'A' command.
 * Returns number of bytes read, or 0 on failure.
 */
static int ms_read_realtime_block(int fd, unsigned char *buf, size_t buflen) {
    ssize_t n = read(fd, buf, buflen);
    if (n <= 0) {
        return 0;
    }
    return (int)n;
}

/* Optionally detect that we are really talking to a MegaSquirt by sending
 * 'V' (version) and looking for 'MS' in the response. Failure is not fatal
 * but is logged.
 */
static void ms_detect_version(int fd) {
    char resp[64];

    if (!ms_write_line(fd, "V\r")) {
        return;
    }
    if (!ms_read_response(fd, resp, sizeof(resp))) {
        dbg(lvl_debug, "Driver Break MegaSquirt: No response to version query");
        return;
    }
    if (strstr(resp, "MS")) {
        dbg(lvl_info, "Driver Break MegaSquirt: Detected firmware: %s", resp);
    } else {
        dbg(lvl_debug, "Driver Break MegaSquirt: Unexpected version response: %s", resp);
    }
}

/* Convert injector pulse width and RPM into fuel rate (L/h).
 *
 * This uses a simple model:
 *
 *   - pw_ms is injector open time per engine cycle for one cylinder.
 *   - rpm is crankshaft RPM.
 *   - n_cyl is number of cylinders.
 *   - injector_flow_cc_min is injector flow at rated pressure (cc/min).
 *
 * Total injector open time per minute across all cylinders is:
 *   t_open_min = pw_ms/1000 * (rpm/2) * n_cyl * 60
 * and volume per minute is:
 *   V_cc_min = (t_open_min / 60) * injector_flow_cc_min
 *
 * Combining and simplifying yields approximately:
 *   fuel_rate_l_h = pw_ms * rpm * n_cyl * injector_flow_cc_min / 2_000_000
 */
static double ms_injector_to_fuel_rate(double pw_ms, unsigned int rpm, int n_cyl, int injector_flow_cc_min) {
    if (pw_ms <= 0.0 || rpm == 0 || n_cyl <= 0 || injector_flow_cc_min <= 0) {
        return 0.0;
    }

    double rate = pw_ms * (double)rpm * (double)n_cyl * (double)injector_flow_cc_min / 2000000.0;

    if (rate < 0.0 || rate > 600.0) {
        /* Reject obviously bogus values */
        return 0.0;
    }
    return rate;
}

static void ms_poll(struct driver_break_megasquirt *ctx);

static void ms_poll_timeout(struct event_timeout *to, void *data) {
    struct driver_break_megasquirt *ctx = (struct driver_break_megasquirt *)data;
    (void)to;
    ms_poll(ctx);
}

static void ms_poll(struct driver_break_megasquirt *ctx) {
    unsigned char buf[MS_REALTIME_BUF_SIZE];
    int n;

    if (!ctx || ctx->fd < 0 || !ctx->priv) {
        return;
    }

    /* Request realtime data block */
    if (!ms_write_line(ctx->fd, "A\r")) {
        dbg(lvl_debug, "Driver Break MegaSquirt: Failed to send realtime command");
        goto schedule_next;
    }

    n = ms_read_realtime_block(ctx->fd, buf, sizeof(buf));
    if (n < (MS_OFFSET_PW1 + 2)) {
        dbg(lvl_debug, "Driver Break MegaSquirt: Short realtime block (%d bytes)", n);
        goto schedule_next;
    }

    unsigned int rpm = ((unsigned int)buf[MS_OFFSET_RPM] << 8) | (unsigned int)buf[MS_OFFSET_RPM + 1];
    unsigned int pw_raw = ((unsigned int)buf[MS_OFFSET_PW1] << 8) | (unsigned int)buf[MS_OFFSET_PW1 + 1];
    double pw_ms = (double)pw_raw / 1000.0;

    if (rpm > 200 && rpm < 12000 && pw_ms > 0.0 && pw_ms < 50.0) {
        int flow_cc = ctx->priv->config.fuel_injector_flow_cc_min;
        if (flow_cc <= 0) {
            dbg(lvl_debug, "Driver Break MegaSquirt: Injector flow not configured, skipping update");
            goto schedule_next;
        }

        /* For now use MS_N_CYL_DEFAULT; future versions may take this from config. */
        double rate_l_h = ms_injector_to_fuel_rate(pw_ms, rpm, ctx->n_cyl, flow_cc);
        if (rate_l_h > 0.0) {
            ctx->priv->fuel_rate_l_h = rate_l_h;
            dbg(lvl_debug, "Driver Break MegaSquirt: rpm=%u pw=%.3f ms n_cyl=%d flow=%d cc/min -> fuel_rate=%.2f L/h",
                rpm, pw_ms, MS_N_CYL_DEFAULT, flow_cc, rate_l_h);
        }
    }

schedule_next:
    /* Schedule next poll in 3 seconds */
    ctx->poll_timeout = event_add_timeout(3000, 1, callback_new_1(callback_cast(ms_poll_timeout), ctx));
}

struct driver_break_megasquirt *driver_break_megasquirt_start(struct driver_break_priv *priv) {
    struct driver_break_megasquirt *ctx;
    const char *device = MS_DEFAULT_DEVICE;

    if (!priv) {
        return NULL;
    }

    /* Only start if explicitly enabled; this avoids accidentally grabbing the same serial device as OBD-II. */
    if (!priv->config.fuel_megasquirt_available || priv->config.fuel_injector_flow_cc_min <= 0) {
        return NULL;
    }

    ctx = g_new0(struct driver_break_megasquirt, 1);
    ctx->priv = priv;
    ctx->n_cyl = MS_N_CYL_DEFAULT;
    ctx->fd = ms_open_serial(device);
    if (ctx->fd < 0) {
        g_free(ctx);
        return NULL;
    }

    ms_detect_version(ctx->fd);

    ctx->initialized = 1;
    ctx->poll_timeout = event_add_timeout(3000, 1, callback_new_1(callback_cast(ms_poll_timeout), ctx));
    dbg(lvl_info, "Driver Break MegaSquirt: Backend started on %s", device);

    return ctx;
}

void driver_break_megasquirt_stop(struct driver_break_megasquirt *ctx) {
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
