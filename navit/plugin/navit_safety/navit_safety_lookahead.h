/**
 * @file navit_safety_lookahead.h
 * @brief Lookahead gap detection for the Navit Safety plugin.
 *
 * Scans an ordered list of resupply stops along a route and finds the largest
 * gap between reliable (Medium-or-higher confidence) stops, including the legs
 * from the start to the first reliable stop and from the last reliable stop to
 * the destination. If the worst gap exceeds the usable range it raises a
 * pre-departure warning and reports the shortfall. See
 * docs/user/plugins/navit_safety.rst ("Lookahead planning").
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef NAVIT_PLUGIN_NAVIT_SAFETY_LOOKAHEAD_H
#define NAVIT_PLUGIN_NAVIT_SAFETY_LOOKAHEAD_H

#include "navit_safety_confidence.h"

/** @brief A candidate resupply stop along the route. */
struct navit_safety_stop {
    int distance_km;                         /**< Distance from the route start. */
    enum navit_safety_confidence confidence; /**< Reliability of this stop. */
};

/** @brief Result of a lookahead scan. */
struct navit_safety_gap_result {
    int max_gap_km;     /**< Largest gap between reliable stops (and the route ends). */
    int gap_start_km;   /**< Distance from start where the worst gap begins. */
    int warning;        /**< Nonzero if max_gap_km exceeds the usable range. */
    int shortfall_km;   /**< How far max_gap_km exceeds the usable range (0 if none). */
};

/**
 * @brief Build a resupply gap plan for a route.
 *
 * Only stops whose confidence counts as resupply (Medium or High) are used;
 * low-confidence stops are ignored, as if they did not exist. Stops are
 * expected to be ordered by distance; out-of-range stops (negative distance or
 * beyond the route length) are skipped.
 *
 * @param stops Array of candidate stops (may be NULL when n_stops is 0).
 * @param n_stops Number of entries in @p stops.
 * @param route_length_km Total route length.
 * @param usable_range_km Usable range with the applicable buffer already removed.
 * @param out Result, filled on return (must not be NULL).
 */
void navit_safety_lookahead_plan(const struct navit_safety_stop *stops, int n_stops,
                                 int route_length_km, int usable_range_km,
                                 struct navit_safety_gap_result *out);

#endif /* NAVIT_PLUGIN_NAVIT_SAFETY_LOOKAHEAD_H */
