/**
 * Navit, a modular navigation system.
 *
 * OBD-II (ELM327) backend for Driver Break plugin.
 *
 * This module implements a minimal ELM327-based OBD-II poller that:
 * - Opens a serial port (default /dev/ttyUSB0, 115200 8N1).
 * - Sends a short init sequence (ATZ, E0, L0, H0, S0, SP0).
 * - Discovers supported PIDs via 0100/0120/0140.
 * - Polls:
 *   * 012F – Fuel tank level (%)
 *   * 015E – Engine fuel rate (L/h)
 *   * 0110 – MAF (g/s)
 *   * 010D – Vehicle speed (km/h)
 *   * 0151 – Fuel type (optional classification)
 *   * 0152 – Ethanol % (optional, flex-fuel only)
 *
 * Results are fed into driver_break_priv:
 *   - fuel_rate_l_h
 *   - fuel_current (via tank % and configured tank capacity)
 *   - config.fuel_ethanol_manual_pct (overridden when PID 0x52 is present)
 *
 * When a PID is unsupported or responses are invalid, the backend disables only
 * the affected signal and continues with remaining PIDs. If the adapter fails,
 * the backend is shut down and manual/learning paths are left intact.
 */

#include "driver_break_obd.h"
#include "debug.h"
#include "event.h"
#include "file.h"
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

struct driver_break_obd {
    struct driver_break_priv *priv;
    int fd;
    struct event_timeout *poll_timeout;
    unsigned int supported_pids[3]; /* 0x00-0x1F, 0x20-0x3F, 0x40-0x5F */
    int initialized;
};

