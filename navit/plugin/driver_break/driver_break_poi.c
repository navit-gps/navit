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

#include "driver_break_poi.h"
#include "config.h"
#include "coord.h"
#include "debug.h"
#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CURL
#    include <curl/curl.h>

/* Overpass API query structure */
struct overpass_query {
    char *response;
    size_t size;
};

static size_t overpass_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct overpass_query *mem = (struct overpass_query *)userp;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (!ptr) {
        return 0;
    }

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

/* Build Overpass API query for POI discovery */
static char *build_overpass_query(struct coord_geo *center, int radius_km, const char **poi_categories,
                                  int num_categories) {
    GString *query = g_string_new("[out:json][timeout:25];(");

    /* Build query for each POI category */
    int i;
    for (i = 0; i < num_categories; i++) {
        if (i > 0) {
            g_string_append(query, ";");
        }

        /* Query format: node[amenity=cafe](around:radius,lat,lng); */
        g_string_append_printf(query, "node[\"%s\"](around:%d,%.6f,%.6f);", poi_categories[i], radius_km * 1000,
                               center->lat, center->lng);
    }

    g_string_append(query, ");out body;");

    char *result = g_string_free(query, FALSE);
    return result;
}

/* Simple JSON value extractor - extracts string value from "key":"value" */
static char *extract_json_string(const char *json, const char *key, int *consumed) {
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":\"", key);
    const char *pos = strstr(json, search_pattern);
    if (!pos) {
        if (consumed)
            *consumed = 0;
        return NULL;
    }

    pos += strlen(search_pattern);
    const char *end = strchr(pos, '"');
    if (!end) {
        if (consumed)
            *consumed = 0;
        return NULL;
    }

    int len = end - pos;
    char *value = g_malloc(len + 1);
    memcpy(value, pos, len);
    value[len] = '\0';

    if (consumed)
        *consumed = (int)(end - json + 1);
    return value;
}

/* Extract JSON number (double) */
static double extract_json_number(const char *json, const char *key, int *consumed) {
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
    const char *pos = strstr(json, search_pattern);
    if (!pos) {
        if (consumed)
            *consumed = 0;
        return 0.0;
    }

    pos += strlen(search_pattern);
    /* Skip whitespace */
    while (*pos == ' ' || *pos == '\t')
        pos++;

    double value = strtod(pos, (char **)&pos);
    if (consumed)
        *consumed = (int)(pos - json);
    return value;
}

/* Create POI from one Overpass node object, or NULL if invalid. Caller frees POI. */
static struct driver_break_poi *parse_overpass_node(const char *obj_start, struct coord_geo *center) {
    double lat = extract_json_number(obj_start, "lat", NULL);
    double lon = extract_json_number(obj_start, "lon", NULL);
    if (lat == 0.0 || lon == 0.0) {
        return NULL;
    }

    long node_id = (long)extract_json_number(obj_start, "id", NULL);
    struct driver_break_poi *poi = g_new0(struct driver_break_poi, 1);
    poi->coord.lat = lat;
    poi->coord.lng = lon;

    const char *tags_start = strstr(obj_start, "\"tags\"");
    if (tags_start) {
        const char *tags_obj = strchr(tags_start, '{');
        if (tags_obj) {
            poi->name = extract_json_string(tags_obj, "name", NULL);
            poi->category = extract_json_string(tags_obj, "amenity", NULL);
            if (!poi->category) {
                poi->category = extract_json_string(tags_obj, "tourism", NULL);
            }
            if (!poi->category) {
                poi->category = extract_json_string(tags_obj, "leisure", NULL);
            }
            poi->opening_hours = extract_json_string(tags_obj, "opening_hours", NULL);
        }
    }

    if (!poi->name) {
        poi->name = g_strdup_printf("POI %ld", node_id);
    }
    if (!poi->category) {
        poi->category = g_strdup("unknown");
    }
    poi->distance_from_driver_break_stop = coord_distance(&poi->coord, center);
    return poi;
}

