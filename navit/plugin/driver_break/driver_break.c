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

#include "attr.h"
#include "callback.h"
#include "config.h"
#include "coord.h"
#include "debug.h"
#include "driver_break_cycling.h"
#include "driver_break_db.h"
#include "driver_break_energy.h"
#include "driver_break_finder.h"
#include "driver_break_glacier.h"
#include "driver_break_hiking.h"
#include "driver_break_osd.h"
#include "driver_break_poi.h"
#include "driver_break_poi_hiking.h"
#include "driver_break_poi_map.h"
#include "driver_break_route_validator.h"
#include "driver_break_srtm.h"
#include "driver_break_srtm_osd.h"
#include "driver_break_obd.h"
#include "driver_break_j1939.h"
#include "event.h"
#include "file.h"
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "plugin.h"
#include "projection.h"
#include "transform.h"
#include <glib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* item_def.h included via item.h */
#include "color.h"
#include "command.h"
#include "mapset.h"
#include "menu.h"
#include "navigation.h"
#include "navit.h"
#include "osd.h"
#include "popup.h"
#include "route.h"
#include "vehicle.h"

/* Get database path from attributes or use default */
static char *driver_break_get_database_path(struct attr **attrs) {
    char *db_path = NULL;
    struct attr *db_path_attr = attr_search(attrs, attr_data);

    if (db_path_attr && db_path_attr->u.str) {
        /* Expand environment variables in path (e.g., $HOME) */
        struct file_wordexp *we = file_wordexp_new(db_path_attr->u.str);
        int count = file_wordexp_get_count(we);
        char **array = file_wordexp_get_array(we);
        if (count > 0 && array[0]) {
            db_path = g_strdup(array[0]);
        } else {
            /* Fallback to default if expansion fails */
            const char *user_data_dir = navit_get_user_data_directory(1);
            if (user_data_dir) {
                db_path = g_build_filename(user_data_dir, "driver_break_plugin.db", NULL);
            }
        }
        file_wordexp_destroy(we);
    } else {
        /* Default database path */
        const char *user_data_dir = navit_get_user_data_directory(1);
        if (user_data_dir) {
            db_path = g_build_filename(user_data_dir, "driver_break_plugin.db", NULL);
        }
    }

    return db_path;
}

