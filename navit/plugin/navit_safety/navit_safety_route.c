/**
 * @file navit_safety_route.c
 * @brief Live route-corridor scanning and planning state implementation.
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if NAVIT_SAFETY_WITH_OSD
#    include "attr.h"
#    include "callback.h"
#    include "coord.h"
#    include "debug.h"
#    include "item.h"
#    include "map.h"
#    include "mapset.h"
#    include "navit.h"
#    include "route.h"
#    include "transform.h"
#    include <glib.h>
#    if NAVIT_SAFETY_WITH_SQLITE
#        include "navit_safety_cache.h"
#    endif
#endif /* NAVIT_SAFETY_WITH_OSD */
#include "navit_safety.h"
#include "navit_safety_confidence.h"
#include "navit_safety_heat.h"
#include "navit_safety_route.h"

#define NAVIT_SAFETY_CORRIDOR_HALF_M 25000
#define NAVIT_SAFETY_SAMPLE_STEP_M 25000
#define NAVIT_SAFETY_MAX_POIS 128

void navit_safety_route_state_init(struct navit_safety_route_state *state) {
    if (!state)
        return;
    memset(state, 0, sizeof(*state));
    state->nearest_idx = -1;
}

void navit_safety_route_state_clear(struct navit_safety_route_state *state) {
    if (!state)
        return;
    free(state->pois);
    state->pois = NULL;
    state->n_pois = 0;
    state->valid = 0;
    state->nearest_idx = -1;
    state->status[0] = 0;
}

void navit_safety_build_plan_stops(const struct navit_safety_poi_entry *pois, int n_pois,
                                   struct navit_safety_stop *stops, int *n_stops) {
    int i;
    int out;

    if (!n_stops)
        return;
    out = 0;
    for (i = 0; pois && i < n_pois; i++) {
        if (pois[i].denied)
            continue;
        if (!stops)
            continue;
        stops[out].distance_km = pois[i].distance_km;
        stops[out].confidence = pois[i].confidence;
        out++;
    }
    *n_stops = out;
}

static const char *remote_mode_label(int remote_mode) {
    switch (remote_mode) {
    case NAVIT_SAFETY_REMOTE_ALWAYS:
        return "Always";
    case NAVIT_SAFETY_REMOTE_OFF:
        return "Off";
    case NAVIT_SAFETY_REMOTE_AUTO:
    default:
        return "Auto";
    }
}

static const char *heat_level_suffix(enum navit_safety_heat_level level) {
    switch (level) {
    case NAVIT_SAFETY_HEAT_CAUTION:
        return "  Heat: caution";
    case NAVIT_SAFETY_HEAT_WARNING:
        return "  Heat: warning";
    case NAVIT_SAFETY_HEAT_DANGER:
        return "  Heat: danger";
    default:
        return "";
    }
}

static void append_foot_travel_status(const struct navit_safety_route_state *state, char *out, int out_len) {
    char extra[128];

    snprintf(extra, sizeof(extra), "\nWater: %d ml/h%s", state->heat.water_ml_per_hour,
             heat_level_suffix(state->heat.level));
    strncat(out, extra, out_len - (int)strlen(out) - 1);
}

void navit_safety_format_status(const struct navit_safety_route_state *state, const struct navit_safety_config *config,
                                char *out, int out_len) {
    const struct navit_safety_plan_result *plan;
    const char *mode;
    const char *remote;

    if (!out || out_len <= 0)
        return;
    out[0] = 0;
    if (!state || !config)
        return;

    if (!state->valid) {
        snprintf(out, out_len, "Navit Safety\nNo active route");
        return;
    }

    plan = &state->fuel_plan;
    mode = remote_mode_label(config->remote_mode);
    remote = plan->remote_active ? "active" : "inactive";

    snprintf(out, out_len, "Navit Safety\nRemote: %s (%s)\nBuffer: %d km  Range: %d km\nGap: %d km%s%s", mode, remote,
             plan->buffer_km, plan->usable_range_km, plan->gap.max_gap_km, plan->gap.warning ? "  WARN" : "",
             plan->desert_warning ? "\nDesert: higher consumption likely" : "");

    if (config->foot_travel_mode)
        append_foot_travel_status(state, out, out_len);
}

