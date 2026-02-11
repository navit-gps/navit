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
#include "coord.h"
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "route.h"
#include "transform.h"
#include "projection.h"
#include "driver_break_finder.h"
#include "driver_break_poi.h"
#include "driver_break_glacier.h"
#include "transform.h"
#include "item.h"
#include "mapset.h"

/* Calculate distance between two coordinates in meters */
static double coord_distance(struct coord_geo *c1, struct coord_geo *c2) {
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
    enum item_type landuse_types[] = {
        type_poly_farm,
        type_poly_commercial,
        type_poly_military,
        type_poly_recreation_ground,
        type_none
    };
    struct map_selection *sel;
    struct coord_rect rect;

    if (!ms) return 0;

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
        if (!mr) continue;

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

    if (!ms) return 1; /* Assume OK if no mapset */

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
        if (!mr) continue;

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
    if (!coord || !config) return 0;

    /* Basic coordinate validation */
    if (coord->lat < -90 || coord->lat > 90) return 0;
    if (coord->lng < -180 || coord->lng > 180) return 0;

    return 1;
}

/* Check if location is valid for nightly rest stop */
int driver_break_finder_is_valid_nightly_location(struct coord_geo *coord,
                                            struct driver_break_config *config,
                                            struct mapset *ms,
                                            int has_camping_building) {
    if (!driver_break_finder_is_valid_location(coord, config)) {
        return 0;
    }

    /* Check distance from buildings (150m minimum) */
    if (ms) {
        if (!check_distance_from_buildings(coord, config->min_distance_from_buildings, ms)) {
            return 0;  /* Too close to buildings */
        }
    }

    /* Check distance from glaciers (300m minimum for nightly) */
    if (glacier_is_too_close_for_camping(coord, ms, has_camping_building)) {
        return 0;  /* Too close to glacier */
    }

    return 1;
}

/* Find rest stops along a route */
GList *driver_break_finder_find_along_route(struct route *route,
                                     struct driver_break_config *config,
                                     int mandatory_break_required) {
    GList *stops = NULL;
    struct map *route_map;
    struct map_rect *mr;
    struct item *item;
    struct coord_geo coord;
    struct driver_break_stop *stop;
    int count = 0;
    const int max_stops = 10;

    if (!route || !config) {
        return NULL;
    }

    /* Get route map */
    route_map = route_get_map(route);
    if (!route_map) {
        dbg(lvl_warning, "Driver Break plugin: No route map available");
        return NULL;
    }

    dbg(lvl_info, "Driver Break plugin: Finding rest stops along route");

    /* Iterate through route items to find suitable locations */
    mr = map_rect_new(route_map, NULL);
    if (!mr) {
        return NULL;
    }

    /* Iterate through route segments to find suitable rest stop locations */
    struct attr attr;
    double accumulated_distance = 0.0;
    struct coord prev_coord = {0, 0};
    int first_coord = 1;

    while ((item = map_rect_get_item(mr)) && count < max_stops) {
        if (item->type != type_street_route) {
            continue;
        }

        /* Get segment length */
        if (item_attr_get(item, attr_length, &attr)) {
            double segment_length = (double)attr.u.num;
            accumulated_distance += segment_length;

            /* Get street item to check highway type */
            struct item *street_item = NULL;
            struct attr street_attr;
            if (item_attr_get(item, attr_street_item, &street_attr)) {
                street_item = street_attr.u.item;
            }

            /* Check if this segment is at a suitable intersection */
            if (street_item) {
                /* Check for suitable highway types */
                int is_suitable = 0;
                if (street_item->type == type_street_0 ||
                    street_item->type == type_street_service ||
                    street_item->type == type_track ||
                    street_item->type == type_street_3_land ||
                    street_item->type == type_street_3_city) {
                    is_suitable = 1;
                }

                /* Get coordinates for this segment */
                struct coord c;
                item_coord_rewind(item);
                if (item_coord_get(item, &c, 1)) {
                    if (first_coord) {
                        prev_coord = c;
                        first_coord = 0;
                    }

                    /* Check if we should suggest a rest stop here */
                    /* For mandatory breaks, suggest every 5-10 km */
                    if (is_suitable && accumulated_distance >= 5000.0) {
                        struct coord_geo coord_geo;
                        transform_to_geo(projection_mg, &c, &coord_geo);

                        /* Validate location */
                        if (driver_break_finder_is_valid_location(&coord_geo, config)) {
                            stop = g_new0(struct driver_break_stop, 1);
                            stop->coord = coord_geo;
                            stop->distance_from_route = 0;
                            stop->name = g_strdup_printf("Rest stop at %.1f km", accumulated_distance / 1000.0);

                            /* Get highway type name */
                            if (street_item->type == type_street_0) {
                                stop->highway_type = g_strdup("unclassified");
                            } else if (street_item->type == type_street_service) {
                                stop->highway_type = g_strdup("service");
                            } else if (street_item->type == type_track) {
                                stop->highway_type = g_strdup("track");
                            } else {
                                stop->highway_type = g_strdup("tertiary");
                            }

                            /* Discover POIs using map data if available */
                            /* Note: mapset should be passed as parameter - for now use Overpass fallback */
                            stop->pois = driver_break_poi_discover(&coord_geo, config->poi_search_radius_km,
                                                           NULL, 0);

                            stops = g_list_append(stops, stop);
                            count++;
                            accumulated_distance = 0.0;  /* Reset for next stop */
                        }
                    }
                }
            }
        }
    }

    map_rect_destroy(mr);

    dbg(lvl_info, "Driver Break plugin: Found %d rest stop candidates", count);

    return stops;
}

/* Find rest stops near a coordinate */
GList *driver_break_finder_find_near(struct coord_geo *center,
                              double distance_km,
                              struct driver_break_config *config) {
    GList *stops = NULL;

    if (!center || !config) {
        return NULL;
    }

    dbg(lvl_info, "Driver Break plugin: Finding rest stops near %.6f,%.6f (radius %.1f km)",
        center->lat, center->lng, distance_km);

    /* Query OSM data for suitable locations using Overpass API */
    /* Search for intersections of suitable highway types */
    const char *poi_categories[] = {
        "highway=unclassified",
        "highway=service",
        "highway=track",
        "highway=driver_break_area",
        "highway=tertiary"
    };

    /* Use POI discovery to find highway nodes, then filter for intersections */
    GList *candidates = driver_break_poi_discover(center, (int)distance_km,
                                         poi_categories,
                                         sizeof(poi_categories)/sizeof(poi_categories[0]));

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
                    stop->name = candidate->name ? g_strdup(candidate->name) :
                                 g_strdup("Rest stop");
                    stop->highway_type = candidate->category ? g_strdup(candidate->category) :
                                        g_strdup("unknown");

                    /* Discover additional POIs */
                    stop->pois = driver_break_poi_discover(&candidate->coord,
                                                   config->poi_search_radius_km,
                                                   NULL, 0);

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
