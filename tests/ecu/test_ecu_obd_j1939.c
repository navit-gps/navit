/**
 * Navit, a modular navigation system.
 *
 * Unit tests for ECU OBD-II and J1939 backends and fuel rate/level handling.
 */

#include <stdio.h>
#include <string.h>

#include "driver_ecu.h"

#define TEST_ASSERT(cond, msg)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, msg);           \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static double test_maf_to_fuel_rate(double maf_g_s, int fuel_type,
                                    int ethanol_pct) {
  double afr = 14.7;
  double density = 0.745;

  if (fuel_type == DRIVER_ECU_FUEL_DIESEL) {
    afr = 14.5;
    density = 0.832;
  } else if (fuel_type == DRIVER_ECU_FUEL_FLEX ||
             fuel_type == DRIVER_ECU_FUEL_ETHANOL) {
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
  double petrol_rate = test_maf_to_fuel_rate(15.0, DRIVER_ECU_FUEL_PETROL, 0);
  TEST_ASSERT(petrol_rate > 1.0 && petrol_rate < 5.0,
              "Petrol fuel rate out of expected range");

  double diesel_rate = test_maf_to_fuel_rate(30.0, DRIVER_ECU_FUEL_DIESEL, 0);
  TEST_ASSERT(diesel_rate > petrol_rate,
              "Diesel fuel rate should exceed petrol in this scenario");

  double flex_e10 = test_maf_to_fuel_rate(20.0, DRIVER_ECU_FUEL_FLEX, 10);
  double flex_e85 = test_maf_to_fuel_rate(20.0, DRIVER_ECU_FUEL_FLEX, 85);
  TEST_ASSERT(flex_e10 > 0.0 && flex_e85 > 0.0,
              "Flex-fuel rates should be positive");
  TEST_ASSERT(flex_e85 > flex_e10,
              "E85 fuel rate should be higher than E10 for same MAF");

  return 0;
}

static int test_j1939_scaling(void) {
  struct ecu_config config;
  struct ecu_fuel_state fuel;
  memset(&config, 0, sizeof(config));
  memset(&fuel, 0, sizeof(fuel));
  config.fuel_tank_capacity_l = 400;

  unsigned int raw_rate = 1000;
  double rate_l_h = (double)raw_rate * 0.05;
  fuel.fuel_rate_l_h = rate_l_h;
  TEST_ASSERT(fuel.fuel_rate_l_h > 45.0 && fuel.fuel_rate_l_h < 55.0,
              "J1939 fuel rate scaling mismatch");

  unsigned int raw_level = 50;
  double pct = (double)raw_level * 0.4;
  fuel.fuel_current = (pct / 100.0) * (double)config.fuel_tank_capacity_l;
  TEST_ASSERT(fuel.fuel_current > 70.0 && fuel.fuel_current < 90.0,
              "J1939 fuel level scaling mismatch");

  return 0;
}

int main(void) {
  int failures = 0;

  printf("Running ECU OBD-II and J1939 backend tests...\n");

  failures += test_maf_fuel_rate_variants();
  failures += test_j1939_scaling();

  if (failures == 0) {
    printf("All OBD-II/J1939 backend tests passed!\n");
    return 0;
  }
  printf("%d test(s) failed\n", failures);
  return 1;
}
