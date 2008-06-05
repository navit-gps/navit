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
#include <glib/gprintf.h>
#include <string.h>
#include "debug.h"
#include "file.h"
#include "coord.h"
#include "layout.h"
#include "mapset.h"
#include "projection.h"
#include "map.h"
#include "navigation.h"
#include "navit.h"
#include "plugin.h"
#include "route.h"
#include "speech.h"
#include "track.h"
#include "vehicle.h"
#include "point.h"
#include "graphics.h"
#include "gui.h"
#include "osd.h"
#include "log.h"
#include "xmlconfig.h"
#include "config.h"

struct xistate {
	struct xistate *parent;
	struct xistate *child;
	const gchar *element;
	gchar **attribute_names;
	gchar **attribute_values;
};

struct xmldocument {
	const gchar *href;
	const gchar *xpointer;	
	gpointer user_data;
	struct xistate *first;
	struct xistate *last;
	int active;
	int level;
};

struct xmlstate {
	const gchar **attribute_names;
	const gchar **attribute_values;
	struct xmlstate *parent;
	struct attr element_attr;
	const gchar *element;
	GError **error;
	struct element_func *func;
	struct xmldocument *document;
};



static struct attr ** convert_to_attrs(struct xmlstate *state)
{
	const gchar **attribute_name=state->attribute_names;
	const gchar **attribute_value=state->attribute_values;
	int count=0;
	struct attr **ret;

	while (*attribute_name) {
		count++;
		attribute_name++;
	}
	ret=g_new(struct attr *, count+1);
	attribute_name=state->attribute_names;
	count=0;
	while (*attribute_name) {
		ret[count]=attr_new_from_text(*attribute_name,*attribute_value);
		if (ret[count])
			count++;
		attribute_name++;
		attribute_value++;
	}	
	ret[count]=NULL;
	dbg(1,"ret=%p\n", ret);
	return ret;
}


static const char * find_attribute(struct xmlstate *state, const char *attribute, int required)
{
	const gchar **attribute_name=state->attribute_names;
	const gchar **attribute_value=state->attribute_values;
	while(*attribute_name) {
		if(! g_ascii_strcasecmp(attribute,*attribute_name))
			return *attribute_value;
		attribute_name++;
		attribute_value++;
	}
	if (required) 
		g_set_error(state->error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT, "element '%s' is missing attribute '%s'", state->element, attribute);
	return NULL;
}

static int
find_color(struct xmlstate *state, int required, struct color *color)
{
	const char *value;
	int r,g,b,a;

	value=find_attribute(state, "color", required);
	if (! value)
		return 0;
	if(strlen(value)==7){
		sscanf(value,"#%02x%02x%02x", &r, &g, &b);
		color->r = (r << 8) | r;
		color->g = (g << 8) | g;
		color->b = (b << 8) | b;
		color->a = (65535);
	} else if(strlen(value)==9){
		sscanf(value,"#%02x%02x%02x%02x", &r, &g, &b, &a);
		color->r = (r << 8) | r;
		color->g = (g << 8) | g;
		color->b = (b << 8) | b;
		color->a = (a << 8) | a;
	} else {
		dbg(0,"color %i has unknown format\n",value);
	}
	return 1;
}

static int
find_order(struct xmlstate *state, int required, int *min, int *max)
{
	const char *value, *pos;
	int ret;

	*min=0;
	*max=18;
	value=find_attribute(state, "order", required);
	if (! value)
		return 0;
	pos=strchr(value, '-');
	if (! pos) {
		ret=sscanf(value,"%d",min);
		*max=*min;
	} else if (pos == value) 
		ret=sscanf(value,"-%d",max);
	else
		ret=sscanf(value,"%d-%d", min, max);
	return ret;
}

static int
find_boolean(struct xmlstate *state, const char *attribute, int deflt, int required)
{
	const char *value;

	value=find_attribute(state, attribute, required);
	if (! value)
		return deflt;
	if (g_ascii_strcasecmp(value,"no") && g_ascii_strcasecmp(value,"0") && g_ascii_strcasecmp(value,"false")) 
		return 1;
	return 0;
}

