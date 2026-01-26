/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024 Navit Team
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
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <glib.h>
#include "debug.h"
#include "coord.h"
#include "item.h"
#include "map.h"
#include "plugin.h"
#include "callback.h"
#include "event.h"
#include "attr.h"
#include "projection.h"
#include "transform.h"
#include "file.h"
#include "rest.h"
#include "rest_db.h"
#include "rest_finder.h"
#include "rest_poi.h"
#include "rest_osd.h"
#include "rest_energy.h"
#include "rest_hiking.h"
#include "rest_cycling.h"
#include "rest_poi_hiking.h"
#include "rest_route_validator.h"
#include "rest_glacier.h"
#include "rest_srtm.h"
#include "rest_srtm_osd.h"
#include "rest_poi_map.h"
#include "transform.h"
#include "projection.h"
#include "mapset.h"
/* item_def.h included via item.h */
#include "popup.h"
#include "vehicle.h"
#include "mapset.h"
#include "navit.h"
#include "menu.h"
#include "route.h"
#include "navigation.h"
#include "color.h"
#include "osd.h"
#include "command.h"

/* Default configuration */
static void rest_config_default(struct rest_config *config) {
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
    config->hiking_rest_distance_main = 11295.0;  /* 11.295 km */
    config->hiking_rest_distance_alt = 2275.2;    /* 2.275 km */
    config->hiking_max_daily_distance = 40000.0;  /* 40 km (46.4 km when DNT priority enabled) */
    
    /* Cycling defaults */
    config->cycling_rest_distance_main = 28240.0;  /* 28.24 km */
    config->cycling_rest_distance_alt = 5690.0;    /* 5.69 km */
    config->cycling_max_daily_distance = 100000.0; /* 100 km */
    
    /* Rest stop defaults - ALL fields must be initialized */
    config->min_distance_from_buildings = 150;
    config->min_distance_from_glaciers = 300;  /* 300m for nightly */
    config->poi_search_radius_km = 15;
    config->poi_water_search_radius_km = 2;    /* 2 km for water */
    config->poi_cabin_search_radius_km = 5;    /* 5 km for cabins */
    config->network_hut_search_radius_km = 25;  /* 25 km for network huts (DNT, STF) */
    
    /* Network and route priority defaults */
    config->enable_dnt_priority = 0;  /* Disabled by default */
    config->enable_hiking_pilgrimage_priority = 0;  /* Disabled by default */
    
    /* Rest interval defaults */
    config->rest_interval_min_hours = 1;
    config->rest_interval_max_hours = 6;
    
    /* Energy routing defaults */
    config->use_energy_routing = 0;  /* Disabled by default */
    config->total_weight = 80.0;     /* 80 kg default (person + gear) */
    
    config->vehicle_type = REST_VEHICLE_CAR; /* Default to car */
    
    dbg(lvl_debug, "Rest plugin: Config defaults initialized - buildings=%d, glaciers=%d, poi=%d, water=%d, cabin=%d",
        config->min_distance_from_buildings, config->min_distance_from_glaciers,
        config->poi_search_radius_km, config->poi_water_search_radius_km,
        config->poi_cabin_search_radius_km);
}

/* Calculate driving time and check for mandatory breaks */
static void rest_check_driving_time(struct rest_priv *priv) {
    time_t now = time(NULL);
    struct rest_config *config = &priv->config;
    int mandatory_break = 0;
    
    if (priv->session.start_time == 0) {
        priv->session.start_time = now;
        priv->session.last_break_time = now;
        return;
    }
    
    /* Calculate continuous driving time since last break */
    priv->session.continuous_driving_minutes = 
        (int)((now - priv->session.last_break_time) / 60);
    
    /* Check for mandatory break based on vehicle type */
    if (config->vehicle_type == REST_VEHICLE_TRUCK) {
        if (priv->session.continuous_driving_minutes >= 
            (config->truck_mandatory_break_after_hours * 60)) {
            mandatory_break = 1;
        }
    } else if (config->vehicle_type == REST_VEHICLE_CAR) {
        if (priv->session.continuous_driving_minutes >= 
            (config->car_break_interval_hours * 60)) {
            /* Recommended break, not mandatory for cars */
            mandatory_break = 0;
        }
    } else if (config->vehicle_type == REST_VEHICLE_HIKING || 
               config->vehicle_type == REST_VEHICLE_CYCLING) {
        /* For hiking/cycling, breaks are suggested based on distance, not time */
        /* This is handled separately in rest stop calculation */
        mandatory_break = 0;
    }
    
    priv->session.mandatory_break_required = mandatory_break;
    
    if (mandatory_break) {
        dbg(lvl_info, "Rest plugin: Mandatory break required after %d minutes of driving",
            priv->session.continuous_driving_minutes);
    }
}

