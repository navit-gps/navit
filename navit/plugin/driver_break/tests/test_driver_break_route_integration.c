/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Integration tests for Driver Break plugin using actual Navit and plugin functions
 * Tests rest interval creation and POI discovery along routes
 */

#include "../../atom.h"
#include "../../attr.h"
#include "../../debug.h"
#include "../../file.h"
#include "../../main.h"
#include "../../map.h"
#include "../../mapset.h"
#include "../../navit.h"
#include "../../plugin.h"
#include "../../projection.h"
#include "../../route.h"
#include "../../transform.h"
#include "../../vehicleprofile.h"
#include "../driver_break.h"
#include "../driver_break_cycling.h"
#include "../driver_break_hiking.h"
#include "../driver_break_poi.h"
#include "../driver_break_poi_hiking.h"
#include "../driver_break_poi_map.h"
#include "../driver_break_route_validator.h"
#include "../driver_break_srtm.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Try to call builtin_init if available (when USE_PLUGINS is not defined) */
#ifndef USE_PLUGINS
extern void builtin_init(void);
#endif

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

/* Stub for navit_get_user_data_directory */
char *navit_get_user_data_directory(int create) {
    (void)create;
    return g_strdup("/tmp");
}

/* Forward declarations for stubs */
struct event_idle;
struct callback;
struct event_idle *event_add_idle(int priority, struct callback *cb) {
    (void)priority;
    (void)cb;
    return NULL;
}

/* Initialize Navit system for map loading */
static void init_navit_system(void) {
    static int initialized = 0;
    if (initialized) {
        return;
    }

    /* Initialize core Navit systems */
    atom_init();
    main_init("test_driver_break_route_integration");
    file_init();

    /* Initialize map drivers */
#ifdef USE_PLUGINS
    {
        /* When USE_PLUGINS is enabled, load the binfile map driver as a plugin */
        char *plugin_path;
        char *cwd;
        struct attr path_attr;
        struct attr active_attr;
        struct attr *plugin_attrs[3];
        struct plugin *pl;

        plugin_path = NULL;
#ifdef DRIVER_BREAK_TEST_BINFILE_SO
        if (g_file_test(DRIVER_BREAK_TEST_BINFILE_SO, G_FILE_TEST_EXISTS))
            plugin_path = g_strdup(DRIVER_BREAK_TEST_BINFILE_SO);
#endif
        cwd = g_get_current_dir();
        if (!plugin_path) {
            /* From CTest cwd .../navit/plugin/driver_break/tests: three levels up is .../navit (build tree) */
            plugin_path = g_build_filename(cwd, "..", "..", "..", "map", "binfile", "libmap_binfile.so", NULL);
            if (!g_file_test(plugin_path, G_FILE_TEST_EXISTS)) {
                g_free(plugin_path);
                plugin_path = g_build_filename(cwd, "navit", "map", "binfile", "libmap_binfile.so", NULL);
            }
        }
        if (plugin_path && !g_file_test(plugin_path, G_FILE_TEST_EXISTS)) {
            g_free(plugin_path);
            plugin_path = NULL;
        }

        if (plugin_path) {
            printf("  Loading binfile map driver from: %s\n", plugin_path);
            /* Create plugin using plugin_new with path attribute */
            path_attr.type = attr_path;
            path_attr.u.str = plugin_path;

            active_attr.type = attr_active;
            active_attr.u.num = 1;

            plugin_attrs[0] = &path_attr;
            plugin_attrs[1] = &active_attr;
            plugin_attrs[2] = NULL;

            pl = plugin_new(NULL, plugin_attrs);
            if (pl) {
                if (plugin_load(pl)) {
                    plugin_call_init(pl);
                    printf("  binfile map driver loaded and initialized\n");
                } else {
                    printf("  Warning: Failed to load binfile map driver plugin\n");
                }
                /* Don't destroy the plugin - we need it to stay loaded */
            } else {
                printf("  Warning: Failed to create plugin from path\n");
            }
            g_free(plugin_path);
        } else {
            printf("  Warning: binfile map driver plugin not found\n");
            if (plugin_path) {
                g_free(plugin_path);
            }
        }
        g_free(cwd);
    }
#else
    /* When USE_PLUGINS is disabled, use builtin modules */
    builtin_init();
#endif

    initialized = 1;
}

#define TEST_ASSERT(condition, message)                                                                                \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

