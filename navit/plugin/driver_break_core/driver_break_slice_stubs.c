/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Standalone-build stubs for ECU, OSD commands, SRTM, and POI slices
 * that live on other feature branches until trunk merge.
 */

#include "driver_break_osd.h"
#include "driver_break_poi.h"
#include "driver_break_poi_hiking.h"
#include "driver_break_poi_map.h"
#include "driver_break_srtm.h"
#include "driver_break_srtm_osd.h"
#include "driver_ecu.h"
#include "item.h"
#include "navit.h"
#include <glib.h>

struct driver_ecu_obd {
    int unused;
};

struct driver_ecu_j1939 {
    int unused;
};

struct driver_ecu_megasquirt {
    int unused;
};

struct driver_ecu_obd *driver_ecu_obd_start(const struct ecu_config *config, struct ecu_fuel_state *fuel) {
    (void)config;
    (void)fuel;
    return NULL;
}

void driver_ecu_obd_stop(struct driver_ecu_obd *obd) {
    (void)obd;
}

struct driver_ecu_j1939 *driver_ecu_j1939_start(const struct ecu_config *config, struct ecu_fuel_state *fuel) {
    (void)config;
    (void)fuel;
    return NULL;
}

void driver_ecu_j1939_stop(struct driver_ecu_j1939 *ctx) {
    (void)ctx;
}

struct driver_ecu_megasquirt *driver_ecu_megasquirt_start(const struct ecu_config *config,
                                                          struct ecu_fuel_state *fuel) {
    (void)config;
    (void)fuel;
    return NULL;
}

void driver_ecu_megasquirt_stop(struct driver_ecu_megasquirt *ctx) {
    (void)ctx;
}

void driver_break_register_commands(struct navit *nav) {
    (void)nav;
}

void driver_break_srtm_register_commands(struct navit *nav) {
    (void)nav;
}

int srtm_init(const char *srtm_dir) {
    (void)srtm_dir;
    return 0;
}

GList *driver_break_poi_discover(struct coord_geo *center, int radius_km, const char **poi_categories,
                                 int num_categories) {
    (void)center;
    (void)radius_km;
    (void)poi_categories;
    (void)num_categories;
    return NULL;
}

void driver_break_poi_rank(GList *pois, struct coord_geo *driver_break_stop, struct driver_break_config *config) {
    (void)pois;
    (void)driver_break_stop;
    (void)config;
}

void driver_break_poi_free_list(GList *pois) {
    if (pois) {
        g_list_free(pois);
    }
}

void driver_break_poi_free(struct driver_break_poi *poi) {
    if (!poi) {
        return;
    }
    g_free(poi->name);
    g_free(poi->type);
    g_free(poi->category);
    g_free(poi->opening_hours);
    g_free(poi);
}

GList *poi_search_water_points_map(struct coord_geo *driver_break_stop, double radius_km, struct mapset *ms) {
    (void)driver_break_stop;
    (void)radius_km;
    (void)ms;
    return NULL;
}

GList *poi_search_cabins_map(struct coord_geo *driver_break_stop, double radius_km, struct mapset *ms,
                             int enable_dnt_priority) {
    (void)driver_break_stop;
    (void)radius_km;
    (void)ms;
    (void)enable_dnt_priority;
    return NULL;
}

void poi_free_water_points(GList *water_points) {
    if (water_points) {
        g_list_free(water_points);
    }
}

void poi_free_cabins(GList *cabins) {
    if (cabins) {
        g_list_free(cabins);
    }
}

GList *driver_break_poi_map_search_fuel(struct coord_geo *center, double radius_km, struct mapset *ms,
                                        enum driver_break_vehicle_type vehicle_type, int fuel_type) {
    (void)center;
    (void)radius_km;
    (void)ms;
    (void)vehicle_type;
    (void)fuel_type;
    return NULL;
}

int driver_break_route_is_pilgrimage_hiking(struct item *item) {
    (void)item;
    return 0;
}
