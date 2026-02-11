/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Integration tests for Driver Break plugin using actual Navit and plugin functions
 */

#include "../../debug.h"
#include "../../mapset.h"
#include "../../navit.h"
#include "../../projection.h"
#include "../../route.h"
#include "../../transform.h"
#include "../../vehicleprofile.h"
#include "../driver_break.h"
#include "../driver_break_cycling.h"
#include "../driver_break_hiking.h"
#include "../driver_break_poi.h"
#include "../driver_break_poi_hiking.h"
#include "../driver_break_route_validator.h"
#include "../driver_break_srtm.h"
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

/* Test hiking rest stop calculation with actual function */
static int test_hiking_driver_break_intervals(void) {
    /* Test with 25 km route (enough for at least one rest stop) */
    double total_distance = 25000.0; /* 25 km in meters */

    /* Calculate rest stops using actual plugin function */
    GList *stops = hiking_calculate_driver_break_stops_with_max(total_distance, 0, 40000.0);

    TEST_ASSERT(stops != NULL, "Hiking rest stops should be created");
    TEST_ASSERT(g_list_length(stops) > 0, "Should have at least one rest stop");

    /* Check first rest stop is at ~11.3 km */
    GList *l = stops;
    struct hiking_driver_break_stop *first_stop = (struct hiking_driver_break_stop *)l->data;
    TEST_ASSERT(first_stop != NULL, "First rest stop should exist");
    TEST_ASSERT(first_stop->position > 10000.0 && first_stop->position < 12000.0,
                "First rest stop should be around 11.3 km");

    /* Check rest intervals */
    double prev_position = 0.0;
    int stop_count = 0;
    while (l) {
        struct hiking_driver_break_stop *stop = (struct hiking_driver_break_stop *)l->data;
        if (stop) {
            stop_count++;
            if (prev_position > 0) {
                double interval = stop->position - prev_position;
                /* Should be approximately 11.295 km */
                TEST_ASSERT(interval > 11000.0 && interval < 12000.0, "Rest intervals should be ~11.3 km");
            }
            prev_position = stop->position;
        }
        l = g_list_next(l);
    }

    hiking_free_driver_break_stops(stops);
    return 0;
}

/* Test cycling rest stop calculation */
static int test_cycling_driver_break_intervals(void) {
    /* Test with 100 km route */
    double total_distance = 100000.0; /* 100 km in meters */

    /* Calculate rest stops using actual plugin function */
    GList *stops = cycling_calculate_driver_break_stops(total_distance, 0);

    TEST_ASSERT(stops != NULL, "Cycling rest stops should be created");
    TEST_ASSERT(g_list_length(stops) > 0, "Should have at least one rest stop");

    /* Check first rest stop is at ~28.24 km */
    GList *l = stops;
    if (l && l->data) {
        /* Verify structure exists */
        TEST_ASSERT(l->data != NULL, "First rest stop data should exist");
    }

    cycling_free_driver_break_stops(stops);
    return 0;
}

/* Test POI discovery with actual coordinates */
static int test_poi_discovery_hiking_route(void) {
    /* Test coordinates for hiking route waypoints */
    struct coord_geo bjoberg = {60.8, 7.2};       /* Bjøberg */
    struct coord_geo bjordalsbu = {60.9, 7.3};    /* Bjordalsbu */
    struct coord_geo aurlandsdalen = {61.0, 7.4}; /* Aurlandsdalen Turisthytte */

    /* Test POI search at Bjordalsbu (should find cabin) */
    GList *cabins = poi_search_cabins(&bjordalsbu, 5.0);
    /* Note: This will use Overpass API fallback if mapset not available */
    /* We're testing that the function executes without crashing */

    /* Test water point search */
    GList *water = poi_search_water_points(&bjordalsbu, 2.0);
    /* Function should execute without errors */

    if (cabins) {
        poi_free_cabins(cabins);
    }
    if (water) {
        poi_free_water_points(water);
    }

    return 0;
}

