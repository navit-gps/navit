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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "debug.h"
#include "plugin.h"
#include "projection.h"
#include "item.h"
#include "map.h"
#include "maptype.h"
#include "attr.h"
#include "coord.h"
#include "transform.h"
#include "file.h"
#include "zipfile.h"
#include "linguistics.h"
#include "endianess.h"

struct filter_entry {
	enum item_type first,last;
};

struct filter {
	GList *old;
	GList *new;
};

struct map_priv {
	struct map *parent;
	GList *filters;
};

struct map_rect_priv {
        struct map_selection *sel;
        struct map_priv *m;
	struct map_rect *parent;
	struct item item,*parent_item;
};

struct map_search_priv {
	struct map_rect_priv *mr;
};

static enum item_type
filter_type(struct map_priv *m, enum item_type type)
{
	GList *filters=m->filters;
	while (filters) {
		struct filter *filter=filters->data;
		int pos=0,count=0;
		GList *old,*new;
		old=filter->old;
		while (old) {
			struct filter_entry *entry=old->data;
			if (type >= entry->first && type <= entry->last)
				break;
			pos+=entry->last-entry->first+1;
			old=g_list_next(old);
		}
		if (old) {
			new=filter->new;
			while (new) {
				struct filter_entry *entry=new->data;
				count+=entry->last-entry->first+1;
				new=g_list_next(new);
			}
			pos%=count;
			new=filter->new;
			while (new) {
				struct filter_entry *entry=new->data;
				if (pos <= entry->last-entry->first) 
					return entry->first+pos;
				pos-=entry->last-entry->first+1;
				new=g_list_next(new);
			}
		}
		filters=g_list_next(filters);
	}
	return type_none;
}

static void
free_filter(struct filter *filter)
{
	g_list_foreach(filter->old, (GFunc)g_free, NULL);
	g_list_free(filter->old);
	filter->old=NULL;
	g_list_foreach(filter->new, (GFunc)g_free, NULL);
	g_list_free(filter->new);
	filter->new=NULL;
}

static void
free_filters(struct map_priv *m)
{
	g_list_foreach(m->filters, (GFunc)free_filter, NULL);
	g_list_free(m->filters);
	m->filters=NULL;
}

static GList *
parse_filter(char *filter)
{
	GList *ret=NULL;
	for (;;) {
		char *range,*next=strchr(filter,',');
		struct filter_entry *entry=g_new(struct filter_entry, 1);
		if (next)
			*next++='\0';
		range=strchr(filter,'-');
		if (range)
			*range++='\0';
		entry->first=item_from_name(filter);
		if (range)
			entry->last=item_from_name(range);
		else
			entry->last=entry->first;
		ret=g_list_append(ret, entry);	
		if (!next)
			break;
		filter=next;
	}
	return ret;
}

static void
parse_filters(struct map_priv *m, char *filter)
{
	char *filter_copy=g_strdup(filter);
	char *str=filter_copy;

	free_filters(m);	
	for (;;) {
		char *eq,*next=strchr(str,';');
		struct filter *filter=g_new0(struct filter, 1); 
		if (next)
			*next++='\0';
		eq=strchr(str,'=');
		if (eq) 
			*eq++='\0';
		filter->old=parse_filter(str);
		if (eq)
			filter->new=parse_filter(eq);
		m->filters=g_list_append(m->filters,filter);
		if (!next)
			break;
		str=next;
	}
	g_free(filter_copy);
}

static void
map_filter_coord_rewind(void *priv_data)
{
	struct map_rect_priv *mr=priv_data;
	item_coord_rewind(mr->parent_item);
}

static int
map_filter_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *mr=priv_data;
	return item_coord_get(mr->parent_item, c, count);
}

static void
map_filter_attr_rewind(void *priv_data)
{
	struct map_rect_priv *mr=priv_data;
	item_attr_rewind(mr->parent_item);
}

static int
map_filter_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct map_rect_priv *mr=priv_data;
	return item_attr_get(mr->parent_item, attr_type, attr);
}

