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

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_SRTM_H
#define NAVIT_PLUGIN_DRIVER_BREAK_SRTM_H

#include "config.h"
#include "coord.h"
#include "driver_break.h"
#include <glib.h>

/* SRTM region definition */
struct srtm_region {
    char *name;           /* Region name (e.g., "Germany", "Europe") */
    double bbox_min_lon;  /* Minimum longitude */
    double bbox_min_lat;  /* Minimum latitude */
    double bbox_max_lon;  /* Maximum longitude */
    double bbox_max_lat;  /* Maximum latitude */
    int tile_count;       /* Number of tiles needed */
    long long size_bytes; /* Estimated download size in bytes */
    int downloaded;       /* 1 if all tiles downloaded, 0 otherwise */
    int progress_percent; /* Download progress (0-100) */
};

/* SRTM tile information */
struct srtm_tile {
    int lon;              /* Longitude index (e.g., 6 for E006) */
    int lat;              /* Latitude index (e.g., 45 for N45) */
    char *filename;       /* HGT tile filename (e.g., "N45E006.hgt") */
    char *filename_geotiff; /* GeoTIFF filename when using Copernicus (optional) */
    char *url_primary;    /* Primary download URL (Copernicus GeoTIFF or Viewfinder HGT) */
    char *url_fallback;   /* Fallback URL (e.g. Viewfinder HGT zip) */
    char *url_fallback2;  /* Second fallback (e.g. NASA SRTMGL1 zip) */
    int downloaded;       /* 1 if downloaded, 0 otherwise */
    long long size_bytes; /* File size in bytes */
    char *checksum_md5;   /* MD5 checksum (if available) */
};

/* SRTM download status */
enum srtm_download_status {
    SRTM_DOWNLOAD_IDLE,
    SRTM_DOWNLOAD_DOWNLOADING,
    SRTM_DOWNLOAD_PAUSED,
    SRTM_DOWNLOAD_COMPLETED,
    SRTM_DOWNLOAD_ERROR
};

/* SRTM download context */
struct srtm_download_context {
    struct srtm_region *region;
    GList *tiles; /* List of struct srtm_tile* */
    enum srtm_download_status status;
    int current_tile_index;
    long long downloaded_bytes;
    long long total_bytes;
    char *error_message;
    void (*progress_callback)(struct srtm_download_context *ctx, void *user_data);
    void *user_data;
};

/* Initialize SRTM system */
int srtm_init(const char *srtm_dir);

/* Get SRTM data directory */
char *srtm_get_data_directory(void);

/* Get elevation for coordinate (returns -32768 if not available) */
int srtm_get_elevation(struct coord_geo *coord);

/* Calculate tiles needed for bounding box */
GList *srtm_calculate_tiles(double min_lon, double min_lat, double max_lon, double max_lat);

/* Get list of available regions */
GList *srtm_get_regions(void);

/* Get region by name */
struct srtm_region *srtm_get_region(const char *name);

/* Start downloading region */
struct srtm_download_context *srtm_download_region(struct srtm_region *region,
                                                   void (*progress_callback)(struct srtm_download_context *, void *),
                                                   void *user_data);

/* Pause download */
int srtm_download_pause(struct srtm_download_context *ctx);

/* Resume download */
int srtm_download_resume(struct srtm_download_context *ctx);

/* Cancel download */
int srtm_download_cancel(struct srtm_download_context *ctx);

/* Check if tile exists */
int srtm_tile_exists(int lon, int lat);

/* Get tile filename */
char *srtm_get_tile_filename(int lon, int lat);

/* Free region list */
void srtm_free_regions(GList *regions);

/* Free tile list */
void srtm_free_tiles(GList *tiles);

/* Free download context */
void srtm_free_download_context(struct srtm_download_context *ctx);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_SRTM_H */