static int
convert_number(const char *val)
{
	if (val)
		return g_ascii_strtoull(val,NULL,0);
	else
		return 0;
}

static int
convert_number_list(const char *val, int *table, int size)
{
	char *tok,*str,*val_str,*saveptr=NULL;
	int i;
	str=val_str=g_strdup(val);
	for (i=0; i<size && (tok=strtok_r(str, ",", &saveptr)); i++) {
		table[i]=convert_number(tok);
		str=NULL;
	}
	g_free(val_str);
    return i;
}

static int
xmlconfig_config(struct xmlstate *state)
{
	state->element_attr.u.data = (void *)1;
	return 1;
}

static int
xmlconfig_plugin(struct xmlstate *state)
{
	const char *path;
	int active,lazy;

	state->element_attr.u.data=state->parent->element_attr.u.data;
	path=find_attribute(state, "path", 1);
	if (! path)
		return 0;
	active=find_boolean(state, "active", 1, 0);
	lazy=find_boolean(state, "lazy", 1, 0);
	plugins_add_path(state->parent->element_attr.u.data, path, active, lazy);
	return 1;
}

static int
xmlconfig_speech(struct xmlstate *state)
{
	const char *type;
	const char *data;
	type=find_attribute(state, "type", 1);
	if (! type)
		return 0;
	data=find_attribute(state, "data", 0);
	state->element_attr.u.data = speech_new(type, data);
	if (! state->element_attr.u.data)
		return 0;
	navit_set_speech(state->parent->element_attr.u.data, state->element_attr.u.data);
	return 1;
}

static int
xmlconfig_debug(struct xmlstate *state)
{
	const char *name,*level;
	name=find_attribute(state, "name", 1);
	if (! name)
		return 0;
	level=find_attribute(state, "level", 1);
	if (! level)
		return 0;
	debug_level_set(name, convert_number(level));
	return 1;
}

static int
xmlconfig_vehicle(struct xmlstate *state)
{
	struct attr **attrs;
	attrs=convert_to_attrs(state);

	state->element_attr.u.data = vehicle_new(attrs);
	if (! state->element_attr.u.data)
		return 0;
	navit_add_vehicle(state->parent->element_attr.u.data, state->element_attr.u.data, attrs);
	return 1;
}

static int
xmlconfig_log_vehicle(struct xmlstate *state)
{
	struct attr attr;
	struct attr **attrs;
	attrs=convert_to_attrs(state);
	state->element_attr.u.data = log_new(attrs);
	if (! state->element_attr.u.data)
		return 0;
	attr.type=attr_log;
	attr.u.log=state->element_attr.u.data;
	if (vehicle_add_attr(state->parent->element_attr.u.data, &attr))
		return 0;
	return 1;
}

static int
xmlconfig_log_navit(struct xmlstate *state)
{
	struct attr attr;
	struct attr **attrs;
	attrs=convert_to_attrs(state);
	state->element_attr.u.data = log_new(attrs);
	if (! state->element_attr.u.data)
		return 0;
	attr.type=attr_log;
	attr.u.log=state->element_attr.u.data;
	if (navit_add_attr(state->parent->element_attr.u.data, &attr))
		return 0;
	return 1;
}


static int
xmlconfig_window_items(struct xmlstate *state)
{
	int distance=-1;
	enum item_type itype;
	const char *name=find_attribute(state, "name", 1);
	const char *value=find_attribute(state, "distance", 0);
	const char *type=find_attribute(state, "type", 1);
	char *tok,*str,*type_str,*saveptr=NULL;
	if (! name || !type)
		return 0;
	if (value) 
		distance=convert_number(value);
	state->element_attr.u.data = navit_window_items_new(name, distance);
	type_str=g_strdup(type);
	str=type_str;
	while ((tok=strtok_r(str, ",", &saveptr))) {
		itype=item_from_name(tok);
		navit_window_items_add_item(state->element_attr.u.data, itype);
		str=NULL;
	}
	g_free(type_str);

	navit_add_window_items(state->parent->element_attr.u.data, state->element_attr.u.data);

	return 1;
}


