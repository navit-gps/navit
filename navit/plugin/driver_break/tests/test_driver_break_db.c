/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Unit tests for Driver Break plugin database operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <glib.h>
#include "../../debug.h"
#include "../driver_break_db.h"
#include "../driver_break.h"

/* Stub for debug_printf and max_debug_level for unit tests */
dbg_level max_debug_level = lvl_error;

void debug_printf(dbg_level level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, ...) {
    (void)level;
    (void)module;
    (void)mlen;
    (void)function;
    (void)flen;
    (void)prefix;
    (void)fmt;
    /* No-op for tests */
}

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message); \
            return 1; \
        } \
    } while(0)

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
    config.vehicle_type = 1; /* Truck */
    config.car_soft_limit_hours = 7;
    config.car_max_hours = 10;
    config.truck_mandatory_break_after_hours = 4;
    config.truck_mandatory_break_duration_min = 45;
    config.min_distance_from_buildings = 150;
    config.poi_search_radius_km = 15;

    int result = driver_break_db_save_config(db, &config);
    TEST_ASSERT(result == 1, "Config save failed");

    struct driver_break_config loaded_config;
    memset(&loaded_config, 0, sizeof(loaded_config));
    result = driver_break_db_load_config(db, &loaded_config);
    TEST_ASSERT(result == 1, "Config load failed");
    TEST_ASSERT(loaded_config.vehicle_type == 1, "Vehicle type mismatch");
    TEST_ASSERT(loaded_config.car_soft_limit_hours == 7, "Car soft limit mismatch");
    TEST_ASSERT(loaded_config.truck_mandatory_break_after_hours == 4, "Truck break hours mismatch");
    TEST_ASSERT(loaded_config.min_distance_from_buildings == 150, "Min distance mismatch");

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

    if (failures == 0) {
        printf("All database tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
}
