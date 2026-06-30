/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "driver_break_osd_lifecycle.h"
#include "callback.h"
#include "debug.h"
#include "driver_break.h"
#include "driver_break_db.h"
#include "driver_break_osd.h"
#include "driver_break_srtm.h"
#include "driver_break_srtm_osd.h"
#include "driver_ecu.h"
#include "event.h"
#include "file.h"
#include "navit.h"
#include <glib.h>
#include <string.h>
#include <time.h>

extern void driver_break_vehicle_callback_wrapper(void *priv_data);
extern void driver_break_route_callback_wrapper(void *priv_data);
extern void driver_break_check_timeout(struct event_timeout *ev, void *data);

struct driver_break_ecu_runtime {
    struct ecu_config config;
    struct ecu_fuel_state fuel;
};

static struct driver_break_ecu_runtime driver_break_ecu;

static void driver_break_ecu_config_from_priv(const struct driver_break_priv *priv, struct ecu_config *cfg) {
    cfg->fuel_type = priv->config.fuel_type;
    cfg->fuel_tank_capacity_l = priv->config.fuel_tank_capacity_l;
    cfg->fuel_ethanol_manual_pct = priv->config.fuel_ethanol_manual_pct;
    cfg->fuel_obd_available = priv->config.fuel_obd_available;
    cfg->fuel_j1939_available = priv->config.fuel_j1939_available;
    cfg->fuel_megasquirt_available = priv->config.fuel_megasquirt_available;
    cfg->fuel_injector_flow_cc_min = priv->config.fuel_injector_flow_cc_min;
}

static void driver_break_ecu_fuel_from_priv(struct driver_break_priv *priv) {
    driver_break_ecu.fuel.fuel_current = priv->fuel_current;
    driver_break_ecu.fuel.fuel_rate_l_h = priv->fuel_rate_l_h;
}

void driver_break_ecu_sync_to_priv(struct driver_break_priv *priv) {
    priv->fuel_current = driver_break_ecu.fuel.fuel_current;
    priv->fuel_rate_l_h = driver_break_ecu.fuel.fuel_rate_l_h;
    priv->config.fuel_ethanol_manual_pct = driver_break_ecu.config.fuel_ethanol_manual_pct;
}

static char *driver_break_get_database_path(struct attr **attrs) {
    char *db_path = NULL;
    struct attr *db_path_attr = attr_search(attrs, attr_data);

    if (db_path_attr && db_path_attr->u.str) {
        struct file_wordexp *we = file_wordexp_new(db_path_attr->u.str);
        int count = file_wordexp_get_count(we);
        char **array = file_wordexp_get_array(we);

        if (count > 0 && array[0]) {
            db_path = g_strdup(array[0]);
        } else {
            const char *user_data_dir = navit_get_user_data_directory(1);
            if (user_data_dir) {
                db_path = g_build_filename(user_data_dir, "driver_break_plugin.db", NULL);
            }
        }
        file_wordexp_destroy(we);
    } else {
        const char *user_data_dir = navit_get_user_data_directory(1);
        if (user_data_dir) {
            db_path = g_build_filename(user_data_dir, "driver_break_plugin.db", NULL);
        }
    }

    return db_path;
}

static void driver_break_osd_clear_instance(struct driver_break_priv *priv) {
    extern struct driver_break_priv *driver_break_plugin_instance;

    if (driver_break_plugin_instance == priv) {
        driver_break_plugin_instance = NULL;
    }
}

static void driver_break_osd_destroy_callbacks(struct driver_break_priv *priv) {
    if (priv->vehicle_cb) {
        callback_destroy(priv->vehicle_cb);
    }
    if (priv->route_cb) {
        callback_destroy(priv->route_cb);
    }
    if (priv->check_timeout) {
        event_remove_timeout(priv->check_timeout);
    }
}

static void driver_break_osd_stop_backends(struct driver_break_priv *priv) {
    if (priv->obd_backend) {
        driver_ecu_obd_stop((struct driver_ecu_obd *)priv->obd_backend);
        priv->obd_backend = NULL;
    }
    if (priv->j1939_backend) {
        driver_ecu_j1939_stop((struct driver_ecu_j1939 *)priv->j1939_backend);
        priv->j1939_backend = NULL;
    }
    if (priv->megasquirt_backend) {
        driver_ecu_megasquirt_stop((struct driver_ecu_megasquirt *)priv->megasquirt_backend);
        priv->megasquirt_backend = NULL;
    }
}

