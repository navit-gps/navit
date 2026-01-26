/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024 Navit Team
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

#ifndef NAVIT_PLUGIN_REST_H
#define NAVIT_PLUGIN_REST_H

#include "config.h"
#include <glib.h>
#include <time.h>
#include "coord.h"
#include "item.h"
#include "attr.h"

/* Vehicle types */
enum rest_vehicle_type {
    REST_VEHICLE_CAR = 0,
    REST_VEHICLE_TRUCK = 1,
    REST_VEHICLE_HIKING = 2,
    REST_VEHICLE_CYCLING = 3
};

/* Rest period configuration */
struct rest_config {
    int vehicle_type;  /* 0=car, 1=truck, 2=hiking, 3=cycling */
    
    /* Car settings */
    int car_soft_limit_hours;      /* 5-9 hours */
    int car_max_hours;              /* 10-12 hours */
    int car_break_interval_hours;  /* 4.5 hours */
    int car_break_duration_min;    /* 15-45 minutes */
    
    /* Truck settings (EU Regulation EC 561/2006) */
    int truck_mandatory_break_after_hours;  /* 4.5 hours */
    int truck_mandatory_break_duration_min; /* 45 minutes */
    int truck_max_daily_hours;              /* 9 hours */
    
    /* Hiking settings */
    double hiking_rest_distance_main;      /* 11.295 km */
    double hiking_rest_distance_alt;       /* 2.275 km */
    double hiking_max_daily_distance;      /* 40 km */
    
    /* Cycling settings */
    double cycling_rest_distance_main;      /* 28.24 km */
    double cycling_rest_distance_alt;      /* 5.69 km */
    double cycling_max_daily_distance;      /* 100 km */
    
    /* Rest stop settings */
    int min_distance_from_buildings;  /* 150 meters */
    int min_distance_from_glaciers;  /* 300 meters for nightly */
    int poi_search_radius_km;         /* 15 km */
    int poi_water_search_radius_km;   /* 2 km */
    int poi_cabin_search_radius_km;   /* 5 km */
    int network_hut_search_radius_km;  /* 25 km for network huts when DNT priority enabled */
    
    /* Network and route priority */
    int enable_dnt_priority;  /* Enable DNT/network priority mode (0=disabled, 1=enabled) */
    int enable_hiking_pilgrimage_priority;  /* Enable hiking/pilgrimage route priority (0=disabled, 1=enabled) */
    
    /* Rest interval range */
    int rest_interval_min_hours;  /* 1 hour */
    int rest_interval_max_hours;  /* 6 hours */
    
    /* Energy-based routing */
    int use_energy_routing;        /* 1 to enable energy-based routing */
    double total_weight;           /* Total weight for energy calculations (kg) */
};

/* Rest stop location */
struct rest_stop {
    struct coord_geo coord;
    char *name;
    char *highway_type;
    double distance_from_route;  /* meters */
    GList *pois;  /* List of nearby POIs */
    int score;    /* Ranking score */
};

/* POI information */
struct rest_poi {
    struct coord_geo coord;
    char *name;
    char *type;  /* amenity, tourism, leisure, etc. */
    char *category;  /* cafe, restaurant, museum, etc. */
    double distance_from_rest_stop;  /* meters */
    char *opening_hours;
    int accessibility;
    double rating;  /* If available */
};

/* Driving session tracking */
struct driving_session {
    time_t start_time;
    time_t last_break_time;
    int total_driving_minutes;
    int continuous_driving_minutes;
    int breaks_taken;
    int mandatory_break_required;
};

/* Rest stop history entry */
struct rest_stop_history {
    time_t timestamp;
    struct coord_geo coord;
    char *name;
    int duration_minutes;
    int was_mandatory;
};

/* Forward declarations */
struct navit;
struct route;
struct callback;
struct event_timeout;
struct rest_db;

/* Rest plugin private data structure */
struct rest_priv {
    struct navit *nav;
    struct rest_config config;
    struct driving_session session;
    struct rest_db *db;
    struct callback *vehicle_cb;
    struct callback *route_cb;
    struct event_timeout *check_timeout;
    struct route *current_route;
    GList *suggested_stops;
    int active;
};

#endif /* NAVIT_PLUGIN_REST_H */
