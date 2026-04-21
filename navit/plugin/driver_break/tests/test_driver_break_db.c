/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Unit tests for Driver Break plugin database operations
 */

#include "../../debug.h"
#include "../driver_break.h"
#include "../driver_break_db.h"
#include <glib.h>
#include <math.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Stub for debug_printf and max_debug_level for unit tests */
dbg_level max_debug_level = lvl_error;

void debug_printf(dbg_level level, const char *module, const int mlen, const char *function, const int flen, int prefix,
                  const char *fmt, ...) {
    (void)level;
    (void)module;
    (void)mlen;
    (void)function;
    (void)flen;
    (void)prefix;
    (void)fmt;
    /* No-op for tests */
}

#define TEST_ASSERT(condition, message)                                                                                \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static char *create_temp_db(void) {
    char template[] = "/tmp/driver_break_test_db_XXXXXX";
    int fd = mkstemp(template);
    if (fd == -1) {
        return NULL;
    }
    close(fd);
    unlink(template); /* Remove file, we just want the name */
    return g_strdup(template);
}

static int test_db_create_destroy(void) {
    char *db_path = create_temp_db();
    struct driver_break_db *db = driver_break_db_new(db_path);
    TEST_ASSERT(db != NULL, "Database creation failed");
    driver_break_db_destroy(db);
    unlink(db_path);
    g_free(db_path);
    return 0;
}

static int test_db_add_history(void) {
    char *db_path = create_temp_db();
    struct driver_break_db *db = driver_break_db_new(db_path);
    TEST_ASSERT(db != NULL, "Database creation failed");

    struct driver_break_stop_history entry;
    memset(&entry, 0, sizeof(entry));
    entry.timestamp = time(NULL);
    entry.coord.lat = 37.7749;
    entry.coord.lng = -122.4194;
    entry.name = g_strdup("Test Rest Stop");
    entry.duration_minutes = 30;
    entry.was_mandatory = 1;

    int result = driver_break_db_add_history(db, &entry);
    TEST_ASSERT(result == 1, "History insert failed");

    g_free(entry.name);
    driver_break_db_destroy(db);
    unlink(db_path);
    g_free(db_path);
    return 0;
}

static int test_db_get_history(void) {
    char *db_path = create_temp_db();
    struct driver_break_db *db = driver_break_db_new(db_path);
    TEST_ASSERT(db != NULL, "Database creation failed");

    time_t now = time(NULL);
    struct driver_break_stop_history entry;
    memset(&entry, 0, sizeof(entry));
    entry.timestamp = now;
    entry.coord.lat = 40.7128;
    entry.coord.lng = -74.0060;
    entry.name = g_strdup("New York Rest Stop");
    entry.duration_minutes = 45;
    entry.was_mandatory = 0;

    driver_break_db_add_history(db, &entry);

    GList *history = driver_break_db_get_history(db, now - 100);
    TEST_ASSERT(history != NULL, "History retrieval failed");
    TEST_ASSERT(g_list_length(history) == 1, "Wrong number of history entries");

    struct driver_break_stop_history *retrieved = (struct driver_break_stop_history *)history->data;
    TEST_ASSERT(retrieved->coord.lat == 40.7128, "Latitude mismatch");
    TEST_ASSERT(retrieved->coord.lng == -74.0060, "Longitude mismatch");
    TEST_ASSERT(retrieved->duration_minutes == 45, "Duration mismatch");
    TEST_ASSERT(retrieved->was_mandatory == 0, "Mandatory flag mismatch");

    /* Free history list */
    GList *l = history;
    while (l) {
        struct driver_break_stop_history *h = (struct driver_break_stop_history *)l->data;
        g_free(h->name);
        g_free(h);
        l = g_list_next(l);
    }
    g_list_free(history);

    g_free(entry.name);
    driver_break_db_destroy(db);
    unlink(db_path);
    g_free(db_path);
    return 0;
}

