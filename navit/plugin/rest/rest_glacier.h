/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024 Navit Team
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

#ifndef NAVIT_PLUGIN_REST_GLACIER_H
#define NAVIT_PLUGIN_REST_GLACIER_H

#include "config.h"
#include "coord.h"
#include "rest.h"

/* Minimum distance from glaciers for nightly camping (meters) */
#define GLACIER_MIN_CAMPING_DISTANCE 300.0  /* 300 meters */

/* Check if position is too close to glacier for camping */
int glacier_is_too_close_for_camping(struct coord_geo *position, 
                                      struct mapset *ms,
                                      int has_camping_building);

/* Find nearby glaciers */
GList *glacier_find_nearby(struct coord_geo *position, double radius_km, struct mapset *ms);

/* Free glacier list */
void glacier_free_list(GList *glaciers);

/* Get minimum camping distance from glaciers */
double glacier_get_min_camping_distance(void);

#endif /* NAVIT_PLUGIN_REST_GLACIER_H */
