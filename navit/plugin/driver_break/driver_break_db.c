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

#include "driver_break_db.h"
#include "config.h"
#include "debug.h"
#include "file.h"
#include <glib.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

/* Constants for validation bounds */
#define MAX_HOURS_PER_DAY 12
#define MAX_BREAK_DURATION_MIN 120
#define MAX_VEHICLE_TYPE 3
#define SQLITE_COLUMN_INDEX_WAS_MANDATORY 5
#define MAX_DISTANCE_METERS 10000
#define MAX_POI_SEARCH_RADIUS_KM 1000
#define MAX_POI_WATER_CABIN_RADIUS_KM 100
#define MAX_HOURS_PER_DAY_24 24
#define MAX_CONFIG_VALUE 1000000
#define DECIMAL_BASE 10

struct driver_break_db {
    sqlite3 *db;
};

/* Configuration key mapping structure */
struct config_key_map {
    const char *key;
    size_t offset;
    int min_value;
    int max_value;
    int allow_zero;
};

/* Validate and set a configuration value */
static int driver_break_db_set_config_value(const struct config_key_map *map, struct driver_break_config *config, int value,
                                            int *loaded_count) {
    int valid = 0;

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
    } else {
        dbg(lvl_warning, "Driver Break plugin: Invalid %s value: %d, using default", map->key, value);
        return 0;
    }
}

