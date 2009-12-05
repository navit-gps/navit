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
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "coord.h"
#include "search.h"

struct search_list_level {
	struct mapset *ms;
	struct search_list_common *parent;
	struct attr *attr;
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
	struct search_list_result last_result;
	int last_result_valid;
};

static guint
search_item_hash_hash(gconstpointer key)
{
	const struct item *itm=key;
	gconstpointer hashkey=(gconstpointer)GINT_TO_POINTER(itm->id_hi^itm->id_lo);
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
	case attr_district_name:
	case attr_town_or_district_name:
		return 1;
	case attr_street_name:
		return 2;
	case attr_house_number:
		return 3;
	default:
		dbg(0,"unknown search '%s'\n",attr_to_name(attr_type));
		return -1;
	}
}

void
search_list_search(struct search_list *this_, struct attr *search_attr, int partial)
{
	struct search_list_level *le;
	int level=search_list_level(search_attr->type);
	dbg(1,"level=%d\n", level);
	if (level != -1) {
		this_->result.id=0;
		this_->level=level;
		le=&this_->levels[level];
		search_list_search_free(this_, level);
		le->attr=attr_dup(search_attr);
		le->partial=partial;
		if (level > 0) {
			le=&this_->levels[level-1];
			le->curr=le->list;
		}
		dbg(1,"le=%p partial=%d\n", le, partial);
	}
}

struct search_list_common *
search_list_select(struct search_list *this_, enum attr_type attr_type, int id, int mode)
{
	int level=search_list_level(attr_type);
	int num=0;
	struct search_list_level *le;
	struct search_list_common *slc;
	GList *curr;
	le=&this_->levels[level];
	curr=le->list;
	if (mode > 0 || !id)
		le->selected=mode;
	dbg(1,"enter level=%d %d %d %p\n", level, id, mode, curr);
	while (curr) {
		num++;
		if (! id || num == id) {
			slc=curr->data;
			slc->selected=mode;
			if (id) {
				le->last=curr;
				dbg(0,"found\n");
				return slc;
			}
		}
		curr=g_list_next(curr);
	}
	dbg(1,"not found\n");
	return NULL;
}

static void
search_list_common_new(struct item *item, struct search_list_common *common)
{
	struct attr attr;
	if (item_attr_get(item, attr_town_name, &attr))
		common->town_name=map_convert_string(item->map, attr.u.str);
	else
		common->town_name=NULL;
	if (item_attr_get(item, attr_district_name, &attr))
		common->district_name=map_convert_string(item->map, attr.u.str);
	else
		common->district_name=NULL;
	if (item_attr_get(item, attr_postal, &attr))
		common->postal=map_convert_string(item->map, attr.u.str);
	else
		common->postal=NULL;
	if (item_attr_get(item, attr_postal_mask, &attr)) 
		common->postal_mask=map_convert_string(item->map, attr.u.str);
	else 
		common->postal_mask=NULL;
}

static void
search_list_common_destroy(struct search_list_common *common)
{
	map_convert_free(common->town_name);
	map_convert_free(common->district_name);
	map_convert_free(common->postal);
	map_convert_free(common->postal_mask);
}

static struct search_list_country *
search_list_country_new(struct item *item)
{
	struct search_list_country *ret=g_new0(struct search_list_country, 1);
	struct attr attr;

	ret->common.item=ret->common.unique=*item;
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
	ret->common.item=ret->common.unique=*item;
	if (item_attr_get(item, attr_town_streets_item, &attr)) {
		dbg(1,"town_assoc 0x%x 0x%x\n", attr.u.item->id_hi, attr.u.item->id_lo);
		ret->common.unique=*attr.u.item;
	}
	search_list_common_new(item, &ret->common);
	if (item_attr_get(item, attr_county_name, &attr))
		ret->county=map_convert_string(item->map,attr.u.str);
	else
		ret->county=NULL;
	if (item_coord_get(item, &c, 1)) {
		ret->common.c=g_new(struct pcoord, 1);
		ret->common.c->x=c.x;
		ret->common.c->y=c.y;
		ret->common.c->pro = map_projection(item->map);
	}
	return ret;
}

static void
search_list_town_destroy(struct search_list_town *this_)
{
	map_convert_free(this_->county);
	search_list_common_destroy(&this_->common);
	if (this_->common.c)
		g_free(this_->common.c);
	g_free(this_);
}


