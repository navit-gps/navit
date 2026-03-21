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

#include "driver_break_poi_map.h"
#include "attr.h"
#include "config.h"
#include "debug.h"
#include "driver_break_poi.h"
#include "item.h"
#include "map.h"
#include "projection.h"
#include "transform.h"
#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
/* item_def.h included via item.h */

/* Calculate distance between two coordinates in meters */
static double coord_distance_geo(struct coord_geo *c1, struct coord_geo *c2) {
    double lat1 = c1->lat * M_PI / 180.0;
    double lat2 = c2->lat * M_PI / 180.0;
    double dlat = (c2->lat - c1->lat) * M_PI / 180.0;
    double dlng = (c2->lng - c1->lng) * M_PI / 180.0;

    double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlng / 2) * sin(dlng / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return 6371000.0 * c; /* Earth radius in meters */
}

/* Sort helper for fuel POIs by distance */
static gint driver_break_poi_compare_distance(gconstpointer a, gconstpointer b) {
    const struct driver_break_poi *pa = (const struct driver_break_poi *)a;
    const struct driver_break_poi *pb = (const struct driver_break_poi *)b;

    if (!pa || !pb)
        return 0;
    if (pa->distance_from_driver_break_stop < pb->distance_from_driver_break_stop)
        return -1;
    if (pa->distance_from_driver_break_stop > pb->distance_from_driver_break_stop)
        return 1;
    return 0;
}

/* Extract OSM tag value from item */
static char *get_osm_tag_value(struct item *item, const char *key) {
    struct attr attr;
    char tag_key[256];
    char *tag_value = NULL;

    /* Try to get OSM tag attribute */
    if (item_attr_get(item, attr_osm_tag, &attr)) {
        /* OSM tags are stored as "key=value" pairs */
        const char *tags = attr.u.str;
        if (tags) {
            snprintf(tag_key, sizeof(tag_key), "%s=", key);
            const char *pos = strstr(tags, tag_key);
            if (pos) {
                pos += strlen(tag_key);
                const char *end = strchr(pos, ' ');
                if (!end)
                    end = pos + strlen(pos);
                int len = end - pos;
                if (len > 0) {
                    tag_value = g_malloc(len + 1);
                    memcpy(tag_value, pos, len);
                    tag_value[len] = '\0';
                }
            }
        }
    }

    return tag_value;
}

/* Check if POI item has DNT/network tags */
int driver_break_poi_is_network_cabin(struct item *item, char **network_name) {
    if (!item || !network_name) {
        return 0;
    }

    *network_name = NULL;

    /* Check for operator tags (DNT, STF, etc.) */
    char *operator= get_osm_tag_value(item, "operator");
    if (operator) {
        char *op_lower = g_ascii_strdown(operator, - 1);

        if (strstr(op_lower, "dnt") || strstr(op_lower, "den norske turistforening")) {
            *network_name = g_strdup("DNT");
            g_free(op_lower);
            g_free(operator);
            return 1;
        } else if (strstr(op_lower, "stf") || strstr(op_lower, "svenska turistforeningen")) {
            *network_name = g_strdup("STF");
            g_free(op_lower);
            g_free(operator);
            return 1;
        }

        g_free(op_lower);
        g_free(operator);
    }

    /* Check for network tag */
    char *network = get_osm_tag_value(item, "network");
    if (network) {
        *network_name = g_strdup(network);
        g_free(network);
        return 1;
    }

    return 0;
}

/* Check if route segment has pilgrimage/hiking route tags */
int driver_break_route_is_pilgrimage_hiking(struct item *item) {
    if (!item) {
        return 0;
    }

    /* Check for route=hiking tag */
    char *route = get_osm_tag_value(item, "route");
    if (route) {
        char *route_lower = g_ascii_strdown(route, -1);
        int is_hiking = (strcmp(route_lower, "hiking") == 0);
        g_free(route_lower);
        g_free(route);
        if (is_hiking) {
            return 1;
        }
    }

    /* Check for pilgrimage=yes tag */
    char *pilgrimage = get_osm_tag_value(item, "pilgrimage");
    if (pilgrimage) {
        char *pil_lower = g_ascii_strdown(pilgrimage, -1);
        int is_pilgrimage = (strcmp(pil_lower, "yes") == 0);
        g_free(pil_lower);
        g_free(pilgrimage);
        if (is_pilgrimage) {
            return 1;
        }
    }

    return 0;
}

