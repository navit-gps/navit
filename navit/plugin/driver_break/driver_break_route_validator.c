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

/* Copy OSM tag value for key into static buffer; returns NULL if key missing or buffer too small */
static const char *get_osm_tag_value_item(struct item *item, const char *key, char *buf, size_t buf_size) {
    struct attr attr;
    if (!item || !key || !buf || buf_size == 0)
        return NULL;
    if (!item_attr_get(item, attr_osm_tag, &attr) || !attr.u.str)
        return NULL;
    size_t key_len = strlen(key);
    const char *tags = attr.u.str;
    const char *pos = tags;
    for (;;) {
        pos = strstr(pos, key);
        if (!pos)
            return NULL;
        if ((pos == tags || pos[-1] == ' ') && pos[key_len] == '=') {
            pos += key_len + 1;
            const char *end = strchr(pos, ' ');
            size_t len = end ? (size_t)(end - pos) : strlen(pos);
            if (len >= buf_size)
                return NULL;
            memcpy(buf, pos, len);
            buf[len] = '\0';
            return buf;
        }
        pos += key_len;
    }
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

int route_validator_is_cycling_priority_highway(const char *highway_type) {
    return highway_type && !strcmp(highway_type, "cycleway");
}

/* mtb:scale / mtb:scale:uphill: leading digit 0-6 (0+) style tags use first digit */
static int parse_mtb_scale_digit(const char *s) {
    if (!s || !*s)
        return -1;
    if (s[0] >= '0' && s[0] <= '6')
        return (int)(s[0] - '0');
    return -1;
}

static void update_mtb_max_from_item(struct item *street_item, int *mtb_scale_max, int *mtb_uphill_max) {
    char buf[32];
    int v;
    if (!street_item || !mtb_scale_max || !mtb_uphill_max)
        return;
    if (get_osm_tag_value_item(street_item, "mtb:scale", buf, sizeof(buf))) {
        v = parse_mtb_scale_digit(buf);
        if (v > *mtb_scale_max)
            *mtb_scale_max = v;
    }
    if (get_osm_tag_value_item(street_item, "mtb:scale:uphill", buf, sizeof(buf))) {
        v = parse_mtb_scale_digit(buf);
        if (v > *mtb_uphill_max)
            *mtb_uphill_max = v;
    }
}

/* Cycle-priority: highway=cycleway, or bicycle=yes/designated on path-like highways, or signed cycle network tags */
static int route_validator_cycling_priority_segment(struct item *street_item) {
    char bicycle[32];
    const char *hw;

    if (!street_item)
        return 0;
    hw = route_validator_map_item_to_highway_type(street_item);
    if (hw && !strcmp(hw, "cycleway"))
        return 1;
    if (driver_break_route_is_cycle_network_way(street_item))
        return 1;
    if (!get_osm_tag_value_item(street_item, "bicycle", bicycle, sizeof(bicycle)))
        return 0;
    if (strcmp(bicycle, "yes") != 0 && strcmp(bicycle, "designated") != 0)
        return 0;
    if (!hw)
        return 0;
    if (!strcmp(hw, "path") || !strcmp(hw, "footway") || !strcmp(hw, "track") || !strcmp(hw, "living_street")
        || !strcmp(hw, "pedestrian") || !strcmp(hw, "service")) {
        return 1;
    }
    return 0;
}

static void classify_cycling_segment(double segment_length, struct item *street_item, double *path_length,
                                     double *forbidden_length, double *secondary_length,
                                     struct route_validation_result *result) {
    int secondary_for_pilgrimage = 0;

    if (!street_item) {
        *secondary_length += segment_length;
        return;
    }

    const char *highway_type = route_validator_map_item_to_highway_type(street_item);
    if (highway_type) {
        if (route_validator_is_forbidden_highway(highway_type)) {
            *forbidden_length += segment_length;
            result->forbidden_segments++;
        } else if (route_validator_cycling_priority_segment(street_item)) {
            *path_length += segment_length;
        } else {
            *secondary_length += segment_length;
            secondary_for_pilgrimage = 1;
        }
    } else if (driver_break_route_is_cycle_network_way(street_item)) {
        *path_length += segment_length;
    } else {
        *secondary_length += segment_length;
        secondary_for_pilgrimage = 1;
    }

    /* pilgrimage=yes: promote secondary segments to cycle-priority (forbidden highways stay forbidden) */
    if (secondary_for_pilgrimage) {
        char pil[32];
        if (get_osm_tag_value_item(street_item, "pilgrimage", pil, sizeof(pil)) && !g_ascii_strcasecmp(pil, "yes")) {
            *path_length += segment_length;
            if (*secondary_length >= segment_length)
                *secondary_length -= segment_length;
        }
    }
}

static void append_cycling_validation_warnings(struct route_validation_result *result, int mtb_scale_max,
                                               int mtb_uphill_max) {
    if (result->forbidden_percentage > 0) {
        result->warnings =
            g_list_append(result->warnings,
                          g_strdup_printf("Route uses %.1f%% of distance on highways unsuitable for cycling "
                                          "(motorway, trunk, primary and links)",
                                          result->forbidden_percentage));
    }
    if (result->path_percentage < 50.0 && result->forbidden_percentage == 0) {
        result->warnings =
            g_list_append(result->warnings,
                          g_strdup_printf("Only %.1f%% on cycle-priority infrastructure (cycleway, bicycle=yes/"
                                          "designated on paths, route=bicycle|mtb, network=ncn|rcn|lcn|icn, "
                                          "pilgrimage=yes on otherwise secondary segments where tagged)",
                                          result->path_percentage));
    }
    if (mtb_scale_max >= 4) {
        result->warnings =
            g_list_append(result->warnings,
                          g_strdup_printf("Difficult MTB terrain: mtb:scale up to %d (0=easiest, 6=extreme)",
                                          mtb_scale_max));
    }
    if (mtb_uphill_max >= 4) {
        result->warnings =
            g_list_append(result->warnings,
                          g_strdup_printf("Steep MTB climbs: mtb:scale:uphill up to %d (0=easiest, 5=extreme)",
                                          mtb_uphill_max));
    }
}

/* Classify one segment and update length accumulators and result->forbidden_segments */
static void classify_segment(double segment_length, struct item *street_item, int enable_hiking_pilgrimage_priority,
                             double *path_length, double *forbidden_length, double *secondary_length,
                             struct route_validation_result *result) {
    int added_secondary = 0;

    if (!street_item) {
        *secondary_length += segment_length;
        return;
    }

    const char *highway_type = route_validator_map_item_to_highway_type(street_item);
    if (highway_type) {
        if (route_validator_is_forbidden_highway(highway_type)) {
            *forbidden_length += segment_length;
            result->forbidden_segments++;
        } else if (route_validator_is_priority_path(highway_type)) {
            *path_length += segment_length;
        } else {
            *secondary_length += segment_length;
            added_secondary = 1;
        }
    } else {
        *secondary_length += segment_length;
        added_secondary = 1;
    }

    if (enable_hiking_pilgrimage_priority && added_secondary && driver_break_route_is_pilgrimage_hiking(street_item)) {
        *path_length += segment_length;
        if (*secondary_length >= segment_length) {
            *secondary_length -= segment_length;
        }
    }
}

/* Append validation warnings to result based on percentages */
static void append_validation_warnings(struct route_validation_result *result) {
    if (result->forbidden_percentage > 0) {
        char *warning = g_strdup_printf("Route uses %.1f%% forbidden highways (motorways, trunk, primary roads)",
                                        result->forbidden_percentage);
        result->warnings = g_list_append(result->warnings, warning);
    }
    if (result->path_percentage < 50.0 && result->forbidden_percentage == 0) {
        char *warning = g_strdup_printf("Only %.1f%% of route uses priority paths (footways, paths, tracks)",
                                        result->path_percentage);
        result->warnings = g_list_append(result->warnings, warning);
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
        classify_segment(segment_length, street_item, enable_hiking_pilgrimage_priority, &path_length,
                         &forbidden_length, &secondary_length, result);
    }

    map_rect_destroy(mr);

    if (total_length > 0) {
        result->path_percentage = (path_length / total_length) * 100.0;
        result->forbidden_percentage = (forbidden_length / total_length) * 100.0;
        result->secondary_percentage = (secondary_length / total_length) * 100.0;
    }

    append_validation_warnings(result);
    result->is_valid = (result->forbidden_percentage == 0);
    return result;
}

struct route_validation_result *route_validator_validate_cycling(struct route *route) {
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
    int mtb_scale_max = -1;
    int mtb_uphill_max = -1;
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
        classify_cycling_segment(segment_length, street_item, &path_length, &forbidden_length, &secondary_length,
                                 result);
        if (street_item)
            update_mtb_max_from_item(street_item, &mtb_scale_max, &mtb_uphill_max);
    }

    map_rect_destroy(mr);

    if (total_length > 0) {
        result->path_percentage = (path_length / total_length) * 100.0;
        result->forbidden_percentage = (forbidden_length / total_length) * 100.0;
        result->secondary_percentage = (secondary_length / total_length) * 100.0;
    }

    append_cycling_validation_warnings(result, mtb_scale_max, mtb_uphill_max);
    result->is_valid = (result->forbidden_percentage == 0);
    return result;
}

