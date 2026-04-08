/**
 * Navit, a modular navigation system.
 *
 * J1939 backend for Driver Break plugin.
 *
 * Listens on SocketCAN (default can0) for:
 *   - PGN 65266 (FEEA) Engine fuel rate
 *   - PGN 65276 (FEF4) Fuel level
 *
 * Scaling (SAE J1939-71, common implementations):
 *   - Engine fuel rate SPN 183: 0.05 L/h per bit, 0 = not available.
 *   - Fuel level SPN 96: 0.4 % per bit, 0-250 %.
 */

#include "driver_break_j1939.h"
#include "callback.h"
#include "debug.h"
#include "event.h"
#include <errno.h>
#include <glib.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

struct driver_break_j1939 {
    struct driver_break_priv *priv;
    int sock;
    struct event_idle *idle;
};

/* Extract PGN from 29-bit CAN ID (J1939). */
static unsigned int j1939_get_pgn(canid_t can_id) {
    unsigned int id = can_id & CAN_EFF_MASK;
    unsigned int pgn = (id >> 8) & 0xFFFF;
    return pgn;
}

static void j1939_idle(struct driver_break_j1939 *ctx) {
    if (!ctx || ctx->sock < 0 || !ctx->priv) {
        return;
    }

    struct can_frame frame;
    ssize_t n = read(ctx->sock, &frame, sizeof(frame));
    if (n < (ssize_t)sizeof(struct can_frame)) {
        return;
    }

    if (!(frame.can_id & CAN_EFF_FLAG)) {
        return;
    }

    unsigned int pgn = j1939_get_pgn(frame.can_id);
    struct driver_break_priv *priv = ctx->priv;

    /* PGN 65266 (FEEA) – Engine fuel rate */
    if (pgn == 0xFEEA && frame.can_dlc >= 8) {
        unsigned int raw = frame.data[2] | (frame.data[3] << 8);
        if (raw != 0xFFFF) {
            double rate_l_h = (double)raw * 0.05;
            if (rate_l_h > 0.0 && rate_l_h < 5000.0) {
                priv->fuel_rate_l_h = rate_l_h;
            }
        }
    }

    /* PGN 65276 (FEF4) – Fuel level */
    if (pgn == 0xFEF4 && frame.can_dlc >= 8) {
        unsigned int raw = frame.data[1];
        if (raw != 0xFF) {
            double pct = (double)raw * 0.4;
            if (priv->config.fuel_tank_capacity_l > 0) {
                double capacity = (double)priv->config.fuel_tank_capacity_l;
                priv->fuel_current = (pct / 100.0) * capacity;
            }
        }
    }
}

struct driver_break_j1939 *driver_break_j1939_start(struct driver_break_priv *priv) {
    if (!priv) {
        return NULL;
    }
    if (!priv->config.fuel_j1939_available) {
        return NULL;
    }

    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        dbg(lvl_warning, "Driver Break J1939: socket() failed: %s", strerror(errno));
        return NULL;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "can0");
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        dbg(lvl_warning, "Driver Break J1939: SIOCGIFINDEX failed for can0: %s", strerror(errno));
        close(sock);
        return NULL;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        dbg(lvl_warning, "Driver Break J1939: bind failed: %s", strerror(errno));
        close(sock);
        return NULL;
    }

    /* No filter: receive all frames and filter by PGN in user space. */

    struct driver_break_j1939 *ctx = g_new0(struct driver_break_j1939, 1);
    ctx->priv = priv;
    ctx->sock = sock;
    ctx->idle = event_add_idle(50, callback_new_1(callback_cast(j1939_idle), ctx));

    dbg(lvl_info, "Driver Break J1939: Backend started on can0");
    return ctx;
}

void driver_break_j1939_stop(struct driver_break_j1939 *ctx) {
    if (!ctx)
        return;
    if (ctx->idle) {
        event_remove_idle(ctx->idle);
    }
    if (ctx->sock >= 0) {
        close(ctx->sock);
    }
    g_free(ctx);
}