/* Return 1 if item type is in the list */
static int item_type_matches(struct item *item, enum item_type *poi_types, int num_types) {
    int i;
    for (i = 0; i < num_types; i++) {
        if (item->type == poi_types[i]) {
            return 1;
        }
    }
    return 0;
}

/* Create a driver_break_poi from a map item. Caller frees POI. */
static struct driver_break_poi *poi_from_map_item(struct item *item, struct coord_geo *item_geo, double distance) {
    struct driver_break_poi *poi = g_new0(struct driver_break_poi, 1);
    struct attr label_attr;
    struct attr hours_attr;

    poi->coord = *item_geo;
    poi->distance_from_driver_break_stop = distance;
    poi->type = g_strdup("poi");
    poi->category = g_strdup(item_to_name(item->type));

    if (item_attr_get(item, attr_label, &label_attr)) {
        poi->name = g_strdup(map_convert_string(item->map, label_attr.u.str));
    }
    if (item_attr_get(item, attr_open_hours, &hours_attr)) {
        poi->opening_hours = g_strdup(hours_attr.u.str);
    }
    return poi;
}

/* Check if fuel station item matches vehicle fuel profile (based on OSM fuel:* tags).
 * fuel_type is enum driver_break_fuel_type. vehicle_type distinguishes car vs truck. */
static int fuel_station_matches_profile(struct item *item, enum driver_break_vehicle_type vehicle_type, int fuel_type);

/* Collect POIs from one map that match types and are within radius_m of center. Caller owns sel. */
static GList *collect_pois_from_map(struct map *map, struct map_selection *sel, struct coord_geo *center,
                                    double radius_m, enum item_type *poi_types, int num_types) {
    GList *pois = NULL;
    struct map_rect *mr = map_rect_new(map, sel);
    if (!mr)
        return NULL;

    struct item *item;
    while ((item = map_rect_get_item(mr))) {
        if (!item_type_matches(item, poi_types, num_types))
            continue;
        struct coord item_coord;
        if (!item_coord_get(item, &item_coord, 1))
            continue;
        struct coord_geo item_geo;
        transform_to_geo(projection_mg, &item_coord, &item_geo);
        double distance = coord_distance_geo(center, &item_geo);
        if (distance > radius_m)
            continue;
        struct driver_break_poi *poi = poi_from_map_item(item, &item_geo, distance);
        pois = g_list_append(pois, poi);
    }
    map_rect_destroy(mr);
    return pois;
}

/* Search for POIs in maps near a coordinate */
GList *driver_break_poi_map_search(struct coord_geo *center, double radius_km, enum item_type *poi_types, int num_types,
                                   struct mapset *ms) {
    if (!center || !ms || radius_km <= 0 || !poi_types || num_types == 0)
        return NULL;

    struct coord c;
    transform_from_geo(projection_mg, center, &c);
    double radius_m = radius_km * 1000.0;
    int radius_internal = (int)(radius_m * 9.0);

    struct coord_rect rect;
    rect.lu = c;
    rect.rl = c;
    rect.lu.x -= radius_internal;
    rect.lu.y += radius_internal;
    rect.rl.x += radius_internal;
    rect.rl.y -= radius_internal;

    struct map_selection *sel = g_new0(struct map_selection, 1);
    sel->u.c_rect = rect;
    sel->order = 18;
    sel->range = item_range_all;

    GList *pois = NULL;
    struct map *map;
    struct attr map_attr;
    struct attr_iter *iter = mapset_attr_iter_new(NULL);
    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        GList *from_map = collect_pois_from_map(map, sel, center, radius_m, poi_types, num_types);
        GList *node = from_map;
        while (node) {
            pois = g_list_append(pois, node->data);
            node = g_list_next(node);
        }
        g_list_free(from_map);
    }
    mapset_attr_iter_destroy(iter);
    g_free(sel);
    return pois;
}

