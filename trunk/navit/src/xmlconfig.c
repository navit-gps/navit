#include <glib.h>
#include <glib/gprintf.h>
#include <string.h>
#include "debug.h"
#include "coord.h"
#include "layout.h"
#include "mapset.h"
#include "map.h"
#include "navigation.h"
#include "navit.h"
#include "plugin.h"
#include "projection.h"
#include "route.h"
#include "speech.h"
#include "track.h"
#include "vehicle.h"
#include "xmlconfig.h"


struct xmlstate {
	const gchar **attribute_names;
	const gchar **attribute_values;
	struct xmlstate *parent;
	void *element_object;
	const gchar *element;
	GError **error;
	struct element_func *func;
} *xmlstate_root;


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
	int r,g,b;

	value=find_attribute(state, "color", required);
	if (! value)
		return 0;

	sscanf(value,"#%02x%02x%02x", &r, &g, &b);
	color->r = (r << 8) | r;
	color->g = (g << 8) | g;
	color->b = (b << 8) | b;
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
	pos=index(value, '-');
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
	return g_ascii_strtoull(val,NULL,0);
}

static int
xmlconfig_plugins(struct xmlstate *state)
{
	state->element_object = plugins_new();
	if (! state->element_object)
		return 0;
	return 1;
}

static int
xmlconfig_plugin(struct xmlstate *state)
{
	const char *path;
	int active,lazy;

	state->element_object=state->parent->element_object;
	path=find_attribute(state, "path", 1);
	if (! path)
		return 0;
	active=find_boolean(state, "active", 1, 0);
	lazy=find_boolean(state, "lazy", 1, 0);
	plugins_add_path(state->parent->element_object, path, active, lazy);
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
	state->element_object = speech_new(type, data);
	if (! state->element_object)
		return 0;
	navit_set_speech(state->parent->element_object, state->element_object);
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
xmlconfig_navit(struct xmlstate *state)
{
	const char *value,*gui,*graphics;
	int zoom=0;
	struct coord c;
	enum projection pro=projection_mg;

	value=find_attribute(state, "zoom", 0);
	if (value) 
		zoom=convert_number(value);
	if (! zoom)
		zoom=256;
	value=find_attribute(state, "center", 0);
	if (! value || ! coord_parse(value, pro, &c)) {
		c.x=1300000;
		c.y=7000000;
	}
	gui=find_attribute(state, "gui", 0);
	if (! gui)
		gui="gtk";
	graphics=find_attribute(state, "graphics", 0);
	if (! graphics)
		graphics="gtk_drawing_area";
	state->element_object = navit_new(gui, graphics, &c, pro, zoom);
	if (! state->element_object)
		return 0;
	return 1;
}

static int
xmlconfig_vehicle(struct xmlstate *state)
{
	const char *s=find_attribute(state, "source", 1);
	const char *value;
	struct color color;
	int update=1, follow=0, active;

	if (! s)
		return 0;
	if (! find_color(state, 1, &color))
		return 0;
	state->element_object = vehicle_new(s);
	if (! state->element_object)
		return 0;
	if ((value=find_attribute(state, "update", 0)))
		update=convert_number(value);
	if ((value=find_attribute(state, "follow", 0)))
		follow=convert_number(value);
	active=find_boolean(state, "active", 1, 0);

	navit_vehicle_add(state->parent->element_object, state->element_object, &color, update, follow, active);
	return 1;
}

static int
xmlconfig_tracking(struct xmlstate *state)
{
	state->element_object = tracking_new(NULL);
	navit_tracking_add(state->parent->element_object, state->element_object);
	return 1;
}

static int
xmlconfig_route(struct xmlstate *state)
{
	state->element_object = route_new(NULL);
	navit_route_add(state->parent->element_object, state->element_object);
	return 1;
}

static int
xmlconfig_speed(struct xmlstate *state)
{
	const char *type;
	const char *value;
	int v;
	enum item_type itype;
	char *saveptr, *tok, *type_str, *str;

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
		route_set_speed(state->parent->element_object, itype, v);
		str=NULL;
	}
	g_free(type_str);

	return 1;
}


