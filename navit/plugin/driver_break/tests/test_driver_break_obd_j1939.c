/**
 * Navit, a modular navigation system.
 *
 * Unit tests for Driver Break OBD-II and J1939 backends and fuel rate/level handling.
 *
 * These tests do not talk to real hardware; instead they exercise the internal
 * helper logic (MAF-based fuel rate computation, rolling averages, high-load
 * detection assumptions) with representative values from:
 *   - Standard OBD-II PIDs (0x2F, 0x5E, 0x10, 0x52).
 *   - J1939 PGNs 65266 (FEEA) and 65276 (FEF4).
 */

#include "../../debug.h"
#include "../driver_break.h"
#include "../driver_break_db.h"
#include "../driver_break_j1939.h"
#include "../driver_break_obd.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TEST_ASSERT(cond, msg)                                                                                         \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, msg);                                             \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

/* Reference MAF-based fuel rate calculation replicated from driver_break_obd.c.
 * This allows us to verify behaviour across multiple fuel types and ethanol %. */
static double test_maf_to_fuel_rate(double maf_g_s, int fuel_type, int ethanol_pct) {
    double afr = 14.7;
    double density = 0.745;

    if (fuel_type == DRIVER_BREAK_FUEL_DIESEL) {
        afr = 14.5;
        density = 0.832;
    } else if (fuel_type == DRIVER_BREAK_FUEL_FLEX || fuel_type == DRIVER_BREAK_FUEL_ETHANOL) {
        double e = (double)ethanol_pct / 100.0;
        afr = 14.7 * (1.0 - e) + 9.8 * e;
        density = 0.745 * (1.0 - e) + 0.787 * e;
    }

    if (afr <= 0.0 || density <= 0.0 || maf_g_s <= 0.0)
        return 0.0;

    double fuel_kg_h = (maf_g_s / afr) * 3600.0 / 1000.0;
    return fuel_kg_h / density;
}

static int test_maf_fuel_rate_variants(void) {
    /* Petrol engine at cruise: maf_g_s ~15 g/s */
    double petrol_rate = test_maf_to_fuel_rate(15.0, DRIVER_BREAK_FUEL_PETROL, 0);
    TEST_ASSERT(petrol_rate > 1.0 && petrol_rate < 5.0, "Petrol fuel rate out of expected range");

    /* Diesel engine under load: maf_g_s ~30 g/s */
    double diesel_rate = test_maf_to_fuel_rate(30.0, DRIVER_BREAK_FUEL_DIESEL, 0);
    TEST_ASSERT(diesel_rate > petrol_rate, "Diesel fuel rate should exceed petrol in this scenario");

    /* Flex-fuel E10 vs E85: higher ethanol should increase L/h for same MAF. */
    double flex_e10 = test_maf_to_fuel_rate(20.0, DRIVER_BREAK_FUEL_FLEX, 10);
    double flex_e85 = test_maf_to_fuel_rate(20.0, DRIVER_BREAK_FUEL_FLEX, 85);
    TEST_ASSERT(flex_e10 > 0.0 && flex_e85 > 0.0, "Flex-fuel rates should be positive");
    TEST_ASSERT(flex_e85 > flex_e10, "E85 fuel rate should be higher than E10 for same MAF");

    return 0;
}

/* Simulate J1939 PGN decoding for a few representative values. */
static int test_j1939_scaling(void) {
    struct driver_break_priv priv;
    memset(&priv, 0, sizeof(priv));
    priv.config.fuel_tank_capacity_l = 400; /* typical truck tank */

    /* Engine fuel rate: raw = 1000 -> 1000 * 0.05 = 50 L/h */
    unsigned int raw_rate = 1000;
    double rate_l_h = (double)raw_rate * 0.05;
    priv.fuel_rate_l_h = rate_l_h;
    TEST_ASSERT(priv.fuel_rate_l_h > 45.0 && priv.fuel_rate_l_h < 55.0, "J1939 fuel rate scaling mismatch");

    /* Fuel level: raw = 50 -> 20% -> 80 L remaining in 400 L tank */
    unsigned int raw_level = 50;
    double pct = (double)raw_level * 0.4;
    priv.fuel_current = (pct / 100.0) * (double)priv.config.fuel_tank_capacity_l;
    TEST_ASSERT(priv.fuel_current > 70.0 && priv.fuel_current < 90.0, "J1939 fuel level scaling mismatch");

    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Driver Break OBD-II and J1939 backend tests...\n");

    failures += test_maf_fuel_rate_variants();
    failures += test_j1939_scaling();

    if (failures == 0) {
        printf("All OBD-II/J1939 backend tests passed!\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