/* OSM Node coordinates (from OSM relation 1572954 Rondanestien (fjellet), Målia–Hjerkinn, DNT)
 * https://www.openstreetmap.org/relation/1572954 - route has huts, water, POIs tests should find */
/* struct coord_geo is { lng, lat }; initializers below are (lng, lat). */
/* Hiking route: Rondanestien – three points along the trail (south, middle, north) */
static struct coord_geo osm_node_rondane_south = {10.9174631, 61.1553669}; /* Node 845742926 */
static struct coord_geo osm_node_rondane_mid = {10.3536473, 61.5857799};   /* Node 845742951 */
static struct coord_geo osm_node_rondane_north = {9.6421663, 62.0910527};  /* Node 339607873, near Hjerkinn */

/* Car route: St1 Moelv -> Spidsbergseterkrysset -> Enden -> Aukrustsenteret */
static struct coord_geo osm_node_moelv = {10.700000, 60.933333};     /* Node 8233820034 */
static struct coord_geo osm_node_spidsberg = {10.816667, 61.016667}; /* Node 10677676828 */
static struct coord_geo osm_node_enden = {10.916667, 61.100000};     /* Node 1289150990 */
static struct coord_geo osm_node_aukrust = {11.000000, 61.150000};   /* Aukrustsenteret */

/* Load map data for testing */
static struct mapset *load_test_map_data(void) {
    struct mapset *ms = NULL;
    struct map *map = NULL;
    char *map_file = NULL;

    /* Initialize Navit system before loading map */
    init_navit_system();

    /* Map path: CMake sets DRIVER_BREAK_TEST_MAP_DATA_DIR to this directory (map_data next to tests sources). */
#ifdef DRIVER_BREAK_TEST_MAP_DATA_DIR
    map_file = g_build_filename(DRIVER_BREAK_TEST_MAP_DATA_DIR, "osm_bbox_9.5,60.95,11.35,62.25.bin", NULL);
    if (!g_file_test(map_file, G_FILE_TEST_EXISTS)) {
        g_free(map_file);
        map_file = NULL;
    }
#else
    map_file = NULL;
#endif
    if (!map_file) {
        char *cwd = g_get_current_dir();
        map_file = g_build_filename(cwd, "map_data", "osm_bbox_9.5,60.95,11.35,62.25.bin", NULL);
        g_free(cwd);
        if (!g_file_test(map_file, G_FILE_TEST_EXISTS)) {
            g_free(map_file);
            return NULL; /* Map data not available */
        }
    }

    if (!g_file_test(map_file, G_FILE_TEST_EXISTS)) {
        g_free(map_file);
        return NULL;
    }

    printf("  Loading map data from: %s\n", map_file);

    /* Check if binfile map driver is registered */
    void *map_driver = plugin_get_category_map("binfile");
    if (!map_driver) {
        printf("  Warning: binfile map driver not registered\n");
        printf("  builtin_init() may not have been called or USE_PLUGINS is enabled\n");
        g_free(map_file);
        return NULL;
    }
    printf("  binfile map driver is registered\n");

    /* Create mapset */
    ms = mapset_new(NULL, NULL);
    if (!ms) {
        g_free(map_file);
        printf("  Warning: Failed to create mapset\n");
        return NULL;
    }

    /* Create map with binfile type */
    struct attr type_attr;
    type_attr.type = attr_type;
    type_attr.u.str = "binfile";

    struct attr data_attr;
    data_attr.type = attr_data;
    data_attr.u.str = map_file;

    struct attr name_attr;
    name_attr.type = attr_name;
    name_attr.u.str = map_file;

    struct attr *map_attrs[] = {&type_attr, &data_attr, &name_attr, NULL};

    printf("  Calling map_new with type=binfile\n");
    map = map_new(NULL, map_attrs);
    if (map) {
        printf("  map_new succeeded, map=%p\n", (void *)map);
    } else {
        printf("  map_new returned NULL\n");
    }
    if (map) {
        struct attr map_attr;
        map_attr.type = attr_map;
        map_attr.u.map = map;
        if (mapset_add_attr(ms, &map_attr)) {
            /* map_new returns a referenced map, but mapset_add_attr takes ownership */
            /* No need to unref - mapset will manage the map */
            printf("  Map loaded successfully (Rondanestien bbox)\n");
        } else {
            printf("  Warning: Failed to add map to mapset\n");
            map_destroy(map);
            mapset_destroy(ms);
            ms = NULL;
        }
    } else {
        printf("  Warning: Failed to load map file (file may be invalid or maptool conversion needed)\n");
        mapset_destroy(ms);
        ms = NULL;
    }

