/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "debug.h"
#include "file.h"
#include "driver_break_srtm.h"
#include "navit.h"
#include "event.h"
#include "callback.h"

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

/* SRTM data directory */
static char *srtm_data_dir = NULL;

/* Viewfinder Panoramas base URL (void-filled 1 arc-second) */
#define SRTM_URL_VIEWFINDER "https://viewfinderpanoramas.org/data/SRTM1v3.0/SRTM1/"
/* NASA SRTMGL1 fallback URL */
#define SRTM_URL_NASA "https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL1.003/2000.02.11/"

/* HGT file size: 1 arc-second = 3601x3601 pixels = 25,934,402 bytes */
#define SRTM_TILE_SIZE_BYTES 25934402
#define SRTM_TILE_SIZE_COMPRESSED 2800000  /* ~2.8 MB compressed */

/* Initialize SRTM system */
int srtm_init(const char *srtm_dir) {
    char *new_dir = NULL;
    
    /* Free old directory if already initialized */
    if (srtm_data_dir) {
        g_free(srtm_data_dir);
        srtm_data_dir = NULL;
    }
    
    if (srtm_dir) {
        new_dir = g_strdup(srtm_dir);
    } else {
        /* Use default: ~/.navit/srtm/ */
        const char *user_data_dir = navit_get_user_data_directory(1);
        if (!user_data_dir) {
            dbg(lvl_error, "SRTM: Cannot get user data directory");
            return 0;
        }
        new_dir = g_build_filename(user_data_dir, "srtm", NULL);
        /* Note: user_data_dir is from getenv(), do not free */
    }
    
    if (!new_dir) {
        dbg(lvl_error, "SRTM: Failed to allocate directory path");
        return 0;
    }
    
    /* Create directory if it doesn't exist */
    if (!file_is_dir(new_dir)) {
        if (file_mkdir(new_dir, 1) != 0) {
            dbg(lvl_error, "SRTM: Failed to create directory %s", new_dir);
            g_free(new_dir);
            return 0;
        }
    }
    
    srtm_data_dir = new_dir;
    dbg(lvl_info, "SRTM: Initialized with directory %s", srtm_data_dir);
    return 1;
}

/* Get SRTM data directory */
char *srtm_get_data_directory(void) {
    if (!srtm_data_dir) {
        srtm_init(NULL);
    }
    return g_strdup(srtm_data_dir);
}

/* Get tile filename from lon/lat indices */
char *srtm_get_tile_filename(int lon, int lat) {
    char *filename;
    char lon_dir = (lon >= 0) ? 'E' : 'W';
    char lat_dir = (lat >= 0) ? 'N' : 'S';
    int lon_abs = abs(lon);
    int lat_abs = abs(lat);
    
    filename = g_strdup_printf("%c%02d%c%03d.hgt", lat_dir, lat_abs, lon_dir, lon_abs);
    return filename;
}

/* Check if tile file exists */
int srtm_tile_exists(int lon, int lat) {
    char *filename = srtm_get_tile_filename(lon, lat);
    char *filepath = g_build_filename(srtm_data_dir, filename, NULL);
    int exists = g_file_test(filepath, G_FILE_TEST_EXISTS);
    
    g_free(filename);
    g_free(filepath);
    return exists;
}

/* Calculate tiles needed for bounding box */
GList *srtm_calculate_tiles(double min_lon, double min_lat, 
                            double max_lon, double max_lat) {
    GList *tiles = NULL;
    
    /* Convert bounding box to tile indices */
    /* SRTM tiles are 1°×1° */
    /* Use floor to ensure we get the correct tile index for negative coordinates */
    int min_lon_idx = (int)floor(min_lon);
    int max_lon_idx = (int)floor(max_lon);
    int min_lat_idx = (int)floor(min_lat);
    int max_lat_idx = (int)floor(max_lat);
    
    /* Iterate through all tiles in bounding box */
    int lat;
    int lon;
    for (lat = min_lat_idx; lat <= max_lat_idx; lat++) {
        for (lon = min_lon_idx; lon <= max_lon_idx; lon++) {
            /* Skip if tile already in list */
            int found = 0;
            GList *l = tiles;
            while (l) {
                struct srtm_tile *t = (struct srtm_tile *)l->data;
                if (t->lon == lon && t->lat == lat) {
                    found = 1;
                    break;
                }
                l = g_list_next(l);
            }
            
            if (!found) {
                struct srtm_tile *tile = g_new0(struct srtm_tile, 1);
                tile->lon = lon;
                tile->lat = lat;
                tile->filename = srtm_get_tile_filename(lon, lat);
                
                /* Build URLs */
                char lon_dir = (lon >= 0) ? 'E' : 'W';
                char lat_dir = (lat >= 0) ? 'N' : 'S';
                int lon_abs = abs(lon);
                int lat_abs = abs(lat);
                
                /* Viewfinder Panoramas URL */
                tile->url_primary = g_strdup_printf("%s%c%02d%c%03d.hgt.zip",
                                                   SRTM_URL_VIEWFINDER,
                                                   lat_dir, lat_abs, lon_dir, lon_abs);
                
                /* NASA fallback URL (different format) */
                tile->url_fallback = g_strdup_printf("%s%c%02d%c%03d.SRTMGL1.hgt.zip",
                                                     SRTM_URL_NASA,
                                                     lat_dir, lat_abs, lon_dir, lon_abs);
                
                tile->size_bytes = SRTM_TILE_SIZE_BYTES;
                tile->downloaded = srtm_tile_exists(lon, lat);
                
                tiles = g_list_append(tiles, tile);
            }
        }
    }
    
    return tiles;
}