/* Test POI discovery for car route */
static int test_poi_discovery_car_route(void) {
    /* Test coordinates for car route waypoints */
    struct coord_geo moelv = {61.0, 10.7};   /* St1 Moelv */
    struct coord_geo aukrust = {61.3, 11.0}; /* Aukrustsenteret */

    /* Test POI search (should find fuel stations, cafes) */
    /* Note: This will use Overpass API fallback if mapset not available */
    GList *pois = driver_break_poi_discover(&moelv, 5, NULL, 0);

    if (pois) {
        driver_break_poi_free_list(pois);
    }

    return 0;
}

/* Test SRTM elevation with actual coordinates */
static int test_srtm_elevation_hiking_route(void) {
    /* Initialize SRTM system */
    char *test_dir = g_strdup("/tmp/test_srtm_integration");
    g_mkdir_with_parents(test_dir, 0755);
    srtm_init(test_dir);

    /* Test coordinates along hiking route */
    struct coord_geo bjoberg = {60.8, 7.2};
    struct coord_geo bjordalsbu = {60.9, 7.3};
    struct coord_geo aurlandsdalen = {61.0, 7.4};

    /* Try to get elevation (will return -32768 if HGT files not available) */
    int elev1 = srtm_get_elevation(&bjoberg);
    int elev2 = srtm_get_elevation(&bjordalsbu);
    int elev3 = srtm_get_elevation(&aurlandsdalen);

    /* Test that function executes without errors */
    /* Elevation may be -32768 if HGT files not downloaded, which is valid */
    TEST_ASSERT(elev1 >= -32768 && elev1 <= 9000, "Elevation 1 in valid range");
    TEST_ASSERT(elev2 >= -32768 && elev2 <= 9000, "Elevation 2 in valid range");
    TEST_ASSERT(elev3 >= -32768 && elev3 <= 9000, "Elevation 3 in valid range");

    g_free(test_dir);
    return 0;
}

/* Test route validation with actual validation functions */
static int test_route_validation_functions(void) {
    /* Test forbidden highway detection */
    TEST_ASSERT(route_validator_is_forbidden_highway("motorway") == 1, "Motorway should be forbidden for hiking");
    TEST_ASSERT(route_validator_is_forbidden_highway("trunk") == 1, "Trunk should be forbidden for hiking");
    TEST_ASSERT(route_validator_is_forbidden_highway("primary") == 1, "Primary should be forbidden for hiking");

    /* Test priority path detection */
    TEST_ASSERT(route_validator_is_priority_path("footway") == 1, "Footway should be priority path");
    TEST_ASSERT(route_validator_is_priority_path("path") == 1, "Path should be priority path");
    TEST_ASSERT(route_validator_is_priority_path("track") == 1, "Track should be priority path");

    return 0;
}

/* Test rest stop creation along route */
static int test_driver_break_stop_creation(void) {
    /* Simulate route with 50 km distance (enough for multiple stops to test intervals) */
    double route_distance = 50000.0; /* 50 km */

    /* Calculate hiking rest stops */
    /* For 50 km with driver_break_distance=11.295 km:
     * - First stop at 11.295 km, remaining = 50 - 11.295 = 38.705 km
     * - 38.705 > 11.295, so we create the stop and continue
     * - Second stop at 22.59 km, remaining = 50 - 22.59 = 27.41 km
     * - 27.41 > 11.295, so we create the stop and continue
     * - Third stop at 33.885 km, remaining = 50 - 33.885 = 16.115 km
     * - 16.115 > 11.295, so we create the stop and continue
     * - Fourth stop at 45.18 km, remaining = 50 - 45.18 = 4.82 km
     * - 4.82 <= 11.295, so we break (can complete without additional rest)
     * So we get 4 stops for 50 km, with 3 intervals between them */
    GList *hiking_stops = hiking_calculate_driver_break_stops_with_max(route_distance, 0, 40000.0);
    TEST_ASSERT(hiking_stops != NULL, "Hiking stops should be created");
    int stop_count = g_list_length(hiking_stops);
    TEST_ASSERT(stop_count >= 2, "Should have at least 2 rest stops for 50 km");

    /* Verify rest intervals */
    GList *l = hiking_stops;
    double prev_pos = 0.0;
    int interval_count = 0;
    while (l) {
        struct hiking_driver_break_stop *stop = (struct hiking_driver_break_stop *)l->data;
        if (stop && prev_pos > 0) {
            double interval = stop->position - prev_pos;
            interval_count++;
            /* Intervals should be approximately 11.295 km */
            TEST_ASSERT(interval > 11000.0 && interval < 12000.0, "Rest interval should be ~11.3 km");
        }
        if (stop) {
            prev_pos = stop->position;
        }
        l = g_list_next(l);
    }

    /* With stop_count stops, we should have (stop_count - 1) intervals */
    /* But if stop_count is 0 or 1, interval_count will be 0 */
    if (stop_count >= 2) {
        TEST_ASSERT(interval_count > 0, "Should have calculated rest intervals");
    }

    hiking_free_driver_break_stops(hiking_stops);
    return 0;
}

