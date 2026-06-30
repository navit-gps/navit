/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * SRTM tile download via libcurl with Copernicus, Viewfinder, and NASA fallbacks.
 */

#include "callback.h"
#include "config.h"
#include "debug.h"
#include "driver_break_srtm.h"
#include "driver_break_srtm_priv.h"
#include "event.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CURL
#    include <curl/curl.h>

static void srtm_download_progress_callback(void *data);
static int srtm_download_tile(struct srtm_tile *tile, struct srtm_download_context *ctx);

static size_t srtm_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    FILE *fp = (FILE *)userp;
    return fwrite(contents, size, nmemb, fp);
}

static int srtm_geotiff_magic_valid(const char *geotiff_path) {
    FILE *f = fopen(geotiff_path, "rb");
    unsigned char magic[2];
    int ok;

    if (!f)
        return 0;
    ok = (fread(magic, 1, 2, f) == 2
          && ((magic[0] == 0x49 && magic[1] == 0x49) || (magic[0] == 0x4D && magic[1] == 0x4D)));
    fclose(f);
    return ok;
}

static int srtm_try_download_copernicus(CURL *curl, struct srtm_tile *tile, char *filepath, char *zip_path) {
#    ifdef HAVE_GEOTIFF
    char *geotiff_path;
    FILE *fp_gt;
    CURLcode res;
    GStatBuf st;

    if (!tile->filename_geotiff || !tile->url_primary || !strstr(tile->url_primary, ".tif"))
        return 0;

    geotiff_path = g_build_filename(srtm_data_dir, tile->filename_geotiff, NULL);
    fp_gt = fopen(geotiff_path, "wb");
    if (!fp_gt) {
        g_free(geotiff_path);
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, tile->url_primary);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, srtm_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp_gt);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
    res = curl_easy_perform(curl);
    fclose(fp_gt);

    if (res == CURLE_OK && g_stat(geotiff_path, &st) == 0 && st.st_size > 1000
        && srtm_geotiff_magic_valid(geotiff_path)) {
        tile->downloaded = 1;
        g_free(geotiff_path);
        g_free(filepath);
        g_free(zip_path);
        curl_easy_cleanup(curl);
        return 1;
    }

    g_unlink(geotiff_path);
    g_free(geotiff_path);
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
    CURLcode res;

    if (url_to_try && strstr(url_to_try, ".tif"))
        url_to_try = tile->url_fallback;

    if (url_to_try)
        curl_easy_setopt(curl, CURLOPT_URL, url_to_try);

    res = curl_easy_perform(curl);
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

static int srtm_hgt_size_valid(const char *filepath) {
    GStatBuf stat_buf;
    if (g_stat(filepath, &stat_buf) != 0)
        return 0;
    return stat_buf.st_size > 20000000 && stat_buf.st_size < 30000000;
}

static int srtm_extract_primary_hgt(char *zip_path, char *filepath, struct srtm_tile *tile) {
    char *extract_cmd =
        g_strdup_printf("cd '%s' && unzip -o -q '%s' '%s' 2>/dev/null", srtm_data_dir, zip_path, tile->filename);
    int extract_result = system(extract_cmd);
    g_free(extract_cmd);

    if (extract_result == 0 && g_file_test(filepath, G_FILE_TEST_EXISTS) && srtm_hgt_size_valid(filepath)) {
        tile->downloaded = 1;
        g_unlink(zip_path);
        g_free(zip_path);
        g_free(filepath);
        return 1;
    }
    if (g_file_test(filepath, G_FILE_TEST_EXISTS))
        g_unlink(filepath);
    return 0;
}

static int srtm_extract_nasa_hgt(char *zip_path, char *filepath, struct srtm_tile *tile) {
    char lat_dir = (tile->lat >= 0) ? 'N' : 'S';
    char lon_dir = (tile->lon >= 0) ? 'E' : 'W';
    int lat_abs = abs(tile->lat);
    int lon_abs = abs(tile->lon);
    char *nasa_name = g_strdup_printf("%c%02d%c%03d.SRTMGL1.hgt", lat_dir, lat_abs, lon_dir, lon_abs);
    char *extract_cmd =
        g_strdup_printf("cd '%s' && unzip -o -q '%s' '%s' 2>/dev/null", srtm_data_dir, zip_path, nasa_name);
    int extract_result = system(extract_cmd);
    char *nasa_path;

    g_free(extract_cmd);
    if (extract_result != 0) {
        g_free(nasa_name);
        return 0;
    }

    nasa_path = g_build_filename(srtm_data_dir, nasa_name, NULL);
    if (g_file_test(nasa_path, G_FILE_TEST_EXISTS) && g_rename(nasa_path, filepath) == 0
        && srtm_hgt_size_valid(filepath)) {
        tile->downloaded = 1;
        g_unlink(zip_path);
        g_free(zip_path);
        g_free(filepath);
        g_free(nasa_path);
        g_free(nasa_name);
        return 1;
    }
    if (g_file_test(nasa_path, G_FILE_TEST_EXISTS))
        g_unlink(nasa_path);
    g_free(nasa_path);
    g_free(nasa_name);
    return 0;
}

