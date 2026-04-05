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

#include "driver_break_finder.h"
#include "config.h"
#include "coord.h"
#include "debug.h"
#include "driver_break_glacier.h"
#include "driver_break_poi.h"
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "projection.h"
#include "route.h"
#include "transform.h"
#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Calculate distance between two coordinates in meters */
static double coord_distance(struct coord_geo *c1, struct coord_geo *c2) {
    double lat1 = c1->lat * M_PI / 180.0;
    double lat2 = c2->lat * M_PI / 180.0;
    double dlat = (c2->lat - c1->lat) * M_PI / 180.0;
    double dlng = (c2->lng - c1->lng) * M_PI / 180.0;

    double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlng / 2) * sin(dlng / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return 6371000.0 * c; /* Earth radius in meters */
}

/* Create map selection from coord_rect */
static struct map_selection *create_map_selection_from_rect(struct coord_rect *rect) {
    struct map_selection *sel = g_new0(struct map_selection, 1);
    sel->u.c_rect = *rect;
    sel->order = 18;
    sel->range = item_range_all;
    return sel;
}

/* Check if a coordinate is in excluded landuse areas */
static int is_excluded_landuse(struct coord_geo *coord, struct mapset *ms) {
    struct map *map;
    struct attr map_attr;
    struct attr_iter *iter;
    struct coord c;
    struct map_rect *mr;
    struct item *item;
    enum item_type landuse_types[] = {type_poly_farm, type_poly_commercial, type_poly_military,
                                      type_poly_recreation_ground, type_none};
    struct map_selection *sel;
    struct coord_rect rect;

    if (!ms)
        return 0;

    transform_from_geo(projection_mg, coord, &c);

    /* Create a small rectangle around the coordinate */
    rect.lu = c;
    rect.rl = c;
    rect.lu.x -= 200;
    rect.lu.y += 200;
    rect.rl.x += 200;
    rect.rl.y -= 200;

    sel = create_map_selection_from_rect(&rect);

    iter = mapset_attr_iter_new(NULL);
    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        mr = map_rect_new(map, sel);
        if (!mr)
            continue;

        while ((item = map_rect_get_item(mr))) {
            int i;
            for (i = 0; landuse_types[i] != type_none; i++) {
                if (item->type == landuse_types[i]) {
                    map_rect_destroy(mr);
                    map_selection_destroy(sel);
                    mapset_attr_iter_destroy(iter);
                    return 1; /* Found excluded landuse */
                }
            }
        }
        map_rect_destroy(mr);
    }
    map_selection_destroy(sel);
    mapset_attr_iter_destroy(iter);

    return 0;
}

/* Check if location is minimum distance from buildings */
static int check_distance_from_buildings(struct coord_geo *coord, int min_distance, struct mapset *ms) {
    struct map *map;
    struct attr map_attr;
    struct attr_iter *iter;
    struct coord c;
    struct map_rect *mr;
    struct item *item;
    struct coord_geo item_geo;
    double distance;
    struct map_selection *sel;
    struct coord_rect rect;

    if (!ms)
        return 1; /* Assume OK if no mapset */

    transform_from_geo(projection_mg, coord, &c);

    /* Create rectangle around coordinate */
    rect.lu = c;
    rect.rl = c;
    rect.lu.x -= min_distance * 2;
    rect.lu.y += min_distance * 2;
    rect.rl.x += min_distance * 2;
    rect.rl.y -= min_distance * 2;

    sel = create_map_selection_from_rect(&rect);

    iter = mapset_attr_iter_new(NULL);
    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        mr = map_rect_new(map, sel);
        if (!mr)
            continue;

        while ((item = map_rect_get_item(mr))) {
            if (item->type == type_poly_building) {
                struct coord item_coord;
                if (item_coord_get(item, &item_coord, 1) > 0) {
                    transform_to_geo(projection_mg, &item_coord, &item_geo);
                    distance = coord_distance(coord, &item_geo);
                    if (distance < min_distance) {
                        map_rect_destroy(mr);
                        map_selection_destroy(sel);
                        mapset_attr_iter_destroy(iter);
                        return 0; /* Too close to building */
                    }
                }
            }
        }
        map_rect_destroy(mr);
    }
    map_selection_destroy(sel);
    mapset_attr_iter_destroy(iter);

    return 1; /* OK */
}

