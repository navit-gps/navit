/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_ROUTE_STOPS_H
#define NAVIT_PLUGIN_DRIVER_BREAK_ROUTE_STOPS_H

#include "driver_break.h"

void driver_break_process_hiking_stops(struct driver_break_priv *priv, GList *hiking_stops);
void driver_break_process_cycling_stops(struct driver_break_priv *priv, GList *cycling_stops);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_ROUTE_STOPS_H */
