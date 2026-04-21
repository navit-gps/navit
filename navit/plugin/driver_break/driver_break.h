/**
 * @file rest.h
 * @brief Driver Break plugin public API and data structures
 *
 * This file defines the public API for the Driver Break plugin, including
 * configuration structures, data types, and forward declarations.
 * For detailed API documentation, see https://doxygen.navit-project.org/
 *
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

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_H
#define NAVIT_PLUGIN_DRIVER_BREAK_H

#include "attr.h"
#include "config.h"
#include "coord.h"
#include "item.h"
#include <glib.h>
#include <time.h>

/**
 * @brief Vehicle type enumeration
 */
enum driver_break_vehicle_type {
    DRIVER_BREAK_VEHICLE_CAR = 0,       /**< Car vehicle type */
    DRIVER_BREAK_VEHICLE_TRUCK = 1,     /**< Truck vehicle type (EU Regulation EC 561/2006) */
    DRIVER_BREAK_VEHICLE_HIKING = 2,    /**< Hiking vehicle type */
    DRIVER_BREAK_VEHICLE_CYCLING = 3,   /**< Cycling vehicle type */
    DRIVER_BREAK_VEHICLE_MOTORCYCLE = 4 /**< Motorcycle (road or adventure/dual-sport) */
};

/** Motorcycle terrain sub-type: road (paved only) or adventure (unpaved/track, access-restricted) */
enum driver_break_motorcycle_terrain {
    DRIVER_BREAK_MC_TERRAIN_ROAD = 0,     /**< Paved only (surface=asphalt, paved) */
    DRIVER_BREAK_MC_TERRAIN_ADVENTURE = 1 /**< Allow gravel/track; strict access (no utmark/private) */
};

/**
 * @brief Fuel type enumeration for vehicle profile
 */
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

/**
 * @brief Rest period and fuel configuration structure
 *
 * Contains all configurable parameters for rest stop management,
 * including vehicle-specific limits, POI search radii, routing options,
 * and basic fuel profile / range estimation settings.
 */
struct driver_break_config {
    int vehicle_type; /* 0=car, 1=truck, 2=hiking, 3=cycling, 4=motorcycle */

    /* Car settings */
    int car_soft_limit_hours;     /* 5-9 hours */
    int car_max_hours;            /* 10-12 hours */
    int car_break_interval_hours; /* 4.5 hours */
    int car_break_duration_min;   /* 15-45 minutes */

    /* Truck settings (EU Regulation EC 561/2006) */
    int truck_mandatory_break_after_hours;  /* 4.5 hours */
    int truck_mandatory_break_duration_min; /* 45 minutes */
    int truck_max_daily_hours;              /* 9 hours */

    /* Hiking settings */
    double hiking_driver_break_distance_main; /* 11.295 km */
    double hiking_driver_break_distance_alt;  /* 2.275 km */
    double hiking_max_daily_distance;         /* 40 km */

    /* Cycling settings */
    double cycling_driver_break_distance_main; /* 28.24 km */
    double cycling_driver_break_distance_alt;  /* 5.69 km */
    double cycling_max_daily_distance;         /* 100 km */

    /* Motorcycle settings */
    int motorcycle_soft_limit_minutes;            /* Soft limit (e.g. 120 = 2 h) */
    int motorcycle_mandatory_break_after_minutes; /* Mandatory break after (e.g. 210 = 3.5 h) */
    int motorcycle_break_duration_min;            /* Break duration 15-30 min */
    int motorcycle_terrain_subtype;               /* enum driver_break_motorcycle_terrain: road=0, adventure=1 */
    int motorcycle_adventure_max_smoothness;      /* Max smoothness (0=excellent..4=very_bad); e.g. 3=bad */
    int motorcycle_adventure_max_tracktype;       /* Max tracktype grade (1-3 for adventure) */
    int motorcycle_default_weight_kg;             /* Rider+bike for energy (e.g. 250 kg) */

    /* Rest stop settings */
    int min_distance_from_buildings;   /* 150 meters */
    int min_distance_from_glaciers;    /* 300 meters for nightly */
    int poi_search_radius_km;          /* 15 km */
    int poi_water_search_radius_km;    /* 2 km */
    int enable_water_pois_remote_arid; /* 1 = include water POIs for car/truck/motorcycle (remote/arid/hot) */
    int poi_cabin_search_radius_km;    /* 5 km */
    int network_hut_search_radius_km;  /* 25 km for network huts when DNT priority enabled */

    /* Network and route priority */
    int enable_dnt_priority;               /* Enable DNT/network priority mode (0=disabled, 1=enabled) */
    int enable_hiking_pilgrimage_priority; /* Hiking/cycling: pilgrimage route priority + place_of_worship POIs at rest
                                              stops */

    /* Rest interval range */
    int driver_break_interval_min_hours; /* 1 hour */
    int driver_break_interval_max_hours; /* 6 hours */

    /* Energy-based routing */
    int use_energy_routing;         /* 1 to enable energy-based (kinetic) routing */
    int use_ecu_route_cost;         /* 1 to prefer live ECU fuel data in route cost (eco) */
    double total_weight;            /* Total mass for energy model (kg) */
    double energy_drag_cd;          /* Aerodynamic drag coefficient Cd (dimensionless) */
    double energy_frontal_area_sqm; /* Frontal area A (m^2); air drag uses 0.5 * rho * Cd * A * v^2 */