int driver_break_finder_is_valid_location(struct coord_geo *coord, struct driver_break_config *config) {
    if (!coord || !config) {
        dbg(lvl_warning, "Driver Break plugin: Invalid location check (coord/config missing)");
        return 0;
    }

    /* Basic coordinate validation */
    if (coord->lat < -90 || coord->lat > 90) {
        dbg(lvl_warning, "Driver Break plugin: Location rejected (invalid latitude %.5f)", coord->lat);
        return 0;
    }
    if (coord->lng < -180 || coord->lng > 180) {
        dbg(lvl_warning, "Driver Break plugin: Location rejected (invalid longitude %.5f)", coord->lng);
        return 0;
    }

    dbg(lvl_info, "Driver Break plugin: Basic location validation passed (lat=%.5f lon=%.5f)", coord->lat, coord->lng);
    return 1;
}

/* Check if location is valid for nightly rest stop */
int driver_break_finder_is_valid_nightly_location(struct coord_geo *coord, struct driver_break_config *config,
                                                  struct mapset *ms, int has_camping_building) {
    if (!driver_break_finder_is_valid_location(coord, config)) {
        dbg(lvl_warning, "Driver Break plugin: Nightly location rejected by basic validation");
        return 0;
    }

    /* Check distance from buildings (150m minimum) */
    if (ms) {
        if (!check_distance_from_buildings(coord, config->min_distance_from_buildings, ms)) {
            dbg(lvl_warning,
                "Driver Break plugin: Nightly location rejected (too close to buildings, min=%d m, lat=%.5f lon=%.5f)",
                config->min_distance_from_buildings, coord->lat, coord->lng);
            return 0; /* Too close to buildings */
        }
    }

    /* Check distance from glaciers (300m minimum for nightly) */
    if (glacier_is_too_close_for_camping(coord, ms, has_camping_building)) {
        dbg(lvl_warning,
            "Driver Break plugin: Nightly location rejected (too close to glacier, has_camping_building=%d, "
            "lat=%.5f lon=%.5f)",
            has_camping_building, coord->lat, coord->lng);
        return 0; /* Too close to glacier */
    }

    dbg(lvl_info, "Driver Break plugin: Nightly location accepted (lat=%.5f lon=%.5f)", coord->lat, coord->lng);
    return 1;
}

/* Return 1 if street item type is suitable for a rest stop */
static int is_suitable_highway_for_stop(struct item *street_item) {
    if (!street_item) {
        return 0;
    }
    return (street_item->type == type_street_0 || street_item->type == type_street_service
            || street_item->type == type_track || street_item->type == type_street_3_land
            || street_item->type == type_street_3_city);
}

/* Return allocated highway type name for display (caller frees) */
static char *highway_type_name_from_item(struct item *street_item) {
    if (!street_item) {
        return g_strdup("tertiary");
    }
    if (street_item->type == type_street_0) {
        return g_strdup("unclassified");
    }
    if (street_item->type == type_street_service) {
        return g_strdup("service");
    }
    if (street_item->type == type_track) {
        return g_strdup("track");
    }
    return g_strdup("tertiary");
}

/* Water POI categories for remote/arid/hot mode (car, truck, motorcycle) */
static const char *water_poi_categories[] = {"amenity=drinking_water", "amenity=fountain", "natural=spring"};

/* Create one rest stop at the given coordinate. Caller frees stop. */
static struct driver_break_stop *create_rest_stop_at(struct coord_geo *coord_geo, double accumulated_km,
                                                     struct item *street_item, struct driver_break_config *config) {
    struct driver_break_stop *stop = g_new0(struct driver_break_stop, 1);
    stop->coord = *coord_geo;
    stop->distance_from_route = 0;
    stop->name = g_strdup_printf("Rest stop at %.1f km", accumulated_km);
    stop->highway_type = highway_type_name_from_item(street_item);
    stop->pois = driver_break_poi_discover(coord_geo, config->poi_search_radius_km, NULL, 0);
    if (config->enable_water_pois_remote_arid && config->poi_water_search_radius_km > 0) {
        GList *water =
            driver_break_poi_discover(coord_geo, config->poi_water_search_radius_km, water_poi_categories, 3);
        if (water)
            stop->pois = g_list_concat(stop->pois, water);
    }
    dbg(lvl_info, "Driver Break plugin: Rest stop created at lat=%.5f lon=%.5f distance=%.1f km", coord_geo->lat,
        coord_geo->lng, accumulated_km);
    return stop;
}