/* Search for water points in maps */
GList *driver_break_poi_map_search_water(struct coord_geo *center, double radius_km, struct mapset *ms) {
    enum item_type water_types[] = {type_poi_potable_water, type_poi_fountain, type_none};

    return driver_break_poi_map_search(center, radius_km, water_types, 2, ms);
}

/* Process cabin item and add to POI list if within radius */
static void process_cabin_item(struct item *item, struct coord_geo *center, double radius_m, GList **pois) {
    struct coord item_coord;
    if (!item_coord_get(item, &item_coord, 1)) {
        return;
    }

    struct coord_geo item_geo;
    transform_to_geo(projection_mg, &item_coord, &item_geo);

    double distance = coord_distance_geo(center, &item_geo);
    if (distance > radius_m) {
        return;
    }

    /* Create POI structure */
    struct driver_break_poi *poi = g_new0(struct driver_break_poi, 1);
    poi->coord = item_geo;
    poi->distance_from_driver_break_stop = distance;

    /* Get name/label */
    struct attr label_attr;
    if (item_attr_get(item, attr_label, &label_attr)) {
        poi->name = g_strdup(map_convert_string(item->map, label_attr.u.str));
    }

    poi->type = g_strdup("tourism");
    poi->category = g_strdup(item_to_name(item->type));

    /* Check for DNT/network tags */
    char *network_name = NULL;
    if (driver_break_poi_is_network_cabin(item, &network_name)) {
        if (network_name && poi->name) {
            char *new_name = g_strdup_printf("%s [%s]", poi->name, network_name);
            g_free(poi->name);
            poi->name = new_name;
        } else if (network_name) {
            poi->name = g_strdup_printf("[%s]", network_name);
        }
        g_free(network_name);
    }

    *pois = g_list_append(*pois, poi);
}

/* Search for cabins/huts in maps (with DNT/network detection) */
GList *driver_break_poi_map_search_cabins(struct coord_geo *center, double radius_km, struct mapset *ms) {
    GList *pois = NULL;

    if (!center || !ms || radius_km <= 0) {
        return NULL;
    }

    enum item_type cabin_types[] = {type_poi_hostel, type_poi_camping, type_none};

    double radius_m = radius_km * 1000.0;
    struct coord c;
    transform_from_geo(projection_mg, center, &c);

    int radius_internal = (int)(radius_m * 9.0);
    struct coord_rect rect;
    rect.lu = c;
    rect.rl = c;
    rect.lu.x -= radius_internal;
    rect.lu.y += radius_internal;
    rect.rl.x += radius_internal;
    rect.rl.y -= radius_internal;

    struct map_selection *sel = g_new0(struct map_selection, 1);
    sel->u.c_rect = rect;
    sel->order = 18;
    sel->range = item_range_all;

    struct map *map;
    struct attr map_attr;
    struct attr_iter *iter = mapset_attr_iter_new(NULL);

    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        struct map_rect *mr = map_rect_new(map, sel);
        if (!mr) {
            continue;
        }

        struct item *item;
        while ((item = map_rect_get_item(mr))) {
            int matches = 0;
            int i;
            for (i = 0; cabin_types[i] != type_none; i++) {
                if (item->type == cabin_types[i]) {
                    matches = 1;
                    break;
                }
            }

            if (matches) {
                process_cabin_item(item, center, radius_m, &pois);
            }
        }

        map_rect_destroy(mr);
    }

    mapset_attr_iter_destroy(iter);
    g_free(sel);

    return pois;
}

/* Search for car POIs in maps (including convenience, stores, malls, bike shops/repair) */
GList *driver_break_poi_map_search_car_pois(struct coord_geo *center, double radius_km, struct mapset *ms) {
    enum item_type car_poi_types[] = {type_poi_cafe,
                                      type_poi_restaurant,
                                      type_poi_museum_history,
                                      type_poi_viewpoint,
                                      type_poi_zoo,
                                      type_poi_picnic,
                                      type_poi_attraction,
                                      type_poi_shop_grocery, /* convenience store, grocery */
                                      type_poi_shopping,     /* supermarket, general store */
                                      type_poi_mall,
                                      type_poi_shop_bicycle, /* bike shop, parts, repairs */
                                      type_poi_repair_service,
                                      type_none};

    return driver_break_poi_map_search(center, radius_km, car_poi_types, 12, ms);
}