#if NAVIT_SAFETY_WITH_OSD

static int poi_entry_compare(const void *a, const void *b) {
    const struct navit_safety_poi_entry *pa = a;
    const struct navit_safety_poi_entry *pb = b;
    return pa->distance_km - pb->distance_km;
}

static int item_is_fuel(enum item_type type) {
    return type == type_poi_fuel;
}

static int item_is_water(enum item_type type) {
    return type == type_poi_drinking_water || type == type_poi_water_feature;
}

static int item_matches_resource(enum item_type type, int foot_travel) {
    if (foot_travel)
        return item_is_water(type);
    return item_is_fuel(type);
}

static void poi_id_from_item(struct item *item, char *out, int out_len) {
    struct attr attr;

    if (!out || out_len <= 0)
        return;
    out[0] = 0;
    if (!item)
        return;
    if (item_attr_get(item, attr_osm_wayid, &attr)) {
        g_snprintf(out, out_len, "osm:%llu", (unsigned long long)*attr.u.num64);
        return;
    }
    g_snprintf(out, out_len, "map:%d:%d", item->id_hi, item->id_lo);
}

static void poi_label_from_item(struct item *item, char *out, int out_len) {
    struct attr attr;

    if (!out || out_len <= 0)
        return;
    out[0] = 0;
    if (!item)
        return;
    if (item_attr_get(item, attr_label, &attr))
        g_snprintf(out, out_len, "%s", map_convert_string_tmp(item->map, attr.u.str));
    else
        g_snprintf(out, out_len, "%s", item_to_name(item->type));
}

static enum navit_safety_confidence score_poi_entry(const struct navit_safety_config *config,
                                                    struct navit_safety_cache *cache, const char *trip_id,
                                                    const char *poi_id, int denied) {
    struct navit_safety_poi poi;

    if (denied)
        return NAVIT_SAFETY_CONFIDENCE_LOW;

#    if NAVIT_SAFETY_WITH_SQLITE
    if (cache && trip_id && poi_id[0] && navit_safety_cache_is_confirmed(cache, poi_id, trip_id))
        return NAVIT_SAFETY_CONFIDENCE_HIGH;
#    else
    (void)cache;
    (void)trip_id;
#    endif

    (void)config;
    memset(&poi, 0, sizeof(poi));
    poi.source = NAVIT_SAFETY_SRC_OSM_INDEPENDENT;
    poi.age_days = -1;
    poi.depopulating_region = 0;
    return navit_safety_score_poi(&poi);
}

static int poi_index_by_id(struct navit_safety_poi_entry *pois, int n_pois, const char *poi_id) {
    int i;

    for (i = 0; i < n_pois; i++) {
        if (!strcmp(pois[i].poi_id, poi_id))
            return i;
    }
    return -1;
}

static void route_run_plan(struct navit_safety_route_state *state, const struct navit_safety_config *config,
                           int full_range_km, double wbgt_c) {
    struct navit_safety_stop stops[NAVIT_SAFETY_MAX_POIS];
    int n_stops;

    navit_safety_build_plan_stops(state->pois, state->n_pois, stops, &n_stops);
    navit_safety_plan(config, NAVIT_SAFETY_RESOURCE_FUEL, state->mid_lat, state->mid_lon, stops, n_stops,
                      state->route_length_km, full_range_km, &state->fuel_plan);
    if (config->foot_travel_mode) {
        navit_safety_plan(config, NAVIT_SAFETY_RESOURCE_WATER, state->mid_lat, state->mid_lon, stops, n_stops,
                          state->route_length_km, full_range_km, &state->water_plan);
        navit_safety_heat_plan(config, wbgt_c, 1, &state->heat);
    } else {
        memset(&state->water_plan, 0, sizeof(state->water_plan));
        memset(&state->heat, 0, sizeof(state->heat));
    }
    navit_safety_format_status(state, config, state->status, (int)sizeof(state->status));
}

