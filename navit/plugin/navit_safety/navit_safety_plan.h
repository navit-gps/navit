/**
 * @file navit_safety_plan.h
 * @brief Resource-planning orchestrator for the Navit Safety plugin.
 *
 * Ties the individual engines together: it classifies the route's terrain
 * (Koppen), decides whether remote mode applies, selects the appropriate fuel
 * or water buffer, and runs lookahead gap detection over the confirmed
 * resupply stops. This is the engine the route hook calls once a route is
 * available. See docs/user/plugins/navit_safety.rst ("Resource planning in
 * remote mode").
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef NAVIT_PLUGIN_NAVIT_SAFETY_PLAN_H
#define NAVIT_PLUGIN_NAVIT_SAFETY_PLAN_H

#include "navit_safety.h"
#include "navit_safety_koppen.h"
#include "navit_safety_lookahead.h"

/** @brief Resource being planned for. */
enum navit_safety_resource {
    NAVIT_SAFETY_RESOURCE_FUEL = 0, /**< Vehicle fuel. */
    NAVIT_SAFETY_RESOURCE_WATER     /**< Foot-travel water. */
};

/** @brief Outcome of a resource plan over a route. */
struct navit_safety_plan_result {
    enum navit_safety_koppen zone;        /**< Terrain zone at the sampled midpoint. */
    int remote_active;                    /**< Nonzero if remote-mode planning applies. */
    int density_sparse;                   /**< Nonzero if confirmed-POI spacing exceeded the density threshold. */
    int desert_warning;                   /**< Nonzero if arid conditions warrant a higher-consumption warning. */
    int buffer_km;                        /**< Buffer selected for the conditions. */
    int usable_range_km;                  /**< Range after the buffer is removed. */
    struct navit_safety_gap_result gap;   /**< Lookahead gap result. */
};

/**
 * @brief Plan a single resource over a route.
 *
 * Remote mode is active when the configured mode is Always, or when it is Auto
 * and either the Koppen trigger (if enabled) classifies the sampled midpoint as
 * a group-B (arid/semi-arid) zone, or the confirmed-POI spacing is sparser than
 * the density threshold (config->poi_density_threshold_km, halved for water).
 * The buffer is the arid buffer in desert zones, the remote buffer when remote
 * mode is otherwise active, and the standard buffer in populated terrain.
 * Lookahead then runs over @p stops with the resulting usable range.
 *
 * @param config Plugin configuration (NULL fails).
 * @param resource Fuel or water.
 * @param mid_lat Latitude of a representative route midpoint.
 * @param mid_lon Longitude of a representative route midpoint.
 * @param stops Ordered candidate resupply stops (may be NULL when n_stops is 0).
 * @param n_stops Number of stops.
 * @param route_length_km Total route length.
 * @param full_range_km Range on a full tank/water load before any buffer.
 * @param out Result, filled on return (must not be NULL).
 */
void navit_safety_plan(const struct navit_safety_config *config,
                       enum navit_safety_resource resource,
                       double mid_lat, double mid_lon,
                       const struct navit_safety_stop *stops, int n_stops,
                       int route_length_km, int full_range_km,
                       struct navit_safety_plan_result *out);

#endif /* NAVIT_PLUGIN_NAVIT_SAFETY_PLAN_H */
