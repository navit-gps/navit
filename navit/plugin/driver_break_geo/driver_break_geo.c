/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Shared geodesy helpers for the Driver Break plugin.
 */

#include "driver_break_geo.h"

#include <math.h>

#include "item.h"
#include "map.h"
#include "projection.h"
#include "route.h"
#include "transform.h"

double driver_break_coord_distance_geo(const struct coord_geo *c1, const struct coord_geo *c2) {
    double lat1;
    double lat2;
    double dlat;
    double dlng;
    double a;
    double c;

    if (!c1 || !c2)
        return 0.0;

    lat1 = c1->lat * M_PI / 180.0;
    lat2 = c2->lat * M_PI / 180.0;
    dlat = (c2->lat - c1->lat) * M_PI / 180.0;
    dlng = (c2->lng - c1->lng) * M_PI / 180.0;

    a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlng / 2) * sin(dlng / 2);
    c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return 6371000.0 * c;
}

static int driver_break_route_collect_coord(struct coord_geo *geo, void *user_data) {
    double *bbox = (double *)user_data;

    if (geo->lng < bbox[0])
        bbox[0] = geo->lng;
    if (geo->lat < bbox[1])
        bbox[1] = geo->lat;
    if (geo->lng > bbox[2])
        bbox[2] = geo->lng;
    if (geo->lat > bbox[3])
        bbox[3] = geo->lat;
    return 1;
}

static int driver_break_route_foreach_coord(struct route *route, int (*cb)(struct coord_geo *geo, void *user_data),
                                            void *user_data) {
    struct map *route_map;
    struct map_rect *mr;
    struct item *item;
    int count = 0;

    if (!route || !cb)
        return 0;

    route_map = route_get_map(route);
    if (!route_map)
        return 0;

    mr = map_rect_new(route_map, NULL);
    if (!mr)
        return 0;

    while ((item = map_rect_get_item(mr))) {
        struct coord c;
        struct coord_geo geo;

        if (item->type != type_street_route)
            continue;

        item_coord_rewind(item);
        while (item_coord_get(item, &c, 1) > 0) {
            transform_to_geo(projection_mg, &c, &geo);
            if (!cb(&geo, user_data))
                goto done;
            count++;
        }
    }

done:
    map_rect_destroy(mr);
    return count;
}

int driver_break_route_bbox(struct route *route, double *min_lon, double *min_lat, double *max_lon, double *max_lat) {
    double bbox[4] = {180.0, 90.0, -180.0, -90.0};
    int n;

    if (!min_lon || !min_lat || !max_lon || !max_lat)
        return 0;

    n = driver_break_route_foreach_coord(route, driver_break_route_collect_coord, bbox);
    if (n < 1)
        return 0;

    *min_lon = bbox[0];
    *min_lat = bbox[1];
    *max_lon = bbox[2];
    *max_lat = bbox[3];
    return 1;
}
