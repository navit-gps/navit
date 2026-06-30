/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "debug.h"
#include "driver_break_db_priv.h"
#include <glib.h>
#include <string.h>

int driver_break_db_set_config_value(const struct config_key_map *map, struct driver_break_config *config, int value,
                                     int *loaded_count) {
    int valid;

    if (map->allow_zero) {
        valid = (value >= map->min_value && value <= map->max_value);
    } else {
        valid = (value > map->min_value && value <= map->max_value);
    }

    if (valid) {
        int *field = (int *)((char *)config + map->offset);
        *field = value;
        (*loaded_count)++;
        return 0;
    }

    dbg(lvl_warning, "Driver Break plugin: Invalid %s value: %d, using default", map->key, value);
    return 0;
}

int driver_break_db_load_config_value(const char *key, int value, struct driver_break_config *config,
                                      int *loaded_count) {
    static const struct config_key_map maps[] = {
        {"vehicle_type", offsetof(struct driver_break_config, vehicle_type), 0, MAX_VEHICLE_TYPE, 1},
        {"car_soft_limit_hours", offsetof(struct driver_break_config, car_soft_limit_hours), 0, MAX_HOURS_PER_DAY, 0},
        {"car_max_hours", offsetof(struct driver_break_config, car_max_hours), 0, MAX_HOURS_PER_DAY, 0},
        {"car_break_interval_hours", offsetof(struct driver_break_config, car_break_interval_hours), 0,
         MAX_HOURS_PER_DAY, 0},
        {"car_break_duration_min", offsetof(struct driver_break_config, car_break_duration_min), 0,
         MAX_BREAK_DURATION_MIN, 0},
        {"truck_mandatory_break_after_hours", offsetof(struct driver_break_config, truck_mandatory_break_after_hours),
         0, MAX_HOURS_PER_DAY, 0},
        {"truck_mandatory_break_duration_min", offsetof(struct driver_break_config, truck_mandatory_break_duration_min),
         0, MAX_BREAK_DURATION_MIN, 0},
        {"truck_max_daily_hours", offsetof(struct driver_break_config, truck_max_daily_hours), 0, MAX_HOURS_PER_DAY, 0},
        {"min_distance_from_buildings", offsetof(struct driver_break_config, min_distance_from_buildings), 0,
         MAX_DISTANCE_METERS, 0},
        {"poi_search_radius_km", offsetof(struct driver_break_config, poi_search_radius_km), 0,
         MAX_POI_SEARCH_RADIUS_KM, 0},
        {"poi_water_search_radius_km", offsetof(struct driver_break_config, poi_water_search_radius_km), 0,
         MAX_POI_WATER_CABIN_RADIUS_KM, 0},
        {"poi_cabin_search_radius_km", offsetof(struct driver_break_config, poi_cabin_search_radius_km), 0,
         MAX_POI_WATER_CABIN_RADIUS_KM, 0},
        {"min_distance_from_glaciers", offsetof(struct driver_break_config, min_distance_from_glaciers), 0,
         MAX_DISTANCE_METERS, 0},
        {"driver_break_interval_min_hours", offsetof(struct driver_break_config, driver_break_interval_min_hours), 0,
         MAX_HOURS_PER_DAY_24, 0},
        {"driver_break_interval_max_hours", offsetof(struct driver_break_config, driver_break_interval_max_hours), 0,
         MAX_HOURS_PER_DAY_24, 0},
        {"fuel_type", offsetof(struct driver_break_config, fuel_type), 0, 7, 1},
        {"fuel_tank_capacity_l", offsetof(struct driver_break_config, fuel_tank_capacity_l), 0, MAX_CONFIG_VALUE, 0},
        {"fuel_avg_consumption_x10", offsetof(struct driver_break_config, fuel_avg_consumption_x10), 0,
         MAX_CONFIG_VALUE, 0},
        {"fuel_obd_available", offsetof(struct driver_break_config, fuel_obd_available), 0, 1, 1},
        {"fuel_j1939_available", offsetof(struct driver_break_config, fuel_j1939_available), 0, 1, 1},
        {"fuel_megasquirt_available", offsetof(struct driver_break_config, fuel_megasquirt_available), 0, 1, 1},
        {"fuel_injector_flow_cc_min", offsetof(struct driver_break_config, fuel_injector_flow_cc_min), 0,
         MAX_CONFIG_VALUE, 1},
        {"fuel_ethanol_manual_pct", offsetof(struct driver_break_config, fuel_ethanol_manual_pct), 0, 100, 1},
        {"fuel_low_warning_km", offsetof(struct driver_break_config, fuel_low_warning_km), 0, MAX_CONFIG_VALUE, 0},
        {"fuel_search_buffer_km", offsetof(struct driver_break_config, fuel_search_buffer_km), 0, MAX_CONFIG_VALUE, 0},
        {"fuel_high_load_threshold", offsetof(struct driver_break_config, fuel_high_load_threshold), 0, 100, 0},
        {NULL, 0, 0, 0, 0},
    };
    int i;

    for (i = 0; maps[i].key != NULL; i++) {
        if (!strcmp(key, maps[i].key)) {
            return driver_break_db_set_config_value(&maps[i], config, value, loaded_count);
        }
    }

    return -1;
}

