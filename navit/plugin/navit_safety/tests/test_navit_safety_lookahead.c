/**
 * Navit, a modular navigation system.
 *
 * Unit tests for Navit Safety lookahead gap detection.
 */

#include <stdio.h>
#include "navit_safety_confidence.h"
#include "navit_safety_lookahead.h"

#define TEST_ASSERT(cond, message)                                                                                     \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static int test_no_stops_warns(void) {
    struct navit_safety_gap_result result;
    navit_safety_lookahead_plan(NULL, 0, 300, 200, &result);
    TEST_ASSERT(result.max_gap_km == 300, "Whole route is one gap with no stops");
    TEST_ASSERT(result.warning, "300 km gap exceeds 200 km range");
    TEST_ASSERT(result.shortfall_km == 100, "Shortfall is 100 km");
    TEST_ASSERT(result.gap_start_km == 0, "Gap starts at the origin");
    return 0;
}

static int test_reliable_stops_close_gap(void) {
    struct navit_safety_stop stops[2];
    struct navit_safety_gap_result result;
    stops[0].distance_km = 150;
    stops[0].confidence = NAVIT_SAFETY_CONFIDENCE_HIGH;
    stops[1].distance_km = 280;
    stops[1].confidence = NAVIT_SAFETY_CONFIDENCE_MEDIUM;
    navit_safety_lookahead_plan(stops, 2, 300, 200, &result);
    TEST_ASSERT(result.max_gap_km == 150, "Largest leg is start->150");
    TEST_ASSERT(!result.warning, "150 km gap is within 200 km range");
    TEST_ASSERT(result.shortfall_km == 0, "No shortfall");
    return 0;
}

static int test_low_confidence_ignored(void) {
    struct navit_safety_stop stops[1];
    struct navit_safety_gap_result result;
    stops[0].distance_km = 150;
    stops[0].confidence = NAVIT_SAFETY_CONFIDENCE_LOW;
    navit_safety_lookahead_plan(stops, 1, 300, 200, &result);
    TEST_ASSERT(result.max_gap_km == 300, "Low-confidence stop is ignored, so 300 km gap");
    TEST_ASSERT(result.warning, "Gap exceeds range because the stop does not count");
    return 0;
}

static int test_final_leg_gap(void) {
    struct navit_safety_stop stops[1];
    struct navit_safety_gap_result result;
    stops[0].distance_km = 50;
    stops[0].confidence = NAVIT_SAFETY_CONFIDENCE_HIGH;
    navit_safety_lookahead_plan(stops, 1, 300, 200, &result);
    TEST_ASSERT(result.max_gap_km == 250, "Final leg 50->300 is the worst gap");
    TEST_ASSERT(result.gap_start_km == 50, "Worst gap starts at the last reliable stop");
    TEST_ASSERT(result.warning, "250 km exceeds 200 km range");
    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Navit Safety lookahead tests...\n");
    failures += test_no_stops_warns();
    failures += test_reliable_stops_close_gap();
    failures += test_low_confidence_ignored();
    failures += test_final_leg_gap();

    if (failures == 0) {
        printf("All Navit Safety lookahead tests passed!\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