/* Load a single configuration value - returns 0 on success, -1 if key not recognized */
static int driver_break_db_load_config_value(const char *key, int value, struct driver_break_config *config,
                                             int *loaded_count) {
    static const struct config_key_map maps[] = {
        {"vehicle_type", offsetof(struct driver_break_config, vehicle_type), 0, MAX_VEHICLE_TYPE, 1},
        {"car_soft_limit_hours", offsetof(struct driver_break_config, car_soft_limit_hours), 0, MAX_HOURS_PER_DAY, 0},
        {"car_max_hours", offsetof(struct driver_break_config, car_max_hours), 0, MAX_HOURS_PER_DAY, 0},
        {"car_break_interval_hours", offsetof(struct driver_break_config, car_break_interval_hours), 0, MAX_HOURS_PER_DAY, 0},
        {"car_break_duration_min", offsetof(struct driver_break_config, car_break_duration_min), 0, MAX_BREAK_DURATION_MIN, 0},
        {"truck_mandatory_break_after_hours", offsetof(struct driver_break_config, truck_mandatory_break_after_hours), 0, MAX_HOURS_PER_DAY, 0},
        {"truck_mandatory_break_duration_min", offsetof(struct driver_break_config, truck_mandatory_break_duration_min), 0, MAX_BREAK_DURATION_MIN, 0},
        {"truck_max_daily_hours", offsetof(struct driver_break_config, truck_max_daily_hours), 0, MAX_HOURS_PER_DAY, 0},
        {"min_distance_from_buildings", offsetof(struct driver_break_config, min_distance_from_buildings), 0, MAX_DISTANCE_METERS, 0},
        {"poi_search_radius_km", offsetof(struct driver_break_config, poi_search_radius_km), 0, MAX_POI_SEARCH_RADIUS_KM, 0},
        {"poi_water_search_radius_km", offsetof(struct driver_break_config, poi_water_search_radius_km), 0, MAX_POI_WATER_CABIN_RADIUS_KM, 0},
        {"poi_cabin_search_radius_km", offsetof(struct driver_break_config, poi_cabin_search_radius_km), 0, MAX_POI_WATER_CABIN_RADIUS_KM, 0},
        {"min_distance_from_glaciers", offsetof(struct driver_break_config, min_distance_from_glaciers), 0, MAX_DISTANCE_METERS, 0},
        {"driver_break_interval_min_hours", offsetof(struct driver_break_config, driver_break_interval_min_hours), 0, MAX_HOURS_PER_DAY_24, 0},
        {"driver_break_interval_max_hours", offsetof(struct driver_break_config, driver_break_interval_max_hours), 0, MAX_HOURS_PER_DAY_24, 0},
        /* Fuel and range configuration (stored as integers/scaled values) */
        {"fuel_type", offsetof(struct driver_break_config, fuel_type), 0, 7, 1},
        {"fuel_tank_capacity_l", offsetof(struct driver_break_config, fuel_tank_capacity_l), 0, MAX_CONFIG_VALUE, 0},
        {"fuel_avg_consumption_x10", offsetof(struct driver_break_config, fuel_avg_consumption_x10), 0, MAX_CONFIG_VALUE, 0},
        {"fuel_obd_available", offsetof(struct driver_break_config, fuel_obd_available), 0, 1, 1},
        {"fuel_j1939_available", offsetof(struct driver_break_config, fuel_j1939_available), 0, 1, 1},
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

/* Clean up corrupted config entries from database */
static void driver_break_db_clean_corrupted_config(struct driver_break_db *db) {
    char *sql;
    char *err_msg = NULL;
    int rc;

    if (!db || !db->db) {
        return;
    }

    /* Delete entries with invalid values (too large, negative, etc.) */
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

struct driver_break_db *driver_break_db_new(const char *db_path) {
    struct driver_break_db *db;
    char *err_msg = NULL;
    int rc;
    const char *sql;
    char *db_dir;

    db = g_new0(struct driver_break_db, 1);

    /* Ensure directory exists */
    db_dir = g_path_get_dirname(db_path);
    if (db_dir && !file_is_dir(db_dir)) {
        /* Create directory if it doesn't exist */
        if (file_mkdir(db_dir, 1) != 0) {
            dbg(lvl_warning, "Driver Break plugin: Could not create database directory %s", db_dir);
        }
    }
    g_free(db_dir);

    rc = sqlite3_open(db_path, &db->db);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: Cannot open database %s: %s", db_path, sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        g_free(db);
        return NULL;
    }

    /* Create rest stop history table */
    sql = "CREATE TABLE IF NOT EXISTS driver_break_stop_history ("
          "id INTEGER PRIMARY KEY AUTOINCREMENT,"
          "timestamp INTEGER NOT NULL,"
          "latitude REAL NOT NULL,"
          "longitude REAL NOT NULL,"
          "name TEXT,"
          "duration_minutes INTEGER,"
          "was_mandatory INTEGER"
          ");";

    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: SQL error creating history table: %s", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db->db);
        g_free(db);
        return NULL;
    }

    /* Create config table */
    sql = "CREATE TABLE IF NOT EXISTS driver_break_config ("
          "key TEXT PRIMARY KEY,"
          "value TEXT"
          ");";

    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: SQL error creating config table: %s", err_msg);
        sqlite3_free(err_msg);
    }

    /* Create fuel stop history table */
    sql = "CREATE TABLE IF NOT EXISTS driver_break_fuel_stops ("
          "id INTEGER PRIMARY KEY AUTOINCREMENT,"
          "timestamp INTEGER NOT NULL,"
          "latitude REAL NOT NULL,"
          "longitude REAL NOT NULL,"
          "fuel_added REAL,"
          "fuel_level_after REAL,"
          "cost REAL,"
          "currency TEXT,"
          "ethanol_pct INTEGER"
          ");";

    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: SQL error creating fuel stop table: %s", err_msg);
        sqlite3_free(err_msg);
    }

    /* Create fuel_samples table for adaptive consumption learning */
    sql = "CREATE TABLE IF NOT EXISTS driver_break_fuel_samples ("
          "id INTEGER PRIMARY KEY AUTOINCREMENT,"
          "timestamp INTEGER NOT NULL,"
          "distance_m REAL NOT NULL,"
          "fuel_used REAL NOT NULL,"
          "inst_consumption REAL NOT NULL,"
          "speed_kmh REAL NOT NULL,"
          "engine_load INTEGER"
          ");";

    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: SQL error creating fuel samples table: %s", err_msg);
        sqlite3_free(err_msg);
    }

    /* Create trip summary table */
    sql = "CREATE TABLE IF NOT EXISTS driver_break_trip_summaries ("
          "id INTEGER PRIMARY KEY AUTOINCREMENT,"
          "start_time INTEGER NOT NULL,"
          "end_time INTEGER NOT NULL,"
          "total_distance_m REAL NOT NULL,"
          "total_fuel REAL NOT NULL,"
          "avg_consumption REAL NOT NULL,"
          "peak_consumption REAL NOT NULL,"
          "high_load_flag INTEGER NOT NULL"
          ");";

    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: SQL error creating trip summary table: %s", err_msg);
        sqlite3_free(err_msg);
    }

    /* Clean up any corrupted entries that might exist */
    driver_break_db_clean_corrupted_config(db);

    dbg(lvl_info, "Driver Break plugin: Database initialized at %s", db_path);

    return db;
}