static int
xmlconfig_tracking(struct xmlstate *state)
{
	state->element_attr.u.data = tracking_new(NULL);
	navit_tracking_add(state->parent->element_attr.u.data, state->element_attr.u.data);
	return 1;
}

static int
xmlconfig_route(struct xmlstate *state)
{
	struct attr **attrs;
	struct attr route_attr;

	attrs=convert_to_attrs(state);
	state->element_attr.u.data = route_new(attrs);
	if (! state->element_attr.u.data) {
		dbg(0,"Failed to create route object\n");
		return 0;
	}
	route_attr.type=attr_route;
	route_attr.u.route=state->element_attr.u.data;
	return navit_add_attr(state->parent->element_attr.u.data, &route_attr);
}

static int
xmlconfig_speed(struct xmlstate *state)
{
	const char *type;
	const char *value;
	int v;
	enum item_type itype;
	char *saveptr=NULL, *tok, *type_str, *str;

	type=find_attribute(state, "type", 1);
	if (! type)
		return 0;
	value=find_attribute(state, "value", 1);
	if (! value)
		return 0;
	v=convert_number(value);
	type_str=g_strdup(type);
	str=type_str;
	while ((tok=strtok_r(str, ",", &saveptr))) {
		itype=item_from_name(tok);
		route_set_speed(state->parent->element_attr.u.data, itype, v);
		str=NULL;
	}
	g_free(type_str);

	return 1;
}


static int
xmlconfig_navigation(struct xmlstate *state)
{
	struct attr **attrs;
	struct attr navigation_attr;

	attrs=convert_to_attrs(state);
	state->element_attr.u.data = navigation_new(attrs);
	if (! state->element_attr.u.data) {
		dbg(0,"Failed to create navigation object\n");
		return 0;
	}
	navigation_attr.type=attr_navigation;
	navigation_attr.u.navigation=state->element_attr.u.data;
	return navit_add_attr(state->parent->element_attr.u.data, &navigation_attr);
}

static int
xmlconfig_osd(struct xmlstate *state)
{
	struct attr **attrs;
	const char *type=find_attribute(state, "type", 1);
	if (! type)
		return 0;
	attrs=convert_to_attrs(state);
	state->element_attr.u.data = osd_new(state->parent->element_attr.u.data, type, attrs);
	return 1;
}

static int
xmlconfig_announce(struct xmlstate *state)
{
	const char *type,*value;
	char key[32];
	int level[3];
	int i;
	enum item_type itype;
	char *saveptr=NULL, *tok, *type_str, *str;

	type=find_attribute(state, "type", 1);
	if (! type)
		return 0;
	for (i = 0 ; i < 3 ; i++) {
		sprintf(key,"level%d", i);
		value=find_attribute(state, key, 0);
		if (value) 
			level[i]=convert_number(value);
		else
			level[i]=-1;
	}
	type_str=g_strdup(type);
	str=type_str;
	while ((tok=strtok_r(str, ",", &saveptr))) {
		itype=item_from_name(tok);
		navigation_set_announce(state->parent->element_attr.u.data, itype, level);
		str=NULL;
	}
	g_free(type_str);
	return 1;
}

static int
xmlconfig_mapset(struct xmlstate *state)
{
	state->element_attr.u.data = mapset_new();
	if (! state->element_attr.u.data)
		return 0;
	navit_add_mapset(state->parent->element_attr.u.data, state->element_attr.u.data);

	return 1;
}

static int
xmlconfig_map(struct xmlstate *state)
{
	struct attr **attrs;
	const char *type=find_attribute(state, "type", 1);
	if (! type)
		return 0;
	attrs=convert_to_attrs(state);
	state->element_attr.u.data = map_new(type, attrs);
	if (! state->element_attr.u.data)
		return 0;
	if (!find_boolean(state, "active", 1, 0))
		map_set_active(state->element_attr.u.data, 0);
	mapset_add(state->parent->element_attr.u.data, state->element_attr.u.data);

	return 1;
}