/* Motorcycle: 1 if forbidden (motorcycle=no or motor_vehicle=no) */
int route_validator_motorcycle_is_forbidden(struct item *item) {
    char val[32];
    if (!item)
        return 0;
    if (get_osm_tag_value_item(item, "motorcycle", val, sizeof(val)) && !strcmp(val, "no"))
        return 1;
    if (get_osm_tag_value_item(item, "motor_vehicle", val, sizeof(val)) && !strcmp(val, "no"))
        return 1;
    return 0;
}

/* Motorcycle: 1 if preferred (motorcycle=yes, designated, permissive) */
int route_validator_motorcycle_is_preferred(struct item *item) {
    char val[32];
    if (!item || !get_osm_tag_value_item(item, "motorcycle", val, sizeof(val)))
        return 0;
    return (!strcmp(val, "yes") || !strcmp(val, "designated") || !strcmp(val, "permissive"));
}

/* Motorcycle road terrain: 1 if surface is paved only (asphalt, paved) */
int route_validator_motorcycle_road_surface_ok(struct item *item) {
    char val[32];
    if (!item)
        return 0;
    if (!get_osm_tag_value_item(item, "surface", val, sizeof(val)))
        return 1; /* No surface tag: assume allowed */
    return (!strcmp(val, "asphalt") || !strcmp(val, "paved"));
}

/* Motorcycle adventure: 1 if access allowed; never route access=private or access=no (legal requirement) */
int route_validator_motorcycle_adventure_access_ok(struct item *item) {
    char val[32];
    if (!item)
        return 0;
    if (get_osm_tag_value_item(item, "access", val, sizeof(val))) {
        if (!strcmp(val, "private") || !strcmp(val, "no"))
            return 0;
        if (!strcmp(val, "yes") || !strcmp(val, "permissive"))
            return 1;
    }
    if (get_osm_tag_value_item(item, "motorcycle", val, sizeof(val))) {
        if (!strcmp(val, "yes") || !strcmp(val, "designated") || !strcmp(val, "permissive"))
            return 1;
    }
    return 0;
}

void route_validator_free_result(struct route_validation_result *result) {
    if (result) {
        if (result->warnings) {
            g_list_free_full(result->warnings, g_free);
        }
        g_free(result);
    }
}
