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

#ifndef NAVIT_VEHICLE_H
#define NAVIT_VEHICLE_H

#ifdef __cplusplus
extern "C" {
#endif
struct vehicle;
struct vehicle_priv;
enum attr_type;
struct attr;

struct vehicle_methods {
	void (*destroy)(struct vehicle_priv *priv);
	int (*position_attr_get)(struct vehicle_priv *priv, enum attr_type type, struct attr *attr);
	int (*set_attr)(struct vehicle_priv *priv, struct attr *attr, struct attr **attrs);

};

/* prototypes */
struct vehicle *vehicle_new(struct attr **attrs);
int vehicle_get_attr(struct vehicle *this_, enum attr_type type, struct attr *attr);
int vehicle_set_attr(struct vehicle *this_, struct attr *attr, struct attr **attrs);
int vehicle_add_attr(struct vehicle *this_, struct attr *attr);
int vehicle_remove_attr(struct vehicle *this_, struct attr *attr);
void vehicle_destroy(struct vehicle *this_);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif

