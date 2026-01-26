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
#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <glib.h>
#include "debug.h"
#include "file.h"
#include "rest_db.h"
#include "rest.h"

struct rest_db {
    sqlite3 *db;
};

/* Clean up corrupted config entries from database */
static void rest_db_clean_corrupted_config(struct rest_db *db) {
    char *sql;
    char *err_msg = NULL;
    int rc;
    
    if (!db || !db->db) return;
    
    /* Delete entries with invalid values (too large, negative, etc.) */
    sql = "DELETE FROM rest_config WHERE "
          "CAST(value AS INTEGER) < 0 OR "
          "CAST(value AS INTEGER) > 1000000 OR "
          "value IS NULL OR "
          "value = '';";
    
    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    if (rc != SQLITE_OK) {
        dbg(lvl_warning, "Rest plugin: Error cleaning corrupted config: %s", err_msg);
        sqlite3_free(err_msg);
    } else {
        dbg(lvl_debug, "Rest plugin: Cleaned corrupted config entries");
    }
}

struct rest_db *rest_db_new(const char *db_path) {
    struct rest_db *db;
    char *err_msg = NULL;
    int rc;
    const char *sql;
    char *db_dir;
    
    db = g_new0(struct rest_db, 1);
    
    /* Ensure directory exists */
    db_dir = g_path_get_dirname(db_path);
    if (db_dir && !file_is_dir(db_dir)) {
        /* Create directory if it doesn't exist */
        if (file_mkdir(db_dir, 1) != 0) {
            dbg(lvl_warning, "Rest plugin: Could not create database directory %s", db_dir);
        }
    }
    g_free(db_dir);
    
    rc = sqlite3_open(db_path, &db->db);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Rest plugin: Cannot open database %s: %s", 
            db_path, sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        g_free(db);
        return NULL;
    }
    
    /* Create rest stop history table */
    sql = "CREATE TABLE IF NOT EXISTS rest_stop_history ("
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
        dbg(lvl_error, "Rest plugin: SQL error creating history table: %s", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db->db);
        g_free(db);
        return NULL;
    }
    
    /* Create config table */
    sql = "CREATE TABLE IF NOT EXISTS rest_config ("
          "key TEXT PRIMARY KEY,"
          "value TEXT"
          ");";
    
    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Rest plugin: SQL error creating config table: %s", err_msg);
        sqlite3_free(err_msg);
    }
    
    /* Clean up any corrupted entries that might exist */
    rest_db_clean_corrupted_config(db);
    
    dbg(lvl_info, "Rest plugin: Database initialized at %s", db_path);
    
    return db;
}

void rest_db_destroy(struct rest_db *db) {
    if (db) {
        if (db->db) {
            sqlite3_close(db->db);
        }
        g_free(db);
    }
}

int rest_db_add_history(struct rest_db *db, struct rest_stop_history *entry) {
    char *sql;
    char *err_msg = NULL;
    int rc;
    
    if (!db || !entry) {
        return 0;
    }
    
    sql = sqlite3_mprintf(
        "INSERT INTO rest_stop_history (timestamp, latitude, longitude, name, duration_minutes, was_mandatory) "
        "VALUES (%ld, %f, %f, %Q, %d, %d);",
        (long)entry->timestamp,
        entry->coord.lat,
        entry->coord.lng,
        entry->name ? entry->name : "",
        entry->duration_minutes,
        entry->was_mandatory
    );
    
    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    sqlite3_free(sql);
    
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Rest plugin: SQL error adding history: %s", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    
    return 1;
}

