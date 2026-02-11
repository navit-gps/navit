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

#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "debug.h"
#include "driver_break_cycling.h"

/* Compare function for sorting rest stops by position */
static int cycling_compare_position(const struct cycling_driver_break_stop *a,
                                     const struct cycling_driver_break_stop *b) {
    if (a->position < b->position) return -1;
    if (a->position > b->position) return 1;
    return 0;
}

/* Calculate cycling rest stops */
GList *cycling_calculate_driver_break_stops(double total_distance, int use_alternative) {
    GList *driver_break_stops = NULL;

    if (total_distance <= CYCLING_DRIVER_BREAK_DISTANCE_MAIN) {
        return NULL;  /* No rest needed */
    }

    /* First, calculate main rest stops (every 28.24 km) */
    GList *main_driver_break_positions = NULL;
    double current_distance = CYCLING_DRIVER_BREAK_DISTANCE_MAIN;

    while (current_distance < total_distance) {
        struct cycling_driver_break_stop *driver_break_stop = g_new0(struct cycling_driver_break_stop, 1);
        driver_break_stop->position = current_distance;
        driver_break_stop->distance_from_start = current_distance;
        driver_break_stop->is_main_rest = 1;
        driver_break_stop->is_alternative = 0;

        driver_break_stops = g_list_append(driver_break_stops, driver_break_stop);
        main_driver_break_positions = g_list_append(main_driver_break_positions, GINT_TO_POINTER((int)current_distance));

        current_distance += CYCLING_DRIVER_BREAK_DISTANCE_MAIN;
    }

    /* If using alternative, add alternative rest stops (every 5.69 km) */
    /* but only if >1 km from nearest main rest */
    if (use_alternative) {
        current_distance = CYCLING_DRIVER_BREAK_DISTANCE_ALT;

        while (current_distance < total_distance) {
            /* Check if this position is >1 km from any main rest */
            int too_close = 0;
            GList *l = main_driver_break_positions;
            while (l) {
                double main_driver_break_pos = (double)GPOINTER_TO_INT(l->data);
                double distance_to_main = fabs(current_distance - main_driver_break_pos);
                if (distance_to_main <= CYCLING_MIN_DIST_FROM_MAIN) {
                    too_close = 1;
                    break;
                }
                l = g_list_next(l);
            }

            /* Also check if we already have a rest stop at this position */
            int already_exists = 0;
            l = driver_break_stops;
            while (l) {
                struct cycling_driver_break_stop *existing = (struct cycling_driver_break_stop *)l->data;
                double distance_to_existing = fabs(current_distance - existing->position);
                if (distance_to_existing < 100.0) {  /* Within 100m, consider it the same */
                    already_exists = 1;
                    break;
                }
                l = g_list_next(l);
            }

            if (!too_close && !already_exists) {
                struct cycling_driver_break_stop *driver_break_stop = g_new0(struct cycling_driver_break_stop, 1);
                driver_break_stop->position = current_distance;
                driver_break_stop->distance_from_start = current_distance;
                driver_break_stop->is_main_rest = 0;
                driver_break_stop->is_alternative = 1;

                driver_break_stops = g_list_append(driver_break_stops, driver_break_stop);
            }

            current_distance += CYCLING_DRIVER_BREAK_DISTANCE_ALT;
        }

        /* Sort rest stops by position */
        driver_break_stops = g_list_sort(driver_break_stops, (GCompareFunc)cycling_compare_position);
    }

    g_list_free(main_driver_break_positions);

    return driver_break_stops;
}

/* Calculate daily segments for cycling */
GList *cycling_calculate_daily_segments(double total_distance) {
    GList *segments = NULL;

    if (total_distance <= CYCLING_MAX_DAILY_DISTANCE) {
        struct cycling_daily_segment *segment = g_new0(struct cycling_daily_segment, 1);
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
        double segment_distance = fmin(CYCLING_MAX_DAILY_DISTANCE, remaining_distance);

        struct cycling_daily_segment *segment = g_new0(struct cycling_daily_segment, 1);
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

void cycling_free_driver_break_stops(GList *stops) {
    GList *l = stops;
    while (l) {
        struct cycling_driver_break_stop *stop = (struct cycling_driver_break_stop *)l->data;
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

void cycling_free_daily_segments(GList *segments) {
    GList *l = segments;
    while (l) {
        g_free(l->data);
        l = g_list_next(l);
    }
    g_list_free(segments);
}
