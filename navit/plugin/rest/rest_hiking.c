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

#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "debug.h"
#include "rest_hiking.h"

/* Calculate hiking rest stops */
GList *hiking_calculate_rest_stops(double total_distance, int use_alternative) {
    return hiking_calculate_rest_stops_with_max(total_distance, use_alternative, HIKING_MAX_DAILY_DISTANCE);
}

/* Calculate rest stops with configurable max daily distance */
GList *hiking_calculate_rest_stops_with_max(double total_distance, int use_alternative, double max_daily_distance) {
    GList *rest_stops = NULL;
    double rest_distance = use_alternative ? HIKING_REST_DISTANCE_ALT : HIKING_REST_DISTANCE_MAIN;
    
    if (total_distance <= rest_distance) {
        return NULL;  /* No rest needed */
    }
    
    double current_distance = rest_distance;
    double daily_distance = rest_distance;  /* Track distance traveled today */
    int day = 1;
    
    while (current_distance < total_distance) {
        double remaining_distance = total_distance - current_distance;
        
        if (remaining_distance <= rest_distance) {
            break;  /* Can complete without additional rest */
        }
        
        struct hiking_rest_stop *rest_stop = g_new0(struct hiking_rest_stop, 1);
        rest_stop->position = current_distance;
        rest_stop->distance_from_start = current_distance;
        rest_stop->is_alternative = use_alternative;
        rest_stop->day = day;
        
        rest_stops = g_list_append(rest_stops, rest_stop);
        
        current_distance += rest_distance;
        daily_distance += rest_distance;
        
        /* Check if we've exceeded max daily distance */
        if (daily_distance >= max_daily_distance) {
            /* Start new day */
            daily_distance = 0.0;
            day++;
        }
    }
    
    return rest_stops;
}

/* Calculate daily segments for hiking */
GList *hiking_calculate_daily_segments(double total_distance, double max_daily_km) {
    GList *segments = NULL;
    
    /* Validate daily distance: between 20 and 50 km */
    double min_daily = 20.0;
    double max_daily = 50.0;
    double default_daily = 40.0;
    
    if (max_daily_km < min_daily || max_daily_km > max_daily) {
        max_daily_km = default_daily;
    }
    
    double max_daily_meters = max_daily_km * 1000.0;
    
    if (total_distance <= max_daily_meters) {
        struct hiking_daily_segment *segment = g_new0(struct hiking_daily_segment, 1);
        segment->day = 1;
        segment->start_distance = 0;
        segment->end_distance = total_distance;
        segment->distance = total_distance;
        segments = g_list_append(segments, segment);
        return segments;
    }
    
    int day = 1;
    double current_distance = 0;
    
    while (current_distance < total_distance) {
        double remaining_distance = total_distance - current_distance;
        double segment_distance = fmin(max_daily_meters, remaining_distance);
        
        struct hiking_daily_segment *segment = g_new0(struct hiking_daily_segment, 1);
        segment->day = day;
        segment->start_distance = current_distance;
        segment->end_distance = current_distance + segment_distance;
        segment->distance = segment_distance;
        
        segments = g_list_append(segments, segment);
        
        current_distance += segment_distance;
        day++;
    }
    
    return segments;
}

void hiking_free_rest_stops(GList *stops) {
    GList *l = stops;
    while (l) {
        struct hiking_rest_stop *stop = (struct hiking_rest_stop *)l->data;
        if (stop) {
            if (stop->nearby_water) {
                g_list_free_full(stop->nearby_water, g_free);
            }
            if (stop->nearby_cabins) {
                g_list_free_full(stop->nearby_cabins, g_free);
            }
            g_free(stop);
        }
        l = g_list_next(l);
    }
    g_list_free(stops);
}

void hiking_free_daily_segments(GList *segments) {
    GList *l = segments;
    while (l) {
        g_free(l->data);
        l = g_list_next(l);
    }
    g_list_free(segments);
}
