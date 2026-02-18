/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Unit tests for APRS database operations
 */

#include "../../debug.h"
#include "../aprs_db.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

static int test_db_create_destroy(void) {
    struct aprs_db *db = aprs_db_new(NULL); /* In-memory database */
    TEST_ASSERT(db != NULL, "Database creation failed");
    aprs_db_destroy(db);
    return 0;
}

static int test_db_insert_station(void) {
    struct aprs_db *db = aprs_db_new(NULL);
    struct aprs_station *station = aprs_station_new();

    station->callsign = g_strdup("TEST1");
    station->position.lat = 37.7749;
    station->position.lng = -122.4194;
    station->altitude = 100.0;
    station->course = 45;
    station->speed = 30;
    station->timestamp = time(NULL);
    station->symbol_table = '/';
    station->symbol_code = '>';

    int result = aprs_db_update_station(db, station);
    TEST_ASSERT(result == 1, "Station insert failed");

    aprs_station_free(station);
    aprs_db_destroy(db);
    return 0;
}

static int test_db_get_station(void) {
    struct aprs_db *db = aprs_db_new(NULL);
    struct aprs_station *station = aprs_station_new();

    station->callsign = g_strdup("TEST2");
    station->position.lat = 40.7128;
    station->position.lng = -74.0060;
    station->timestamp = time(NULL);

    aprs_db_update_station(db, station);

    struct aprs_station *retrieved = aprs_station_new();
    int result = aprs_db_get_station(db, "TEST2", retrieved);
    TEST_ASSERT(result == 1, "Station retrieval failed");
    TEST_ASSERT(strcmp(retrieved->callsign, "TEST2") == 0, "Callsign mismatch");
    TEST_ASSERT(retrieved->position.lat == 40.7128, "Latitude mismatch");
    TEST_ASSERT(retrieved->position.lng == -74.0060, "Longitude mismatch");

    aprs_station_free(station);
    aprs_station_free(retrieved);
    aprs_db_destroy(db);
    return 0;
}

static int test_db_update_station(void) {
    struct aprs_db *db = aprs_db_new(NULL);
    struct aprs_station *station = aprs_station_new();

    station->callsign = g_strdup("TEST3");
    station->position.lat = 51.5074;
    station->position.lng = -0.1278;
    station->timestamp = time(NULL);

    aprs_db_update_station(db, station);

    /* Update position */
    station->position.lat = 52.5200;
    station->position.lng = 13.4050;
    aprs_db_update_station(db, station);

    struct aprs_station *retrieved = aprs_station_new();
    aprs_db_get_station(db, "TEST3", retrieved);
    TEST_ASSERT(retrieved->position.lat == 52.5200, "Update failed - latitude");
    TEST_ASSERT(retrieved->position.lng == 13.4050, "Update failed - longitude");

    aprs_station_free(station);
    aprs_station_free(retrieved);
    aprs_db_destroy(db);
    return 0;
}

static int test_db_delete_expired(void) {
    struct aprs_db *db = aprs_db_new(NULL);
    struct aprs_station *station = aprs_station_new();

    station->callsign = g_strdup("OLD");
    station->position.lat = 0.0;
    station->position.lng = 0.0;
    station->timestamp = time(NULL) - 10000; /* 10 seconds ago */

    aprs_db_update_station(db, station);

    int deleted = aprs_db_delete_expired(db, 5); /* Delete older than 5 seconds */
    TEST_ASSERT(deleted == 1, "Expired station not deleted");

    struct aprs_station *retrieved = aprs_station_new();
    int result = aprs_db_get_station(db, "OLD", retrieved);
    TEST_ASSERT(result == 0, "Expired station still exists");

    aprs_station_free(station);
    aprs_station_free(retrieved);
    aprs_db_destroy(db);
    return 0;
}

static int test_db_range_filtering(void) {
    struct aprs_db *db = aprs_db_new(NULL);
    struct coord_geo center = {.lng = -122.4194, .lat = 37.7749}; /* San Francisco */
    GList *stations = NULL;

    /* Add station in range (Oakland, ~10 km away) */
    struct aprs_station *station1 = aprs_station_new();
    station1->callsign = g_strdup("NEAR");
    station1->position.lat = 37.8044;
    station1->position.lng = -122.2711;
    station1->timestamp = time(NULL);
    station1->symbol_table = '/';
    station1->symbol_code = '>';
    int result1 = aprs_db_update_station(db, station1);
    TEST_ASSERT(result1 == 1, "Failed to insert NEAR station");

    /* Add station out of range (New York, ~4000 km away) */
    struct aprs_station *station2 = aprs_station_new();
    station2->callsign = g_strdup("FAR");
    station2->position.lat = 40.7128;
    station2->position.lng = -74.0060;
    station2->timestamp = time(NULL);
    station2->symbol_table = '/';
    station2->symbol_code = '>';
    int result2 = aprs_db_update_station(db, station2);
    TEST_ASSERT(result2 == 1, "Failed to insert FAR station");

    int result = aprs_db_get_stations_in_range(db, &center, 50.0, &stations);

    TEST_ASSERT(result == 1, "Range query failed");
    TEST_ASSERT(g_list_length(stations) == 1, "Wrong number of stations in range");
    TEST_ASSERT(stations != NULL, "Stations list is NULL");

    struct aprs_station *found = (struct aprs_station *)stations->data;
    TEST_ASSERT(found != NULL, "Station data is NULL");
    TEST_ASSERT(strcmp(found->callsign, "NEAR") == 0, "Wrong station in range");

    g_list_free_full(stations, (GDestroyNotify)aprs_station_free);
    aprs_station_free(station1);
    aprs_station_free(station2);
    aprs_db_destroy(db);
    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running APRS database tests...\n");

    failures += test_db_create_destroy();
    failures += test_db_insert_station();
    failures += test_db_get_station();
    failures += test_db_update_station();
    failures += test_db_delete_expired();
    failures += test_db_range_filtering();

    if (failures == 0) {
        printf("All database tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
}