/* Parse Overpass API JSON response */
static GList *parse_overpass_response(const char *json_response, struct coord_geo *center) {
    GList *pois = NULL;

    if (!json_response || !center) {
        return NULL;
    }

    dbg(lvl_debug, "Driver Break plugin: Parsing Overpass API response");

    const char *elements_start = strstr(json_response, "\"elements\"");
    if (!elements_start) {
        dbg(lvl_warning, "Driver Break plugin: No 'elements' found in Overpass response");
        return NULL;
    }

    const char *array_start = strchr(elements_start, '[');
    if (!array_start) {
        return NULL;
    }

    const char *current = array_start + 1;
    int element_count = 0;

    while (*current && *current != ']') {
        const char *node_start = strstr(current, "\"type\":\"node\"");
        if (!node_start) {
            break;
        }

        const char *obj_start = node_start;
        while (obj_start > current && *obj_start != '{') {
            obj_start--;
        }
        if (*obj_start != '{') {
            current = node_start + 1;
            continue;
        }

        struct driver_break_poi *poi = parse_overpass_node(obj_start, center);
        if (poi) {
            pois = g_list_append(pois, poi);
            element_count++;
        }

        const char *obj_end = strchr(obj_start, '}');
        if (obj_end) {
            current = obj_end + 1;
        } else {
            break;
        }
    }

    dbg(lvl_info, "Driver Break plugin: Parsed %d POIs from Overpass response", element_count);
    return pois;
}

#endif /* HAVE_CURL */

#ifdef HAVE_CURL
#ifdef HAVE_CURL
/* Shared helper for Overpass queries (general and specific). */
static GList *driver_break_poi_overpass_search(struct coord_geo *center, int radius_km, const char **categories,
                                               int num_categories, int is_general) {
    CURL *curl;
    CURLcode res;
    struct overpass_query query_data;
    char *overpass_query;
    char *url;
    GList *pois = NULL;

    curl = curl_easy_init();
    if (!curl) {
        dbg(lvl_error, "Driver Break plugin: Failed to initialize libcurl");
        return NULL;
    }

    overpass_query = build_overpass_query(center, radius_km, categories, num_categories);

    /* URL encode the query */
    char *encoded_query = curl_easy_escape(curl, overpass_query, 0);
    if (!encoded_query) {
        dbg(lvl_error, "Driver Break plugin: Failed to encode Overpass query");
        curl_easy_cleanup(curl);
        g_free(overpass_query);
        return NULL;
    }

    url = g_strdup_printf("https://overpass-api.de/api/interpreter?data=%s", encoded_query);

    query_data.response = malloc(1);
    query_data.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, overpass_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&query_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        dbg(lvl_warning, "Driver Break plugin: Overpass API request failed: %s",
            curl_easy_strerror(res));
    } else {
        /* Parse response */
        pois = parse_overpass_response(query_data.response, center);
        if (!pois) {
            dbg(lvl_warning,
                "Driver Break plugin: Overpass API returned 0 POIs at lat=%.5f lon=%.5f (%s categories)", center->lat,
                center->lng, is_general ? "general" : "specific");
        } else {
            dbg(lvl_info, "Driver Break plugin: Overpass API returned POIs for %s search",
                is_general ? "general" : "specific");
        }
    }

    free(query_data.response);
    curl_free(encoded_query);
    curl_easy_cleanup(curl);
    g_free(url);
    g_free(overpass_query);

    return pois;
}
#endif /* HAVE_CURL */

