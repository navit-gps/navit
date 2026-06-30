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

#include "driver_break_poi_hiking.h"
#include "config.h"
#include "debug.h"
#include "driver_break_geo.h"
#include "driver_break_poi.h"
#include "driver_break_poi_map.h"
#include "mapset.h"
#include "navit.h"
#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Forward declaration - navit_get_default may not be exported */
struct navit *navit_get_default(void);

/* Compare function for sorting water points by distance */
static int poi_compare_water_distance(const struct water_point *a, const struct water_point *b) {
    if (a->distance_from_driver_break_stop < b->distance_from_driver_break_stop)
        return -1;
    if (a->distance_from_driver_break_stop > b->distance_from_driver_break_stop)
        return 1;
    return 0;
}

/* Compare function for sorting cabins by distance (prioritize network cabins if DNT priority) */
static int poi_compare_cabin_distance(const struct cabin_hut *a, const struct cabin_hut *b) {
    if (!a || !b)
        return 0;

    /* Prioritize network cabins (DNT/STF) */
    if (a->is_network && !b->is_network)
        return -1;
    if (!a->is_network && b->is_network)
        return 1;

    /* Then sort by distance */
    if (a->distance_from_driver_break_stop < b->distance_from_driver_break_stop)
        return -1;
    if (a->distance_from_driver_break_stop > b->distance_from_driver_break_stop)
        return 1;
    return 0;
}

static struct water_point *water_point_from_poi(struct driver_break_poi *poi, struct coord_geo *driver_break_stop,
                                                const char *spring_warning) {
    struct water_point *wp = g_new0(struct water_point, 1);

    wp->coord = poi->coord;
    wp->name = poi->name ? g_strdup(poi->name) : NULL;
    wp->type = poi->category ? g_strdup(poi->category) : NULL;
    wp->distance_from_driver_break_stop = driver_break_coord_distance_geo(driver_break_stop, &poi->coord);
    wp->is_spring = (poi->category && strstr(poi->category, "spring")) ? 1 : 0;
    if (wp->is_spring && spring_warning)
        wp->warning = g_strdup(spring_warning);
    return wp;
}

static GList *water_points_from_poi_list(GList *pois, struct coord_geo *driver_break_stop, const char *spring_warning) {
    GList *water_points = NULL;
    GList *l;

    for (l = pois; l; l = g_list_next(l)) {
        struct driver_break_poi *poi = (struct driver_break_poi *)l->data;
        if (poi)
            water_points = g_list_append(water_points, water_point_from_poi(poi, driver_break_stop, spring_warning));
    }
    return g_list_sort(water_points, (GCompareFunc)poi_compare_water_distance);
}

static void cabin_detect_network_from_name(struct cabin_hut *cabin, const char *name) {
    char *name_lower;

    if (!cabin || !name)
        return;

    name_lower = g_ascii_strdown(name, -1);
    if (strstr(name_lower, "dnt")) {
        cabin->is_dnt = 1;
        cabin->is_network = 1;
        cabin->network = g_strdup("DNT");
    } else if (strstr(name_lower, "stf")) {
        cabin->is_network = 1;
        cabin->network = g_strdup("STF");
    }
    g_free(name_lower);
}

static struct cabin_hut *cabin_from_poi(struct driver_break_poi *poi, struct coord_geo *driver_break_stop) {
    struct cabin_hut *cabin = g_new0(struct cabin_hut, 1);

    cabin->coord = poi->coord;
    cabin->name = poi->name ? g_strdup(poi->name) : NULL;
    cabin->type = poi->category ? g_strdup(poi->category) : NULL;
    cabin->distance_from_driver_break_stop = driver_break_coord_distance_geo(driver_break_stop, &poi->coord);
    cabin_detect_network_from_name(cabin, poi->name);
    return cabin;
}

static GList *cabins_from_poi_list(GList *pois, struct coord_geo *driver_break_stop) {
    GList *cabins = NULL;
    GList *l;

    for (l = pois; l; l = g_list_next(l)) {
        struct driver_break_poi *poi = (struct driver_break_poi *)l->data;
        if (poi)
            cabins = g_list_append(cabins, cabin_from_poi(poi, driver_break_stop));
    }
    return g_list_sort(cabins, (GCompareFunc)poi_compare_cabin_distance);
}

GList *poi_search_water_points(struct coord_geo *driver_break_stop, double radius_km) {
    const char *water_categories[] = {"amenity=drinking_water", "amenity=fountain", "natural=spring"};
    GList *pois;
    GList *water_points;

    if (!driver_break_stop || radius_km <= 0)
        return NULL;

    pois = driver_break_poi_discover(driver_break_stop, (int)radius_km, water_categories,
                                     sizeof(water_categories) / sizeof(water_categories[0]));
    if (!pois)
        return NULL;

    water_points = water_points_from_poi_list(pois, driver_break_stop, "Natural spring - drink at your own risk");
    driver_break_poi_free_list(pois);
    return water_points;
}

GList *poi_search_water_points_map(struct coord_geo *driver_break_stop, double radius_km, struct mapset *ms) {
    GList *pois;
    GList *water_points;

    if (!driver_break_stop || !ms || radius_km <= 0)
        return NULL;

    pois = driver_break_poi_map_search_water(driver_break_stop, radius_km, ms);
    if (!pois)
        return NULL;

    water_points = water_points_from_poi_list(pois, driver_break_stop, "Natural spring - may need treatment");
    driver_break_poi_free_list(pois);
    return water_points;
}

GList *poi_search_cabins(struct coord_geo *driver_break_stop, double radius_km) {
    const char *cabin_categories[] = {"tourism=wilderness_hut", "tourism=alpine_hut", "tourism=hostel",
                                      "tourism=camp_site"};
    GList *pois;
    GList *cabins;

    if (!driver_break_stop || radius_km <= 0)
        return NULL;

    pois = driver_break_poi_discover(driver_break_stop, (int)radius_km, cabin_categories,
                                     sizeof(cabin_categories) / sizeof(cabin_categories[0]));
    if (!pois)
        return NULL;

    cabins = cabins_from_poi_list(pois, driver_break_stop);
    driver_break_poi_free_list(pois);
    return cabins;
}

GList *poi_search_cabins_map(struct coord_geo *driver_break_stop, double radius_km, struct mapset *ms,
                             int enable_dnt_priority) {
    double search_radius = enable_dnt_priority ? 25.0 : radius_km;
    GList *pois;
    GList *cabins;

    if (!driver_break_stop || !ms || radius_km <= 0)
        return NULL;

    pois = driver_break_poi_map_search_cabins(driver_break_stop, search_radius, ms);
    if (!pois)
        return NULL;

    cabins = cabins_from_poi_list(pois, driver_break_stop);
    driver_break_poi_free_list(pois);
    return cabins;
}

void poi_free_water_points(GList *water_points) {
    GList *l = water_points;
    while (l) {
        poi_free_water_point((struct water_point *)l->data);
        l = g_list_next(l);
    }
    g_list_free(water_points);
}

void poi_free_water_point(struct water_point *wp) {
    if (wp) {
        g_free(wp->name);
        g_free(wp->type);
        g_free(wp->warning);
        g_free(wp);
    }
}

void poi_free_cabins(GList *cabins) {
    GList *l = cabins;
    while (l) {
        poi_free_cabin((struct cabin_hut *)l->data);
        l = g_list_next(l);
    }
    g_list_free(cabins);
}

void poi_free_cabin(struct cabin_hut *cabin) {
    if (cabin) {
        g_free(cabin->name);
        g_free(cabin->type);
        g_free(cabin->network);
        g_free(cabin);
    }
}