static int
xmlconfig_navigation(struct xmlstate *state)
{
	state->element_object = navigation_new(NULL);
	navit_navigation_add(state->parent->element_object, state->element_object);
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
	char *saveptr, *tok, *type_str, *str;

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
		navigation_set_announce(state->parent->element_object, itype, level);
		str=NULL;
	}
	g_free(type_str);
	return 1;
}

static int
xmlconfig_mapset(struct xmlstate *state)
{
	state->element_object = mapset_new();
	if (! state->element_object)
		return 0;
	navit_add_mapset(state->parent->element_object, state->element_object);

	return 1;
}

static int
xmlconfig_map(struct xmlstate *state)
{
	const char *type=find_attribute(state, "type", 1);
	const char *data=find_attribute(state, "data", 1);
	if (! type || ! data)
		return 0;
	state->element_object = map_new(type, data);
	if (! state->element_object)
		return 0;
	if (!find_boolean(state, "active", 1, 0))
		map_set_active(state->element_object, 0);
	mapset_add(state->parent->element_object, state->element_object);

	return 1;
}

static int
xmlconfig_layout(struct xmlstate *state)
{
	const char *name=find_attribute(state, "name", 1);

	if (! name)
		return 0;
	state->element_object = layout_new(name);
	if (! state->element_object)
		return 0;
	navit_add_layout(state->parent->element_object, state->element_object);
	return 1;
}

static int
xmlconfig_layer(struct xmlstate *state)
{
	const char *name=find_attribute(state, "name", 1);
	if (! name)
		return 0;
	state->element_object = layer_new(name, convert_number(find_attribute(state, "details", 0)));
	if (! state->element_object)
		return 0;
	layout_add_layer(state->parent->element_object, state->element_object);
	return 1;
}

static int
xmlconfig_item(struct xmlstate *state)
{
	const char *type=find_attribute(state, "type", 1);
	int min, max;
	enum item_type itype;
	char *saveptr, *tok, *type_str, *str;

	if (! type)
		return 0;
	if (! find_order(state, 1, &min, &max))
		return 0;
	state->element_object=itemtype_new(min, max);
	if (! state->element_object)
		return 0;
	type_str=g_strdup(type);
	str=type_str;
	layer_add_itemtype(state->parent->element_object, state->element_object);
	while ((tok=strtok_r(str, ",", &saveptr))) {
		itype=item_from_name(tok);
		itemtype_add_type(state->element_object, itype);
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
	state->element_object=polygon_new(&color);
	if (! state->element_object)
		return 0;
	itemtype_add_element(state->parent->element_object, state->element_object);

	return 1;
}

static int
xmlconfig_polyline(struct xmlstate *state)
{
	struct color color;
	const char *width;
	int w=0;

	if (! find_color(state, 1, &color))
		return 0;
	width=find_attribute(state, "width", 0);
	if (width) 
		w=convert_number(width);
	state->element_object=polyline_new(&color, w);
	if (! state->element_object)
		return 0;
	itemtype_add_element(state->parent->element_object, state->element_object);

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
	state->element_object=circle_new(&color, r, w, ls);
	if (! state->element_object)
		return 0;
	itemtype_add_element(state->parent->element_object, state->element_object);

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
	state->element_object=label_new(ls);
	if (! state->element_object)
		return 0;
	itemtype_add_element(state->parent->element_object, state->element_object);

	return 1;
}

static int
xmlconfig_icon(struct xmlstate *state)
{
	const char *src=find_attribute(state, "src", 1);

	if (! src)
		return 0;
	state->element_object=icon_new(src);
	if (! state->element_object)
		return 0;
	itemtype_add_element(state->parent->element_object, state->element_object);

	return 1;
}

static int
xmlconfig_image(struct xmlstate *state)
{
	state->element_object=image_new();
	if (! state->element_object)
		return 0;
	itemtype_add_element(state->parent->element_object, state->element_object);

	return 1;
}

struct element_func {
	char *name;
	char *parent;
	int (*func)(struct xmlstate *state);
} elements[] = {
	{ "debug", NULL, xmlconfig_debug},
	{ "navit", NULL, xmlconfig_navit},
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
	{ "announce", "navigation", xmlconfig_announce},
	{ "speech", "navit", xmlconfig_speech},
	{ "tracking", "navit", xmlconfig_tracking},
	{ "route", "navit", xmlconfig_route},
	{ "speed", "route", xmlconfig_speed},
	{ "vehicle", "navit", xmlconfig_vehicle},
	{ "plugins", NULL, xmlconfig_plugins},
	{ "plugin", "plugins", xmlconfig_plugin},
	{},
};

static void
start_element (GMarkupParseContext *context,
		const gchar         *element_name,
		const gchar        **attribute_names,
		const gchar        **attribute_values,
		gpointer             user_data,
		GError             **error)
{
	struct xmlstate *new=NULL, **parent = user_data;
	struct element_func *e=elements,*func=NULL;
	const char *parent_name=NULL;
	while (e->name) {
		if (!g_ascii_strcasecmp(element_name, e->name)) {
			func=e;
		}
		e++;
	}
	if (! func) {
		g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_UNKNOWN_ELEMENT,
				"Unknown element '%s'", element_name);
		return;
	}
	if (*parent)
		parent_name=(*parent)->element;
	if ((parent_name && func->parent && g_ascii_strcasecmp(parent_name, func->parent)) || 
	    (!parent_name && func->parent) || (parent_name && !func->parent)) {
		g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
				"Element '%s' within unexpected context '%s'. Expected '%s'",
				element_name, parent_name, func->parent);
		return;
	}

	new=g_new(struct xmlstate, 1);
	new->attribute_names=attribute_names;
	new->attribute_values=attribute_values;
	new->parent=*parent;
	new->element_object=NULL;
	new->element=element_name;
	new->error=error;
	new->func=func;
	*parent=new;
	if (!find_boolean(new, "enabled", 1, 0))
		return;
	if (new->parent && !new->parent->element_object)
		return;
	if (!func->func(new)) {
		return;
	}
	return;
