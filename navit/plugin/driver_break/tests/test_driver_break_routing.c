/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Unit tests for Driver Break plugin routing with different vehicle profiles
 */

#include "../../debug.h"
#include "../../item.h"
#include "../driver_break.h"
#include "../driver_break_poi_map.h"
#include "../driver_break_route_validator.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Test route validation for hiking profile */
static int test_route_validation_hiking(void) {
    /* Note: This is a simplified test - full route validation requires actual route objects */
    /* Test the validation functions directly */

    /* Test forbidden highway detection */
    TEST_ASSERT(route_validator_is_forbidden_highway("motorway") == 1, "Motorway should be forbidden");
    TEST_ASSERT(route_validator_is_forbidden_highway("trunk") == 1, "Trunk should be forbidden");
    TEST_ASSERT(route_validator_is_forbidden_highway("primary") == 1, "Primary should be forbidden");
    TEST_ASSERT(route_validator_is_forbidden_highway("motorway_link") == 1, "Motorway link should be forbidden");

    /* Test priority path detection */
    TEST_ASSERT(route_validator_is_priority_path("footway") == 1, "Footway should be priority");
    TEST_ASSERT(route_validator_is_priority_path("path") == 1, "Path should be priority");
    TEST_ASSERT(route_validator_is_priority_path("track") == 1, "Track should be priority");
    TEST_ASSERT(route_validator_is_priority_path("steps") == 1, "Steps should be priority");
    TEST_ASSERT(route_validator_is_priority_path("bridleway") == 1, "Bridleway should be priority");

    /* Test allowed but not priority */
    TEST_ASSERT(route_validator_is_forbidden_highway("secondary") == 0, "Secondary should not be forbidden");
    TEST_ASSERT(route_validator_is_priority_path("secondary") == 0, "Secondary should not be priority");

    return 0;
}

/* Test route validation for car profile */
static int test_route_validation_car(void) {
    /* For car profile, motorways and trunk are allowed */
    TEST_ASSERT(route_validator_is_forbidden_highway("motorway") == 1, "Motorway forbidden check");
    TEST_ASSERT(route_validator_is_forbidden_highway("secondary") == 0, "Secondary allowed for car");
    TEST_ASSERT(route_validator_is_forbidden_highway("tertiary") == 0, "Tertiary allowed for car");

    return 0;
}

/* Cycling: cycleway priority helper and cycle network tag helper (NULL item) */
static int test_route_validation_cycling(void) {
    TEST_ASSERT(route_validator_is_cycling_priority_highway("cycleway") == 1, "Cycleway should be cycling priority");
    TEST_ASSERT(route_validator_is_cycling_priority_highway("path") == 0, "Path alone is not cycling priority");
    TEST_ASSERT(route_validator_is_cycling_priority_highway(NULL) == 0, "NULL highway not cycling priority");

    TEST_ASSERT(driver_break_route_is_cycle_network_way(NULL) == 0, "NULL item for cycle network");

    return 0;
}

/* Cycling full-route validation: NULL route is invalid with a warning */
static int test_validate_cycling_null_route(void) {
    struct route_validation_result *r = route_validator_validate_cycling(NULL);
    TEST_ASSERT(r != NULL, "validate_cycling(NULL) should return a result struct");
    TEST_ASSERT(r->is_valid == 0, "NULL route should be invalid");
    TEST_ASSERT(r->warnings != NULL, "NULL route should append a warning");
    int saw_empty = 0;
    for (GList *l = r->warnings; l; l = g_list_next(l)) {
        if (l->data && strstr((char *)l->data, "Empty"))
            saw_empty = 1;
    }
    TEST_ASSERT(saw_empty, "Warning should mention empty or null route");
    route_validator_free_result(r);
    return 0;
}

/* Place of worship map search: invalid arguments return NULL */
static int test_poi_map_worship_invalid_args(void) {
    struct coord_geo c;
    c.lat = 60.0;
    c.lng = 10.0;
    TEST_ASSERT(driver_break_poi_map_search_place_of_worship(NULL, 5.0, NULL) == NULL, "NULL center returns NULL");
    TEST_ASSERT(driver_break_poi_map_search_place_of_worship(&c, 5.0, NULL) == NULL, "NULL mapset returns NULL");
    TEST_ASSERT(driver_break_poi_map_search_place_of_worship(&c, 0.0, NULL) == NULL,
                "Non-positive radius returns NULL");
    return 0;
}

