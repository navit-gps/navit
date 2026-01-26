/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024 Navit Team
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

#ifndef NAVIT_PLUGIN_REST_FINDER_H
#define NAVIT_PLUGIN_REST_FINDER_H

#include "config.h"
#include "coord.h"
#include "route.h"
#include "rest.h"

/* Find rest stops along a route */
GList *rest_finder_find_along_route(struct route *route, 
                                     struct rest_config *config,
                                     int mandatory_break_required);

/* Find rest stops near a coordinate */
GList *rest_finder_find_near(struct coord_geo *center, 
                              double distance_km,
                              struct rest_config *config);

/* Check if location meets rest stop criteria */
int rest_finder_is_valid_location(struct coord_geo *coord, 
                                  struct rest_config *config);

/* Check if location is valid for nightly rest stop (with glacier check) */
int rest_finder_is_valid_nightly_location(struct coord_geo *coord, 
                                            struct rest_config *config,
                                            struct mapset *ms,
                                            int has_camping_building);

/* Free rest stop list */
void rest_finder_free_list(GList *stops);

/* Free single rest stop */
void rest_finder_free(struct rest_stop *stop);

#endif /* NAVIT_PLUGIN_REST_FINDER_H */
