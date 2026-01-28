/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Unit tests for Driver Break plugin rest stop finder
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include "../../debug.h"
#include "../driver_break_finder.h"
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

static double coord_distance(struct coord_geo *c1, struct coord_geo *c2) {
    double lat1 = c1->lat * M_PI / 180.0;
    double lat2 = c2->lat * M_PI / 180.0;
    double dlat = (c2->lat - c1->lat) * M_PI / 180.0;
    double dlng = (c2->lng - c1->lng) * M_PI / 180.0;
    
    double a = sin(dlat/2) * sin(dlat/2) +
               cos(lat1) * cos(lat2) *
               sin(dlng/2) * sin(dlng/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return 6371000.0 * c; /* Earth radius in meters */
}

static int test_coord_validation(void) {
    struct driver_break_config config;
    memset(&config, 0, sizeof(config));
    config.min_distance_from_buildings = 150;
    
    /* Valid coordinates */
    /* Note: struct coord_geo has lng first, then lat */
    struct coord_geo valid_coord;
    valid_coord.lng = -122.4194;  /* Longitude first */
    valid_coord.lat = 37.7749;    /* Latitude second */
    int result = driver_break_finder_is_valid_location(&valid_coord, &config);
    TEST_ASSERT(result == 1, "Valid coordinates should pass validation");
    
    /* Invalid latitude */
    struct coord_geo invalid_lat;
    invalid_lat.lng = -122.4194;
    invalid_lat.lat = 91.0;
    result = driver_break_finder_is_valid_location(&invalid_lat, &config);
    TEST_ASSERT(result == 0, "Invalid latitude should fail validation");
    
    /* Invalid longitude */
    struct coord_geo invalid_lng;
    invalid_lng.lng = -181.0;
    invalid_lng.lat = 37.7749;
    result = driver_break_finder_is_valid_location(&invalid_lng, &config);
    TEST_ASSERT(result == 0, "Invalid longitude should fail validation");
    
    /* NULL coordinate */
    result = driver_break_finder_is_valid_location(NULL, &config);
    TEST_ASSERT(result == 0, "NULL coordinate should fail validation");
    
    return 0;
}

static int test_distance_calculation(void) {
    /* San Francisco to Oakland - approximately 12.5 km */
    /* Note: struct coord_geo has lng first, then lat */
    struct coord_geo sf;
    sf.lng = -122.4194;  /* Longitude first */
    sf.lat = 37.7749;    /* Latitude second */
    struct coord_geo oakland;
    oakland.lng = -122.2711;  /* Longitude first */
    oakland.lat = 37.8044;    /* Latitude second */
    
    double distance = coord_distance(&sf, &oakland);
    double expected_distance = 12500.0; /* Approximately 12.5 km */
    double tolerance = 2000.0; /* 2 km tolerance */
    
    TEST_ASSERT(distance > 0, "Distance should be positive");
    TEST_ASSERT(fabs(distance - expected_distance) < tolerance, 
                "Distance calculation should be approximately correct");
    
    /* Same coordinates should have zero distance */
    double zero_distance = coord_distance(&sf, &sf);
    TEST_ASSERT(zero_distance < 1.0, "Same coordinates should have near-zero distance");
    
    return 0;
}

static int test_driver_break_stop_free(void) {
    struct driver_break_stop *stop = g_new0(struct driver_break_stop, 1);
    stop->name = g_strdup("Test Stop");
    stop->highway_type = g_strdup("unclassified");
    stop->coord.lng = -122.4194;  /* Longitude first */
    stop->coord.lat = 37.7749;    /* Latitude second */
    stop->distance_from_route = 100.0;
    stop->score = 85;
    
    driver_break_finder_free(stop);
    /* If we get here without crash, free worked */
    
    return 0;
}

static int test_driver_break_stop_list_free(void) {
    GList *stops = NULL;
    
    struct driver_break_stop *stop1 = g_new0(struct driver_break_stop, 1);
    stop1->name = g_strdup("Stop 1");
    stop1->coord.lng = -122.4194;  /* Longitude first */
    stop1->coord.lat = 37.7749;    /* Latitude second */
    
    struct driver_break_stop *stop2 = g_new0(struct driver_break_stop, 1);
    stop2->name = g_strdup("Stop 2");
    stop2->coord.lng = -74.0060;   /* Longitude first */
    stop2->coord.lat = 40.7128;    /* Latitude second */
    
    stops = g_list_append(stops, stop1);
    stops = g_list_append(stops, stop2);
    
    TEST_ASSERT(g_list_length(stops) == 2, "List should have 2 stops");
    
    driver_break_finder_free_list(stops);
    /* If we get here without crash, free list worked */
    
    return 0;
}

static int test_driver_break_finder_near_null_params(void) {
    struct driver_break_config config;
    memset(&config, 0, sizeof(config));
    
    /* NULL center should return NULL */
    GList *result = driver_break_finder_find_near(NULL, 10.0, &config);
    TEST_ASSERT(result == NULL, "NULL center should return NULL");
    
    /* NULL config should return NULL */
    /* Note: struct coord_geo has lng first, then lat */
    struct coord_geo center;
    center.lng = -122.4194;  /* Longitude first */
    center.lat = 37.7749;    /* Latitude second */
    result = driver_break_finder_find_near(&center, 10.0, NULL);
    TEST_ASSERT(result == NULL, "NULL config should return NULL");
    
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("Running Driver Break plugin finder tests...\n");
    
    failures += test_coord_validation();
    failures += test_distance_calculation();
    failures += test_driver_break_stop_free();
    failures += test_driver_break_stop_list_free();
    failures += test_driver_break_finder_near_null_params();
    
    if (failures == 0) {
        printf("All finder tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
}
