/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "debug.h"
#include "driver_break_db_priv.h"

int driver_break_db_add_fuel_stop(struct driver_break_db *db, struct driver_break_fuel_stop *stop) {
    char *sql;
    char *err_msg = NULL;
    int rc;

    if (!db || !stop) {
        return 0;
    }

    sql = sqlite3_mprintf("INSERT INTO driver_break_fuel_stops "
                          "(timestamp, latitude, longitude, fuel_added, fuel_level_after, cost, currency, ethanol_pct) "
                          "VALUES (%ld, %f, %f, %f, %f, %f, %Q, %d);",
                          (long)stop->timestamp, stop->coord.lat, stop->coord.lng, stop->fuel_added,
                          stop->fuel_level_after, stop->cost, stop->currency ? stop->currency : "", stop->ethanol_pct);

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

    sql = sqlite3_mprintf("INSERT INTO driver_break_fuel_samples "
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
