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

#ifndef NAVIT_LAYOUT_H
#define NAVIT_LAYOUT_H

#include "item.h"
#include "color.h"

struct element_line;
struct element_text;

struct element {
	enum { element_point, element_polyline, element_polygon, element_circle, element_text, element_icon, element_image, element_arrows } type;
	struct color color;
	int text_size;
	union {
		struct element_point {
		} point;
		struct element_polyline {
			int width;
			int directed;
			int dash_num;
			int offset;
			unsigned char dash_table[4];
		} polyline;
		struct element_polygon {
		} polygon;
		struct element_circle {
			int width;
			int radius;
		} circle;
		struct element_icon {
			char *src;
			int width;
			int height;
		} icon;
	} u;
};


struct itemgra { 
	struct order order;
	GList *type;
	GList *elements;
};

struct color;

struct layer { char *name; int details; GList *itemgras; };

struct layout { char *name; char *font; struct color color; GList *layers; int order_delta; };

/* prototypes */
enum item_type;
struct element;
struct itemgra;
struct layer;
struct layout;
struct layout *layout_new(struct attr *parent, struct attr **attrs);
int layout_add_attr(struct layout *layout, struct attr *attr);
struct layer *layer_new(struct attr *parent, struct attr **attrs);
int layer_add_attr(struct layer *layer, struct attr *attr);
struct itemgra *itemgra_new(struct attr *parent, struct attr **attrs);
int itemgra_add_attr(struct itemgra *itemgra, struct attr *attr);
struct polygon *polygon_new(struct attr *parent, struct attr **attrs);
struct polyline *polyline_new(struct attr *parent, struct attr **attrs);
struct circle *circle_new(struct attr *parent, struct attr **attrs);
struct text *text_new(struct attr *parent, struct attr **attrs);
struct icon *icon_new(struct attr *parent, struct attr **attrs);
struct image *image_new(struct attr *parent, struct attr **attrs);
struct arrows *arrows_new(struct attr *parent, struct attr **attrs);
#endif

