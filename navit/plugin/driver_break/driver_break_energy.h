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

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_ENERGY_H
#define NAVIT_PLUGIN_DRIVER_BREAK_ENERGY_H

#include "config.h"
#include "coord.h"
#include "driver_break.h"

/* Energy-based routing model (inspired by BRouter's KinematicModel) */
struct energy_model {
    /* Physical parameters */
    double totalweight;      /* Total weight (kg) - vehicle + cargo + person */
    double f_roll;           /* Rolling resistance coefficient */
    double f_air;            /* Air resistance coefficient */
    double f_recup;          /* Recuperation coefficient (for downhill) */
    double p_standby;        /* Standby power (W) */
    double recup_efficiency; /* Recuperation efficiency (0.0-1.0) */
    double outside_temp;     /* Outside temperature (Celsius) */

    /* Speed parameters */
    double vmax; /* Maximum speed (m/s) */

    /* Derived values */
    double pw;    /* Balance power (W) */
    double cost0; /* Minimum possible cost per meter */
};

/* Energy calculation result */
struct energy_result {
    double energy; /* Energy consumed (Joules) */
    double time;   /* Time taken (seconds) */
    double cost;   /* Routing cost (normalized) */
};

/* Initialize energy model */
void energy_model_init(struct energy_model *model, double totalweight, double f_roll, double f_air, double p_standby);

/* f_air coefficient (N*s^2/m^2) from Cd and frontal area at sea-level air density (1.225 kg/m^3) */
double driver_break_energy_f_air_from_drag(double cd, double frontal_area_sqm);

/* Fill model from plugin config (rolling resistance ~Crr*m*g with Crr=0.015, air from Cd and area) */
void driver_break_energy_model_from_config(struct energy_model *model, const struct driver_break_config *cfg);

/* Calculate energy cost for a segment */
struct energy_result energy_calculate_segment(struct energy_model *model, double distance, /* meters */
                                              double delta_h,      /* elevation change (meters) */
                                              double elevation,    /* current elevation (meters) */
                                              double speed_limit); /* speed limit (m/s) */

/* Calculate gradient from elevation change */
double energy_calculate_gradient(double distance, double delta_h);

/* Get effective speed limit considering model constraints */
double energy_get_effective_speed(struct energy_model *model, double speed_limit);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_ENERGY_H */
