/**
 * @file navit_safety_koppen.c
 * @brief Koppen climate-zone classification implementation.
 *
 * The lookup uses a small table of axis-aligned bounding boxes covering the
 * world's major arid and semi-arid regions. It is a coarse approximation
 * meant to flag obviously remote/arid terrain for conservative planning, not
 * a substitute for a gridded Koppen-Geiger dataset.
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2
 * as published by the Free Software Foundation.
 */

#include "navit_safety_koppen.h"

#define NAVIT_SAFETY_LAT_MAX 90.0
#define NAVIT_SAFETY_LON_MAX 180.0

/* One arid/semi-arid region as a bounding box (degrees) plus its zone. */
struct koppen_box {
    double lat_min;
    double lat_max;
    double lon_min;
    double lon_max;
    enum navit_safety_koppen zone;
};

/* Coarse coverage of the major arid belts. Boxes are deliberately broad;
 * earlier entries win when boxes overlap. */
static const struct koppen_box koppen_boxes[] = {
    /* Sahara and Sahel. */
    {15.0,  31.0,  -17.0,  38.0,   NAVIT_SAFETY_KOPPEN_BWH},
    {11.0,  15.0,  -17.0,  38.0,   NAVIT_SAFETY_KOPPEN_BSH},
    /* Arabian Peninsula. */
    {15.0,  31.0,  34.0,   60.0,   NAVIT_SAFETY_KOPPEN_BWH},
    /* Iran, Thar and Central Asia deserts. */
    {27.0,  40.0,  44.0,   78.0,   NAVIT_SAFETY_KOPPEN_BWK},
    /* Gobi and Taklamakan. */
    {36.0,  48.0,  78.0,   116.0,  NAVIT_SAFETY_KOPPEN_BWK},
    /* Australian interior. */
    {-33.0, -19.0, 117.0,  142.0,  NAVIT_SAFETY_KOPPEN_BWH},
    {-28.0, -19.0, 142.0,  148.0,  NAVIT_SAFETY_KOPPEN_BSH},
    /* Kalahari and Namib. */
    {-29.0, -18.0, 12.0,   25.0,   NAVIT_SAFETY_KOPPEN_BWH},
    /* Karoo. */
    {-33.0, -29.0, 18.0,   26.0,   NAVIT_SAFETY_KOPPEN_BSK},
    /* North American south-west deserts. */
    {28.0,  42.0,  -120.0, -103.0, NAVIT_SAFETY_KOPPEN_BWK},
    /* US Great Plains (cold semi-arid). */
    {32.0,  49.0,  -103.0, -97.0,  NAVIT_SAFETY_KOPPEN_BSK},
    /* Atacama and Peruvian coast. */
    {-30.0, -5.0,  -76.0,  -69.0,  NAVIT_SAFETY_KOPPEN_BWK},
    /* Patagonian steppe. */
    {-52.0, -38.0, -72.0,  -65.0,  NAVIT_SAFETY_KOPPEN_BWK}
};

#define NAVIT_SAFETY_KOPPEN_BOX_COUNT (sizeof(koppen_boxes) / sizeof(koppen_boxes[0]))

int navit_safety_koppen_is_arid(enum navit_safety_koppen zone) {
    return zone == NAVIT_SAFETY_KOPPEN_BWH || zone == NAVIT_SAFETY_KOPPEN_BWK;
}

int navit_safety_koppen_is_semiarid(enum navit_safety_koppen zone) {
    return zone == NAVIT_SAFETY_KOPPEN_BSH || zone == NAVIT_SAFETY_KOPPEN_BSK;
}

int navit_safety_koppen_triggers_remote(enum navit_safety_koppen zone) {
    return navit_safety_koppen_is_arid(zone) || navit_safety_koppen_is_semiarid(zone);
}

enum navit_safety_koppen navit_safety_koppen_lookup(double lat, double lon) {
    unsigned int i;

    if (lat < -NAVIT_SAFETY_LAT_MAX || lat > NAVIT_SAFETY_LAT_MAX || lon < -NAVIT_SAFETY_LON_MAX
        || lon > NAVIT_SAFETY_LON_MAX)
        return NAVIT_SAFETY_KOPPEN_UNKNOWN;

    for (i = 0; i < NAVIT_SAFETY_KOPPEN_BOX_COUNT; i++) {
        const struct koppen_box *box = &koppen_boxes[i];
        if (lat >= box->lat_min && lat <= box->lat_max && lon >= box->lon_min && lon <= box->lon_max)
            return box->zone;
    }

    return NAVIT_SAFETY_KOPPEN_OTHER;
}

const char *navit_safety_koppen_code(enum navit_safety_koppen zone) {
    switch (zone) {
    case NAVIT_SAFETY_KOPPEN_BWH:
        return "BWh";
    case NAVIT_SAFETY_KOPPEN_BWK:
        return "BWk";
    case NAVIT_SAFETY_KOPPEN_BSH:
        return "BSh";
    case NAVIT_SAFETY_KOPPEN_BSK:
        return "BSk";
    case NAVIT_SAFETY_KOPPEN_OTHER:
        return "other";
    case NAVIT_SAFETY_KOPPEN_UNKNOWN:
    default:
        return "unknown";
    }
}