GList *rest_db_get_history(struct rest_db *db, time_t since) {
    GList *list = NULL;
    sqlite3_stmt *stmt;
    const char *sql;
    int rc;
    
    if (!db) {
        return NULL;
    }
    
    sql = "SELECT timestamp, latitude, longitude, name, duration_minutes, was_mandatory "
          "FROM rest_stop_history WHERE timestamp >= ? ORDER BY timestamp DESC;";
    
    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Rest plugin: SQL error preparing history query: %s", 
            sqlite3_errmsg(db->db));
        return NULL;
    }
    
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)since);
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        struct rest_stop_history *entry = g_new0(struct rest_stop_history, 1);
        
        entry->timestamp = (time_t)sqlite3_column_int64(stmt, 0);
        entry->coord.lat = sqlite3_column_double(stmt, 1);
        entry->coord.lng = sqlite3_column_double(stmt, 2);
        
        const char *name = (const char *)sqlite3_column_text(stmt, 3);
        if (name) {
            entry->name = g_strdup(name);
        }
        
        entry->duration_minutes = sqlite3_column_int(stmt, 4);
        entry->was_mandatory = sqlite3_column_int(stmt, 5);
        
        list = g_list_prepend(list, entry);
    }
    
    sqlite3_finalize(stmt);
    
    return g_list_reverse(list);
}

int rest_db_clear_history(struct rest_db *db, time_t before) {
    char *sql;
    char *err_msg = NULL;
    int rc;
    
    if (!db) {
        return 0;
    }
    
    sql = sqlite3_mprintf("DELETE FROM rest_stop_history WHERE timestamp < %ld;", 
                          (long)before);
    
    rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    sqlite3_free(sql);
    
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Rest plugin: SQL error clearing history: %s", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    
    return 1;
}

int rest_db_save_config(struct rest_db *db, struct rest_config *config) {
    char *sql;
    char *err_msg = NULL;
    int rc;
    
    if (!db || !config) {
        return 0;
    }
    
    /* Save configuration as key-value pairs */
    const char *keys[] = {
        "vehicle_type",
        "car_soft_limit_hours",
        "car_max_hours",
        "car_break_interval_hours",
        "car_break_duration_min",
        "truck_mandatory_break_after_hours",
        "truck_mandatory_break_duration_min",
        "truck_max_daily_hours",
        "min_distance_from_buildings",
        "poi_search_radius_km",
        "rest_interval_min_hours",
        "rest_interval_max_hours"
    };
    
    int values[] = {
        config->vehicle_type,
        config->car_soft_limit_hours,
        config->car_max_hours,
        config->car_break_interval_hours,
        config->car_break_duration_min,
        config->truck_mandatory_break_after_hours,
        config->truck_mandatory_break_duration_min,
        config->truck_max_daily_hours,
        config->min_distance_from_buildings,
        config->poi_search_radius_km,
        config->rest_interval_min_hours,
        config->rest_interval_max_hours
    };
    
    for (int i = 0; i < G_N_ELEMENTS(keys); i++) {
        sql = sqlite3_mprintf(
            "INSERT OR REPLACE INTO rest_config (key, value) VALUES (%Q, %d);",
            keys[i], values[i]
        );
        
        rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
        sqlite3_free(sql);
        
        if (rc != SQLITE_OK) {
            dbg(lvl_error, "Rest plugin: SQL error saving config %s: %s", keys[i], err_msg);
            sqlite3_free(err_msg);
        }
    }
    
    return 1;
}

