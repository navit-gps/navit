/**
 * @file navit_safety_cache.c
 * @brief SQLite-backed confirmation cache implementation.
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <stdlib.h>
#include <sqlite3.h>
#include "navit_safety_cache.h"

struct navit_safety_cache {
    sqlite3 *db;
};

static int cache_create_schema(sqlite3 *db) {
    static const char *sql =
        "CREATE TABLE IF NOT EXISTS poi_confirmations ("
        "poi_id TEXT NOT NULL,"
        "trip_id TEXT NOT NULL,"
        "confirmed_at INTEGER NOT NULL,"
        "PRIMARY KEY (poi_id, trip_id));";
    return sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK;
}

struct navit_safety_cache *navit_safety_cache_open(const char *path) {
    struct navit_safety_cache *cache;

    if (!path)
        return NULL;

    cache = calloc(1, sizeof(*cache));
    if (!cache)
        return NULL;

    if (sqlite3_open(path, &cache->db) != SQLITE_OK || !cache->db) {
        navit_safety_cache_close(cache);
        return NULL;
    }

    if (!cache_create_schema(cache->db)) {
        navit_safety_cache_close(cache);
        return NULL;
    }

    return cache;
}

void navit_safety_cache_close(struct navit_safety_cache *cache) {
    if (!cache)
        return;
    if (cache->db)
        sqlite3_close(cache->db);
    free(cache);
}

int navit_safety_cache_confirm(struct navit_safety_cache *cache, const char *poi_id,
                               const char *trip_id, long timestamp) {
    static const char *sql =
        "INSERT OR REPLACE INTO poi_confirmations (poi_id, trip_id, confirmed_at) "
        "VALUES (?, ?, ?);";
    sqlite3_stmt *stmt = NULL;
    int ok = 0;

    if (!cache || !cache->db || !poi_id || !trip_id)
        return 0;

    if (sqlite3_prepare_v2(cache->db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return 0;

    if (sqlite3_bind_text(stmt, 1, poi_id, -1, SQLITE_TRANSIENT) == SQLITE_OK &&
            sqlite3_bind_text(stmt, 2, trip_id, -1, SQLITE_TRANSIENT) == SQLITE_OK &&
            sqlite3_bind_int64(stmt, 3, (sqlite3_int64)timestamp) == SQLITE_OK) {
        ok = sqlite3_step(stmt) == SQLITE_DONE;
    }

    sqlite3_finalize(stmt);
    return ok;
}

int navit_safety_cache_is_confirmed(struct navit_safety_cache *cache, const char *poi_id,
                                    const char *trip_id) {
    static const char *sql =
        "SELECT 1 FROM poi_confirmations WHERE poi_id = ? AND trip_id = ? LIMIT 1;";
    sqlite3_stmt *stmt = NULL;
    int confirmed = 0;

    if (!cache || !cache->db || !poi_id || !trip_id)
        return 0;

    if (sqlite3_prepare_v2(cache->db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return 0;

    if (sqlite3_bind_text(stmt, 1, poi_id, -1, SQLITE_TRANSIENT) == SQLITE_OK &&
            sqlite3_bind_text(stmt, 2, trip_id, -1, SQLITE_TRANSIENT) == SQLITE_OK) {
        confirmed = sqlite3_step(stmt) == SQLITE_ROW;
    }

    sqlite3_finalize(stmt);
    return confirmed;
}
