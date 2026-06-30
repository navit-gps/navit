/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "debug.h"
#include "driver_break_db_priv.h"
#include "file.h"
#include <glib.h>
#include <stdlib.h>

static int driver_break_db_exec_sql(sqlite3 *db, const char *sql, const char *label) {
    char *err_msg = NULL;
    int rc;

    rc = sqlite3_exec(db, sql, NULL, 0, &err_msg);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: SQL error creating %s: %s", label, err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    return 1;
}

static int driver_break_db_create_tables(sqlite3 *db) {
    static const struct {
        const char *label;
        const char *sql;
        int required;
    } schemas[] = {
        {"history table",
         "CREATE TABLE IF NOT EXISTS driver_break_stop_history ("
         "id INTEGER PRIMARY KEY AUTOINCREMENT,"
         "timestamp INTEGER NOT NULL,"
         "latitude REAL NOT NULL,"
         "longitude REAL NOT NULL,"
         "name TEXT,"
         "duration_minutes INTEGER,"
         "was_mandatory INTEGER"
         ");", 1},
        {"config table",
         "CREATE TABLE IF NOT EXISTS driver_break_config ("
         "key TEXT PRIMARY KEY,"
         "value TEXT"
         ");", 0},
        {"fuel stop table",
         "CREATE TABLE IF NOT EXISTS driver_break_fuel_stops ("
         "id INTEGER PRIMARY KEY AUTOINCREMENT,"
         "timestamp INTEGER NOT NULL,"
         "latitude REAL NOT NULL,"
         "longitude REAL NOT NULL,"
         "fuel_added REAL,"
         "fuel_level_after REAL,"
         "cost REAL,"
         "currency TEXT,"
         "ethanol_pct INTEGER"
         ");", 0},
        {"fuel samples table",
         "CREATE TABLE IF NOT EXISTS driver_break_fuel_samples ("
         "id INTEGER PRIMARY KEY AUTOINCREMENT,"
         "timestamp INTEGER NOT NULL,"
         "distance_m REAL NOT NULL,"
         "fuel_used REAL NOT NULL,"
         "inst_consumption REAL NOT NULL,"
         "speed_kmh REAL NOT NULL,"
         "engine_load INTEGER"
         ");", 0},
        {"trip summary table",
         "CREATE TABLE IF NOT EXISTS driver_break_trip_summaries ("
         "id INTEGER PRIMARY KEY AUTOINCREMENT,"
         "start_time INTEGER NOT NULL,"
         "end_time INTEGER NOT NULL,"
         "total_distance_m REAL NOT NULL,"
         "total_fuel REAL NOT NULL,"
         "avg_consumption REAL NOT NULL,"
         "peak_consumption REAL NOT NULL,"
         "high_load_flag INTEGER NOT NULL"
         ");", 0},
    };
    size_t i;

    for (i = 0; i < G_N_ELEMENTS(schemas); i++) {
        if (!driver_break_db_exec_sql(db, schemas[i].sql, schemas[i].label)) {
            return schemas[i].required ? 0 : 1;
        }
    }

    return 1;
}

static int driver_break_db_ensure_directory(const char *db_path) {
    char *db_dir;

    db_dir = g_path_get_dirname(db_path);
    if (db_dir && !file_is_dir(db_dir)) {
        if (file_mkdir(db_dir, 1) != 0) {
            dbg(lvl_warning, "Driver Break plugin: Could not create database directory %s", db_dir);
        }
    }
    g_free(db_dir);
    return 1;
}

struct driver_break_db *driver_break_db_new(const char *db_path) {
    struct driver_break_db *db;
    int rc;

    db = g_new0(struct driver_break_db, 1);
    driver_break_db_ensure_directory(db_path);

    rc = sqlite3_open(db_path, &db->db);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Driver Break plugin: Cannot open database %s: %s", db_path, sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        g_free(db);
        return NULL;
    }

    if (!driver_break_db_create_tables(db->db)) {
        sqlite3_close(db->db);
        g_free(db);
        return NULL;
    }

    driver_break_db_clean_corrupted_config(db);
    dbg(lvl_info, "Driver Break plugin: Database initialized at %s", db_path);
    return db;
}

void driver_break_db_destroy(struct driver_break_db *db) {
    if (!db) {
        return;
    }

    if (db->db) {
        sqlite3_close(db->db);
    }
    g_free(db);
}
