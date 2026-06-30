/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * SRTM tile indexing, region metadata, and cache bookkeeping.
 */

#include "config.h"
#include "debug.h"
#include "driver_break_srtm.h"
#include "driver_break_srtm_priv.h"
#include "file.h"
#include "navit.h"
#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_GEOTIFF
#    include <stdarg.h>
#    include <tiffio.h>

static void srtm_tiff_warning_suppress(const char *module, const char *fmt, va_list ap) {
    (void)module;
    (void)fmt;
    (void)ap;
}
#endif

char *srtm_data_dir = NULL;

int srtm_init(const char *srtm_dir) {
    char *new_dir = NULL;

    if (srtm_data_dir) {
        g_free(srtm_data_dir);
        srtm_data_dir = NULL;
    }

    if (srtm_dir) {
        new_dir = g_strdup(srtm_dir);
    } else {
        const char *user_data_dir = navit_get_user_data_directory(1);
        if (!user_data_dir) {
            dbg(lvl_error, "SRTM: Cannot get user data directory");
            return 0;
        }
        new_dir = g_build_filename(user_data_dir, "srtm", NULL);
    }

    if (!new_dir) {
        dbg(lvl_error, "SRTM: Failed to allocate directory path");
        return 0;
    }

    if (!file_is_dir(new_dir)) {
        if (file_mkdir(new_dir, 1) != 0) {
            dbg(lvl_error, "SRTM: Failed to create directory %s", new_dir);
            g_free(new_dir);
            return 0;
        }
    }

    srtm_data_dir = new_dir;
#ifdef HAVE_GEOTIFF
    TIFFSetWarningHandler(srtm_tiff_warning_suppress);
#endif
    dbg(lvl_info, "SRTM: Initialized with directory %s", srtm_data_dir);
    return 1;
}

char *srtm_get_data_directory(void) {
    if (!srtm_data_dir) {
        srtm_init(NULL);
    }
    return g_strdup(srtm_data_dir);
}

char *srtm_get_tile_filename(int lon, int lat) {
    char lon_dir = (lon >= 0) ? 'E' : 'W';
    char lat_dir = (lat >= 0) ? 'N' : 'S';
    int lon_abs = abs(lon);
    int lat_abs = abs(lat);

    return g_strdup_printf("%c%02d%c%03d.hgt", lat_dir, lat_abs, lon_dir, lon_abs);
}

const char *srtm_viewfinder_zone(int lat, int lon) {
    if (lat >= 60 && lat <= 63 && lon >= 6 && lon <= 12)
        return "M32";
    return NULL;
}

#ifdef HAVE_GEOTIFF
char *srtm_get_geotiff_tile_filename(int lon, int lat) {
    char lon_dir = (lon >= 0) ? 'E' : 'W';
    char lat_dir = (lat >= 0) ? 'N' : 'S';
    int lon_abs = abs(lon);
    int lat_abs = abs(lat);

    return g_strdup_printf("Copernicus_DSM_COG_10_%c%02d_00_%c%03d_00_DEM.tif", lat_dir, lat_abs, lon_dir, lon_abs);
}
#endif

int srtm_tile_exists(int lon, int lat) {
#ifdef HAVE_GEOTIFF
    char *geotiff_name = srtm_get_geotiff_tile_filename(lon, lat);
    char *geotiff_path = g_build_filename(srtm_data_dir, geotiff_name, NULL);
    int exists_gt = g_file_test(geotiff_path, G_FILE_TEST_EXISTS);
    g_free(geotiff_name);
    g_free(geotiff_path);
    if (exists_gt)
        return 1;
#endif
    {
        char *filename = srtm_get_tile_filename(lon, lat);
        char *filepath = g_build_filename(srtm_data_dir, filename, NULL);
        int exists = g_file_test(filepath, G_FILE_TEST_EXISTS);
        g_free(filename);
        g_free(filepath);
        return exists;
    }
}

