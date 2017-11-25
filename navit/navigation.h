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

#define FEET_PER_METER  3.2808399
#define FEET_PER_MILE   5280
#define KILOMETERS_TO_MILES	0.62137119	/* Kilometers to miles */

/* It appears that distances to be displayed, such as distances to
 * maneuvers, are in meters. Multiply that by METERS_PER_MILE and you
 * have miles. */
#define METERS_TO_MILES (KILOMETERS_TO_MILES/1000.0) /* Meters to miles */
/* #define METERS_PER_MILE (1000.0/KILOMETERS_TO_MILES) */

/* Meters per second to kilometers per hour. GPSD delivers speeds in
 * meters per second. */
#define MPS_TO_KPH	3.6

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Values for the {@code nav_status} attribute
 */
enum nav_status {
	status_invalid = -2,		/*!< Status is unknown. The {@code nav_status} attribute will never return this
								     value but code that listens to changes to this attribute may use this value
								     as a placeholder until the first actual status has been obtained. */
	status_no_route = -1,		/*!< No route was found */
	status_no_destination = 0,	/*!< No destination set, not routing */
	status_position_wait = 1,	/*!< Destination is set but current position is unknown */
	status_calculating = 2,		/*!< A new route is being calculated and turn instructions are being generated */
	status_recalculating = 3,	/*!< The existing route is being recalculated, along with its turn instructions.
								     Note that as the vehicle follows a route, status will flip between
								     {@code status_routing} and {@code status_recalculating} with every position
								     update. */
	status_routing = 4,			/*!< A route with turn instructions has been calculated and the user is being
								     guided along it */
};


/* prototypes */
enum attr_type;
enum item_type;
struct attr;
struct attr_iter;
struct callback;
struct map;
struct navigation;
struct route;
char *nav_status_to_text(int status);
int navigation_get_attr(struct navigation *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int navigation_set_attr(struct navigation *this_, struct attr *attr);
struct navigation *navigation_new(struct attr *parent, struct attr **attrs);
int navigation_set_announce(struct navigation *this_, enum item_type type, int *level);
void navigation_destroy(struct navigation *this_);
int navigation_register_callback(struct navigation *this_, enum attr_type type, struct callback *cb);
void navigation_unregister_callback(struct navigation *this_, enum attr_type type, struct callback *cb);
struct map *navigation_get_map(struct navigation *this_);
void navigation_set_route(struct navigation *this_, struct route *route);
void navigation_init(void);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
