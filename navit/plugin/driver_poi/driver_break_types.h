/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Shared Driver Break data types used by the POI module on this branch.
 * Full plugin configuration lives on trunk in driver_break.h after merge.
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_TYPES_H
#define NAVIT_PLUGIN_DRIVER_BREAK_TYPES_H

#include "config.h"
#include "coord.h"
#include <glib.h>

enum driver_break_vehicle_type {
    DRIVER_BREAK_VEHICLE_CAR = 0,
    DRIVER_BREAK_VEHICLE_TRUCK = 1,
    DRIVER_BREAK_VEHICLE_HIKING = 2,
    DRIVER_BREAK_VEHICLE_CYCLING = 3
};

enum driver_break_fuel_type {
    DRIVER_BREAK_FUEL_PETROL = 0,
    DRIVER_BREAK_FUEL_DIESEL = 1,
    DRIVER_BREAK_FUEL_FLEX = 2,
    DRIVER_BREAK_FUEL_CNG = 3,
    DRIVER_BREAK_FUEL_LNG = 4,
    DRIVER_BREAK_FUEL_LPG = 5,
    DRIVER_BREAK_FUEL_HYDROGEN = 6,
    DRIVER_BREAK_FUEL_ETHANOL = 7
};

struct driver_break_config {
    int vehicle_type;
    int car_soft_limit_hours;
    int car_max_hours;
    int car_break_interval_hours;
    int car_break_duration_min;
    int truck_mandatory_break_after_hours;
    int truck_mandatory_break_duration_min;
    int truck_max_daily_hours;
    double hiking_driver_break_distance_main;
    double hiking_driver_break_distance_alt;
    double hiking_max_daily_distance;
    double cycling_driver_break_distance_main;
    double cycling_driver_break_distance_alt;
    double cycling_max_daily_distance;
    int min_distance_from_buildings;
    int fuel_type;
};

struct driver_break_poi {
    struct coord_geo coord;
    char *name;
    char *type;
    char *category;
    double distance_from_driver_break_stop;
    char *opening_hours;
    int accessibility;
    double rating;
};

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_TYPES_H */