/* Vehicle callback wrapper */
static void rest_vehicle_callback_wrapper(void *priv_data) {
    struct rest_priv *priv = (struct rest_priv *)priv_data;
    if (!priv) return;
    
    /* Get position from navit */
    struct attr attr;
    if (navit_get_attr(priv->nav, attr_position_coord_geo, &attr, NULL)) {
        if (attr.u.coord_geo) {
            /* Vehicle is moving, update driving time */
            rest_check_driving_time(priv);
        }
    }
}

/* Route callback wrapper */
static void rest_route_callback_wrapper(void *priv_data) {
    struct rest_priv *priv = (struct rest_priv *)priv_data;
    if (!priv) return;
    
    /* Get route from navit */
    struct attr attr;
    if (navit_get_attr(priv->nav, attr_route, &attr, NULL)) {
        priv->current_route = attr.u.route;
        
        /* Find rest stops along the route based on vehicle type */
        if (priv->current_route) {
            dbg(lvl_info, "Rest plugin: Finding rest stops along route (vehicle_type=%d)", 
                priv->config.vehicle_type);
            
            /* For hiking/cycling, calculate rest stops based on distance */
            if (priv->config.vehicle_type == REST_VEHICLE_HIKING) {
                /* Get route distance */
                struct attr dist_attr;
                double total_distance = 0.0;
                if (route_get_attr(priv->current_route, attr_destination_length, &dist_attr, NULL)) {
                    total_distance = (double)dist_attr.u.num;  /* Distance in meters */
                }
                
                /* Adjust max daily distance if DNT priority enabled (matches BRouter) */
                double max_daily = priv->config.hiking_max_daily_distance;
                if (priv->config.enable_dnt_priority) {
                    max_daily = 46400.0;  /* 46.4 km (90th percentile of DNT hut spacing) */
                }
                
                if (total_distance > 0) {
                    /* Calculate hiking rest stops with adjusted max daily distance */
                    GList *hiking_stops = hiking_calculate_rest_stops_with_max(total_distance, 0, max_daily);
                    if (hiking_stops) {
                        /* Convert hiking rest stops to rest_stop format */
                        GList *l = hiking_stops;
                        while (l) {
                            struct hiking_rest_stop *h_stop = (struct hiking_rest_stop *)l->data;
                            if (h_stop) {
                                /* Get coordinate at distance along route */
                                struct coord c = route_get_coord_dist(priv->current_route, (int)h_stop->position);
                                transform_to_geo(projection_mg, &c, &h_stop->coord);
                                
                                /* Create rest_stop from hiking_rest_stop */
                                struct rest_stop *stop = g_new0(struct rest_stop, 1);
                                stop->coord = h_stop->coord;
                                stop->distance_from_route = 0;
                                stop->name = g_strdup_printf("Hiking rest stop at %.1f km", h_stop->position / 1000.0);
                                
                                /* Search for POIs using map data if available */
                                struct mapset *ms = navit_get_mapset(priv->nav);
                                if (ms) {
                                    /* Use map-based POI search */
                                    GList *water_pois_list = poi_search_water_points_map(&h_stop->coord, 
                                                                                        priv->config.poi_water_search_radius_km, ms);
                                    GList *cabin_pois_list = poi_search_cabins_map(&h_stop->coord, 
                                                                                  priv->config.poi_cabin_search_radius_km, ms,
                                                                                  priv->config.enable_dnt_priority);
                                    
                                    /* Convert water points and cabins to rest_poi format */
                                    GList *l = water_pois_list;
                                    while (l) {
                                        struct water_point *wp = (struct water_point *)l->data;
                                        if (wp) {
                                            struct rest_poi *poi = g_new0(struct rest_poi, 1);
                                            poi->coord = wp->coord;
                                            poi->name = wp->name ? g_strdup(wp->name) : NULL;
                                            poi->type = g_strdup("amenity");
                                            poi->category = wp->type ? g_strdup(wp->type) : NULL;
                                            poi->distance_from_rest_stop = wp->distance_from_rest_stop;
                                            stop->pois = g_list_append(stop->pois, poi);
                                        }
                                        l = g_list_next(l);
                                    }
                                    
                                    l = cabin_pois_list;
                                    while (l) {
                                        struct cabin_hut *cabin = (struct cabin_hut *)l->data;
                                        if (cabin) {
                                            struct rest_poi *poi = g_new0(struct rest_poi, 1);
                                            poi->coord = cabin->coord;
                                            poi->name = cabin->name ? g_strdup(cabin->name) : NULL;
                                            poi->type = g_strdup("tourism");
                                            poi->category = cabin->type ? g_strdup(cabin->type) : NULL;
                                            poi->distance_from_rest_stop = cabin->distance_from_rest_stop;
                                            stop->pois = g_list_append(stop->pois, poi);
                                        }
                                        l = g_list_next(l);
                                    }
                                    
                                    poi_free_water_points(water_pois_list);
                                    poi_free_cabins(cabin_pois_list);
                                } else {
                                    /* Fallback to Overpass API */
                                    stop->pois = rest_poi_discover(&h_stop->coord, priv->config.poi_search_radius_km,
                                                                   NULL, 0);
                                }
                                
                                priv->suggested_stops = g_list_append(priv->suggested_stops, stop);
                            }
                            l = g_list_next(l);
                        }
                        hiking_free_rest_stops(hiking_stops);
                    }
                }
            } else if (priv->config.vehicle_type == REST_VEHICLE_CYCLING) {
                /* Get route distance */
                struct attr dist_attr;
                double total_distance = 0.0;
                if (route_get_attr(priv->current_route, attr_destination_length, &dist_attr, NULL)) {
                    total_distance = (double)dist_attr.u.num;  /* Distance in meters */
                }
                
                if (total_distance > 0) {
                    /* Calculate cycling rest stops */
                    GList *cycling_stops = cycling_calculate_rest_stops(total_distance, 0);
                    if (cycling_stops) {
                        /* Convert cycling rest stops to rest_stop format */
                        GList *l = cycling_stops;
                        while (l) {
                            struct cycling_rest_stop *c_stop = (struct cycling_rest_stop *)l->data;
                            if (c_stop) {
                                /* Get coordinate at distance along route */
                                struct coord c = route_get_coord_dist(priv->current_route, (int)c_stop->position);
                                transform_to_geo(projection_mg, &c, &c_stop->coord);
                                
                                /* Create rest_stop from cycling_rest_stop */
                                struct rest_stop *stop = g_new0(struct rest_stop, 1);
                                stop->coord = c_stop->coord;
                                stop->distance_from_route = 0;
                                stop->name = g_strdup_printf("Cycling rest stop at %.1f km", c_stop->position / 1000.0);
                                
                                /* Search for POIs using map data if available */
                                struct mapset *ms = navit_get_mapset(priv->nav);
                                if (ms) {
                                    /* Use map-based POI search */
                                    GList *water_pois_list = poi_search_water_points_map(&c_stop->coord, 
                                                                                        priv->config.poi_water_search_radius_km, ms);
                                    GList *cabin_pois_list = poi_search_cabins_map(&c_stop->coord, 
                                                                                  priv->config.poi_cabin_search_radius_km, ms,
                                                                                  priv->config.enable_dnt_priority);
                                    
                                    /* Convert water points and cabins to rest_poi format */
                                    GList *l = water_pois_list;
                                    while (l) {
                                        struct water_point *wp = (struct water_point *)l->data;
                                        if (wp) {
                                            struct rest_poi *poi = g_new0(struct rest_poi, 1);
                                            poi->coord = wp->coord;
                                            poi->name = wp->name ? g_strdup(wp->name) : NULL;
                                            poi->type = g_strdup("amenity");
                                            poi->category = wp->type ? g_strdup(wp->type) : NULL;
                                            poi->distance_from_rest_stop = wp->distance_from_rest_stop;
                                            stop->pois = g_list_append(stop->pois, poi);
                                        }
                                        l = g_list_next(l);
                                    }
                                    
                                    l = cabin_pois_list;
                                    while (l) {
                                        struct cabin_hut *cabin = (struct cabin_hut *)l->data;
                                        if (cabin) {
                                            struct rest_poi *poi = g_new0(struct rest_poi, 1);
                                            poi->coord = cabin->coord;
                                            poi->name = cabin->name ? g_strdup(cabin->name) : NULL;
                                            poi->type = g_strdup("tourism");
                                            poi->category = cabin->type ? g_strdup(cabin->type) : NULL;
                                            poi->distance_from_rest_stop = cabin->distance_from_rest_stop;
                                            stop->pois = g_list_append(stop->pois, poi);
                                        }
                                        l = g_list_next(l);
                                    }
                                    
                                    poi_free_water_points(water_pois_list);
                                    poi_free_cabins(cabin_pois_list);
                                } else {
                                    /* Fallback to Overpass API */
                                    stop->pois = rest_poi_discover(&c_stop->coord, priv->config.poi_search_radius_km,
                                                                   NULL, 0);
                                }
                                
                                priv->suggested_stops = g_list_append(priv->suggested_stops, stop);
                            }
                            l = g_list_next(l);
                        }
                        cycling_free_rest_stops(cycling_stops);
                    }
                }
            } else if (priv->session.mandatory_break_required) {
                /* For cars/trucks, find rest stops when mandatory break required */
                priv->suggested_stops = rest_finder_find_along_route(
                    priv->current_route, 
                    &priv->config,
                    priv->session.mandatory_break_required
                );
            }
        }
    }
}