    /* Fuel profile and range estimation (per-vehicle) */
    int fuel_type;                      /* enum driver_break_fuel_type */
    int fuel_tank_capacity_l;           /* Tank capacity in liters (or equivalent unit for gas fuels) */
    int fuel_avg_consumption_x10;       /* Average consumption in 0.1 L/100km units */
    int fuel_obd_available;             /* 1 if OBD-II adapter available (auto-detected or user-set) */
    int fuel_j1939_available;           /* 1 if J1939 available (truck mode) */
    int fuel_megasquirt_available;      /* 1 if MegaSquirt serial backend should be used */
    int fuel_injector_flow_cc_min;      /* Injector flow rate (cc/min at rated pressure) for MegaSquirt */
    int fuel_ethanol_manual_pct;        /* Manual ethanol % for flex-fuel when PID 0x52 unavailable (0-100) */
    int fuel_low_warning_km;            /* Low fuel warning threshold (km of range remaining) */
    int fuel_search_buffer_km;          /* Extra km buffer for gas station search (beyond destination distance) */
    int fuel_high_load_threshold;       /* High-load detection threshold (% above baseline, e.g. 25) */
    int fuel_adaptive_learning_enabled; /* 1 = enable adaptive fuel learning (samples, trip summaries) */
};

/* Initialize configuration structure with safe defaults. */
#ifndef DRIVER_BREAK_SKIP_CONFIG_DEFAULT
void driver_break_config_default(struct driver_break_config *config);
#endif

/**
 * @brief Rest stop location structure
 *
 * Represents a candidate rest stop location with coordinates,
 * metadata, nearby POIs, and ranking score.
 */
struct driver_break_stop {
    struct coord_geo coord;
    char *name;
    char *highway_type;
    double distance_from_route; /* meters */
    GList *pois;                /* List of nearby POIs */
    int score;                  /* Ranking score */
};

/**
 * @brief Point of Interest structure
 *
 * Represents a POI near a rest stop, including location,
 * category, distance, and optional metadata.
 */
struct driver_break_poi {
    struct coord_geo coord;
    char *name;
    char *type;                             /* amenity, tourism, leisure, etc. */
    char *category;                         /* cafe, restaurant, museum, etc. */
    double distance_from_driver_break_stop; /* meters */
    char *opening_hours;
    int accessibility;
    double rating; /* If available */
};

/**
 * @brief Driving session tracking structure
 *
 * Tracks current driving session state including start time,
 * break history, and mandatory break status.
 */
struct driving_session {
    time_t start_time;
    time_t last_break_time;
    int total_driving_minutes;
    int continuous_driving_minutes;
    int breaks_taken;
    int mandatory_break_required;
};

/**
 * @brief Rest stop history entry
 *
 * Represents a historical rest stop visit stored in the database.
 */
struct driver_break_stop_history {
    time_t timestamp;
    struct coord_geo coord;
    char *name;
    int duration_minutes;
    int was_mandatory;
};

/**
 * @brief Fuel stop history entry
 *
 * Represents a fuel stop event stored in the database.
 */
struct driver_break_fuel_stop {
    time_t timestamp;
    struct coord_geo coord;
    double fuel_added;       /* Amount of fuel added (liters / kg / m³) */
    double fuel_level_after; /* Fuel level after fill (same unit) */
    double cost;             /* Total cost (optional, 0 if unknown) */
    char *currency;          /* Optional currency code, e.g. \"EUR\" */
    int ethanol_pct;         /* Ethanol % for flex-fuel (0-100, or -1 if unknown) */
};

/* Forward declarations */
struct navit;
struct route;
struct callback;
struct event_timeout;
struct driver_break_db;

/* Driver Break plugin private data structure */
struct driver_break_priv {
    struct navit *nav;
    struct driver_break_config config;
    struct driving_session session;
    struct driver_break_db *db;
    struct callback *vehicle_cb;
    struct callback *route_cb;
    struct event_timeout *check_timeout;
    struct route *current_route;
    GList *suggested_stops;
    time_t current_break_start_time;         /* 0 if no break in progress */
    struct coord_geo current_break_location; /* Location where break started */
    int active;

    /* Fuel state (runtime) */
    double fuel_current;         /* Current fuel amount (liters / kg / m³) */
    double fuel_rate_l_h;        /* Current fuel rate estimate (L/h or equivalent) */
    double fuel_remaining_range; /* Remaining range estimate (km) */

    /* Adaptive fuel learning (runtime only, persisted data in SQLite) */
    struct coord_geo last_sample_coord;
    time_t last_sample_time;
    double rolling_short_distance_m;
    double rolling_short_fuel;
    double rolling_long_distance_m;
    double rolling_long_fuel;
    double peak_consumption_l_per_100km;
    int high_load_active;
    time_t trip_start_time;
    double trip_total_distance_m;
    double trip_total_fuel;

    /* Optional live backends */
    void *obd_backend;
    void *j1939_backend;
    void *megasquirt_backend;
};

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_H */
