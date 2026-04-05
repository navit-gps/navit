/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "driver_break_energy.h"
#include "config.h"
#include "debug.h"
#include "driver_break_srtm.h"
#include <math.h>
#include <string.h>

/* Initialize energy model with default or custom parameters */
void energy_model_init(struct energy_model *model, double totalweight, double f_roll, double f_air, double p_standby) {
    memset(model, 0, sizeof(*model));

    model->totalweight = totalweight;
    model->f_roll = f_roll;
    model->f_air = f_air;
    model->p_standby = p_standby;
    model->f_recup = 400.0;        /* Default recuperation coefficient */
    model->recup_efficiency = 0.7; /* 70% efficiency */
    model->outside_temp = 20.0;    /* 20 degrees Celsius */
    model->vmax = 80.0 / 3.6;      /* 80 km/h in m/s */

    /* Calculate balance power: 2 * f_air * vmax^3 - p_standby */
    model->pw = 2.0 * model->f_air * model->vmax * model->vmax * model->vmax - model->p_standby;

    /* Calculate minimum cost per meter */
    model->cost0 = (model->pw + model->p_standby) / model->vmax + model->f_roll
                   + model->f_air * model->vmax * model->vmax;

    dbg(lvl_info,
        "Driver Break plugin: Energy model init totalweight=%.1f kg f_roll=%.4f f_air=%.4f p_standby=%.1f W "
        "vmax=%.1f m/s",
        model->totalweight, model->f_roll, model->f_air, model->p_standby, model->vmax);
}

/* Calculate gradient percentage */
double energy_calculate_gradient(double distance, double delta_h) {
    if (distance <= 0.0)
        return 0.0;
    return (delta_h / distance) * 100.0; /* Percentage gradient */
}

/* Get effective speed considering model constraints */
double energy_get_effective_speed(struct energy_model *model, double speed_limit) {
    if (!model)
        return speed_limit;
    if (speed_limit > model->vmax)
        return model->vmax;
    return speed_limit;
}

/* Calculate energy cost for a segment (simplified kinematic model) */
struct energy_result energy_calculate_segment(struct energy_model *model, double distance, double delta_h,
                                              double elevation, double speed_limit) {
    struct energy_result result;
    memset(&result, 0, sizeof(result));

    if (!model || distance <= 0.0) {
        result.cost = 1e9; /* Very high cost for invalid segments */
        return result;
    }

    /* Get effective speed */
    double v = energy_get_effective_speed(model, speed_limit);
    if (v <= 0.0) {
        result.cost = 1e9;
        return result;
    }

    /* Temperature correction (linear) */
    double tcorr = (20.0 - model->outside_temp) * 0.0035;

    /* Air pressure correction (decreases 1mb per 8m elevation) */
    double ecorr = 0.0001375 * (elevation - 100.0);

    /* Adjusted air resistance */
    double f_air_adj = model->f_air * (1.0 + tcorr - ecorr);

    /* Elevation force (gravity component) */
    double fh = delta_h * model->totalweight * 9.81 / distance;

    /* Calculate time */
    double time = distance / v;

    /* Calculate forces */
    double f_roll_force = model->f_roll;
    double f_air_force = f_air_adj * v * v;
    double f_total = f_roll_force + f_air_force + fh;

    /* Recuperation for downhill (negative fh) */
    double f_recup_force = 0.0;
    if (fh < 0.0) {
        /* Downhill: can recuperate energy */
        f_recup_force = -fh * model->recup_efficiency;
        f_total = f_roll_force + f_air_force + fh + f_recup_force;
    }

    /* Energy calculation */
    double work = f_total * distance;                /* Work done (Joules) */
    double standby_energy = model->p_standby * time; /* Standby energy (Joules) */

    result.energy = work + standby_energy;
    result.time = time;

    /* Cost = (power * time + energy) / cost0 */
    double power_cost = model->pw * time;
    result.cost = (power_cost + result.energy) / model->cost0;

    dbg(lvl_info,
        "Driver Break plugin: Energy segment dist=%.1f m delta_h=%.1f m elev=%.1f m speed=%.1f m/s "
        "f_roll=%.4f f_air=%.4f fh=%.4f f_recup=%.4f work=%.1f J standby=%.1f J cost=%.4f",
        distance, delta_h, elevation, v, f_roll_force, f_air_force, fh, f_recup_force, work, standby_energy,
        result.cost);

    if (result.cost < 0.0) {
        dbg(lvl_warning, "Driver Break plugin: Energy segment cost negative (%.4f), check recuperation parameters",
            result.cost);
    }

    return result;
}
