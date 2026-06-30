/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Unit tests for driver_poi (ranking, free helpers, glacier constants).
 */

#include "debug.h"
#include "driver_break_glacier.h"
#include "driver_break_poi.h"
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

static struct driver_break_poi *poi_new(double lat, double lng,
                                        const char *name) {
  struct driver_break_poi *poi = g_new0(struct driver_break_poi, 1);

  poi->coord.lat = lat;
  poi->coord.lng = lng;
  poi->name = g_strdup(name);
  return poi;
}

static int test_poi_rank_sorts_by_distance(void) {
  struct coord_geo stop = {.lat = 59.91, .lng = 10.75};
  struct driver_break_config config;
  GList *pois = NULL;
  struct driver_break_poi *near_poi;
  struct driver_break_poi *far_poi;
  struct driver_break_poi *first;

  memset(&config, 0, sizeof(config));
  near_poi = poi_new(59.915, 10.755, "near");
  far_poi = poi_new(60.40, 5.32, "far");
  pois = g_list_append(pois, far_poi);
  pois = g_list_append(pois, near_poi);

  pois = driver_break_poi_rank(pois, &stop, &config);
  first = (struct driver_break_poi *)pois->data;
  TEST_ASSERT(first == near_poi, "Closest POI should rank first");

  driver_break_poi_free_list(pois);
  return 0;
}

static int test_poi_discover_null_center(void) {
  GList *pois = driver_break_poi_discover(NULL, 5, NULL, 0);

  TEST_ASSERT(pois == NULL, "NULL center should not produce POI list");
  return 0;
}

static int test_glacier_min_distance(void) {
  TEST_ASSERT(fabs(glacier_get_min_camping_distance() - 300.0) < 0.01,
              "Glacier camping buffer should be 300 m");
  return 0;
}

int main(void) {
  printf("Running Driver Break POI unit tests...\n");

  if (test_poi_rank_sorts_by_distance() != 0) {
    return 1;
  }
  if (test_poi_discover_null_center() != 0) {
    return 1;
  }
  if (test_glacier_min_distance() != 0) {
    return 1;
  }

  printf("All POI unit tests passed!\n");
  return 0;
}
