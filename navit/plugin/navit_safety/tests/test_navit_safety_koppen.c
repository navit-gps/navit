/**
 * Navit, a modular navigation system.
 *
 * Unit tests for Navit Safety Koppen classification.
 */

#include <stdio.h>
#include <string.h>
#include "navit_safety_koppen.h"

#define TEST_ASSERT(cond, message)                                                                                     \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static int test_zone_predicates(void) {
    TEST_ASSERT(navit_safety_koppen_is_arid(NAVIT_SAFETY_KOPPEN_BWH), "BWh is arid");
    TEST_ASSERT(navit_safety_koppen_is_arid(NAVIT_SAFETY_KOPPEN_BWK), "BWk is arid");
    TEST_ASSERT(!navit_safety_koppen_is_arid(NAVIT_SAFETY_KOPPEN_BSH), "BSh is not desert");
    TEST_ASSERT(navit_safety_koppen_is_semiarid(NAVIT_SAFETY_KOPPEN_BSH), "BSh is semi-arid");
    TEST_ASSERT(navit_safety_koppen_is_semiarid(NAVIT_SAFETY_KOPPEN_BSK), "BSk is semi-arid");
    TEST_ASSERT(navit_safety_koppen_triggers_remote(NAVIT_SAFETY_KOPPEN_BWH), "BWh triggers remote");
    TEST_ASSERT(navit_safety_koppen_triggers_remote(NAVIT_SAFETY_KOPPEN_BSK), "BSk triggers remote");
    TEST_ASSERT(!navit_safety_koppen_triggers_remote(NAVIT_SAFETY_KOPPEN_OTHER), "Other does not trigger");
    TEST_ASSERT(!navit_safety_koppen_triggers_remote(NAVIT_SAFETY_KOPPEN_UNKNOWN), "Unknown does not trigger");
    return 0;
}

static int test_lookup_arid(void) {
    /* Central Sahara (Libya). */
    TEST_ASSERT(navit_safety_koppen_is_arid(navit_safety_koppen_lookup(25.0, 15.0)),
                "Central Sahara should be desert");
    /* Empty Quarter, Arabian Peninsula. */
    TEST_ASSERT(navit_safety_koppen_is_arid(navit_safety_koppen_lookup(20.0, 48.0)),
                "Arabian Peninsula should be desert");
    /* Australian interior. */
    TEST_ASSERT(navit_safety_koppen_triggers_remote(navit_safety_koppen_lookup(-25.0, 130.0)),
                "Australian interior should trigger remote");
    return 0;
}

static int test_lookup_non_arid(void) {
    /* London. */
    TEST_ASSERT(navit_safety_koppen_lookup(51.5, -0.12) == NAVIT_SAFETY_KOPPEN_OTHER,
                "London should be a non-arid zone");
    /* Out of range. */
    TEST_ASSERT(navit_safety_koppen_lookup(200.0, 0.0) == NAVIT_SAFETY_KOPPEN_UNKNOWN,
                "Out-of-range latitude should be Unknown");
    return 0;
}

static int test_code_strings(void) {
    TEST_ASSERT(strcmp(navit_safety_koppen_code(NAVIT_SAFETY_KOPPEN_BWH), "BWh") == 0, "BWh code");
    TEST_ASSERT(strcmp(navit_safety_koppen_code(NAVIT_SAFETY_KOPPEN_BSK), "BSk") == 0, "BSk code");
    TEST_ASSERT(navit_safety_koppen_code(NAVIT_SAFETY_KOPPEN_UNKNOWN) != NULL, "code is never NULL");
    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Navit Safety Koppen tests...\n");
    failures += test_zone_predicates();
    failures += test_lookup_arid();
    failures += test_lookup_non_arid();
    failures += test_code_strings();

    if (failures == 0) {
        printf("All Navit Safety Koppen tests passed!\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
