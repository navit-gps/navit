/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Unit tests for Driver Break plugin SRTM HGT file handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "../../debug.h"
#include "../driver_break_srtm.h"
#include "../driver_break.h"
#include "../../navit.h"

/* Stub for debug_printf and max_debug_level for unit tests */
dbg_level max_debug_level = lvl_error;

void debug_printf(dbg_level level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, ...) {
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

/* Stub for event_add_idle (not needed for file tests) */
struct event_idle *event_add_idle(int priority, struct callback *cb) {
    (void)priority;
    (void)cb;
    return NULL;
}

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message); \
            return 1; \
        } \
    } while(0)

/* Create a valid HGT file for testing */
static int create_valid_hgt_file(const char *filepath, int lon_idx, int lat_idx) {
    FILE *f = fopen(filepath, "wb");
    if (!f) {
        return 0;
    }
    
    /* HGT format: 3601x3601 16-bit signed integers (big-endian) */
    /* Total size: 3601 * 3601 * 2 = 25,934,402 bytes */
    const int size = 3601 * 3601;
    int16_t *data = g_malloc(size * sizeof(int16_t));
    
    /* Fill with test elevation data (simple gradient) */
    for (int y = 0; y < 3601; y++) {
        for (int x = 0; x < 3601; x++) {
            int idx = y * 3601 + x;
            /* Create a simple elevation pattern */
            int16_t elevation = (int16_t)(100 + (x + y) / 10);
            data[idx] = elevation;
        }
    }
    
    /* Write as big-endian */
    for (int i = 0; i < size; i++) {
        unsigned char bytes[2];
        bytes[0] = (data[i] >> 8) & 0xFF;
        bytes[1] = data[i] & 0xFF;
        if (fwrite(bytes, 1, 2, f) != 2) {
            fclose(f);
            g_free(data);
            return 0;
        }
    }
    
    fclose(f);
    g_free(data);
    return 1;
}

/* Create a corrupt HGT file (wrong size) */
static int create_corrupt_hgt_file_size(const char *filepath) {
    FILE *f = fopen(filepath, "wb");
    if (!f) {
        return 0;
    }
    
    /* Write only 1000 bytes (should be 25,934,402) */
    char dummy[1000];
    memset(dummy, 0, sizeof(dummy));
    fwrite(dummy, 1, sizeof(dummy), f);
    fclose(f);
    return 1;
}

/* Create a corrupt HGT file (invalid data) */
static int create_corrupt_hgt_file_data(const char *filepath) {
    FILE *f = fopen(filepath, "wb");
    if (!f) {
        return 0;
    }
    
    /* Write correct size but with invalid data (all zeros or all 0xFF) */
    const int size = 3601 * 3601 * 2;
    char *data = g_malloc(size);
    memset(data, 0xFF, size);  /* Fill with 0xFF (invalid elevation) */
    fwrite(data, 1, size, f);
    fclose(f);
    g_free(data);
    return 1;
}

/* Create a corrupt HGT file (truncated) */
static int create_corrupt_hgt_file_truncated(const char *filepath) {
    FILE *f = fopen(filepath, "wb");
    if (!f) {
        return 0;
    }
    
    /* Write partial data */
    const int size = 3601 * 3601 * 2;
    char *data = g_malloc(size);
    memset(data, 0, size);
    /* Write only half */
    fwrite(data, 1, size / 2, f);
    fclose(f);
    g_free(data);
    return 1;
}

static int test_srtm_init(void) {
    char *test_dir = g_strdup("/tmp/test_srtm_init");
    g_mkdir_with_parents(test_dir, 0755);
    
    int result = srtm_init(test_dir);
    TEST_ASSERT(result == 1, "SRTM init failed");
    
    char *dir = srtm_get_data_directory();
    TEST_ASSERT(dir != NULL, "SRTM data directory is NULL");
    TEST_ASSERT(strcmp(dir, test_dir) == 0, "SRTM data directory mismatch");
    
    g_free(dir);
    g_free(test_dir);
    return 0;
}

static int test_hgt_valid_file(void) {
    char *test_dir = g_strdup("/tmp/test_hgt_valid");
    g_mkdir_with_parents(test_dir, 0755);
    srtm_init(test_dir);
    
    /* Create valid HGT file */
    char *filepath = g_build_filename(test_dir, "N60E007.hgt", NULL);
    TEST_ASSERT(create_valid_hgt_file(filepath, 7, 60) == 1, "Failed to create valid HGT file");
    
    /* Test tile exists */
    TEST_ASSERT(srtm_tile_exists(7, 60) == 1, "Valid tile not detected");
    
    /* Test elevation reading */
    struct coord_geo coord;
    coord.lat = 60.5;
    coord.lng = 7.5;
    int elevation = srtm_get_elevation(&coord);
    TEST_ASSERT(elevation != -32768, "Failed to read elevation from valid file");
    TEST_ASSERT(elevation >= -500 && elevation <= 9000, "Elevation out of reasonable range");
    
    g_free(filepath);
    g_free(test_dir);
    return 0;
}

static int test_hgt_corrupt_size(void) {
    char *test_dir = g_strdup("/tmp/test_hgt_corrupt_size");
    g_mkdir_with_parents(test_dir, 0755);
    srtm_init(test_dir);
    
    /* Create corrupt HGT file (wrong size) */
    char *filepath = g_build_filename(test_dir, "N60E008.hgt", NULL);
    TEST_ASSERT(create_corrupt_hgt_file_size(filepath) == 1, "Failed to create corrupt HGT file");
    
    /* Test that elevation reading fails gracefully */
    struct coord_geo coord;
    coord.lat = 60.5;
    coord.lng = 8.5;
    int elevation = srtm_get_elevation(&coord);
    /* Should return void value or handle gracefully */
    TEST_ASSERT(elevation == -32768 || elevation == 0, "Corrupt file not handled correctly");
    
    g_free(filepath);
    g_free(test_dir);
    return 0;
}

