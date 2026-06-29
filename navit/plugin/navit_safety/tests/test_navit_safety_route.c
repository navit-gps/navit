/**
 * Navit, a modular navigation system.
 *
 * Unit tests for Navit Safety route-state helpers (no Navit core dependency).
 */

#include <stdio.h>
#include <string.h>
#include "navit_safety.h"
#include "navit_safety_confidence.h"
#include "navit_safety_lookahead.h"
#include "navit_safety_route.h"

#define TEST_ASSERT(cond, message)                                                                                     \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static int test_build_plan_stops_skips_denied(void) {
    const struct navit_safety_poi_entry pois[] = {
        {"a", "A", 0.0, 0.0, 10, NAVIT_SAFETY_CONFIDENCE_HIGH, 0},
        {"b", "B", 0.0, 0.0, 50, NAVIT_SAFETY_CONFIDENCE_MEDIUM, 1},
        {"c", "C", 0.0, 0.0, 90, NAVIT_SAFETY_CONFIDENCE_LOW, 0},
    };
    struct navit_safety_stop stops[4];
    int n_stops = 0;

    navit_safety_build_plan_stops(pois, 3, stops, &n_stops);
    TEST_ASSERT(n_stops == 2, "Denied and low-confidence stops are passed through to the planner");
    TEST_ASSERT(stops[0].distance_km == 10, "First reliable stop kept");
    TEST_ASSERT(stops[1].distance_km == 90, "Low-confidence stop still listed for the planner to filter");
    return 0;
}

static int test_format_status_no_route(void) {
    struct navit_safety_config config;
    struct navit_safety_route_state state;
    char text[128];

    navit_safety_config_default(&config);
    navit_safety_route_state_init(&state);
    navit_safety_format_status(&state, &config, text, sizeof(text));
    TEST_ASSERT(strstr(text, "No active route") != NULL, "Idle status mentions missing route");
    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Navit Safety route helper tests...\n");
    failures += test_build_plan_stops_skips_denied();
    failures += test_format_status_no_route();

    if (failures == 0) {
        printf("All Navit Safety route helper tests passed!\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
