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
#include "projection.h"
#include "map.h"
#include "mapset.h"
#include "coord.h"
#include "item.h"
#include "search.h"

struct search_list_level {
	struct mapset *ms;
	struct item *parent;
	struct attr attr;
	int partial;
	int selected;
	struct mapset_search *search;
	GHashTable *hash;
	GList *list,*curr,*last;
};

struct search_list {
	struct mapset *ms;
	int level;
	struct search_list_level levels[4];
	struct search_list_result result;
};

static guint
search_item_hash_hash(gconstpointer key)
{
	const struct item *itm=key;
	gconstpointer hashkey=(gconstpointer)(itm->id_hi^itm->id_lo);
	return g_direct_hash(hashkey);
}

static gboolean
search_item_hash_equal(gconstpointer a, gconstpointer b)
{
	const struct item *itm_a=a;
	const struct item *itm_b=b;
	if (item_is_equal_id(*itm_a, *itm_b))
		return TRUE;
	return FALSE;
}

struct search_list *
search_list_new(struct mapset *ms)
{
	struct search_list *ret;

	ret=g_new0(struct search_list, 1);
	ret->ms=ms;
	
	return ret;
}

static void search_list_search_free(struct search_list *sl, int level);

static int
search_list_level(enum attr_type attr_type)
{
	switch(attr_type) {
	case attr_country_all:
	case attr_country_id:
	case attr_country_iso2:
	case attr_country_iso3:
	case attr_country_car:
	case attr_country_name:
		return 0;
	case attr_town_postal:
		return 1;
	case attr_town_name:
		return 1;
	case attr_street_name:
		return 2;
	default:
		return -1;
	}
}

void
search_list_search(struct search_list *this_, struct attr *search_attr, int partial)
{
	struct search_list_level *le;
	int level=search_list_level(search_attr->type);
	dbg(0,"level=%d\n", level);
	if (level != -1) {
		this_->result.id=0;
		this_->level=level;
		le=&this_->levels[level];
		le->attr=*search_attr;
		if (search_attr->type != attr_country_id)
			le->attr.u.str=g_strdup(search_attr->u.str);
		search_list_search_free(this_, level);
		le->partial=partial;
		if (level > 0) {
			le=&this_->levels[level-1];
			le->curr=le->list;
		}
		dbg(1,"le=%p partial=%d\n", le, partial);
	}
}

int
search_list_select(struct search_list *this_, enum attr_type attr_type, int id, int mode)
{
	int level=search_list_level(attr_type);
	int num=0;
	struct search_list_level *le;
	struct search_list_common *slc;
	GList *curr;
	le=&this_->levels[level];
	curr=le->list;
	if (mode > 0)
		le->selected=mode;
	dbg(0,"enter %d %d\n", id, mode);
	while (curr) {
		num++;
		if (! id || num == id) {
			slc=curr->data;
			slc->selected=mode;
			if (id) {
				dbg(0,"found\n");
				return 1;
			}
		}
		curr=g_list_next(curr);
	}
	dbg(0,"not found\n");
	return 0;
}

static struct search_list_country *
search_list_country_new(struct item *item)
{
	struct search_list_country *ret=g_new0(struct search_list_country, 1);
	struct attr attr;

	ret->item=*item;
	if (item_attr_get(item, attr_country_car, &attr))
		ret->car=g_strdup(attr.u.str);
	if (item_attr_get(item, attr_country_iso2, &attr)) {
		ret->iso2=g_strdup(attr.u.str);
		ret->flag=g_strdup_printf("country_%s", ret->iso2);
	}
	if (item_attr_get(item, attr_country_iso3, &attr))
		ret->iso3=g_strdup(attr.u.str);
	if (item_attr_get(item, attr_country_name, &attr))
		ret->name=g_strdup(attr.u.str);
	return ret;
}

static void
search_list_country_destroy(struct search_list_country *this_)
{
	g_free(this_->car);
	g_free(this_->iso2);
	g_free(this_->iso3);
	g_free(this_->flag);
	g_free(this_->name);
	g_free(this_);
}

static struct search_list_town *
search_list_town_new(struct item *item)
{
	struct search_list_town *ret=g_new0(struct search_list_town, 1);
	struct attr attr;
	struct coord c;
	
	ret->itemt=*item;
	if (item_attr_get(item, attr_town_streets_item, &attr)) {
		dbg(1,"town_assoc 0x%x 0x%x\n", attr.u.item->id_hi, attr.u.item->id_lo);
		ret->item=*attr.u.item;
	}
	else
		ret->item=*item;
	if (item_attr_get(item, attr_town_name, &attr))
		ret->name=map_convert_string(item->map,attr.u.str);
	if (item_attr_get(item, attr_town_postal, &attr))
		ret->postal=map_convert_string(item->map,attr.u.str);
	if (item_attr_get(item, attr_district_name, &attr))
		ret->district=map_convert_string(item->map,attr.u.str);
	if (item_coord_get(item, &c, 1)) {
		ret->c=g_new(struct pcoord, 1);
		ret->c->x=c.x;
		ret->c->y=c.y;
		ret->c->pro = map_projection(item->map);
	}
	return ret;
}

