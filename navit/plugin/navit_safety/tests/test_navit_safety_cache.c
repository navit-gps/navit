/**
 * Navit, a modular navigation system.
 *
 * Unit tests for the Navit Safety SQLite confirmation cache.
 */

#include "navit_safety_cache.h"
#include <stdio.h>

#define TEST_ASSERT(cond, message)                                                                                     \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static int test_confirm_and_query(void) {
    struct navit_safety_cache *cache = navit_safety_cache_open(":memory:");
    TEST_ASSERT(cache != NULL, "in-memory cache should open");

    TEST_ASSERT(!navit_safety_cache_is_confirmed(cache, "poi-1", "trip-A"), "unknown POI should not be confirmed");
    TEST_ASSERT(navit_safety_cache_confirm(cache, "poi-1", "trip-A", 1000), "confirming a POI should succeed");
    TEST_ASSERT(navit_safety_cache_is_confirmed(cache, "poi-1", "trip-A"), "confirmed POI should report confirmed");
    TEST_ASSERT(!navit_safety_cache_is_confirmed(cache, "poi-1", "trip-B"), "confirmation is scoped to its trip");
    TEST_ASSERT(!navit_safety_cache_is_confirmed(cache, "poi-2", "trip-A"), "other POIs remain unconfirmed");

    /* Re-confirming the same key must not fail (INSERT OR REPLACE). */
    TEST_ASSERT(navit_safety_cache_confirm(cache, "poi-1", "trip-A", 2000), "re-confirming should succeed");

    navit_safety_cache_close(cache);
    return 0;
}

static int test_null_safety(void) {
    TEST_ASSERT(navit_safety_cache_open(NULL) == NULL, "NULL path should fail to open");
    TEST_ASSERT(!navit_safety_cache_confirm(NULL, "p", "t", 0), "confirm on NULL cache fails");
    TEST_ASSERT(!navit_safety_cache_is_confirmed(NULL, "p", "t"), "query on NULL cache is false");
    navit_safety_cache_close(NULL); /* must not crash */
    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Navit Safety cache tests...\n");
    failures += test_confirm_and_query();
    failures += test_null_safety();

    if (failures == 0) {
        printf("All Navit Safety cache tests passed!\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
