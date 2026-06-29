/**
 * @file navit_safety_lookahead.c
 * @brief Lookahead gap detection implementation.
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2
 * as published by the Free Software Foundation.
 */

#include "navit_safety_lookahead.h"

void navit_safety_lookahead_plan(const struct navit_safety_stop *stops, int n_stops,
                                 int route_length_km, int usable_range_km,
                                 struct navit_safety_gap_result *out) {
    int previous_km;
    int i;

    if (!out)
        return;

    out->max_gap_km = 0;
    out->gap_start_km = 0;
    out->warning = 0;
    out->shortfall_km = 0;

    if (route_length_km <= 0)
        return;

    /* Walk reliable stops in order, measuring each leg from the previous
     * reliable position. The route start counts as the first position and the
     * destination as the final one. Stops are assumed ordered by distance. */
    previous_km = 0;
    for (i = 0; stops && i < n_stops; i++) {
        const struct navit_safety_stop *stop = &stops[i];
        int gap;

        if (!navit_safety_confidence_counts_as_resupply(stop->confidence))
            continue;
        if (stop->distance_km < previous_km || stop->distance_km > route_length_km)
            continue;

        gap = stop->distance_km - previous_km;
        if (gap > out->max_gap_km) {
            out->max_gap_km = gap;
            out->gap_start_km = previous_km;
        }
        previous_km = stop->distance_km;
    }

    /* Final leg from the last reliable position to the destination. */
    {
        int final_gap = route_length_km - previous_km;
        if (final_gap > out->max_gap_km) {
            out->max_gap_km = final_gap;
            out->gap_start_km = previous_km;
        }
    }

    if (usable_range_km > 0 && out->max_gap_km > usable_range_km) {
        out->warning = 1;
        out->shortfall_km = out->max_gap_km - usable_range_km;
    }
}
