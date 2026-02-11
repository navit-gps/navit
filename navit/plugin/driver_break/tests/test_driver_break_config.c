/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Unit tests for Driver Break plugin configuration and driving time calculations
 */

#include "../../debug.h"
#include "../driver_break.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

static void driver_break_config_default(struct driver_break_config *config) {
    memset(config, 0, sizeof(*config));
    config->car_soft_limit_hours = 7;
    config->car_max_hours = 10;
    config->car_break_interval_hours = 4;
    config->car_break_duration_min = 30;
    config->truck_mandatory_break_after_hours = 4;
    config->truck_mandatory_break_duration_min = 45;
    config->truck_max_daily_hours = 9;
    config->min_distance_from_buildings = 150;
    config->poi_search_radius_km = 15;
    config->driver_break_interval_min_hours = 1;
    config->driver_break_interval_max_hours = 6;
    config->vehicle_type = 0;
}

static int test_config_defaults(void) {
    struct driver_break_config config;
    driver_break_config_default(&config);

    TEST_ASSERT(config.vehicle_type == 0, "Default vehicle type should be car");
    TEST_ASSERT(config.car_soft_limit_hours == 7, "Default car soft limit incorrect");
    TEST_ASSERT(config.car_max_hours == 10, "Default car max hours incorrect");
    TEST_ASSERT(config.truck_mandatory_break_after_hours == 4, "Default truck break hours incorrect");
    TEST_ASSERT(config.truck_mandatory_break_duration_min == 45, "Default truck break duration incorrect");
    TEST_ASSERT(config.min_distance_from_buildings == 150, "Default min distance incorrect");

    return 0;
}

static int test_driving_session_init(void) {
    struct driving_session session;
    memset(&session, 0, sizeof(session));

    TEST_ASSERT(session.start_time == 0, "Session should start uninitialized");
    TEST_ASSERT(session.total_driving_minutes == 0, "Total driving should start at 0");
    TEST_ASSERT(session.continuous_driving_minutes == 0, "Continuous driving should start at 0");
    TEST_ASSERT(session.mandatory_break_required == 0, "No mandatory break initially");

    return 0;
}

static int test_truck_mandatory_break_calculation(void) {
    struct driver_break_config config;
    driver_break_config_default(&config);
    config.vehicle_type = 1; /* Truck */

    /* Test: 4.5 hours = 270 minutes should require mandatory break */
    int driving_minutes = 270;
    int should_require_break = (driving_minutes >= (config.truck_mandatory_break_after_hours * 60));

    TEST_ASSERT(should_require_break == 1, "Truck should require break after 4.5 hours");

    /* Test: 3 hours = 180 minutes should NOT require mandatory break */
    driving_minutes = 180;
    should_require_break = (driving_minutes >= (config.truck_mandatory_break_after_hours * 60));
    TEST_ASSERT(should_require_break == 0, "Truck should NOT require break after 3 hours");

    return 0;
}

static int test_car_break_recommendation(void) {
    struct driver_break_config config;
    driver_break_config_default(&config);
    config.vehicle_type = 0; /* Car */

    /* Test: 4 hours = 240 minutes should trigger recommendation */
    int driving_minutes = 240;
    int should_recommend = (driving_minutes >= (config.car_break_interval_hours * 60));

    TEST_ASSERT(should_recommend == 1, "Car should recommend break after 4 hours");

    /* Test: 2 hours = 120 minutes should NOT trigger recommendation */
    driving_minutes = 120;
    should_recommend = (driving_minutes >= (config.car_break_interval_hours * 60));
    TEST_ASSERT(should_recommend == 0, "Car should NOT recommend break after 2 hours");

    return 0;
}

static int test_daily_driving_limits(void) {
    struct driver_break_config config;
    driver_break_config_default(&config);

    /* Car limits */
    TEST_ASSERT(config.car_soft_limit_hours >= 5 && config.car_soft_limit_hours <= 9,
                "Car soft limit should be 5-9 hours");
    TEST_ASSERT(config.car_max_hours >= 10 && config.car_max_hours <= 12, "Car max limit should be 10-12 hours");

    /* Truck limits (EU Regulation EC 561/2006) */
    TEST_ASSERT(config.truck_max_daily_hours == 9, "Truck max daily hours should be 9 (EU Regulation)");
    TEST_ASSERT(config.truck_mandatory_break_after_hours == 4,
                "Truck mandatory break should be after 4.5 hours (EU Regulation)");
    TEST_ASSERT(config.truck_mandatory_break_duration_min == 45,
                "Truck mandatory break duration should be 45 minutes (EU Regulation)");

    return 0;
}

static int test_driver_break_interval_range(void) {
    struct driver_break_config config;
    driver_break_config_default(&config);

    TEST_ASSERT(config.driver_break_interval_min_hours == 1, "Min rest interval should be 1 hour");
    TEST_ASSERT(config.driver_break_interval_max_hours == 6, "Max rest interval should be 6 hours");
    TEST_ASSERT(config.driver_break_interval_min_hours < config.driver_break_interval_max_hours,
                "Min rest interval should be less than max");

    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running Driver Break plugin configuration tests...\n");

    failures += test_config_defaults();
    failures += test_driving_session_init();
    failures += test_truck_mandatory_break_calculation();
    failures += test_car_break_recommendation();
    failures += test_daily_driving_limits();
    failures += test_driver_break_interval_range();

    if (failures == 0) {
        printf("All configuration tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
}
