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

typedef GError xmlerror;
#ifdef HAVE_GLIB
#define ATTR_DISTANCE 1
#else
#include "ezxml.h"
#define ATTR_DISTANCE 2
#define G_MARKUP_ERROR 0
#define G_MARKUP_ERROR_INVALID_CONTENT 0
#define G_MARKUP_ERROR_PARSE 0
#define G_MARKUP_ERROR_UNKNOWN_ELEMENT 0
typedef void * GMarkupParseContext;
#endif

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
	xmlerror **error;
	struct element_func *func;
	struct xmldocument *document;
};


struct attr_fixme {
	char *element;
	char **attr_fixme;
};

static struct attr ** convert_to_attrs(struct xmlstate *state, struct attr_fixme *fixme)
{
	const gchar **attribute_name=state->attribute_names;
	const gchar **attribute_value=state->attribute_values;
	const gchar *name;
	int count=0;
	struct attr **ret;
	static int fixme_count;

	while (*attribute_name) {
		count++;
		attribute_name++;
	}
	ret=g_new(struct attr *, count+1);
	attribute_name=state->attribute_names;
	count=0;
	while (*attribute_name) {
		name=*attribute_name;
		if (fixme) {
			char **attr_fixme=fixme->attr_fixme;
			while (attr_fixme[0]) {
				if (! strcmp(name, attr_fixme[0])) {
					name=attr_fixme[1];
					if (fixme_count++ < 10)
						dbg(0,"Please change attribute '%s' to '%s' in <%s />\n", attr_fixme[0], attr_fixme[1], fixme->element);
					break;
				}
				attr_fixme+=2;
			}
		}
		ret[count]=attr_new_from_text(name,*attribute_value);
		if (ret[count])
			count++;
		else if (strcmp(*attribute_name,"enabled"))
			dbg(0,"failed to create attribute '%s' with value '%s'\n", *attribute_name,*attribute_value);
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
xmlconfig_config(struct xmlstate *state)
{
	state->element_attr.u.data = (void *)1;
	return 1;
}

static int
xmlconfig_plugin(struct xmlstate *state)
{
	struct attr **attrs;
	attrs=convert_to_attrs(state,NULL);
	plugins_add_path(state->parent->element_attr.u.data, attrs);
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

	attrs=convert_to_attrs(state,NULL);
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
	{ "announce", "navigation", xmlconfig_announce},
	{ "speech", "navit", xmlconfig_speech},
	{ "tracking", "navit", xmlconfig_tracking},
	{ "route", "navit", xmlconfig_route},
	{ "speed", "route", xmlconfig_speed},
	{ "mapset", "navit", NULL, NEW(mapset_new), ADD(mapset_add_attr)},
	{ "map",  "mapset", NULL, NEW(map_new)},
	{ "debug", "config", NULL, NEW(debug_new)},
	{ "osd", "navit", NULL, NEW(osd_new)},
	{ "navigation", "navit", NULL, NEW(navigation_new)},
	{ "navit", "config", NULL, NEW(navit_new), ADD(navit_add_attr), INIT(navit_init), DESTROY(navit_destroy)},
	{ "graphics", "navit", NULL, NEW(graphics_new)},
	{ "gui", "navit", NULL, NEW(gui_new)},
	{ "layout", "navit", NULL, NEW(layout_new), ADD(layout_add_attr)},
	{ "layer", "layout", NULL, NEW(layer_new), ADD(layer_add_attr)},
	{ "itemgra", "layer", NULL, NEW(itemgra_new), ADD(itemgra_add_attr)},
	{ "circle", "itemgra", NULL, NEW(circle_new)},
	{ "icon", "itemgra", NULL, NEW(icon_new)},
	{ "image", "itemgra", NULL, NEW(image_new)},
	{ "text", "itemgra", NULL, NEW(text_new)},
	{ "polygon", "itemgra", NULL, NEW(polygon_new)},
	{ "polyline", "itemgra", NULL, NEW(polyline_new)},
	{ "arrows", "itemgra", NULL, NEW(arrows_new)},
	{ "vehicle", "navit", NULL, NEW(vehicle_new)},
	{ "log", "vehicle", NULL, NEW(log_new)},
	{ "log", "navit", NULL, NEW(log_new)},
	{ "window_items", "navit", xmlconfig_window_items},
	{ "plugins", "config", NULL, NEW(plugins_new), NULL, INIT(plugins_init)},
	{ "plugin", "plugins", xmlconfig_plugin},
	{},
};


static char *attr_fixme_itemgra[]={
	"type","item_types",
	NULL,NULL,
};

static char *attr_fixme_text[]={
	"label_size","text_size",
	NULL,NULL,
};

static char *attr_fixme_circle[]={
	"label_size","text_size",
	NULL,NULL,
};

static struct attr_fixme attr_fixmes[]={
	{"item",attr_fixme_itemgra},
	{"itemgra",attr_fixme_itemgra},
	{"text",attr_fixme_text},
	{"label",attr_fixme_text},
	{"circle",attr_fixme_circle},
	{NULL,NULL},
};


static char *element_fixmes[]={
	"item","itemgra",
	"label","text",
	NULL,NULL,
};

static void
start_element(GMarkupParseContext *context,
		const gchar         *element_name,
		const gchar        **attribute_names,
		const gchar        **attribute_values,
		gpointer             user_data,
		xmlerror             **error)
{
	struct xmlstate *new=NULL, **parent = user_data;
	struct element_func *e=elements,*func=NULL;
	struct attr_fixme *attr_fixme=attr_fixmes;
	char **element_fixme=element_fixmes;
	int found=0;
	static int fixme_count;
	const char *parent_name=NULL;
	char *s,*sep="",*possible_parents;
	dbg(2,"name='%s' parent='%s'\n", element_name, *parent ? (*parent)->element:NULL);

	while (attr_fixme[0].element) {
		if (!strcmp(element_name,attr_fixme[0].element))
			break;
		attr_fixme++;
	}
	if (!attr_fixme[0].element)
		attr_fixme=NULL;
	
	while (element_fixme[0]) {
		if (!strcmp(element_name,element_fixme[0])) {
			element_name=element_fixme[1];
			if (fixme_count++ < 10)
				dbg(0,"Please change <%s /> to <%s /> in config file\n", element_fixme[0], element_fixme[1]);
		}
		element_fixme+=2;
	}
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

		attrs=convert_to_attrs(new,attr_fixme);
		new->element_attr.type=attr_none;
		new->element_attr.u.data = func->new(&new->parent->element_attr, attrs);
		if (! new->element_attr.u.data)
			return;
		new->element_attr.type=attr_from_name(element_name);
		if (new->element_attr.type == attr_none) 
			dbg(0,"failed to create object of type '%s'\n", element_name);
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
		xmlerror             **error)
{
	struct xmlstate *curr, **state = user_data;

	dbg(2,"name='%s'\n", element_name);
	curr=*state;
	if (curr->func->init)
		curr->func->init(curr->element_attr.u.data);
	*state=curr->parent;
	g_free(curr);
}

static gboolean parse_file(struct xmldocument *document, xmlerror **error);

static void
xinclude(GMarkupParseContext *context, const gchar **attribute_names, const gchar **attribute_values, struct xmldocument *doc_old, xmlerror **error)
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
		xmlerror             **error)
{
	struct xmldocument *doc=user_data;
	struct xistate *xistate;
	int i,count=0;
	while (attribute_names[count++*ATTR_DISTANCE]);
	xistate=g_new0(struct xistate, 1);
	xistate->element=element_name;
	xistate->attribute_names=g_new(char *, count);
	xistate->attribute_values=g_new(char *, count);
	for (i = 0 ; i < count ; i++) {
		xistate->attribute_names[i]=g_strdup(attribute_names[i*ATTR_DISTANCE]);
		xistate->attribute_values[i]=g_strdup(attribute_values[i*ATTR_DISTANCE]);
	}
	xistate->parent=doc->last;
	
	if (doc->last) {
		doc->last->child=xistate;
	} else
		doc->first=xistate;
	doc->last=xistate;
	if (doc->active > 0 || xpointer_match(doc->xpointer, doc->first)) {
		if(!g_ascii_strcasecmp("xi:include", element_name)) {
			xinclude(context, xistate->attribute_names, xistate->attribute_values, doc, error);
			return;
		}
		start_element(context, element_name, xistate->attribute_names, xistate->attribute_values, doc->user_data, error);
		doc->active++;
	}
		
}

static void
xi_end_element (GMarkupParseContext *context,
		const gchar         *element_name,
		gpointer             user_data,
		xmlerror             **error)
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
		xmlerror               **error)
{
}


#ifdef HAVE_GLIB

static const GMarkupParser parser = {
	xi_start_element,
	xi_end_element,
	xi_text,
	NULL,
	NULL
};

static gboolean
parse_file(struct xmldocument *document, xmlerror **error)
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
#else
static void
parse_node(struct xmldocument *document, ezxml_t node)
{
	while (node) {
		xi_start_element(NULL,node->name, node->attr, node->attr+1, document, NULL);
		if (node->txt)
			xi_text(NULL,node->txt,strlen(node->txt),document,NULL);
		if (node->child)
			parse_node(document, node->child);
		xi_end_element (NULL,node->name,document,NULL);
		node=node->ordered;
	}
}

static gboolean
parse_file(struct xmldocument *document, xmlerror **error)
{
	ezxml_t root = ezxml_parse_file(document->href);

	if (!root)
		return FALSE;
	document->active=document->xpointer ? 0:1;
	document->first=NULL;
	document->last=NULL;

	parse_node(document, root);

	return TRUE;
}
#endif

gboolean config_load(char *filename, xmlerror **error)
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

