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

#include "config.h"
#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <glib.h>
#include "debug.h"
#include "aprs_db.h"
#include "coord.h"

struct aprs_db {
    sqlite3 *db;
    char *db_path;
};

static int aprs_db_init_schema(struct aprs_db *db) {
    char *err_msg = NULL;
    const char *sql = 
        "CREATE TABLE IF NOT EXISTS stations ("
        "  callsign TEXT PRIMARY KEY,"
        "  latitude REAL NOT NULL,"
        "  longitude REAL NOT NULL,"
        "  altitude REAL,"
        "  course INTEGER,"
        "  speed INTEGER,"
        "  timestamp INTEGER NOT NULL,"
        "  comment TEXT,"
        "  symbol_table TEXT,"
        "  symbol_code TEXT,"
        "  path TEXT"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_timestamp ON stations(timestamp);"
        "CREATE INDEX IF NOT EXISTS idx_position ON stations(latitude, longitude);";
    
    int rc = sqlite3_exec(db->db, sql, NULL, 0, &err_msg);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "SQLite error: %s", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}

struct aprs_db *aprs_db_new(const char *db_path) {
    struct aprs_db *db = g_new0(struct aprs_db, 1);
    
    if (!db_path) {
        db_path = ":memory:";
    }
    
    db->db_path = g_strdup(db_path);
    
    int rc = sqlite3_open(db_path, &db->db);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "Cannot open database %s: %s", db_path, sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        g_free(db->db_path);
        g_free(db);
        return NULL;
    }
    
    if (!aprs_db_init_schema(db)) {
        sqlite3_close(db->db);
        g_free(db->db_path);
        g_free(db);
        return NULL;
    }
    
    return db;
}

void aprs_db_destroy(struct aprs_db *db) {
    if (!db) return;
    
    if (db->db) {
        sqlite3_close(db->db);
    }
    g_free(db->db_path);
    g_free(db);
}

static char *aprs_db_serialize_path(char **path, int count) {
    if (!path || count == 0) return NULL;
    
    GString *str = g_string_new("");
    for (int i = 0; i < count; i++) {
        if (i > 0) g_string_append_c(str, ',');
        g_string_append(str, path[i]);
    }
    return g_string_free(str, FALSE);
}

static int aprs_db_deserialize_path(const char *path_str, char ***path, int *count) {
    if (!path_str || !*path_str) {
        *path = NULL;
        *count = 0;
        return 1;
    }
    
    *path = g_strsplit(path_str, ",", -1);
    *count = g_strv_length(*path);
    return 1;
}

int aprs_db_update_station(struct aprs_db *db, struct aprs_station *station) {
    if (!db || !station || !station->callsign) return 0;
    
    sqlite3_stmt *stmt;
    const char *sql = 
        "INSERT OR REPLACE INTO stations "
        "(callsign, latitude, longitude, altitude, course, speed, timestamp, comment, symbol_table, symbol_code, path) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "SQLite prepare error: %s", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, station->callsign, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, station->position.lat);
    sqlite3_bind_double(stmt, 3, station->position.lng);
    sqlite3_bind_double(stmt, 4, station->altitude);
    sqlite3_bind_int(stmt, 5, station->course);
    sqlite3_bind_int(stmt, 6, station->speed);
    sqlite3_bind_int64(stmt, 7, (sqlite3_int64)station->timestamp);
    sqlite3_bind_text(stmt, 8, station->comment, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, &station->symbol_table, 1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, &station->symbol_code, 1, SQLITE_STATIC);
    
    char *path_str = aprs_db_serialize_path(station->path, station->path_count);
    sqlite3_bind_text(stmt, 11, path_str, -1, SQLITE_TRANSIENT);
    g_free(path_str);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        dbg(lvl_error, "SQLite step error: %s", sqlite3_errmsg(db->db));
        return 0;
    }
    
    return 1;
}

int aprs_db_get_station(struct aprs_db *db, const char *callsign, struct aprs_station *station) {
    if (!db || !callsign || !station) return 0;
    
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT callsign, latitude, longitude, altitude, course, speed, timestamp, "
        "comment, symbol_table, symbol_code, path FROM stations WHERE callsign = ?";
    
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "SQLite prepare error: %s", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, callsign, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        station->callsign = g_strdup((const char *)sqlite3_column_text(stmt, 0));
        station->position.lat = sqlite3_column_double(stmt, 1);
        station->position.lng = sqlite3_column_double(stmt, 2);
        station->altitude = sqlite3_column_double(stmt, 3);
        station->course = sqlite3_column_int(stmt, 4);
        station->speed = sqlite3_column_int(stmt, 5);
        station->timestamp = (time_t)sqlite3_column_int64(stmt, 6);
        station->comment = g_strdup((const char *)sqlite3_column_text(stmt, 7));
        const char *sym_tbl = (const char *)sqlite3_column_text(stmt, 8);
        const char *sym_code = (const char *)sqlite3_column_text(stmt, 9);
        if (sym_tbl && *sym_tbl) station->symbol_table = sym_tbl[0];
        if (sym_code && *sym_code) station->symbol_code = sym_code[0];
        
        const char *path_str = (const char *)sqlite3_column_text(stmt, 10);
        aprs_db_deserialize_path(path_str, &station->path, &station->path_count);
        
        sqlite3_finalize(stmt);
        return 1;
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

int aprs_db_delete_station(struct aprs_db *db, const char *callsign) {
    if (!db || !callsign) return 0;
    
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM stations WHERE callsign = ?";
    
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "SQLite prepare error: %s", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, callsign, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        dbg(lvl_error, "SQLite step error: %s", sqlite3_errmsg(db->db));
        return 0;
    }
    
    return 1;
}