static int test_db_clear_history(void) {
    char *db_path = create_temp_db();
    struct driver_break_db *db = driver_break_db_new(db_path);
    TEST_ASSERT(db != NULL, "Database creation failed");

    time_t now = time(NULL);
    struct driver_break_stop_history entry1, entry2;

    memset(&entry1, 0, sizeof(entry1));
    entry1.timestamp = now - 200;
    entry1.coord.lat = 51.5074;
    entry1.coord.lng = -0.1278;
    entry1.name = g_strdup("Old Entry");
    entry1.duration_minutes = 30;

    memset(&entry2, 0, sizeof(entry2));
    entry2.timestamp = now;
    entry2.coord.lat = 52.5200;
    entry2.coord.lng = 13.4050;
    entry2.name = g_strdup("Recent Entry");
    entry2.duration_minutes = 45;

    driver_break_db_add_history(db, &entry1);
    driver_break_db_add_history(db, &entry2);

    /* Clear entries older than now - 100 */
    int result = driver_break_db_clear_history(db, now - 100);
    TEST_ASSERT(result == 1, "Clear history failed");

    GList *history = driver_break_db_get_history(db, 0);
    TEST_ASSERT(history != NULL, "History retrieval after clear failed");
    TEST_ASSERT(g_list_length(history) == 1, "Wrong number of entries after clear");

    struct driver_break_stop_history *retrieved = (struct driver_break_stop_history *)history->data;
    TEST_ASSERT(strcmp(retrieved->name, "Recent Entry") == 0, "Wrong entry remaining");

    /* Free history */
    GList *l = history;
    while (l) {
        struct driver_break_stop_history *h = (struct driver_break_stop_history *)l->data;
        g_free(h->name);
        g_free(h);
        l = g_list_next(l);
    }
    g_list_free(history);

    g_free(entry1.name);
    g_free(entry2.name);
    driver_break_db_destroy(db);
    unlink(db_path);
    g_free(db_path);
    return 0;
}

static int test_db_save_load_config(void) {
    char *db_path = create_temp_db();
    struct driver_break_db *db = driver_break_db_new(db_path);
    TEST_ASSERT(db != NULL, "Database creation failed");

    struct driver_break_config config;
    memset(&config, 0, sizeof(config));
    config.vehicle_type = DRIVER_BREAK_VEHICLE_TRUCK;
    config.car_soft_limit_hours = 7;
    config.car_max_hours = 10;
    config.truck_mandatory_break_after_hours = 4;
    config.truck_mandatory_break_duration_min = 45;
    config.min_distance_from_buildings = 150;
    config.poi_search_radius_km = 15;

    /* Fuel-related configuration fields */
    config.fuel_type = DRIVER_BREAK_FUEL_DIESEL;
    config.fuel_tank_capacity_l = 70;
    config.fuel_avg_consumption_x10 = 85; /* 8.5 L/100km */
    config.fuel_obd_available = 1;
    config.fuel_j1939_available = 0;
    config.fuel_ethanol_manual_pct = 10;
    config.fuel_low_warning_km = 100;
    config.fuel_search_buffer_km = 25;
    config.fuel_high_load_threshold = 30;
    config.total_weight = 1520.0;
    config.energy_drag_cd = 0.31;
    config.energy_frontal_area_sqm = 2.45;

    int result = driver_break_db_save_config(db, &config);
    TEST_ASSERT(result == 1, "Config save failed");

    struct driver_break_config loaded_config;
    memset(&loaded_config, 0, sizeof(loaded_config));
    result = driver_break_db_load_config(db, &loaded_config);
    TEST_ASSERT(result == 1, "Config load failed");
    TEST_ASSERT(loaded_config.vehicle_type == DRIVER_BREAK_VEHICLE_TRUCK, "Vehicle type mismatch");
    TEST_ASSERT(loaded_config.car_soft_limit_hours == 7, "Car soft limit mismatch");
    TEST_ASSERT(loaded_config.truck_mandatory_break_after_hours == 4, "Truck break hours mismatch");
    TEST_ASSERT(loaded_config.min_distance_from_buildings == 150, "Min distance mismatch");

    /* Verify fuel configuration persisted correctly */
    TEST_ASSERT(loaded_config.fuel_type == DRIVER_BREAK_FUEL_DIESEL, "Fuel type mismatch");
    TEST_ASSERT(loaded_config.fuel_tank_capacity_l == 70, "Fuel tank capacity mismatch");
    TEST_ASSERT(loaded_config.fuel_avg_consumption_x10 == 85, "Fuel avg consumption mismatch");
    TEST_ASSERT(loaded_config.fuel_obd_available == 1, "Fuel OBD flag mismatch");
    TEST_ASSERT(loaded_config.fuel_j1939_available == 0, "Fuel J1939 flag mismatch");
    TEST_ASSERT(loaded_config.fuel_ethanol_manual_pct == 10, "Fuel ethanol manual pct mismatch");
    TEST_ASSERT(loaded_config.fuel_low_warning_km == 100, "Fuel low warning km mismatch");
    TEST_ASSERT(loaded_config.fuel_search_buffer_km == 25, "Fuel search buffer km mismatch");
    TEST_ASSERT(loaded_config.fuel_high_load_threshold == 30, "Fuel high load threshold mismatch");
    TEST_ASSERT(fabs(loaded_config.total_weight - 1520.0) < 1e-5, "total_weight mismatch");
    TEST_ASSERT(fabs(loaded_config.energy_drag_cd - 0.31) < 1e-5, "energy_drag_cd mismatch");
    TEST_ASSERT(fabs(loaded_config.energy_frontal_area_sqm - 2.45) < 1e-5, "energy_frontal_area_sqm mismatch");

    driver_break_db_destroy(db);
    unlink(db_path);
    g_free(db_path);
    return 0;
}