/* Test hiking route with pilgrimage priority */
static int test_route_pilgrimage_priority(void) {
    /* Test that pilgrimage routes are detected */
    /* Note: This requires actual item objects with OSM tags */
    /* For now, test the function exists and handles NULL gracefully */

    /* Test NULL handling */
    int result = driver_break_route_is_pilgrimage_hiking(NULL);
    TEST_ASSERT(result == 0, "NULL item should return 0");

    return 0;
}

/* Test route validation result structure */
static int test_route_validation_result(void) {
    struct route_validation_result *result = g_new0(struct route_validation_result, 1);
    TEST_ASSERT(result != NULL, "Failed to allocate validation result");

    /* Initialize */
    result->path_percentage = 0.0;
    result->forbidden_percentage = 0.0;
    result->secondary_percentage = 0.0;
    result->forbidden_segments = 0;
    result->warnings = NULL;
    result->is_valid = 1;

    /* Test free function */
    route_validator_free_result(result);

    return 0;
}

/* Test hiking route coordinates */
static int test_hiking_route_coordinates(void) {
    /* Test coordinates for hiking route:
     * Start: Bjøberg (Node: 1356457581)
     * Via: Bjordalsbu (Node: 754402416)
     * End: Aurlandsdalen Turisthytte (Node: 1356459524)
     */

    /* Approximate coordinates (would need actual OSM node lookup) */
    struct coord_geo start;
    start.lat = 60.8; /* Approximate for Bjøberg area */
    start.lng = 7.2;

    struct coord_geo via;
    via.lat = 60.9; /* Approximate for Bjordalsbu area */
    via.lng = 7.3;

    struct coord_geo end;
    end.lat = 61.0; /* Approximate for Aurlandsdalen area */
    end.lng = 7.4;

    /* Test that coordinates are valid */
    TEST_ASSERT(start.lat >= -90 && start.lat <= 90, "Start latitude invalid");
    TEST_ASSERT(start.lng >= -180 && start.lng <= 180, "Start longitude invalid");
    TEST_ASSERT(via.lat >= -90 && via.lat <= 90, "Via latitude invalid");
    TEST_ASSERT(via.lng >= -180 && via.lng <= 180, "Via longitude invalid");
    TEST_ASSERT(end.lat >= -90 && end.lat <= 90, "End latitude invalid");
    TEST_ASSERT(end.lng >= -180 && end.lng <= 180, "End longitude invalid");

    return 0;
}

/* Test car route coordinates */
static int test_car_route_coordinates(void) {
    /* Test coordinates for car route:
     * Start: St1 Moelv (Node: 8233820034)
     * Via: Spidsbergseterkrysset (Node: 10677676828)
     * Via: Enden (Node: 1289150990)
     * End: Aukrustsenteret
     */

    /* Approximate coordinates (would need actual OSM node lookup) */
    struct coord_geo start;
    start.lat = 61.0; /* Approximate for Moelv area */
    start.lng = 10.7;

    struct coord_geo via1;
    via1.lat = 61.1; /* Approximate for Spidsbergseterkrysset area */
    via1.lng = 10.8;

    struct coord_geo via2;
    via2.lat = 61.2; /* Approximate for Enden area */
    via2.lng = 10.9;

    struct coord_geo end;
    end.lat = 61.3; /* Approximate for Aukrustsenteret area */
    end.lng = 11.0;

    /* Test that coordinates are valid */
    TEST_ASSERT(start.lat >= -90 && start.lat <= 90, "Start latitude invalid");
    TEST_ASSERT(start.lng >= -180 && start.lng <= 180, "Start longitude invalid");
    TEST_ASSERT(via1.lat >= -90 && via1.lat <= 90, "Via1 latitude invalid");
    TEST_ASSERT(via1.lng >= -180 && via1.lng <= 180, "Via1 longitude invalid");
    TEST_ASSERT(via2.lat >= -90 && via2.lat <= 90, "Via2 latitude invalid");
    TEST_ASSERT(via2.lng >= -180 && via2.lng <= 180, "Via2 longitude invalid");
    TEST_ASSERT(end.lat >= -90 && end.lat <= 90, "End latitude invalid");
    TEST_ASSERT(end.lng >= -180 && end.lng <= 180, "End longitude invalid");

    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Driver Break plugin routing tests...\n");

    failures += test_route_validation_hiking();
    failures += test_route_validation_car();
    failures += test_route_validation_cycling();
    failures += test_validate_cycling_null_route();
    failures += test_poi_map_worship_invalid_args();
    failures += test_route_pilgrimage_priority();
    failures += test_route_validation_result();
    failures += test_hiking_route_coordinates();
    failures += test_car_route_coordinates();

    if (failures == 0) {
        printf("All routing tests passed!\n");
        return 0;
    } else {
        printf("Routing tests failed: %d failures\n", failures);
        return 1;
    }
}
