/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Internal declarations shared between SRTM source files.
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_SRTM_PRIV_H
#define NAVIT_PLUGIN_DRIVER_BREAK_SRTM_PRIV_H

#include "driver_break_srtm.h"

#define SRTM_URL_COPERNICUS "https://copernicus-dem-30m.s3.amazonaws.com/"
#define SRTM_URL_VIEWFINDER_DEM3 "http://www.viewfinderpanoramas.org/dem3/"
#define SRTM_CURL_USERAGENT                                                                                            \
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"
#define SRTM_URL_NASA "https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL1.003/2000.02.11/"
#define SRTM_TILE_SIZE_BYTES 25934402

extern char *srtm_data_dir;

const char *srtm_viewfinder_zone(int lat, int lon);

#ifdef HAVE_GEOTIFF
char *srtm_get_geotiff_tile_filename(int lon, int lat);
#endif

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_SRTM_PRIV_H */