/* Discover POIs using map data (preferred) or Overpass API (fallback) */
GList *driver_break_poi_discover(struct coord_geo *center, int radius_km, const char **poi_categories,
                                 int num_categories) {
    GList *pois = NULL;

    if (!center) {
        return NULL;
    }

    dbg(lvl_info, "Driver Break plugin: POI discovery at lat=%.5f lon=%.5f radius=%d km categories=%d", center->lat,
        center->lng, radius_km, num_categories);

    /* Try to get mapset from navit (if available) */
    /* For now, use Overpass API as fallback if mapset not available */
    /* This function signature is kept for compatibility */

    /* If poi_categories is NULL, search for general POIs */
    if (!poi_categories || num_categories == 0) {
#ifdef HAVE_CURL
        /* General POIs: cafes, shops, bike, etc. */
        const char *general_categories[] = {"amenity=cafe",     "amenity=restaurant",
                                            "tourism=museum",   "tourism=viewpoint",
                                            "shop=convenience", "shop=farm",
                                            "shop=supermarket", "shop=mall",
                                            "shop=bicycle",     "amenity=bicycle_repair_station"};
        pois = driver_break_poi_overpass_search(center, radius_km, general_categories,
                                                sizeof(general_categories) / sizeof(general_categories[0]), 1);
#else
        dbg(lvl_warning, "Driver Break plugin: libcurl not available, POI discovery disabled");
#endif
    } else {
        /* Use Overpass API for specific categories */
#ifdef HAVE_CURL
        pois = driver_break_poi_overpass_search(center, radius_km, poi_categories, num_categories, 0);
#else
        dbg(lvl_warning, "Driver Break plugin: libcurl not available, POI discovery disabled");
#endif
    }

    return pois;
}

/* Calculate distance between two coordinates */
static double coord_distance(struct coord_geo *c1, struct coord_geo *c2) {
    double lat1 = c1->lat * M_PI / 180.0;
    double lat2 = c2->lat * M_PI / 180.0;
    double dlat = (c2->lat - c1->lat) * M_PI / 180.0;
    double dlng = (c2->lng - c1->lng) * M_PI / 180.0;

    double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlng / 2) * sin(dlng / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return 6371000.0 * c; /* Earth radius in meters */
}

/* Compare function for sorting POIs by distance */
static int driver_break_poi_compare_distance(gconstpointer a, gconstpointer b) {
    const struct driver_break_poi *poi_a = (const struct driver_break_poi *)a;
    const struct driver_break_poi *poi_b = (const struct driver_break_poi *)b;

    if (!poi_a || !poi_b)
        return 0;
    if (poi_a->distance_from_driver_break_stop < poi_b->distance_from_driver_break_stop)
        return -1;
    if (poi_a->distance_from_driver_break_stop > poi_b->distance_from_driver_break_stop)
        return 1;
    return 0;
}

/* Rank POIs by distance and other factors */
void driver_break_poi_rank(GList *pois, struct coord_geo *driver_break_stop, struct driver_break_config *config) {
    GList *l;

    if (!pois || !driver_break_stop) {
        return;
    }

    for (l = pois; l; l = g_list_next(l)) {
        struct driver_break_poi *poi = (struct driver_break_poi *)l->data;
        if (poi) {
            /* Calculate distance from rest stop */
            poi->distance_from_driver_break_stop = coord_distance(&poi->coord, driver_break_stop);

            /* Simple ranking: prefer closer POIs */
            /* Full implementation would consider:
             * - User preferences
             * - Operating hours
             * - Ratings
             * - Accessibility
             */
        }
    }

    /* Sort by distance (closest first) */
    g_list_sort(pois, driver_break_poi_compare_distance);
}

void driver_break_poi_free_list(GList *pois) {
    GList *l = pois;
    while (l) {
        driver_break_poi_free((struct driver_break_poi *)l->data);
        l = g_list_next(l);
    }
    g_list_free(pois);
}

void driver_break_poi_free(struct driver_break_poi *poi) {
    if (poi) {
        g_free(poi->name);
        g_free(poi->type);
        g_free(poi->category);
        g_free(poi->opening_hours);
        g_free(poi);
    }
}