static int
xmlconfig_layout(struct xmlstate *state)
{
	const char *name=find_attribute(state, "name", 1);
	struct color color = {0xffff, 0xefef, 0xb7b7, 0xffff};

	if (! name)
		return 0;
	find_color(state, 0, &color);
	state->element_attr.u.data = layout_new(name, &color);
	if (! state->element_attr.u.data)
		return 0;
	navit_add_layout(state->parent->element_attr.u.data, state->element_attr.u.data);
	return 1;
}

static int
xmlconfig_layer(struct xmlstate *state)
{
	const char *name=find_attribute(state, "name", 1);
	if (! name)
		return 0;
	state->element_attr.u.data = layer_new(name, convert_number(find_attribute(state, "details", 0)));
	if (! state->element_attr.u.data)
		return 0;
	layout_add_layer(state->parent->element_attr.u.data, state->element_attr.u.data);
	return 1;
}

static int
xmlconfig_item(struct xmlstate *state)
{
	const char *type=find_attribute(state, "type", 1);
	int min, max;
	enum item_type itype;
	char *saveptr=NULL, *tok, *type_str, *str;

	if (! type)
		return 0;
	if (! find_order(state, 1, &min, &max))
		return 0;
	state->element_attr.u.data=itemtype_new(min, max);
	if (! state->element_attr.u.data)
		return 0;
	type_str=g_strdup(type);
	str=type_str;
	layer_add_itemtype(state->parent->element_attr.u.data, state->element_attr.u.data);
	while ((tok=strtok_r(str, ",", &saveptr))) {
		itype=item_from_name(tok);
		itemtype_add_type(state->element_attr.u.data, itype);
		str=NULL;
	}
	g_free(type_str);

	return 1;
}

static int
xmlconfig_polygon(struct xmlstate *state)
{
	struct color color;

	if (! find_color(state, 1, &color))
		return 0;
	state->element_attr.u.data=polygon_new(&color);
	if (! state->element_attr.u.data)
		return 0;
	itemtype_add_element(state->parent->element_attr.u.data, state->element_attr.u.data);

	return 1;
}

static int
xmlconfig_polyline(struct xmlstate *state)
{
	struct color color;
	const char *width, *dash, *directed;
	int w=0, d=0, dt[4], ds=0;

	if (! find_color(state, 1, &color))
		return 0;
	width=find_attribute(state, "width", 0);
	if (width) 
		w=convert_number(width);
	dash=find_attribute(state, "dash", 0);
	if (dash)
		ds=convert_number_list(dash, dt, sizeof(dt)/sizeof(*dt));
	directed=find_attribute(state, "directed", 0);
	if (directed) 
		d=convert_number(directed);
	
	state->element_attr.u.data=polyline_new(&color, w, d, dt, ds);
	if (! state->element_attr.u.data)
		return 0;
	itemtype_add_element(state->parent->element_attr.u.data, state->element_attr.u.data);

	return 1;
}

static int
xmlconfig_circle(struct xmlstate *state)
{
	struct color color;
	const char *width, *radius, *label_size;
	int w=0,r=0,ls=0;

	if (! find_color(state, 1, &color))
		return 0;
	width=find_attribute(state, "width", 0);
	if (width) 
		w=convert_number(width);
	radius=find_attribute(state, "radius", 0);
	if (radius) 
		r=convert_number(radius);
	label_size=find_attribute(state, "label_size", 0);
	if (label_size) 
		ls=convert_number(label_size);
	state->element_attr.u.data=circle_new(&color, r, w, ls);
	if (! state->element_attr.u.data)
		return 0;
	itemtype_add_element(state->parent->element_attr.u.data, state->element_attr.u.data);

	return 1;
}