/* Get elevation from HGT file */
static int read_hgt_elevation(const char *filepath, int lon_idx, int lat_idx, 
                               double lon, double lat) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        return -32768;  /* SRTM void value */
    }
    
    /* HGT file format: 3601x3601 16-bit signed integers (big-endian) */
    /* Each pixel represents 1 arc-second */
    /* File covers 1°×1° = 3600 arc-seconds, but has 3601 pixels (inclusive bounds) */
    
    /* Calculate pixel position within tile */
    double tile_min_lon = (double)lon_idx;
    double tile_max_lon = (double)(lon_idx + 1);
    double tile_min_lat = (double)lat_idx;
    double tile_max_lat = (double)(lat_idx + 1);
    
    /* Clamp coordinates to tile bounds */
    if (lon < tile_min_lon) lon = tile_min_lon;
    if (lon > tile_max_lon) lon = tile_max_lon;
    if (lat < tile_min_lat) lat = tile_min_lat;
    if (lat > tile_max_lat) lat = tile_max_lat;
    
    /* Calculate pixel indices (0-3600) */
    int x = (int)((lon - tile_min_lon) * 3600.0);
    int y = (int)((tile_max_lat - lat) * 3600.0);  /* Y is inverted */
    
    /* Clamp to valid range */
    if (x < 0) x = 0;
    if (x > 3600) x = 3600;
    if (y < 0) y = 0;
    if (y > 3600) y = 3600;
    
    /* Calculate byte offset */
    long offset = (long)y * 3601L * 2L + (long)x * 2L;
    
    if (fseek(f, offset, SEEK_SET) != 0) {
        fclose(f);
        return -32768;
    }
    
    /* Read 16-bit big-endian value */
    unsigned char bytes[2];
    if (fread(bytes, 1, 2, f) != 2) {
        fclose(f);
        return -32768;
    }
    
    fclose(f);
    
    /* Convert big-endian to int16 */
    int16_t elevation = (int16_t)((bytes[0] << 8) | bytes[1]);
    
    /* SRTM void value is -32768 */
    if (elevation == -32768) {
        return -32768;
    }
    
    return (int)elevation;
}

/* Get elevation for coordinate */
int srtm_get_elevation(struct coord_geo *coord) {
    if (!coord || !srtm_data_dir) {
        return -32768;
    }
    
    /* Calculate tile indices (use floor to match srtm_calculate_tiles) */
    int lon_idx = (int)floor(coord->lng);
    int lat_idx = (int)floor(coord->lat);
    
    /* Check if tile exists */
    if (!srtm_tile_exists(lon_idx, lat_idx)) {
        return -32768;  /* Tile not downloaded */
    }
    
    /* Build file path */
    char *filename = srtm_get_tile_filename(lon_idx, lat_idx);
    char *filepath = g_build_filename(srtm_data_dir, filename, NULL);
    g_free(filename);
    
    /* Read elevation from HGT file */
    int elevation = read_hgt_elevation(filepath, lon_idx, lat_idx, coord->lng, coord->lat);
    
    g_free(filepath);
    return elevation;
}

