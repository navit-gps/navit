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

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_POI_MAP_H
#define NAVIT_PLUGIN_DRIVER_BREAK_POI_MAP_H

#include "config.h"
#include "coord.h"
#include "driver_break.h"
#include "mapset.h"

/* Search for POIs in maps near a coordinate */
GList *driver_break_poi_map_search(struct coord_geo *center, double radius_km, enum item_type *poi_types, int num_types,
                                   struct mapset *ms);

/* Search for water points in maps */
GList *driver_break_poi_map_search_water(struct coord_geo *center, double radius_km, struct mapset *ms);

/* Search for cabins/huts in maps (with DNT/network detection) */
GList *driver_break_poi_map_search_cabins(struct coord_geo *center, double radius_km, struct mapset *ms);

/* Search for car POIs in maps (cafes, restaurants, museums, etc.) */
GList *driver_break_poi_map_search_car_pois(struct coord_geo *center, double radius_km, struct mapset *ms);

/* Check if POI item has DNT/network tags */
int driver_break_poi_is_network_cabin(struct item *item, char **network_name);

/* Check if route segment has pilgrimage/hiking route tags */
int driver_break_route_is_pilgrimage_hiking(struct item *item);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_POI_MAP_H */
