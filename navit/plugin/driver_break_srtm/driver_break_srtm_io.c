/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * SRTM elevation reads from cached HGT and GeoTIFF tiles.
 */

#include "config.h"
#include "driver_break_srtm.h"
#include "driver_break_srtm_priv.h"
#include <glib.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_GEOTIFF
#    include <tiffio.h>
#endif

static void read_hgt_clamp_coord(double *lon, double *lat, int lon_idx, int lat_idx) {
    double tile_min_lon = (double)lon_idx;
    double tile_max_lon = (double)(lon_idx + 1);
    double tile_min_lat = (double)lat_idx;
    double tile_max_lat = (double)(lat_idx + 1);

    if (*lon < tile_min_lon)
        *lon = tile_min_lon;
    if (*lon > tile_max_lon)
        *lon = tile_max_lon;
    if (*lat < tile_min_lat)
        *lat = tile_min_lat;
    if (*lat > tile_max_lat)
        *lat = tile_max_lat;
}

static void read_hgt_pixel_indices(double lon, double lat, int lon_idx, int lat_idx, int *x, int *y) {
    double tile_min_lon = (double)lon_idx;
    double tile_max_lat = (double)(lat_idx + 1);

    *x = (int)((lon - tile_min_lon) * 3600.0);
    *y = (int)((tile_max_lat - lat) * 3600.0);
    if (*x < 0)
        *x = 0;
    if (*x > 3600)
        *x = 3600;
    if (*y < 0)
        *y = 0;
    if (*y > 3600)
        *y = 3600;
}

static int read_hgt_elevation(const char *filepath, int lon_idx, int lat_idx, double lon, double lat) {
    FILE *f = fopen(filepath, "rb");
    int x;
    int y;
    long offset;
    unsigned char bytes[2];
    int16_t elevation;

    if (!f)
        return -32768;

    read_hgt_clamp_coord(&lon, &lat, lon_idx, lat_idx);
    read_hgt_pixel_indices(lon, lat, lon_idx, lat_idx, &x, &y);

    offset = (long)y * 3601L * 2L + (long)x * 2L;
    if (fseek(f, offset, SEEK_SET) != 0) {
        fclose(f);
        return -32768;
    }

    if (fread(bytes, 1, 2, f) != 2) {
        fclose(f);
        return -32768;
    }

    fclose(f);
    elevation = (int16_t)((bytes[0] << 8) | bytes[1]);
    if (elevation == -32768)
        return -32768;

    return (int)elevation;
}

#ifdef HAVE_GEOTIFF
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

static int read_geotiff_tiled_at(TIFF *tif, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                                 uint16_t sample_format, uint16_t bits_per_sample) {
    uint32_t tile_width = 0, tile_height = 0;
    ttile_t tile_index;
    tsize_t tile_size;
    unsigned char *buf;
    uint32_t tx;
    uint32_t ty;
    uint32_t pixel_index;
    int elevation_val = -32768;

    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_height);
    if (tile_width == 0)
        tile_width = width;
    if (tile_height == 0)
        tile_height = height;

    tile_index = TIFFComputeTile(tif, x, y, 0, 0);
    tile_size = TIFFTileSize(tif);
    buf = (unsigned char *)g_malloc(tile_size);
    if (buf && TIFFReadEncodedTile(tif, tile_index, buf, tile_size) >= 0) {
        tx = x % tile_width;
        ty = y % tile_height;
        pixel_index = ty * tile_width + tx;
        elevation_val = geotiff_elev_from_pixel(sample_format, bits_per_sample, buf, pixel_index);
    }
    g_free(buf);
    return elevation_val;
}

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

static int read_geotiff_elevation(const char *filepath, int lon_idx, int lat_idx, double lon, double lat) {
    TIFF *tif = TIFFOpen(filepath, "r");
    uint32_t width = 0, height = 0;
    uint16_t sample_format = 0, bits_per_sample = 0;
    double tile_min_lon;
    double tile_max_lat;
    double xd;
    double yd;
    uint32_t x;
    uint32_t y;
    int elevation_val;

    if (!tif)
        return -32768;

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sample_format);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);

    if (width == 0 || height == 0) {
        TIFFClose(tif);
        return -32768;
    }

    tile_min_lon = (double)lon_idx;
    tile_max_lat = (double)(lat_idx + 1);
    xd = (lon - tile_min_lon) * (double)width;
    yd = (tile_max_lat - lat) * (double)height;
    x = (uint32_t)xd;
    y = (uint32_t)yd;
    if (x >= width)
        x = width - 1;
    if (y >= height)
        y = height - 1;

    elevation_val = TIFFIsTiled(tif) ? read_geotiff_tiled_at(tif, x, y, width, height, sample_format, bits_per_sample)
                                     : read_geotiff_scanline_at(tif, x, y, sample_format, bits_per_sample);

    TIFFClose(tif);
    return elevation_val;
}
#endif

int srtm_get_elevation(struct coord_geo *coord) {
    int lon_idx;
    int lat_idx;

    if (!coord || !srtm_data_dir)
        return -32768;

    lon_idx = (int)floor(coord->lng);
    lat_idx = (int)floor(coord->lat);

    if (!srtm_tile_exists(lon_idx, lat_idx))
        return -32768;

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
        int elevation;
        g_free(filename);
        elevation = read_hgt_elevation(filepath, lon_idx, lat_idx, coord->lng, coord->lat);
        g_free(filepath);
        return elevation;
    }
}