static void
search_list_town_destroy(struct search_list_town *this_)
{
	map_convert_free(this_->name);
	map_convert_free(this_->postal);
	if (this_->c)
		g_free(this_->c);
	g_free(this_);
}

static struct search_list_street *
search_list_street_new(struct item *item)
{
	struct search_list_street *ret=g_new0(struct search_list_street, 1);
	struct attr attr;
	struct coord c;
	
	ret->item=*item;
	if (item_attr_get(item, attr_street_name, &attr))
		ret->name=map_convert_string(item->map, attr.u.str);
	if (item_coord_get(item, &c, 1)) {
		ret->c=g_new(struct pcoord, 1);
		ret->c->x=c.x;
		ret->c->y=c.y;
		ret->c->pro = map_projection(item->map);
	}
	return ret;
}

static void
search_list_street_destroy(struct search_list_street *this_)
{
	map_convert_free(this_->name);
	if (this_->c)
		g_free(this_->c);
	g_free(this_);
}


static void
search_list_result_destroy(int level, void *p)
{
	switch (level) {
	case 0:
		search_list_country_destroy(p);
		break;
	case 1:
		search_list_town_destroy(p);
		break;
	case 2:
		search_list_street_destroy(p);
		break;
	}
}

static void
search_list_search_free(struct search_list *sl, int level)
{
	struct search_list_level *le=&sl->levels[level];
	GList *next,*curr;
	if (le->search) {
		mapset_search_destroy(le->search);
		le->search=NULL;
	}
#if 0 /* FIXME */
	if (le->hash) {
		g_hash_table_destroy(le->hash);
		le->hash=NULL;
	}
#endif
	curr=le->list;
	while (curr) {
		search_list_result_destroy(level, curr->data);
		next=g_list_next(curr);
		curr=next;
	}
	g_list_free(le->list);
	le->list=NULL;
	le->curr=NULL;
	le->last=NULL;

}

static int
search_add_result(struct search_list_level *le, void *p)
{
	if (! g_hash_table_lookup(le->hash, p)) {
		g_hash_table_insert(le->hash, p, (void *)1);	
		le->list=g_list_append(le->list, p);
		return 1;
	}
	return 0;
}

struct search_list_result *
search_list_get_result(struct search_list *this_)
{
	struct search_list_level *le,*leu;
	struct item *item;
	int level=this_->level;

	dbg(1,"enter\n");
	le=&this_->levels[level];
	dbg(1,"le=%p\n", le);
	for (;;) {
		dbg(1,"le->search=%p\n", le->search);
		if (! le->search) {
			dbg(1,"partial=%d level=%d\n", le->partial, level);
			if (! level) 
				le->parent=NULL;
			else {
				leu=&this_->levels[level-1];
				dbg(1,"leu->curr=%p\n", leu->curr);
				for (;;) {
					struct search_list_common *slc;
					if (! leu->curr)
						return NULL;
					le->parent=leu->curr->data;
					leu->last=leu->curr;
					leu->curr=g_list_next(leu->curr);
					slc=(struct search_list_common *)(le->parent);
					if (!slc)
						break;
					if (slc->selected == leu->selected)
						break;
				}
			}
			if (le->parent)
				dbg(1,"mapset_search_new with item(%d,%d)\n", le->parent->id_hi, le->parent->id_lo);
			dbg(1,"attr=%s\n", attr_to_name(le->attr.type));	
			le->search=mapset_search_new(this_->ms, le->parent, &le->attr, le->partial);
			le->hash=g_hash_table_new(search_item_hash_hash, search_item_hash_equal);
		}
		dbg(1,"le->search=%p\n", le->search);
		item=mapset_search_get_item(le->search);
		dbg(1,"item=%p\n", item);
		if (item) {
			void *p=NULL;
			dbg(1,"id_hi=%d id_lo=%d\n", item->id_hi, item->id_lo);
			this_->result.country=NULL;
			this_->result.town=NULL;
			this_->result.street=NULL;
			this_->result.c=NULL;
			switch (level) {
			case 0:
				p=search_list_country_new(item);
				this_->result.country=p;
				break;
			case 1:
				p=search_list_town_new(item);
				this_->result.country=this_->levels[0].last->data;
				this_->result.town=p;
				this_->result.c=this_->result.town->c;
				break;
			case 2:
				p=search_list_street_new(item);
				this_->result.country=this_->levels[0].last->data;
				this_->result.town=this_->levels[1].last->data;
				this_->result.street=p;
				this_->result.c=this_->result.street->c;
				break;
			}
			if (p) {
				if (search_add_result(le, p)) {
					this_->result.id++;
					return &this_->result;
				} else 
					search_list_result_destroy(level, p);
			}
		} else {
			mapset_search_destroy(le->search);
			le->search=NULL;
			g_hash_table_destroy(le->hash);
			if (! level)
				break;
		}
	}
	return NULL;
}

void
search_list_destroy(struct search_list *this_)
{
	g_free(this_);
}

void
search_init(void)
{
}
