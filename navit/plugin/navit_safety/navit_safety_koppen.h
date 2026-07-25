/**
 * @file navit_safety_koppen.h
 * @brief Koppen climate-zone classification for the Navit Safety plugin.
 *
 * Remote mode activates when a route segment lies in a hot/cold desert or
 * hot/cold semi-arid zone (Koppen group B). This module provides the trigger
 * decision plus a lightweight, coarse lat/lon lookup derived from the major
 * arid regions; it is intentionally approximate and not a full Koppen-Geiger
 * dataset. See docs/user/plugins/navit_safety.rst ("Location-aware remote
 * mode").
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef NAVIT_PLUGIN_NAVIT_SAFETY_KOPPEN_H
#define NAVIT_PLUGIN_NAVIT_SAFETY_KOPPEN_H

/** @brief Koppen zones relevant to remote-mode triggering. */
enum navit_safety_koppen {
    NAVIT_SAFETY_KOPPEN_UNKNOWN = 0, /**< Could not be determined. */
    NAVIT_SAFETY_KOPPEN_OTHER,       /**< A non-arid zone (no trigger). */
    NAVIT_SAFETY_KOPPEN_BWH,         /**< Hot desert. */
    NAVIT_SAFETY_KOPPEN_BWK,         /**< Cold desert. */
    NAVIT_SAFETY_KOPPEN_BSH,         /**< Hot semi-arid. */
    NAVIT_SAFETY_KOPPEN_BSK          /**< Cold semi-arid. */
};

/**
 * @brief Whether a zone is a desert (BWh/BWk).
 * @param zone Zone to test.
 * @return Nonzero for hot or cold desert.
 */
int navit_safety_koppen_is_arid(enum navit_safety_koppen zone);

/**
 * @brief Whether a zone is semi-arid (BSh/BSk).
 * @param zone Zone to test.
 * @return Nonzero for hot or cold semi-arid.
 */
int navit_safety_koppen_is_semiarid(enum navit_safety_koppen zone);

/**
 * @brief Whether a zone triggers remote mode (any Koppen group B zone).
 * @param zone Zone to test.
 * @return Nonzero for desert or semi-arid zones.
 */
int navit_safety_koppen_triggers_remote(enum navit_safety_koppen zone);

/**
 * @brief Coarse classification of a coordinate into a Koppen zone.
 * @param lat Latitude in degrees (-90..90).
 * @param lon Longitude in degrees (-180..180).
 * @return The matching arid/semi-arid zone, OTHER for non-arid, or UNKNOWN
 *         for out-of-range coordinates.
 */
enum navit_safety_koppen navit_safety_koppen_lookup(double lat, double lon);

/**
 * @brief Short code string for a zone, e.g. "BWh".
 * @param zone Zone to format.
 * @return A static, non-NULL string.
 */
const char *navit_safety_koppen_code(enum navit_safety_koppen zone);

#endif /* NAVIT_PLUGIN_NAVIT_SAFETY_KOPPEN_H */
