/**
 * Navit, a modular navigation system.
 *
 * J1939 backend for ECU fuel monitoring.
 *
 * Listens on SocketCAN (default can0) for:
 *   - PGN 65266 (FEEA) Engine fuel rate
 *   - PGN 65276 (FEF4) Fuel level
 */

#include "driver_ecu_j1939.h"

#include <errno.h>
#include <glib.h>

#include "callback.h"
#include "debug.h"
#include "event.h"
#include <linux/can.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

struct driver_ecu_j1939 {
    struct ecu_config config;
    struct ecu_fuel_state *fuel;
    int sock;
    struct event_idle *idle;
};

static unsigned int j1939_get_pgn(canid_t can_id) {
    unsigned int id = can_id & CAN_EFF_MASK;
    unsigned int pgn = (id >> 8) & 0xFFFF;
    return pgn;
}

static void j1939_handle_fuel_rate(struct ecu_fuel_state *fuel, const struct can_frame *frame) {
    if (frame->can_dlc < 8) {
        return;
    }
    unsigned int raw = frame->data[2] | (frame->data[3] << 8);
    if (raw == 0xFFFF) {
        return;
    }
    double rate_l_h = (double)raw * 0.05;
    if (rate_l_h > 0.0 && rate_l_h < 5000.0) {
        fuel->fuel_rate_l_h = rate_l_h;
    }
}

static void j1939_handle_fuel_level(struct ecu_config *config, struct ecu_fuel_state *fuel,
                                    const struct can_frame *frame) {
    if (frame->can_dlc < 8 || config->fuel_tank_capacity_l <= 0) {
        return;
    }
    unsigned int raw = frame->data[1];
    if (raw == 0xFF) {
        return;
    }
    double pct = (double)raw * 0.4;
    double capacity = (double)config->fuel_tank_capacity_l;
    fuel->fuel_current = (pct / 100.0) * capacity;
}

static void j1939_idle(struct driver_ecu_j1939 *ctx) {
    if (!ctx || ctx->sock < 0 || !ctx->fuel) {
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
    if (pgn == 0xFEEA) {
        j1939_handle_fuel_rate(ctx->fuel, &frame);
    } else if (pgn == 0xFEF4) {
        j1939_handle_fuel_level(&ctx->config, ctx->fuel, &frame);
    }
}

struct driver_ecu_j1939 *driver_ecu_j1939_start(const struct ecu_config *config, struct ecu_fuel_state *fuel) {
    if (!config || !fuel) {
        return NULL;
    }
    if (!config->fuel_j1939_available) {
        return NULL;
    }

    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        dbg(lvl_warning, "Driver ECU J1939: socket() failed: %s", strerror(errno));
        return NULL;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "can0");
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        dbg(lvl_warning, "Driver ECU J1939: SIOCGIFINDEX failed for can0: %s", strerror(errno));
        close(sock);
        return NULL;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        dbg(lvl_warning, "Driver ECU J1939: bind failed: %s", strerror(errno));
        close(sock);
        return NULL;
    }

    struct driver_ecu_j1939 *ctx = g_new0(struct driver_ecu_j1939, 1);
    ctx->config = *config;
    ctx->fuel = fuel;
    ctx->sock = sock;
    ctx->idle = event_add_idle(50, callback_new_1(callback_cast(j1939_idle), ctx));

    dbg(lvl_info, "Driver ECU J1939: Backend started on can0");
    return ctx;
}

void driver_ecu_j1939_stop(struct driver_ecu_j1939 *ctx) {
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