/* Simulate malformed or corrupted config entries (e.g. due to power loss) and
 * verify that driver_break_db_clean_corrupted_config()/load_config discards them
 * and falls back to defaults rather than propagating invalid values. */
static int test_db_malformed_config_entries(void) {
    char *db_path = create_temp_db();
    struct driver_break_db *db = driver_break_db_new(db_path);
    TEST_ASSERT(db != NULL, "Database creation failed");

    sqlite3 *raw = NULL;
    int rc = sqlite3_open(db_path, &raw);
    TEST_ASSERT(rc == SQLITE_OK, "Failed to open raw SQLite database");

    /* Inject malformed and out-of-range entries into config table. */
    rc = sqlite3_exec(raw,
                      "INSERT INTO driver_break_config (key, value) VALUES "
                      "('car_soft_limit_hours', '-5'),"            /* negative */
                      "('truck_max_daily_hours', '9999'),"         /* too large */
                      "('fuel_avg_consumption_x10', 'not_a_num')", /* non-numeric */
                      NULL, NULL, NULL);
    TEST_ASSERT(rc == SQLITE_OK, "Failed to insert malformed config rows");

    sqlite3_close(raw);

    /* Load configuration; malformed entries should be cleaned and skipped. */
    struct driver_break_config loaded_config;
    memset(&loaded_config, 0, sizeof(loaded_config));
    int result = driver_break_db_load_config(db, &loaded_config);

    /* We only wrote malformed entries, all of which should be discarded by the
     * cleaner. load_config may therefore return 0, and the struct must remain
     * at its zero-initialized defaults. */
    TEST_ASSERT(result == 0, "Malformed-only config should not be reported as successfully loaded");
    TEST_ASSERT(loaded_config.car_soft_limit_hours == 0, "Malformed car_soft_limit_hours should be ignored");
    TEST_ASSERT(loaded_config.truck_max_daily_hours == 0, "Malformed truck_max_daily_hours should be ignored");
    TEST_ASSERT(loaded_config.fuel_avg_consumption_x10 == 0, "Malformed fuel_avg_consumption_x10 should be ignored");

    driver_break_db_destroy(db);
    unlink(db_path);
    g_free(db_path);
    (void)result; /* result may be 0 if all entries were skipped; this is acceptable. */
    return 0;
}

static int test_db_add_fuel_stop(void) {
    char *db_path = create_temp_db();
    struct driver_break_db *db = driver_break_db_new(db_path);
    TEST_ASSERT(db != NULL, "Database creation failed");

    struct driver_break_fuel_stop stop;
    memset(&stop, 0, sizeof(stop));
    stop.timestamp = time(NULL);
    stop.coord.lat = 59.9111;
    stop.coord.lng = 10.7528;
    stop.fuel_added = 40.0;
    stop.fuel_level_after = 55.0;
    stop.cost = 800.0;
    stop.currency = g_strdup("NOK");
    stop.ethanol_pct = 5;

    int result = driver_break_db_add_fuel_stop(db, &stop);
    TEST_ASSERT(result == 1, "Fuel stop insert failed");

    /* Verify via direct SQLite query */
    sqlite3 *raw = NULL;
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_open(db_path, &raw);
    TEST_ASSERT(rc == SQLITE_OK, "Failed to open raw SQLite database");

    rc = sqlite3_prepare_v2(raw,
                            "SELECT COUNT(*), SUM(fuel_added), SUM(fuel_level_after), MAX(ethanol_pct) "
                            "FROM driver_break_fuel_stops;",
                            -1, &stmt, NULL);
    TEST_ASSERT(rc == SQLITE_OK, "Failed to prepare fuel stop count query");

    rc = sqlite3_step(stmt);
    TEST_ASSERT(rc == SQLITE_ROW, "No row returned from fuel stop table");

    int count = sqlite3_column_int(stmt, 0);
    double sum_added = sqlite3_column_double(stmt, 1);
    double sum_level = sqlite3_column_double(stmt, 2);
    int max_ethanol = sqlite3_column_int(stmt, 3);

    TEST_ASSERT(count == 1, "Fuel stop count mismatch");
    TEST_ASSERT(sum_added == 40.0, "Fuel added sum mismatch");
    TEST_ASSERT(sum_level == 55.0, "Fuel level sum mismatch");
    TEST_ASSERT(max_ethanol == 5, "Ethanol pct mismatch");

    sqlite3_finalize(stmt);
    sqlite3_close(raw);

    g_free(stop.currency);
    driver_break_db_destroy(db);
    unlink(db_path);
    g_free(db_path);
    return 0;
}