/* Default configuration */
void driver_break_config_default(struct driver_break_config *config) {
    /* Zero-initialize entire structure first */
    memset(config, 0, sizeof(*config));

    /* Car defaults */
    config->car_soft_limit_hours = 7;
    config->car_max_hours = 10;
    config->car_break_interval_hours = 4;
    config->car_break_duration_min = 30;

    /* Truck defaults (EU Regulation EC 561/2006) */
    config->truck_mandatory_break_after_hours = 4;
    config->truck_mandatory_break_duration_min = 45;
    config->truck_max_daily_hours = 9;

    /* Hiking defaults */
    config->hiking_driver_break_distance_main = 11295.0; /* 11.295 km */
    config->hiking_driver_break_distance_alt = 2275.2;   /* 2.275 km */
    config->hiking_max_daily_distance = 40000.0;         /* 40 km (46.4 km when DNT priority enabled) */

    /* Cycling defaults */
    config->cycling_driver_break_distance_main = 28240.0; /* 28.24 km */
    config->cycling_driver_break_distance_alt = 5690.0;   /* 5.69 km */
    config->cycling_max_daily_distance = 100000.0;        /* 100 km */

    /* Rest stop defaults - ALL fields must be initialized */
    config->min_distance_from_buildings = 150;
    config->min_distance_from_glaciers = 300; /* 300m for nightly */
    config->poi_search_radius_km = 15;
    config->poi_water_search_radius_km = 2;    /* 2 km for water */
    config->poi_cabin_search_radius_km = 5;    /* 5 km for cabins */
    config->network_hut_search_radius_km = 25; /* 25 km for network huts (DNT, STF) */

    /* Network and route priority defaults */
    config->enable_dnt_priority = 0;               /* Disabled by default */
    config->enable_hiking_pilgrimage_priority = 0; /* Disabled by default */

    /* Rest interval defaults */
    config->driver_break_interval_min_hours = 1;
    config->driver_break_interval_max_hours = 6;

    /* Energy routing defaults */
    config->use_energy_routing = 0; /* Disabled by default */
    config->total_weight = 80.0;    /* 80 kg default (person + gear) */

    /* Fuel profile defaults */
    config->fuel_type = DRIVER_BREAK_FUEL_PETROL;
    config->fuel_tank_capacity_l = 50;    /* 50 L tank */
    config->fuel_avg_consumption_x10 = 75; /* 7.5 L/100km */
    config->fuel_obd_available = 0;
    config->fuel_j1939_available = 0;
    config->fuel_ethanol_manual_pct = 0;
    config->fuel_low_warning_km = 80;     /* warn when <80 km remaining */
    config->fuel_search_buffer_km = 20;   /* default buffer for fuel search */
    config->fuel_high_load_threshold = 25; /* 25% above baseline */

    config->vehicle_type = DRIVER_BREAK_VEHICLE_CAR; /* Default to car */

    dbg(lvl_debug,
        "Driver Break plugin: Config defaults initialized - buildings=%d, glaciers=%d, poi=%d, water=%d, cabin=%d",
        config->min_distance_from_buildings, config->min_distance_from_glaciers, config->poi_search_radius_km,
        config->poi_water_search_radius_km, config->poi_cabin_search_radius_km);
}

/* Calculate driving time and check for mandatory breaks */
static void driver_break_check_driving_time(struct driver_break_priv *priv) {
    time_t now = time(NULL);
    struct driver_break_config *config = &priv->config;
    int mandatory_break = 0;

    if (priv->session.start_time == 0) {
        priv->session.start_time = now;
        priv->session.last_break_time = now;
        return;
    }

    /* Calculate continuous driving time since last break */
    priv->session.continuous_driving_minutes = (int)((now - priv->session.last_break_time) / 60);

    /* Check for mandatory break based on vehicle type */
    if (config->vehicle_type == DRIVER_BREAK_VEHICLE_TRUCK) {
        if (priv->session.continuous_driving_minutes >= (config->truck_mandatory_break_after_hours * 60)) {
            mandatory_break = 1;
        }
    } else if (config->vehicle_type == DRIVER_BREAK_VEHICLE_CAR) {
        if (priv->session.continuous_driving_minutes >= (config->car_break_interval_hours * 60)) {
            /* Recommended break, not mandatory for cars */
            mandatory_break = 0;
        }
    } else if (config->vehicle_type == DRIVER_BREAK_VEHICLE_HIKING
               || config->vehicle_type == DRIVER_BREAK_VEHICLE_CYCLING) {
        /* For hiking/cycling, breaks are suggested based on distance, not time */
        /* This is handled separately in rest stop calculation */
        mandatory_break = 0;
    }

    priv->session.mandatory_break_required = mandatory_break;

    if (mandatory_break) {
        dbg(lvl_info, "Driver Break plugin: Mandatory break required after %d minutes of driving",
            priv->session.continuous_driving_minutes);
    }
}

/* Calculate distance between two coordinates in meters (Haversine) */
static double driver_break_coord_distance_geo(struct coord_geo *c1, struct coord_geo *c2) {
    double lat1 = c1->lat * M_PI / 180.0;
    double lat2 = c2->lat * M_PI / 180.0;
    double dlat = (c2->lat - c1->lat) * M_PI / 180.0;
    double dlng = (c2->lng - c1->lng) * M_PI / 180.0;

    double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlng / 2) * sin(dlng / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return 6371000.0 * c;
}

