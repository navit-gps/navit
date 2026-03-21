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

#include "driver_break_srtm.h"
#include "callback.h"
#include "config.h"
#include "debug.h"
#include "event.h"
#include "file.h"
#include "navit.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_CURL
#    include <curl/curl.h>
#endif

#ifdef HAVE_GEOTIFF
#    include <stdarg.h>
#    include <tiffio.h>
/* No-op warning handler to suppress "Unknown field" for GeoTIFF tags (33550, 33922, 34735, etc.) */
static void srtm_tiff_warning_suppress(const char *module, const char *fmt, va_list ap) {
    (void)module;
    (void)fmt;
    (void)ap;
}
#endif

/* SRTM data directory */
static char *srtm_data_dir = NULL;

/* Primary: Copernicus DEM GLO-30 (AWS S3, no auth). URL: {base}/{tilename}/{tilename}.tif */
#define SRTM_URL_COPERNICUS "https://copernicus-dem-30m.s3.amazonaws.com/"
/* Fallback: Viewfinder Panoramas dem3 (tiles in zone folders). URL: {base}/{ZONE}/{TILENAME}.zip.
 * Viewfinder blocks scripted downloads without a browser User-Agent; tile index: dem3list.txt */
#define SRTM_URL_VIEWFINDER_DEM3 "http://www.viewfinderpanoramas.org/dem3/"
#define SRTM_CURL_USERAGENT                                                                                            \
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"
/* Second fallback: NASA SRTMGL1 (flat URL) */
#define SRTM_URL_NASA "https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL1.003/2000.02.11/"

/* HGT file size: 1 arc-second = 3601x3601 pixels = 25,934,402 bytes */
#define SRTM_TILE_SIZE_BYTES 25934402
#define SRTM_TILE_SIZE_COMPRESSED 2800000 /* ~2.8 MB compressed */

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
#ifdef HAVE_GEOTIFF
    /* Suppress libtiff "Unknown field" warnings for GeoTIFF tags (33550, 33922, 34735, etc.); elevation read is
     * unaffected */
    TIFFSetWarningHandler(srtm_tiff_warning_suppress);
#endif
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

/* Get tile filename from lon/lat indices (HGT) */
char *srtm_get_tile_filename(int lon, int lat) {
    char *filename;
    char lon_dir = (lon >= 0) ? 'E' : 'W';
    char lat_dir = (lat >= 0) ? 'N' : 'S';
    int lon_abs = abs(lon);
    int lat_abs = abs(lat);

    filename = g_strdup_printf("%c%02d%c%03d.hgt", lat_dir, lat_abs, lon_dir, lon_abs);
    return filename;
}

/* Viewfinder dem3 zone for given tile (integer lat/lon). Returns e.g. "M32" or NULL if unknown. */
static const char *srtm_viewfinder_zone(int lat, int lon) {
    /* Norway / Scandinavia: 60-63N, 6-12E -> M32. Extend as needed for other regions. */
    if (lat >= 60 && lat <= 63 && lon >= 6 && lon <= 12)
        return "M32";
    return NULL;
}

#ifdef HAVE_GEOTIFF
/* Copernicus DEM GLO-30 tile name (with .tif). Folder name is same without .tif. */
static char *srtm_get_geotiff_tile_filename(int lon, int lat) {
    char lon_dir = (lon >= 0) ? 'E' : 'W';
    char lat_dir = (lat >= 0) ? 'N' : 'S';
    int lon_abs = abs(lon);
    int lat_abs = abs(lat);
    return g_strdup_printf("Copernicus_DSM_COG_10_%c%02d_00_%c%03d_00_DEM.tif", lat_dir, lat_abs, lon_dir, lon_abs);
}
#endif

/* Check if tile file exists (GeoTIFF or HGT) */
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