/* Periodic check for rest stop suggestions */
static void rest_check_timeout(struct event_timeout *ev, void *data) {
    struct rest_priv *priv = (struct rest_priv *)data;
    
    rest_check_driving_time(priv);
    
    /* If mandatory break required and we have a route, find rest stops */
    if (priv->session.mandatory_break_required && priv->current_route) {
        /* Trigger rest stop search */
        dbg(lvl_debug, "Rest plugin: Checking for rest stops");
    }
}

/* OSD methods */
static void rest_osd_destroy(struct osd_priv *osd) {
    struct rest_priv *priv = (struct rest_priv *)osd;
    extern struct rest_priv *rest_plugin_instance;
    
    /* Clear static instance pointer */
    if (rest_plugin_instance == priv) {
        rest_plugin_instance = NULL;
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
    if (priv->db) {
        rest_db_destroy(priv->db);
    }
    
    g_free(priv);
}

static int rest_osd_set_attr(struct osd_priv *osd, struct attr *attr) {
    return 0;
}

static struct osd_methods rest_osd_meth = {
    rest_osd_destroy,
    rest_osd_set_attr,
    rest_osd_destroy,
    NULL,
};

/* OSD constructor */
static struct osd_priv *rest_osd_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs) {
    struct rest_priv *priv;
    struct attr *db_path_attr, *vehicle_type_attr;
    char *db_path = NULL;
    
