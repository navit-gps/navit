/**
 * Navit, a modular navigation system.
 *
 * Unit tests for the Navit Safety heat-stress and water-requirement engine.
 */

#include <stdio.h>
#include "navit_safety.h"
#include "navit_safety_heat.h"

#define TEST_ASSERT(cond, message)                                                                                     \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

/* WBGT thresholds map to the documented risk levels. */
static int test_assess_thresholds(void) {
    TEST_ASSERT(navit_safety_heat_assess(27.9) == NAVIT_SAFETY_HEAT_NONE, "Below 28 C is no warning");
    TEST_ASSERT(navit_safety_heat_assess(28.0) == NAVIT_SAFETY_HEAT_CAUTION, "28 C is caution");
    TEST_ASSERT(navit_safety_heat_assess(31.9) == NAVIT_SAFETY_HEAT_CAUTION, "Just under 32 C is caution");
    TEST_ASSERT(navit_safety_heat_assess(32.0) == NAVIT_SAFETY_HEAT_WARNING, "32 C is warning");
    TEST_ASSERT(navit_safety_heat_assess(34.9) == NAVIT_SAFETY_HEAT_WARNING, "Just under 35 C is warning");
    TEST_ASSERT(navit_safety_heat_assess(35.0) == NAVIT_SAFETY_HEAT_DANGER, "35 C is danger");
    return 0;
}

/* The water model combines maintenance, exertion, and heat terms. */
static int test_water_requirement(void) {
    TEST_ASSERT(navit_safety_heat_water_ml_per_hour(0, 30.0, 1) == 0, "Non-positive weight yields zero");
    TEST_ASSERT(navit_safety_heat_water_ml_per_hour(70, 20.0, 0) == 70, "Rest in mild conditions is maintenance only");
    TEST_ASSERT(navit_safety_heat_water_ml_per_hour(70, 25.0, 1) == 350, "Strenuous at the heat base has no heat term");
    /* 70 rest + 280 exertion + (32-25)*20 = 140 heat = 490 */
    TEST_ASSERT(navit_safety_heat_water_ml_per_hour(70, 32.0, 1) == 490, "Strenuous in heat adds all three terms");
    return 0;
}

/* The plan honours the configuration: water always computed, level gated. */
static int test_heat_plan_config(void) {
    struct navit_safety_config config;
    struct navit_safety_heat_result result;

    navit_safety_config_default(&config);
    /* 70 rest + 280 exertion + (36-25)*20 = 220 heat = 570 */
    navit_safety_heat_plan(&config, 36.0, 1, &result);
    TEST_ASSERT(result.level == NAVIT_SAFETY_HEAT_DANGER, "36 C is danger with warnings enabled");
    TEST_ASSERT(result.avoid_exertion, "Danger level advises avoiding exertion");
    TEST_ASSERT(result.water_ml_per_hour == 570, "Water uses the configured body weight");

    config.heat_stress_warnings = 0;
    navit_safety_heat_plan(&config, 36.0, 1, &result);
    TEST_ASSERT(result.level == NAVIT_SAFETY_HEAT_NONE, "Disabled warnings report no level");
    TEST_ASSERT(!result.avoid_exertion, "Disabled warnings clear the avoid flag");
    TEST_ASSERT(result.water_ml_per_hour == 570, "Water requirement is still computed");
    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Navit Safety heat tests...\n");
    failures += test_assess_thresholds();
    failures += test_water_requirement();
    failures += test_heat_plan_config();

    if (failures == 0) {
        printf("All Navit Safety heat tests passed!\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
