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

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include "debug.h"
#include "driver_break_poi_map.h"
#include "driver_break_poi.h"
#include "map.h"
#include "item.h"
#include "transform.h"
#include "attr.h"
#include "projection.h"
/* item_def.h included via item.h */

/* Calculate distance between two coordinates in meters */
static double coord_distance_geo(struct coord_geo *c1, struct coord_geo *c2) {
    double lat1 = c1->lat * M_PI / 180.0;
    double lat2 = c2->lat * M_PI / 180.0;
    double dlat = (c2->lat - c1->lat) * M_PI / 180.0;
    double dlng = (c2->lng - c1->lng) * M_PI / 180.0;
    
    double a = sin(dlat/2) * sin(dlat/2) +
               cos(lat1) * cos(lat2) *
               sin(dlng/2) * sin(dlng/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return 6371000.0 * c; /* Earth radius in meters */
}

/* Extract OSM tag value from item */
static char *get_osm_tag_value(struct item *item, const char *key) {
    struct attr attr;
    char tag_key[256];
    char *tag_value = NULL;
    
    /* Try to get OSM tag attribute */
    if (item_attr_get(item, attr_osm_tag, &attr)) {
        /* OSM tags are stored as "key=value" pairs */
        const char *tags = attr.u.str;
        if (tags) {
            snprintf(tag_key, sizeof(tag_key), "%s=", key);
            const char *pos = strstr(tags, tag_key);
            if (pos) {
                pos += strlen(tag_key);
                const char *end = strchr(pos, ' ');
                if (!end) end = pos + strlen(pos);
                int len = end - pos;
                if (len > 0) {
                    tag_value = g_malloc(len + 1);
                    memcpy(tag_value, pos, len);
                    tag_value[len] = '\0';
                }
            }
        }
    }
    
    return tag_value;
}

/* Check if POI item has DNT/network tags */
int driver_break_poi_is_network_cabin(struct item *item, char **network_name) {
    if (!item || !network_name) {
        return 0;
    }
    
    *network_name = NULL;
    
    /* Check for operator tags (DNT, STF, etc.) */
    char *operator = get_osm_tag_value(item, "operator");
    if (operator) {
        char *op_lower = g_ascii_strdown(operator, -1);
        
        if (strstr(op_lower, "dnt") || strstr(op_lower, "den norske turistforening")) {
            *network_name = g_strdup("DNT");
            g_free(op_lower);
            g_free(operator);
            return 1;
        } else if (strstr(op_lower, "stf") || strstr(op_lower, "svenska turistforeningen")) {
            *network_name = g_strdup("STF");
            g_free(op_lower);
            g_free(operator);
            return 1;
        }
        
        g_free(op_lower);
        g_free(operator);
    }
    
    /* Check for network tag */
    char *network = get_osm_tag_value(item, "network");
    if (network) {
        *network_name = g_strdup(network);
        g_free(network);
        return 1;
    }
    
    return 0;
}

/* Check if route segment has pilgrimage/hiking route tags */
int driver_break_route_is_pilgrimage_hiking(struct item *item) {
    if (!item) {
        return 0;
    }
    
    /* Check for route=hiking tag */
    char *route = get_osm_tag_value(item, "route");
    if (route) {
        char *route_lower = g_ascii_strdown(route, -1);
        int is_hiking = (strcmp(route_lower, "hiking") == 0);
        g_free(route_lower);
        g_free(route);
        if (is_hiking) {
            return 1;
        }
    }
    
    /* Check for pilgrimage=yes tag */
    char *pilgrimage = get_osm_tag_value(item, "pilgrimage");
    if (pilgrimage) {
        char *pil_lower = g_ascii_strdown(pilgrimage, -1);
        int is_pilgrimage = (strcmp(pil_lower, "yes") == 0);
        g_free(pil_lower);
        g_free(pilgrimage);
        if (is_pilgrimage) {
            return 1;
        }
    }
    
    return 0;
}

/* Search for POIs in maps near a coordinate */
GList *driver_break_poi_map_search(struct coord_geo *center, double radius_km, 
                           enum item_type *poi_types, int num_types,
                           struct mapset *ms) {
    GList *pois = NULL;
    
    if (!center || !ms || radius_km <= 0 || !poi_types || num_types == 0) {
        return NULL;
    }
    
    /* Convert center to map coordinates */
    struct coord c;
    transform_from_geo(projection_mg, center, &c);
    
    /* Create map selection rectangle */
    double radius_m = radius_km * 1000.0;
    int radius_internal = (int)(radius_m * 9.0); /* Approximate conversion */
    
    struct coord_rect rect;
    rect.lu = c;
    rect.rl = c;
    rect.lu.x -= radius_internal;
    rect.lu.y += radius_internal;
    rect.rl.x += radius_internal;
    rect.rl.y -= radius_internal;
    
    struct map_selection *sel = g_new0(struct map_selection, 1);
    sel->u.c_rect = rect;
    sel->order = 18;
    sel->range = item_range_all;
    
    /* Search in mapset */
    struct map *map;
    struct attr map_attr;
    struct attr_iter *iter = mapset_attr_iter_new(NULL);
    
    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        struct map_rect *mr = map_rect_new(map, sel);
        if (!mr) continue;
        
        struct item *item;
        while ((item = map_rect_get_item(mr))) {
            /* Check if item type matches requested POI types */
            int matches = 0;
            int i;
            for (i = 0; i < num_types; i++) {
                if (item->type == poi_types[i]) {
                    matches = 1;
                    break;
                }
            }
            
            if (matches) {
                /* Get item coordinates */
                struct coord item_coord;
                if (item_coord_get(item, &item_coord, 1)) {
                    struct coord_geo item_geo;
                    transform_to_geo(projection_mg, &item_coord, &item_geo);
                    
                    /* Calculate distance */
                    double distance = coord_distance_geo(center, &item_geo);
                    if (distance <= radius_m) {
                        /* Create POI structure */
                        struct driver_break_poi *poi = g_new0(struct driver_break_poi, 1);
                        poi->coord = item_geo;
                        poi->distance_from_driver_break_stop = distance;
                        
                        /* Get name/label */
                        struct attr label_attr;
                        if (item_attr_get(item, attr_label, &label_attr)) {
                            poi->name = g_strdup(map_convert_string(item->map, label_attr.u.str));
                        }
                        
                        /* Get type from item type name */
                        poi->type = g_strdup("poi");
                        poi->category = g_strdup(item_to_name(item->type));
                        
                        /* Get opening hours if available */
                        struct attr hours_attr;
                        if (item_attr_get(item, attr_open_hours, &hours_attr)) {
                            poi->opening_hours = g_strdup(hours_attr.u.str);
                        }
                        
                        pois = g_list_append(pois, poi);
                    }
                }
            }
        }
        
        map_rect_destroy(mr);
    }
    
    mapset_attr_iter_destroy(iter);
    g_free(sel);
    
    return pois;
}