/* Update rolling windows and trip totals from a single sample. */
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
    if (inst_l_per_100 > priv->peak_consumption_l_per_100km)
        priv->peak_consumption_l_per_100km = inst_l_per_100;
    priv->trip_total_distance_m += distance_m;
    priv->trip_total_fuel += fuel_used;
}

/* Compute range from short_avg and run high-load detection vs baseline. */
static void driver_break_apply_baseline_and_high_load(struct driver_break_priv *priv, double short_avg,
                                                      double long_avg) {
    if (short_avg > 0.0 && priv->fuel_current > 0.0)
        priv->fuel_remaining_range = (priv->fuel_current / short_avg) * 100.0;

    double baseline = long_avg > 0.0 ? long_avg
                                    : (priv->config.fuel_avg_consumption_x10 > 0
                                           ? priv->config.fuel_avg_consumption_x10 / 10.0
                                           : 0.0);
    if (baseline <= 0.0 || short_avg <= 0.0)
        return;
    double threshold = (double)priv->config.fuel_high_load_threshold;
    double limit = baseline * (1.0 + threshold / 100.0);
    if (!priv->high_load_active && short_avg > limit) {
        priv->high_load_active = 1;
        if (priv->nav) {
            char msg[128];
            double delta_pct = ((short_avg / baseline) - 1.0) * 100.0;
            snprintf(msg, sizeof(msg),
                     "Driver Break plugin: Fuel consumption is %.0f%% above baseline – range adjusted", delta_pct);
            navit_add_message(priv->nav, msg);
        }
    } else if (priv->high_load_active && short_avg <= baseline) {
        priv->high_load_active = 0;
    }
}

/* Update adaptive fuel learning based on current position and fuel rate */
static void driver_break_update_fuel_learning(struct driver_break_priv *priv, struct coord_geo *pos, time_t now) {
    if (!priv || !pos || priv->fuel_rate_l_h <= 0.0)
        return;

    if (priv->last_sample_time == 0) {
        priv->last_sample_coord = *pos;
        priv->last_sample_time = now;
        if (priv->trip_start_time == 0)
            priv->trip_start_time = now;
        return;
    }

    double dt_s = difftime(now, priv->last_sample_time);
    if (dt_s <= 0.0)
        return;
    double distance_m = driver_break_coord_distance_geo(&priv->last_sample_coord, pos);
    if (distance_m <= 1.0)
        return;

    double dt_h = dt_s / 3600.0;
    double fuel_used = priv->fuel_rate_l_h * dt_h;
    if (fuel_used <= 0.0)
        return;

    double distance_km = distance_m / 1000.0;
    double speed_kmh = distance_km / dt_h;
    double inst_l_per_100 = (distance_km > 0.0) ? (fuel_used / distance_km) * 100.0 : 0.0;

    if (priv->db)
        driver_break_db_add_fuel_sample(priv->db, now, distance_m, fuel_used, inst_l_per_100, speed_kmh, -1);

    driver_break_rolling_update(priv, distance_m, fuel_used, inst_l_per_100);

    double short_avg = 0.0;
    double long_avg = 0.0;
    if (priv->rolling_short_distance_m > 1000.0 && priv->rolling_short_fuel > 0.0)
        short_avg = (priv->rolling_short_fuel / (priv->rolling_short_distance_m / 1000.0)) * 100.0;
    if (priv->rolling_long_distance_m > 10000.0 && priv->rolling_long_fuel > 0.0)
        long_avg = (priv->rolling_long_fuel / (priv->rolling_long_distance_m / 1000.0)) * 100.0;

    driver_break_apply_baseline_and_high_load(priv, short_avg, long_avg);

    priv->last_sample_coord = *pos;
    priv->last_sample_time = now;
}

