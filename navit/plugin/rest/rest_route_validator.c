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

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include "debug.h"
#include "rest_route_validator.h"
#include "route.h"
#include "item.h"
#include "map.h"
#include "rest_poi_map.h"

struct item;

/* Forbidden highways for hikers */
static const char *forbidden_highways[] = {
    "motorway",
    "trunk",
    "primary",
    "primary_link",
    "motorway_link",
    "trunk_link",
    NULL
};

/* Priority paths for hikers (lowest cost) */
static const char *priority_paths[] = {
    "footway",
    "path",
    "track",
    "steps",
    "bridleway",
    NULL
};

/* Check if highway type is forbidden */
int route_validator_is_forbidden_highway(const char *highway_type) {
    if (!highway_type) return 0;
    
    for (int i = 0; forbidden_highways[i]; i++) {
        if (!strcmp(highway_type, forbidden_highways[i])) {
            return 1;
        }
    }
    return 0;
}

/* Check if highway type is a priority path */
int route_validator_is_priority_path(const char *highway_type) {
    if (!highway_type) return 0;
    
    for (int i = 0; priority_paths[i]; i++) {
        if (!strcmp(highway_type, priority_paths[i])) {
            return 1;
        }
    }
    return 0;
}

/* Validate hiking route */
struct route_validation_result *route_validator_validate_hiking(struct route *route) {
    return route_validator_validate_hiking_with_priority(route, 0);
}

/* Validate hiking route with pilgrimage/hiking priority check */
struct route_validation_result *route_validator_validate_hiking_with_priority(struct route *route, int enable_hiking_pilgrimage_priority) {
    struct route_validation_result *result = g_new0(struct route_validation_result, 1);
    result->warnings = NULL;
    result->is_valid = 1;
    
    if (!route) {
        result->is_valid = 0;
        result->warnings = g_list_append(result->warnings, g_strdup("Empty or null route"));
        return result;
    }
    
    /* Get route map */
    struct map *route_map = route_get_map(route);
    if (!route_map) {
        result->is_valid = 0;
        result->warnings = g_list_append(result->warnings, g_strdup("No route map available"));
        return result;
    }
    
    /* Iterate through route segments */
    struct map_rect *mr = map_rect_new(route_map, NULL);
    if (!mr) {
        result->is_valid = 0;
        result->warnings = g_list_append(result->warnings, g_strdup("Cannot access route map"));
        return result;
    }
    
    double total_length = 0;
    double path_length = 0;
    double forbidden_length = 0;
    double secondary_length = 0;
    
    struct item *item;
    struct attr attr;
    
    while ((item = map_rect_get_item(mr))) {
        /* Only process street route items */
        if (item->type != type_street_route) {
            continue;
        }
        
        /* Get segment length */
        if (item_attr_get(item, attr_length, &attr)) {
            double segment_length = (double)attr.u.num;  /* Length in meters */
            total_length += segment_length;
            
            /* Get the underlying street item to determine highway type */
            struct item *street_item = NULL;
            struct attr street_attr;
            if (item_attr_get(item, attr_street_item, &street_attr)) {
                street_item = street_attr.u.item;
            }
            
            if (street_item) {
                const char *highway_type = route_validator_map_item_to_highway_type(street_item);
                
                /* Classify segment */
                if (highway_type) {
                    if (route_validator_is_forbidden_highway(highway_type)) {
                        forbidden_length += segment_length;
                        result->forbidden_segments++;
                    } else if (route_validator_is_priority_path(highway_type)) {
                        path_length += segment_length;
                    } else {
                        secondary_length += segment_length;
                    }
                } else {
                    /* Unknown type - count as secondary */
                    secondary_length += segment_length;
                }
                
                /* Check for pilgrimage/hiking route tags if priority enabled */
                if (enable_hiking_pilgrimage_priority && street_item) {
                    if (rest_route_is_pilgrimage_hiking(street_item)) {
                        /* Prioritize hiking/pilgrimage routes - count as priority path */
                        path_length += segment_length;
                        /* Adjust secondary length if it was counted there */
                        if (secondary_length >= segment_length) {
                            secondary_length -= segment_length;
                        }
                    }
                }
            } else {
                /* No street item - count as secondary */
                secondary_length += segment_length;
            }
        }
    }
    
    map_rect_destroy(mr);
    
    /* Calculate percentages */
    if (total_length > 0) {
        result->path_percentage = (path_length / total_length) * 100.0;
        result->forbidden_percentage = (forbidden_length / total_length) * 100.0;
        result->secondary_percentage = (secondary_length / total_length) * 100.0;
    }
    
    /* Generate warnings */
    if (result->forbidden_percentage > 0) {
        char *warning = g_strdup_printf(
            "Route uses %.1f%% forbidden highways (motorways, trunk, primary roads)",
            result->forbidden_percentage
        );
        result->warnings = g_list_append(result->warnings, warning);
    }
    
    if (result->path_percentage < 50.0 && result->forbidden_percentage == 0) {
        char *warning = g_strdup_printf(
            "Only %.1f%% of route uses priority paths (footways, paths, tracks)",
            result->path_percentage
        );
        result->warnings = g_list_append(result->warnings, warning);
    }
    
    /* Route is valid if no forbidden highways */
    result->is_valid = (result->forbidden_percentage == 0);
    
    return result;
}

void route_validator_free_result(struct route_validation_result *result) {
    if (result) {
        if (result->warnings) {
            g_list_free_full(result->warnings, g_free);
        }
        g_free(result);
    }
}
