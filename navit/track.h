/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
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

#ifndef NAVIT_TRACK_H
#define NAVIT_TRACK_H
#include <time.h>
#include "xmlconfig.h"
#include "coord.h"
#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
enum attr_type;
enum projection;
struct attr;
struct attr_iter;
struct coord;
struct item;
struct map;
struct mapset;
struct route;
struct street_data;
struct tracking;
struct vehicle;
struct vehicleprofile;
int tracking_get_angle(struct tracking *tr);
struct coord *tracking_get_pos(struct tracking *tr);
int tracking_get_street_direction(struct tracking *tr);
int tracking_get_segment_pos(struct tracking *tr);
struct street_data *tracking_get_street_data(struct tracking *tr);
int tracking_get_attr(struct tracking *_this, enum attr_type type, struct attr *attr, struct attr_iter *attr_iter);
struct item *tracking_get_current_item(struct tracking *_this);
int *tracking_get_current_flags(struct tracking *_this);
int tracking_get_current_tunnel(struct tracking *_this);
void tracking_flush(struct tracking *tr);
void tracking_update(struct tracking *tr, struct vehicle *v, struct vehicleprofile *vehicleprofile, enum projection pro);
int tracking_set_attr(struct tracking *tr, struct attr *attr);
struct tracking *tracking_new(struct attr *parent, struct attr **attrs);
void tracking_set_mapset(struct tracking *this_, struct mapset *ms);
void tracking_set_route(struct tracking *this_, struct route *rt);
void tracking_destroy(struct tracking *tr);
struct map *tracking_get_map(struct tracking *this_);
int tracking_add_attr(struct tracking *this_, struct attr *attr);
int tracking_remove_attr(struct tracking *this_, struct attr *attr);
struct tracking *tracking_ref(struct tracking *this_);
void tracking_unref(struct tracking *this_);
void tracking_init(void);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