/* Vehicle callback wrapper */
static void driver_break_vehicle_callback_wrapper(void *priv_data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)priv_data;
    if (!priv)
        return;

    /* Get position from navit */
    struct attr attr;
    if (navit_get_attr(priv->nav, attr_position_coord_geo, &attr, NULL)) {
        if (attr.u.coord_geo) {
            /* Vehicle is moving, update driving time */
            driver_break_check_driving_time(priv);

            /* Update adaptive fuel learning using GPS distance and current fuel rate */
            driver_break_update_fuel_learning(priv, attr.u.coord_geo, time(NULL));
        }
    }
}

/* Route callback wrapper */
/* Convert hiking rest stops to driver_break_stop format and add POIs */
static void process_hiking_stops(struct driver_break_priv *priv, GList *hiking_stops) {
    GList *l = hiking_stops;
    while (l) {
        struct hiking_driver_break_stop *h_stop = (struct hiking_driver_break_stop *)l->data;
        if (h_stop) {
            /* Get coordinate at distance along route */
            struct coord c = route_get_coord_dist(priv->current_route, (int)h_stop->position);
            transform_to_geo(projection_mg, &c, &h_stop->coord);

            /* Create driver_break_stop from hiking_driver_break_stop */
            struct driver_break_stop *stop = g_new0(struct driver_break_stop, 1);
            stop->coord = h_stop->coord;
            stop->distance_from_route = 0;
            stop->name = g_strdup_printf("Hiking rest stop at %.1f km", h_stop->position / 1000.0);

            /* Search for POIs using map data if available */
            struct mapset *ms = navit_get_mapset(priv->nav);
            if (ms) {
                GList *water_pois_list =
                    poi_search_water_points_map(&h_stop->coord, priv->config.poi_water_search_radius_km, ms);
                GList *cabin_pois_list = poi_search_cabins_map(&h_stop->coord, priv->config.poi_cabin_search_radius_km,
                                                               ms, priv->config.enable_dnt_priority);

                /* Convert water points and cabins to driver_break_poi format */
                GList *poi_list = water_pois_list;
                while (poi_list) {
                    struct water_point *wp = (struct water_point *)poi_list->data;
                    if (wp) {
                        struct driver_break_poi *poi = g_new0(struct driver_break_poi, 1);
                        poi->coord = wp->coord;
                        poi->name = wp->name ? g_strdup(wp->name) : NULL;
                        poi->type = g_strdup("amenity");
                        poi->category = wp->type ? g_strdup(wp->type) : NULL;
                        poi->distance_from_driver_break_stop = wp->distance_from_driver_break_stop;
                        stop->pois = g_list_append(stop->pois, poi);
                    }
                    poi_list = g_list_next(poi_list);
                }

                poi_list = cabin_pois_list;
                while (poi_list) {
                    struct cabin_hut *cabin = (struct cabin_hut *)poi_list->data;
                    if (cabin) {
                        struct driver_break_poi *poi = g_new0(struct driver_break_poi, 1);
                        poi->coord = cabin->coord;
                        poi->name = cabin->name ? g_strdup(cabin->name) : NULL;
                        poi->type = g_strdup("tourism");
                        poi->category = cabin->type ? g_strdup(cabin->type) : NULL;
                        poi->distance_from_driver_break_stop = cabin->distance_from_driver_break_stop;
                        stop->pois = g_list_append(stop->pois, poi);
                    }
                    poi_list = g_list_next(poi_list);
                }

                poi_free_water_points(water_pois_list);
                poi_free_cabins(cabin_pois_list);
            } else {
                stop->pois = driver_break_poi_discover(&h_stop->coord, priv->config.poi_search_radius_km, NULL, 0);
            }

            priv->suggested_stops = g_list_append(priv->suggested_stops, stop);
        }
        l = g_list_next(l);
    }
}

