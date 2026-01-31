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

#include "driver_break_route_validator.h"
#include "attr.h"
#include "config.h"
#include "debug.h"
#include "driver_break_poi_map.h"
#include "item.h"
#include "map.h"
#include "route.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>

struct item;

/* Extract highway type from street item OSM tags (caller must not free - points to static buffer) */
static const char *get_highway_from_item(struct item *item) {
    static char highway_buf[64];
    struct attr attr;
    highway_buf[0] = '\0';
    if (!item_attr_get(item, attr_osm_tag, &attr) || !attr.u.str)
        return NULL;
    const char *tags = attr.u.str;
    const char *pos = strstr(tags, "highway=");
    if (!pos)
        return NULL;
    pos += 8; /* strlen("highway=") */
    const char *end = strchr(pos, ' ');
    if (!end)
        end = pos + strlen(pos);
    size_t len = end - pos;
    if (len >= sizeof(highway_buf))
        len = sizeof(highway_buf) - 1;
    memcpy(highway_buf, pos, len);
    highway_buf[len] = '\0';
    return highway_buf[0] ? highway_buf : NULL;
}

const char *route_validator_map_item_to_highway_type(struct item *street_item) {
    if (!street_item)
        return NULL;
    return get_highway_from_item(street_item);
}

/* Forbidden highways for hikers */
static const char *forbidden_highways[] = {"motorway",      "trunk",      "primary", "primary_link",
                                           "motorway_link", "trunk_link", NULL};

/* Priority paths for hikers (lowest cost) */
static const char *priority_paths[] = {"footway", "path", "track", "steps", "bridleway", NULL};

/* Check if highway type is forbidden */
int route_validator_is_forbidden_highway(const char *highway_type) {
    if (!highway_type)
        return 0;

    int i;
    for (i = 0; forbidden_highways[i]; i++) {
        if (!strcmp(highway_type, forbidden_highways[i])) {
            return 1;
        }
    }
    return 0;
}

/* Check if highway type is a priority path */
int route_validator_is_priority_path(const char *highway_type) {
    if (!highway_type)
        return 0;

    int i;
    for (i = 0; priority_paths[i]; i++) {
        if (!strcmp(highway_type, priority_paths[i])) {
            return 1;
        }
    }
    return 0;
}

/* Segment category: 0=forbidden, 1=path, 2=secondary */
static int classify_segment(struct item *street_item) {
    if (!street_item) {
        return 2; /* secondary */
    }
    const char *highway_type = route_validator_map_item_to_highway_type(street_item);
    if (!highway_type) {
        return 2;
    }
    if (route_validator_is_forbidden_highway(highway_type)) {
        return 0;
    }
    if (route_validator_is_priority_path(highway_type)) {
        return 1;
    }
    return 2;
}

/* Apply pilgrimage adjustment: if segment was secondary and is pilgrimage, move to path */
static void apply_pilgrimage_adjustment(struct item *street_item, int enable, int was_secondary, double segment_length,
                                        double *path_length, double *secondary_length) {
    if (!enable || !was_secondary || !street_item || !driver_break_route_is_pilgrimage_hiking(street_item)) {
        return;
    }
    *path_length += segment_length;
    if (*secondary_length >= segment_length) {
        *secondary_length -= segment_length;
    }
}

/* Add validation warnings to result */
static void add_route_warnings(struct route_validation_result *result) {
    if (result->forbidden_percentage > 0) {
        char *w = g_strdup_printf("Route uses %.1f%% forbidden highways (motorways, trunk, primary roads)",
                                  result->forbidden_percentage);
        result->warnings = g_list_append(result->warnings, w);
    }
    if (result->path_percentage < 50.0 && result->forbidden_percentage == 0) {
        char *w = g_strdup_printf("Only %.1f%% of route uses priority paths (footways, paths, tracks)",
                                  result->path_percentage);
        result->warnings = g_list_append(result->warnings, w);
    }
}

/* Validate hiking route */
struct route_validation_result *route_validator_validate_hiking(struct route *route) {
    return route_validator_validate_hiking_with_priority(route, 0);
}

/* Validate hiking route with pilgrimage/hiking priority check */
struct route_validation_result *route_validator_validate_hiking_with_priority(struct route *route,
                                                                              int enable_hiking_pilgrimage_priority) {
    struct route_validation_result *result = g_new0(struct route_validation_result, 1);
    result->warnings = NULL;
    result->is_valid = 1;

    if (!route) {
        result->is_valid = 0;
        result->warnings = g_list_append(result->warnings, g_strdup("Empty or null route"));
        return result;
    }

    struct map *route_map = route_get_map(route);
    if (!route_map) {
        result->is_valid = 0;
        result->warnings = g_list_append(result->warnings, g_strdup("No route map available"));
        return result;
    }

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
        if (item->type != type_street_route) {
            continue;
        }

        if (!item_attr_get(item, attr_length, &attr)) {
            continue;
        }

        double segment_length = (double)attr.u.num;
        total_length += segment_length;

        struct item *street_item = NULL;
        struct attr street_attr;
        if (item_attr_get(item, attr_street_item, &street_attr)) {
            street_item = street_attr.u.item;
        }

        int cat = classify_segment(street_item);
        if (cat == 0) {
            forbidden_length += segment_length;
            result->forbidden_segments++;
        } else if (cat == 1) {
            path_length += segment_length;
        } else {
            secondary_length += segment_length;
        }

        apply_pilgrimage_adjustment(street_item, enable_hiking_pilgrimage_priority, (cat == 2), segment_length,
                                    &path_length, &secondary_length);
    }

    map_rect_destroy(mr);

    if (total_length > 0) {
        result->path_percentage = (path_length / total_length) * 100.0;
        result->forbidden_percentage = (forbidden_length / total_length) * 100.0;
        result->secondary_percentage = (secondary_length / total_length) * 100.0;
    }

    add_route_warnings(result);
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
