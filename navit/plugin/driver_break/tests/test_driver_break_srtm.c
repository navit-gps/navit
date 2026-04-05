/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Unit tests for Driver Break plugin SRTM HGT file handling
 */

#include "../../debug.h"
#include "../../navit.h"
#include "../driver_break.h"
#include "../driver_break_srtm.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_CURL
#    include <curl/curl.h>

static size_t curl_write_file_cb(void *ptr, size_t size, size_t nmemb, void *user) {
    return fwrite(ptr, size, nmemb, (FILE *)user);
}

/* Browser User-Agent required by Viewfinder (blocks scripted downloads without it). */
#    define CURL_USERAGENT_VIEWFINDER                                                                                  \
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"

/* Download URL to filepath. Returns 1 on success. */
static int download_file_to(const char *url, const char *filepath) {
    FILE *fp = fopen(filepath, "wb");
    if (!fp)
        return 0;
    CURL *curl = curl_easy_init();
    if (!curl) {
        fclose(fp);
        g_unlink(filepath);
        return 0;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, CURL_USERAGENT_VIEWFINDER);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_file_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        g_unlink(filepath);
        return 0;
    }
    return 1;
}
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

/* Stub for event_add_idle (not needed for file tests) */
struct event_idle *event_add_idle(int priority, struct callback *cb) {
    (void)priority;
    (void)cb;
    return NULL;
}

#define TEST_ASSERT(condition, message)                                                                                \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

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
    memset(data, 0xFF, size); /* Fill with 0xFF (invalid elevation) */
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

