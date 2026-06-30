/**
 * Navit, a modular navigation system.
 *
 * OBD-II (ELM327) backend for ECU fuel monitoring.
 *
 * This module implements a minimal ELM327-based OBD-II poller that:
 * - Opens a serial port (default /dev/ttyUSB0, 115200 8N1).
 * - Sends a short init sequence (ATZ, E0, L0, H0, S0, SP0).
 * - Discovers supported PIDs via 0100/0120/0140.
 * - Polls key PIDs and writes fuel level and rate into ecu_fuel_state.
 *
 * When a PID is unsupported or responses are invalid, the backend disables only
 * the affected signal and continues with remaining PIDs. If the adapter fails,
 * the backend is shut down and manual/learning paths are left intact.
 */

#include "driver_ecu_obd.h"

#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "callback.h"
#include "debug.h"
#include "event.h"

struct event_timeout;

struct driver_ecu_obd {
    struct ecu_config *config;
    struct ecu_fuel_state *fuel;
    int fd;
    struct event_timeout *poll_timeout;
    unsigned int supported_pids[3]; /* 0x00-0x1F, 0x20-0x3F, 0x40-0x5F */
    int initialized;
};

static int obd_open_serial(const char *device) {
    int fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        dbg(lvl_warning, "Driver ECU OBD: Cannot open %s: %s", device, strerror(errno));
        return -1;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        dbg(lvl_warning, "Driver ECU OBD: tcgetattr failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    cfsetospeed(&tty, (speed_t)B115200);
    cfsetispeed(&tty, (speed_t)B115200);

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag = 0;
    tty.c_iflag = 0;
    tty.c_oflag = 0;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;

    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        dbg(lvl_warning, "Driver ECU OBD: tcsetattr failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

static int obd_write_line(int fd, const char *cmd) {
    size_t len = strlen(cmd);
    ssize_t written = write(fd, cmd, len);
    if (written < 0 || (size_t)written != len) {
        dbg(lvl_warning, "Driver ECU OBD: write '%s' failed: %s", cmd, strerror(errno));
        return 0;
    }
    return 1;
}

static int obd_read_line(int fd, char *buf, size_t buflen) {
    size_t pos = 0;
    char c;
    while (pos + 1 < buflen) {
        ssize_t n = read(fd, &c, 1);
        if (n <= 0) {
            break;
        }
        if (c == '>' || c == '\r' || c == '\n') {
            if (pos == 0)
                continue;
            break;
        }
        buf[pos++] = c;
    }
    buf[pos] = '\0';
    return (int)pos;
}

static int obd_send_query(int fd, const char *cmd, char *response, size_t response_len) {
    if (!obd_write_line(fd, cmd))
        return 0;

    if (!obd_write_line(fd, "\r"))
        return 0;

    int n = obd_read_line(fd, response, response_len);
    if (n <= 0) {
        dbg(lvl_debug, "Driver ECU OBD: Empty response for %s", cmd);
        return 0;
    }
    return 1;
}

static int obd_parse_hex_bytes(const char *s, unsigned char *out, int max_bytes) {
    int count = 0;
    while (*s && count < max_bytes) {
        while (*s == ' ')
            s++;
        if (!*s)
            break;
        char byte_str[3] = {0};
        byte_str[0] = s[0];
        if (!s[1])
            break;
        byte_str[1] = s[1];
        out[count++] = (unsigned char)strtoul(byte_str, NULL, 16);
        s += 2;
    }
    return count;
}

static void obd_query_pid_mask(struct driver_ecu_obd *obd, const char *cmd, int mask_index, int expected_mid) {
    char resp[128];
    unsigned char bytes[8];
    int n;

    if (!obd_send_query(obd->fd, cmd, resp, sizeof(resp))) {
        return;
    }

    n = obd_parse_hex_bytes(resp, bytes, 8);
    if (n >= 6 && bytes[0] == 0x41 && bytes[1] == (unsigned char)expected_mid) {
        unsigned int mask = (bytes[2] << 24) | (bytes[3] << 16) | (bytes[4] << 8) | bytes[5];
        obd->supported_pids[mask_index] = mask;
    }
}

static void obd_detect_supported_pids(struct driver_ecu_obd *obd) {
    memset(obd->supported_pids, 0, sizeof(obd->supported_pids));

    obd_query_pid_mask(obd, "0100", 0, 0x00);
    obd_query_pid_mask(obd, "0120", 1, 0x20);
    obd_query_pid_mask(obd, "0140", 2, 0x40);

    dbg(lvl_info, "Driver ECU OBD: Supported PID bitmasks: 0x%08x 0x%08x 0x%08x", obd->supported_pids[0],
        obd->supported_pids[1], obd->supported_pids[2]);
}

static int obd_pid_supported(struct driver_ecu_obd *obd, int pid) {
    if (pid < 0x01 || pid > 0x60)
        return 0;
    int idx = (pid - 1) / 0x20;
    int local = pid - idx * 0x20;
    unsigned int mask = obd->supported_pids[idx];
    return (mask & (1U << (32 - local))) != 0;
}

static double obd_maf_to_fuel_rate(double maf_g_s, int fuel_type, int ethanol_pct) {
    double afr = 14.7;
    double density = 0.745;

    if (fuel_type == DRIVER_ECU_FUEL_DIESEL) {
        afr = 14.5;
        density = 0.832;
    } else if (fuel_type == DRIVER_ECU_FUEL_FLEX || fuel_type == DRIVER_ECU_FUEL_ETHANOL) {
        double e = (double)ethanol_pct / 100.0;
        afr = 14.7 * (1.0 - e) + 9.8 * e;
        density = 0.745 * (1.0 - e) + 0.787 * e;
    }

    if (afr <= 0.0 || density <= 0.0 || maf_g_s <= 0.0)
        return 0.0;

    double fuel_kg_h = (maf_g_s / afr) * 3600.0 / 1000.0;
    return fuel_kg_h / density;
}

static void obd_poll(struct driver_ecu_obd *obd);

static void obd_poll_timeout(struct event_timeout *to, void *data) {
    (void)to;
    struct driver_ecu_obd *obd = (struct driver_ecu_obd *)data;
    obd_poll(obd);
}

static double obd_read_fuel_rate_pid(struct driver_ecu_obd *obd, double current_rate) {
    if (!obd_pid_supported(obd, 0x5E)) {
        return current_rate;
    }

    char resp[128];
    unsigned char bytes[8];
    int n;

    if (!obd_send_query(obd->fd, "015E", resp, sizeof(resp))) {
        return current_rate;
    }

    n = obd_parse_hex_bytes(resp, bytes, 8);
    if (n >= 5 && bytes[0] == 0x41 && bytes[1] == 0x5E) {
        unsigned int v = (bytes[2] << 8) | bytes[3];
        return (double)v * 0.05;
    }

    return current_rate;
}

static int obd_read_maf(struct driver_ecu_obd *obd, double *maf_g_s) {
    char resp[128];
    unsigned char bytes[8];
    int n;

    if (!obd_pid_supported(obd, 0x10)) {
        return 0;
    }
    if (!obd_send_query(obd->fd, "0110", resp, sizeof(resp))) {
        return 0;
    }

    n = obd_parse_hex_bytes(resp, bytes, 8);
    if (n >= 5 && bytes[0] == 0x41 && bytes[1] == 0x10) {
        unsigned int v = (bytes[2] << 8) | bytes[3];
        *maf_g_s = (double)v / 100.0;
        return 1;
    }
    return 0;
}

static void obd_read_ethanol(struct driver_ecu_obd *obd, int *ethanol_pct) {
    char resp[128];
    unsigned char bytes[8];
    int n;

    if (!obd_pid_supported(obd, 0x52)) {
        return;
    }
    if (!obd_send_query(obd->fd, "0152", resp, sizeof(resp))) {
        return;
    }

    n = obd_parse_hex_bytes(resp, bytes, 8);
    if (n >= 4 && bytes[0] == 0x41 && bytes[1] == 0x52) {
        int epct = (int)(((double)bytes[2] * 100.0) / 255.0 + 0.5);
        if (epct < 0)
            epct = 0;
        if (epct > 100)
            epct = 100;
        *ethanol_pct = epct;
    }
}

static int obd_read_maf_and_ethanol(struct driver_ecu_obd *obd, double *maf_g_s, int *ethanol_pct) {
    int have_maf = obd_read_maf(obd, maf_g_s);
    obd_read_ethanol(obd, ethanol_pct);
    return have_maf;
}

static void obd_update_tank_level(struct driver_ecu_obd *obd) {
    if (!obd_pid_supported(obd, 0x2F)) {
        return;
    }

    char resp[128];
    unsigned char bytes[8];
    int n;

    if (!obd_send_query(obd->fd, "012F", resp, sizeof(resp))) {
        return;
    }

    n = obd_parse_hex_bytes(resp, bytes, 8);
    if (n >= 4 && bytes[0] == 0x41 && bytes[1] == 0x2F && obd->fuel && obd->config
        && obd->config->fuel_tank_capacity_l > 0) {
        double pct = ((double)bytes[2] * 100.0) / 255.0;
        double capacity = (double)obd->config->fuel_tank_capacity_l;
        obd->fuel->fuel_current = (pct / 100.0) * capacity;
    }
}

static void obd_poll(struct driver_ecu_obd *obd) {
    if (!obd || obd->fd < 0 || !obd->fuel || !obd->config) {
        return;
    }

    double fuel_rate_l_h = obd->fuel->fuel_rate_l_h;
    double maf_g_s = 0.0;
    int maf_valid = 0;
    int ethanol_pct = obd->config->fuel_ethanol_manual_pct;

    fuel_rate_l_h = obd_read_fuel_rate_pid(obd, fuel_rate_l_h);

    maf_valid = obd_read_maf_and_ethanol(obd, &maf_g_s, &ethanol_pct);
    obd->config->fuel_ethanol_manual_pct = ethanol_pct;

    if (fuel_rate_l_h <= 0.0 && maf_valid) {
        fuel_rate_l_h = obd_maf_to_fuel_rate(maf_g_s, obd->config->fuel_type, ethanol_pct);
    }

    obd_update_tank_level(obd);

    if (fuel_rate_l_h > 0.0 && fuel_rate_l_h < 1000.0) {
        obd->fuel->fuel_rate_l_h = fuel_rate_l_h;
    }

    obd->poll_timeout = event_add_timeout(5000, 1, callback_new_1(callback_cast(obd_poll_timeout), obd));
}

struct driver_ecu_obd *driver_ecu_obd_start(const struct ecu_config *config, struct ecu_fuel_state *fuel) {
    if (!config || !fuel) {
        return NULL;
    }

    if (!config->fuel_obd_available) {
        return NULL;
    }

    struct driver_ecu_obd *obd = g_new0(struct driver_ecu_obd, 1);
    obd->config = (struct ecu_config *)config;
    obd->fuel = fuel;

    const char *device = "/dev/ttyUSB0";
    obd->fd = obd_open_serial(device);
    if (obd->fd < 0) {
        g_free(obd);
        return NULL;
    }

    obd_write_line(obd->fd, "ATZ\r");
    (void)obd_read_line(obd->fd, (char[64]){0}, 64);
    obd_write_line(obd->fd, "ATE0\r");
    (void)obd_read_line(obd->fd, (char[64]){0}, 64);
    obd_write_line(obd->fd, "ATL0\r");
    (void)obd_read_line(obd->fd, (char[64]){0}, 64);
    obd_write_line(obd->fd, "ATH0\r");
    (void)obd_read_line(obd->fd, (char[64]){0}, 64);
    obd_write_line(obd->fd, "ATS0\r");
    (void)obd_read_line(obd->fd, (char[64]){0}, 64);
    obd_write_line(obd->fd, "ATSP0\r");
    (void)obd_read_line(obd->fd, (char[64]){0}, 64);

    obd_detect_supported_pids(obd);
    obd->initialized = 1;

    obd->poll_timeout = event_add_timeout(3000, 1, callback_new_1(callback_cast(obd_poll_timeout), obd));

    dbg(lvl_info, "Driver ECU OBD: Backend started on %s", device);
    return obd;
}

void driver_ecu_obd_stop(struct driver_ecu_obd *obd) {
    if (!obd)
        return;
    if (obd->poll_timeout) {
        event_remove_timeout(obd->poll_timeout);
    }
    if (obd->fd >= 0) {
        close(obd->fd);
    }
    g_free(obd);
}