static struct search_list_street *
search_list_street_new(struct item *item)
{
	struct search_list_street *ret=g_new0(struct search_list_street, 1);
	struct attr attr;
	struct coord c;
	
	ret->common.item=ret->common.unique=*item;
	if (item_attr_get(item, attr_street_name, &attr))
		ret->name=map_convert_string(item->map, attr.u.str);
	else
		ret->name=NULL;
	search_list_common_new(item, &ret->common);
	if (item_coord_get(item, &c, 1)) {
		ret->common.c=g_new(struct pcoord, 1);
		ret->common.c->x=c.x;
		ret->common.c->y=c.y;
		ret->common.c->pro = map_projection(item->map);
	}
	return ret;
}


static void
search_list_street_destroy(struct search_list_street *this_)
{
	map_convert_free(this_->name);
	search_list_common_destroy(&this_->common);
	if (this_->common.c)
		g_free(this_->common.c);
	g_free(this_);
}

static struct search_list_house_number *
search_list_house_number_new(struct item *item)
{
	struct search_list_house_number *ret=g_new0(struct search_list_house_number, 1);
	struct attr attr;
	struct coord c;
	
	ret->common.item=ret->common.unique=*item;
	if (item_attr_get(item, attr_house_number, &attr))
		ret->house_number=map_convert_string(item->map, attr.u.str);
	search_list_common_new(item, &ret->common);
	if (item_coord_get(item, &c, 1)) {
		ret->common.c=g_new(struct pcoord, 1);
		ret->common.c->x=c.x;
		ret->common.c->y=c.y;
		ret->common.c->pro = map_projection(item->map);
	}
	return ret;
}

static void
search_list_house_number_destroy(struct search_list_house_number *this_)
{
	map_convert_free(this_->house_number);
	search_list_common_destroy(&this_->common);
	if (this_->common.c)
		g_free(this_->common.c);
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
	case 3:
		search_list_house_number_destroy(p);
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
	attr_free(le->attr);
	g_list_free(le->list);
	le->list=NULL;
	le->curr=NULL;
	le->last=NULL;

}

static char *
postal_merge(char *mask, char *new)
{
	dbg(1,"enter %s %s\n", mask, new);
	int i;
	char *ret=NULL;
	if (!new)
		return NULL;
	if (!mask)
		return g_strdup(new);
	i=0;
	while (mask[i] && new[i]) {
		if (mask[i] != '.' && mask[i] != new[i])
			break;
		i++;
		
	}
	if (mask[i]) {
		ret=g_strdup(mask);
		while (mask[i]) 
			ret[i++]='.';
	}
	dbg(1,"merged %s with %s as %s\n", mask, new, ret);	
	return ret;
}

