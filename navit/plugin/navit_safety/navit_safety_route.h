/**
 * @file navit_safety_route.h
 * @brief Live route-corridor scanning and planning state for the Navit Safety plugin.
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef NAVIT_PLUGIN_NAVIT_SAFETY_ROUTE_H
#define NAVIT_PLUGIN_NAVIT_SAFETY_ROUTE_H

#include "navit_safety_confidence.h"
#include "navit_safety_heat.h"
#include "navit_safety_lookahead.h"
#include "navit_safety_plan.h"

struct navit;
struct navit_safety_cache;
struct navit_safety_config;

/** @brief A resupply POI found along the active route corridor. */
struct navit_safety_poi_entry {
    char poi_id[64];                     /**< Stable identifier (OSM way id or map item id). */
    char label[96];                      /**< Display label. */
    double lat;                          /**< WGS84 latitude. */
    double lon;                          /**< WGS84 longitude. */
    int distance_km;                     /**< Distance from the route start. */
    enum navit_safety_confidence confidence; /**< Scored reliability. */
    int denied;                          /**< Nonzero if the user denied this stop for the trip. */
};

/** @brief Cached planning state for the active route. */
struct navit_safety_route_state {
    struct navit_safety_plan_result fuel_plan;   /**< Fuel (or primary) resource plan. */
    struct navit_safety_plan_result water_plan;  /**< Water plan when foot travel is enabled. */
    struct navit_safety_heat_result heat;        /**< Foot-travel heat assessment. */
    struct navit_safety_poi_entry *pois;         /**< POIs along the corridor (owned). */
    int n_pois;                                  /**< Number of entries in @p pois. */
    int route_length_km;                         /**< Active route length. */
    double mid_lat;                            /**< Mid-route latitude for terrain lookup. */
    double mid_lon;                            /**< Mid-route longitude for terrain lookup. */
    int nearest_idx;                           /**< Index of the nearest actionable POI, or -1. */
    char status[512];                          /**< Multi-line status for the OSD. */
    int valid;                                 /**< Nonzero when a calculated route is loaded. */
};

/** @brief Initialise an empty route state (does not allocate POI storage). */
void navit_safety_route_state_init(struct navit_safety_route_state *state);

/** @brief Release POI storage held by @p state. */
void navit_safety_route_state_clear(struct navit_safety_route_state *state);

/**
 * @brief Build the ordered stop list used by the planning engines.
 *
 * Denied POIs are omitted. @p stops must hold at least @p n_pois entries.
 *
 * @param pois POI array (may be NULL when @p n_pois is 0).
 * @param n_pois Number of POIs.
 * @param stops Output stop array.
 * @param n_stops Filled with the number of stops written.
 */
void navit_safety_build_plan_stops(const struct navit_safety_poi_entry *pois, int n_pois,
                                   struct navit_safety_stop *stops, int *n_stops);

/**
 * @brief Compose the multi-line OSD status string from a route state.
 *
 * @param state Route state (must not be NULL).
 * @param config Plugin configuration (must not be NULL).
 * @param out Output buffer.
 * @param out_len Size of @p out.
 */
void navit_safety_format_status(const struct navit_safety_route_state *state,
                                const struct navit_safety_config *config,
                                char *out, int out_len);

#if NAVIT_SAFETY_WITH_OSD

/**
 * @brief Scan the active route corridor and refresh the planning state.
 *
 * Reads the calculated route from @p nav, searches the mapset for fuel or water
 * POIs within the corridor, scores them, and runs the planning engines. When
 * @p vehicle_lat and @p vehicle_lon are finite, @p state->nearest_idx points at
 * the closest POI that can be confirmed or denied.
 *
 * @param state State to refresh (must not be NULL).
 * @param nav Navit instance.
 * @param config Plugin configuration.
 * @param cache Confirmation cache (may be NULL).
 * @param trip_id Trip identifier for cache lookups.
 * @param full_range_km Usable range on a full tank or water load, before buffers.
 * @param wbgt_c Wet-bulb globe temperature for the foot-travel heat model.
 * @param vehicle_lat Vehicle latitude (NaN to skip nearest-POI selection).
 * @param vehicle_lon Vehicle longitude (NaN to skip nearest-POI selection).
 */
void navit_safety_route_refresh(struct navit_safety_route_state *state, struct navit *nav,
                                const struct navit_safety_config *config, struct navit_safety_cache *cache,
                                const char *trip_id, int full_range_km, double wbgt_c,
                                double vehicle_lat, double vehicle_lon);

/**
 * @brief Mark the nearest actionable POI as confirmed and recalculate the plan.
 * @return 0 on success, -1 when no POI is selected.
 */
int navit_safety_route_confirm_nearest(struct navit_safety_route_state *state, struct navit *nav,
                                     const struct navit_safety_config *config, struct navit_safety_cache *cache,
                                     const char *trip_id, int full_range_km, double wbgt_c,
                                     double vehicle_lat, double vehicle_lon);

/**
 * @brief Mark the nearest actionable POI as denied and recalculate the plan.
 * @return 0 on success, -1 when no POI is selected.
 */
int navit_safety_route_deny_nearest(struct navit_safety_route_state *state, struct navit *nav,
                                  const struct navit_safety_config *config, struct navit_safety_cache *cache,
                                  const char *trip_id, int full_range_km, double wbgt_c,
                                  double vehicle_lat, double vehicle_lon);

#endif /* NAVIT_SAFETY_WITH_OSD */

#endif /* NAVIT_PLUGIN_NAVIT_SAFETY_ROUTE_H */