void driver_break_db_destroy(struct driver_break_db *db) {
    if (db) {
        if (db->db) {
            sqlite3_close(db->db);
        }
        g_free(db);
    }
}

int driver_break_db_add_history(struct driver_break_db *db, struct driver_break_stop_history *entry) {
    char *sql;
    char *err_msg = NULL;
    int rc;

    if (!db || !entry) {
        return 0;
    }

    sql = sqlite3_mprintf(
        "INSERT INTO driver_break_stop_history (timestamp, latitude, longitude, name, duration_minutes, was_mandatory) "
        "VALUES (%ld, %f, %f, %Q, %d, %d);",
        (long)entry->timestamp, entry->coord.lat, entry->coord.lng, entry->name ? entry->name : "",
        entry->duration_minutes, entry->was_mandatory);

    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: SQL error adding history: %s", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    return 1;
}

GList *driver_break_db_get_history(struct driver_break_db *db, time_t since) {
    GList *list = NULL;
    sqlite3_stmt *stmt;
    const char *sql;
    int rc;

    if (!db) {
        return NULL;
    }

    sql = "SELECT timestamp, latitude, longitude, name, duration_minutes, was_mandatory "
          "FROM driver_break_stop_history WHERE timestamp >= ? ORDER BY timestamp DESC;";

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: SQL error preparing history query: %s", sqlite3_errmsg(db->db));
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)since);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        struct driver_break_stop_history *entry = g_new0(struct driver_break_stop_history, 1);

        entry->timestamp = sqlite3_column_int64(stmt, 0);
        entry->coord.lat = sqlite3_column_double(stmt, 1);
        entry->coord.lng = sqlite3_column_double(stmt, 2);

        const char *name = (const char *)sqlite3_column_text(stmt, 3);
        if (name) {
            entry->name = g_strdup(name);
        }

        entry->duration_minutes = sqlite3_column_int(stmt, 4);
        entry->was_mandatory = sqlite3_column_int(stmt, SQLITE_COLUMN_INDEX_WAS_MANDATORY);

        list = g_list_prepend(list, entry);
    }

    sqlite3_finalize(stmt);

    return g_list_reverse(list);
}

int driver_break_db_clear_history(struct driver_break_db *db, time_t before) {
    char *sql;
    char *err_msg = NULL;
    int rc;

    if (!db) {
        return 0;
    }

    sql = sqlite3_mprintf("DELETE FROM driver_break_stop_history WHERE timestamp < %ld;", (long)before);

    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: SQL error clearing history: %s", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    return 1;
}