/* Calculate tiles needed for bounding box */
GList *srtm_calculate_tiles(double min_lon, double min_lat, double max_lon, double max_lat) {
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

                char lon_dir = (lon >= 0) ? 'E' : 'W';
                char lat_dir = (lat >= 0) ? 'N' : 'S';
                int lon_abs = abs(lon);
                int lat_abs = abs(lat);
                const char *vf_zone = srtm_viewfinder_zone(lat, lon);
                char *vf_zip_url = vf_zone ? g_strdup_printf("%s%s/%c%02d%c%03d.zip", SRTM_URL_VIEWFINDER_DEM3, vf_zone,
                                                             lat_dir, lat_abs, lon_dir, lon_abs)
                                           : NULL;

                tile->size_bytes = SRTM_TILE_SIZE_BYTES;
#ifdef HAVE_GEOTIFF
                tile->filename_geotiff = srtm_get_geotiff_tile_filename(lon, lat);
                /* Primary: Copernicus DEM GLO-30 (S3). URL: {base}/{tilename}/{tilename}.tif */
                tile->url_primary = g_strdup_printf(
                    "%sCopernicus_DSM_COG_10_%c%02d_00_%c%03d_00_DEM/Copernicus_DSM_COG_10_%c%02d_00_%c%03d_00_DEM.tif",
                    SRTM_URL_COPERNICUS, lat_dir, lat_abs, lon_dir, lon_abs, lat_dir, lat_abs, lon_dir, lon_abs);
                tile->url_fallback = vf_zip_url; /* Viewfinder dem3 */
                tile->url_fallback2 = g_strdup_printf("%s%c%02d%c%03d.SRTMGL1.hgt.zip", SRTM_URL_NASA, lat_dir, lat_abs,
                                                      lon_dir, lon_abs);
#else
                /* No GeoTIFF: primary Viewfinder dem3, fallback NASA */
                tile->url_primary = vf_zip_url ? vf_zip_url
                                               : g_strdup_printf("%s%c%02d%c%03d.SRTMGL1.hgt.zip", SRTM_URL_NASA,
                                                                 lat_dir, lat_abs, lon_dir, lon_abs);
                tile->url_fallback = vf_zip_url ? g_strdup_printf("%s%c%02d%c%03d.SRTMGL1.hgt.zip", SRTM_URL_NASA,
                                                                  lat_dir, lat_abs, lon_dir, lon_abs)
                                                : NULL;
                tile->url_fallback2 = NULL;
#endif

                tile->downloaded = srtm_tile_exists(lon, lat);

                tiles = g_list_append(tiles, tile);
            }
        }
    }

    return tiles;
}

/* Get elevation from HGT file */
static int read_hgt_elevation(const char *filepath, int lon_idx, int lat_idx, double lon, double lat) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        return -32768; /* SRTM void value */
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
    if (lon < tile_min_lon)
        lon = tile_min_lon;
    if (lon > tile_max_lon)
        lon = tile_max_lon;
    if (lat < tile_min_lat)
        lat = tile_min_lat;
    if (lat > tile_max_lat)
        lat = tile_max_lat;

    /* Calculate pixel indices (0-3600) */
    int x = (int)((lon - tile_min_lon) * 3600.0);
    int y = (int)((tile_max_lat - lat) * 3600.0); /* Y is inverted */

    /* Clamp to valid range */
    if (x < 0)
        x = 0;
    if (x > 3600)
        x = 3600;
    if (y < 0)
        y = 0;
    if (y > 3600)
        y = 3600;

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

#ifdef HAVE_GEOTIFF
/* Parse elevation from Int16 or Float32 buffer at given pixel index. Returns -32768 if invalid. */
static int geotiff_elev_from_pixel(uint16_t sample_format, uint16_t bits_per_sample, const void *buf,
                                   uint32_t pixel_index) {
    if (sample_format == SAMPLEFORMAT_INT && bits_per_sample == 16) {
        const int16_t *s = (const int16_t *)buf;
        int16_t v = s[pixel_index];
        return (v != -32768) ? (int)v : -32768;
    }
    if (sample_format == SAMPLEFORMAT_IEEEFP && bits_per_sample == 32) {
        const float *f = (const float *)buf;
        float v = f[pixel_index];
        return (v >= -500.0f && v <= 9000.0f) ? (int)(v + 0.5f) : -32768;
    }
    return -32768;
}

