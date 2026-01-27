/**
 * @file driver_break_finder.h
 * @brief Rest stop finder API
 *
 * Functions for finding suitable rest stop locations along routes
 * and validating locations against rest stop criteria.
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

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_FINDER_H
#define NAVIT_PLUGIN_DRIVER_BREAK_FINDER_H

#include "config.h"
#include "coord.h"
#include "route.h"
#include "rest.h"

/**
 * @brief Find rest stops along a route
 * @param route Route to search along
 * @param config Rest configuration
 * @param mandatory_break_required 1 if mandatory break is required
 * @return GList of struct driver_break_stop* (caller must free with driver_break_finder_free_list)
 */
GList *driver_break_finder_find_along_route(struct route *route, 
                                     struct driver_break_config *config,
                                     int mandatory_break_required);

/**
 * @brief Find rest stops near a coordinate
 * @param center Center coordinate for search
 * @param distance_km Search radius in kilometers
 * @param config Rest configuration
 * @return GList of struct driver_break_stop* (caller must free with driver_break_finder_free_list)
 */
GList *driver_break_finder_find_near(struct coord_geo *center, 
                              double distance_km,
                              struct driver_break_config *config);

/**
 * @brief Check if location meets rest stop criteria
 * @param coord Location to validate
 * @param config Rest configuration
 * @return 1 if valid, 0 otherwise
 */
int driver_break_finder_is_valid_location(struct coord_geo *coord, 
                                  struct driver_break_config *config);

/**
 * @brief Check if location is valid for nightly rest stop (with glacier check)
 * @param coord Location to validate
 * @param config Rest configuration
 * @param ms Mapset for map queries
 * @param has_camping_building 1 if location has camping building
 * @return 1 if valid, 0 otherwise
 */
int driver_break_finder_is_valid_nightly_location(struct coord_geo *coord, 
                                            struct driver_break_config *config,
                                            struct mapset *ms,
                                            int has_camping_building);

/**
 * @brief Free a list of rest stops
 * @param stops GList of struct driver_break_stop* to free
 */
void driver_break_finder_free_list(GList *stops);

/**
 * @brief Free a single rest stop structure
 * @param stop Rest stop to free
 */
void driver_break_finder_free(struct driver_break_stop *stop);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_FINDER_H */