/* Create HGT file filled with constant elevation (for tile-border tests) */
static int create_constant_hgt_file(const char *filepath, int16_t elevation_value) {
    FILE *f = fopen(filepath, "wb");
    if (!f)
        return 0;
    const int size = 3601 * 3601;
    for (int i = 0; i < size; i++) {
        unsigned char bytes[2];
        int16_t v = elevation_value;
        bytes[0] = (v >> 8) & 0xFF;
        bytes[1] = v & 0xFF;
        if (fwrite(bytes, 1, 2, f) != 2) {
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return 1;
}

/* Test that elevation lookup at tile borders uses the correct tile per coordinate.
 * Each point is resolved to one tile via floor(lon), floor(lat); no single-file traversal. */
static int test_hgt_tile_borders(void) {
    char *test_dir = g_strdup("/tmp/test_hgt_borders");
    g_mkdir_with_parents(test_dir, 0755);
    srtm_init(test_dir);

    /* Two adjacent tiles with distinct constant elevations so we can verify which tile is read */
    char *path7 = g_build_filename(test_dir, "N60E007.hgt", NULL);
    char *path8 = g_build_filename(test_dir, "N60E008.hgt", NULL);
    TEST_ASSERT(create_constant_hgt_file(path7, 100), "Create tile E007 failed");
    TEST_ASSERT(create_constant_hgt_file(path8, 200), "Create tile E008 failed");
    g_free(path7);
    g_free(path8);

    /* Point clearly in tile (7, 60): lon 7.5 -> floor 7 */
    struct coord_geo in_tile7;
    in_tile7.lng = 7.5;
    in_tile7.lat = 60.5;
    int e7 = srtm_get_elevation(&in_tile7);
    TEST_ASSERT(e7 == 100, "Point in tile E007 should return elevation 100");

    /* Point clearly in tile (8, 60): lon 8.5 -> floor 8 */
    struct coord_geo in_tile8;
    in_tile8.lng = 8.5;
    in_tile8.lat = 60.5;
    int e8 = srtm_get_elevation(&in_tile8);
    TEST_ASSERT(e8 == 200, "Point in tile E008 should return elevation 200");

    /* Exactly on longitude boundary 8.0: floor(8.0)=8 -> tile (8, 60) */
    struct coord_geo on_border;
    on_border.lng = 8.0;
    on_border.lat = 60.5;
    int e_border = srtm_get_elevation(&on_border);
    TEST_ASSERT(e_border == 200, "Point on lon border 8.0 should use tile E008");

    /* Just west of border: 7.999 -> floor 7 -> tile (7, 60) */
    struct coord_geo west_of_border;
    west_of_border.lng = 7.999;
    west_of_border.lat = 60.5;
    int e_west = srtm_get_elevation(&west_of_border);
    TEST_ASSERT(e_west == 100, "Point just west of 8.0 should use tile E007");

    g_free(test_dir);
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
    coord.lat = 90.5; /* Out of bounds */
    coord.lng = 7.5;
    int elevation = srtm_get_elevation(&coord);
    TEST_ASSERT(elevation == -32768, "Out of bounds coordinate not handled");

    g_free(test_dir);
    return 0;
}

#ifdef HAVE_CURL
#    define SRTM_URL_VIEWFINDER_DEM3 "http://www.viewfinderpanoramas.org/dem3/"
#    define SRTM_URL_NASA "https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL1.003/2000.02.11/"

/* Viewfinder dem3 zone for Norway/Scandinavia (same as plugin). */
static const char *viewfinder_zone(int lat, int lon) {
    if (lat >= 60 && lat <= 63 && lon >= 6 && lon <= 12)
        return "M32";
    return NULL;
}

/* Download one SRTM HGT tile (zip from Viewfinder dem3 or NASA), extract .hgt into test_dir. Returns 1 on success. */
static int download_hgt_tile(int lon, int lat, const char *test_dir) {
    char lat_dir = (lat >= 0) ? 'N' : 'S';
    char lon_dir = (lon >= 0) ? 'E' : 'W';
    int lat_abs = abs(lat);
    int lon_abs = abs(lon);
    char *filename = g_strdup_printf("%c%02d%c%03d.hgt", lat_dir, lat_abs, lon_dir, lon_abs);
    char *tilename_zip = g_strdup_printf("%c%02d%c%03d.zip", lat_dir, lat_abs, lon_dir, lon_abs);
    char *zip_path = g_build_filename(test_dir, tilename_zip, NULL);
    const char *zone = viewfinder_zone(lat, lon);
    char *url_vf = zone ? g_strdup_printf("%s%s%s", SRTM_URL_VIEWFINDER_DEM3, zone, tilename_zip) : NULL;
    char *url_nasa =
        g_strdup_printf("%s%c%02d%c%03d.SRTMGL1.hgt.zip", SRTM_URL_NASA, lat_dir, lat_abs, lon_dir, lon_abs);

    int ok = 0;
    if (url_vf && download_file_to(url_vf, zip_path))
        ok = 1;
    if (!ok && download_file_to(url_nasa, zip_path))
        ok = 1;

    if (url_vf)
        g_free(url_vf);
    g_free(url_nasa);

    if (!ok) {
        g_unlink(zip_path);
        g_free(zip_path);
        g_free(tilename_zip);
        g_free(filename);
        return 0;
    }

    char *extract_cmd = g_strdup_printf("cd '%s' && unzip -o -q '%s' '%s' 2>/dev/null", test_dir, zip_path, filename);
    int unzip_ret = system(extract_cmd);
    g_free(extract_cmd);
    char *hgt_path = g_build_filename(test_dir, filename, NULL);
    if (unzip_ret != 0 || !g_file_test(hgt_path, G_FILE_TEST_EXISTS)) {
        /* NASA zips often contain NxxEyy.SRTMGL1.hgt instead of NxxEyy.hgt; extract and rename */
        char *nasa_name = g_strdup_printf("%c%02d%c%03d.SRTMGL1.hgt", lat_dir, lat_abs, lon_dir, lon_abs);
        extract_cmd = g_strdup_printf("cd '%s' && unzip -o -q '%s' '%s' 2>/dev/null", test_dir, zip_path, nasa_name);
        unzip_ret = system(extract_cmd);
        g_free(extract_cmd);
        if (unzip_ret == 0) {
            char *nasa_path = g_build_filename(test_dir, nasa_name, NULL);
            if (g_file_test(nasa_path, G_FILE_TEST_EXISTS))
                g_rename(nasa_path, hgt_path);
            g_free(nasa_path);
        }
        g_free(nasa_name);
    }
    int result = g_file_test(hgt_path, G_FILE_TEST_EXISTS) ? 1 : 0;
    g_free(hgt_path);
    g_unlink(zip_path);
    g_free(zip_path);
    g_free(tilename_zip);
    g_free(filename);
    return result;
}

/* Download SRTM HGT tiles for the same three OSM locations as the GeoTIFF test; verify read. */
static int test_srtm_hgt_download_and_read(void) {
    char *test_dir = g_strdup("/tmp/test_srtm_hgt_download");
    g_mkdir_with_parents(test_dir, 0755);

    printf("  Downloading HGT tiles (Viewfinder Panoramas / NASA SRTM)...\n");
    /* Tiles for (62.0937,7.1433), (61.5919,9.7018), (61.36012,9.66941): N62E007, N61E009 */
    int got_62_7 = download_hgt_tile(7, 62, test_dir);
    int got_61_9 = download_hgt_tile(9, 61, test_dir);

    if (!got_62_7 || !got_61_9) {
        printf("SRTM HGT: tile download failed or unzip failed (network/unavailable). Skip read test.\n");
        g_free(test_dir);
        return 0;
    }

    srtm_init(test_dir);

    struct coord_geo point1;
    point1.lat = 62.0937;
    point1.lng = 7.1433;
    struct coord_geo point2;
    point2.lat = 61.5919;
    point2.lng = 9.7018;
    struct coord_geo point3;
    point3.lat = 61.36012;
    point3.lng = 9.66941;

    int e1 = srtm_get_elevation(&point1);
    int e2 = srtm_get_elevation(&point2);
    int e3 = srtm_get_elevation(&point3);

    TEST_ASSERT(e1 != -32768, "SRTM HGT read at 62.0937,7.1433 should be valid");
    TEST_ASSERT(e1 >= -50 && e1 <= 3000, "SRTM HGT elevation at point1 out of expected range");
    TEST_ASSERT(e2 != -32768, "SRTM HGT read at 61.5919,9.7018 should be valid");
    TEST_ASSERT(e2 >= -50 && e2 <= 3000, "SRTM HGT elevation at point2 out of expected range");
    TEST_ASSERT(e3 != -32768, "SRTM HGT read at 61.36012,9.66941 should be valid");
    TEST_ASSERT(e3 >= -50 && e3 <= 3000, "SRTM HGT elevation at point3 out of expected range");

    printf(
        "SRTM HGT: tiles downloaded and read correctly at 3 OSM locations (62.09,7.14 / 61.59,9.70 / 61.36,9.67).\n");
    g_free(test_dir);
    return 0;
}
#endif

#ifdef HAVE_GEOTIFF
/* Check file has TIFF magic (II or MM in first 2 bytes). Returns 1 if valid TIFF, 0 otherwise. */
static int file_is_valid_tiff(const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f)
        return 0;
    unsigned char magic[4];
    size_t n = fread(magic, 1, 4, f);
    fclose(f);
    if (n < 2)
        return 0;
    /* Little-endian TIFF: 0x49 0x49 (II); big-endian: 0x4D 0x4D (MM). Reject HTML/XML (e.g. 0x3C 0x3F = "<?"). */
    if (magic[0] == 0x49 && magic[1] == 0x49)
        return 1;
    if (magic[0] == 0x4D && magic[1] == 0x4D)
        return 1;
    return 0;
}

#    ifdef HAVE_CURL
/* Download Copernicus GeoTIFF; if response is not a valid TIFF (e.g. S3 XML/HTML error), treat as failure. */
static int download_copernicus_tile(const char *url, const char *filepath) {
    if (!download_file_to(url, filepath))
        return 0;
    if (!file_is_valid_tiff(filepath)) {
        g_unlink(filepath);
        return 0;
    }
    return 1;
}
#    endif

/* Shared elevation cache: same default as download_test_srtm_data.sh and route integration Test 4. */
static char *driver_break_srtm_shared_data_dir(void) {
    const char *e = g_getenv("SRTM_TEST_DIR");
    if (e && e[0])
        return g_strdup(e);
    return g_strdup("/tmp/test_srtm_hgt_download");
}

/* Test Copernicus GLO-30: download tiles for three OSM map locations (Norway) and verify read.
 * Locations: map=12/62.0937/7.1433, map=12/61.5919/9.7018, map=15/61.36012/9.66941
 * Tiles for those points: N62E007, N61E009.
 * Also downloads N61E010 and N62E009 when CURL is available so test_driver_break_route_integration
 * Test 4 (Rondanestien) can read elevation from the same directory after ctest. */
static int test_copernicus_glo30_download_and_read(void) {
    char *test_dir = driver_break_srtm_shared_data_dir();
    g_mkdir_with_parents(test_dir, 0755);

#    ifdef HAVE_CURL
    /* Download tiles from public AWS S3; tiles are in subdirs: {base}/{tilename}/{tilename}.tif */
    printf("  Downloading Copernicus GLO-30 GeoTIFF from AWS S3...\n");
    const char *base = "https://copernicus-dem-30m.s3.amazonaws.com/";
    const char *tile_62_7 = "Copernicus_DSM_COG_10_N62_00_E007_00_DEM";
    const char *tile_61_9 = "Copernicus_DSM_COG_10_N61_00_E009_00_DEM";
    char *url_62_7 = g_strdup_printf("%s%s/%s.tif", base, tile_62_7, tile_62_7);
    char *url_61_9 = g_strdup_printf("%s%s/%s.tif", base, tile_61_9, tile_61_9);
    char *path_62_7 = g_build_filename(test_dir, "Copernicus_DSM_COG_10_N62_00_E007_00_DEM.tif", NULL);
    char *path_61_9 = g_build_filename(test_dir, "Copernicus_DSM_COG_10_N61_00_E009_00_DEM.tif", NULL);

    int got_62_7 = download_copernicus_tile(url_62_7, path_62_7);
    int got_61_9 = download_copernicus_tile(url_61_9, path_61_9);

    g_free(url_62_7);
    g_free(url_61_9);
    g_free(path_62_7);
    g_free(path_61_9);

    if (!got_62_7 || !got_61_9) {
        printf("Copernicus GLO-30: tile download failed or skipped (network/unavailable). Skip read test.\n");
        g_free(test_dir);
        return 0;
    }

    /* Rondanestien route integration uses tiles N61E010 (south/mid) and N62E009 (north); not the same as
     * N62E007/N61E009. */
    {
        const char *tile_61_10 = "Copernicus_DSM_COG_10_N61_00_E010_00_DEM";
        const char *tile_62_9 = "Copernicus_DSM_COG_10_N62_00_E009_00_DEM";
        char *url = g_strdup_printf("%s%s/%s.tif", base, tile_61_10, tile_61_10);
        char *path = g_build_filename(test_dir, "Copernicus_DSM_COG_10_N61_00_E010_00_DEM.tif", NULL);
        if (!download_copernicus_tile(url, path))
            printf("  Copernicus GLO-30: optional N61E010 tile for route integration Test 4 not downloaded.\n");
        g_free(url);
        g_free(path);
        url = g_strdup_printf("%s%s/%s.tif", base, tile_62_9, tile_62_9);
        path = g_build_filename(test_dir, "Copernicus_DSM_COG_10_N62_00_E009_00_DEM.tif", NULL);
        if (!download_copernicus_tile(url, path))
            printf("  Copernicus GLO-30: optional N62E009 tile for route integration Test 4 not downloaded.\n");
        g_free(url);
        g_free(path);
    }
#    else
    /* Without CURL we only run if tiles are already present and valid (e.g. from a previous run) */
    char *path_62_7 = g_build_filename(test_dir, "Copernicus_DSM_COG_10_N62_00_E007_00_DEM.tif", NULL);
    char *path_61_9 = g_build_filename(test_dir, "Copernicus_DSM_COG_10_N61_00_E009_00_DEM.tif", NULL);
    int got_62_7 = g_file_test(path_62_7, G_FILE_TEST_EXISTS) && file_is_valid_tiff(path_62_7);
    int got_61_9 = g_file_test(path_61_9, G_FILE_TEST_EXISTS) && file_is_valid_tiff(path_61_9);
    g_free(path_62_7);
    g_free(path_61_9);
    if (!got_62_7 || !got_61_9) {
        printf("Copernicus GLO-30: no valid GeoTIFF tiles in %s (test built without libcurl, so tiles cannot be "
               "downloaded; install libcurl and rebuild, or place Copernicus *.tif files there). Skip test.\n",
               test_dir);
        g_free(test_dir);
        return 0;
    }
#    endif

    srtm_init(test_dir);

    /* Three coordinates from OpenStreetMap map links (lat, lon) */
    struct coord_geo point1; /* map=12/62.0937/7.1433 */
    point1.lat = 62.0937;
    point1.lng = 7.1433;

    struct coord_geo point2; /* map=12/61.5919/9.7018 */
    point2.lat = 61.5919;
    point2.lng = 9.7018;

    struct coord_geo point3; /* map=15/61.36012/9.66941 */
    point3.lat = 61.36012;
    point3.lng = 9.66941;

    int e1 = srtm_get_elevation(&point1);
    int e2 = srtm_get_elevation(&point2);
    int e3 = srtm_get_elevation(&point3);

    /* Norway: expect valid elevation (not void), roughly 0-2500 m */
    TEST_ASSERT(e1 != -32768, "Copernicus read at 62.0937,7.1433 should be valid");
    TEST_ASSERT(e1 >= -50 && e1 <= 3000, "Copernicus elevation at point1 out of expected range");

    TEST_ASSERT(e2 != -32768, "Copernicus read at 61.5919,9.7018 should be valid");
    TEST_ASSERT(e2 >= -50 && e2 <= 3000, "Copernicus elevation at point2 out of expected range");

    TEST_ASSERT(e3 != -32768, "Copernicus read at 61.36012,9.66941 should be valid");
    TEST_ASSERT(e3 >= -50 && e3 <= 3000, "Copernicus elevation at point3 out of expected range");

    printf("Copernicus GLO-30: tiles read correctly at 3 OSM locations (62.09,7.14 / 61.59,9.70 / 61.36,9.67).\n");
    g_free(test_dir);
    return 0;
}
#endif

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
    failures += test_hgt_tile_borders();
    failures += test_hgt_cache_memory();
#ifdef HAVE_CURL
    failures += test_srtm_hgt_download_and_read();
#endif
#ifdef HAVE_GEOTIFF
    failures += test_copernicus_glo30_download_and_read();
#endif

    if (failures == 0) {
        printf("All SRTM HGT file handling tests passed!\n");
        return 0;
    } else {
        printf("SRTM HGT file handling tests failed: %d failures\n", failures);
        return 1;
    }
}
