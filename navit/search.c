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
#include <stdlib.h>
#include <math.h>
#include "debug.h"
#include "projection.h"
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "coord.h"
#include "transform.h"
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

struct interpolation {
	int side, mode, rev;
	char *first, *last, *curr;
};

struct search_list {
	struct mapset *ms;
	struct item *item;
	int level;
	struct search_list_level levels[4];
	struct search_list_result result;
	struct search_list_result last_result;
	int last_result_valid;
	char *postal;
	struct interpolation inter;
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
	case attr_postal:
		return -1;
	default:
		dbg(0,"unknown search '%s'\n",attr_to_name(attr_type));
		return -1;
	}
}

static void
interpolation_clear(struct interpolation *inter)
{
	inter->mode=inter->side=0;
	g_free(inter->first);
	g_free(inter->last);
	g_free(inter->curr);
	inter->first=inter->last=inter->curr=NULL;
}

void
search_list_search(struct search_list *this_, struct attr *search_attr, int partial)
{
	struct search_list_level *le;
	int level=search_list_level(search_attr->type);
	this_->item=NULL;
	interpolation_clear(&this_->inter);
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
	} else if (search_attr->type == attr_postal) {
		g_free(this_->postal);
		this_->postal=g_strdup(search_attr->u.str);
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
	else if (item_attr_get(item, attr_town_postal, &attr))
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

static char *
search_interpolate(struct interpolation *inter)
{
	dbg(1,"interpolate %s-%s %s\n",inter->first,inter->last,inter->curr);
	if (!inter->first || !inter->last)
		return NULL;
	if (!inter->curr) 
		inter->curr=g_strdup(inter->first);
	else {
		if (strcmp(inter->curr, inter->last)) {
			int next=atoi(inter->curr)+(inter->mode?2:1);
			g_free(inter->curr);
			if (next == atoi(inter->last))
				inter->curr=g_strdup(inter->last);
			else
				inter->curr=g_strdup_printf("%d",next);
		} else {
			g_free(inter->curr);
			inter->curr=NULL;
		}
	}
	dbg(1,"interpolate result %s\n",inter->curr);
	return inter->curr;
}

static void
search_interpolation_split(char *str, struct interpolation *inter)
{
	char *pos=strchr(str,'-');
	char *first,*last;
	int len;
	if (!pos) {
		inter->first=g_strdup(str);
		inter->last=g_strdup(str);
		inter->rev=0;
		return;
	}
	len=pos-str;
	first=g_malloc(len+1);
	strncpy(first, str, len);
	first[len]='\0';
	last=g_strdup(pos+1);
	dbg(1,"%s = %s - %s\n",str, first, last);
	if (atoi(first) > atoi(last)) {
		inter->first=last;
		inter->last=first;
		inter->rev=1;
	} else {
		inter->first=first;
		inter->last=last;
		inter->rev=0;
	}
}

static int
search_setup_interpolation(struct item *item, enum attr_type i0, enum attr_type i1, enum attr_type i2, struct interpolation *inter)
{
	struct attr attr;
	g_free(inter->first);
	g_free(inter->last);
	g_free(inter->curr);
	inter->first=inter->last=inter->curr=NULL;
	dbg(1,"setup %s\n",attr_to_name(i0));
	if (item_attr_get(item, i0, &attr)) {
		search_interpolation_split(attr.u.str, inter);
		inter->mode=0;
	} else if (item_attr_get(item, i1, &attr)) {
		search_interpolation_split(attr.u.str, inter);
		inter->mode=1;
	} else if (item_attr_get(item, i2, &attr)) {
		search_interpolation_split(attr.u.str, inter);
		inter->mode=2;
	} else
		return 0;
	return 1;
}

static int
search_match(char *str, char *search, int partial)
{
	if (!partial)
		return (!strcasecmp(str, search));
	else
		return (!strncasecmp(str, search, strlen(search)));
}

static struct pcoord *
search_house_number_coordinate(struct item *item, struct interpolation *inter)
{
	struct pcoord *ret=g_new(struct pcoord, 1);
	ret->pro = map_projection(item->map);
	if (item_is_point(*item)) {
		struct coord c;
		if (item_coord_get(item, &c, 1)) {
			ret->x=c.x;
			ret->y=c.y;
		} else {
			g_free(ret);
			ret=NULL;
		}
	} else {
		int count,max=1024;
		struct coord c[max];
		count=item_coord_get(item, c, max);
		int hn_pos,hn_length=atoi(inter->last)-atoi(inter->first);
		if (inter->rev)
			hn_pos=atoi(inter->last)-atoi(inter->curr);
		else
			hn_pos=atoi(inter->curr)-atoi(inter->first);
		if (count && hn_length) {
			int i,distances[count-1],distance_sum=0,hn_distance;
			dbg(1,"count=%d hn_length=%d hn_pos=%d (%s of %s-%s)\n",count,hn_length,hn_pos,inter->curr,inter->first,inter->last);
			if (count == max) 
				dbg(0,"coordinate overflow\n");
			for (i = 0 ; i < count-1 ; i++) {
				distances[i]=navit_sqrt(transform_distance_sq(&c[i],&c[i+1]));
				distance_sum+=distances[i];
				dbg(1,"distance[%d]=%d\n",i,distances[i]);
			}
			dbg(1,"sum=%d\n",distance_sum);
			hn_distance=distance_sum*hn_pos/hn_length;
			dbg(1,"hn_distance=%d\n",hn_distance);
			i=0;
			while (hn_distance > distances[i] && i < count-1) 
				hn_distance-=distances[i++];
			dbg(1,"remaining distance=%d from %d\n",hn_distance,distances[i]);
			ret->x=(c[i+1].x-c[i].x)*hn_distance/distances[i]+c[i].x;
			ret->y=(c[i+1].y-c[i].y)*hn_distance/distances[i]+c[i].y;
		}
	}
	return ret;
}

static struct search_list_house_number *
search_list_house_number_new(struct item *item, struct interpolation *inter, char *inter_match, int inter_partial)
{
	struct search_list_house_number *ret=g_new0(struct search_list_house_number, 1);
	struct attr attr;
	char *hn;
	
	ret->common.item=ret->common.unique=*item;
	if (item_attr_get(item, attr_house_number, &attr))
		ret->house_number=map_convert_string(item->map, attr.u.str);
	else {
#if 0
		if (item_attr_get(item, attr_street_name, &attr))
			dbg(0,"%s\n",attr.u.str);
#endif
		for (;;) {
			ret->interpolation=1;
			switch(inter->side) {
			case 0:
				inter->side=-1;
				search_setup_interpolation(item, attr_house_number_left, attr_house_number_left_odd, attr_house_number_left_even, inter);
			case -1:
				if ((hn=search_interpolate(inter)))
					break;
				inter->side=1;
				search_setup_interpolation(item, attr_house_number_right, attr_house_number_right_odd, attr_house_number_right_even, inter);
			case 1:
				if ((hn=search_interpolate(inter)))
					break;
			default:
				g_free(ret);
				return NULL;
			}
			if (search_match(hn, inter_match, inter_partial)) {
#if 0
				dbg(0,"match %s %s-%s\n",hn, inter->first, inter->last);
#endif
				ret->house_number=map_convert_string(item->map, hn);
				break;
			}
		}
	}
	search_list_common_new(item, &ret->common);
	ret->common.c=search_house_number_coordinate(item, ret->interpolation?inter:NULL);
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

char *
search_postal_merge(char *mask, char *new)
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

char *
search_postal_merge_replace(char *mask, char *new)
{
	char *ret=search_postal_merge(mask, new);
	if (!ret)
		return mask;
	g_free(mask);
	return ret;
}


static int
postal_match(char *postal, char *mask)
{
	for (;;) {
		if ((*postal != *mask) && (*mask != '.'))
			return 0;
		if (!*postal) {
			if (!*mask)
				return 1;
			else
				return 0;
		}
		postal++;
		mask++;
	}
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
	merged=search_postal_merge(slo->postal_mask, slc->postal);
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
		if (!this_->item)
			this_->item=mapset_search_get_item(le->search);
		if (this_->item) {
			void *p=NULL;
			dbg(1,"id_hi=%d id_lo=%d\n", this_->item->id_hi, this_->item->id_lo);
			if (this_->postal) {
				struct attr postal;
				if (item_attr_get(this_->item, attr_postal_mask, &postal)) {
					if (!postal_match(this_->postal, postal.u.str))
						continue;
				} else if (item_attr_get(this_->item, attr_postal, &postal)) {
					if (strcmp(this_->postal, postal.u.str))
						continue;
				}
			}
			this_->result.country=NULL;
			this_->result.town=NULL;
			this_->result.street=NULL;
			this_->result.c=NULL;
			switch (level) {
			case 0:
				p=search_list_country_new(this_->item);
				this_->result.country=p;
				this_->result.country->common.parent=NULL;
				this_->item=NULL;
				break;
			case 1:
				p=search_list_town_new(this_->item);
				this_->result.town=p;
				this_->result.town->common.parent=this_->levels[0].last->data;
				this_->result.country=this_->result.town->common.parent;
				this_->result.c=this_->result.town->common.c;
				this_->item=NULL;
				break;
			case 2:
				p=search_list_street_new(this_->item);
				this_->result.street=p;
				this_->result.street->common.parent=this_->levels[1].last->data;
				this_->result.town=this_->result.street->common.parent;
				this_->result.country=this_->result.town->common.parent;
				this_->result.c=this_->result.street->common.c;
				this_->item=NULL;
				break;
			case 3:
				p=search_list_house_number_new(this_->item, &this_->inter, le->attr->u.str, le->partial);
				if (!p) {
					interpolation_clear(&this_->inter);
					this_->item=NULL;
					continue;
				}
				this_->result.house_number=p;
				if (!this_->result.house_number->interpolation)
					this_->item=NULL;
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
	g_free(this_->postal);
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