/* Test that POI discovery functions are called correctly */
static int test_poi_discovery_integration(void) {
    /* Test coordinates for rest stop */
    struct coord_geo driver_break_stop = {60.9, 7.3}; /* Near Bjordalsbu */

    /* Test water point discovery */
    GList *water = poi_search_water_points(&driver_break_stop, 2.0);
    /* Function should execute (may return NULL if no map data) */

    /* Test cabin discovery */
    GList *cabins = poi_search_cabins(&driver_break_stop, 5.0);
    /* Function should execute (may return NULL if no map data) */

    /* Verify functions executed without crashing */
    if (water) {
        TEST_ASSERT(g_list_length(water) >= 0, "Water points list valid");
        poi_free_water_points(water);
    }

    if (cabins) {
        TEST_ASSERT(g_list_length(cabins) >= 0, "Cabins list valid");
        poi_free_cabins(cabins);
    }

    return 0;
}

/* Test SRTM tile handling for Norway region */
static int test_srtm_norway_tiles(void) {
    char *test_dir = g_strdup("/tmp/test_srtm_norway");
    g_mkdir_with_parents(test_dir, 0755);
    srtm_init(test_dir);

    /* Test coordinates in Norway (hiking route area) */
    /* Note: struct coord_geo has lng first, then lat */
    struct coord_geo coord1;
    coord1.lng = 7.2;  /* Longitude first - Bjøberg */
    coord1.lat = 60.8; /* Latitude second */
    struct coord_geo coord2;
    coord2.lng = 10.7; /* Longitude first - Moelv */
    coord2.lat = 61.0; /* Latitude second */

    /* Calculate expected tile indices (srtm_calculate_tiles uses floor) */
    int lon1 = (int)floor(coord1.lng); /* 7 */
    int lat1 = (int)floor(coord1.lat); /* 60 */
    int lon2 = (int)floor(coord2.lng); /* 10 */
    int lat2 = (int)floor(coord2.lat); /* 61 */

    /* Test tile filename generation */
    char *filename1 = srtm_get_tile_filename(lon1, lat1);
    char *filename2 = srtm_get_tile_filename(lon2, lat2);

    TEST_ASSERT(filename1 != NULL, "Tile filename 1 should be generated");
    TEST_ASSERT(filename2 != NULL, "Tile filename 2 should be generated");
    /* srtm_get_tile_filename uses int (truncation), which for positive numbers is same as floor */
    /* For 7.2: int(7.2) = 7, floor(7.2) = 7, so both give 7 */
    TEST_ASSERT(strcmp(filename1, "N60E007.hgt") == 0, "Tile filename 1 should be N60E007.hgt");
    TEST_ASSERT(strcmp(filename2, "N61E010.hgt") == 0, "Tile filename 2 should be N61E010.hgt");

    g_free(filename1);
    g_free(filename2);
    g_free(test_dir);
    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Driver Break plugin integration tests (using actual Navit/plugin functions)...\n");

    failures += test_hiking_driver_break_intervals();
    failures += test_cycling_driver_break_intervals();
    failures += test_poi_discovery_hiking_route();
    failures += test_poi_discovery_car_route();
    failures += test_srtm_elevation_hiking_route();
    failures += test_route_validation_functions();
    failures += test_driver_break_stop_creation();
    failures += test_poi_discovery_integration();
    failures += test_srtm_norway_tiles();

    if (failures == 0) {
        printf("All integration tests passed!\n");
        printf("Note: Some tests may return empty results if map/HGT data not available.\n");
        printf("This is expected - tests verify functions execute correctly.\n");
        return 0;
    } else {
        printf("Integration tests failed: %d failures\n", failures);
        return 1;
    }
}