static struct srtm_tile *srtm_tile_new(int lon, int lat) {
    struct srtm_tile *tile = g_new0(struct srtm_tile, 1);
    char lon_dir = (lon >= 0) ? 'E' : 'W';
    char lat_dir = (lat >= 0) ? 'N' : 'S';
    int lon_abs = abs(lon);
    int lat_abs = abs(lat);
    const char *vf_zone = srtm_viewfinder_zone(lat, lon);
    char *vf_zip_url = vf_zone ? g_strdup_printf("%s%s/%c%02d%c%03d.zip", SRTM_URL_VIEWFINDER_DEM3, vf_zone, lat_dir,
                                                 lat_abs, lon_dir, lon_abs)
                               : NULL;

    tile->lon = lon;
    tile->lat = lat;
    tile->filename = srtm_get_tile_filename(lon, lat);
    tile->size_bytes = SRTM_TILE_SIZE_BYTES;
#ifdef HAVE_GEOTIFF
    tile->filename_geotiff = srtm_get_geotiff_tile_filename(lon, lat);
    tile->url_primary = g_strdup_printf(
        "%sCopernicus_DSM_COG_10_%c%02d_00_%c%03d_00_DEM/Copernicus_DSM_COG_10_%c%02d_00_%c%03d_00_DEM.tif",
        SRTM_URL_COPERNICUS, lat_dir, lat_abs, lon_dir, lon_abs, lat_dir, lat_abs, lon_dir, lon_abs);
    tile->url_fallback = vf_zip_url;
    tile->url_fallback2 =
        g_strdup_printf("%s%c%02d%c%03d.SRTMGL1.hgt.zip", SRTM_URL_NASA, lat_dir, lat_abs, lon_dir, lon_abs);
#else
    tile->url_primary = vf_zip_url ? vf_zip_url
                                   : g_strdup_printf("%s%c%02d%c%03d.SRTMGL1.hgt.zip", SRTM_URL_NASA, lat_dir, lat_abs,
                                                     lon_dir, lon_abs);
    tile->url_fallback = vf_zip_url ? g_strdup_printf("%s%c%02d%c%03d.SRTMGL1.hgt.zip", SRTM_URL_NASA, lat_dir, lat_abs,
                                                      lon_dir, lon_abs)
                                    : NULL;
    tile->url_fallback2 = NULL;
#endif
    tile->downloaded = srtm_tile_exists(lon, lat);
    return tile;
}

static int srtm_tile_list_contains(GList *tiles, int lon, int lat) {
    GList *l = tiles;
    while (l) {
        struct srtm_tile *t = (struct srtm_tile *)l->data;
        if (t->lon == lon && t->lat == lat)
            return 1;
        l = g_list_next(l);
    }
    return 0;
}

GList *srtm_calculate_tiles(double min_lon, double min_lat, double max_lon, double max_lat) {
    GList *tiles = NULL;
    int min_lon_idx = (int)floor(min_lon);
    int max_lon_idx = (int)floor(max_lon);
    int min_lat_idx = (int)floor(min_lat);
    int max_lat_idx = (int)floor(max_lat);
    int lat;
    int lon;

    for (lat = min_lat_idx; lat <= max_lat_idx; lat++) {
        for (lon = min_lon_idx; lon <= max_lon_idx; lon++) {
            if (!srtm_tile_list_contains(tiles, lon, lat))
                tiles = g_list_append(tiles, srtm_tile_new(lon, lat));
        }
    }

    return tiles;
}

