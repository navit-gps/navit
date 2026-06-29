/**
 * @file navit_safety_plan.c
 * @brief Resource-planning orchestrator implementation.
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2
 * as published by the Free Software Foundation.
 */

#include "navit_safety.h"
#include "navit_safety_confidence.h"
#include "navit_safety_plan.h"

/* Average spacing between confirmed (Medium-or-higher) stops over the route,
 * counting the legs from the start to the first stop and from the last stop to
 * the destination. Returns nonzero when that spacing exceeds threshold_km,
 * which is the POI-density signal for automatic remote mode. */
static int density_is_sparse(const struct navit_safety_stop *stops, int n_stops,
                             int route_length_km, int threshold_km) {
    int reliable = 0;
    int i;

    if (route_length_km <= 0 || threshold_km <= 0)
        return 0;

    for (i = 0; stops && i < n_stops; i++) {
        if (!navit_safety_confidence_counts_as_resupply(stops[i].confidence))
            continue;
        if (stops[i].distance_km < 0 || stops[i].distance_km > route_length_km)
            continue;
        reliable++;
    }

    return route_length_km / (reliable + 1) > threshold_km;
}

/* Pick the buffer for the conditions: arid zones use the arid buffer, other
 * remote terrain uses the remote buffer, populated terrain the standard one. */
static int select_buffer(const struct navit_safety_config *config,
                         enum navit_safety_resource resource,
                         int remote_active, int arid) {
    if (resource == NAVIT_SAFETY_RESOURCE_WATER) {
        if (arid)
            return config->water_buffer_arid_km;
        if (remote_active)
            return config->water_buffer_remote_km;
        return config->water_buffer_standard_km;
    }
    if (arid)
        return config->fuel_buffer_arid_km;
    if (remote_active)
        return config->fuel_buffer_remote_km;
    return config->fuel_buffer_standard_km;
}

void navit_safety_plan(const struct navit_safety_config *config,
                       enum navit_safety_resource resource,
                       double mid_lat, double mid_lon,
                       const struct navit_safety_stop *stops, int n_stops,
                       int route_length_km, int full_range_km,
                       struct navit_safety_plan_result *out) {
    enum navit_safety_koppen zone;
    int koppen_remote;
    int density_threshold;
    int arid;

    if (!out)
        return;

    out->zone = NAVIT_SAFETY_KOPPEN_UNKNOWN;
    out->remote_active = 0;
    out->density_sparse = 0;
    out->desert_warning = 0;
    out->buffer_km = 0;
    out->usable_range_km = 0;
    out->gap.max_gap_km = 0;
    out->gap.gap_start_km = 0;
    out->gap.warning = 0;
    out->gap.shortfall_km = 0;

    if (!config)
        return;

    zone = navit_safety_koppen_lookup(mid_lat, mid_lon);
    arid = navit_safety_koppen_is_arid(zone);
    koppen_remote = config->koppen_trigger && navit_safety_koppen_triggers_remote(zone);

    /* Water on foot uses a tighter spacing threshold than fuel (spec: 40 vs 80 km). */
    density_threshold = config->poi_density_threshold_km;
    if (resource == NAVIT_SAFETY_RESOURCE_WATER)
        density_threshold /= 2;
    out->density_sparse = density_is_sparse(stops, n_stops, route_length_km, density_threshold);

    switch (config->remote_mode) {
    case NAVIT_SAFETY_REMOTE_ALWAYS:
        out->remote_active = 1;
        break;
    case NAVIT_SAFETY_REMOTE_AUTO:
        out->remote_active = koppen_remote || out->density_sparse;
        break;
    case NAVIT_SAFETY_REMOTE_OFF:
    default:
        out->remote_active = 0;
        break;
    }

    /* Arid handling only applies when remote planning is in effect. */
    arid = arid && out->remote_active;
    out->desert_warning = arid && config->desert_consumption_warning;

    out->zone = zone;
    out->buffer_km = select_buffer(config, resource, out->remote_active, arid);

    out->usable_range_km = full_range_km - out->buffer_km;
    if (out->usable_range_km < 0)
        out->usable_range_km = 0;

    navit_safety_lookahead_plan(stops, n_stops, route_length_km, out->usable_range_km, &out->gap);
}
