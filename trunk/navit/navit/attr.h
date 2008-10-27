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

#define AF_ONEWAY	(1<<0)
#define AF_ONEWAYREV	(1<<1)
#define AF_NOPASS	(AF_ONEWAY|AF_ONEWAYREV)
#define AF_ONEWAYMASK	(AF_ONEWAY|AF_ONEWAYREV)
#define AF_SEGMENTED	(1<<2)


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
		struct order {
			short min, max;
		} order;
		int *dash;
		enum item_type *item_types;
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
int attr_data_size(struct attr *attr);
void *attr_data_get(struct attr *attr);
void attr_data_set(struct attr *attr, void *data);
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