static struct srtm_region srtm_regions[] = {
    {"Europe",         -12.97, 33.59, 34.15, 72.10, 0, 0, 0, 0},
    {"Germany",        5.18,   46.84, 15.47, 55.64, 0, 0, 0, 0},
    {"France",         -5.5,   41.0,  9.5,   51.5,  0, 0, 0, 0},
    {"United Kingdom", -11.17, 49.5,  2.0,   61.0,  0, 0, 0, 0},
    {"Italy",          6.0,    35.5,  19.0,  47.5,  0, 0, 0, 0},
    {"Spain",          -10.0,  35.0,  5.0,   44.5,  0, 0, 0, 0},
    {"Norway",         4.0,    57.0,  32.0,  81.0,  0, 0, 0, 0},
    {"Sweden",         10.0,   55.0,  25.0,  69.0,  0, 0, 0, 0},
    {"Poland",         14.0,   48.5,  25.0,  55.0,  0, 0, 0, 0},
    {"United States",  -180.0, 18.0,  -66.0, 72.0,  0, 0, 0, 0},
    {"China",          67.3,   5.3,   135.0, 54.5,  0, 0, 0, 0},
    {"Japan",          123.6,  25.2,  151.3, 47.1,  0, 0, 0, 0},
    {NULL,             0,      0,     0,     0,     0, 0, 0, 0}
};

static void srtm_region_fill_stats(struct srtm_region *region) {
    GList *tiles =
        srtm_calculate_tiles(region->bbox_min_lon, region->bbox_min_lat, region->bbox_max_lon, region->bbox_max_lat);
    int downloaded_count = 0;
    GList *l = tiles;

    region->tile_count = g_list_length(tiles);
    region->size_bytes = 0;
    while (l) {
        struct srtm_tile *tile = (struct srtm_tile *)l->data;
        region->size_bytes += tile->size_bytes;
        if (tile->downloaded)
            downloaded_count++;
        l = g_list_next(l);
    }

    region->downloaded = (downloaded_count == region->tile_count) ? 1 : 0;
    region->progress_percent = region->tile_count > 0 ? (downloaded_count * 100 / region->tile_count) : 0;
    srtm_free_tiles(tiles);
}

GList *srtm_get_regions(void) {
    GList *regions = NULL;
    int i;

    for (i = 0; srtm_regions[i].name; i++) {
        struct srtm_region *region = g_new0(struct srtm_region, 1);
        region->name = g_strdup(srtm_regions[i].name);
        region->bbox_min_lon = srtm_regions[i].bbox_min_lon;
        region->bbox_min_lat = srtm_regions[i].bbox_min_lat;
        region->bbox_max_lon = srtm_regions[i].bbox_max_lon;
        region->bbox_max_lat = srtm_regions[i].bbox_max_lat;
        srtm_region_fill_stats(region);
        regions = g_list_append(regions, region);
    }

    return regions;
}

struct srtm_region *srtm_get_region(const char *name) {
    GList *regions = srtm_get_regions();
    GList *l = regions;

    while (l) {
        struct srtm_region *region = (struct srtm_region *)l->data;
        if (!strcmp(region->name, name)) {
            struct srtm_region *result = g_new0(struct srtm_region, 1);
            *result = *region;
            result->name = g_strdup(region->name);
            srtm_free_regions(regions);
            return result;
        }
        l = g_list_next(l);
    }

    srtm_free_regions(regions);
    return NULL;
}

void srtm_free_regions(GList *regions) {
    GList *l = regions;
    while (l) {
        struct srtm_region *region = (struct srtm_region *)l->data;
        if (region) {
            g_free(region->name);
            g_free(region);
        }
        l = g_list_next(l);
    }
    g_list_free(regions);
}

void srtm_free_tiles(GList *tiles) {
    GList *l = tiles;
    while (l) {
        struct srtm_tile *tile = (struct srtm_tile *)l->data;
        if (tile) {
            g_free(tile->filename);
            g_free(tile->filename_geotiff);
            g_free(tile->url_primary);
            g_free(tile->url_fallback);
            g_free(tile->url_fallback2);
            g_free(tile->checksum_md5);
            g_free(tile);
        }
        l = g_list_next(l);
    }
    g_list_free(tiles);
}

void srtm_free_download_context(struct srtm_download_context *ctx) {
    if (ctx) {
        srtm_free_tiles(ctx->tiles);
        g_free(ctx->error_message);
        g_free(ctx);
    }
}
