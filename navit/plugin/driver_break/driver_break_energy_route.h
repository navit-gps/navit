/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Plugin-local elevation-aware energy / kinetic route analysis.
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_ENERGY_ROUTE_H
#define NAVIT_PLUGIN_DRIVER_BREAK_ENERGY_ROUTE_H

#include "coord.h"

struct driver_break_priv;
struct route;
struct route_validation_result;
struct navit;

/* Haversine distance in meters between two WGS84 points. */
double driver_break_coord_distance_geo(const struct coord_geo *c1, const struct coord_geo *c2);

/* Bounding box of all route polyline coordinates (1 if bbox valid). */
int driver_break_route_bbox(struct route *route, double *min_lon, double *min_lat, double *max_lon, double *max_lat);

/* Warn when kinetic routing is enabled but SRTM tiles are missing for route or position. */
void driver_break_check_srtm_coverage(struct driver_break_priv *priv);

/* Accumulate energy/kinetic cost along the planned route; updates priv->route_energy_*. */
void driver_break_compute_route_energy(struct driver_break_priv *priv);

/* Post route validation warnings to the Navit message area. */
void driver_break_post_validation_warnings(struct navit *nav, struct route_validation_result *result);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_ENERGY_ROUTE_H */
