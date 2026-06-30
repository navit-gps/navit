/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "driver_break_route_stops.h"
#include "driver_break_cycling.h"
#include "driver_break_hiking.h"
#include "driver_break_poi.h"
#include "driver_break_poi_hiking.h"
#include "navit.h"
#include "projection.h"
#include "route.h"
#include "transform.h"
#include <glib.h>

static struct driver_break_poi *driver_break_poi_from_water_point(struct water_point *wp) {
    struct driver_break_poi *poi;

    if (!wp) {
        return NULL;
    }

    poi = g_new0(struct driver_break_poi, 1);
    poi->coord = wp->coord;
    poi->name = wp->name ? g_strdup(wp->name) : NULL;
    poi->type = g_strdup("amenity");
    poi->category = wp->type ? g_strdup(wp->type) : NULL;
    poi->distance_from_driver_break_stop = wp->distance_from_driver_break_stop;
    return poi;
}

static struct driver_break_poi *driver_break_poi_from_cabin(struct cabin_hut *cabin) {
    struct driver_break_poi *poi;

    if (!cabin) {
        return NULL;
    }

    poi = g_new0(struct driver_break_poi, 1);
    poi->coord = cabin->coord;
    poi->name = cabin->name ? g_strdup(cabin->name) : NULL;
    poi->type = g_strdup("tourism");
    poi->category = cabin->type ? g_strdup(cabin->type) : NULL;
    poi->distance_from_driver_break_stop = cabin->distance_from_driver_break_stop;
    return poi;
}

static void driver_break_stop_append_water_pois(struct driver_break_stop *stop, GList *water_pois_list) {
    GList *l;

    for (l = water_pois_list; l; l = g_list_next(l)) {
        struct driver_break_poi *poi = driver_break_poi_from_water_point((struct water_point *)l->data);
        if (poi) {
            stop->pois = g_list_append(stop->pois, poi);
        }
    }
}

static void driver_break_stop_append_cabin_pois(struct driver_break_stop *stop, GList *cabin_pois_list) {
    GList *l;

    for (l = cabin_pois_list; l; l = g_list_next(l)) {
        struct driver_break_poi *poi = driver_break_poi_from_cabin((struct cabin_hut *)l->data);
        if (poi) {
            stop->pois = g_list_append(stop->pois, poi);
        }
    }
}

static void driver_break_stop_populate_pois(struct driver_break_priv *priv, struct driver_break_stop *stop,
                                            struct coord_geo *coord) {
    struct mapset *ms = navit_get_mapset(priv->nav);

    if (ms) {
        GList *water_pois_list = poi_search_water_points_map(coord, priv->config.poi_water_search_radius_km, ms);
        GList *cabin_pois_list =
            poi_search_cabins_map(coord, priv->config.poi_cabin_search_radius_km, ms, priv->config.enable_dnt_priority);

        driver_break_stop_append_water_pois(stop, water_pois_list);
        driver_break_stop_append_cabin_pois(stop, cabin_pois_list);
        poi_free_water_points(water_pois_list);
        poi_free_cabins(cabin_pois_list);
        return;
    }

    stop->pois = driver_break_poi_discover(coord, priv->config.poi_search_radius_km, NULL, 0);
}

static struct driver_break_stop *driver_break_stop_from_route_position(struct driver_break_priv *priv, int position_m,
                                                                       const char *label) {
    struct driver_break_stop *stop;
    struct coord c;

    if (!priv->current_route) {
        return NULL;
    }

    c = route_get_coord_dist(priv->current_route, position_m);
    stop = g_new0(struct driver_break_stop, 1);
    transform_to_geo(projection_mg, &c, &stop->coord);
    stop->distance_from_route = 0;
    stop->name = g_strdup_printf("%s at %.1f km", label, position_m / 1000.0);
    driver_break_stop_populate_pois(priv, stop, &stop->coord);
    return stop;
}

void driver_break_process_hiking_stops(struct driver_break_priv *priv, GList *hiking_stops) {
    GList *l;

    for (l = hiking_stops; l; l = g_list_next(l)) {
        struct hiking_driver_break_stop *h_stop = (struct hiking_driver_break_stop *)l->data;
        struct driver_break_stop *stop;

        if (!h_stop) {
            continue;
        }

        stop = driver_break_stop_from_route_position(priv, (int)h_stop->position, "Hiking rest stop");
        if (stop) {
            h_stop->coord = stop->coord;
            priv->suggested_stops = g_list_append(priv->suggested_stops, stop);
        }
    }
}

void driver_break_process_cycling_stops(struct driver_break_priv *priv, GList *cycling_stops) {
    GList *l;

    for (l = cycling_stops; l; l = g_list_next(l)) {
        struct cycling_driver_break_stop *c_stop = (struct cycling_driver_break_stop *)l->data;
        struct driver_break_stop *stop;

        if (!c_stop) {
            continue;
        }

        stop = driver_break_stop_from_route_position(priv, (int)c_stop->position, "Cycling rest stop");
        if (stop) {
            c_stop->coord = stop->coord;
            priv->suggested_stops = g_list_append(priv->suggested_stops, stop);
        }
    }
}
