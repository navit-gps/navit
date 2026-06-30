/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_FUEL_CORE_H
#define NAVIT_PLUGIN_DRIVER_BREAK_FUEL_CORE_H

#include "driver_break.h"

void driver_break_update_fuel_learning(struct driver_break_priv *priv, struct coord_geo *pos, time_t now);
void driver_break_check_fuel_low_range(struct driver_break_priv *priv);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_FUEL_CORE_H */