static int srtm_download_tile(struct srtm_tile *tile, struct srtm_download_context *ctx) {
    char *filepath = g_build_filename(srtm_data_dir, tile->filename, NULL);
    char *zip_path = g_strdup_printf("%s.zip", filepath);
    CURL *curl = curl_easy_init();
    FILE *fp;
    CURLcode res;

    if (!curl) {
        g_free(filepath);
        g_free(zip_path);
        return 0;
    }

    (void)ctx;
    curl_easy_setopt(curl, CURLOPT_USERAGENT, SRTM_CURL_USERAGENT);

    if (srtm_try_download_copernicus(curl, tile, filepath, zip_path))
        return 1;

    fp = fopen(zip_path, "wb");
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

    res = srtm_download_zip_with_fallbacks(curl, tile, fp);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && g_file_test(zip_path, G_FILE_TEST_EXISTS)) {
        if (srtm_extract_primary_hgt(zip_path, filepath, tile))
            return 1;
        if (!g_file_test(filepath, G_FILE_TEST_EXISTS) && srtm_extract_nasa_hgt(zip_path, filepath, tile))
            return 1;
        g_unlink(zip_path);
    }

    g_unlink(zip_path);
    g_free(zip_path);
    g_free(filepath);
    return 0;
}

static void srtm_download_notify(struct srtm_download_context *ctx) {
    if (ctx->progress_callback)
        ctx->progress_callback(ctx, ctx->user_data);
}

static void srtm_download_progress_callback(void *data) {
    struct srtm_download_context *ctx = (struct srtm_download_context *)data;
    GList *tile_list;
    struct srtm_tile *tile;

    if (!ctx || ctx->status != SRTM_DOWNLOAD_DOWNLOADING)
        return;

    tile_list = g_list_nth(ctx->tiles, ctx->current_tile_index);
    if (!tile_list) {
        ctx->status = SRTM_DOWNLOAD_COMPLETED;
        srtm_download_notify(ctx);
        return;
    }

    tile = (struct srtm_tile *)tile_list->data;
    if (!tile->downloaded) {
        if (srtm_download_tile(tile, ctx))
            ctx->downloaded_bytes += tile->size_bytes;
        ctx->current_tile_index++;
        srtm_download_notify(ctx);
    } else {
        ctx->current_tile_index++;
    }

    if (ctx->current_tile_index < g_list_length(ctx->tiles)) {
        struct callback *cb = callback_new_1(callback_cast(srtm_download_progress_callback), ctx);
        event_add_idle(0, cb);
    } else {
        ctx->status = SRTM_DOWNLOAD_COMPLETED;
        srtm_download_notify(ctx);
    }
}
#endif

struct srtm_download_context *srtm_download_region(struct srtm_region *region,
                                                   void (*progress_callback)(struct srtm_download_context *, void *),
                                                   void *user_data) {
#ifdef HAVE_CURL
    struct srtm_download_context *ctx = g_new0(struct srtm_download_context, 1);
    GList *t;

    ctx->region = region;
    ctx->tiles =
        srtm_calculate_tiles(region->bbox_min_lon, region->bbox_min_lat, region->bbox_max_lon, region->bbox_max_lat);
    ctx->status = SRTM_DOWNLOAD_DOWNLOADING;
    ctx->current_tile_index = 0;
    ctx->progress_callback = progress_callback;
    ctx->user_data = user_data;

    for (t = ctx->tiles; t; t = g_list_next(t)) {
        struct srtm_tile *tl = (struct srtm_tile *)t->data;
        ctx->total_bytes += tl->size_bytes;
    }

    {
        struct callback *cb = callback_new_1(callback_cast(srtm_download_progress_callback), ctx);
        event_add_idle(0, cb);
    }

    return ctx;
#else
    dbg(lvl_error, "SRTM: libcurl not available, cannot download");
    (void)region;
    (void)progress_callback;
    (void)user_data;
    return NULL;
#endif
}

int srtm_download_pause(struct srtm_download_context *ctx) {
    if (!ctx || ctx->status != SRTM_DOWNLOAD_DOWNLOADING)
        return 0;
    ctx->status = SRTM_DOWNLOAD_PAUSED;
    return 1;
}

int srtm_download_resume(struct srtm_download_context *ctx) {
    if (!ctx || ctx->status != SRTM_DOWNLOAD_PAUSED)
        return 0;
    ctx->status = SRTM_DOWNLOAD_DOWNLOADING;
    return 1;
}

int srtm_download_cancel(struct srtm_download_context *ctx) {
    if (!ctx)
        return 0;
    ctx->status = SRTM_DOWNLOAD_IDLE;
    return 1;
}
