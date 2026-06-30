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

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_ROUTE_VALIDATOR_H
#define NAVIT_PLUGIN_DRIVER_BREAK_ROUTE_VALIDATOR_H

#include "config.h"
#include "driver_break.h"
#include "route.h"

/* Route validation result */
struct route_validation_result {
    double path_percentage;      /* Percentage of route on priority paths */
    double forbidden_percentage; /* Percentage of route on forbidden highways */
    double secondary_percentage; /* Percentage on secondary roads */
    int forbidden_segments;      /* Number of forbidden segments */
    GList *warnings;             /* Warning messages */
    int is_valid;                /* Overall validity */
};

/* Validate hiking route against forbidden highways */
struct route_validation_result *route_validator_validate_hiking(struct route *route);

/* Validate hiking route with pilgrimage/hiking priority check */
struct route_validation_result *route_validator_validate_hiking_with_priority(struct route *route,
                                                                              int enable_hiking_pilgrimage_priority);

/* Check if highway type is forbidden for hikers */
int route_validator_is_forbidden_highway(const char *highway_type);

/* Check if highway type is a priority path for hikers */
int route_validator_is_priority_path(const char *highway_type);

/* Map Navit item types to OSM highway types */
const char *route_validator_map_item_to_highway_type(struct item *street_item);

/* Free validation result */
void route_validator_free_result(struct route_validation_result *result);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_ROUTE_VALIDATOR_H */
