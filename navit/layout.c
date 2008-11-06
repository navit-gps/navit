/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <glib.h>
#include <string.h>
#include "item.h"
#include "attr.h"
#include "layout.h"
#include "coord.h"
#include "debug.h"

struct layout * layout_new(struct attr *parent, struct attr **attrs)
{
	struct layout *l;
	struct color def_color = {0xffff, 0xefef, 0xb7b7, 0xffff};
	struct attr *name_attr,*color_attr,*order_delta_attr,*font_attr;

	if (! (name_attr=attr_search(attrs, NULL, attr_name)))
		return NULL;
	l = g_new0(struct layout, 1);
	l->name = g_strdup(name_attr->u.str);
	if ((font_attr=attr_search(attrs, NULL, attr_font))) {
		l->font = g_strdup(font_attr->u.str);
	}
	if ((color_attr=attr_search(attrs, NULL, attr_color)))
		l->color = *color_attr->u.color;
	else
		l->color = def_color;
	if ((order_delta_attr=attr_search(attrs, NULL, attr_order_delta)))
		l->order_delta=order_delta_attr->u.num;
	return l;
}

int
layout_add_attr(struct layout *layout, struct attr *attr)
{
	switch (attr->type) {
	case attr_layer:
		layout->layers = g_list_append(layout->layers, attr->u.layer);
		return 1;
	default:
		return 0;
	}
}



struct layer * layer_new(struct attr *parent, struct attr **attrs)
{
	struct layer *l;

	struct attr *name, *details;
	l = g_new0(struct layer, 1);
	name=attr_search(attrs, NULL, attr_name);
	if (name)
		l->name = g_strdup(name->u.str);
	details=attr_search(attrs, NULL, attr_details);
	if (details)
		l->details = details->u.num;
	return l;
}

int
layer_add_attr(struct layer *layer, struct attr *attr)
{
	switch (attr->type) {
	case attr_itemgra:
		layer->itemgras = g_list_append(layer->itemgras, attr->u.itemgra);
		return 1;
	default:
		return 0;
	}
}


struct itemgra * itemgra_new(struct attr *parent, struct attr **attrs)
{
	struct itemgra *itm;
	struct attr *order, *item_types, *speed_range, *angle_range, *sequence_range;
	enum item_type *type;
	struct range defrange;
	
	itm = g_new0(struct itemgra, 1);
	order=attr_search(attrs, NULL, attr_order);
	item_types=attr_search(attrs, NULL, attr_item_types);
	speed_range=attr_search(attrs, NULL, attr_speed_range);
	angle_range=attr_search(attrs, NULL, attr_angle_range);
	sequence_range=attr_search(attrs, NULL, attr_sequence_range);
	defrange.min=0;
	defrange.max=32767;
	if (order) 
		itm->order=order->u.range;
	else 
		itm->order=defrange;
	if (speed_range) 
		itm->speed_range=speed_range->u.range;
	else 
		itm->speed_range=defrange;
	if (angle_range) 
		itm->angle_range=angle_range->u.range;
	else 
		itm->angle_range=defrange;
	if (sequence_range) 
		itm->sequence_range=sequence_range->u.range;
	else 
		itm->sequence_range=defrange;
	if (item_types) {
		type=item_types->u.item_types;
		while (type && *type != type_none) {
			itm->type=g_list_append(itm->type, GINT_TO_POINTER(*type));
			type++;
		}
	}
	return itm;
}
int
itemgra_add_attr(struct itemgra *itemgra, struct attr *attr)
{
	switch (attr->type) {
	case attr_polygon:
	case attr_polyline:
	case attr_circle:
	case attr_text:
	case attr_icon:
	case attr_image:
	case attr_arrows:
		itemgra->elements = g_list_append(itemgra->elements, attr->u.element);
		return 1;
	default:
		dbg(0,"unknown: %s\n", attr_to_name(attr->type));
		return 0;
	}
}

static void
element_set_color(struct element *e, struct attr **attrs)
{
	struct attr *color;
	color=attr_search(attrs, NULL, attr_color);
	if (color)
		e->color=*color->u.color;
}

static void
element_set_text_size(struct element *e, struct attr **attrs)
{
	struct attr *text_size;
	text_size=attr_search(attrs, NULL, attr_text_size);
	if (text_size)
		e->text_size=text_size->u.num;
}