/* Convert cycling rest stops to driver_break_stop format and add POIs */
static void process_cycling_stops(struct driver_break_priv *priv, GList *cycling_stops) {
    GList *l = cycling_stops;
    while (l) {
        struct cycling_driver_break_stop *c_stop = (struct cycling_driver_break_stop *)l->data;
        if (c_stop) {
            /* Get coordinate at distance along route */
            struct coord c = route_get_coord_dist(priv->current_route, (int)c_stop->position);
            transform_to_geo(projection_mg, &c, &c_stop->coord);

            /* Create driver_break_stop from cycling_driver_break_stop */
            struct driver_break_stop *stop = g_new0(struct driver_break_stop, 1);
            stop->coord = c_stop->coord;
            stop->distance_from_route = 0;
            stop->name = g_strdup_printf("Cycling rest stop at %.1f km", c_stop->position / 1000.0);

            /* Search for POIs using map data if available */
            struct mapset *ms = navit_get_mapset(priv->nav);
            if (ms) {
                GList *water_pois_list =
                    poi_search_water_points_map(&c_stop->coord, priv->config.poi_water_search_radius_km, ms);
                GList *cabin_pois_list = poi_search_cabins_map(&c_stop->coord, priv->config.poi_cabin_search_radius_km,
                                                               ms, priv->config.enable_dnt_priority);

                /* Convert water points and cabins to driver_break_poi format */
                GList *poi_list = water_pois_list;
                while (poi_list) {
                    struct water_point *wp = (struct water_point *)poi_list->data;
                    if (wp) {
                        struct driver_break_poi *poi = g_new0(struct driver_break_poi, 1);
                        poi->coord = wp->coord;
                        poi->name = wp->name ? g_strdup(wp->name) : NULL;
                        poi->type = g_strdup("amenity");
                        poi->category = wp->type ? g_strdup(wp->type) : NULL;
                        poi->distance_from_driver_break_stop = wp->distance_from_driver_break_stop;
                        stop->pois = g_list_append(stop->pois, poi);
                    }
                    poi_list = g_list_next(poi_list);
                }

                poi_list = cabin_pois_list;
                while (poi_list) {
                    struct cabin_hut *cabin = (struct cabin_hut *)poi_list->data;
                    if (cabin) {
                        struct driver_break_poi *poi = g_new0(struct driver_break_poi, 1);
                        poi->coord = cabin->coord;
                        poi->name = cabin->name ? g_strdup(cabin->name) : NULL;
                        poi->type = g_strdup("tourism");
                        poi->category = cabin->type ? g_strdup(cabin->type) : NULL;
                        poi->distance_from_driver_break_stop = cabin->distance_from_driver_break_stop;
                        stop->pois = g_list_append(stop->pois, poi);
                    }
                    poi_list = g_list_next(poi_list);
                }

                poi_free_water_points(water_pois_list);
                poi_free_cabins(cabin_pois_list);
            } else {
                stop->pois = driver_break_poi_discover(&c_stop->coord, priv->config.poi_search_radius_km, NULL, 0);
            }

            priv->suggested_stops = g_list_append(priv->suggested_stops, stop);
        }
        l = g_list_next(l);
    }
}

static void driver_break_handle_hiking_route(struct driver_break_priv *priv) {
    struct attr dist_attr;
    double total_distance = 0.0;

    if (!priv->current_route) {
        return;
    }

    if (route_get_attr(priv->current_route, attr_destination_length, &dist_attr, NULL)) {
        total_distance = (double)dist_attr.u.num;
    }

    /* Adjust max daily distance if DNT priority enabled (matches BRouter) */
    if (total_distance > 0) {
        double max_daily = priv->config.hiking_max_daily_distance;
        if (priv->config.enable_dnt_priority) {
            max_daily = 46400.0; /* 46.4 km (90th percentile of DNT hut spacing) */
        }

        GList *hiking_stops = hiking_calculate_driver_break_stops_with_max(total_distance, 0, max_daily);
        if (hiking_stops) {
            process_hiking_stops(priv, hiking_stops);
            hiking_free_driver_break_stops(hiking_stops);
        }
    }
}

