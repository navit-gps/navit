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

#include "driver_break_glacier.h"
#include "config.h"
#include "debug.h"
#include "driver_break_geo.h"
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "transform.h"
#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Glacier location */
struct glacier_location {
    struct coord_geo coord;
    char *name;
    double distance; /* Distance from position */
};

static struct map_selection *glacier_build_selection(struct coord_geo *position, double radius_m) {
    struct coord c;
    struct map_selection *sel;
    int radius_internal = (int)(radius_m * 9.0);
    struct coord_rect rect;

    transform_from_geo(projection_mg, position, &c);
    rect.lu = c;
    rect.rl = c;
    rect.lu.x -= radius_internal;
    rect.lu.y += radius_internal;
    rect.rl.x += radius_internal;
    rect.rl.y -= radius_internal;

    sel = g_new0(struct map_selection, 1);
    sel->u.c_rect = rect;
    sel->order = 18;
    sel->range = item_range_all;
    return sel;
}

static struct glacier_location *glacier_location_from_item(struct item *item, struct coord_geo *position,
                                                           double radius_m) {
    struct coord c;
    struct coord_geo g;
    struct glacier_location *glacier;
    struct attr attr;
    double distance;

    item_coord_rewind(item);
    if (!item_coord_get(item, &c, 1))
        return NULL;

    transform_to_geo(projection_mg, &c, &g);
    distance = driver_break_coord_distance_geo(position, &g);
    if (distance > radius_m)
        return NULL;

    glacier = g_new0(struct glacier_location, 1);
    glacier->coord = g;
    glacier->distance = distance;
    if (item_attr_get(item, attr_label, &attr) && attr.u.str)
        glacier->name = g_strdup(attr.u.str);
    else
        glacier->name = g_strdup("Glacier");
    return glacier;
}

static GList *glacier_collect_from_map(struct map *map, struct map_selection *sel, struct coord_geo *position,
                                       double radius_m) {
    GList *glaciers = NULL;
    struct map_rect *mr = map_rect_new(map, sel);
    struct item *item;

    if (!mr)
        return NULL;

    while ((item = map_rect_get_item(mr))) {
        struct glacier_location *glacier;
        if (item->type != type_poly_glacier)
            continue;
        glacier = glacier_location_from_item(item, position, radius_m);
        if (glacier)
            glaciers = g_list_append(glaciers, glacier);
    }

    map_rect_destroy(mr);
    return glaciers;
}

/* Find nearby glaciers */
GList *glacier_find_nearby(struct coord_geo *position, double radius_km, struct mapset *ms) {
    GList *glaciers = NULL;
    double radius_m;
    struct map_selection *sel;
    struct map *map;
    struct attr map_attr;
    struct attr_iter *iter;

    if (!position || !ms || radius_km <= 0)
        return NULL;

    radius_m = radius_km * 1000.0;
    sel = glacier_build_selection(position, radius_m);
    iter = mapset_attr_iter_new(NULL);

    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        GList *found = glacier_collect_from_map(map_attr.u.map, sel, position, radius_m);
        if (found)
            glaciers = g_list_concat(glaciers, found);
    }

    mapset_attr_iter_destroy(iter);
    g_free(sel);
    return glaciers;
}

/* Check if position is too close to glacier */
int glacier_is_too_close_for_camping(struct coord_geo *position, struct mapset *ms, int has_camping_building) {
    if (!position || !ms) {
        return 0; /* Assume safe if can't check */
    }

    /* Buildings/huts suitable for camping are exempt */
    if (has_camping_building) {
        return 0;
    }

    /* Find nearby glaciers */
    GList *glaciers = glacier_find_nearby(position, 1.0, ms); /* Search 1 km radius */

    if (!glaciers) {
        return 0; /* No glaciers nearby, safe */
    }

    /* Check distance to nearest glacier */
    GList *l = glaciers;
    while (l) {
        struct glacier_location *glacier = (struct glacier_location *)l->data;
        if (glacier && glacier->distance < GLACIER_MIN_CAMPING_DISTANCE) {
            glacier_free_list(glaciers);
            return 1; /* Too close to glacier */
        }
        l = g_list_next(l);
    }

    glacier_free_list(glaciers);
    return 0; /* Safe distance from all glaciers */
}

void glacier_free_list(GList *glaciers) {
    GList *l = glaciers;
    while (l) {
        struct glacier_location *glacier = (struct glacier_location *)l->data;
        if (glacier) {
            g_free(glacier->name);
            g_free(glacier);
        }
        l = g_list_next(l);
    }
    g_list_free(glaciers);
}

double glacier_get_min_camping_distance(void) {
    return GLACIER_MIN_CAMPING_DISTANCE;
}
