/**
 * @file driver_break_poi.h
 * @brief POI discovery API
 *
 * Functions for discovering Points of Interest near rest stops
 * using map-based queries or Overpass API fallback.
 * For detailed API documentation, see https://doxygen.navit-project.org/
 *
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

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_POI_H
#define NAVIT_PLUGIN_DRIVER_BREAK_POI_H

#include "config.h"
#include "coord.h"
#include "driver_break.h"

/**
 * @brief Discover POIs near a location
 * @param center Center coordinate for search
 * @param radius_km Search radius in kilometers
 * @param poi_categories Array of POI category strings (e.g., "amenity=cafe")
 * @param num_categories Number of categories in array
 * @return GList of struct driver_break_poi* (caller must free with driver_break_poi_free_list)
 *
 * Uses map-based queries (preferred) or Overpass API fallback if map data unavailable.
 */
GList *driver_break_poi_discover(struct coord_geo *center, int radius_km, 
                         const char **poi_categories, int num_categories);

/**
 * @brief Rank POIs by distance and preferences
 * @param pois GList of struct driver_break_poi* to rank (modified in-place)
 * @param driver_break_stop Rest stop location for distance calculation
 * @param config Rest configuration
 */
void driver_break_poi_rank(GList *pois, struct coord_geo *driver_break_stop, 
                   struct driver_break_config *config);

/**
 * @brief Free a list of POIs
 * @param pois GList of struct driver_break_poi* to free
 */
void driver_break_poi_free_list(GList *pois);

/**
 * @brief Free a single POI structure
 * @param poi POI to free
 */
void driver_break_poi_free(struct driver_break_poi *poi);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_POI_H */
