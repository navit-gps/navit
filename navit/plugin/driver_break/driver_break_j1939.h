/**
 * Navit, a modular navigation system.
 *
 * J1939 backend for Driver Break plugin (truck mode).
 *
 * This module listens on a SocketCAN interface (default can0) for:
 *   - PGN 65266 (FEEA) Engine Fuel Rate
 *   - PGN 65276 (FEF4) Fuel Level
 *
 * It translates these signals into:
 *   - fuel_rate_l_h
 *   - fuel_current (via tank level % and configured tank capacity)
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_J1939_H
#define NAVIT_PLUGIN_DRIVER_BREAK_J1939_H

#include "config.h"
#include "driver_break.h"

struct driver_break_j1939;

struct driver_break_j1939 *driver_break_j1939_start(struct driver_break_priv *priv);

void driver_break_j1939_stop(struct driver_break_j1939 *ctx);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_J1939_H */