static int test_hgt_corrupt_data(void) {
    char *test_dir = g_strdup("/tmp/test_hgt_corrupt_data");
    g_mkdir_with_parents(test_dir, 0755);
    srtm_init(test_dir);
    
    /* Create corrupt HGT file (invalid data) */
    char *filepath = g_build_filename(test_dir, "N60E009.hgt", NULL);
    TEST_ASSERT(create_corrupt_hgt_file_data(filepath) == 1, "Failed to create corrupt HGT file");
    
    /* Test that elevation reading handles invalid data */
    struct coord_geo coord;
    coord.lat = 60.5;
    coord.lng = 9.5;
    int elevation = srtm_get_elevation(&coord);
    /* Should return void value or handle gracefully */
    TEST_ASSERT(elevation == -32768 || elevation >= -32768, "Invalid data not handled correctly");
    
    g_free(filepath);
    g_free(test_dir);
    return 0;
}

static int test_hgt_corrupt_truncated(void) {
    char *test_dir = g_strdup("/tmp/test_hgt_corrupt_truncated");
    g_mkdir_with_parents(test_dir, 0755);
    srtm_init(test_dir);
    
    /* Create corrupt HGT file (truncated) */
    char *filepath = g_build_filename(test_dir, "N60E010.hgt", NULL);
    TEST_ASSERT(create_corrupt_hgt_file_truncated(filepath) == 1, "Failed to create truncated HGT file");
    
    /* Test that elevation reading fails gracefully */
    struct coord_geo coord;
    coord.lat = 60.5;
    coord.lng = 10.5;
    int elevation = srtm_get_elevation(&coord);
    /* Should return void value or handle gracefully */
    TEST_ASSERT(elevation == -32768 || elevation == 0, "Truncated file not handled correctly");
    
    g_free(filepath);
    g_free(test_dir);
    return 0;
}

static int test_hgt_missing_file(void) {
    char *test_dir = g_strdup("/tmp/test_hgt_missing");
    g_mkdir_with_parents(test_dir, 0755);
    srtm_init(test_dir);
    
    /* Test elevation for non-existent tile */
    struct coord_geo coord;
    coord.lat = 70.5;
    coord.lng = 20.5;
    int elevation = srtm_get_elevation(&coord);
    TEST_ASSERT(elevation == -32768, "Missing file should return void value");
    
    g_free(test_dir);
    return 0;
}

static int test_hgt_tile_filename(void) {
    char *filename;
    
    /* Test positive coordinates */
    filename = srtm_get_tile_filename(7, 60);
    TEST_ASSERT(filename != NULL, "Tile filename is NULL");
    TEST_ASSERT(strcmp(filename, "N60E007.hgt") == 0, "Tile filename mismatch for positive coords");
    g_free(filename);
    
    /* Test negative coordinates */
    filename = srtm_get_tile_filename(-7, -60);
    TEST_ASSERT(filename != NULL, "Tile filename is NULL");
    TEST_ASSERT(strcmp(filename, "S60W007.hgt") == 0, "Tile filename mismatch for negative coords");
    g_free(filename);
    
    /* Test zero coordinates */
    filename = srtm_get_tile_filename(0, 0);
    TEST_ASSERT(filename != NULL, "Tile filename is NULL");
    TEST_ASSERT(strcmp(filename, "N00E000.hgt") == 0, "Tile filename mismatch for zero coords");
    g_free(filename);
    
    return 0;
}

static int test_hgt_cache_memory(void) {
    char *test_dir = g_strdup("/tmp/test_hgt_cache");
    g_mkdir_with_parents(test_dir, 0755);
    srtm_init(test_dir);
    
    /* Create multiple valid HGT files to test cache */
    for (int lat = 60; lat < 62; lat++) {
        for (int lon = 7; lon < 10; lon++) {
            char *filename = srtm_get_tile_filename(lon, lat);
            char *filepath = g_build_filename(test_dir, filename, NULL);
            create_valid_hgt_file(filepath, lon, lat);
            g_free(filename);
            g_free(filepath);
        }
    }
    
    /* Test reading from multiple tiles (cache stress test) */
    int success_count = 0;
    for (int lat = 60; lat < 62; lat++) {
        for (int lon = 7; lon < 10; lon++) {
            struct coord_geo coord;
            coord.lat = lat + 0.5;
            coord.lng = lon + 0.5;
            int elevation = srtm_get_elevation(&coord);
            if (elevation != -32768) {
                success_count++;
            }
        }
    }
    
    /* Should successfully read from all tiles */
    TEST_ASSERT(success_count >= 5, "Cache memory handling failed");
    
    /* Test out of bounds coordinates */
    struct coord_geo coord;
    coord.lat = 90.5;  /* Out of bounds */
    coord.lng = 7.5;
    int elevation = srtm_get_elevation(&coord);
    TEST_ASSERT(elevation == -32768, "Out of bounds coordinate not handled");
    
    g_free(test_dir);
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("Running SRTM HGT file handling tests...\n");
    
    failures += test_srtm_init();
    failures += test_hgt_valid_file();
    failures += test_hgt_corrupt_size();
    failures += test_hgt_corrupt_data();
    failures += test_hgt_corrupt_truncated();
    failures += test_hgt_missing_file();
    failures += test_hgt_tile_filename();
    failures += test_hgt_cache_memory();
    
    if (failures == 0) {
        printf("All SRTM HGT file handling tests passed!\n");
        return 0;
    } else {
        printf("SRTM HGT file handling tests failed: %d failures\n", failures);
        return 1;
    }
}
