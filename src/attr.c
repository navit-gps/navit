#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include "debug.h"
#include "item.h"
#include "coord.h"
#include "transform.h"
#include "color.h"
#include "attr.h"

struct attr_name {
	enum attr_type attr;
	char *name;
};


static struct attr_name attr_names[]={
#define ATTR2(x,y) ATTR(y)
#define ATTR(x) { attr_##x, #x },
#include "attr_def.h"
#undef ATTR2
#undef ATTR
};

enum attr_type
attr_from_name(const char *name)
{
	int i;

	for (i=0 ; i < sizeof(attr_names)/sizeof(struct attr_name) ; i++) {
		if (! strcmp(attr_names[i].name, name))
			return attr_names[i].attr;
	}
	return attr_none;
}

char *
attr_to_name(enum attr_type attr)
{
	int i;

	for (i=0 ; i < sizeof(attr_names)/sizeof(struct attr_name) ; i++) {
		if (attr_names[i].attr == attr)
			return attr_names[i].name;
	}
	return NULL; 
}

struct attr *
attr_new_from_text(const char *name, const char *value)
{
	enum attr_type attr;
	struct attr *ret;
	struct coord_geo *g;
	struct coord c;

	ret=g_new0(struct attr, 1);
	dbg(1,"enter name='%s' value='%s'\n", name, value);
	attr=attr_from_name(name);
	ret->type=attr;
	switch (attr) {
	case attr_item_type:
		ret->u.item_type=item_from_name(value);
		break;
	default:
		if (attr >= attr_type_string_begin && attr <= attr_type_string_end) {
			ret->u.str=(char *)value;
			break;
		}
		if (attr >= attr_type_int_begin && attr <= attr_type_int_end) {
			ret->u.num=atoi(value);
			break;
		}
		if (attr >= attr_type_color_begin && attr <= attr_type_color_end) {
			struct color *color=g_new0(struct color, 1);
			int r,g,b,a;
			ret->u.color=color;
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
				dbg(0,"color %s has unknown format\n",value);
			}
			break;
		}
		if (attr >= attr_type_coord_geo_start && attr <= attr_type_coord_geo_end) {
			g=g_new(struct coord_geo, 1);
			ret->u.coord_geo=g;
			coord_parse(value, projection_mg, &c);
			transform_to_geo(projection_mg, &c, g);
			break;
		}
		dbg(1,"default\n");
		g_free(ret);
		ret=NULL;
	}
	return ret;
}

struct attr *
attr_search(struct attr **attrs, struct attr *last, enum attr_type attr)
{
	dbg(1, "enter attrs=%p\n", attrs);
	while (*attrs) {
		dbg(1,"*attrs=%p\n", *attrs);
		if ((*attrs)->type == attr) {
			return *attrs;
		}
		attrs++;
	}
	return NULL;
}

int
attr_data_size(struct attr *attr)
{
	if (attr->type >= attr_type_string_begin && attr->type <= attr_type_string_end) {
		return strlen(attr->u.str)+1;
	}
	if (attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) {
		return sizeof(attr->u.num);
	}
	return 0;
}

void *
attr_data_get(struct attr *attr)
{
	if (attr->type >= attr_type_string_begin && attr->type <= attr_type_string_end) {
		return attr->u.str;
	}
	if (attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) {
		return &attr->u.num;
	}
	return NULL;
}

void
attr_data_set(struct attr *attr, void *data)
{
	if (attr->type >= attr_type_string_begin && attr->type <= attr_type_string_end) {
		attr->u.str=data;
	}
	if (attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) {
		attr->u.num=*((int *)data);
	}
}

void
attr_free(struct attr *attr)
{
	if (attr->type == attr_position_coord_geo)
		g_free(attr->u.coord_geo);
	if (attr->type >= attr_type_color_begin && attr->type <= attr_type_color_end) 
		g_free(attr->u.color);
	g_free(attr);
}
