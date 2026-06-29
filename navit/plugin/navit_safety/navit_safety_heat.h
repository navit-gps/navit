/**
 * @file navit_safety_heat.h
 * @brief Heat-stress assessment and foot-travel water requirement.
 *
 * Implements the foot-travel heat model from the spec: a WBGT-based heat
 * illness level (caution / warning / danger) and an estimated water
 * requirement derived from body weight, heat, and exertion. See
 * docs/user/plugins/navit_safety.rst ("Foot travel and water sources").
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef NAVIT_PLUGIN_NAVIT_SAFETY_HEAT_H
#define NAVIT_PLUGIN_NAVIT_SAFETY_HEAT_H

struct navit_safety_config;

/** @brief Heat-illness risk level by WBGT (wet-bulb globe temperature). */
enum navit_safety_heat_level {
    NAVIT_SAFETY_HEAT_NONE = 0, /**< Below 28 C WBGT: no specific warning. */
    NAVIT_SAFETY_HEAT_CAUTION,  /**< 28 C and above: caution for strenuous activity. */
    NAVIT_SAFETY_HEAT_WARNING,  /**< 32 C and above: strong warning. */
    NAVIT_SAFETY_HEAT_DANGER    /**< 35 C and above: avoid sustained exertion. */
};

/** @brief Outcome of a foot-travel heat plan. */
struct navit_safety_heat_result {
    enum navit_safety_heat_level level; /**< Risk level (NONE when warnings are disabled). */
    int water_ml_per_hour;              /**< Estimated water requirement, millilitres per hour. */
    int avoid_exertion;                 /**< Nonzero when sustained exertion should be avoided. */
};

/**
 * @brief Classify a WBGT value into a heat-illness level.
 * @param wbgt_c Wet-bulb globe temperature in degrees Celsius.
 * @return The heat-illness level.
 */
enum navit_safety_heat_level navit_safety_heat_assess(double wbgt_c);

/**
 * @brief Estimate water requirement for one hour of travel.
 *
 * Combines a resting maintenance term (scaled by body weight), an exertion term
 * for strenuous activity, and a heat term above 25 C WBGT.
 *
 * @param body_weight_kg Body weight in kilograms (non-positive yields 0).
 * @param wbgt_c Wet-bulb globe temperature in degrees Celsius.
 * @param strenuous Nonzero for strenuous activity (hiking with load, fast pace).
 * @return Water requirement in millilitres per hour.
 */
int navit_safety_heat_water_ml_per_hour(int body_weight_kg, double wbgt_c, int strenuous);

/**
 * @brief Build a foot-travel heat plan from the plugin configuration.
 *
 * The water requirement always uses config->body_weight_kg. The heat level and
 * the avoid-exertion flag are only reported when config->heat_stress_warnings is
 * enabled.
 *
 * @param config Plugin configuration (NULL yields a zeroed result).
 * @param wbgt_c Wet-bulb globe temperature in degrees Celsius.
 * @param strenuous Nonzero for strenuous activity.
 * @param out Result, filled on return (must not be NULL).
 */
void navit_safety_heat_plan(const struct navit_safety_config *config, double wbgt_c, int strenuous,
                            struct navit_safety_heat_result *out);

#endif /* NAVIT_PLUGIN_NAVIT_SAFETY_HEAT_H */