static int
xmlconfig_label(struct xmlstate *state)
{
	const char *label_size;
	int ls=0;

	label_size=find_attribute(state, "label_size", 0);
	if (label_size) 
		ls=convert_number(label_size);
	state->element_attr.u.data=label_new(ls);
	if (! state->element_attr.u.data)
		return 0;
	itemtype_add_element(state->parent->element_attr.u.data, state->element_attr.u.data);

	return 1;
}

static int
xmlconfig_icon(struct xmlstate *state)
{
	const char *src=find_attribute(state, "src", 1);

	if (! src)
		return 0;
	state->element_attr.u.data=icon_new(src);
	if (! state->element_attr.u.data)
		return 0;
	itemtype_add_element(state->parent->element_attr.u.data, state->element_attr.u.data);

	return 1;
}

static int
xmlconfig_image(struct xmlstate *state)
{
	state->element_attr.u.data=image_new();
	if (! state->element_attr.u.data)
		return 0;
	itemtype_add_element(state->parent->element_attr.u.data, state->element_attr.u.data);

	return 1;
}

#define NEW(x) (void *(*)(struct attr *, struct attr **))(x)
#define ADD(x) (int (*)(void *, struct attr *attr))(x)
#define INIT(x) (int (*)(void *))(x)
#define DESTROY(x) (void (*)(void *))(x)
struct element_func {
	char *name;
	char *parent;
	int (*func)(struct xmlstate *state);
	void *(*new)(struct attr *parent, struct attr **attrs);
	int (*add_attr)(void *, struct attr *attr);
	int (*init)(void *);
	void (*destroy)(void *);
} elements[] = {
	{ "config", NULL, xmlconfig_config},
	{ "debug", "config", xmlconfig_debug},
	{ "navit", "config", NULL, NEW(navit_new), ADD(navit_add_attr), INIT(navit_init), DESTROY(navit_destroy)},
	{ "graphics", "navit", NULL, NEW(graphics_new), NULL, NULL, NULL},
	{ "gui", "navit", NULL, NEW(gui_new), NULL, NULL, NULL},
	{ "layout", "navit", xmlconfig_layout},
	{ "layer", "layout", xmlconfig_layer},
	{ "item", "layer", xmlconfig_item},
	{ "circle", "item", xmlconfig_circle},
	{ "icon", "item", xmlconfig_icon},
	{ "image", "item", xmlconfig_image},
	{ "label", "item", xmlconfig_label},
	{ "polygon", "item", xmlconfig_polygon},
	{ "polyline", "item", xmlconfig_polyline},
	{ "mapset", "navit", xmlconfig_mapset},
	{ "map",  "mapset", xmlconfig_map},
	{ "navigation", "navit", xmlconfig_navigation},
	{ "osd", "navit", xmlconfig_osd},
	{ "announce", "navigation", xmlconfig_announce},
	{ "speech", "navit", xmlconfig_speech},
	{ "tracking", "navit", xmlconfig_tracking},
	{ "route", "navit", xmlconfig_route},
	{ "speed", "route", xmlconfig_speed},
	{ "vehicle", "navit", xmlconfig_vehicle},
	{ "log", "vehicle", xmlconfig_log_vehicle},
	{ "log", "navit", xmlconfig_log_navit},
	{ "window_items", "navit", xmlconfig_window_items},
	{ "plugins", "config", NULL, NEW(plugins_new), NULL, INIT(plugins_init), NULL},
	{ "plugin", "plugins", xmlconfig_plugin},
	{},
};