/* Read elevation from a tiled GeoTIFF at pixel (x, y). */
static int read_geotiff_tiled_at(TIFF *tif, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                                 uint16_t sample_format, uint16_t bits_per_sample) {
    uint32_t tile_width = 0, tile_height = 0;
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_height);
    if (tile_width == 0)
        tile_width = width;
    if (tile_height == 0)
        tile_height = height;

    ttile_t tile_index = TIFFComputeTile(tif, x, y, 0, 0);
    tsize_t tile_size = TIFFTileSize(tif);
    unsigned char *buf = (unsigned char *)g_malloc(tile_size);
    int elevation_val = -32768;
    if (buf && TIFFReadEncodedTile(tif, tile_index, buf, tile_size) >= 0) {
        uint32_t tx = x % tile_width;
        uint32_t ty = y % tile_height;
        uint32_t pixel_index = ty * tile_width + tx;
        elevation_val = geotiff_elev_from_pixel(sample_format, bits_per_sample, buf, pixel_index);
    }
    g_free(buf);
    return elevation_val;
}

/* Read elevation from a scanline GeoTIFF at pixel (x, y). */
static int read_geotiff_scanline_at(TIFF *tif, uint32_t x, uint32_t y, uint16_t sample_format,
                                    uint16_t bits_per_sample) {
    tsize_t row_size = TIFFScanlineSize(tif);
    unsigned char *row_buf = (unsigned char *)g_malloc(row_size);
    int elevation_val = -32768;
    if (row_buf && TIFFReadScanline(tif, row_buf, y, 0) >= 0)
        elevation_val = geotiff_elev_from_pixel(sample_format, bits_per_sample, row_buf, x);
    g_free(row_buf);
    return elevation_val;
}

/* Get elevation from Copernicus GLO-30 GeoTIFF (1x1 degree, Float32 or Int16) */
static int read_geotiff_elevation(const char *filepath, int lon_idx, int lat_idx, double lon, double lat) {
    TIFF *tif = TIFFOpen(filepath, "r");
    if (!tif)
        return -32768;

    uint32_t width = 0, height = 0;
    uint16_t sample_format = 0, bits_per_sample = 0;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sample_format);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);

    if (width == 0 || height == 0) {
        TIFFClose(tif);
        return -32768;
    }

    double tile_min_lon = (double)lon_idx;
    double tile_max_lat = (double)(lat_idx + 1);
    double xd = (lon - tile_min_lon) * (double)width;
    double yd = (tile_max_lat - lat) * (double)height;
    uint32_t x = (uint32_t)xd;
    uint32_t y = (uint32_t)yd;
    if (x >= width)
        x = width - 1;
    if (y >= height)
        y = height - 1;

    int elevation_val = TIFFIsTiled(tif)
                            ? read_geotiff_tiled_at(tif, x, y, width, height, sample_format, bits_per_sample)
                            : read_geotiff_scanline_at(tif, x, y, sample_format, bits_per_sample);

    TIFFClose(tif);
    return elevation_val;
}
#endif

/* Get elevation for coordinate (tries GeoTIFF then HGT).
 * Tile borders: each (lng, lat) maps to exactly one 1x1 degree tile via floor(lng), floor(lat).
 * There is no single-file traversal; callers (e.g. route iteration) invoke this per point, so
 * crossing a tile boundary is handled by the next call using the adjacent tile. */
int srtm_get_elevation(struct coord_geo *coord) {
    if (!coord || !srtm_data_dir) {
        return -32768;
    }

    int lon_idx = (int)floor(coord->lng);
    int lat_idx = (int)floor(coord->lat);

    if (!srtm_tile_exists(lon_idx, lat_idx)) {
        return -32768;
    }

#ifdef HAVE_GEOTIFF
    {
        char *geotiff_name = srtm_get_geotiff_tile_filename(lon_idx, lat_idx);
        char *geotiff_path = g_build_filename(srtm_data_dir, geotiff_name, NULL);
        if (g_file_test(geotiff_path, G_FILE_TEST_EXISTS)) {
            int elevation = read_geotiff_elevation(geotiff_path, lon_idx, lat_idx, coord->lng, coord->lat);
            g_free(geotiff_name);
            g_free(geotiff_path);
            return elevation;
        }
        g_free(geotiff_name);
        g_free(geotiff_path);
    }
#endif

    {
        char *filename = srtm_get_tile_filename(lon_idx, lat_idx);
        char *filepath = g_build_filename(srtm_data_dir, filename, NULL);
        g_free(filename);
        int elevation = read_hgt_elevation(filepath, lon_idx, lat_idx, coord->lng, coord->lat);
        g_free(filepath);
        return elevation;
    }
}