static void driver_break_handle_cycling_route(struct driver_break_priv *priv) {
    struct attr dist_attr;
    double total_distance = 0.0;

    if (!priv->current_route) {
        return;
    }

    if (route_get_attr(priv->current_route, attr_destination_length, &dist_attr, NULL)) {
        total_distance = (double)dist_attr.u.num;
    }

    if (total_distance > 0) {
        GList *cycling_stops = cycling_calculate_driver_break_stops(total_distance, 0);
        if (cycling_stops) {
            process_cycling_stops(priv, cycling_stops);
            cycling_free_driver_break_stops(cycling_stops);
        }
    }
}

static void driver_break_handle_motor_route(struct driver_break_priv *priv) {
    if (!priv->current_route || !priv->session.mandatory_break_required) {
        return;
    }

    priv->suggested_stops = driver_break_finder_find_along_route(priv->current_route, &priv->config,
                                                                 priv->session.mandatory_break_required);
}

static void driver_break_route_callback_wrapper(void *priv_data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)priv_data;
    if (!priv) {
        return;
    }

    /* Get route from navit */
    struct attr attr;
    if (!navit_get_attr(priv->nav, attr_route, &attr, NULL)) {
        return;
    }

    priv->current_route = attr.u.route;
    if (!priv->current_route) {
        return;
    }

    dbg(lvl_info, "Driver Break plugin: Finding rest stops along route (vehicle_type=%d)", priv->config.vehicle_type);

    /* For hiking/cycling, calculate rest stops based on distance; for cars/trucks follow mandatory breaks. */
    switch (priv->config.vehicle_type) {
    case DRIVER_BREAK_VEHICLE_HIKING:
        driver_break_handle_hiking_route(priv);
        break;
    case DRIVER_BREAK_VEHICLE_CYCLING:
        driver_break_handle_cycling_route(priv);
        break;
    case DRIVER_BREAK_VEHICLE_CAR:
    case DRIVER_BREAK_VEHICLE_TRUCK:
        driver_break_handle_motor_route(priv);
        break;
    default:
        break;
    }
}

/* Run fuel low-range check: search for fuel stations and notify user. Caller ensures vehicle is car/truck and route set. */
static void driver_break_check_fuel_low_range(struct driver_break_priv *priv) {
    if (!priv->current_route)
        return;
    struct attr dist_attr;
    double distance_km = 0.0;
    double buffer_km = priv->config.fuel_search_buffer_km > 0 ? priv->config.fuel_search_buffer_km : 20.0;

    if (route_get_attr(priv->current_route, attr_destination_length, &dist_attr, NULL))
        distance_km = (double)dist_attr.u.num / 1000.0;

    if (distance_km <= 0.0 || priv->fuel_remaining_range <= 0.0)
        return;
    double threshold = distance_km + buffer_km;
    if (priv->fuel_remaining_range >= threshold
        || priv->fuel_remaining_range >= (double)priv->config.fuel_low_warning_km)
        return;

    struct mapset *ms = navit_get_mapset(priv->nav);
    struct attr pos_attr;
    struct coord_geo center;
    GList *fuel_pois = NULL;
    if (ms && navit_get_attr(priv->nav, attr_position_coord_geo, &pos_attr, NULL) && pos_attr.u.coord_geo) {
        center = *pos_attr.u.coord_geo;
        double search_radius_km = priv->fuel_remaining_range;
        if (search_radius_km < buffer_km)
            search_radius_km = buffer_km;
        fuel_pois = driver_break_poi_map_search_fuel(&center, search_radius_km, ms,
                                                     priv->config.vehicle_type, priv->config.fuel_type);
    }

    if (fuel_pois) {
        int count = g_list_length(fuel_pois);
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "Driver Break plugin: Low range (%.0f km). Found %d fuel stations within %.0f km.",
                 priv->fuel_remaining_range, count, priv->fuel_remaining_range);
        navit_add_message(priv->nav, msg);
        dbg(lvl_info, "%s", msg);
        driver_break_poi_free_list(fuel_pois);
    } else if (priv->fuel_remaining_range < threshold) {
        navit_add_message(priv->nav,
                          "Driver Break plugin: Low range and no fuel stations found within range");
        dbg(lvl_info,
            "Driver Break plugin: Low range (%.0f km) and no fuel stations found within threshold",
            priv->fuel_remaining_range);
    }
}