static void
start_element(GMarkupParseContext *context,
		const gchar         *element_name,
		const gchar        **attribute_names,
		const gchar        **attribute_values,
		gpointer             user_data,
		GError             **error)
{
	struct xmlstate *new=NULL, **parent = user_data;
	struct element_func *e=elements,*func=NULL;
	int found=0;
	const char *parent_name=NULL;
	char *s,*sep="",*possible_parents;
	dbg(2,"name='%s' parent='%s'\n", element_name, *parent ? (*parent)->element:NULL);
	possible_parents=g_strdup("");
	if (*parent)
		parent_name=(*parent)->element;
	while (e->name) {
		if (!g_ascii_strcasecmp(element_name, e->name)) {
			found=1;
			s=g_strconcat(possible_parents,sep,e->parent,NULL);
			g_free(possible_parents);
			possible_parents=s;
			sep=",";
			if ((parent_name && e->parent && !g_ascii_strcasecmp(parent_name, e->parent)) ||
			    (!parent_name && !e->parent))
				func=e;
		}
		e++;
	}
	if (! found) {
		g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_UNKNOWN_ELEMENT,
				"Unknown element '%s'", element_name);
		g_free(possible_parents);
		return;
	}
	if (! func) {
		g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
				"Element '%s' within unexpected context '%s'. Expected '%s'%s",
				element_name, parent_name, possible_parents, ! strcmp(possible_parents, "config") ? "\nPlease add <config> </config> tags at the beginning/end of your navit.xml": "");
		g_free(possible_parents);
		return;
	}
	g_free(possible_parents);

	new=g_new(struct xmlstate, 1);
	new->attribute_names=attribute_names;
	new->attribute_values=attribute_values;
	new->parent=*parent;
	new->element_attr.u.data=NULL;
	new->element=element_name;
	new->error=error;
	new->func=func;
	*parent=new;
	if (!find_boolean(new, "enabled", 1, 0))
		return;
	if (new->parent && !new->parent->element_attr.u.data)
		return;
	if (func->func) {
		if (!func->func(new)) {
			return;
		}
	} else {
		struct attr **attrs;

		attrs=convert_to_attrs(new);
		new->element_attr.type=attr_none;
		new->element_attr.u.data = func->new(&new->parent->element_attr, attrs);
		if (! new->element_attr.u.data)
			return;
		new->element_attr.type=attr_from_name(element_name);
		if (new->parent->func->add_attr) 
			new->parent->func->add_attr(new->parent->element_attr.u.data, &new->element_attr);
	}
	return;
}


/* Called for close tags </foo> */
static void
end_element (GMarkupParseContext *context,
		const gchar         *element_name,
		gpointer             user_data,
		GError             **error)
{
	struct xmlstate *curr, **state = user_data;

	dbg(2,"name='%s'\n", element_name);
	curr=*state;
	if (curr->func->init)
		curr->func->init(curr->element_attr.u.data);
	*state=curr->parent;
	g_free(curr);
}

static gboolean parse_file(struct xmldocument *document, GError **error);

static void
xinclude(GMarkupParseContext *context, const gchar **attribute_names, const gchar **attribute_values, struct xmldocument *doc_old, GError **error)
{
	struct xmldocument doc_new;
	struct file_wordexp *we;
	int i,count;
	const char *href=NULL;
	char **we_files;

	if (doc_old->level >= 16) {
		g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT, "xi:include recursion too deep");
		return;
	}
	memset(&doc_new, 0, sizeof(doc_new));
	i=0;
	while (attribute_names[i]) {
		if(!g_ascii_strcasecmp("href", attribute_names[i])) {
			if (!href) 
				href=attribute_values[i];
			else {
				g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT, "xi:include has more than one href");
				return;
			}
		} else if(!g_ascii_strcasecmp("xpointer", attribute_names[i])) {
			if (!doc_new.xpointer) 
				doc_new.xpointer=attribute_values[i];
			else {
				g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT, "xi:include has more than one xpointer");
				return;
			}
		} else {
			g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT, "xi:include has invalid attributes");
			return;
		}
		i++;
	}
	if (!doc_new.xpointer && !href) {
		g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT, "xi:include has neither href nor xpointer");
		return;
	}
	doc_new.level=doc_old->level+1;
	doc_new.user_data=doc_old->user_data;
	if (! href) {
		dbg(1,"no href, using '%s'\n", doc_old->href);
		doc_new.href=doc_old->href;
		parse_file(&doc_new, error);
	} else {
		dbg(1,"expanding '%s'\n", href);
		we=file_wordexp_new(href);
		we_files=file_wordexp_get_array(we);
		count=file_wordexp_get_count(we);
		dbg(1,"%d results\n", count);
		for (i = 0 ; i < count ; i++) {
			dbg(1,"result[%d]='%s'\n", i, we_files[i]);
			doc_new.href=we_files[i];
			parse_file(&doc_new, error);
		}
		file_wordexp_destroy(we);	
		
	}
	
}
static int
strncmp_len(const char *s1, int s1len, const char *s2)
{
	int ret;
#if 0
	char c[s1len+1];
	strncpy(c, s1, s1len);
	c[s1len]='\0';
	dbg(0,"'%s' vs '%s'\n", c, s2);
#endif

	ret=strncmp(s1, s2, s1len);
	if (ret)
		return ret;
	return strlen(s2)-s1len;
}