static int test_db_add_fuel_sample_and_trip_summary(void) {
    char *db_path = create_temp_db();
    struct driver_break_db *db = driver_break_db_new(db_path);
    TEST_ASSERT(db != NULL, "Database creation failed");

    time_t now = time(NULL);

    /* Add a few fuel samples */
    int result = driver_break_db_add_fuel_sample(db, now - 60, 500.0, 0.4, 8.0, 60.0, 50);
    TEST_ASSERT(result == 1, "Fuel sample 1 insert failed");
    result = driver_break_db_add_fuel_sample(db, now, 600.0, 0.5, 8.3, 65.0, -1);
    TEST_ASSERT(result == 1, "Fuel sample 2 insert failed");

    /* Add a trip summary */
    result = driver_break_db_add_trip_summary(db, now - 600, now, 20000.0, 12.0, 7.0, 10.5, 1);
    TEST_ASSERT(result == 1, "Trip summary insert failed");

    /* Verify via direct SQLite query */
    sqlite3 *raw = NULL;
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_open(db_path, &raw);
    TEST_ASSERT(rc == SQLITE_OK, "Failed to open raw SQLite database");

    /* Check fuel_samples row count */
    rc = sqlite3_prepare_v2(raw,
                            "SELECT COUNT(*), SUM(distance_m), SUM(fuel_used) "
                            "FROM driver_break_fuel_samples;",
                            -1, &stmt, NULL);
    TEST_ASSERT(rc == SQLITE_OK, "Failed to prepare fuel_samples query");

    rc = sqlite3_step(stmt);
    TEST_ASSERT(rc == SQLITE_ROW, "No row returned from fuel_samples table");

    int count = sqlite3_column_int(stmt, 0);
    double sum_distance = sqlite3_column_double(stmt, 1);
    double sum_fuel = sqlite3_column_double(stmt, 2);

    TEST_ASSERT(count == 2, "Fuel samples count mismatch");
    TEST_ASSERT(sum_distance > 1000.0 && sum_distance < 2000.0, "Fuel samples distance sum unexpected");
    TEST_ASSERT(sum_fuel > 0.8 && sum_fuel < 1.0, "Fuel samples fuel sum unexpected");

    sqlite3_finalize(stmt);

    /* Check trip summaries row count */
    rc = sqlite3_prepare_v2(raw,
                            "SELECT COUNT(*), SUM(total_distance_m), SUM(total_fuel), MAX(high_load_flag) "
                            "FROM driver_break_trip_summaries;",
                            -1, &stmt, NULL);
    TEST_ASSERT(rc == SQLITE_OK, "Failed to prepare trip summary query");

    rc = sqlite3_step(stmt);
    TEST_ASSERT(rc == SQLITE_ROW, "No row returned from trip summary table");

    count = sqlite3_column_int(stmt, 0);
    double sum_trip_distance = sqlite3_column_double(stmt, 1);
    double sum_trip_fuel = sqlite3_column_double(stmt, 2);
    int max_high_load = sqlite3_column_int(stmt, 3);

    TEST_ASSERT(count == 1, "Trip summary count mismatch");
    TEST_ASSERT(sum_trip_distance == 20000.0, "Trip summary distance mismatch");
    TEST_ASSERT(sum_trip_fuel == 12.0, "Trip summary fuel mismatch");
    TEST_ASSERT(max_high_load == 1, "Trip summary high_load_flag mismatch");

    sqlite3_finalize(stmt);
    sqlite3_close(raw);

    driver_break_db_destroy(db);
    unlink(db_path);
    g_free(db_path);
    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Driver Break plugin database tests...\n");

    failures += test_db_create_destroy();
    failures += test_db_add_history();
    failures += test_db_get_history();
    failures += test_db_clear_history();
    failures += test_db_save_load_config();
    failures += test_db_malformed_config_entries();
    failures += test_db_add_fuel_stop();
    failures += test_db_add_fuel_sample_and_trip_summary();

    if (failures == 0) {
        printf("All database tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
}
