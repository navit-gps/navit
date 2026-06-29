/**
 * @file navit_safety_confidence.h
 * @brief POI confidence scoring for the Navit Safety plugin.
 *
 * Ranks a point of interest by the reliability of its source and age so that
 * only confirmed or reasonably recent stops count toward resupply in
 * minimum-range calculations. See docs/user/plugins/navit_safety.rst
 * ("POI confidence hierarchy").
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef NAVIT_PLUGIN_NAVIT_SAFETY_CONFIDENCE_H
#define NAVIT_PLUGIN_NAVIT_SAFETY_CONFIDENCE_H

/** @brief Reliability of a resupply point, lowest to highest. */
enum navit_safety_confidence {
    NAVIT_SAFETY_CONFIDENCE_UNKNOWN = 0, /**< No usable metadata; treated as Low. */
    NAVIT_SAFETY_CONFIDENCE_LOW,         /**< Unverifiable or stale; not reliable resupply. */
    NAVIT_SAFETY_CONFIDENCE_MEDIUM,      /**< Likely operational; counts with a buffer. */
    NAVIT_SAFETY_CONFIDENCE_HIGH         /**< Confirmed operational; counts fully. */
};

/** @brief Data source a POI was obtained from, used for source ranking. */
enum navit_safety_poi_source {
    NAVIT_SAFETY_SRC_UNKNOWN = 0,       /**< Source not known. */
    NAVIT_SAFETY_SRC_CHAIN_API,         /**< Live chain operator API (Shell, Circle K). */
    NAVIT_SAFETY_SRC_NREL,              /**< NREL Alternative Fuel Stations. */
    NAVIT_SAFETY_SRC_OSM_NREL_MATCH,    /**< OSM cross-referenced with NREL. */
    NAVIT_SAFETY_SRC_IOVERLANDER,       /**< iOverlander CSV import (offline). */
    NAVIT_SAFETY_SRC_OSM_CHAIN,         /**< OSM with a known chain operator tag. */
    NAVIT_SAFETY_SRC_OSM_INDEPENDENT,   /**< OSM independent operator. */
    NAVIT_SAFETY_SRC_USER_CONFIRMED     /**< Confirmed by the user on the current trip. */
};

/** @brief A POI candidate to be scored. */
struct navit_safety_poi {
    enum navit_safety_poi_source source; /**< Where the POI came from. */
    int age_days;                        /**< Age of the data in days; negative if unknown. */
    int depopulating_region;             /**< Nonzero if in a known depopulating county/region. */
};

/**
 * @brief Score a POI's reliability from its source and age.
 * @param poi POI to score (NULL yields UNKNOWN).
 * @return Confidence level per the source ranking table.
 */
enum navit_safety_confidence navit_safety_score_poi(const struct navit_safety_poi *poi);

/**
 * @brief Whether a confidence level counts as reliable resupply.
 * @param confidence Level to test.
 * @return Nonzero for Medium or High; zero otherwise.
 */
int navit_safety_confidence_counts_as_resupply(enum navit_safety_confidence confidence);

#endif /* NAVIT_PLUGIN_NAVIT_SAFETY_CONFIDENCE_H */
