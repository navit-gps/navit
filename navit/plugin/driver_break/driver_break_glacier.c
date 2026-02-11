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
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include "debug.h"
#include "driver_break_glacier.h"
#include "map.h"
#include "mapset.h"
#include "item.h"
#include "transform.h"

/* Calculate distance between two coordinates */
static double coord_distance(struct coord_geo *c1, struct coord_geo *c2) {
    double lat1 = c1->lat * M_PI / 180.0;
    double lat2 = c2->lat * M_PI / 180.0;
    double dlat = (c2->lat - c1->lat) * M_PI / 180.0;
    double dlng = (c2->lng - c1->lng) * M_PI / 180.0;

    double a = sin(dlat/2) * sin(dlat/2) +
               cos(lat1) * cos(lat2) *
               sin(dlng/2) * sin(dlng/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));

    return 6371000.0 * c; /* Earth radius in meters */
}

/* Glacier location */
struct glacier_location {
    struct coord_geo coord;
    char *name;
    double distance;  /* Distance from position */
};

/* Find nearby glaciers */
GList *glacier_find_nearby(struct coord_geo *position, double radius_km, struct mapset *ms) {
    GList *glaciers = NULL;

    if (!position || !ms || radius_km <= 0) {
        return NULL;
    }

    /* Convert radius to meters */
    double radius_m = radius_km * 1000.0;

    /* Create map selection around position */
    struct coord c;
    transform_from_geo(projection_mg, position, &c);

    struct coord_rect rect;
    rect.lu = c;
    rect.rl = c;
    int radius_internal = (int)(radius_m * 9.0); /* Approximate conversion */
    rect.lu.x -= radius_internal;
    rect.lu.y += radius_internal;
    rect.rl.x += radius_internal;
    rect.rl.y -= radius_internal;

    struct map_selection *sel = g_new0(struct map_selection, 1);
    sel->u.c_rect = rect;
    sel->order = 18;
    sel->range = item_range_all;

    /* Search for glaciers in mapset */
    struct map *map;
    struct attr map_attr;
    struct attr_iter *iter = mapset_attr_iter_new(NULL);

    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        struct map_rect *mr = map_rect_new(map, sel);
        if (!mr) continue;

        struct item *item;
        while ((item = map_rect_get_item(mr))) {
            /* Check for glacier polygons */
            if (item->type == type_poly_glacier) {
                /* Get polygon coordinates */
                struct coord c;
                struct coord_geo g;
                item_coord_rewind(item);

                /* Get first coordinate to represent glacier location */
                if (item_coord_get(item, &c, 1)) {
                    transform_to_geo(projection_mg, &c, &g);

                    /* Calculate distance from position */
                    double distance = coord_distance(position, &g);

                    if (distance <= radius_m) {
                        struct glacier_location *glacier = g_new0(struct glacier_location, 1);
                        glacier->coord = g;
                        glacier->distance = distance;

                        /* Try to get name from item attributes */
                        struct attr attr;
                        if (item_attr_get(item, attr_label, &attr) && attr.u.str) {
                            glacier->name = g_strdup(attr.u.str);
                        } else {
                            glacier->name = g_strdup("Glacier");
                        }

                        glaciers = g_list_append(glaciers, glacier);
                    }
                }
            }
        }

        map_rect_destroy(mr);
    }

    mapset_attr_iter_destroy(iter);
    g_free(sel);

    return glaciers;
}

/* Check if position is too close to glacier */
int glacier_is_too_close_for_camping(struct coord_geo *position,
                                      struct mapset *ms,
                                      int has_camping_building) {
    if (!position || !ms) {
        return 0;  /* Assume safe if can't check */
    }

    /* Buildings/huts suitable for camping are exempt */
    if (has_camping_building) {
        return 0;
    }

    /* Find nearby glaciers */
    GList *glaciers = glacier_find_nearby(position, 1.0, ms);  /* Search 1 km radius */

    if (!glaciers) {
        return 0;  /* No glaciers nearby, safe */
    }

    /* Check distance to nearest glacier */
    GList *l = glaciers;
    while (l) {
        struct glacier_location *glacier = (struct glacier_location *)l->data;
        if (glacier && glacier->distance < GLACIER_MIN_CAMPING_DISTANCE) {
            glacier_free_list(glaciers);
            return 1;  /* Too close to glacier */
        }
        l = g_list_next(l);
    }

    glacier_free_list(glaciers);
    return 0;  /* Safe distance from all glaciers */
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
