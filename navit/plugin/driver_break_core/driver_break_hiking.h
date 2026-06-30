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

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_HIKING_H
#define NAVIT_PLUGIN_DRIVER_BREAK_HIKING_H

#include "config.h"
#include "coord.h"
#include "driver_break.h"

/* Hiking rest constants */
#define HIKING_DRIVER_BREAK_DISTANCE_MAIN 11295.0 /* 11.295 km */
#define HIKING_DRIVER_BREAK_DISTANCE_ALT 2275.2   /* 2.275 km */
#define HIKING_MAX_DAILY_DISTANCE 40000.0         /* 40 km per day */

/* Hiking rest stop */
struct hiking_driver_break_stop {
    double position;            /* Position along route (meters) */
    double distance_from_start; /* Distance from start (meters) */
    int is_alternative;         /* 1 if using alternative distance, 0 if main */
    int day;                    /* Day number (for multi-day hikes) */
    struct coord_geo coord;     /* Geographic coordinates */
    GList *nearby_water;        /* Water points within 2 km */
    GList *nearby_cabins;       /* Cabins/huts within 5 km */
};

/* Daily hiking segment */
struct hiking_daily_segment {
    int day;
    double start_distance; /* Start distance (meters) */
    double end_distance;   /* End distance (meters) */
    double distance;       /* Segment distance (meters) */
};

/* Calculate hiking rest stops */
GList *hiking_calculate_driver_break_stops(double total_distance, int use_alternative);

/* Calculate hiking rest stops with configurable max daily distance */
GList *hiking_calculate_driver_break_stops_with_max(double total_distance, int use_alternative,
                                                    double max_daily_distance);

/* Calculate daily segments for hiking */
GList *hiking_calculate_daily_segments(double total_distance, double max_daily_km);

/* Free hiking rest stop list */
void hiking_free_driver_break_stops(GList *stops);

/* Free daily segment list */
void hiking_free_daily_segments(GList *segments);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_HIKING_H */