static int 
xpointer_value(const char *test, int len, struct xistate *elem, const char **out, int out_len)
{
	int i,ret=0;
	if (len <= 0 || out_len <= 0) {
		return 0;
	}
	if (!(strncmp_len(test,len,"name(.)"))) {
		out[0]=elem->element;
		return 1;
	}
	if (test[0] == '@') {
		i=0;
		while (elem->attribute_names[i] && out_len > 0) {
			if (!strncmp_len(test+1,len-1,elem->attribute_names[i])) {
				out[ret++]=elem->attribute_values[i];
				out_len--;
			}
			i++;
		}
		return ret;
	}
	return 0;
}

static int
xpointer_test(const char *test, int len, struct xistate *elem)
{
	int eq,i,count,vlen,cond_req=1,cond=0;
	char c;
	const char *tmp[16];

	if (!len)
		return 0;
	c=test[len-1];
	if (c != '\'' && c != '"')
		return 0;
	eq=strcspn(test, "=");
	if (eq >= len || test[eq+1] != c)
		return 0;
	vlen=eq;
	if (eq > 0 && test[eq-1] == '!') {
		cond_req=0;
		vlen--;
	}
	count=xpointer_value(test,vlen,elem,tmp,16);
	for (i = 0 ; i < count ; i++) {
		if (!strncmp_len(test+eq+2,len-eq-3, tmp[i]))
			cond=1;
	}
	if (cond == cond_req)
		return 1;
	return 0;
}

static int
xpointer_element_match(const char *xpointer, int len, struct xistate *elem)
{
	int len_test;
	len_test=strcspn(xpointer, "[");
	if (len_test > len)
		len_test=len;
	if (strncmp_len(xpointer, len_test, elem->element) && (len_test != 1 || xpointer[0] != '*'))
		return 0;
	if (len_test == len)
		return 1;
	if (xpointer[len-1] != ']')
		return 0;
	return xpointer_test(xpointer+len_test+1, len-len_test-2, elem);
}

static int
xpointer_xpointer_match(const char *xpointer, int len, struct xistate *first)
{
	const char *c;
	int s;
	dbg(2,"%s\n", xpointer);
	if (xpointer[0] != '/')
		return 0;
	c=xpointer+1;
	len--;
	do {
		s=strcspn(c, "/");
		if (s > len)
			s=len;
		if (! xpointer_element_match(c, s, first))
			return 0;
		first=first->child;
		c+=s+1;
		len-=s+1;
	} while (s < len && first);
	if (s < len)
		return 0;
	return 1;
}

static int
xpointer_match(const char *xpointer, struct xistate *first)
{
	char *prefix="xpointer(";
	int len=strlen(xpointer);
	if (! xpointer)
		return 1;
	if (strncmp(xpointer,prefix,strlen(prefix)))
		return 0;
	if (xpointer[len-1] != ')')
		return 0;
	return xpointer_xpointer_match(xpointer+strlen(prefix), len-strlen(prefix)-1, first);

}