static void
element_set_polyline_width(struct element *e, struct attr **attrs)
{
	struct attr *width;
	width=attr_search(attrs, NULL, attr_width);
	if (width)
		e->u.polyline.width=width->u.num;
}

static void
element_set_polyline_directed(struct element *e, struct attr **attrs)
{
	struct attr *directed;
	directed=attr_search(attrs, NULL, attr_directed);
	if (directed)
		e->u.polyline.directed=directed->u.num;
}

static void
element_set_polyline_dash(struct element *e, struct attr **attrs)
{
	struct attr *dash;
	int i;

	dash=attr_search(attrs, NULL, attr_dash);
	if (dash) {
		for (i=0; i<4; i++) {
			if (!dash->u.dash[i])
				break;
			e->u.polyline.dash_table[i] = dash->u.dash[i];
		}
		e->u.polyline.dash_num=i;
	}
}

static void
element_set_polyline_offset(struct element *e, struct attr **attrs)
{
	struct attr *offset;
	offset=attr_search(attrs, NULL, attr_offset);
	if (offset)
		e->u.polyline.offset=offset->u.num;
}

static void
element_set_circle_width(struct element *e, struct attr **attrs)
{
	struct attr *width;
	width=attr_search(attrs, NULL, attr_width);
	if (width)
		e->u.circle.width=width->u.num;
}

static void
element_set_circle_radius(struct element *e, struct attr **attrs)
{
	struct attr *radius;
	radius=attr_search(attrs, NULL, attr_radius);
	if (radius)
		e->u.circle.radius=radius->u.num;
}

struct polygon *
polygon_new(struct attr *parent, struct attr **attrs)
{
	struct element *e;
	e = g_new0(struct element, 1);
	e->type=element_polygon;
	element_set_color(e, attrs);

	return (struct polygon *)e;
}

struct polyline *
polyline_new(struct attr *parent, struct attr **attrs)
{
	struct element *e;
	
	e = g_new0(struct element, 1);
	e->type=element_polyline;
	element_set_color(e, attrs);
	element_set_polyline_width(e, attrs);
	element_set_polyline_directed(e, attrs);
	element_set_polyline_dash(e, attrs);
	element_set_polyline_offset(e, attrs);
	return (struct polyline *)e;
}

struct circle *
circle_new(struct attr *parent, struct attr **attrs)
{
	struct element *e;

	e = g_new0(struct element, 1);
	e->type=element_circle;
	element_set_color(e, attrs);
	element_set_text_size(e, attrs);
	element_set_circle_width(e, attrs);
	element_set_circle_radius(e, attrs);

	return (struct circle *)e;
}

struct text *
text_new(struct attr *parent, struct attr **attrs)
{
	struct element *e;
	
	e = g_new0(struct element, 1);
	e->type=element_text;
	element_set_text_size(e, attrs);

	return (struct text *)e;
}

struct icon *
icon_new(struct attr *parent, struct attr **attrs)
{
	struct element *e;
	struct attr *src,*w,*h;
	src=attr_search(attrs, NULL, attr_src);
	if (! src)
		return NULL;

	e = g_malloc0(sizeof(*e)+strlen(src->u.str)+1);
	e->type=element_icon;
	e->u.icon.src=(char *)(e+1);
	if ((w=attr_search(attrs, NULL, attr_w)))
		e->u.icon.width=w->u.num;
	else
		e->u.icon.width=-1;
	if ((h=attr_search(attrs, NULL, attr_h)))
		e->u.icon.height=h->u.num;
	else
		e->u.icon.height=-1;
	strcpy(e->u.icon.src,src->u.str);

	return (struct icon *)e;	
}

struct image *
image_new(struct attr *parent, struct attr **attrs)
{
	struct element *e;

	e = g_malloc0(sizeof(*e));
	e->type=element_image;

	return (struct image *)e;	
}

struct arrows *
arrows_new(struct attr *parent, struct attr **attrs)
{
	struct element *e;
	e = g_malloc0(sizeof(*e));
	e->type=element_arrows;
	element_set_color(e, attrs);
	return (struct arrows *)e;	
}

int
element_add_attr(struct element *e, struct attr *attr)
{
	switch (attr->type) {
	case attr_coord:
		e->coord=g_realloc(e->coord,(e->coord_count+1)*sizeof(struct coord));
		e->coord[e->coord_count++]=*attr->u.coord;
		return 1;
	default:
		return 0;
	}
}
