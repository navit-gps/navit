/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Standalone stubs for finder APIs referenced by the OSD slice on this branch.
 */

#include "driver_break_finder.h"
#include <glib.h>

GList *driver_break_finder_find_near(struct coord_geo *center, double distance_km, struct driver_break_config *config) {
    (void)center;
    (void)distance_km;
    (void)config;
    return NULL;
}

int driver_break_finder_is_valid_nightly_location(struct coord_geo *coord, struct driver_break_config *config,
                                                  struct mapset *ms, int has_camping_building) {
    (void)ms;
    (void)has_camping_building;
    return driver_break_finder_is_valid_location(coord, config);
}

GList *driver_break_finder_find_along_route(struct route *route, struct driver_break_config *config,
                                            int mandatory_break_required) {
    (void)route;
    (void)config;
    (void)mandatory_break_required;
    return NULL;
}

void driver_break_finder_free_list(GList *stops) {
    GList *l;

    if (!stops) {
        return;
    }

    for (l = stops; l; l = g_list_next(l)) {
        struct driver_break_stop *stop = (struct driver_break_stop *)l->data;
        if (stop) {
            if (stop->name) {
                g_free(stop->name);
            }
            if (stop->highway_type) {
                g_free(stop->highway_type);
            }
            if (stop->pois) {
                g_list_free(stop->pois);
            }
            g_free(stop);
        }
    }
    g_list_free(stops);
}

int driver_break_finder_is_valid_location(struct coord_geo *coord, struct driver_break_config *config) {
    (void)config;
    if (!coord) {
        return 0;
    }
    if (coord->lat < -90.0 || coord->lat > 90.0) {
        return 0;
    }
    if (coord->lng < -180.0 || coord->lng > 180.0) {
        return 0;
    }
    return 1;
}

void driver_break_finder_free(struct driver_break_stop *stop) {
    if (!stop) {
        return;
    }
    if (stop->name) {
        g_free(stop->name);
    }
    if (stop->highway_type) {
        g_free(stop->highway_type);
    }
    if (stop->pois) {
        g_list_free(stop->pois);
    }
    g_free(stop);
}