static void
xi_start_element(GMarkupParseContext *context,
		const gchar         *element_name,
		const gchar        **attribute_names,
		const gchar        **attribute_values,
		gpointer             user_data,
		GError             **error)
{
	struct xmldocument *doc=user_data;
	struct xistate *xistate;
	int i,count=0;
	while (attribute_names[count++]);
	xistate=g_new0(struct xistate, 1);
	xistate->element=element_name;
	xistate->attribute_names=g_new(char *, count);
	xistate->attribute_values=g_new(char *, count);
	for (i = 0 ; i < count ; i++) {
		xistate->attribute_names[i]=g_strdup(attribute_names[i]);
		xistate->attribute_values[i]=g_strdup(attribute_values[i]);
	}
	xistate->parent=doc->last;
	
	if (doc->last) {
		doc->last->child=xistate;
	} else
		doc->first=xistate;
	doc->last=xistate;
	if (doc->active > 0 || xpointer_match(doc->xpointer, doc->first)) {
		if(!g_ascii_strcasecmp("xi:include", element_name)) {
			xinclude(context, attribute_names, attribute_values, doc, error);
			return;
		}
		start_element(context, element_name, attribute_names, attribute_values, doc->user_data, error);
		doc->active++;
	}
		
}

static void
xi_end_element (GMarkupParseContext *context,
		const gchar         *element_name,
		gpointer             user_data,
		GError             **error)
{
	struct xmldocument *doc=user_data;
	struct xistate *xistate=doc->last;
	int i=0;
	doc->last=doc->last->parent;
	if (! doc->last)
		doc->first=NULL;
	else
		doc->last->child=NULL;
	if (doc->active > 0) {
		if(!g_ascii_strcasecmp("xi:include", element_name)) {
			return;
		}
		end_element(context, element_name, doc->user_data, error);
		doc->active--;
	}
	while (xistate->attribute_names[i]) {
		g_free(xistate->attribute_names[i]);
		g_free(xistate->attribute_values[i]);
		i++;
	}
	g_free(xistate->attribute_names);
	g_free(xistate->attribute_values);
	g_free(xistate);
}

/* Called for character data */
/* text is not nul-terminated */
static void
xi_text (GMarkupParseContext *context,
		const gchar            *text,
		gsize                   text_len,
		gpointer                user_data,
		GError               **error)
{
}



static const GMarkupParser parser = {
	xi_start_element,
	xi_end_element,
	xi_text,
	NULL,
	NULL
};

static gboolean
parse_file(struct xmldocument *document, GError **error)
{
	GMarkupParseContext *context;
	gchar *contents, *message;
	gsize len;
	gint line, chr;
	gboolean result;

	dbg(1,"enter filename='%s'\n", document->href);
	context = g_markup_parse_context_new (&parser, 0, document, NULL);

	if (!g_file_get_contents (document->href, &contents, &len, error)) {
		g_markup_parse_context_free (context);
		return FALSE;
	}
	document->active=document->xpointer ? 0:1;
	document->first=NULL;
	document->last=NULL;
	result = g_markup_parse_context_parse (context, contents, len, error);
	if (!result && error && *error) {
		g_markup_parse_context_get_position(context, &line, &chr);
		message=g_strdup_printf("%s at line %d, char %d\n", (*error)->message, line, chr);
		g_free((*error)->message);
		(*error)->message=message;
	}
	g_markup_parse_context_free (context);
	g_free (contents);
	dbg(1,"return %d\n", result);

	return result;
}

gboolean config_load(char *filename, GError **error)
{
	struct xmldocument document;
	struct xmlstate *curr=NULL;
	gboolean result;

	dbg(1,"enter filename='%s'\n", filename);
	memset(&document, 0, sizeof(document));
	document.href=filename;
	document.user_data=&curr;
	result=parse_file(&document, error);
	if (result && curr) {
		g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_PARSE, "element '%s' not closed", curr->element);
		result=FALSE;
	}
	dbg(1,"return %d\n", result);
	return result;
}