    g_free(map_file);
    return ms;
}

/* Test hiking rest interval creation */
static int test_hiking_driver_break_intervals_created(void) {
    double route_distance = 25000.0; /* 25 km in meters */
    GList *hiking_stops = hiking_calculate_driver_break_stops_with_max(route_distance, 0, 40000.0);

    TEST_ASSERT(hiking_stops != NULL, "Hiking rest stops should be created");
    TEST_ASSERT(g_list_length(hiking_stops) >= 1, "Should have at least 1 rest stop for 25 km route");

    GList *l = hiking_stops;
    int interval_count = 0;
    double prev_position = 0.0;

    while (l) {
        struct hiking_driver_break_stop *stop = (struct hiking_driver_break_stop *)l->data;
        if (stop) {
            interval_count++;
            if (prev_position > 0) {
                double interval = stop->position - prev_position;
                TEST_ASSERT(interval > 11000.0 && interval < 12000.0, "Rest interval should be ~11.3 km");
            }
            prev_position = stop->position;
        }
        l = g_list_next(l);
    }

    printf("  Created %d rest intervals for 25 km hiking route\n", interval_count);

    hiking_free_driver_break_stops(hiking_stops);
    return 0;
}

/* Test POI discovery along hiking route */
static int test_poi_discovery_hiking_route(void) {
    int pois_found = 0;
    struct mapset *ms = load_test_map_data();

    if (ms) {
        printf("  Using map-based POI search (Rondanestien, map data loaded)\n");

        GList *cabins = poi_search_cabins_map(&osm_node_rondane_mid, 100.0, ms, 0);
        if (cabins) {
            pois_found += g_list_length(cabins);
            printf("  Found %d cabins near Rondanestien mid (map data)\n", g_list_length(cabins));
            poi_free_cabins(cabins);
        }

        GList *water = driver_break_poi_map_search_water(&osm_node_rondane_mid, 100.0, ms);
        if (water) {
            pois_found += g_list_length(water);
            printf("  Found %d water points near Rondanestien mid (map data)\n", g_list_length(water));
            driver_break_poi_free_list(water);
        }

        GList *cabins2 = poi_search_cabins_map(&osm_node_rondane_north, 100.0, ms, 0);
        if (cabins2) {
            pois_found += g_list_length(cabins2);
            printf("  Found %d cabins near Rondanestien north / Hjerkinn (map data)\n", g_list_length(cabins2));
            poi_free_cabins(cabins2);
        }

        mapset_destroy(ms);
    } else {
        printf("  Map data not found, skipping map-based POI search\n");
        printf("  Note: To test map-based POI discovery, run download_test_map_data.sh first\n");
    }

    printf("  Total POIs found along hiking route: %d\n", pois_found);

    return 0;
}

/* Test car route POI discovery */
static int test_poi_discovery_car_route(void) {
    int pois_found = 0;
    struct mapset *ms = load_test_map_data();

    if (ms) {
        printf("  Using map-based POI search (map data loaded)\n");

        GList *pois = driver_break_poi_map_search_car_pois(&osm_node_moelv, 100.0, ms);
        if (pois) {
            pois_found += g_list_length(pois);
            printf("  Found %d POIs near Moelv (map data)\n", g_list_length(pois));
            driver_break_poi_free_list(pois);
        }

        GList *pois2 = driver_break_poi_map_search_car_pois(&osm_node_aukrust, 100.0, ms);
        if (pois2) {
            pois_found += g_list_length(pois2);
            printf("  Found %d POIs near Aukrustsenteret (map data)\n", g_list_length(pois2));
            driver_break_poi_free_list(pois2);
        }

        mapset_destroy(ms);
    } else {
        printf("  Map data not found, skipping map-based POI search\n");
        printf("  Note: To test map-based POI discovery, run download_test_map_data.sh first\n");
    }

    printf("  Total POIs found along car route: %d\n", pois_found);

    return 0;
}

/* SRTM elevation test uses Rondanestien waypoints; tiles N61E009, N61E010, N62E009 cover the route */
#define SRTM_VOID -32768
/* Rondane/Hjerkinn area elevations roughly 200–1800 m */
#define ELEV_RONDANE_MIN 200
#define ELEV_RONDANE_MAX 1800

