/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "driver_break_fuel_core.h"
#include "debug.h"
#include "driver_break_db.h"
#include "driver_break_poi.h"
#include "driver_break_poi_map.h"
#include "mapset.h"
#include "navit.h"
#include "route.h"
#include <glib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

static double driver_break_coord_distance_geo(struct coord_geo *c1, struct coord_geo *c2) {
    double lat1 = c1->lat * M_PI / 180.0;
    double lat2 = c2->lat * M_PI / 180.0;
    double dlat = (c2->lat - c1->lat) * M_PI / 180.0;
    double dlng = (c2->lng - c1->lng) * M_PI / 180.0;
    double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlng / 2) * sin(dlng / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return 6371000.0 * c;
}

static void driver_break_rolling_update(struct driver_break_priv *priv, double distance_m, double fuel_used,
                                        double inst_l_per_100) {
    priv->rolling_short_distance_m += distance_m;
    priv->rolling_short_fuel += fuel_used;
    priv->rolling_long_distance_m += distance_m;
    priv->rolling_long_fuel += fuel_used;
    if (priv->rolling_short_distance_m > 20000.0) {
        double scale = 20000.0 / priv->rolling_short_distance_m;
        priv->rolling_short_distance_m *= scale;
        priv->rolling_short_fuel *= scale;
    }
    if (priv->rolling_long_distance_m > 500000.0) {
        double scale = 500000.0 / priv->rolling_long_distance_m;
        priv->rolling_long_distance_m *= scale;
        priv->rolling_long_fuel *= scale;
    }
    if (inst_l_per_100 > priv->peak_consumption_l_per_100km) {
        priv->peak_consumption_l_per_100km = inst_l_per_100;
    }
    priv->trip_total_distance_m += distance_m;
    priv->trip_total_fuel += fuel_used;
}

static double driver_break_fuel_baseline(struct driver_break_priv *priv, double long_avg) {
    if (long_avg > 0.0) {
        return long_avg;
    }
    if (priv->config.fuel_avg_consumption_x10 > 0) {
        return priv->config.fuel_avg_consumption_x10 / 10.0;
    }
    return 0.0;
}

static void driver_break_notify_high_load(struct driver_break_priv *priv, double short_avg, double baseline) {
    char msg[128];
    double delta_pct = ((short_avg / baseline) - 1.0) * 100.0;

    snprintf(msg, sizeof(msg), "Driver Break plugin: Fuel consumption is %.0f%% above baseline – range adjusted",
             delta_pct);
    navit_add_message(priv->nav, msg);
}

static void driver_break_apply_baseline_and_high_load(struct driver_break_priv *priv, double short_avg,
                                                      double long_avg) {
    double baseline;
    double threshold;
    double limit;

    if (short_avg > 0.0 && priv->fuel_current > 0.0) {
        priv->fuel_remaining_range = (priv->fuel_current / short_avg) * 100.0;
    }

    baseline = driver_break_fuel_baseline(priv, long_avg);
    if (baseline <= 0.0 || short_avg <= 0.0) {
        return;
    }

    threshold = (double)priv->config.fuel_high_load_threshold;
    limit = baseline * (1.0 + threshold / 100.0);
    if (!priv->high_load_active && short_avg > limit) {
        priv->high_load_active = 1;
        if (priv->nav) {
            driver_break_notify_high_load(priv, short_avg, baseline);
        }
    } else if (priv->high_load_active && short_avg <= baseline) {
        priv->high_load_active = 0;
    }
}

static double driver_break_fuel_sample_distance(struct driver_break_priv *priv, struct coord_geo *pos, time_t now,
                                                double *dt_h_out) {
    double dt_s;
    double distance_m;

    if (priv->last_sample_time == 0) {
        priv->last_sample_coord = *pos;
        priv->last_sample_time = now;
        if (priv->trip_start_time == 0) {
            priv->trip_start_time = now;
        }
        return -1.0;
    }

    dt_s = difftime(now, priv->last_sample_time);
    if (dt_s <= 0.0) {
        return -1.0;
    }

    distance_m = driver_break_coord_distance_geo(&priv->last_sample_coord, pos);
    if (distance_m <= 1.0) {
        return -1.0;
    }

    *dt_h_out = dt_s / 3600.0;
    return distance_m;
}

static void driver_break_fuel_compute_averages(struct driver_break_priv *priv, double *short_avg_out,
                                               double *long_avg_out) {
    *short_avg_out = 0.0;
    *long_avg_out = 0.0;

    if (priv->rolling_short_distance_m > 1000.0 && priv->rolling_short_fuel > 0.0) {
        *short_avg_out = (priv->rolling_short_fuel / (priv->rolling_short_distance_m / 1000.0)) * 100.0;
    }
    if (priv->rolling_long_distance_m > 10000.0 && priv->rolling_long_fuel > 0.0) {
        *long_avg_out = (priv->rolling_long_fuel / (priv->rolling_long_distance_m / 1000.0)) * 100.0;
    }
}

