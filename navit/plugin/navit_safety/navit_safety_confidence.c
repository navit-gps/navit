/**
 * @file navit_safety_confidence.c
 * @brief POI confidence scoring implementation.
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2
 * as published by the Free Software Foundation.
 */

#include "navit_safety_confidence.h"

/* Age thresholds (days) used by the source ranking table. */
#define NAVIT_SAFETY_AGE_30_DAYS 30
#define NAVIT_SAFETY_AGE_60_DAYS 60
#define NAVIT_SAFETY_AGE_12_MONTHS 365
#define NAVIT_SAFETY_AGE_3_YEARS 1095

int navit_safety_confidence_counts_as_resupply(enum navit_safety_confidence confidence) {
    return confidence >= NAVIT_SAFETY_CONFIDENCE_MEDIUM;
}

/* Clamp a confidence so it never exceeds a ceiling (used for downgrades). */
static enum navit_safety_confidence cap_confidence(enum navit_safety_confidence value,
                                                   enum navit_safety_confidence ceiling) {
    return value > ceiling ? ceiling : value;
}

/* Map source and age to a base confidence before global downgrades. */
static enum navit_safety_confidence score_by_source(enum navit_safety_poi_source source, int age) {
    switch (source) {
    case NAVIT_SAFETY_SRC_CHAIN_API:
    case NAVIT_SAFETY_SRC_NREL:
        return NAVIT_SAFETY_CONFIDENCE_HIGH;
    case NAVIT_SAFETY_SRC_OSM_NREL_MATCH:
        return (age >= 0 && age <= NAVIT_SAFETY_AGE_30_DAYS) ? NAVIT_SAFETY_CONFIDENCE_HIGH
                                                             : NAVIT_SAFETY_CONFIDENCE_MEDIUM;
    case NAVIT_SAFETY_SRC_IOVERLANDER:
        return (age >= 0 && age <= NAVIT_SAFETY_AGE_60_DAYS) ? NAVIT_SAFETY_CONFIDENCE_MEDIUM
                                                             : NAVIT_SAFETY_CONFIDENCE_LOW;
    case NAVIT_SAFETY_SRC_OSM_CHAIN:
        return NAVIT_SAFETY_CONFIDENCE_MEDIUM;
    case NAVIT_SAFETY_SRC_OSM_INDEPENDENT:
        if (age < 0 || age > NAVIT_SAFETY_AGE_3_YEARS)
            return NAVIT_SAFETY_CONFIDENCE_LOW;
        if (age <= NAVIT_SAFETY_AGE_12_MONTHS)
            return NAVIT_SAFETY_CONFIDENCE_MEDIUM;
        return NAVIT_SAFETY_CONFIDENCE_LOW;
    case NAVIT_SAFETY_SRC_UNKNOWN:
    default:
        return NAVIT_SAFETY_CONFIDENCE_UNKNOWN;
    }
}

enum navit_safety_confidence navit_safety_score_poi(const struct navit_safety_poi *poi) {
    enum navit_safety_confidence result;
    int age;

    if (!poi)
        return NAVIT_SAFETY_CONFIDENCE_UNKNOWN;

    if (poi->source == NAVIT_SAFETY_SRC_USER_CONFIRMED)
        return NAVIT_SAFETY_CONFIDENCE_HIGH;

    age = poi->age_days;
    result = score_by_source(poi->source, age);

    /* Data older than three years has a high closure risk regardless of source
     * (except the live/authoritative and user-confirmed sources handled above). */
    if (age > NAVIT_SAFETY_AGE_3_YEARS)
        result = cap_confidence(result, NAVIT_SAFETY_CONFIDENCE_LOW);

    /* Known depopulating regions cap any map-derived POI at Low. */
    if (poi->depopulating_region)
        result = cap_confidence(result, NAVIT_SAFETY_CONFIDENCE_LOW);

    return result;
}