/* Periodic check for rest stop suggestions */
static void driver_break_check_timeout(struct event_timeout *ev, void *data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)data;

    driver_break_check_driving_time(priv);

    if (priv->session.mandatory_break_required && priv->current_route)
        dbg(lvl_debug, "Driver Break plugin: Checking for rest stops");

    if (priv->config.vehicle_type == DRIVER_BREAK_VEHICLE_CAR
        || priv->config.vehicle_type == DRIVER_BREAK_VEHICLE_TRUCK)
        driver_break_check_fuel_low_range(priv);
}

/* OSD methods */
static void driver_break_osd_destroy(struct osd_priv *osd) {
    struct driver_break_priv *priv = (struct driver_break_priv *)osd;
    extern struct driver_break_priv *driver_break_plugin_instance;

    /* Clear static instance pointer */
    if (driver_break_plugin_instance == priv) {
        driver_break_plugin_instance = NULL;
    }

    if (priv->vehicle_cb) {
        callback_destroy(priv->vehicle_cb);
    }
    if (priv->route_cb) {
        callback_destroy(priv->route_cb);
    }
    if (priv->check_timeout) {
        event_remove_timeout(priv->check_timeout);
    }

    /* Stop live backends if running */
    if (priv->obd_backend) {
        driver_break_obd_stop((struct driver_break_obd *)priv->obd_backend);
        priv->obd_backend = NULL;
    }
    if (priv->j1939_backend) {
        driver_break_j1939_stop((struct driver_break_j1939 *)priv->j1939_backend);
        priv->j1939_backend = NULL;
    }

    if (priv->db) {
        /* Write a simple trip summary if we have collected any distance and fuel samples */
        if (priv->trip_start_time > 0 && priv->trip_total_distance_m > 0.0 && priv->trip_total_fuel > 0.0) {
            time_t now = time(NULL);
            double distance_km = priv->trip_total_distance_m / 1000.0;
            double avg_l_per_100 = (priv->trip_total_fuel / distance_km) * 100.0;
            double peak_l_per_100 = priv->peak_consumption_l_per_100km;
            int high_load_flag = priv->high_load_active ? 1 : 0;

            driver_break_db_add_trip_summary(priv->db, priv->trip_start_time, now, priv->trip_total_distance_m,
                                             priv->trip_total_fuel, avg_l_per_100, peak_l_per_100, high_load_flag);
        }

        driver_break_db_destroy(priv->db);
    }

    g_free(priv);
}

static int driver_break_osd_set_attr(struct osd_priv *osd, struct attr *attr) {
    return 0;
}

static struct osd_methods driver_break_osd_meth = {
    driver_break_osd_destroy,
    driver_break_osd_set_attr,
    driver_break_osd_destroy,
    NULL,
};

/* OSD constructor */
static struct osd_priv *driver_break_osd_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs) {
    struct driver_break_priv *priv;
    struct attr *db_path_attr, *vehicle_type_attr;
    char *db_path = NULL;

    dbg(lvl_info, "Driver Break plugin: driver_break_osd_new called - OSD is being instantiated");

    if (!nav) {
        dbg(lvl_error, "Driver Break plugin: driver_break_osd_new called with NULL nav");
        return NULL;
    }

    priv = g_new0(struct driver_break_priv, 1);
    *meth = driver_break_osd_meth;