void driver_break_db_clean_corrupted_config(struct driver_break_db *db) {
    char *err_msg = NULL;
    int rc;
    const char *sql;

    if (!db || !db->db) {
        return;
    }

    sql = "DELETE FROM driver_break_config WHERE "
          "CAST(value AS INTEGER) < 0 OR "
          "CAST(value AS INTEGER) > 1000000 OR "
          "value IS NULL OR "
          "value = '';";

    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    if (rc != SQLITE_OK) {
        dbg(lvl_warning, "Driver Break plugin: Error cleaning corrupted config: %s", err_msg);
        sqlite3_free(err_msg);
    } else {
        dbg(lvl_debug, "Driver Break plugin: Cleaned corrupted config entries");
    }
}

static int driver_break_db_parse_config_int(const char *key, const char *value_str, int *value_out) {
    char *endptr;
    int value;

    if (!key || !value_str) {
        return 0;
    }

    value = (int)strtol(value_str, &endptr, DECIMAL_BASE);
    if (*endptr != '\0' && *endptr != '\n') {
        dbg(lvl_warning, "Driver Break plugin: Invalid numeric value for %s: '%s', skipping", key, value_str);
        return 0;
    }

    if (value_str[0] == '\0' || (value == 0 && value_str[0] != '0')) {
        dbg(lvl_warning, "Driver Break plugin: Invalid numeric value for %s: '%s', skipping", key, value_str);
        return 0;
    }

    *value_out = value;
    return 1;
}

static int driver_break_db_load_config_row(const char *key, const char *value_str, struct driver_break_config *config,
                                           int *loaded_count) {
    int value;

    if (!driver_break_db_parse_config_int(key, value_str, &value)) {
        return 0;
    }

    if (driver_break_db_load_config_value(key, value, config, loaded_count) != 0) {
        dbg(lvl_debug, "Driver Break plugin: Unknown config key in database: %s", key);
    }

    return 1;
}

int driver_break_db_save_config(struct driver_break_db *db, struct driver_break_config *config) {
    char *sql;
    char *err_msg = NULL;
    int rc;

    static const char *keys[] = {"vehicle_type",
                                 "car_soft_limit_hours",
                                 "car_max_hours",
                                 "car_break_interval_hours",
                                 "car_break_duration_min",
                                 "truck_mandatory_break_after_hours",
                                 "truck_mandatory_break_duration_min",
                                 "truck_max_daily_hours",
                                 "min_distance_from_buildings",
                                 "poi_search_radius_km",
                                 "driver_break_interval_min_hours",
                                 "driver_break_interval_max_hours",
                                 "fuel_type",
                                 "fuel_tank_capacity_l",
                                 "fuel_avg_consumption_x10",
                                 "fuel_obd_available",
                                 "fuel_j1939_available",
                                 "fuel_megasquirt_available",
                                 "fuel_injector_flow_cc_min",
                                 "fuel_ethanol_manual_pct",
                                 "fuel_low_warning_km",
                                 "fuel_search_buffer_km",
                                 "fuel_high_load_threshold"};

    int values[] = {config->vehicle_type,
                    config->car_soft_limit_hours,
                    config->car_max_hours,
                    config->car_break_interval_hours,
                    config->car_break_duration_min,
                    config->truck_mandatory_break_after_hours,
                    config->truck_mandatory_break_duration_min,
                    config->truck_max_daily_hours,
                    config->min_distance_from_buildings,
                    config->poi_search_radius_km,
                    config->driver_break_interval_min_hours,
                    config->driver_break_interval_max_hours,
                    config->fuel_type,
                    config->fuel_tank_capacity_l,
                    config->fuel_avg_consumption_x10,
                    config->fuel_obd_available,
                    config->fuel_j1939_available,
                    config->fuel_megasquirt_available,
                    config->fuel_injector_flow_cc_min,
                    config->fuel_ethanol_manual_pct,
                    config->fuel_low_warning_km,
                    config->fuel_search_buffer_km,
                    config->fuel_high_load_threshold};
    size_t i;

    if (!db || !config) {
        return 0;
    }

    for (i = 0; i < G_N_ELEMENTS(keys); i++) {
        sql = sqlite3_mprintf("INSERT OR REPLACE INTO driver_break_config (key, value) VALUES (%Q, %d);", keys[i],
                              values[i]);

        rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
        sqlite3_free(sql);

        if (rc != SQLITE_OK) {
            dbg(lvl_error, "Driver Break plugin: SQL error saving config %s: %s", keys[i], err_msg);
            sqlite3_free(err_msg);
        }
    }

    return 1;
}

int driver_break_db_load_config(struct driver_break_db *db, struct driver_break_config *config) {
    sqlite3_stmt *stmt;
    const char *sql;
    int rc;
    int loaded_count = 0;

    if (!db || !config) {
        return 0;
    }

    driver_break_db_clean_corrupted_config(db);

    sql = "SELECT key, value FROM driver_break_config WHERE value IS NOT NULL AND value != '';";

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        dbg(lvl_debug, "Driver Break plugin: No saved configuration found, using defaults");
        return 0;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char *key = (const char *)sqlite3_column_text(stmt, 0);
        const char *value_str = (const char *)sqlite3_column_text(stmt, 1);

        driver_break_db_load_config_row(key, value_str, config, &loaded_count);
    }

    sqlite3_finalize(stmt);

    if (loaded_count > 0) {
        dbg(lvl_debug, "Driver Break plugin: Loaded %d configuration values from database", loaded_count);
        return 1;
    }

    dbg(lvl_debug, "Driver Break plugin: No valid configuration values found in database, using defaults");
    return 0;
}
