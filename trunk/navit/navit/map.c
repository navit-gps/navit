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
#include "debug.h"
#include "coord.h"
#include "projection.h"
#include "map.h"
#include "maptype.h"
#include "transform.h"
#include "item.h"
#include "plugin.h"
#include "callback.h"
#include "country.h"


struct map {
	struct map_methods meth;
	struct map_priv *priv;
	char *type;
	char *filename;
	struct callback_list *attr_cbl;
	int active;
};

struct map_rect {
	struct map *m;
	struct map_rect_priv *priv;
};

struct map *
map_new(const char *type, struct attr **attrs)
{
	struct map *m;
	struct map_priv *(*maptype_new)(struct map_methods *meth, struct attr **attrs);
	struct attr *data=attr_search(attrs, NULL, attr_data);


	maptype_new=plugin_get_map_type(type);
	if (! maptype_new)
		return NULL;

	m=g_new0(struct map, 1);
	m->active=1;
	m->type=g_strdup(type);
	if (data) 
		m->filename=g_strdup(data->u.str);
	m->priv=maptype_new(&m->meth, attrs);
	if (! m->priv) {
		g_free(m);
		m=NULL;
	}
	if (m)
		m->attr_cbl=callback_list_new();
	return m;
}

char *
map_get_filename(struct map *this_)
{
	return this_->filename;
}

char *
map_get_type(struct map *this_)
{
	return this_->type;
}

int
map_get_active(struct map *this_)
{
	return this_->active;
}

void
map_set_active(struct map *this_, int active)
{
	this_->active=active;
}

int
map_get_attr(struct map *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	switch (type) {
	case attr_active:
		attr->u.num=this_->active;
		break;
	default:
		return 0;
	}
	return 1;
}

int
map_set_attr(struct map *this_, struct attr *attr)
{
	int attr_updated=0;

	switch (attr->type) {
	case attr_active:
		if (this_->active != !!attr->u.num) {
			this_->active=!!attr->u.num;
			attr_updated=1;
		}
		break;
	default:
		return 0;
	}
	if (attr_updated)
		callback_list_call_attr_2(this_->attr_cbl, attr->type, this_, attr);
}

void
map_add_callback(struct map *this_, struct callback *cb)
{
	callback_list_add(this_->attr_cbl, cb);
}

void
map_remove_callback(struct map *this_, struct callback *cb)
{
	callback_list_remove(this_->attr_cbl, cb);
}



int
map_requires_conversion(struct map *this_)
{
	return (this_->meth.charset != NULL && strcmp(this_->meth.charset, "utf-8"));
}

char *
map_convert_string(struct map *this_, char *str)
{
	return g_convert(str, -1,"utf-8",this_->meth.charset,NULL,NULL,NULL);
}

void
map_convert_free(char *str)
{
	g_free(str);
}

enum projection
map_projection(struct map *this_)
{
	return this_->meth.pro;
}

void
map_set_projection(struct map *this_, enum projection pro)
{
	this_->meth.pro=pro;
}

void
map_destroy(struct map *m)
{
	m->meth.map_destroy(m->priv);
	g_free(m);
}

struct map_rect *
map_rect_new(struct map *m, struct map_selection *sel)
{
	struct map_rect *mr;

#if 0
	printf("map_rect_new 0x%x,0x%x-0x%x,0x%x\n", r->lu.x, r->lu.y, r->rl.x, r->rl.y);
#endif
	mr=g_new0(struct map_rect, 1);
	mr->m=m;
	mr->priv=m->meth.map_rect_new(m->priv, sel);
	if (! mr->priv) {
		g_free(mr);
		mr=NULL;
	}

	return mr;
}

struct item *
map_rect_get_item(struct map_rect *mr)
{
	struct item *ret;
	g_assert(mr != NULL);
	g_assert(mr->m != NULL);
	g_assert(mr->m->meth.map_rect_get_item != NULL);
	ret=mr->m->meth.map_rect_get_item(mr->priv);
	if (ret)
		ret->map=mr->m;
	return ret;
}

struct item *
map_rect_get_item_byid(struct map_rect *mr, int id_hi, int id_lo)
{
	struct item *ret=NULL;
	g_assert(mr != NULL);
	g_assert(mr->m != NULL);
	if (mr->m->meth.map_rect_get_item_byid)
		ret=mr->m->meth.map_rect_get_item_byid(mr->priv, id_hi, id_lo);
	if (ret)
		ret->map=mr->m;
	return ret;
}

void
map_rect_destroy(struct map_rect *mr)
{
	mr->m->meth.map_rect_destroy(mr->priv);
	g_free(mr);
}

struct map_search {
        struct map *m;
        struct attr search_attr;
        void *priv;
};

struct map_search *
map_search_new(struct map *m, struct item *item, struct attr *search_attr, int partial)
{
	struct map_search *this_;
	dbg(1,"enter(%p,%p,%p,%d)\n", m, item, search_attr, partial);
	dbg(1,"0x%x 0x%x 0x%x\n", attr_country_all, search_attr->type, attr_country_name);
	this_=g_new0(struct map_search,1);
	this_->m=m;
	this_->search_attr=*search_attr;
	if (search_attr->type >= attr_country_all && search_attr->type <= attr_country_name)
		this_->priv=country_search_new(&this_->search_attr, partial);
	else {
		if (m->meth.map_search_new) {
			if (m->meth.charset) 
				this_->search_attr.u.str=g_convert(this_->search_attr.u.str, -1,m->meth.charset,"utf-8",NULL,NULL,NULL);
			this_->priv=m->meth.map_search_new(m->priv, item, &this_->search_attr, partial);
			if (! this_->priv) {
				g_free(this_);
				this_=NULL;
			}
		} else {
			g_free(this_);
			this_=NULL;
		}
	}
	return this_;
}

struct item *
map_search_get_item(struct map_search *this_)
{
	struct item *ret;

	if (! this_)
		return NULL;
	if (this_->search_attr.type >= attr_country_all && this_->search_attr.type <= attr_country_name) 
		return country_search_get_item(this_->priv);
	ret=this_->m->meth.map_search_get_item(this_->priv);
	if (ret)
		ret->map=this_->m;
	return ret;
}

void
map_search_destroy(struct map_search *this_)
{
	if (! this_)
		return;
	if (this_->search_attr.type >= attr_country_all && this_->search_attr.type <= attr_country_name)
		country_search_destroy(this_->priv);
	else {
		if (this_->m->meth.charset) 
				g_free(this_->search_attr.u.str);
		this_->m->meth.map_search_destroy(this_->priv);
	}
	g_free(this_);
}

struct map_selection *
map_selection_dup(struct map_selection *sel)
{
	struct map_selection *next,**last;
	struct map_selection *ret=NULL;
	last=&ret;	
	while (sel) {
		next = g_new(struct map_selection, 1);
		*next=*sel;
		*last=next;
		last=&next->next;
		sel = sel->next;
	}
	return ret;
}

void
map_selection_destroy(struct map_selection *sel)
{
	struct map_selection *next;
	while (sel) {
		next = sel->next;
		g_free(sel);
		sel = next;
	}
}

int
map_selection_contains_item_rect(struct map_selection *sel, struct item *item)
{
	struct coord c;
	struct coord_rect r;
	int count=0;
	while (item_coord_get(item, &c, 1)) {
		if (! count) {
			r.lu=c;
			r.rl=c;
		} else 
			coord_rect_extend(&r, &c);
		count++;
	}
	if (! count)
		return 0;
	return map_selection_contains_rect(sel, &r);
	
}


int
map_priv_is(struct map *map, struct map_priv *priv)
{
	return (map->priv == priv);
}