    priv->nav = nav;
    priv->current_break_start_time = 0; /* No break in progress initially */

    /* Initialize config with defaults - ensure all fields are set */
    driver_break_config_default(&priv->config);

    /* Store instance pointer for command handlers (declared in driver_break_osd.c) */
    extern struct driver_break_priv *driver_break_plugin_instance;
    driver_break_plugin_instance = priv;

    dbg(lvl_debug, "Driver Break plugin: Stored instance pointer at %p", priv);

    /* Get database path from config */
    db_path = driver_break_get_database_path(attrs);

    /* Initialize database */
    priv->db = driver_break_db_new(db_path);
    if (priv->db) {
        /* Load saved configuration - if load fails or returns invalid data, defaults are preserved */
        if (!driver_break_db_load_config(priv->db, &priv->config)) {
            dbg(lvl_debug, "Driver Break plugin: Using default configuration values");
        }
    } else {
        dbg(lvl_warning, "Driver Break plugin: Could not initialize database, using defaults");
    }
    g_free(db_path);

    /* Get vehicle type from config */
    vehicle_type_attr = attr_search(attrs, attr_type);
    if (vehicle_type_attr && vehicle_type_attr->u.str) {
        if (!strcmp(vehicle_type_attr->u.str, "truck")) {
            priv->config.vehicle_type = DRIVER_BREAK_VEHICLE_TRUCK;
        } else if (!strcmp(vehicle_type_attr->u.str, "hiking")) {
            priv->config.vehicle_type = DRIVER_BREAK_VEHICLE_HIKING;
        } else if (!strcmp(vehicle_type_attr->u.str, "cycling")) {
            priv->config.vehicle_type = DRIVER_BREAK_VEHICLE_CYCLING;
        }
    }

    /* Register vehicle callback */
    priv->vehicle_cb =
        callback_new_attr_1(callback_cast(driver_break_vehicle_callback_wrapper), attr_position_coord_geo, priv);
    navit_add_callback(nav, priv->vehicle_cb);

    /* Register route callback */
    priv->route_cb = callback_new_attr_1(callback_cast(driver_break_route_callback_wrapper), attr_route, priv);
    navit_add_callback(nav, priv->route_cb);

    /* Set up periodic check (every minute) */
    priv->check_timeout = event_add_timeout(60000, 1, callback_new_1(callback_cast(driver_break_check_timeout), priv));

    /* Start optional live backends if enabled by configuration */
    priv->obd_backend = driver_break_obd_start(priv);
    priv->j1939_backend = driver_break_j1939_start(priv);

    /* Register menu commands */
    driver_break_register_commands(nav);

    /* Register SRTM commands */
    driver_break_srtm_register_commands(nav);

    /* Initialize SRTM system */
    srtm_init(NULL);

    priv->active = 1;

    dbg(lvl_info, "Driver Break plugin OSD initialized successfully (vehicle_type=%d)", priv->config.vehicle_type);
    dbg(lvl_debug, "Driver Break plugin: Config defaults - car: soft=%d max=%d break_int=%d break_dur=%d",
        priv->config.car_soft_limit_hours, priv->config.car_max_hours, priv->config.car_break_interval_hours,
        priv->config.car_break_duration_min);
    dbg(lvl_debug,
        "Driver Break plugin: Config defaults - overnight: buildings=%d glaciers=%d poi=%d water=%d cabin=%d",
        priv->config.min_distance_from_buildings, priv->config.min_distance_from_glaciers,
        priv->config.poi_search_radius_km, priv->config.poi_water_search_radius_km,
        priv->config.poi_cabin_search_radius_km);
    dbg(lvl_info, "Driver Break plugin: Commands should now be available");

    return (struct osd_priv *)priv;
}

/* Plugin initialization */
void plugin_init(void) {
    dbg(lvl_debug, "Driver Break plugin initializing");
    plugin_register_category_osd("driver_break", driver_break_osd_new);
}