/* Define regions matching Navit's map regions */
static struct srtm_region srtm_regions[] = {
    {"Europe", -12.97, 33.59, 34.15, 72.10, 0, 0, 0, 0},
    {"Germany", 5.18, 46.84, 15.47, 55.64, 0, 0, 0, 0},
    {"France", -5.5, 41.0, 9.5, 51.5, 0, 0, 0, 0},
    {"United Kingdom", -11.17, 49.5, 2.0, 61.0, 0, 0, 0, 0},
    {"Italy", 6.0, 35.5, 19.0, 47.5, 0, 0, 0, 0},
    {"Spain", -10.0, 35.0, 5.0, 44.5, 0, 0, 0, 0},
    {"Norway", 4.0, 57.0, 32.0, 81.0, 0, 0, 0, 0},
    {"Sweden", 10.0, 55.0, 25.0, 69.0, 0, 0, 0, 0},
    {"Poland", 14.0, 48.5, 25.0, 55.0, 0, 0, 0, 0},
    {"United States", -180.0, 18.0, -66.0, 72.0, 0, 0, 0, 0},
    {"China", 67.3, 5.3, 135.0, 54.5, 0, 0, 0, 0},
    {"Japan", 123.6, 25.2, 151.3, 47.1, 0, 0, 0, 0},
    {NULL, 0, 0, 0, 0, 0, 0, 0, 0}
};

/* Get list of available regions */
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
        
        /* Calculate tiles and size */
        GList *tiles = srtm_calculate_tiles(region->bbox_min_lon, region->bbox_min_lat,
                                            region->bbox_max_lon, region->bbox_max_lat);
        region->tile_count = g_list_length(tiles);
        region->size_bytes = (long long)region->tile_count * SRTM_TILE_SIZE_COMPRESSED;
        
        /* Check download status */
        int downloaded_count = 0;
        GList *l = tiles;
        while (l) {
            struct srtm_tile *tile = (struct srtm_tile *)l->data;
            if (tile->downloaded) {
                downloaded_count++;
            }
            l = g_list_next(l);
        }
        
        region->downloaded = (downloaded_count == region->tile_count) ? 1 : 0;
        region->progress_percent = region->tile_count > 0 ? 
            (downloaded_count * 100 / region->tile_count) : 0;
        
        srtm_free_tiles(tiles);
        regions = g_list_append(regions, region);
    }
    
    return regions;
}

/* Get region by name */
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

/* Download callback for libcurl */
#ifdef HAVE_CURL
/* Forward declarations */
static void srtm_download_progress_callback(void *data);
static int srtm_download_tile(struct srtm_tile *tile, struct srtm_download_context *ctx);
static size_t srtm_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    FILE *fp = (FILE *)userp;
    return fwrite(contents, size, nmemb, fp);
}

static int srtm_download_tile(struct srtm_tile *tile, struct srtm_download_context *ctx) {
    char *filepath = g_build_filename(srtm_data_dir, tile->filename, NULL);
    char *zip_path = g_strdup_printf("%s.zip", filepath);
    
    CURL *curl = curl_easy_init();
    if (!curl) {
        g_free(filepath);
        g_free(zip_path);
        return 0;
    }
    
    FILE *fp = fopen(zip_path, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        g_free(filepath);
        g_free(zip_path);
        return 0;
    }
    
    /* Try primary URL first */
    curl_easy_setopt(curl, CURLOPT_URL, tile->url_primary);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, srtm_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK && tile->url_fallback) {
        /* Try fallback URL */
        fseek(fp, 0, SEEK_SET);
        ftruncate(fileno(fp), 0);
        curl_easy_setopt(curl, CURLOPT_URL, tile->url_fallback);
        res = curl_easy_perform(curl);
    }
    
    fclose(fp);
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK) {
        if (g_file_test(zip_path, G_FILE_TEST_EXISTS)) {
            /* Extract HGT from ZIP using system unzip command */
            char *extract_cmd = g_strdup_printf(
                "cd '%s' && unzip -o -q '%s' '%s' 2>/dev/null",
                srtm_data_dir, zip_path, tile->filename
            );
            
            int extract_result = system(extract_cmd);
            g_free(extract_cmd);
            
            if (extract_result == 0) {
                /* Verify extracted file exists and has correct size */
                if (g_file_test(filepath, G_FILE_TEST_EXISTS)) {
                    GStatBuf stat_buf;
                    if (g_stat(filepath, &stat_buf) == 0) {
                        /* HGT file should be ~25.9 MB */
                        if (stat_buf.st_size > 20000000 && stat_buf.st_size < 30000000) {
                            tile->downloaded = 1;
                            g_unlink(zip_path);
                            g_free(zip_path);
                            g_free(filepath);
                            return 1;
                        } else {
                            dbg(lvl_warning, "SRTM: Extracted file has unexpected size: %ld bytes", 
                                (long)stat_buf.st_size);
                            g_unlink(filepath);
                        }
                    }
                }
            } else {
                dbg(lvl_warning, "SRTM: Failed to extract ZIP file");
            }
            
            g_unlink(zip_path);
        }
    }
    
    g_unlink(zip_path);
    g_free(zip_path);
    g_free(filepath);
    return 0;
}