static void driver_break_osd_save_trip_summary(struct driver_break_priv *priv) {
    if (!priv->db || priv->trip_start_time <= 0 || priv->trip_total_distance_m <= 0.0 || priv->trip_total_fuel <= 0.0) {
        return;
    }

    time_t now = time(NULL);
    double distance_km = priv->trip_total_distance_m / 1000.0;
    double avg_l_per_100 = (priv->trip_total_fuel / distance_km) * 100.0;
    double peak_l_per_100 = priv->peak_consumption_l_per_100km;
    int high_load_flag = priv->high_load_active ? 1 : 0;

    driver_break_db_add_trip_summary(priv->db, priv->trip_start_time, now, priv->trip_total_distance_m,
                                     priv->trip_total_fuel, avg_l_per_100, peak_l_per_100, high_load_flag);
}

static void driver_break_osd_destroy(struct osd_priv *osd) {
    struct driver_break_priv *priv = (struct driver_break_priv *)osd;

    driver_break_osd_clear_instance(priv);
    driver_break_osd_destroy_callbacks(priv);
    driver_break_osd_stop_backends(priv);

    if (priv->db) {
        driver_break_osd_save_trip_summary(priv);
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

static void driver_break_osd_apply_vehicle_type(struct driver_break_priv *priv, struct attr **attrs) {
    struct attr *vehicle_type_attr = attr_search(attrs, attr_type);

    if (!vehicle_type_attr || !vehicle_type_attr->u.str) {
        return;
    }

    if (!strcmp(vehicle_type_attr->u.str, "truck")) {
        priv->config.vehicle_type = DRIVER_BREAK_VEHICLE_TRUCK;
    } else if (!strcmp(vehicle_type_attr->u.str, "hiking")) {
        priv->config.vehicle_type = DRIVER_BREAK_VEHICLE_HIKING;
    } else if (!strcmp(vehicle_type_attr->u.str, "cycling")) {
        priv->config.vehicle_type = DRIVER_BREAK_VEHICLE_CYCLING;
    }
}

static void driver_break_osd_init_database(struct driver_break_priv *priv, struct attr **attrs) {
    char *db_path = driver_break_get_database_path(attrs);

    priv->db = driver_break_db_new(db_path);
    if (priv->db) {
        if (!driver_break_db_load_config(priv->db, &priv->config)) {
            dbg(lvl_debug, "Driver Break plugin: Using default configuration values");
        }
    } else {
        dbg(lvl_warning, "Driver Break plugin: Could not initialize database, using defaults");
    }
    g_free(db_path);
}

static void driver_break_osd_register_callbacks(struct driver_break_priv *priv) {
    priv->vehicle_cb =
        callback_new_attr_1(callback_cast(driver_break_vehicle_callback_wrapper), attr_position_coord_geo, priv);
    navit_add_callback(priv->nav, priv->vehicle_cb);

    priv->route_cb = callback_new_attr_1(callback_cast(driver_break_route_callback_wrapper), attr_route, priv);
    navit_add_callback(priv->nav, priv->route_cb);

    priv->check_timeout = event_add_timeout(60000, 1, callback_new_1(callback_cast(driver_break_check_timeout), priv));
}

static void driver_break_osd_start_backends(struct driver_break_priv *priv) {
    driver_break_ecu_config_from_priv(priv, &driver_break_ecu.config);
    driver_break_ecu_fuel_from_priv(priv);

    priv->obd_backend = driver_ecu_obd_start(&driver_break_ecu.config, &driver_break_ecu.fuel);
    if (!priv->obd_backend) {
        priv->megasquirt_backend = driver_ecu_megasquirt_start(&driver_break_ecu.config, &driver_break_ecu.fuel);
    }
    priv->j1939_backend = driver_ecu_j1939_start(&driver_break_ecu.config, &driver_break_ecu.fuel);
}

struct osd_priv *driver_break_osd_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs) {
    struct driver_break_priv *priv;
    extern struct driver_break_priv *driver_break_plugin_instance;

    dbg(lvl_info, "Driver Break plugin: driver_break_osd_new called - OSD is being instantiated");

    if (!nav) {
        dbg(lvl_error, "Driver Break plugin: driver_break_osd_new called with NULL nav");
        return NULL;
    }

    priv = g_new0(struct driver_break_priv, 1);
    *meth = driver_break_osd_meth;

    priv->nav = nav;
    priv->current_break_start_time = 0;
    driver_break_config_default(&priv->config);

    driver_break_plugin_instance = priv;
    dbg(lvl_debug, "Driver Break plugin: Stored instance pointer at %p", priv);

    driver_break_osd_init_database(priv, attrs);
    driver_break_osd_apply_vehicle_type(priv, attrs);
    driver_break_osd_register_callbacks(priv);
    driver_break_osd_start_backends(priv);

    driver_break_register_commands(nav);
    driver_break_srtm_register_commands(nav);
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