/* Search for motorcycle POIs: same as car (cafe, restaurant, viewpoint, picnic, repair, shops); motorcycle_repair and
 * shop=motorcycle when mapped to repair/shop types */
GList *driver_break_poi_map_search_motorcycle_pois(struct coord_geo *center, double radius_km, struct mapset *ms) {
    enum item_type mc_poi_types[] = {type_poi_cafe,
                                     type_poi_restaurant,
                                     type_poi_museum_history,
                                     type_poi_viewpoint,
                                     type_poi_picnic,
                                     type_poi_attraction,
                                     type_poi_shop_grocery,
                                     type_poi_shopping,
                                     type_poi_mall,
                                     type_poi_repair_service,
                                     type_none};
    return driver_break_poi_map_search(center, radius_km, mc_poi_types, 10, ms);
}

/* Check if item has any of the given OSM fuel:* tag keys (presence only). */
static int fuel_has_any_tag(struct item *it, const char **keys) {
    int i;
    for (i = 0; keys[i]; i++) {
        char *val = get_osm_tag_value(it, keys[i]);
        if (val) {
            g_free(val);
            return 1;
        }
    }
    return 0;
}

/* Tag sets for fuel-type matching (OSM fuel:* keys). -1 = any vehicle. */
#define FUEL_ANY_VEHICLE (-1)
static const char *diesel_tags_car[] = {
    "fuel:diesel",        "fuel:diesel_b7",      "fuel:diesel_b10", "fuel:biodiesel",
    "fuel:diesel:class2", "fuel:taxfree_diesel", "fuel:b7",         NULL};
static const char *diesel_tags_truck[] = {
    "fuel:HGV_diesel", "fuel:diesel",        "fuel:diesel_b7",      "fuel:diesel_b10",
    "fuel:biodiesel",  "fuel:diesel:class2", "fuel:taxfree_diesel", NULL};
static const char *petrol_eu_tags[] = {"fuel:octane_95", "fuel:octane_98", "fuel:octane_100",
                                       "fuel:regular",   "fuel:premium",   "fuel:ethanol_free",
                                       "fuel:e5",        "fuel:e10",       NULL};
static const char *petrol_us_tags[] = {
    "fuel:octane_87", "fuel:octane_89", "fuel:octane_91", "fuel:octane_93", "fuel:regular", "fuel:premium", NULL};
static const char *flex_tags[] = {"fuel:e85", "fuel:e100", "fuel:ethanol", "fuel:ethanol_free", "fuel:e5",
                                  "fuel:e10", NULL};
static const char *cng_tags[] = {"fuel:cng", "fuel:GNV", "fuel:biogas", NULL};
static const char *lng_tags[] = {"fuel:lng", NULL};
static const char *lpg_tags[] = {"fuel:lpg", "fuel:GPL", NULL};
static const char *hydrogen_tags[] = {"fuel:h70", "fuel:h35", "fuel:LH2", NULL};

/* One or more tag sets per profile; n_sets is 1..3. */
struct fuel_profile_row {
    int fuel_type;
    int vehicle_type;
    const char **sets[3];
    int n_sets;
};
static const struct fuel_profile_row fuel_profile_table[] = {
    {DRIVER_BREAK_FUEL_DIESEL,   DRIVER_BREAK_VEHICLE_TRUCK, {diesel_tags_truck, NULL, NULL},             1},
    {DRIVER_BREAK_FUEL_DIESEL,   FUEL_ANY_VEHICLE,           {diesel_tags_car, NULL, NULL},               1},
    {DRIVER_BREAK_FUEL_PETROL,   FUEL_ANY_VEHICLE,           {petrol_eu_tags, petrol_us_tags, NULL},      2},
    {DRIVER_BREAK_FUEL_FLEX,     FUEL_ANY_VEHICLE,           {flex_tags, petrol_eu_tags, petrol_us_tags}, 3},
    {DRIVER_BREAK_FUEL_CNG,      FUEL_ANY_VEHICLE,           {cng_tags, NULL, NULL},                      1},
    {DRIVER_BREAK_FUEL_LNG,      FUEL_ANY_VEHICLE,           {lng_tags, NULL, NULL},                      1},
    {DRIVER_BREAK_FUEL_LPG,      FUEL_ANY_VEHICLE,           {lpg_tags, NULL, NULL},                      1},
    {DRIVER_BREAK_FUEL_HYDROGEN, FUEL_ANY_VEHICLE,           {hydrogen_tags, NULL, NULL},                 1},
    {DRIVER_BREAK_FUEL_ETHANOL,  FUEL_ANY_VEHICLE,           {flex_tags, NULL, NULL},                     1},
};
#define NUM_FUEL_PROFILE_ROWS (sizeof(fuel_profile_table) / sizeof(fuel_profile_table[0]))

