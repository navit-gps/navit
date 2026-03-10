/**
 * Navit, a modular navigation system.
 *
 * Unit tests for Navit Safety plugin configuration defaults.
 */

#include "../../debug.h"
#include "../navit_safety.h"
#include <glib.h>
#include <stdio.h>
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
}

#define TEST_ASSERT(cond, message)                                                                                       \
    do {                                                                                                                 \
        if (!(cond)) {                                                                                                  \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);                                         \
            return 1;                                                                                                   \
        }                                                                                                               \
    } while (0)

static int test_config_defaults(void) {
    struct navit_safety_config config;

    memset(&config, 0, sizeof(config));
    navit_safety_config_default(&config);

    TEST_ASSERT(config.remote_mode == NAVIT_SAFETY_REMOTE_AUTO, "Default remote mode should be AUTO");
    TEST_ASSERT(config.poi_density_threshold_km == 80, "Default POI density threshold incorrect");
    TEST_ASSERT(config.koppen_trigger == 1, "Koppen trigger should be enabled by default");

    TEST_ASSERT(config.fuel_buffer_standard_km == 25, "Standard fuel buffer should be 25 km");
    TEST_ASSERT(config.fuel_buffer_remote_km == 85, "Remote fuel buffer should be 85 km");
    TEST_ASSERT(config.fuel_buffer_arid_km == 140, "Arid fuel buffer should be 140 km");

    TEST_ASSERT(config.water_buffer_standard_km == 5, "Standard water buffer should be 5 km");
    TEST_ASSERT(config.water_buffer_remote_km == 20, "Remote water buffer should be 20 km");
    TEST_ASSERT(config.water_buffer_arid_km == 30, "Arid water buffer should be 30 km");

    TEST_ASSERT(config.desert_consumption_warning == 1, "Desert consumption warning should be enabled");
    TEST_ASSERT(config.census_depopulation_layer == 1, "Census depopulation layer should be enabled");
    TEST_ASSERT(config.chain_api_queries == 1, "Chain API queries should be enabled by default");
    TEST_ASSERT(config.unconfirmed_poi_display == 1, "Unconfirmed POI display should be enabled");

    TEST_ASSERT(config.foot_travel_mode == 0, "Foot travel mode should be disabled by default");
    TEST_ASSERT(config.body_weight_kg == 70, "Default body weight should be 70 kg");
    TEST_ASSERT(config.heat_stress_warnings == 1, "Heat stress warnings should be enabled");

    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Navit Safety plugin configuration tests...\n");

    failures += test_config_defaults();

    if (failures == 0) {
        printf("All Navit Safety configuration tests passed!\n");
        return 0;
    }

    printf("%d test(s) failed\n", failures);
    return 1;
}