static struct item_methods methods_filter = {
        map_filter_coord_rewind,
        map_filter_coord_get,
        map_filter_attr_rewind,
        map_filter_attr_get,
};

static struct map_rect_priv *
map_filter_rect_new(struct map_priv *map, struct map_selection *sel)
{
	struct map_rect_priv *mr=NULL;
	struct map_rect *parent;
	parent=map_rect_new(map->parent, sel);
	if (parent) {
		mr=g_new0(struct map_rect_priv, 1);
		mr->m=map;
		mr->sel=sel;
		mr->parent=parent;
	}
	mr->item.meth=&methods_filter;
	mr->item.priv_data=mr;
	return mr;
}

static void
map_filter_rect_destroy(struct map_rect_priv *mr)
{
	map_rect_destroy(mr->parent);
        g_free(mr);
}

static struct item *
map_filter_rect_get_item(struct map_rect_priv *mr)
{
	mr->parent_item=map_rect_get_item(mr->parent);
	if (!mr->parent_item)
		return NULL;
	mr->item.type=filter_type(mr->m,mr->parent_item->type);
	mr->item.id_lo=mr->parent_item->id_lo;
	mr->item.id_hi=mr->parent_item->id_hi;
	return &mr->item;
}

static struct item *
map_filter_rect_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	dbg(0,"enter\n");
	mr->parent_item=map_rect_get_item_byid(mr->parent, id_hi, id_lo);
	if (!mr->parent_item)
		return NULL;
	mr->item.type=filter_type(mr->m,mr->parent_item->type);
	mr->item.id_lo=mr->parent_item->id_lo;
	mr->item.id_hi=mr->parent_item->id_hi;
	return &mr->item;
}

static struct map_search_priv *
map_filter_search_new(struct map_priv *map, struct item *item, struct attr *search, int partial)
{
	dbg(0,"enter\n");
	return NULL;
}

static struct item *
map_filter_search_get_item(struct map_search_priv *map_search)
{
	dbg(0,"enter\n");
	return NULL;
}

static void
map_filter_search_destroy(struct map_search_priv *ms)
{
	dbg(0,"enter\n");
}

static void
map_filter_destroy(struct map_priv *m)
{
	map_destroy(m->parent);
	g_free(m);
}

static struct map_methods map_methods_filter = {
	projection_mg,
	"utf-8",
	map_filter_destroy,
	map_filter_rect_new,
	map_filter_rect_destroy,
	map_filter_rect_get_item,
	map_filter_rect_get_item_byid,
	map_filter_search_new,
	map_filter_search_destroy,
	map_filter_search_get_item
};



static struct map_priv *
map_filter_new(struct map_methods *meth, struct attr **attrs)
{
	struct map_priv *m=NULL;
	struct attr **parent_attrs,type,*subtype=attr_search(attrs, NULL, attr_subtype),*filter=attr_search(attrs, NULL, attr_filter);
	struct map *map;
	int i,j;
	if (! subtype || !filter)
		return NULL;
	i=0;
	while (attrs[i])
		i++;
	parent_attrs=g_new(struct attr *,i+1);
	i=0;
	j=0;
	while (attrs[i]) {
		if (attrs[i]->type != attr_filter && attrs[i]->type != attr_type) {
			if (attrs[i]->type == attr_subtype) {
				type=*attrs[i];
				type.type = attr_type;
				parent_attrs[j]=&type;
			} else 
				parent_attrs[j]=attrs[i];
			j++;
		}
		i++;
	}
	parent_attrs[j]=NULL;
	*meth=map_methods_filter;
	map=map_new(NULL, parent_attrs);
	if (map) {
		m=g_new0(struct map_priv, 1);
		m->parent=map;
	}
	parse_filters(m,filter->u.str);
	g_free(parent_attrs);
	return m;
}

void
plugin_init(void)
{
	dbg(1,"filter: plugin_init\n");
	plugin_register_map_type("filter", map_filter_new);
}

