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

#ifndef NAVIT_NAVIGATION_H
#define NAVIT_NAVIGATION_H

#ifdef __cplusplus
extern "C" {
#endif
/* prototypes */
enum attr_type;
enum item_type;
struct attr;
struct callback;
struct map;
struct navigation;
struct route;
struct navigation *navigation_new(struct attr **attrs);
int navigation_set_announce(struct navigation *this_, enum item_type type, int *level);
void navigation_update(struct navigation *this_, struct route *route);
void navigation_flush(struct navigation *this_);
void navigation_destroy(struct navigation *this_);
int navigation_register_callback(struct navigation *this_, enum attr_type type, struct callback *cb);
void navigation_unregister_callback(struct navigation *this_, enum attr_type type, struct callback *cb);
struct map *navigation_get_map(struct navigation *this_);
void navigation_init(void);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
