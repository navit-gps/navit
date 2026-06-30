/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Unit tests for driver_break_geo (Haversine distance and route bbox API).
 */

#include "debug.h"
#include "driver_break_geo.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

dbg_level max_debug_level = lvl_error;

void debug_printf(dbg_level level, const char *module, const int mlen,
                  const char *function, const int flen, int prefix,
                  const char *fmt, ...) {
  (void)level;
  (void)module;
  (void)mlen;
  (void)function;
  (void)flen;
  (void)prefix;
  (void)fmt;
}

#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);       \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static int test_coord_distance_geo(void) {
  struct coord_geo oslo = {.lat = 59.9139, .lng = 10.7522};
  struct coord_geo bergen = {.lat = 60.3913, .lng = 5.3221};
  struct coord_geo same = oslo;
  double distance_m;

  distance_m = driver_break_coord_distance_geo(&oslo, &bergen);
  TEST_ASSERT(distance_m > 300000.0 && distance_m < 350000.0,
              "Oslo-Bergen distance out of range");

  distance_m = driver_break_coord_distance_geo(&oslo, &same);
  TEST_ASSERT(fabs(distance_m) < 1.0,
              "Same point distance should be near zero");

  distance_m = driver_break_coord_distance_geo(NULL, &bergen);
  TEST_ASSERT(distance_m == 0.0, "NULL first argument should return 0");

  return 0;
}

static int test_route_bbox_null_route(void) {
  double min_lon;
  double min_lat;
  double max_lon;
  double max_lat;

  TEST_ASSERT(driver_break_route_bbox(NULL, &min_lon, &min_lat, &max_lon,
                                      &max_lat) == 0,
              "NULL route should not produce bbox");
  return 0;
}

int main(void) {
  printf("Running Driver Break geo unit tests...\n");

  if (test_coord_distance_geo() != 0) {
    return 1;
  }
  if (test_route_bbox_null_route() != 0) {
    return 1;
  }

  printf("All geo unit tests passed!\n");
  return 0;
}
