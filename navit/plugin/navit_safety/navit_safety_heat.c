/**
 * @file navit_safety_heat.c
 * @brief Heat-stress assessment and foot-travel water requirement implementation.
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
#include "navit_safety_heat.h"

/* WBGT thresholds (degrees Celsius) for heat-illness risk, per the spec. */
#define NAVIT_SAFETY_WBGT_CAUTION 28.0
#define NAVIT_SAFETY_WBGT_WARNING 32.0
#define NAVIT_SAFETY_WBGT_DANGER 35.0

/* Water model constants. */
#define NAVIT_SAFETY_WATER_REST_ML_PER_KG 1   /* resting maintenance, ml/kg/hr */
#define NAVIT_SAFETY_WATER_EXERTION_ML_PER_KG 4 /* added when strenuous, ml/kg/hr */
#define NAVIT_SAFETY_WATER_HEAT_BASE_C 25.0   /* heat term starts above this WBGT */
#define NAVIT_SAFETY_WATER_HEAT_ML_PER_C 20.0 /* added per degree above the base */

enum navit_safety_heat_level navit_safety_heat_assess(double wbgt_c) {
    if (wbgt_c >= NAVIT_SAFETY_WBGT_DANGER)
        return NAVIT_SAFETY_HEAT_DANGER;
    if (wbgt_c >= NAVIT_SAFETY_WBGT_WARNING)
        return NAVIT_SAFETY_HEAT_WARNING;
    if (wbgt_c >= NAVIT_SAFETY_WBGT_CAUTION)
        return NAVIT_SAFETY_HEAT_CAUTION;
    return NAVIT_SAFETY_HEAT_NONE;
}

int navit_safety_heat_water_ml_per_hour(int body_weight_kg, double wbgt_c, int strenuous) {
    int ml;

    if (body_weight_kg <= 0)
        return 0;

    ml = body_weight_kg * NAVIT_SAFETY_WATER_REST_ML_PER_KG;
    if (strenuous)
        ml += body_weight_kg * NAVIT_SAFETY_WATER_EXERTION_ML_PER_KG;
    if (wbgt_c > NAVIT_SAFETY_WATER_HEAT_BASE_C)
        ml += (int)((wbgt_c - NAVIT_SAFETY_WATER_HEAT_BASE_C) * NAVIT_SAFETY_WATER_HEAT_ML_PER_C);

    return ml;
}

void navit_safety_heat_plan(const struct navit_safety_config *config, double wbgt_c, int strenuous,
                            struct navit_safety_heat_result *out) {
    if (!out)
        return;

    out->level = NAVIT_SAFETY_HEAT_NONE;
    out->water_ml_per_hour = 0;
    out->avoid_exertion = 0;

    if (!config)
        return;

    out->water_ml_per_hour = navit_safety_heat_water_ml_per_hour(config->body_weight_kg, wbgt_c, strenuous);

    if (config->heat_stress_warnings) {
        out->level = navit_safety_heat_assess(wbgt_c);
        out->avoid_exertion = (out->level == NAVIT_SAFETY_HEAT_DANGER);
    }
}
