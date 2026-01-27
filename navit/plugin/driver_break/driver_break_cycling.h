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

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_CYCLING_H
#define NAVIT_PLUGIN_DRIVER_BREAK_CYCLING_H

#include "config.h"
#include "coord.h"
#include "rest.h"

/* Cycling rest constants (scaled from hiking by 2.5x) */
#define CYCLING_DRIVER_BREAK_DISTANCE_MAIN 28240.0     /* 28.24 km */
#define CYCLING_DRIVER_BREAK_DISTANCE_ALT 5690.0       /* 5.69 km */
#define CYCLING_MAX_DAILY_DISTANCE 100000.0    /* 100 km per day */
#define CYCLING_MIN_DIST_FROM_MAIN 1000.0      /* 1 km minimum from main rest */

/* Cycling rest stop */
struct cycling_driver_break_stop {
    double position;              /* Position along route (meters) */
    double distance_from_start;  /* Distance from start (meters) */
    int is_main_rest;            /* 1 if main rest (28.24 km), 0 if alternative */
    int is_alternative;          /* 1 if alternative rest (5.69 km) */
    struct coord_geo coord;      /* Geographic coordinates */
    GList *nearby_water;         /* Water points within 2 km */
    GList *nearby_cabins;        /* Cabins/huts within 5 km */
};

/* Daily cycling segment */
struct cycling_daily_segment {
    int day;
    double start_distance;       /* Start distance (meters) */
    double end_distance;         /* End distance (meters) */
    double distance;             /* Segment distance (meters) */
};

/* Calculate cycling rest stops */
GList *cycling_calculate_driver_break_stops(double total_distance, int use_alternative);

/* Calculate daily segments for cycling */
GList *cycling_calculate_daily_segments(double total_distance);

/* Free cycling rest stop list */
void cycling_free_driver_break_stops(GList *stops);

/* Free daily segment list */
void cycling_free_daily_segments(GList *segments);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_CYCLING_H */