/* Download progress callback */
static void srtm_download_progress_callback(void *data) {
    struct srtm_download_context *ctx = (struct srtm_download_context *)data;
    
    if (!ctx || ctx->status != SRTM_DOWNLOAD_DOWNLOADING) {
        return;
    }
    
    /* Get current tile */
    GList *tile_list = g_list_nth(ctx->tiles, ctx->current_tile_index);
    if (!tile_list) {
        /* All tiles downloaded */
        ctx->status = SRTM_DOWNLOAD_COMPLETED;
        if (ctx->progress_callback) {
            ctx->progress_callback(ctx, ctx->user_data);
        }
        return;
    }
    
    struct srtm_tile *tile = (struct srtm_tile *)tile_list->data;
    
    /* Download current tile */
    if (!tile->downloaded) {
        if (srtm_download_tile(tile, ctx)) {
            ctx->downloaded_bytes += SRTM_TILE_SIZE_COMPRESSED;
            ctx->current_tile_index++;
            
            /* Update progress */
            if (ctx->progress_callback) {
                ctx->progress_callback(ctx, ctx->user_data);
            }
        } else {
            /* Download failed - try next tile */
            ctx->current_tile_index++;
        }
    } else {
        /* Already downloaded - skip */
        ctx->current_tile_index++;
    }
    
    /* Schedule next tile download */
    if (ctx->current_tile_index < g_list_length(ctx->tiles)) {
        /* Use idle callback to continue downloading */
        extern struct event_idle *event_add_idle(int priority, struct callback *cb);
        struct callback *cb = callback_new_1(callback_cast(srtm_download_progress_callback), ctx);
        event_add_idle(0, cb);
    } else {
        /* All tiles processed */
        ctx->status = SRTM_DOWNLOAD_COMPLETED;
        if (ctx->progress_callback) {
            ctx->progress_callback(ctx, ctx->user_data);
        }
    }
}
#endif

/* Start downloading region */
struct srtm_download_context *srtm_download_region(struct srtm_region *region,
                                                    void (*progress_callback)(struct srtm_download_context *, void *),
                                                    void *user_data) {
#ifdef HAVE_CURL
    struct srtm_download_context *ctx = g_new0(struct srtm_download_context, 1);
    ctx->region = region;
    ctx->tiles = srtm_calculate_tiles(region->bbox_min_lon, region->bbox_min_lat,
                                      region->bbox_max_lon, region->bbox_max_lat);
    ctx->status = SRTM_DOWNLOAD_DOWNLOADING;
    ctx->current_tile_index = 0;
    ctx->progress_callback = progress_callback;
    ctx->user_data = user_data;
    
    /* Calculate total size */
    ctx->total_bytes = (long long)g_list_length(ctx->tiles) * SRTM_TILE_SIZE_COMPRESSED;
    ctx->downloaded_bytes = 0;
    
    /* Start async download using idle callback */
    struct callback *cb = callback_new_1(callback_cast(srtm_download_progress_callback), ctx);
    event_add_idle(0, cb);
    
    return ctx;
#else
    dbg(lvl_error, "SRTM: libcurl not available, cannot download");
    return NULL;
#endif
}

/* Pause download */
int srtm_download_pause(struct srtm_download_context *ctx) {
    if (!ctx || ctx->status != SRTM_DOWNLOAD_DOWNLOADING) {
        return 0;
    }
    ctx->status = SRTM_DOWNLOAD_PAUSED;
    return 1;
}

/* Resume download */
int srtm_download_resume(struct srtm_download_context *ctx) {
    if (!ctx || ctx->status != SRTM_DOWNLOAD_PAUSED) {
        return 0;
    }
    ctx->status = SRTM_DOWNLOAD_DOWNLOADING;
    return 1;
}

/* Cancel download */
int srtm_download_cancel(struct srtm_download_context *ctx) {
    if (!ctx) {
        return 0;
    }
    ctx->status = SRTM_DOWNLOAD_IDLE;
    return 1;
}

/* Free region list */
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

/* Free tile list */
void srtm_free_tiles(GList *tiles) {
    GList *l = tiles;
    while (l) {
        struct srtm_tile *tile = (struct srtm_tile *)l->data;
        if (tile) {
            g_free(tile->filename);
            g_free(tile->url_primary);
            g_free(tile->url_fallback);
            g_free(tile->checksum_md5);
            g_free(tile);
        }
        l = g_list_next(l);
    }
    g_list_free(tiles);
}

/* Free download context */
void srtm_free_download_context(struct srtm_download_context *ctx) {
    if (ctx) {
        srtm_free_tiles(ctx->tiles);
        g_free(ctx->error_message);
        g_free(ctx);
    }
}
