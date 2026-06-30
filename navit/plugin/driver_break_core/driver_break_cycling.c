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

#include "driver_break_cycling.h"
#include "config.h"
#include "debug.h"
#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Compare function for sorting rest stops by position */
static int cycling_compare_position(const struct cycling_driver_break_stop *a,
                                    const struct cycling_driver_break_stop *b) {
    if (a->position < b->position)
        return -1;
    if (a->position > b->position)
        return 1;
    return 0;
}

static struct cycling_driver_break_stop *cycling_new_stop(double position, int is_main) {
    struct cycling_driver_break_stop *driver_break_stop = g_new0(struct cycling_driver_break_stop, 1);

    driver_break_stop->position = position;
    driver_break_stop->distance_from_start = position;
    driver_break_stop->is_main_rest = is_main ? 1 : 0;
    driver_break_stop->is_alternative = is_main ? 0 : 1;
    return driver_break_stop;
}

static GList *cycling_add_main_stops(double total_distance, GList **main_positions_out) {
    GList *driver_break_stops = NULL;
    GList *main_driver_break_positions = NULL;
    double current_distance = CYCLING_DRIVER_BREAK_DISTANCE_MAIN;

    while (current_distance < total_distance) {
        struct cycling_driver_break_stop *driver_break_stop = cycling_new_stop(current_distance, 1);

        driver_break_stops = g_list_append(driver_break_stops, driver_break_stop);
        main_driver_break_positions =
            g_list_append(main_driver_break_positions, GINT_TO_POINTER((int)current_distance));
        current_distance += CYCLING_DRIVER_BREAK_DISTANCE_MAIN;
    }

    *main_positions_out = main_driver_break_positions;
    return driver_break_stops;
}

static int cycling_too_close_to_main(double position, GList *main_positions) {
    GList *l;

    for (l = main_positions; l; l = g_list_next(l)) {
        double main_driver_break_pos = (double)GPOINTER_TO_INT(l->data);
        if (fabs(position - main_driver_break_pos) <= CYCLING_MIN_DIST_FROM_MAIN) {
            return 1;
        }
    }
    return 0;
}

static int cycling_stop_exists_at(double position, GList *driver_break_stops) {
    GList *l;

    for (l = driver_break_stops; l; l = g_list_next(l)) {
        struct cycling_driver_break_stop *existing = (struct cycling_driver_break_stop *)l->data;
        if (fabs(position - existing->position) < 100.0) {
            return 1;
        }
    }
    return 0;
}

static GList *cycling_add_alternative_stops(GList *driver_break_stops, GList *main_positions, double total_distance) {
    double current_distance = CYCLING_DRIVER_BREAK_DISTANCE_ALT;

    while (current_distance < total_distance) {
        if (!cycling_too_close_to_main(current_distance, main_positions)
            && !cycling_stop_exists_at(current_distance, driver_break_stops)) {
            driver_break_stops = g_list_append(driver_break_stops, cycling_new_stop(current_distance, 0));
        }
        current_distance += CYCLING_DRIVER_BREAK_DISTANCE_ALT;
    }

    return g_list_sort(driver_break_stops, (GCompareFunc)cycling_compare_position);
}

/* Calculate cycling rest stops */
GList *cycling_calculate_driver_break_stops(double total_distance, int use_alternative) {
    GList *main_driver_break_positions = NULL;
    GList *driver_break_stops;

    if (total_distance <= CYCLING_DRIVER_BREAK_DISTANCE_MAIN) {
        return NULL;
    }

    driver_break_stops = cycling_add_main_stops(total_distance, &main_driver_break_positions);

    if (use_alternative) {
        driver_break_stops =
            cycling_add_alternative_stops(driver_break_stops, main_driver_break_positions, total_distance);
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