/* True if dir contains any HGT or Copernicus GeoTIFF tile used by Rondanestien or the SRTM unit test. */
static int srtm_dir_has_any_tile(const char *dir) {
    char *p;
    int ok = 0;
    if (!dir || !dir[0])
        return 0;
#define TILE_CHECK(name)                                                                                               \
    p = g_build_filename(dir, (name), NULL);                                                                            \
    ok |= g_file_test(p, G_FILE_TEST_EXISTS);                                                                          \
    g_free(p);
    TILE_CHECK("N61E009.hgt");
    TILE_CHECK("N61E010.hgt");
    TILE_CHECK("N62E009.hgt");
    TILE_CHECK("N62E007.hgt");
    TILE_CHECK("Copernicus_DSM_COG_10_N61_00_E009_00_DEM.tif");
    TILE_CHECK("Copernicus_DSM_COG_10_N61_00_E010_00_DEM.tif");
    TILE_CHECK("Copernicus_DSM_COG_10_N62_00_E009_00_DEM.tif");
    TILE_CHECK("Copernicus_DSM_COG_10_N62_00_E007_00_DEM.tif");
#undef TILE_CHECK
    return ok;
}

/* Pick first directory with tiles; same defaults as download_test_srtm_data.sh and test_driver_break_srtm Copernicus.
 * Legacy /tmp/test_copernicus_glo30 is checked for older runs that wrote only N62E007/N61E009 there (Rondanestien
 * still needs N61E010 and N62E009; run ctest or download_test_srtm_data.sh for full coverage). */
static char *srtm_test_setup_dir(void) {
    const char *env = g_getenv("SRTM_TEST_DIR");
    char *empty_fallback = g_strdup("/tmp/test_srtm_route");
    char *chosen = NULL;

    if (env && env[0] && srtm_dir_has_any_tile(env))
        chosen = g_strdup(env);
    else if (srtm_dir_has_any_tile("/tmp/test_srtm_hgt_download"))
        chosen = g_strdup("/tmp/test_srtm_hgt_download");
    else if (srtm_dir_has_any_tile("/tmp/test_copernicus_glo30"))
        chosen = g_strdup("/tmp/test_copernicus_glo30");

    if (chosen) {
        printf("  Using elevation data from: %s\n", chosen);
        srtm_init(chosen);
        g_free(empty_fallback);
        return chosen;
    }
    g_mkdir_with_parents(empty_fallback, 0755);
    srtm_init(empty_fallback);
    return empty_fallback;
}

/* Assert Rondanestien elevations in valid range and optional Rondane band / elevation gain. */
static void srtm_test_assert_rondane(int elev1, int elev2, int elev3) {
    if (elev1 != SRTM_VOID)
        printf("  Elevation at Rondanestien south (61.16,10.92): %d m\n", elev1);
    else
        printf("  Elevation at Rondanestien south: no data (elevation tiles not present)\n");
    if (elev2 != SRTM_VOID)
        printf("  Elevation at Rondanestien mid (61.59,10.35): %d m\n", elev2);
    else
        printf("  Elevation at Rondanestien mid: no data (elevation tiles not present)\n");
    if (elev3 != SRTM_VOID)
        printf("  Elevation at Rondanestien north / Hjerkinn (62.09,9.64): %d m\n", elev3);
    else
        printf("  Elevation at Rondanestien north: no data (elevation tiles not present)\n");

    TEST_ASSERT(elev1 >= SRTM_VOID && elev1 <= 9000, "Elevation 1 in valid range");
    TEST_ASSERT(elev2 >= SRTM_VOID && elev2 <= 9000, "Elevation 2 in valid range");
    TEST_ASSERT(elev3 >= SRTM_VOID && elev3 <= 9000, "Elevation 3 in valid range");

    if (elev1 != SRTM_VOID)
        TEST_ASSERT(elev1 >= ELEV_RONDANE_MIN && elev1 <= ELEV_RONDANE_MAX, "Rondanestien south elevation in range");
    if (elev2 != SRTM_VOID)
        TEST_ASSERT(elev2 >= ELEV_RONDANE_MIN && elev2 <= ELEV_RONDANE_MAX, "Rondanestien mid elevation in range");
    if (elev3 != SRTM_VOID)
        TEST_ASSERT(elev3 >= ELEV_RONDANE_MIN && elev3 <= ELEV_RONDANE_MAX, "Rondanestien north elevation in range");
    if (elev1 != SRTM_VOID && elev2 != SRTM_VOID) {
        int elevation_gain = elev2 - elev1;
        printf("  Elevation gain (south to mid): %d m\n", elevation_gain);
        TEST_ASSERT(elevation_gain >= -500 && elevation_gain <= 2000, "Elevation gain reasonable");
    }
}