int aprs_db_delete_expired(struct aprs_db *db, time_t expire_seconds) {
    if (!db) return 0;
    
    time_t expire_time = time(NULL) - expire_seconds;
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM stations WHERE timestamp < ?";
    
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "SQLite prepare error: %s", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)expire_time);
    
    rc = sqlite3_step(stmt);
    int deleted = sqlite3_changes(db->db);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        dbg(lvl_error, "SQLite step error: %s", sqlite3_errmsg(db->db));
        return 0;
    }
    
    return deleted;
}

static double haversine_distance(const struct coord_geo *a, const struct coord_geo *b) {
    const double R = 6371.0;
    double dlat = (b->lat - a->lat) * M_PI / 180.0;
    double dlon = (b->lng - a->lng) * M_PI / 180.0;
    double lat1 = a->lat * M_PI / 180.0;
    double lat2 = b->lat * M_PI / 180.0;
    
    double a_val = sin(dlat/2) * sin(dlat/2) +
                   cos(lat1) * cos(lat2) * sin(dlon/2) * sin(dlon/2);
    double c = 2 * atan2(sqrt(a_val), sqrt(1-a_val));
    
    return R * c;
}

int aprs_db_get_stations_in_range(struct aprs_db *db, const struct coord_geo *center, double range_km, 
                                  GList **stations) {
    if (!db || !center || !stations) return 0;
    
    *stations = NULL;
    
    sqlite3_stmt *stmt;
    const char *sql = "SELECT callsign, latitude, longitude, altitude, course, speed, "
                     "timestamp, comment, symbol_table, symbol_code, path FROM stations";
    
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "SQLite prepare error: %s", sqlite3_errmsg(db->db));
        return 0;
    }
    
    int count = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        struct aprs_station *station = aprs_station_new();
        
        station->callsign = g_strdup((const char *)sqlite3_column_text(stmt, 0));
        station->position.lat = sqlite3_column_double(stmt, 1);
        station->position.lng = sqlite3_column_double(stmt, 2);
        station->altitude = sqlite3_column_double(stmt, 3);
        station->course = sqlite3_column_int(stmt, 4);
        station->speed = sqlite3_column_int(stmt, 5);
        station->timestamp = (time_t)sqlite3_column_int64(stmt, 6);
        station->comment = g_strdup((const char *)sqlite3_column_text(stmt, 7));
        const char *sym_tbl = (const char *)sqlite3_column_text(stmt, 8);
        const char *sym_code = (const char *)sqlite3_column_text(stmt, 9);
        if (sym_tbl && *sym_tbl) station->symbol_table = sym_tbl[0];
        if (sym_code && *sym_code) station->symbol_code = sym_code[0];
        
        const char *path_str = (const char *)sqlite3_column_text(stmt, 10);
        aprs_db_deserialize_path(path_str, &station->path, &station->path_count);
        
        double distance = haversine_distance(center, &station->position);
        if (distance <= range_km) {
            *stations = g_list_append(*stations, station);
            count++;
        } else {
            aprs_station_free(station);
        }
    }
    
    sqlite3_finalize(stmt);
    return count;
}

int aprs_db_get_all_stations(struct aprs_db *db, GList **stations) {
    if (!db || !stations) return 0;
    
    *stations = NULL;
    
    sqlite3_stmt *stmt;
    const char *sql = "SELECT callsign, latitude, longitude, altitude, course, speed, "
                     "timestamp, comment, symbol_table, symbol_code, path FROM stations";
    
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        dbg(lvl_error, "SQLite prepare error: %s", sqlite3_errmsg(db->db));
        return 0;
    }
    
    int count = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        struct aprs_station *station = aprs_station_new();
        
        station->callsign = g_strdup((const char *)sqlite3_column_text(stmt, 0));
        station->position.lat = sqlite3_column_double(stmt, 1);
        station->position.lng = sqlite3_column_double(stmt, 2);
        station->altitude = sqlite3_column_double(stmt, 3);
        station->course = sqlite3_column_int(stmt, 4);
        station->speed = sqlite3_column_int(stmt, 5);
        station->timestamp = (time_t)sqlite3_column_int64(stmt, 6);
        station->comment = g_strdup((const char *)sqlite3_column_text(stmt, 7));
        const char *sym_tbl = (const char *)sqlite3_column_text(stmt, 8);
        const char *sym_code = (const char *)sqlite3_column_text(stmt, 9);
        if (sym_tbl && *sym_tbl) station->symbol_table = sym_tbl[0];
        if (sym_code && *sym_code) station->symbol_code = sym_code[0];
        
        const char *path_str = (const char *)sqlite3_column_text(stmt, 10);
        aprs_db_deserialize_path(path_str, &station->path, &station->path_count);
        
        *stations = g_list_append(*stations, station);
        count++;
    }
    
    sqlite3_finalize(stmt);
    return count;
}

struct aprs_station *aprs_station_new(void) {
    struct aprs_station *station = g_new0(struct aprs_station, 1);
    station->altitude = 0.0;
    station->course = -1;
    station->speed = -1;
    station->timestamp = time(NULL);
    station->symbol_table = '/';
    station->symbol_code = '>';
    station->path_count = 0;
    station->path = NULL;
    return station;
}

void aprs_station_free(struct aprs_station *station) {
    if (!station) return;
    
    g_free(station->callsign);
    g_free(station->comment);
    if (station->path) {
        g_strfreev(station->path);
    }
    g_free(station);
}