static void route_find_nearest(struct navit_safety_route_state *state, double vehicle_lat, double vehicle_lon) {
    int i;
    int best = -1;
    double best_dist = 1e18;
    struct coord_geo vehicle;
    struct coord vcoord;
    enum projection pro = projection_mg;

    state->nearest_idx = -1;
    if (!isfinite(vehicle_lat) || !isfinite(vehicle_lon))
        return;

    vehicle.lat = vehicle_lat;
    vehicle.lng = vehicle_lon;
    transform_from_geo(pro, &vehicle, &vcoord);

    for (i = 0; i < state->n_pois; i++) {
        struct coord_geo poi_geo;
        struct coord pcoord;
        double d;

        if (state->pois[i].denied)
            continue;
        if (state->pois[i].confidence >= NAVIT_SAFETY_CONFIDENCE_HIGH)
            continue;

        poi_geo.lat = state->pois[i].lat;
        poi_geo.lng = state->pois[i].lon;
        transform_from_geo(pro, &poi_geo, &pcoord);
        d = transform_distance(pro, &vcoord, &pcoord);
        if (d < best_dist) {
            best_dist = d;
            best = i;
        }
    }
    state->nearest_idx = best;
}

static struct route *route_from_navit(struct navit *nav) {
    struct attr route_attr;
    struct attr route_status;

    if (!navit_get_attr(nav, attr_route, &route_attr, NULL))
        return NULL;
    if (!route_get_attr(route_attr.u.route, attr_route_status, &route_status, NULL))
        return NULL;
    if (route_status.u.num != route_status_path_done_new && route_status.u.num != route_status_path_done_incremental)
        return NULL;
    return route_attr.u.route;
}

static void route_collect_pois(struct navit_safety_route_state *state, struct navit *nav,
                               const struct navit_safety_config *config, struct navit_safety_cache *cache,
                               const char *trip_id, struct route *route, int route_length_m, int foot_travel) {
    struct navit_safety_poi_entry *pois;
    int n_pois;
    int sample;
    enum projection pro = projection_mg;

    pois = g_new0(struct navit_safety_poi_entry, NAVIT_SAFETY_MAX_POIS);
    n_pois = 0;

    for (sample = 0; sample <= route_length_m; sample += NAVIT_SAFETY_SAMPLE_STEP_M) {
        struct coord sample_coord;
        struct pcoord pc;
        struct map_selection *sel;
        struct mapset_handle *handle;
        struct map *map;

        sample_coord = route_get_coord_dist(route, sample);
        pc.x = sample_coord.x;
        pc.y = sample_coord.y;
        pc.pro = pro;

        sel = map_selection_rect_new(&pc, NAVIT_SAFETY_CORRIDOR_HALF_M, 18);
        if (!sel)
            continue;

        handle = mapset_open(navit_get_mapset(nav));
        while ((map = mapset_next(handle, 1))) {
            struct map_rect *mr;
            struct item *item;
            struct map_selection *sel_map;

            sel_map = map_selection_dup_pro(sel, pro, map_projection(map));
            mr = map_rect_new(map, sel_map);
            if (!mr) {
                map_selection_destroy(sel_map);
                continue;
            }
            while ((item = map_rect_get_item(mr))) {
                struct coord c;
                struct coord_geo geo;
                char poi_id[64];
                int idx;
                int along_m;

                if (!item_matches_resource(item->type, foot_travel))
                    continue;
                if (!item_coord_get_pro(item, &c, 1, pro))
                    continue;
                if (!coord_rect_contains(&sel->u.c_rect, &c))
                    continue;

                route_get_distances(route, &c, 1, &along_m);
                if (along_m < 0 || along_m == INT_MAX)
                    continue;

                poi_id_from_item(item, poi_id, sizeof(poi_id));
                idx = poi_index_by_id(pois, n_pois, poi_id);
                if (idx < 0) {
                    struct navit_safety_poi_entry *entry;
                    if (n_pois >= NAVIT_SAFETY_MAX_POIS)
                        continue;
                    entry = &pois[n_pois++];
                    g_strlcpy(entry->poi_id, poi_id, sizeof(entry->poi_id));
                    poi_label_from_item(item, entry->label, sizeof(entry->label));
                    transform_to_geo(pro, &c, &geo);
                    entry->lat = geo.lat;
                    entry->lon = geo.lng;
                    entry->distance_km = along_m / 1000;
                    entry->denied = 0;
                    entry->confidence = score_poi_entry(config, cache, trip_id, poi_id, 0);
                }
            }
            map_rect_destroy(mr);
            map_selection_destroy(sel_map);
        }
        mapset_close(handle);
        map_selection_destroy(sel);
    }

    if (n_pois > 1)
        qsort(pois, n_pois, sizeof(*pois), poi_entry_compare);

    g_free(state->pois);
    state->pois = pois;
    state->n_pois = n_pois;
}