int driver_break_db_save_config(struct driver_break_db *db, struct driver_break_config *config) {
    char *sql;
    char *err_msg = NULL;
    int rc;

    if (!db || !config) {
        return 0;
    }

    /* Save configuration as key-value pairs */
    const char *keys[] = {"vehicle_type",
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
                          /* Fuel configuration keys */
                          "fuel_type",
                          "fuel_tank_capacity_l",
                          "fuel_avg_consumption_x10",
                          "fuel_obd_available",
                          "fuel_j1939_available",
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
                    /* Fuel configuration values */
                    config->fuel_type,
                    config->fuel_tank_capacity_l,
                    config->fuel_avg_consumption_x10,
                    config->fuel_obd_available,
                    config->fuel_j1939_available,
                    config->fuel_ethanol_manual_pct,
                    config->fuel_low_warning_km,
                    config->fuel_search_buffer_km,
                    config->fuel_high_load_threshold};

    int i;
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

    /* Clean up any corrupted entries first */
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
        int value;

        if (!key || !value_str) {
            continue;
        }

        /* Convert text to integer with validation - use strtol for better error handling */
        char *endptr;
        value = (int)strtol(value_str, &endptr, DECIMAL_BASE);
        if (*endptr != '\0' && *endptr != '\n') {
            dbg(lvl_warning, "Driver Break plugin: Invalid numeric value for %s: '%s', skipping", key, value_str);
            continue;
        }

        /* Check if conversion was successful (atoi returns 0 for invalid, but 0 might be valid) */
        /* So we check if the string represents a valid number */
        if (value_str[0] == '\0' || (value == 0 && value_str[0] != '0')) {
            dbg(lvl_warning, "Driver Break plugin: Invalid numeric value for %s: '%s', skipping", key, value_str);
            continue;
        }

        /* Only load non-zero values within reasonable bounds to preserve defaults */
        if (driver_break_db_load_config_value(key, value, config, &loaded_count) != 0) {
            dbg(lvl_debug, "Driver Break plugin: Unknown config key in database: %s", key);
        }
    }

    sqlite3_finalize(stmt);

    if (loaded_count > 0) {
        dbg(lvl_debug, "Driver Break plugin: Loaded %d configuration values from database", loaded_count);
        return 1;
    } else {
        dbg(lvl_debug, "Driver Break plugin: No valid configuration values found in database, using defaults");
        return 0;
    }
}

void driver_break_free_history_entry(struct driver_break_stop_history *entry) {
    if (!entry) {
        return;
    }

    if (entry->name) {
        g_free(entry->name);
    }

    g_free(entry);
}

int driver_break_db_add_fuel_stop(struct driver_break_db *db, struct driver_break_fuel_stop *stop) {
    char *sql;
    char *err_msg = NULL;
    int rc;

    if (!db || !stop) {
        return 0;
    }

    sql = sqlite3_mprintf(
        "INSERT INTO driver_break_fuel_stops "
        "(timestamp, latitude, longitude, fuel_added, fuel_level_after, cost, currency, ethanol_pct) "
        "VALUES (%ld, %f, %f, %f, %f, %f, %Q, %d);",
        (long)stop->timestamp, stop->coord.lat, stop->coord.lng, stop->fuel_added, stop->fuel_level_after,
        stop->cost, stop->currency ? stop->currency : "", stop->ethanol_pct);

    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: SQL error adding fuel stop: %s", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    return 1;
}

int driver_break_db_add_fuel_sample(struct driver_break_db *db, time_t timestamp, double distance_m, double fuel_used,
                                    double inst_consumption, double speed_kmh, int engine_load) {
    char *sql;
    char *err_msg = NULL;
    int rc;

    if (!db) {
        return 0;
    }

    sql = sqlite3_mprintf(
        "INSERT INTO driver_break_fuel_samples "
        "(timestamp, distance_m, fuel_used, inst_consumption, speed_kmh, engine_load) "
        "VALUES (%ld, %f, %f, %f, %f, %d);",
        (long)timestamp, distance_m, fuel_used, inst_consumption, speed_kmh, engine_load);

    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: SQL error adding fuel sample: %s", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    return 1;
}

int driver_break_db_add_trip_summary(struct driver_break_db *db, time_t start_time, time_t end_time,
                                     double total_distance_m, double total_fuel, double avg_consumption,
                                     double peak_consumption, int high_load_flag) {
    char *sql;
    char *err_msg = NULL;
    int rc;

    if (!db) {
        return 0;
    }

    sql = sqlite3_mprintf(
        "INSERT INTO driver_break_trip_summaries "
        "(start_time, end_time, total_distance_m, total_fuel, avg_consumption, peak_consumption, high_load_flag) "
        "VALUES (%ld, %ld, %f, %f, %f, %f, %d);",
        (long)start_time, (long)end_time, total_distance_m, total_fuel, avg_consumption, peak_consumption,
        high_load_flag ? 1 : 0);

    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: SQL error adding trip summary: %s", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    return 1;
}
