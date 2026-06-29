/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Plugin-local elevation-aware energy / kinetic route analysis.
 */

#include "driver_break_energy_route.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "attr.h"
#include "attr_type_def.h"
#include "coord.h"
#include "debug.h"
#include "driver_break.h"
#include "driver_break_energy.h"
#include "driver_break_route_validator.h"
#include "driver_break_srtm.h"
#include "item.h"
#include "map.h"
#include "navit.h"
#include "projection.h"
#include "route.h"
#include "transform.h"

#define SRTM_VOID_ELEVATION (-32768)
#define DEFAULT_SPEED_MPS   (50.0 / 3.6)

double driver_break_coord_distance_geo(const struct coord_geo *c1, const struct coord_geo *c2) {
    double lat1;
    double lat2;
    double dlat;
    double dlng;
    double a;
    double c;

    if (!c1 || !c2)
        return 0.0;

    lat1 = c1->lat * M_PI / 180.0;
    lat2 = c2->lat * M_PI / 180.0;
    dlat = (c2->lat - c1->lat) * M_PI / 180.0;
    dlng = (c2->lng - c1->lng) * M_PI / 180.0;

    a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlng / 2) * sin(dlng / 2);
    c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return 6371000.0 * c;
}

static int driver_break_route_collect_coord(struct coord_geo *geo, void *user_data) {
    double *bbox = (double *)user_data;

    if (geo->lng < bbox[0])
        bbox[0] = geo->lng;
    if (geo->lat < bbox[1])
        bbox[1] = geo->lat;
    if (geo->lng > bbox[2])
        bbox[2] = geo->lng;
    if (geo->lat > bbox[3])
        bbox[3] = geo->lat;
    return 1;
}

static int driver_break_route_foreach_coord(struct route *route,
                                            int (*cb)(struct coord_geo *geo, void *user_data), void *user_data) {
    struct map *route_map;
    struct map_rect *mr;
    struct item *item;
    int count = 0;

    if (!route || !cb)
        return 0;

    route_map = route_get_map(route);
    if (!route_map)
        return 0;

    mr = map_rect_new(route_map, NULL);
    if (!mr)
        return 0;

    while ((item = map_rect_get_item(mr))) {
        struct coord c;
        struct coord_geo geo;

        if (item->type != type_street_route)
            continue;

        item_coord_rewind(item);
        while (item_coord_get(item, &c, 1) > 0) {
            transform_to_geo(projection_mg, &c, &geo);
            if (!cb(&geo, user_data))
                goto done;
            count++;
        }
    }

done:
    map_rect_destroy(mr);
    return count;
}

int driver_break_route_bbox(struct route *route, double *min_lon, double *min_lat, double *max_lon, double *max_lat) {
    double bbox[4] = {180.0, 90.0, -180.0, -90.0};
    int n;

    if (!min_lon || !min_lat || !max_lon || !max_lat)
        return 0;

    n = driver_break_route_foreach_coord(route, driver_break_route_collect_coord, bbox);
    if (n < 1)
        return 0;

    *min_lon = bbox[0];
    *min_lat = bbox[1];
    *max_lon = bbox[2];
    *max_lat = bbox[3];
    return 1;
}

void driver_break_post_validation_warnings(struct navit *nav, struct route_validation_result *result) {
    GList *l;

    if (!nav || !result)
        return;

    for (l = result->warnings; l; l = l->next) {
        if (l->data)
            navit_add_message(nav, (char *)l->data);
    }
    if (!result->is_valid)
        navit_add_message(nav, "Driver Break: route validation reported issues");
}

static void driver_break_warn_missing_srtm(struct driver_break_priv *priv, int missing, int total, const char *where) {
    char msg[320];

    if (!priv || !priv->nav || missing <= 0)
        return;

    snprintf(msg, sizeof(msg),
             "Driver Break: elevation data missing for %s (%d of %d tiles). "
             "Kinetic routing needs SRTM tiles; use srtm_download_here, srtm_download_route, or srtm_download_menu.",
             where ? where : "area", missing, total);
    navit_add_message(priv->nav, msg);
}

void driver_break_check_srtm_coverage(struct driver_break_priv *priv) {
    double min_lon;
    double min_lat;
    double max_lon;
    double max_lat;
    int total = 0;
    int missing;
    struct attr pos_attr;

    if (!priv || !priv->config.use_energy_routing)
        return;

    if (!srtm_get_data_directory())
        srtm_init(NULL);

    if (priv->current_route && driver_break_route_bbox(priv->current_route, &min_lon, &min_lat, &max_lon, &max_lat)) {
        missing = srtm_bbox_missing_tile_count(min_lon, min_lat, max_lon, max_lat, &total);
        driver_break_warn_missing_srtm(priv, missing, total, "planned route");
        return;
    }

    if (navit_get_attr(priv->nav, attr_position_coord_geo, &pos_attr, NULL) && pos_attr.u.coord_geo) {
        struct coord_geo *cg = pos_attr.u.coord_geo;
        min_lon = floor(cg->lng) - 0.5;
        max_lon = floor(cg->lng) + 1.5;
        min_lat = floor(cg->lat) - 0.5;
        max_lat = floor(cg->lat) + 1.5;
        missing = srtm_bbox_missing_tile_count(min_lon, min_lat, max_lon, max_lat, &total);
        driver_break_warn_missing_srtm(priv, missing, total, "current position");
    }
}