int rest_db_load_config(struct rest_db *db, struct rest_config *config) {
    sqlite3_stmt *stmt;
    const char *sql;
    int rc;
    int loaded_count = 0;
    
    if (!db || !config) {
        return 0;
    }
    
    /* Clean up any corrupted entries first */
    rest_db_clean_corrupted_config(db);
    
    sql = "SELECT key, value FROM rest_config WHERE value IS NOT NULL AND value != '';";
    
    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        dbg(lvl_debug, "Rest plugin: No saved configuration found, using defaults");
        return 0;
    }
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char *key = (const char *)sqlite3_column_text(stmt, 0);
        const char *value_str = (const char *)sqlite3_column_text(stmt, 1);
        int value;
        
        if (!key || !value_str) continue;
        
        /* Convert text to integer with validation */
        value = atoi(value_str);
        
        /* Check if conversion was successful (atoi returns 0 for invalid, but 0 might be valid) */
        /* So we check if the string represents a valid number */
        if (value_str[0] == '\0' || (value == 0 && value_str[0] != '0')) {
            dbg(lvl_warning, "Rest plugin: Invalid numeric value for %s: '%s', skipping", key, value_str);
            continue;
        }
        
        /* Only load non-zero values within reasonable bounds to preserve defaults */
        if (!strcmp(key, "vehicle_type")) {
            if (value >= 0 && value <= 3) {
                config->vehicle_type = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid vehicle_type value: %d, using default", value);
            }
        } else if (!strcmp(key, "car_soft_limit_hours")) {
            if (value > 0 && value <= 12) {
                config->car_soft_limit_hours = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid car_soft_limit_hours value: %d, using default", value);
            }
        } else if (!strcmp(key, "car_max_hours")) {
            if (value > 0 && value <= 12) {
                config->car_max_hours = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid car_max_hours value: %d, using default", value);
            }
        } else if (!strcmp(key, "car_break_interval_hours")) {
            if (value > 0 && value <= 12) {
                config->car_break_interval_hours = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid car_break_interval_hours value: %d, using default", value);
            }
        } else if (!strcmp(key, "car_break_duration_min")) {
            if (value > 0 && value <= 120) {
                config->car_break_duration_min = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid car_break_duration_min value: %d, using default", value);
            }
        } else if (!strcmp(key, "truck_mandatory_break_after_hours")) {
            if (value > 0 && value <= 12) {
                config->truck_mandatory_break_after_hours = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid truck_mandatory_break_after_hours value: %d, using default", value);
            }
        } else if (!strcmp(key, "truck_mandatory_break_duration_min")) {
            if (value > 0 && value <= 120) {
                config->truck_mandatory_break_duration_min = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid truck_mandatory_break_duration_min value: %d, using default", value);
            }
        } else if (!strcmp(key, "truck_max_daily_hours")) {
            if (value > 0 && value <= 12) {
                config->truck_max_daily_hours = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid truck_max_daily_hours value: %d, using default", value);
            }
        } else if (!strcmp(key, "min_distance_from_buildings")) {
            if (value > 0 && value <= 10000) {  /* Max 10 km */
                config->min_distance_from_buildings = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid min_distance_from_buildings value: %d, using default", value);
            }
        } else if (!strcmp(key, "poi_search_radius_km")) {
            if (value > 0 && value <= 1000) {  /* Max 1000 km */
                config->poi_search_radius_km = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid poi_search_radius_km value: %d, using default", value);
            }
        } else if (!strcmp(key, "poi_water_search_radius_km")) {
            if (value > 0 && value <= 100) {  /* Max 100 km */
                config->poi_water_search_radius_km = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid poi_water_search_radius_km value: %d, using default", value);
            }
        } else if (!strcmp(key, "poi_cabin_search_radius_km")) {
            if (value > 0 && value <= 100) {  /* Max 100 km */
                config->poi_cabin_search_radius_km = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid poi_cabin_search_radius_km value: %d, using default", value);
            }
        } else if (!strcmp(key, "min_distance_from_glaciers")) {
            if (value > 0 && value <= 10000) {  /* Max 10 km */
                config->min_distance_from_glaciers = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid min_distance_from_glaciers value: %d, using default", value);
            }
        } else if (!strcmp(key, "rest_interval_min_hours")) {
            if (value > 0 && value <= 24) {
                config->rest_interval_min_hours = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid rest_interval_min_hours value: %d, using default", value);
            }
        } else if (!strcmp(key, "rest_interval_max_hours")) {
            if (value > 0 && value <= 24) {
                config->rest_interval_max_hours = value;
                loaded_count++;
            } else {
                dbg(lvl_warning, "Rest plugin: Invalid rest_interval_max_hours value: %d, using default", value);
            }
        } else {
            dbg(lvl_debug, "Rest plugin: Unknown config key in database: %s", key);
        }
    }
    
    sqlite3_finalize(stmt);
    
    if (loaded_count > 0) {
        dbg(lvl_debug, "Rest plugin: Loaded %d configuration values from database", loaded_count);
        return 1;
    } else {
        dbg(lvl_debug, "Rest plugin: No valid configuration values found in database, using defaults");
        return 0;
    }
}