void navit_safety_route_refresh(struct navit_safety_route_state *state, struct navit *nav,
                                const struct navit_safety_config *config, struct navit_safety_cache *cache,
                                const char *trip_id, int full_range_km, double wbgt_c, double vehicle_lat,
                                double vehicle_lon) {
    struct route *route;
    struct attr length_attr;
    struct coord mid_coord;
    struct coord_geo mid_geo;
    int route_length_m;
    enum projection pro;

    if (!state || !nav || !config)
        return;

    state->valid = 0;
    state->nearest_idx = -1;
    state->status[0] = 0;

    route = route_from_navit(nav);
    if (!route) {
        navit_safety_format_status(state, config, state->status, (int)sizeof(state->status));
        return;
    }

    if (!route_get_attr(route, attr_destination_length, &length_attr, NULL) || length_attr.u.num <= 0) {
        navit_safety_format_status(state, config, state->status, (int)sizeof(state->status));
        return;
    }

    route_length_m = length_attr.u.num;
    state->route_length_km = (route_length_m + 999) / 1000;

    pro = projection_mg;
    mid_coord = route_get_coord_dist(route, route_length_m / 2);
    transform_to_geo(pro, &mid_coord, &mid_geo);
    state->mid_lat = mid_geo.lat;
    state->mid_lon = mid_geo.lng;

    route_collect_pois(state, nav, config, cache, trip_id, route, route_length_m, config->foot_travel_mode);
    route_run_plan(state, config, full_range_km, wbgt_c);
    route_find_nearest(state, vehicle_lat, vehicle_lon);
    state->valid = 1;
}

int navit_safety_route_confirm_nearest(struct navit_safety_route_state *state, struct navit *nav,
                                       const struct navit_safety_config *config, struct navit_safety_cache *cache,
                                       const char *trip_id, int full_range_km, double wbgt_c, double vehicle_lat,
                                       double vehicle_lon) {
    struct navit_safety_poi_entry *entry;

    (void)nav;
    if (!state || state->nearest_idx < 0 || state->nearest_idx >= state->n_pois)
        return -1;

    entry = &state->pois[state->nearest_idx];
#    if NAVIT_SAFETY_WITH_SQLITE
    if (cache && trip_id) {
        time_t now = time(NULL);
        navit_safety_cache_confirm(cache, entry->poi_id, trip_id, (long)now);
    }
#    else
    (void)cache;
    (void)trip_id;
#    endif
    entry->denied = 0;
    entry->confidence = NAVIT_SAFETY_CONFIDENCE_HIGH;
    route_run_plan(state, config, full_range_km, wbgt_c);
    route_find_nearest(state, vehicle_lat, vehicle_lon);
    return 0;
}

int navit_safety_route_deny_nearest(struct navit_safety_route_state *state, struct navit *nav,
                                    const struct navit_safety_config *config, struct navit_safety_cache *cache,
                                    const char *trip_id, int full_range_km, double wbgt_c, double vehicle_lat,
                                    double vehicle_lon) {
    struct navit_safety_poi_entry *entry;

    (void)nav;
    (void)cache;
    (void)trip_id;
    if (!state || state->nearest_idx < 0 || state->nearest_idx >= state->n_pois)
        return -1;

    entry = &state->pois[state->nearest_idx];
    entry->denied = 1;
    entry->confidence = NAVIT_SAFETY_CONFIDENCE_LOW;
    route_run_plan(state, config, full_range_km, wbgt_c);
    route_find_nearest(state, vehicle_lat, vehicle_lon);
    return 0;
}

#endif /* NAVIT_SAFETY_WITH_OSD */