static double driver_break_segment_speed_mps(struct item *item) {
    struct attr attr;

    if (item && item_attr_get(item, attr_maxspeed, &attr) && attr.u.num > 0)
        return (double)attr.u.num / 3.6;
    return DEFAULT_SPEED_MPS;
}

struct energy_route_accum {
    struct driver_break_priv *priv;
    struct energy_model model;
    struct coord_geo prev;
    int have_prev;
    int missing_srtm;
};

static int driver_break_energy_route_step(struct coord_geo *geo, void *user_data) {
    struct energy_route_accum *acc = (struct energy_route_accum *)user_data;
    struct driver_break_priv *priv;
    int elev;
    int prev_elev;
    double dist;
    double delta_h;
    double speed;
    struct energy_result seg;

    if (!acc || !geo)
        return 0;

    priv = acc->priv;
    elev = srtm_get_elevation(geo);
    if (elev == SRTM_VOID_ELEVATION) {
        acc->missing_srtm = 1;
        elev = 0;
    }

    if (!acc->have_prev) {
        acc->prev = *geo;
        acc->have_prev = 1;
        return 1;
    }

    dist = driver_break_coord_distance_geo(&acc->prev, geo);
    if (dist < 0.5) {
        acc->prev = *geo;
        return 1;
    }

    prev_elev = srtm_get_elevation(&acc->prev);
    if (prev_elev == SRTM_VOID_ELEVATION) {
        acc->missing_srtm = 1;
        prev_elev = elev;
    }
    delta_h = (double)elev - (double)prev_elev;
    speed = DEFAULT_SPEED_MPS;

    if (priv->config.use_ecu_route_cost && priv->fuel_rate_l_h > 0.0) {
        double time_s = dist / speed;
        double fuel_l = priv->fuel_rate_l_h * (time_s / 3600.0);
        priv->route_energy_j += fuel_l * 36e6;
        priv->route_energy_time_s += time_s;
        priv->route_energy_cost += fuel_l;
        priv->route_energy_est_l += fuel_l;
    } else {
        seg = energy_calculate_segment(&acc->model, dist, delta_h, (double)elev, speed);
        priv->route_energy_j += seg.energy;
        priv->route_energy_time_s += seg.time;
        priv->route_energy_cost += seg.cost;
        if (priv->config.fuel_avg_consumption_x10 > 0) {
            double l_per_100 = priv->config.fuel_avg_consumption_x10 / 10.0;
            priv->route_energy_est_l += (dist / 1000.0) * (l_per_100 / 100.0);
        }
    }

    priv->route_energy_distance_m += dist;
    acc->prev = *geo;
    return 1;
}

static int driver_break_energy_route_segment(struct item *item, void *user_data) {
    struct energy_route_accum *acc = (struct energy_route_accum *)user_data;
    struct coord c;
    struct coord_geo geo;
    double speed;

    if (!item || !acc)
        return 0;

    speed = driver_break_segment_speed_mps(item);
    (void)speed;

    item_coord_rewind(item);
    while (item_coord_get(item, &c, 1) > 0) {
        transform_to_geo(projection_mg, &c, &geo);
        if (!driver_break_energy_route_step(&geo, user_data))
            return 0;
    }
    return 1;
}

void driver_break_compute_route_energy(struct driver_break_priv *priv) {
    struct energy_route_accum acc;
    struct map *route_map;
    struct map_rect *mr;
    struct item *item;
    char msg[256];

    if (!priv || !priv->current_route || !priv->config.use_energy_routing)
        return;

    priv->route_energy_j = 0.0;
    priv->route_energy_time_s = 0.0;
    priv->route_energy_cost = 0.0;
    priv->route_energy_distance_m = 0.0;
    priv->route_energy_est_l = 0.0;
    priv->route_energy_valid = 0;
    priv->route_energy_missing_srtm = 0;

    if (!srtm_get_data_directory())
        srtm_init(NULL);

    memset(&acc, 0, sizeof(acc));
    acc.priv = priv;
    driver_break_energy_model_from_config(&acc.model, &priv->config);

    route_map = route_get_map(priv->current_route);
    if (!route_map)
        return;

    mr = map_rect_new(route_map, NULL);
    if (!mr)
        return;

    while ((item = map_rect_get_item(mr))) {
        if (item->type != type_street_route)
            continue;
        if (!driver_break_energy_route_segment(item, &acc))
            break;
    }
    map_rect_destroy(mr);

    priv->route_energy_missing_srtm = acc.missing_srtm;
    priv->route_energy_valid = (priv->route_energy_distance_m > 1.0) ? 1 : 0;

    if (!priv->nav || !priv->route_energy_valid)
        return;

    if (priv->route_energy_missing_srtm) {
        snprintf(msg, sizeof(msg),
                 "Driver Break: kinetic route estimate incomplete (missing elevation tiles along route)");
        navit_add_message(priv->nav, msg);
    }

    snprintf(msg, sizeof(msg),
             "Driver Break: kinetic route %.1f km, cost %.1f, est. fuel %.1f L%s", priv->route_energy_distance_m / 1000.0,
             priv->route_energy_cost, priv->route_energy_est_l,
             priv->config.use_ecu_route_cost ? " (ECU-weighted)" : "");
    navit_add_message(priv->nav, msg);
}
