/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_POI_HIKING_H
#define NAVIT_PLUGIN_DRIVER_BREAK_POI_HIKING_H

#include "config.h"
#include "coord.h"
#include "driver_break.h"

/* POI search radius constants */
#define POI_WATER_SEARCH_RADIUS 2000.0 /* 2 km for water points */
#define POI_CABIN_SEARCH_RADIUS 5000.0 /* 5 km for cabins/huts */

/* Water point information */
struct water_point {
    struct coord_geo coord;
    char *name;
    char *type;                             /* "drinking_water", "fountain", "spring" */
    double distance_from_driver_break_stop; /* meters */
    int is_spring;                          /* 1 if natural spring (may need treatment) */
    char *warning;                          /* Warning message for springs */
};

/* Cabin/hut information */
struct cabin_hut {
    struct coord_geo coord;
    char *name;
    char *type;                             /* "wilderness_hut", "alpine_hut", "hut", "cabin" */
    double distance_from_driver_break_stop; /* meters */
    int is_dnt;                             /* 1 if DNT (Norwegian Trekking Association) hut */
    int is_network;                         /* 1 if network cabin (DNT, STF, etc.) */
    char *network;                          /* Network name (e.g., "DNT", "STF") */
    int locked;                             /* 1 if locked, 0 if open */
    int has_water;                          /* 1 if has water available */
};

/* Search for water points near rest stop */
GList *poi_search_water_points(struct coord_geo *driver_break_stop, double radius_km);

/* Search for water points near rest stop (with mapset) */
GList *poi_search_water_points_map(struct coord_geo *driver_break_stop, double radius_km, struct mapset *ms);

/* Search for cabins/huts near rest stop */
GList *poi_search_cabins(struct coord_geo *driver_break_stop, double radius_km);

/* Search for cabins/huts near rest stop (with mapset and DNT priority) */
GList *poi_search_cabins_map(struct coord_geo *driver_break_stop, double radius_km, struct mapset *ms,
                             int enable_dnt_priority);

/* Free water point list */
void poi_free_water_points(GList *water_points);

/* Free cabin list */
void poi_free_cabins(GList *cabins);

/* Free single water point */
void poi_free_water_point(struct water_point *wp);

/* Free single cabin */
void poi_free_cabin(struct cabin_hut *cabin);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_POI_HIKING_H */