    dbg(lvl_info, "Rest plugin: rest_osd_new called - OSD is being instantiated");
    
    if (!nav) {
        dbg(lvl_error, "Rest plugin: rest_osd_new called with NULL nav");
        return NULL;
    }
    
    priv = g_new0(struct rest_priv, 1);
    *meth = rest_osd_meth;
    
    priv->nav = nav;
    
    /* Initialize config with defaults - ensure all fields are set */
    rest_config_default(&priv->config);
    
    /* Store instance pointer for command handlers (declared in rest_osd.c) */
    extern struct rest_priv *rest_plugin_instance;
    rest_plugin_instance = priv;
    
    dbg(lvl_debug, "Rest plugin: Stored instance pointer at %p", priv);
    
    /* Get database path from config */
    db_path_attr = attr_search(attrs, attr_data);
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
                db_path = g_build_filename(user_data_dir, "rest_plugin.db", NULL);
            }
            /* Note: user_data_dir is from getenv(), do not free */
        }
        file_wordexp_destroy(we);
    } else {
        /* Default database path */
        const char *user_data_dir = navit_get_user_data_directory(1);
        if (user_data_dir) {
            db_path = g_build_filename(user_data_dir, "rest_plugin.db", NULL);
        }
        /* Note: user_data_dir is from getenv(), do not free */
    }
    
    /* Initialize database */
    priv->db = rest_db_new(db_path);
    if (priv->db) {
        /* Load saved configuration - if load fails or returns invalid data, defaults are preserved */
        if (!rest_db_load_config(priv->db, &priv->config)) {
            dbg(lvl_debug, "Rest plugin: Using default configuration values");
        }
    } else {
        dbg(lvl_warning, "Rest plugin: Could not initialize database, using defaults");
    }
    g_free(db_path);
    
    /* Get vehicle type from config */
    vehicle_type_attr = attr_search(attrs, attr_type);
    if (vehicle_type_attr && vehicle_type_attr->u.str) {
        if (!strcmp(vehicle_type_attr->u.str, "truck")) {
            priv->config.vehicle_type = REST_VEHICLE_TRUCK;
        } else if (!strcmp(vehicle_type_attr->u.str, "hiking")) {
            priv->config.vehicle_type = REST_VEHICLE_HIKING;
        } else if (!strcmp(vehicle_type_attr->u.str, "cycling")) {
            priv->config.vehicle_type = REST_VEHICLE_CYCLING;
        }
    }
    
    /* Register vehicle callback */
    priv->vehicle_cb = callback_new_attr_1(callback_cast(rest_vehicle_callback_wrapper), 
                                           attr_position_coord_geo, priv);
    navit_add_callback(nav, priv->vehicle_cb);
    
    /* Register route callback */
    priv->route_cb = callback_new_attr_1(callback_cast(rest_route_callback_wrapper),
                                         attr_route, priv);
    navit_add_callback(nav, priv->route_cb);
    
    /* Set up periodic check (every minute) */
    priv->check_timeout = event_add_timeout(60000, 1, 
                                             callback_new_1(callback_cast(rest_check_timeout), priv));
    
    /* Register menu commands */
    rest_register_commands(nav);
    
    /* Register SRTM commands */
    rest_srtm_register_commands(nav);
    
    /* Initialize SRTM system */
    srtm_init(NULL);
    
    priv->active = 1;
    
    dbg(lvl_info, "Rest plugin OSD initialized successfully (vehicle_type=%d)", priv->config.vehicle_type);
    dbg(lvl_debug, "Rest plugin: Config defaults - car: soft=%d max=%d break_int=%d break_dur=%d", 
        priv->config.car_soft_limit_hours, priv->config.car_max_hours,
        priv->config.car_break_interval_hours, priv->config.car_break_duration_min);
    dbg(lvl_debug, "Rest plugin: Config defaults - overnight: buildings=%d glaciers=%d poi=%d water=%d cabin=%d",
        priv->config.min_distance_from_buildings, priv->config.min_distance_from_glaciers,
        priv->config.poi_search_radius_km, priv->config.poi_water_search_radius_km,
        priv->config.poi_cabin_search_radius_km);
    dbg(lvl_info, "Rest plugin: Commands should now be available");
    
    return (struct osd_priv *)priv;
}

/* Plugin initialization */
void plugin_init(void) {
    dbg(lvl_debug, "Rest plugin initializing");
    plugin_register_category_osd("rest", rest_osd_new);
}