#if 0
	struct elem_data *data = user_data;
	void *elem=NULL;
	void *parent_object;
	char *parent_token;
	parent_object=data->elem_stack ? data->elem_stack->data : NULL;
	parent_token=data->token_stack ? data->token_stack->data : NULL;

	/* g_printf("start_element: %s AN: %s AV: %s\n",element_name,*attribute_names,*attribute_values); */


	printf("Unknown element '%s'\n", element_name);
#if 0
	data->elem_stack = g_list_prepend(data->elem_stack, elem);
	data->token_stack = g_list_prepend(data->token_stack, (gpointer)element_name);
#endif
#endif
}


/* Called for close tags </foo> */
static void
end_element (GMarkupParseContext *context,
		const gchar         *element_name,
		gpointer             user_data,
		GError             **error)
{
	struct xmlstate *curr, **state = user_data;

	curr=*state;
	if(!g_ascii_strcasecmp("plugins", element_name) && curr->element_object) 
		plugins_init(curr->element_object);
	if(!g_ascii_strcasecmp("navit", element_name) && curr->element_object) 
		navit_init(curr->element_object);
	*state=curr->parent;
	g_free(curr);
}


/* Called for character data */
/* text is not nul-terminated */
static void
text (GMarkupParseContext *context,
		const gchar            *text,
		gsize                   text_len,
		gpointer                user_data,
		GError               **error)
{
	struct xmlstate **state = user_data;

	(void) state;
}



static const GMarkupParser parser = {
	start_element,
	end_element,
	text,
	NULL,
	NULL
};


gboolean config_load(char *filename, GError **error)
{
	GMarkupParseContext *context;
	char *contents;
	gsize len;
	gboolean result;
	gint line;
	gint chr;
	gchar *message;

	struct xmlstate *curr=NULL;
	
	context = g_markup_parse_context_new (&parser, 0, &curr, NULL);

	if (!g_file_get_contents (filename, &contents, &len, error))
		return FALSE;

	result = g_markup_parse_context_parse (context, contents, len, error);
	if (result && curr) {
		g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_PARSE, "element '%s' not closed", curr->element);
		result=FALSE;	
	}
	if (!result && error && *error) {
		g_markup_parse_context_get_position(context, &line, &chr);
		message=g_strdup_printf("%s at line %d, char %d\n", (*error)->message, line, chr);
		g_free((*error)->message);
		(*error)->message=message;
	}
	g_markup_parse_context_free (context);
	g_free (contents);

	return result;
}

