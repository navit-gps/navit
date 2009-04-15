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

#ifndef NAVIT_ATTR_H
#define NAVIT_ATTR_H

#ifdef __cplusplus
extern "C" {
#endif
#ifndef ATTR_H
#define ATTR_H

#include "projection.h"

enum item_type;

enum attr_type {
#define ATTR2(x,y) attr_##y=x,
#define ATTR(x) attr_##x,
#include "attr_def.h"
#undef ATTR2
#undef ATTR
};

#define AF_ONEWAY		(1<<0)
#define AF_ONEWAYREV		(1<<1)
#define AF_NOPASS		(AF_ONEWAY|AF_ONEWAYREV)
#define AF_ONEWAYMASK		(AF_ONEWAY|AF_ONEWAYREV)
#define AF_SEGMENTED		(1<<2)
#define AF_ROUNDABOUT 		(1<<3)
#define AF_ROUNDABOUT_VALID	(1<<4)
#define AF_ONEWAY_EXCEPTION	(1<<5)
#define AF_SPEED_LIMIT		(1<<6)
#define AF_TRUCK_SPEED_LIMIT	(1<<7)
#define AF_SIZE_OR_WEIGHT_LIMIT	(1<<8)
#define AF_THROUGH_TRAFFIC	(1<<9)
#define AF_TOLL			(1<<10)
#define AF_SEASONAL		(1<<11)
#define AF_UNPAVED		(1<<12)
#define AF_DANGEROUS_GOODS	(1<<19)
#define AF_EMERGENCY_VEHICLES	(1<<20)
#define AF_TRANSPORT_TRUCK	(1<<21)
#define AF_DELIVERY_TRUCK	(1<<22)
#define AF_PUBLIC_BUS		(1<<23)
#define AF_TAXI			(1<<24)	
#define AF_HIGH_OCCUPANCY_CAR	(1<<25)	
#define AF_CAR			(1<<26)	
#define AF_MOTORCYCLE		(1<<27)	
#define AF_MOPED		(1<<28)	
#define AF_HORSE		(1<<29)	
#define AF_BIKE			(1<<30)	
#define AF_PEDESTRIAN		(1<<31)	

/* Values for attributes that could carry relative values */
#define ATTR_REL_MAXABS			0x40000000
#define ATTR_REL_RELSHIFT		0x60000000

struct attr {
	enum attr_type type;
	union {
		char *str;
		void *data;
		int num;
		struct item *item;
		enum item_type item_type;
		enum projection projection;
		double * numd;
		struct color *color;
		struct coord_geo *coord_geo;
		struct navit *navit;
		struct callback *callback;
		struct callback_list *callback_list;
		struct vehicle *vehicle;
		struct layout *layout;
		struct layer *layer;
		struct map *map;
		struct mapset *mapset;
		struct log *log;
		struct route *route;
		struct navigation *navigation;
		struct coord *coord;
		struct pcoord *pcoord;
		struct gui *gui;
		struct graphics *graphics;
		struct tracking *tracking;
		struct itemgra *itemgra;
		struct plugin *plugin;
		struct plugins *plugins;
		struct polygon *polygon;
		struct polyline *polyline;
		struct circle *circle;
		struct text *text;
		struct icon *icon;
		struct image *image;
		struct arrows *arrows;
		struct element *element;
		struct speech *speech;
		struct cursor *cursor;
		struct displaylist *displaylist;
		struct transformation *transformation;
		struct vehicleprofile *vehicleprofile;
		struct roadprofile *roadprofile;
		struct range {
			short min, max;
		} range;
		int *dash;
		enum item_type *item_types;
		long long *num64;
	} u;
};

/* prototypes */
enum attr_type;
struct attr;
struct attr_iter;
struct map;
enum attr_type attr_from_name(const char *name);
char *attr_to_name(enum attr_type attr);
struct attr *attr_new_from_text(const char *name, const char *value);
char *attr_to_text(struct attr *attr, struct map *map, int pretty);
struct attr *attr_search(struct attr **attrs, struct attr *last, enum attr_type attr);
int attr_generic_get_attr(struct attr **attrs, struct attr **def_attrs, enum attr_type type, struct attr *attr, struct attr_iter *iter);
struct attr **attr_generic_set_attr(struct attr **attrs, struct attr *attr);
struct attr **attr_generic_add_attr(struct attr **attrs, struct attr *attr);
struct attr **attr_generic_remove_attr(struct attr **attrs, struct attr *attr);
int attr_data_size(struct attr *attr);
void *attr_data_get(struct attr *attr);
void attr_data_set(struct attr *attr, void *data);
void attr_data_set_le(struct attr * attr, void * data);
void attr_free(struct attr *attr);
struct attr *attr_dup(struct attr *attr);
void attr_list_free(struct attr **attrs);
struct attr **attr_list_dup(struct attr **attrs);
/* end of prototypes */
#endif
#ifdef __cplusplus
}
#endif

#endif
