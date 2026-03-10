/**
 * @file navit_safety.h
 * @brief Public configuration types for the Navit Safety plugin.
 *
 * This header exposes configuration structures and enums used by the
 * Navit Safety plugin so they can be tested and, if needed, reused by
 * other components.
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef NAVIT_PLUGIN_NAVIT_SAFETY_H
#define NAVIT_PLUGIN_NAVIT_SAFETY_H

/** @brief Remote mode: off, auto-detect, or always on */
enum navit_safety_remote_mode {
    NAVIT_SAFETY_REMOTE_OFF = 0,
    NAVIT_SAFETY_REMOTE_AUTO = 1,
    NAVIT_SAFETY_REMOTE_ALWAYS = 2
};

/** @brief Plugin configuration (defaults per spec) */
struct navit_safety_config {
    int remote_mode;                 /**< Off / Auto / Always; default Auto */
    int poi_density_threshold_km;    /**< Inter-POI spacing above which remote activates; default 80 */
    int koppen_trigger;             /**< Use Köppen climate zone; default 1 */
    int fuel_buffer_standard_km;    /**< Standard fuel buffer; default 25 */
    int fuel_buffer_remote_km;      /**< Remote fuel buffer; default 85 */
    int fuel_buffer_arid_km;        /**< Arid/desert fuel buffer; default 140 */
    int water_buffer_standard_km;   /**< Standard water buffer (foot); default 5 */
    int water_buffer_remote_km;     /**< Remote water buffer; default 20 */
    int water_buffer_arid_km;       /**< Arid water buffer; default 30 */
    int desert_consumption_warning; /**< Warn when desert conditions increase consumption; default 1 */
    int census_depopulation_layer;  /**< Downgrade POIs in depopulating counties; default 1 */
    int chain_api_queries;          /**< Query chain operator APIs at plan time; default 1 */
    int unconfirmed_poi_display;    /**< Show low-confidence POIs with warning; default 1 */
    int foot_travel_mode;           /**< Enable water source planning; default 0 */
    int body_weight_kg;             /**< For water consumption (foot); default 70 */
    int heat_stress_warnings;       /**< WBGT-based heat illness warnings; default 1 */
};

/** @brief Initialize configuration to default values. */
void navit_safety_config_default(struct navit_safety_config *config);

#endif /* NAVIT_PLUGIN_NAVIT_SAFETY_H */

