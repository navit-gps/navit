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

#ifndef NAVIT_PLUGIN_REST_POI_H
#define NAVIT_PLUGIN_REST_POI_H

#include "config.h"
#include "coord.h"
#include "rest.h"

/* POI discovery using Overpass API */
GList *rest_poi_discover(struct coord_geo *center, int radius_km, 
                         const char **poi_categories, int num_categories);

/* POI ranking */
void rest_poi_rank(GList *pois, struct coord_geo *rest_stop, 
                   struct rest_config *config);

/* Free POI list */
void rest_poi_free_list(GList *pois);

/* Free single POI */
void rest_poi_free(struct rest_poi *poi);

#endif /* NAVIT_PLUGIN_REST_POI_H */