/* Search for water points in maps */
GList *driver_break_poi_map_search_water(struct coord_geo *center, double radius_km, struct mapset *ms) {
    enum item_type water_types[] = {
        type_poi_potable_water,
        type_poi_fountain,
        type_none
    };
    
    return driver_break_poi_map_search(center, radius_km, water_types, 2, ms);
}

/* Process cabin item and add to POI list if within radius */
static void process_cabin_item(struct item *item, struct coord_geo *center, double radius_m, GList **pois) {
    struct coord item_coord;
    if (!item_coord_get(item, &item_coord, 1)) {
        return;
    }
    
    struct coord_geo item_geo;
    transform_to_geo(projection_mg, &item_coord, &item_geo);
    
    double distance = coord_distance_geo(center, &item_geo);
    if (distance > radius_m) {
        return;
    }
    
    /* Create POI structure */
    struct driver_break_poi *poi = g_new0(struct driver_break_poi, 1);
    poi->coord = item_geo;
    poi->distance_from_driver_break_stop = distance;
    
    /* Get name/label */
    struct attr label_attr;
    if (item_attr_get(item, attr_label, &label_attr)) {
        poi->name = g_strdup(map_convert_string(item->map, label_attr.u.str));
    }
    
    poi->type = g_strdup("tourism");
    poi->category = g_strdup(item_to_name(item->type));
    
    /* Check for DNT/network tags */
    char *network_name = NULL;
    if (driver_break_poi_is_network_cabin(item, &network_name)) {
        if (network_name && poi->name) {
            char *new_name = g_strdup_printf("%s [%s]", poi->name, network_name);
            g_free(poi->name);
            poi->name = new_name;
        } else if (network_name) {
            poi->name = g_strdup_printf("[%s]", network_name);
        }
        g_free(network_name);
    }
    
    *pois = g_list_append(*pois, poi);
}

/* Search for cabins/huts in maps (with DNT/network detection) */
GList *driver_break_poi_map_search_cabins(struct coord_geo *center, double radius_km, struct mapset *ms) {
    GList *pois = NULL;
    
    if (!center || !ms || radius_km <= 0) {
        return NULL;
    }
    
    enum item_type cabin_types[] = {
        type_poi_hostel,
        type_poi_camping,
        type_none
    };
    
    double radius_m = radius_km * 1000.0;
    struct coord c;
    transform_from_geo(projection_mg, center, &c);
    
    int radius_internal = (int)(radius_m * 9.0);
    struct coord_rect rect;
    rect.lu = c;
    rect.rl = c;
    rect.lu.x -= radius_internal;
    rect.lu.y += radius_internal;
    rect.rl.x += radius_internal;
    rect.rl.y -= radius_internal;
    
    struct map_selection *sel = g_new0(struct map_selection, 1);
    sel->u.c_rect = rect;
    sel->order = 18;
    sel->range = item_range_all;
    
    struct map *map;
    struct attr map_attr;
    struct attr_iter *iter = mapset_attr_iter_new(NULL);
    
    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        struct map_rect *mr = map_rect_new(map, sel);
        if (!mr) {
            continue;
        }
        
        struct item *item;
        while ((item = map_rect_get_item(mr))) {
            int matches = 0;
            int i;
            for (i = 0; cabin_types[i] != type_none; i++) {
                if (item->type == cabin_types[i]) {
                    matches = 1;
                    break;
                }
            }
            
            if (matches) {
                process_cabin_item(item, center, radius_m, &pois);
            }
        }
        
        map_rect_destroy(mr);
    }
    
    mapset_attr_iter_destroy(iter);
    g_free(sel);
    
    return pois;
}

/* Search for car POIs in maps */
GList *driver_break_poi_map_search_car_pois(struct coord_geo *center, double radius_km, struct mapset *ms) {
    enum item_type car_poi_types[] = {
        type_poi_cafe,
        type_poi_restaurant,
        type_poi_museum_history,
        type_poi_viewpoint,
        type_poi_zoo,
        type_poi_picnic,
        type_poi_attraction,
        type_none
    };
    
    return driver_break_poi_map_search(center, radius_km, car_poi_types, 7, ms);
}