/* Define regions matching Navit's map regions */
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
        GList *tiles = srtm_calculate_tiles(region->bbox_min_lon, region->bbox_min_lat, region->bbox_max_lon,
                                            region->bbox_max_lat);
        region->tile_count = g_list_length(tiles);
        region->size_bytes = 0;
        {
            GList *t = tiles;
            while (t) {
                struct srtm_tile *tl = (struct srtm_tile *)t->data;
                region->size_bytes += tl->size_bytes;
                t = g_list_next(t);
            }
        }

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
        region->progress_percent = region->tile_count > 0 ? (downloaded_count * 100 / region->tile_count) : 0;

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

static int srtm_try_download_copernicus(CURL *curl, struct srtm_tile *tile, char *filepath, char *zip_path) {
#    ifdef HAVE_GEOTIFF
    if (tile->filename_geotiff && tile->url_primary && strstr(tile->url_primary, ".tif")) {
        char *geotiff_path = g_build_filename(srtm_data_dir, tile->filename_geotiff, NULL);
        FILE *fp_gt = fopen(geotiff_path, "wb");
        if (fp_gt) {
            curl_easy_setopt(curl, CURLOPT_URL, tile->url_primary);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, srtm_write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp_gt);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
            CURLcode res = curl_easy_perform(curl);
            fclose(fp_gt);
            if (res == CURLE_OK) {
                GStatBuf st;
                if (g_stat(geotiff_path, &st) == 0 && st.st_size > 1000) {
                    FILE *f = fopen(geotiff_path, "rb");
                    if (f) {
                        unsigned char magic[2];
                        int ok =
                            (fread(magic, 1, 2, f) == 2
                             && ((magic[0] == 0x49 && magic[1] == 0x49) || (magic[0] == 0x4D && magic[1] == 0x4D)));
                        fclose(f);
                        if (ok) {
                            tile->downloaded = 1;
                            g_free(geotiff_path);
                            g_free(filepath);
                            g_free(zip_path);
                            curl_easy_cleanup(curl);
                            return 1;
                        }
                    }
                }
                g_unlink(geotiff_path);
            }
            g_free(geotiff_path);
        }
    }
#    else
    (void)curl;
    (void)tile;
    (void)filepath;
    (void)zip_path;
#    endif
    return 0;
}

static CURLcode srtm_download_zip_with_fallbacks(CURL *curl, struct srtm_tile *tile, FILE *fp) {
    const char *url_to_try = tile->url_primary;

    if (url_to_try && strstr(url_to_try, ".tif"))
        url_to_try = tile->url_fallback;

    if (url_to_try)
        curl_easy_setopt(curl, CURLOPT_URL, url_to_try);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK && tile->url_fallback && url_to_try != tile->url_fallback) {
        fseek(fp, 0, SEEK_SET);
        (void)ftruncate(fileno(fp), 0);
        curl_easy_setopt(curl, CURLOPT_URL, tile->url_fallback);
        res = curl_easy_perform(curl);
    }
    if (res != CURLE_OK && tile->url_fallback2) {
        fseek(fp, 0, SEEK_SET);
        (void)ftruncate(fileno(fp), 0);
        curl_easy_setopt(curl, CURLOPT_URL, tile->url_fallback2);
        res = curl_easy_perform(curl);
    }

    return res;
}