static int driver_break_fuel_compute_sample(struct driver_break_priv *priv, struct coord_geo *pos, time_t now,
                                            double *distance_m_out, double *fuel_used_out, double *inst_l_per_100_out,
                                            double *speed_kmh_out) {
    double dt_h;
    double distance_m;
    double fuel_used;
    double distance_km;
    double speed_kmh;
    double inst_l_per_100;

    distance_m = driver_break_fuel_sample_distance(priv, pos, now, &dt_h);
    if (distance_m < 0.0) {
        return 0;
    }

    fuel_used = priv->fuel_rate_l_h * dt_h;
    if (fuel_used <= 0.0) {
        return 0;
    }

    distance_km = distance_m / 1000.0;
    speed_kmh = distance_km / dt_h;
    inst_l_per_100 = (distance_km > 0.0) ? (fuel_used / distance_km) * 100.0 : 0.0;

    *distance_m_out = distance_m;
    *fuel_used_out = fuel_used;
    *inst_l_per_100_out = inst_l_per_100;
    *speed_kmh_out = speed_kmh;
    return 1;
}

void driver_break_update_fuel_learning(struct driver_break_priv *priv, struct coord_geo *pos, time_t now) {
    double distance_m;
    double fuel_used;
    double inst_l_per_100;
    double speed_kmh;
    double short_avg;
    double long_avg;

    if (!priv || !pos || priv->fuel_rate_l_h <= 0.0) {
        return;
    }

    if (!driver_break_fuel_compute_sample(priv, pos, now, &distance_m, &fuel_used, &inst_l_per_100, &speed_kmh)) {
        return;
    }

    if (priv->db) {
        driver_break_db_add_fuel_sample(priv->db, now, distance_m, fuel_used, inst_l_per_100, speed_kmh, -1);
    }

    driver_break_rolling_update(priv, distance_m, fuel_used, inst_l_per_100);
    driver_break_fuel_compute_averages(priv, &short_avg, &long_avg);
    driver_break_apply_baseline_and_high_load(priv, short_avg, long_avg);

    priv->last_sample_coord = *pos;
    priv->last_sample_time = now;
}

static int driver_break_fuel_range_below_threshold(struct driver_break_priv *priv, double distance_km,
                                                   double buffer_km) {
    double threshold;

    if (distance_km <= 0.0 || priv->fuel_remaining_range <= 0.0) {
        return 0;
    }

    threshold = distance_km + buffer_km;
    if (priv->fuel_remaining_range >= threshold) {
        return 0;
    }
    if (priv->fuel_remaining_range >= (double)priv->config.fuel_low_warning_km) {
        return 0;
    }
    return 1;
}

static GList *driver_break_fuel_search_stations(struct driver_break_priv *priv, double buffer_km) {
    struct mapset *ms = navit_get_mapset(priv->nav);
    struct attr pos_attr;
    struct coord_geo center;
    double search_radius_km;

    if (!ms || !navit_get_attr(priv->nav, attr_position_coord_geo, &pos_attr, NULL) || !pos_attr.u.coord_geo) {
        return NULL;
    }

    center = *pos_attr.u.coord_geo;
    search_radius_km = priv->fuel_remaining_range;
    if (search_radius_km < buffer_km) {
        search_radius_km = buffer_km;
    }

    return driver_break_poi_map_search_fuel(&center, search_radius_km, ms, priv->config.vehicle_type,
                                            priv->config.fuel_type);
}

void driver_break_check_fuel_low_range(struct driver_break_priv *priv) {
    struct attr dist_attr;
    double distance_km = 0.0;
    double buffer_km;
    GList *fuel_pois;

    if (!priv->current_route) {
        return;
    }

    buffer_km = priv->config.fuel_search_buffer_km > 0 ? priv->config.fuel_search_buffer_km : 20.0;

    if (route_get_attr(priv->current_route, attr_destination_length, &dist_attr, NULL)) {
        distance_km = (double)dist_attr.u.num / 1000.0;
    }

    if (!driver_break_fuel_range_below_threshold(priv, distance_km, buffer_km)) {
        return;
    }

    fuel_pois = driver_break_fuel_search_stations(priv, buffer_km);
    if (fuel_pois) {
        int count = g_list_length(fuel_pois);
        char msg[128];

        snprintf(msg, sizeof(msg), "Driver Break plugin: Low range (%.0f km). Found %d fuel stations within %.0f km.",
                 priv->fuel_remaining_range, count, priv->fuel_remaining_range);
        navit_add_message(priv->nav, msg);
        dbg(lvl_info, "%s", msg);
        driver_break_poi_free_list(fuel_pois);
        return;
    }

    navit_add_message(priv->nav, "Driver Break plugin: Low range and no fuel stations found within range");
    dbg(lvl_info, "Driver Break plugin: Low range (%.0f km) and no fuel stations found within threshold",
        priv->fuel_remaining_range);
}
