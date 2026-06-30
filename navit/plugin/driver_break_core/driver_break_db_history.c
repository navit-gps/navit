/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "debug.h"
#include "driver_break_db_priv.h"
#include <glib.h>

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

void driver_break_free_history_entry(struct driver_break_stop_history *entry) {
    if (!entry) {
        return;
    }

    if (entry->name) {
        g_free(entry->name);
    }

    g_free(entry);
}