/* Find rest stops along a route */
GList *driver_break_finder_find_along_route(struct route *route, struct driver_break_config *config,
                                            int mandatory_break_required) {
    GList *stops = NULL;
    struct map *route_map;
    struct map_rect *mr;
    struct item *item;
    struct attr attr;
    double accumulated_distance = 0.0;
    int count = 0;
    const int max_stops = 10;

    (void)mandatory_break_required;

    if (!route || !config) {
        return NULL;
    }

    route_map = route_get_map(route);
    if (!route_map) {
        dbg(lvl_warning, "Driver Break plugin: No route map available");
        return NULL;
    }

    dbg(lvl_info, "Driver Break plugin: Finding rest stops along route");

    mr = map_rect_new(route_map, NULL);
    if (!mr) {
        return NULL;
    }

    while ((item = map_rect_get_item(mr)) && count < max_stops) {
        if (item->type != type_street_route) {
            continue;
        }
        if (!item_attr_get(item, attr_length, &attr)) {
            continue;
        }

        double segment_length = (double)attr.u.num;
        accumulated_distance += segment_length;

        struct item *street_item = NULL;
        struct attr street_attr;
        if (item_attr_get(item, attr_street_item, &street_attr)) {
            street_item = street_attr.u.item;
        }

        if (!is_suitable_highway_for_stop(street_item) || accumulated_distance < 5000.0) {
            continue;
        }

        struct coord c;
        item_coord_rewind(item);
        if (!item_coord_get(item, &c, 1)) {
            continue;
        }

        struct coord_geo coord_geo;
        transform_to_geo(projection_mg, &c, &coord_geo);
        if (!driver_break_finder_is_valid_location(&coord_geo, config)) {
            continue;
        }

        struct driver_break_stop *stop =
            create_rest_stop_at(&coord_geo, accumulated_distance / 1000.0, street_item, config);
        stops = g_list_append(stops, stop);
        count++;
        accumulated_distance = 0.0;
    }

    map_rect_destroy(mr);
    dbg(lvl_info, "Driver Break plugin: Found %d rest stop candidates", count);
    return stops;
}

/* Find rest stops near a coordinate */
GList *driver_break_finder_find_near(struct coord_geo *center, double distance_km, struct driver_break_config *config) {
    GList *stops = NULL;

    if (!center || !config) {
        return NULL;
    }

    dbg(lvl_info, "Driver Break plugin: Finding rest stops near %.6f,%.6f (radius %.1f km)", center->lat, center->lng,
        distance_km);

    /* Query OSM data for suitable locations using Overpass API */
    /* Search for intersections of suitable highway types */
    const char *poi_categories[] = {"highway=unclassified", "highway=service", "highway=track",
                                    "highway=driver_break_area", "highway=tertiary"};

    /* Use POI discovery to find highway nodes, then filter for intersections */
    GList *candidates = driver_break_poi_discover(center, (int)distance_km, poi_categories,
                                                  sizeof(poi_categories) / sizeof(poi_categories[0]));

    if (candidates) {
        GList *l = candidates;
        int count = 0;
        const int max_stops = 10;

        while (l && count < max_stops) {
            struct driver_break_poi *candidate = (struct driver_break_poi *)l->data;
            if (candidate) {
                /* Validate location */
                if (driver_break_finder_is_valid_location(&candidate->coord, config)) {
                    struct driver_break_stop *stop = g_new0(struct driver_break_stop, 1);
                    stop->coord = candidate->coord;
                    stop->distance_from_route = candidate->distance_from_driver_break_stop;
                    stop->name = candidate->name ? g_strdup(candidate->name) : g_strdup("Rest stop");
                    stop->highway_type = candidate->category ? g_strdup(candidate->category) : g_strdup("unknown");

                    /* Discover additional POIs */
                    stop->pois = driver_break_poi_discover(&candidate->coord, config->poi_search_radius_km, NULL, 0);
                    if (config->enable_water_pois_remote_arid && config->poi_water_search_radius_km > 0) {
                        GList *water = driver_break_poi_discover(&candidate->coord, config->poi_water_search_radius_km,
                                                                 water_poi_categories, 3);
                        if (water)
                            stop->pois = g_list_concat(stop->pois, water);
                    }

                    stops = g_list_append(stops, stop);
                    count++;
                }
            }
            l = g_list_next(l);
        }

        driver_break_poi_free_list(candidates);
    }

    return stops;
}

void driver_break_finder_free_list(GList *stops) {
    GList *l = stops;
    while (l) {
        driver_break_finder_free((struct driver_break_stop *)l->data);
        l = g_list_next(l);
    }
    g_list_free(stops);
}

void driver_break_finder_free(struct driver_break_stop *stop) {
    if (stop) {
        g_free(stop->name);
        g_free(stop->highway_type);
        if (stop->pois) {
            driver_break_poi_free_list(stop->pois);
        }
        g_free(stop);
    }
}