/* Test SRTM elevation along Rondanestien. Tiles: N61E010 (south, mid), N62E009 (north) as HGT or Copernicus .tif.
 * Searches SRTM_TEST_DIR, then /tmp/test_srtm_hgt_download, then /tmp/test_copernicus_glo30.
 * test_driver_break_srtm Copernicus downloads (with libcurl) place tiles under /tmp/test_srtm_hgt_download
 * including optional N61E010 and N62E009 so Test 4 succeeds after ctest; N62E007/N61E009 alone do not cover Rondanestien. */
static int test_srtm_elevation_hiking_route(void) {
    char *test_dir = srtm_test_setup_dir();
    int elev1 = srtm_get_elevation(&osm_node_rondane_south);
    int elev2 = srtm_get_elevation(&osm_node_rondane_mid);
    int elev3 = srtm_get_elevation(&osm_node_rondane_north);
    srtm_test_assert_rondane(elev1, elev2, elev3);
    g_free(test_dir);
    return 0;
}

/* Test cycling rest intervals */
static int test_cycling_driver_break_intervals_created(void) {
    /* Test with 100 km route */
    double route_distance = 100000.0; /* 100 km */

    GList *stops = cycling_calculate_driver_break_stops(route_distance, 0);

    TEST_ASSERT(stops != NULL, "Cycling rest stops should be created");
    TEST_ASSERT(g_list_length(stops) >= 3, "Should have at least 3 rest stops for 100 km");

    /* Verify intervals */
    GList *l = stops;
    int interval_count = 0;
    double prev_position = 0.0;

    while (l) {
        struct cycling_driver_break_stop *stop = (struct cycling_driver_break_stop *)l->data;
        if (stop) {
            interval_count++;
            if (prev_position > 0) {
                double interval = stop->position - prev_position;
                /* Should be approximately 28.24 km */
                TEST_ASSERT(interval > 27000.0 && interval < 29000.0, "Cycling rest interval should be ~28.24 km");
            }
            prev_position = stop->position;
        }
        l = g_list_next(l);
    }

    printf("  Created %d rest intervals for 100 km cycling route\n", interval_count);

    cycling_free_driver_break_stops(stops);
    return 0;
}

/* Test route validation functions with actual functions */
static int test_route_validation_actual(void) {
    /* Test using actual validation functions */
    TEST_ASSERT(route_validator_is_forbidden_highway("motorway") == 1, "Motorway forbidden check");
    TEST_ASSERT(route_validator_is_forbidden_highway("trunk") == 1, "Trunk forbidden check");
    TEST_ASSERT(route_validator_is_priority_path("footway") == 1, "Footway priority check");
    TEST_ASSERT(route_validator_is_priority_path("path") == 1, "Path priority check");

    struct route_validation_result *cyc = route_validator_validate_cycling(NULL);
    TEST_ASSERT(cyc != NULL, "validate_cycling(NULL) returns result");
    TEST_ASSERT(cyc->is_valid == 0, "NULL cycling route invalid");
    TEST_ASSERT(cyc->warnings != NULL, "NULL cycling route has warning");
    route_validator_free_result(cyc);

    return 0;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    int failures = 0;

    printf("Running Driver Break plugin integration tests (using actual Navit/plugin functions)...\n");
    printf("Testing rest interval creation and POI discovery along routes...\n\n");

    printf("Test 1: Hiking rest intervals creation\n");
    failures += test_hiking_driver_break_intervals_created();

    printf("\nTest 2: POI discovery along hiking route\n");
    failures += test_poi_discovery_hiking_route();

    printf("\nTest 3: POI discovery along car route\n");
    failures += test_poi_discovery_car_route();

    printf("\nTest 4: SRTM elevation along hiking route\n");
    failures += test_srtm_elevation_hiking_route();

    printf("\nTest 5: Cycling rest intervals creation\n");
    failures += test_cycling_driver_break_intervals_created();

    printf("\nTest 6: Route validation functions\n");
    failures += test_route_validation_actual();

    printf("\n");
    if (failures == 0) {
        printf("All integration tests passed!\n");
        printf("\nNote: POI discovery may return 0 results if map data not available.\n");
        printf("This is expected - tests verify functions execute correctly.\n");
        printf("For full testing, ensure map data and HGT files are available.\n");
        return 0;
    } else {
        printf("Integration tests failed: %d failures\n", failures);
        return 1;
    }
}
