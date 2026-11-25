/**
 * Navit, a modular navigation system.
 *
 * Unit tests for Navit Safety POI confidence scoring.
 */

#include "navit_safety_confidence.h"
#include <stdio.h>

#define TEST_ASSERT(cond, message)                                                                                     \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static enum navit_safety_confidence score(enum navit_safety_poi_source source, int age_days, int depopulating) {
    struct navit_safety_poi poi;
    poi.source = source;
    poi.age_days = age_days;
    poi.depopulating_region = depopulating;
    return navit_safety_score_poi(&poi);
}

static int test_high_sources(void) {
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_CHAIN_API, 0, 0) == NAVIT_SAFETY_CONFIDENCE_HIGH, "Chain API should be High");
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_NREL, 500, 0) == NAVIT_SAFETY_CONFIDENCE_HIGH,
                "NREL should be High regardless of listed age");
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_USER_CONFIRMED, 9999, 1) == NAVIT_SAFETY_CONFIDENCE_HIGH,
                "User confirmation overrides age and depopulation");
    return 0;
}

static int test_age_dependent(void) {
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_OSM_NREL_MATCH, 10, 0) == NAVIT_SAFETY_CONFIDENCE_HIGH,
                "OSM/NREL match within 30 days should be High");
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_OSM_NREL_MATCH, 90, 0) == NAVIT_SAFETY_CONFIDENCE_MEDIUM,
                "OSM/NREL match older than 30 days should be Medium");
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_IOVERLANDER, 30, 0) == NAVIT_SAFETY_CONFIDENCE_MEDIUM,
                "iOverlander within 60 days should be Medium");
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_IOVERLANDER, 120, 0) == NAVIT_SAFETY_CONFIDENCE_LOW,
                "iOverlander older than 60 days should be Low");
    return 0;
}

static int test_osm_operators(void) {
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_OSM_CHAIN, 200, 0) == NAVIT_SAFETY_CONFIDENCE_MEDIUM,
                "OSM chain operator should be Medium");
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_OSM_INDEPENDENT, 100, 0) == NAVIT_SAFETY_CONFIDENCE_MEDIUM,
                "Independent operator within 12 months should be Medium");
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_OSM_INDEPENDENT, 700, 0) == NAVIT_SAFETY_CONFIDENCE_LOW,
                "Independent operator 1-3 years old should be Low");
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_OSM_INDEPENDENT, 2000, 0) == NAVIT_SAFETY_CONFIDENCE_LOW,
                "Independent operator over 3 years should be Low");
    return 0;
}

static int test_downgrades(void) {
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_OSM_CHAIN, 100, 1) == NAVIT_SAFETY_CONFIDENCE_LOW,
                "Depopulating region caps map POIs at Low");
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_OSM_CHAIN, 2000, 0) == NAVIT_SAFETY_CONFIDENCE_LOW,
                "Data over 3 years caps at Low");
    TEST_ASSERT(score(NAVIT_SAFETY_SRC_UNKNOWN, 0, 0) == NAVIT_SAFETY_CONFIDENCE_UNKNOWN,
                "Unknown source yields Unknown");
    TEST_ASSERT(navit_safety_score_poi(NULL) == NAVIT_SAFETY_CONFIDENCE_UNKNOWN, "NULL POI yields Unknown");
    return 0;
}

static int test_resupply_predicate(void) {
    TEST_ASSERT(navit_safety_confidence_counts_as_resupply(NAVIT_SAFETY_CONFIDENCE_HIGH), "High counts as resupply");
    TEST_ASSERT(navit_safety_confidence_counts_as_resupply(NAVIT_SAFETY_CONFIDENCE_MEDIUM),
                "Medium counts as resupply");
    TEST_ASSERT(!navit_safety_confidence_counts_as_resupply(NAVIT_SAFETY_CONFIDENCE_LOW),
                "Low does not count as resupply");
    TEST_ASSERT(!navit_safety_confidence_counts_as_resupply(NAVIT_SAFETY_CONFIDENCE_UNKNOWN),
                "Unknown does not count as resupply");
    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Navit Safety confidence tests...\n");
    failures += test_high_sources();
    failures += test_age_dependent();
    failures += test_osm_operators();
    failures += test_downgrades();
    failures += test_resupply_predicate();

    if (failures == 0) {
        printf("All Navit Safety confidence tests passed!\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
