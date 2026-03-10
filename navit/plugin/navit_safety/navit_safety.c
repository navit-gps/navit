/**
 * @file navit_safety.c
 * @brief Navit Safety plugin: POI confidence, remote mode, resource planning for desert/arid travel.
 *
 * Handles POI confidence scoring, location-aware remote mode activation, and
 * resource planning (fuel/water) for desert driving and foot travel in arid or
 * sparsely serviced terrain. See docs/user/plugins/navit_safety.rst for full spec.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2
 * as published by the Free Software Foundation.
 */

#include "navit_safety.h"
#include "attr.h"
#include "config.h"
#include "debug.h"
#include "navit.h"
#include "osd.h"
#include "plugin.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Set configuration to spec defaults.
 * @param config Configuration structure to fill
 */
void navit_safety_config_default(struct navit_safety_config *config) {
    memset(config, 0, sizeof(*config));
    config->remote_mode = NAVIT_SAFETY_REMOTE_AUTO;
    config->poi_density_threshold_km = 80;
    config->koppen_trigger = 1;
    config->fuel_buffer_standard_km = 25;
    config->fuel_buffer_remote_km = 85;
    config->fuel_buffer_arid_km = 140;
    config->water_buffer_standard_km = 5;
    config->water_buffer_remote_km = 20;
    config->water_buffer_arid_km = 30;
    config->desert_consumption_warning = 1;
    config->census_depopulation_layer = 1;
    config->chain_api_queries = 1;
    config->unconfirmed_poi_display = 1;
    config->foot_travel_mode = 0;
    config->body_weight_kg = 70;
    config->heat_stress_warnings = 1;
}

/* The following definitions are only needed when building Navit itself.
 * Unit tests for configuration defaults link only against navit_safety_config_default. */
#if NAVIT_SAFETY_WITH_OSD

/** @brief OSD private data */
struct navit_safety_priv {
    struct navit *nav;
    struct navit_safety_config config;
    int active;
};

static void navit_safety_osd_destroy(struct osd_priv *osd) {
    struct navit_safety_priv *priv = (struct navit_safety_priv *)osd;
    if (!priv)
        return;
    priv->active = 0;
    g_free(priv);
}

static int navit_safety_osd_set_attr(struct osd_priv *osd, struct attr *attr) {
    (void)osd;
    (void)attr;
    return 0;
}

static struct osd_methods navit_safety_osd_meth = {
    navit_safety_osd_destroy,
    navit_safety_osd_set_attr,
    navit_safety_osd_destroy,
    NULL,
};

/**
 * @brief OSD constructor. Called when Navit instantiates the plugin from config.
 * @param nav Navit instance
 * @param meth OSD methods to fill
 * @param attrs Attributes from XML (e.g. type="navit_safety")
 * @return OSD private data or NULL
 */
static struct osd_priv *navit_safety_osd_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs) {
    struct navit_safety_priv *priv;
    (void)attrs;

    if (!nav) {
        dbg(lvl_error, "Navit Safety: osd_new called with NULL nav");
        return NULL;
    }

    priv = g_new0(struct navit_safety_priv, 1);
    *meth = navit_safety_osd_meth;
    priv->nav = nav;
    priv->active = 1;
    navit_safety_config_default(&priv->config);

    dbg(lvl_info, "Navit Safety plugin: OSD initialized (remote_mode=%d, fuel_buffer_remote=%d km)",
        priv->config.remote_mode, priv->config.fuel_buffer_remote_km);

    return (struct osd_priv *)priv;
}

/**
 * @brief Plugin entry point. Registers the OSD so config can use <osd type=\"navit_safety\"/>.
 */
void plugin_init(void) {
    dbg(lvl_debug, "Navit Safety plugin initializing");
    plugin_register_category_osd("navit_safety", navit_safety_osd_new);
}

#endif /* NAVIT_SAFETY_WITH_OSD */
