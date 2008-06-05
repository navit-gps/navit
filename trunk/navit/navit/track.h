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

#ifdef __cplusplus
extern "C" {
#endif
/* prototypes */
struct coord;
struct mapset;
struct street_data;
struct tracking;
struct coord *tracking_get_pos(struct tracking *tr);
int tracking_get_segment_pos(struct tracking *tr);
struct street_data *tracking_get_street_data(struct tracking *tr);
int tracking_update(struct tracking *tr, struct coord *c, int angle);
struct tracking *tracking_new(struct mapset *ms);
void tracking_set_mapset(struct tracking *this_, struct mapset *ms);
int tracking_get_current_attr(struct tracking *_this, enum attr_type type, struct attr *attr);
struct item *tracking_get_current_item(struct tracking *_this);
void tracking_destroy(struct tracking *tr);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