static int fuel_station_matches_profile(struct item *item, enum driver_break_vehicle_type vehicle_type, int fuel_type) {
    if (!item)
        return 0;
    /* Motorcycle uses same fuel profiles as car (petrol, etc.) */
    if (vehicle_type == DRIVER_BREAK_VEHICLE_MOTORCYCLE)
        vehicle_type = DRIVER_BREAK_VEHICLE_CAR;

    /* Optional amenity=fuel check; some maps still map as fuel POI without it. */
    {
        char *v = get_osm_tag_value(item, "amenity");
        if (v)
            g_free(v);
    }

    {
        size_t r;
        for (r = 0; r < NUM_FUEL_PROFILE_ROWS; r++) {
            const struct fuel_profile_row *row = &fuel_profile_table[r];
            if (row->fuel_type != fuel_type)
                continue;
            if (row->vehicle_type != FUEL_ANY_VEHICLE && row->vehicle_type != (int)vehicle_type)
                continue;
            for (int i = 0; i < row->n_sets && row->sets[i]; i++) {
                if (fuel_has_any_tag(item, row->sets[i]))
                    return 1;
            }
            /* Row matched but no tag set matched; accept generic. */
            return 1;
        }
    }

    /* Unknown fuel_type: accept generic fuel station. */
    return 1;
}

/* Search for fuel stations matching vehicle fuel type.
 * This implementation iterates maps directly so we can inspect full OSM tag sets
 * and apply fuel:* matching logic per vehicle profile. */
GList *driver_break_poi_map_search_fuel(struct coord_geo *center, double radius_km, struct mapset *ms,
                                        enum driver_break_vehicle_type vehicle_type, int fuel_type) {
    GList *result = NULL;

    if (!center || !ms || radius_km <= 0) {
        return NULL;
    }

    double radius_m = radius_km * 1000.0;
    struct coord c;
    transform_from_geo(projection_mg, center, &c);

    int radius_internal = (int)(radius_m * 9.0);
    struct coord_rect rect;
    rect.lu = c;
    rect.rl = c;
    rect.lu.x -= radius_internal;
    rect.lu.y += radius_internal;
    rect.rl.x += radius_internal;
    rect.rl.y -= radius_internal;

    struct map_selection *sel = g_new0(struct map_selection, 1);
    sel->u.c_rect = rect;
    sel->order = 18;
    sel->range = item_range_all;

    struct map *map;
    struct attr map_attr;
    struct attr_iter *iter = mapset_attr_iter_new(NULL);

    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        struct map_rect *mr = map_rect_new(map, sel);
        if (!mr) {
            continue;
        }

        struct item *item;
        while ((item = map_rect_get_item(mr))) {
            if (item->type != type_poi_fuel) {
                continue;
            }

            struct coord item_coord;
            if (!item_coord_get(item, &item_coord, 1)) {
                continue;
            }

            struct coord_geo item_geo;
            transform_to_geo(projection_mg, &item_coord, &item_geo);
            double distance = coord_distance_geo(center, &item_geo);
            if (distance > radius_m) {
                continue;
            }

            if (!fuel_station_matches_profile(item, vehicle_type, fuel_type)) {
                continue;
            }

            struct driver_break_poi *poi = poi_from_map_item(item, &item_geo, distance);
            result = g_list_append(result, poi);
        }

        map_rect_destroy(mr);
    }

    mapset_attr_iter_destroy(iter);
    g_free(sel);

    /* Sort by distance (ascending) so callers can treat list as ranked by detour distance. */
    result = g_list_sort(result, driver_break_poi_compare_distance);

    return result;
}
