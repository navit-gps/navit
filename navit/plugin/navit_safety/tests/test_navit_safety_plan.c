/**
 * Navit, a modular navigation system.
 *
 * Unit tests for the Navit Safety resource-planning orchestrator.
 */

#include <stdio.h>
#include "navit_safety.h"
#include "navit_safety_confidence.h"
#include "navit_safety_koppen.h"
#include "navit_safety_lookahead.h"
#include "navit_safety_plan.h"

#define TEST_ASSERT(cond, message)                                                                                     \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

/* Closely spaced reliable stops keep average spacing under the density threshold. */
static const struct navit_safety_stop dense_stops[] = {
    {25, NAVIT_SAFETY_CONFIDENCE_HIGH},
    {50, NAVIT_SAFETY_CONFIDENCE_HIGH},
    {75, NAVIT_SAFETY_CONFIDENCE_HIGH},
};

/* London midpoint with dense stops: non-arid, so Auto mode keeps standard buffers. */
static int test_populated_route(void) {
    struct navit_safety_config config;
    struct navit_safety_plan_result result;
    navit_safety_config_default(&config);
    navit_safety_plan(&config, NAVIT_SAFETY_RESOURCE_FUEL, 51.5, -0.12, dense_stops, 3, 100, 600, &result);
    TEST_ASSERT(!result.density_sparse, "Stops every 25 km are not sparse");
    TEST_ASSERT(!result.remote_active, "Auto mode in populated terrain is not remote");
    TEST_ASSERT(result.buffer_km == config.fuel_buffer_standard_km, "Standard fuel buffer applies");
    TEST_ASSERT(!result.desert_warning, "No desert warning in populated terrain");
    TEST_ASSERT(!result.gap.warning, "600 km range covers a 100 km route");
    return 0;
}

/* Auto mode in non-arid terrain still goes remote when confirmed POIs are sparse. */
static int test_density_triggers_remote(void) {
    struct navit_safety_config config;
    struct navit_safety_plan_result result;
    navit_safety_config_default(&config);
    navit_safety_plan(&config, NAVIT_SAFETY_RESOURCE_FUEL, 51.5, -0.12, NULL, 0, 300, 600, &result);
    TEST_ASSERT(result.density_sparse, "300 km with no confirmed stops is sparse");
    TEST_ASSERT(result.remote_active, "Sparse density triggers remote in Auto mode");
    TEST_ASSERT(!navit_safety_koppen_is_arid(result.zone), "London is not arid");
    TEST_ASSERT(result.buffer_km == config.fuel_buffer_remote_km, "Remote (non-arid) fuel buffer applies");
    TEST_ASSERT(!result.desert_warning, "Sparse-but-not-arid raises no desert warning");
    return 0;
}

/* Sahara midpoint: Auto + Koppen trigger selects the arid buffer. */
static int test_arid_route_uses_arid_buffer(void) {
    struct navit_safety_config config;
    struct navit_safety_plan_result result;
    navit_safety_config_default(&config);
    navit_safety_plan(&config, NAVIT_SAFETY_RESOURCE_FUEL, 25.0, 15.0, NULL, 0, 500, 600, &result);
    TEST_ASSERT(result.remote_active, "Auto mode in the Sahara is remote");
    TEST_ASSERT(navit_safety_koppen_is_arid(result.zone), "Sahara is classified arid");
    TEST_ASSERT(result.buffer_km == config.fuel_buffer_arid_km, "Arid fuel buffer applies");
    TEST_ASSERT(result.desert_warning, "Arid remote planning raises the desert consumption warning");
    TEST_ASSERT(result.usable_range_km == 600 - config.fuel_buffer_arid_km, "Usable range removes the buffer");
    TEST_ASSERT(result.gap.warning, "500 km route exceeds the buffered range");
    return 0;
}

/* The desert consumption warning honours its config toggle. */
static int test_desert_warning_toggle(void) {
    struct navit_safety_config config;
    struct navit_safety_plan_result result;
    navit_safety_config_default(&config);
    config.desert_consumption_warning = 0;
    navit_safety_plan(&config, NAVIT_SAFETY_RESOURCE_FUEL, 25.0, 15.0, NULL, 0, 500, 600, &result);
    TEST_ASSERT(result.remote_active, "Sahara is still remote with the warning disabled");
    TEST_ASSERT(!result.desert_warning, "Disabled toggle suppresses the desert warning");
    return 0;
}

/* Koppen trigger disabled with dense stops: Auto mode stays non-remote even in the desert. */
static int test_koppen_trigger_disabled(void) {
    struct navit_safety_config config;
    struct navit_safety_plan_result result;
    navit_safety_config_default(&config);
    config.koppen_trigger = 0;
    navit_safety_plan(&config, NAVIT_SAFETY_RESOURCE_FUEL, 25.0, 15.0, dense_stops, 3, 100, 600, &result);
    TEST_ASSERT(!result.density_sparse, "Dense stops keep the density signal clear");
    TEST_ASSERT(!result.remote_active, "Disabled Koppen trigger keeps Auto non-remote");
    TEST_ASSERT(result.buffer_km == config.fuel_buffer_standard_km, "Standard buffer without trigger");
    return 0;
}

/* Always mode forces remote planning regardless of terrain. */
static int test_always_mode(void) {
    struct navit_safety_config config;
    struct navit_safety_plan_result result;
    navit_safety_config_default(&config);
    config.remote_mode = NAVIT_SAFETY_REMOTE_ALWAYS;
    navit_safety_plan(&config, NAVIT_SAFETY_RESOURCE_WATER, 51.5, -0.12, NULL, 0, 30, 40, &result);
    TEST_ASSERT(result.remote_active, "Always mode is remote in populated terrain");
    TEST_ASSERT(result.buffer_km == config.water_buffer_remote_km, "Remote water buffer applies (non-arid)");
    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Navit Safety plan tests...\n");
    failures += test_populated_route();
    failures += test_density_triggers_remote();
    failures += test_arid_route_uses_arid_buffer();
    failures += test_desert_warning_toggle();
    failures += test_koppen_trigger_disabled();
    failures += test_always_mode();

    if (failures == 0) {
        printf("All Navit Safety plan tests passed!\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