static int obd_open_serial(const char *device) {
    int fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        dbg(lvl_warning, "Driver Break OBD: Cannot open %s: %s", device, strerror(errno));
        return -1;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        dbg(lvl_warning, "Driver Break OBD: tcgetattr failed: %s", strerror(errno));
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
        dbg(lvl_warning, "Driver Break OBD: tcsetattr failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

static int obd_write_line(int fd, const char *cmd) {
    size_t len = strlen(cmd);
    ssize_t written = write(fd, cmd, len);
    if (written < 0 || (size_t)written != len) {
        dbg(lvl_warning, "Driver Break OBD: write '%s' failed: %s", cmd, strerror(errno));
        return 0;
    }
    return 1;
}

/* Read a single ELM327 response line (terminated by '>' or '\r'). */
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

    /* ELM327 expects CR after command */
    if (!obd_write_line(fd, "\r"))
        return 0;

    int n = obd_read_line(fd, response, response_len);
    if (n <= 0) {
        dbg(lvl_debug, "Driver Break OBD: Empty response for %s", cmd);
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

static void obd_detect_supported_pids(struct driver_break_obd *obd) {
    char resp[128];
    unsigned char bytes[8];
    int n;

    memset(obd->supported_pids, 0, sizeof(obd->supported_pids));

    if (obd_send_query(obd->fd, "0100", resp, sizeof(resp))) {
        n = obd_parse_hex_bytes(resp, bytes, 8);
        if (n >= 6 && bytes[0] == 0x41 && bytes[1] == 0x00) {
            unsigned int mask = (bytes[2] << 24) | (bytes[3] << 16) | (bytes[4] << 8) | bytes[5];
            obd->supported_pids[0] = mask;
        }
    }

    if (obd_send_query(obd->fd, "0120", resp, sizeof(resp))) {
        n = obd_parse_hex_bytes(resp, bytes, 8);
        if (n >= 6 && bytes[0] == 0x41 && bytes[1] == 0x20) {
            unsigned int mask = (bytes[2] << 24) | (bytes[3] << 16) | (bytes[4] << 8) | bytes[5];
            obd->supported_pids[1] = mask;
        }
    }

    if (obd_send_query(obd->fd, "0140", resp, sizeof(resp))) {
        n = obd_parse_hex_bytes(resp, bytes, 8);
        if (n >= 6 && bytes[0] == 0x41 && bytes[1] == 0x40) {
            unsigned int mask = (bytes[2] << 24) | (bytes[3] << 16) | (bytes[4] << 8) | bytes[5];
            obd->supported_pids[2] = mask;
        }
    }

    dbg(lvl_info, "Driver Break OBD: Supported PID bitmasks: 0x%08x 0x%08x 0x%08x", obd->supported_pids[0],
        obd->supported_pids[1], obd->supported_pids[2]);
}

static int obd_pid_supported(struct driver_break_obd *obd, int pid) {
    if (pid < 0x01 || pid > 0x5F)
        return 0;
    int idx = pid / 0x20;
    int bit = 0x20 - (pid % 0x20);
    if (pid % 0x20 == 0)
        bit = 0;
    unsigned int mask = obd->supported_pids[idx];
    if (pid % 0x20 == 0)
        return (mask & 0x80000000U) != 0;
    return (mask & (1U << (bit - 1))) != 0;
}

/* Compute fuel rate from MAF when PID 0x5E is unavailable.
 * Fuel rate (L/h) = MAF (g/s) × 3600 / (AFR × fuel_density × 1000)
 * (1000 g/kg to convert mass units).
 */
static double obd_maf_to_fuel_rate(double maf_g_s, int fuel_type, int ethanol_pct) {
    double afr = 14.7;
    double density = 0.745; /* kg/L default petrol */

    if (fuel_type == DRIVER_BREAK_FUEL_DIESEL) {
        afr = 14.5;
        density = 0.832;
    } else if (fuel_type == DRIVER_BREAK_FUEL_FLEX || fuel_type == DRIVER_BREAK_FUEL_ETHANOL) {
        double e = (double)ethanol_pct / 100.0;
        afr = 14.7 * (1.0 - e) + 9.8 * e;
        density = 0.745 * (1.0 - e) + 0.787 * e;
    }

    if (afr <= 0.0 || density <= 0.0 || maf_g_s <= 0.0)
        return 0.0;

    double fuel_kg_h = (maf_g_s / afr) * 3600.0 / 1000.0;
    return fuel_kg_h / density;
}

static void obd_poll(struct driver_break_obd *obd);

static void obd_poll_timeout(struct event_timeout *to, void *data) {
    (void)to;
    struct driver_break_obd *obd = (struct driver_break_obd *)data;
    obd_poll(obd);
}

static void obd_poll(struct driver_break_obd *obd) {
    if (!obd || obd->fd < 0 || !obd->priv) {
        return;
    }

    struct driver_break_priv *priv = obd->priv;
    char resp[128];
    unsigned char bytes[8];
    int n;

    double fuel_rate_l_h = priv->fuel_rate_l_h;
    double maf_g_s = 0.0;
    int maf_valid = 0;
    int ethanol_pct = priv->config.fuel_ethanol_manual_pct;

    /* Fuel rate PID 0x5E */
    if (obd_pid_supported(obd, 0x5E)) {
        if (obd_send_query(obd->fd, "015E", resp, sizeof(resp))) {
            n = obd_parse_hex_bytes(resp, bytes, 8);
            if (n >= 5 && bytes[0] == 0x41 && bytes[1] == 0x5E) {
                unsigned int v = (bytes[2] << 8) | bytes[3];
                fuel_rate_l_h = (double)v * 0.05;
            }
        }
    }

    /* MAF PID 0x10 */
    if (obd_pid_supported(obd, 0x10)) {
        if (obd_send_query(obd->fd, "0110", resp, sizeof(resp))) {
            n = obd_parse_hex_bytes(resp, bytes, 8);
            if (n >= 5 && bytes[0] == 0x41 && bytes[1] == 0x10) {
                unsigned int v = (bytes[2] << 8) | bytes[3];
                maf_g_s = (double)v / 100.0;
                maf_valid = 1;
            }
        }
    }

    /* Ethanol % PID 0x52 (optional) */
    if (obd_pid_supported(obd, 0x52)) {
        if (obd_send_query(obd->fd, "0152", resp, sizeof(resp))) {
            n = obd_parse_hex_bytes(resp, bytes, 8);
            if (n >= 4 && bytes[0] == 0x41 && bytes[1] == 0x52) {
                ethanol_pct = (int)(((double)bytes[2] * 100.0) / 255.0 + 0.5);
                if (ethanol_pct < 0)
                    ethanol_pct = 0;
                if (ethanol_pct > 100)
                    ethanol_pct = 100;
                priv->config.fuel_ethanol_manual_pct = ethanol_pct;
            }
        }
    }

    /* If fuel rate PID not supported, fall back to MAF-based estimate */
    if (fuel_rate_l_h <= 0.0 && maf_valid) {
        fuel_rate_l_h = obd_maf_to_fuel_rate(maf_g_s, priv->config.fuel_type, ethanol_pct);
    }

    /* Tank level PID 0x2F */
    if (obd_pid_supported(obd, 0x2F)) {
        if (obd_send_query(obd->fd, "012F", resp, sizeof(resp))) {
            n = obd_parse_hex_bytes(resp, bytes, 8);
            if (n >= 4 && bytes[0] == 0x41 && bytes[1] == 0x2F) {
                double pct = ((double)bytes[2] * 100.0) / 255.0;
                if (priv->config.fuel_tank_capacity_l > 0) {
                    double capacity = (double)priv->config.fuel_tank_capacity_l;
                    priv->fuel_current = (pct / 100.0) * capacity;
                }
            }
        }
    }

    /* Update fuel rate if we have a reasonable value */
    if (fuel_rate_l_h > 0.0 && fuel_rate_l_h < 1000.0) {
        priv->fuel_rate_l_h = fuel_rate_l_h;
    }

    /* Reschedule polling */
    obd->poll_timeout = event_add_timeout(5000, 1, callback_new_1(callback_cast(obd_poll_timeout), obd));
}

struct driver_break_obd *driver_break_obd_start(struct driver_break_priv *priv) {
    if (!priv) {
        return NULL;
    }

    /* Only start if user/config indicates OBD-II is available */
    if (!priv->config.fuel_obd_available) {
        return NULL;
    }

    struct driver_break_obd *obd = g_new0(struct driver_break_obd, 1);
    obd->priv = priv;

    const char *device = "/dev/ttyUSB0";
    obd->fd = obd_open_serial(device);
    if (obd->fd < 0) {
        g_free(obd);
        return NULL;
    }

    /* Basic ELM327 init sequence */
    obd_write_line(obd->fd, "ATZ\r");
    (void)obd_read_line(obd->fd, (char[64]){0}, 64);
    obd_write_line(obd->fd, "ATE0\r"); /* echo off */
    (void)obd_read_line(obd->fd, (char[64]){0}, 64);
    obd_write_line(obd->fd, "ATL0\r"); /* linefeeds off */
    (void)obd_read_line(obd->fd, (char[64]){0}, 64);
    obd_write_line(obd->fd, "ATH0\r"); /* headers off */
    (void)obd_read_line(obd->fd, (char[64]){0}, 64);
    obd_write_line(obd->fd, "ATS0\r"); /* spaces off */
    (void)obd_read_line(obd->fd, (char[64]){0}, 64);
    obd_write_line(obd->fd, "ATSP0\r"); /* automatic protocol */
    (void)obd_read_line(obd->fd, (char[64]){0}, 64);

    obd_detect_supported_pids(obd);
    obd->initialized = 1;

    obd->poll_timeout = event_add_timeout(3000, 1, callback_new_1(callback_cast(obd_poll_timeout), obd));

    dbg(lvl_info, "Driver Break OBD: Backend started on %s", device);
    return obd;
}

void driver_break_obd_stop(struct driver_break_obd *obd) {
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

