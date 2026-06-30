/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "debug.h"
#include "driver_break.h"
#include <glib.h>
#include <string.h>

void driver_break_config_default(struct driver_break_config *config) {
    memset(config, 0, sizeof(*config));

    config->car_soft_limit_hours = 7;
    config->car_max_hours = 10;
    config->car_break_interval_hours = 4;
    config->car_break_duration_min = 30;

    config->truck_mandatory_break_after_hours = 4;
    config->truck_mandatory_break_duration_min = 45;
    config->truck_max_daily_hours = 9;

    config->hiking_driver_break_distance_main = 11295.0;
    config->hiking_driver_break_distance_alt = 2275.2;
    config->hiking_max_daily_distance = 40000.0;

    config->cycling_driver_break_distance_main = 28240.0;
    config->cycling_driver_break_distance_alt = 5690.0;
    config->cycling_max_daily_distance = 100000.0;

    config->min_distance_from_buildings = 150;
    config->min_distance_from_glaciers = 300;
    config->poi_search_radius_km = 15;
    config->poi_water_search_radius_km = 2;
    config->poi_cabin_search_radius_km = 5;
    config->network_hut_search_radius_km = 25;

    config->enable_dnt_priority = 0;
    config->enable_hiking_pilgrimage_priority = 0;

    config->driver_break_interval_min_hours = 1;
    config->driver_break_interval_max_hours = 6;

    config->use_energy_routing = 0;
    config->total_weight = 80.0;

    config->fuel_type = DRIVER_BREAK_FUEL_PETROL;
    config->fuel_tank_capacity_l = 50;
    config->fuel_avg_consumption_x10 = 75;
    config->fuel_obd_available = 0;
    config->fuel_j1939_available = 0;
    config->fuel_megasquirt_available = 0;
    config->fuel_injector_flow_cc_min = 0;
    config->fuel_ethanol_manual_pct = 0;
    config->fuel_low_warning_km = 80;
    config->fuel_search_buffer_km = 20;
    config->fuel_high_load_threshold = 25;

    config->vehicle_type = DRIVER_BREAK_VEHICLE_CAR;

    dbg(lvl_debug,
        "Driver Break plugin: Config defaults initialized - buildings=%d, glaciers=%d, poi=%d, water=%d, cabin=%d",
        config->min_distance_from_buildings, config->min_distance_from_glaciers, config->poi_search_radius_km,
        config->poi_water_search_radius_km, config->poi_cabin_search_radius_km);
}