static int
search_add_result(struct search_list_level *le, struct search_list_common *slc)
{
	struct search_list_common *slo;
	char *merged;
	slo=g_hash_table_lookup(le->hash, &slc->unique);
	if (!slo) {
		g_hash_table_insert(le->hash, &slc->unique, slc);
		if (slc->postal && !slc->postal_mask) {
			slc->postal_mask=g_strdup(slc->postal);
		}
		le->list=g_list_append(le->list, slc);
		return 1;
	}
	merged=postal_merge(slo->postal_mask, slc->postal);
	if (merged) {
		g_free(slo->postal_mask);
		slo->postal_mask=merged;
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
				dbg(1,"mapset_search_new with item(%d,%d)\n", le->parent->item.id_hi, le->parent->item.id_lo);
			dbg(1,"attr=%s\n", attr_to_name(le->attr->type));
			le->search=mapset_search_new(this_->ms, &le->parent->item, le->attr, le->partial);
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
				this_->result.country->common.parent=NULL;
				break;
			case 1:
				p=search_list_town_new(item);
				this_->result.town=p;
				this_->result.town->common.parent=this_->levels[0].last->data;
				this_->result.country=this_->result.town->common.parent;
				this_->result.c=this_->result.town->common.c;
				break;
			case 2:
				p=search_list_street_new(item);
				this_->result.street=p;
				this_->result.street->common.parent=this_->levels[1].last->data;
				this_->result.town=this_->result.street->common.parent;
				this_->result.country=this_->result.town->common.parent;
				this_->result.c=this_->result.street->common.c;
				break;
			case 3:
				p=search_list_house_number_new(item);
				this_->result.house_number=p;
				this_->result.house_number->common.parent=this_->levels[2].last->data;
				this_->result.street=this_->result.house_number->common.parent;
				this_->result.town=this_->result.street->common.parent;
				this_->result.country=this_->result.town->common.parent;
				this_->result.c=this_->result.house_number->common.c;
				
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

#if 0
static char *
search_fix_spaces(char *str)
{
	int i;
	int len=strlen(str);
	char c,*s,*d,*ret=g_strdup(str);

	for (i = 0 ; i < len ; i++) {
		if (ret[i] == ',' || ret[i] == ',' || ret[i] == '/')
			ret[i]=' ';
	}
	s=ret;
	d=ret;
	len=0;
	do {
		c=*s++;
		if (c != ' ' || len != 0) {
			*d++=c;
			len++;
		}
		while (c == ' ' && *s == ' ')
			s++;
		if (c == ' ' && *s == '\0') {
			d--;
			len--;
		}
	} while (c);
	return ret;
}

static GList *
search_split_phrases(char *str)
{
	char *tmp,*s,*d;
	s=str;
	GList *ret=NULL;
	do {
		tmp=g_strdup(s);
		d=tmp+strlen(s)-1;
		ret=g_list_append(ret, g_strdup(s));
		while (d >= tmp) {
			if (*d == ' ') {
				*d = '\0';
				ret=g_list_append(ret, g_strdup(tmp));
			}
			d--;
		}
		g_free(tmp);
		do {
			s++;
			if (*s == ' ') {
				s++;
				break;
			}
		} while (*s != '\0');
	} while (*s != '\0');
	return ret;
}

static void
search_address_housenumber(struct search_list *sl, GList *phrases, GList *exclude1, GList *exclude2, GList *exclude3)
{
	struct search_list_result *slr;
	GList *tmp=phrases;
	int count=0;
	struct attr attr;
	attr.type=attr_street_name;
	while (slr=search_list_get_result(sl)) {
		dbg(0,"%p\n",slr);
		dbg(0,"%p %p\n",slr->country,slr->town);
		dbg(0,"%s %s %s %s %s\n",slr->country->car,slr->town->common.postal,slr->town->name,slr->town->district,slr->street->name);
		count++;
	}
	if (!count)
		return;
	dbg(0,"count %d\n",count);
	while (tmp) {
		if (tmp != exclude1 && tmp != exclude2 && tmp != exclude3) {
			attr.type=attr_house_number;
			attr.u.str=tmp->data;
			search_list_search(sl, &attr, 0);
			while (slr=search_list_get_result(sl)) {
				dbg(0,"result %s %s(%s) %s %s\n",slr->house_number->common.postal,slr->house_number->common.town_name, slr->house_number->common.district_name,slr->street->name,slr->house_number->house_number);
			}
			
		}
		tmp=g_list_next(tmp);
	}
}
static void
search_address_street(struct search_list *sl, GList *phrases, GList *exclude1, GList *exclude2)
{
	struct search_list_result *slr;
	GList *tmp=phrases;
	int count=0;
	struct attr attr;
	attr.type=attr_street_name;
	while (slr=search_list_get_result(sl)) {
#if 0
		dbg(0,"%s %s %s %s",slr->country->car,slr->town->name,slr->town->district,slr->street->name);
#endif
		dbg(0,"%s %s %s %s\n",slr->country->car,slr->town->common.postal,slr->town->name,slr->town->district);
		count++;
	}
	if (!count)
		return;
	dbg(0,"count %d\n",count);
	while (tmp) {
		if (tmp != exclude1 && tmp != exclude2) {
			attr.u.str=tmp->data;
			search_list_search(sl, &attr, 0);
			search_address_housenumber(sl, phrases, exclude1, exclude2, tmp);
		}
		tmp=g_list_next(tmp);
	}
}

static void
search_address_town(struct search_list *sl, GList *phrases, GList *exclude)
{
	GList *tmp=phrases;
	int count=0;
	struct attr attr;
	attr.type=attr_town_or_district_name;
	while (search_list_get_result(sl))
		count++;
	if (!count)
		return;
	dbg(0,"count %d\n",count);
	while (tmp) {
		if (tmp != exclude) {
			attr.u.str=tmp->data;
			search_list_search(sl, &attr, 0);
			search_address_street(sl, phrases, exclude, tmp);
		}
		tmp=g_list_next(tmp);
	}
}

void
search_by_address(struct mapset *ms, char *addr)
{
	char *str=search_fix_spaces(addr);
	GList *tmp,*phrases=search_split_phrases(str);
	dbg(0,"enter %s\n",addr);
	struct search_list *sl;
	struct search_list_result *slr;
	struct attr attr;
	attr.type=attr_country_all;
	tmp=phrases;
	sl=search_list_new(ms);
	while (tmp) {
		attr.u.str=tmp->data;
		search_list_search(sl, &attr, 0);
		search_address_town(sl, phrases, tmp);
		tmp=g_list_next(tmp);
	}
	search_list_search(sl, country_default(), 0);
	search_address_town(sl, phrases, NULL);
	
	g_free(str);
}
#endif