static int srtm_extract_primary_hgt(const char *zip_path, char *filepath, struct srtm_tile *tile) {
    char *extract_cmd =
        g_strdup_printf("cd '%s' && unzip -o -q '%s' '%s' 2>/dev/null", srtm_data_dir, zip_path, tile->filename);
    int extract_result = system(extract_cmd);
    g_free(extract_cmd);

    if (extract_result == 0 && g_file_test(filepath, G_FILE_TEST_EXISTS)) {
        GStatBuf stat_buf;
        if (g_stat(filepath, &stat_buf) == 0 && stat_buf.st_size > 20000000 && stat_buf.st_size < 30000000) {
            tile->downloaded = 1;
            g_unlink(zip_path);
            g_free(zip_path);
            g_free(filepath);
            return 1;
        }
        if (g_file_test(filepath, G_FILE_TEST_EXISTS))
            g_unlink(filepath);
    }
    return 0;
}

static int srtm_extract_nasa_hgt(const char *zip_path, char *filepath, struct srtm_tile *tile) {
    char lat_dir = (tile->lat >= 0) ? 'N' : 'S';
    char lon_dir = (tile->lon >= 0) ? 'E' : 'W';
    int lat_abs = abs(tile->lat);
    int lon_abs = abs(tile->lon);
    char *nasa_name = g_strdup_printf("%c%02d%c%03d.SRTMGL1.hgt", lat_dir, lat_abs, lon_dir, lon_abs);
    char *extract_cmd =
        g_strdup_printf("cd '%s' && unzip -o -q '%s' '%s' 2>/dev/null", srtm_data_dir, zip_path, nasa_name);
    int extract_result = system(extract_cmd);
    g_free(extract_cmd);
    if (extract_result == 0) {
        char *nasa_path = g_build_filename(srtm_data_dir, nasa_name, NULL);
        if (g_file_test(nasa_path, G_FILE_TEST_EXISTS) && g_rename(nasa_path, filepath) == 0) {
            GStatBuf stat_buf;
            if (g_stat(filepath, &stat_buf) == 0 && stat_buf.st_size > 20000000 && stat_buf.st_size < 30000000) {
                tile->downloaded = 1;
                g_unlink(zip_path);
                g_free(zip_path);
                g_free(filepath);
                g_free(nasa_path);
                g_free(nasa_name);
                return 1;
            }
        }
        if (g_file_test(nasa_path, G_FILE_TEST_EXISTS))
            g_unlink(nasa_path);
        g_free(nasa_path);
    }
    g_free(nasa_name);
    return 0;
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

    (void)ctx;

    curl_easy_setopt(curl, CURLOPT_USERAGENT, SRTM_CURL_USERAGENT);

    if (srtm_try_download_copernicus(curl, tile, filepath, zip_path)) {
        return 1;
    }

    FILE *fp = fopen(zip_path, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        g_free(filepath);
        g_free(zip_path);
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, srtm_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);

    CURLcode res = srtm_download_zip_with_fallbacks(curl, tile, fp);

    fclose(fp);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && g_file_test(zip_path, G_FILE_TEST_EXISTS)) {
        if (srtm_extract_primary_hgt(zip_path, filepath, tile)) {
            return 1;
        }
        if (!g_file_test(filepath, G_FILE_TEST_EXISTS)) {
            if (srtm_extract_nasa_hgt(zip_path, filepath, tile)) {
                return 1;
            }
        }
        g_unlink(zip_path);
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
            ctx->downloaded_bytes += tile->size_bytes;
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
    ctx->tiles =
        srtm_calculate_tiles(region->bbox_min_lon, region->bbox_min_lat, region->bbox_max_lon, region->bbox_max_lat);
    ctx->status = SRTM_DOWNLOAD_DOWNLOADING;
    ctx->current_tile_index = 0;
    ctx->progress_callback = progress_callback;
    ctx->user_data = user_data;

    ctx->total_bytes = 0;
    ctx->downloaded_bytes = 0;
    {
        GList *t = ctx->tiles;
        while (t) {
            struct srtm_tile *tl = (struct srtm_tile *)t->data;
            ctx->total_bytes += tl->size_bytes;
            t = g_list_next(t);
        }
    }

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

/* Free download context */
void srtm_free_download_context(struct srtm_download_context *ctx) {
    if (ctx) {
        srtm_free_tiles(ctx->tiles);
        g_free(ctx->error_message);
        g_free(ctx);
    }
}
