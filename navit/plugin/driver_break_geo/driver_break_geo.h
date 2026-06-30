/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Shared geodesy helpers for the Driver Break plugin.
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_GEO_H
#define NAVIT_PLUGIN_DRIVER_BREAK_GEO_H

#include "coord.h"

struct route;

/* Haversine distance in meters between two WGS84 points. */
double driver_break_coord_distance_geo(const struct coord_geo *c1, const struct coord_geo *c2);

/* Bounding box of all route polyline coordinates (1 if bbox valid). */
int driver_break_route_bbox(struct route *route, double *min_lon, double *min_lat, double *max_lon, double *max_lat);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_GEO_H */
