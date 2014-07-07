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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *
 * Boston, MA  02110-1301, USA.
 */

#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <math.h>
#include "debug.h"
#include "projection.h"
#include "item.h"
#include "xmlconfig.h"
#include "map.h"
#include "mapset.h"
#include "coord.h"
#include "transform.h"
#include "search.h"
#include "country.h"
#include "linguistics.h"
#include "geom.h"
#include "util.h"

#ifdef HAVE_API_ANDROID
#include "android.h"
#endif
#include "layout.h"

static struct search_list_result *search_list_result_dup(struct search_list_result *slr);
static void search_list_country_destroy(struct search_list_country *this_);
static void search_list_town_destroy(struct search_list_town *this_);
static void search_list_street_destroy(struct search_list_street *this_);
static void search_list_house_number_destroy(struct search_list_house_number *this_);

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

enum include_end_nodes {
	end_nodes_yes,
	end_nodes_no,
};

struct hn_interpol_attr {
	enum attr_type house_number_interpol_attr;
	int interpol_increment;
	enum include_end_nodes include_end_nodes;
};

#define house_number_interpol_attr_END -1
/**
 * Attributes that indicate a house number interpolation,
 * along with interpolation information.
 */
struct hn_interpol_attr house_number_interpol_attrs[] = {
	{ attr_house_number_left,       1, end_nodes_yes },
	{ attr_house_number_left_odd,   2, end_nodes_yes },
	{ attr_house_number_left_even,  2, end_nodes_yes },
	{ attr_house_number_right,      1, end_nodes_yes },
	{ attr_house_number_right_odd,  2, end_nodes_yes },
	{ attr_house_number_right_even, 2, end_nodes_yes },
	{ attr_house_number_interpolation_no_ends_incrmt_1, 1, end_nodes_no },
	{ attr_house_number_interpolation_no_ends_incrmt_2, 2, end_nodes_no },
	{ house_number_interpol_attr_END, -1, -1 },
};

/** Data for a house number interpolation. */
struct house_number_interpolation {
        /** Index of interpolation attribute currently used. */
	int curr_interpol_attr_idx;
	/** Interpolation increment */
	int increment;
	/** Reverse interpolation? (0/1) */
	int rev;
	/** First number. */
	char *first;
	/** Last number. */
        char *last;
	/** Include first and last node in interpolation results? */
	enum include_end_nodes include_end_nodes;
	/** Current number in running interpolation. */
        char *curr;
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
	struct house_number_interpolation inter;
	int use_address_results;
	GList *address_results,*address_results_pos;
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

/**
 * @brief Create new instance of search_list to run a search.
 *
 * @param ms mapset that is to be searched
 * @returns new search_list
 */
struct search_list *
search_list_new(struct mapset *ms)
{
	struct search_list *ret;

	ret=g_new0(struct search_list, 1);
	ret->ms=ms;

	return ret;
}

static void search_list_search_free(struct search_list *sl, int level);

/**
 * @brief Determine search list level for given attr_type.
 * @param attr_type attribute value
 * @return corresponding search list level (country=0, town=1, ...)
 */
int
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
house_number_interpolation_clear_current(struct house_number_interpolation *inter)
{
	g_free(inter->first);
	g_free(inter->last);
	g_free(inter->curr);
	inter->first=inter->last=inter->curr=NULL;
	inter->increment=inter->include_end_nodes=-1;
}

static void
house_number_interpolation_clear_all(struct house_number_interpolation *inter)
{
	inter->curr_interpol_attr_idx=0;
	house_number_interpolation_clear_current(inter);
}

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

struct phrase {
	char *start;
	char *end;
	int wordcount;
};

static GList *
search_split_phrases(char *str)
{
	char *s,*d;
	int wordcount=0;
	GList *ret=NULL;
	s=str;
	do {
		d=s;
		wordcount=0;
		do {
			d++;
			if (*d == ' ' || *d == '\0') {
				struct phrase *phrase=g_new(struct phrase, 1);
				phrase->start=s;
				phrase->end=d;
				phrase->wordcount=++wordcount;	
				ret=g_list_append(ret, phrase);
			}
		} while (*d != '\0');
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

static char *
search_phrase_str(struct phrase *p)
{
	int len=p->end-p->start;
	char *ret=g_malloc(len+1);
	strncpy(ret, p->start, len);
	ret[len]='\0';
	return ret;
}

static int
search_phrase_used(struct phrase *p, GList *used_phrases)
{
	while (used_phrases) {
		struct phrase *pu=used_phrases->data;
		dbg(1,"'%s'-'%s' vs '%s'-'%s'\n",p->start,p->end,pu->start,pu->end);
		if (p->start < pu->end && p->end > pu->start)
			return 1;
		dbg(1,"unused\n");
		used_phrases=g_list_next(used_phrases);
	}
	return 0;
}

static gint
search_by_address_compare(gconstpointer a, gconstpointer b)
{
	const struct search_list_result *slra=a;
	const struct search_list_result *slrb=b;
	return slrb->id-slra->id;
}


static GList *
search_by_address_attr(GList *results, struct search_list *sl, GList *phrases, GList *exclude, enum attr_type attr_type, int wordcount)
{
	GList *tmp=phrases;
	struct attr attr;
	attr.type=attr_type;
	while (tmp) {
		if (!search_phrase_used(tmp->data, exclude)) {
			struct phrase *p=tmp->data;
			int count=0,wordcount_all=wordcount+p->wordcount;
			struct search_list_result *slr;
			attr.u.str=search_phrase_str(p);
			dbg(1,"%s phrase '%s'\n",attr_to_name(attr_type),attr.u.str);
			search_list_search(sl, &attr, 0);
			while ((slr=search_list_get_result(sl))) {
				if (attr_type != attr_country_all) {
					struct search_list_result *slrd=search_list_result_dup(slr);
					slrd->id=wordcount_all;
					results=g_list_insert_sorted(results, slrd, search_by_address_compare);
				}
				count++;
			}
			dbg(1,"%d results wordcount %d\n",count,wordcount_all);
			if (count) {
				GList *used=g_list_prepend(g_list_copy(exclude), tmp->data);
				enum attr_type new_attr_type=attr_none;
				switch (attr_type) {
				case attr_country_all:
					new_attr_type=attr_town_or_district_name;
					break;
				case attr_town_or_district_name:
					new_attr_type=attr_street_name;
					break;
				case attr_street_name:
					new_attr_type=attr_house_number;
					break;
				default:
					break;
				}
				if (new_attr_type != attr_none)
					results=search_by_address_attr(results, sl, phrases, used, new_attr_type, wordcount_all);
				g_list_free(used);
			}
			g_free(attr.u.str);
		}
		tmp=g_list_next(tmp);
	}
	return results;
}

static void
search_by_address(struct search_list *this_, char *addr)
{
	char *str=search_fix_spaces(addr);
	GList *tmp,*phrases=search_split_phrases(str);
	struct search_list *sl=search_list_new(this_->ms);
	this_->address_results=search_by_address_attr(NULL, sl, phrases, NULL, attr_country_all, 0);
	this_->address_results_pos=this_->address_results;
	search_list_destroy(sl);
	tmp=phrases;
	while (tmp) {
		g_free(tmp->data);
		tmp=g_list_next(tmp);
	}	
	g_list_free(phrases);
	// TODO: Looks like we should g_free(str) here. But this is
	// currently dead code, so no way to test it.
}

static void
search_address_results_free(struct search_list *this_)
{
	GList *tmp;
	tmp=this_->address_results;
	while (tmp) {
		struct search_list_result *slr=tmp->data;
		if (slr->country)
			search_list_country_destroy(slr->country);
		if (slr->town)
			search_list_town_destroy(slr->town);
		if (slr->street)
			search_list_street_destroy(slr->street);
		if (slr->house_number)
			search_list_house_number_destroy(slr->house_number);
		if (slr->c)
			g_free(slr->c);
		g_free(slr);
		tmp=g_list_next(tmp);
	}
	g_list_free(this_->address_results);
	this_->address_results=this_->address_results_pos=NULL;
}

/**
 * @brief Start a search.
 *
 * @param this search_list to use for the search
 * @param search_attr attributes to use for the search
 * @param partial do partial search? (1=yes,0=no)
 */
void
search_list_search(struct search_list *this_, struct attr *search_attr, int partial)
{
	struct search_list_level *le;
	int level;
	dbg(2,"Starting search for '=%s' of type %s\n", search_attr->u.str, attr_to_name(search_attr->type));
	search_address_results_free(this_);
	if (search_attr->type == attr_address) {
		search_by_address(this_, search_attr->u.str);
		this_->use_address_results=1;
		return;	
	}
	this_->use_address_results=0;
	level=search_list_level(search_attr->type);
	this_->item=NULL;
	house_number_interpolation_clear_all(&this_->inter);
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
	//dbg(0,"enter level=%d %d %d %p\n", level, id, mode, curr);
	while (curr) {
		num++;
		if (! id || num == id) {
			slc=curr->data;
			slc->selected=mode;
			if (id) {
				le->last=curr;
				//dbg(0,"found\n");
				return slc;
			}
		}
		curr=g_list_next(curr);
	}
	//dbg(0,"not found\n");
	return NULL;
}

static void
search_list_common_addattr(struct attr* attr,struct search_list_common *common)
{

	common->attrs=attr_generic_prepend_attr(common->attrs,attr);
	switch(attr->type) {
		case attr_town_name:
			common->town_name=common->attrs[0]->u.str;
			break;
		case attr_county_name:
			common->county_name=common->attrs[0]->u.str;
			break;
		case attr_district_name:
			common->district_name=common->attrs[0]->u.str;
			break;
		case attr_postal:
			common->postal=common->attrs[0]->u.str;
			break;
		case attr_town_postal:
			if(!common->postal)
				common->postal=common->attrs[0]->u.str;
			break;
		case attr_postal_mask:
			common->postal_mask=common->attrs[0]->u.str;
			break;
		default:
			break;
	}
}

static void
search_list_common_new(struct item *item, struct search_list_common *common)
{
	struct attr attr;
	int i;
	enum attr_type common_attrs[]={
		attr_state_name, 
		attr_county_name, 
		attr_municipality_name, 
		attr_town_name, 
		attr_district_name, 
		attr_postal, 
		attr_town_postal, 
		attr_postal_mask,
		attr_none
	};

	common->town_name=NULL;
	common->district_name=NULL;
	common->county_name=NULL;
	common->postal=NULL;
	common->postal_mask=NULL;
	common->attrs=NULL;

	for(i=0;common_attrs[i];i++) {
		if (item_attr_get(item, common_attrs[i], &attr)) {
			struct attr at;
			at.type=attr.type;
			at.u.str=map_convert_string(item->map, attr.u.str);
			search_list_common_addattr(&at,common);
			map_convert_free(at.u.str);
		}
	}
}

static void
search_list_common_dup(struct search_list_common *src, struct search_list_common *dst)
{
	int i;

	if(dst->attrs) {
		for(i=0;dst->attrs[i];i++)
			search_list_common_addattr(src->attrs[i],dst);
	}

	if (src->c) {
		dst->c=g_new(struct pcoord, 1);
		*dst->c=*src->c;
	} else
		dst->c=NULL;
}

static void
search_list_common_destroy(struct search_list_common *common)
{
	g_free(common->c);
	attr_list_free(common->attrs);

	common->town_name=NULL;
	common->district_name=NULL;
	common->county_name=NULL;
	common->postal=NULL;
	common->postal_mask=NULL;
	common->c=NULL;
	common->attrs=NULL;
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
#ifdef HAVE_API_ANDROID
		ret->iso2=g_malloc(strlen(attr.u.str)+1);
		strtolower(ret->iso2, attr.u.str);
#else
		ret->iso2=g_strdup(attr.u.str);
#endif
		ret->flag=g_strdup_printf("country_%s", ret->iso2);
	}
	if (item_attr_get(item, attr_country_iso3, &attr))
		ret->iso3=g_strdup(attr.u.str);
	if (item_attr_get(item, attr_country_name, &attr))
		ret->name=g_strdup(attr.u.str);
	return ret;
}

static struct search_list_country *
search_list_country_dup(struct search_list_country *this_)
{
	struct search_list_country *ret=g_new(struct search_list_country, 1);
	ret->car=g_strdup(this_->car);
	ret->iso2=g_strdup(this_->iso2);
	ret->iso3=g_strdup(this_->iso3);
	ret->flag=g_strdup(this_->flag);
	ret->name=g_strdup(this_->name);
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

static struct search_list_town *
search_list_town_dup(struct search_list_town *this_)
{
	struct search_list_town *ret=g_new0(struct search_list_town, 1);
	ret->county=map_convert_dup(this_->county);
	search_list_common_dup(&this_->common, &ret->common);
	return ret;
}

static void
search_list_town_destroy(struct search_list_town *this_)
{
	map_convert_free(this_->county);
	search_list_common_destroy(&this_->common);
	g_free(this_);
}


static struct search_list_street *
search_list_street_new(struct item *item)
{
	struct search_list_street *ret=g_new0(struct search_list_street, 1);
	struct attr attr;
	struct coord p[1024];
	struct coord c;
	int count;

	ret->common.item=ret->common.unique=*item;
	if (item_attr_get(item, attr_street_name, &attr))
		ret->name=map_convert_string(item->map, attr.u.str);
	else
		ret->name=NULL;
	search_list_common_new(item, &ret->common);
	count=item_coord_get(item, p, sizeof(p)/sizeof(*p));
	if (count) {
		geom_line_middle(p,count,&c);
		ret->common.c=g_new(struct pcoord, 1);
		ret->common.c->x=c.x;
		ret->common.c->y=c.y;
		ret->common.c->pro = map_projection(item->map);
	}
	return ret;
}

static struct search_list_street *
search_list_street_dup(struct search_list_street *this_)
{
	struct search_list_street *ret=g_new0(struct search_list_street, 1);
	ret->name=map_convert_dup(this_->name);
	search_list_common_dup(&this_->common, &ret->common);
	return ret;
}

static void
search_list_street_destroy(struct search_list_street *this_)
{
	map_convert_free(this_->name);
	search_list_common_destroy(&this_->common);
	g_free(this_);
}

static char *
search_next_house_number_curr_interpol_with_ends(struct house_number_interpolation *inter)
{
	dbg(1,"interpolate %s-%s %s\n",inter->first,inter->last,inter->curr);
	if (!inter->first || !inter->last)
		return NULL;
	if (!inter->curr)
		inter->curr=g_strdup(inter->first);
	else {
		if (strcmp(inter->curr, inter->last)) {
			int next=atoi(inter->curr)+(inter->increment);
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

static int
house_number_is_end_number(char* house_number, struct house_number_interpolation *inter) {
	return ( (!strcmp(house_number, inter->first))
		|| (!strcmp(house_number, inter->last)) );
}

static char *
search_next_house_number_curr_interpol(struct house_number_interpolation *inter)
{
	char* hn=NULL;
	switch (inter->include_end_nodes) {
		case end_nodes_yes:
			hn=search_next_house_number_curr_interpol_with_ends(inter);
			break;
		case end_nodes_no:
			do {
				hn=search_next_house_number_curr_interpol_with_ends(inter);
			} while (hn!=NULL && house_number_is_end_number(hn, inter));
			break;
	}
	return hn;
}

static void
search_house_number_interpolation_split(char *str, struct house_number_interpolation *inter)
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
search_match(char *str, char *search, int partial)
{
	if (!partial)
		return (!g_ascii_strcasecmp(str, search));
	else
		return (!g_ascii_strncasecmp(str, search, strlen(search)));
}

static struct pcoord *
search_house_number_coordinate(struct item *item, struct house_number_interpolation *inter)
{
	struct pcoord *ret=g_new(struct pcoord, 1);
	ret->pro = map_projection(item->map);
	dbg(1,"%s\n",item_to_name(item->type));
	if (!inter) {
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
		int hn_pos,hn_length;
		int inter_increment=inter->increment;
		struct coord *c=g_alloca(sizeof(struct coord)*max);
		item_coord_rewind(item);
		count=item_coord_get(item, c, max);
		hn_length=atoi(inter->last)-atoi(inter->first);
		if (inter->rev)
			hn_pos=atoi(inter->last)-atoi(inter->curr);
		else
			hn_pos=atoi(inter->curr)-atoi(inter->first);
		if (count) {
			int i,distance_sum=0,hn_distance;
			int *distances=g_alloca(sizeof(int)*(count-1));
			dbg(1,"count=%d hn_length=%d hn_pos=%d (%s of %s-%s)\n",count,hn_length,hn_pos,inter->curr,inter->first,inter->last);
			if (!hn_length) {
				hn_length=2;
				hn_pos=1;
			}
			if (count == max)
				dbg(0,"coordinate overflow\n");
			for (i = 0 ; i < count-1 ; i++) {
				distances[i]=navit_sqrt(transform_distance_sq(&c[i],&c[i+1]));
				distance_sum+=distances[i];
				dbg(1,"distance[%d]=%d\n",i,distances[i]);
			}
			dbg(1,"sum=%d\n",distance_sum);
#if 0
			hn_distance=distance_sum*hn_pos/hn_length;
#else
			hn_distance=(distance_sum*hn_pos+distance_sum*inter_increment/2)/(hn_length+inter_increment);
#endif
			dbg(1,"hn_distance=%d\n",hn_distance);
			i=0;
			while (i < count-1 && hn_distance > distances[i])
				hn_distance-=distances[i++];
			dbg(1,"remaining distance=%d from %d\n",hn_distance,distances[i]);
			ret->x=(c[i+1].x-c[i].x)*hn_distance/distances[i]+c[i].x;
			ret->y=(c[i+1].y-c[i].y)*hn_distance/distances[i]+c[i].y;
		}
	}
	return ret;
}

static char *
search_next_interpolated_house_number(struct item *item, struct house_number_interpolation *inter, char *inter_match, int inter_partial)
{
	while (1) {
		char *hn;
		struct attr attr;
		struct hn_interpol_attr curr_interpol_attr;
		while((hn=search_next_house_number_curr_interpol(inter))){
			if (search_match(hn, inter_match, inter_partial)) {
				return map_convert_string(item->map, hn);
			}
		}

		house_number_interpolation_clear_current(inter);
		curr_interpol_attr=house_number_interpol_attrs[inter->curr_interpol_attr_idx];
		if (curr_interpol_attr.house_number_interpol_attr==house_number_interpol_attr_END) {
			return NULL;
		}
		if (item_attr_get(item, curr_interpol_attr.house_number_interpol_attr, &attr)) {
			search_house_number_interpolation_split(attr.u.str, inter);
			inter->increment=curr_interpol_attr.interpol_increment;
			inter->include_end_nodes=curr_interpol_attr.include_end_nodes;
		}
		inter->curr_interpol_attr_idx++;
	}
}

static struct search_list_house_number *
search_list_house_number_new(struct item *item, struct house_number_interpolation *inter, char *inter_match, int inter_partial)
{
	struct search_list_house_number *ret=g_new0(struct search_list_house_number, 1);
	struct attr attr;
	char *house_number=NULL;

	ret->common.item=ret->common.unique=*item;
	if (item_attr_get(item, attr_house_number, &attr)) {
		house_number=attr.u.str;
	} else {
		ret->house_number_interpolation=1;
		memset(&ret->common.unique, 0, sizeof(ret->common.unique));
		house_number=search_next_interpolated_house_number(item, inter, inter_match, inter_partial);
	}
	if (!house_number) {
		g_free(ret);
		return NULL;
	}
	ret->house_number=map_convert_string(item->map, house_number);
	search_list_common_new(item, &ret->common);
	ret->common.c=search_house_number_coordinate(item, ret->house_number_interpolation?inter:NULL);
	return ret;
}

static struct search_list_house_number *
search_list_house_number_dup(struct search_list_house_number *this_)
{
	struct search_list_house_number *ret=g_new0(struct search_list_house_number, 1);
	ret->house_number=map_convert_dup(this_->house_number);
	search_list_common_dup(&this_->common, &ret->common);
	return ret;
}

static void
search_list_house_number_destroy(struct search_list_house_number *this_)
{
	map_convert_free(this_->house_number);
	search_list_common_destroy(&this_->common);
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

static struct search_list_result *
search_list_result_dup(struct search_list_result *slr)
{
	struct search_list_result *ret=g_new0(struct search_list_result, 1);
	ret->id=slr->id;
	if (slr->c) {
		ret->c=g_new(struct pcoord, 1);
		*ret->c=*slr->c;
	}
	if (slr->country) 
		ret->country=search_list_country_dup(slr->country);
	if (slr->town)
		ret->town=search_list_town_dup(slr->town);
	if (slr->street)
		ret->street=search_list_street_dup(slr->street);
	if (slr->house_number)
		ret->house_number=search_list_house_number_dup(slr->house_number);
	return ret;	
}

static void
search_list_search_free(struct search_list *sl, int level)
{
	struct search_list_level *le=&sl->levels[level];
	GList *next,*curr;
	if (le->search)
	{
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
	while (curr)
	{
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
	int i;
	char *ret=NULL;
	dbg(1,"enter %s %s\n", mask, new);
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
	int unique=0;
	if (slc->unique.type || slc->unique.id_hi || slc->unique.id_lo)
		unique=1;
	if (unique)
		slo=g_hash_table_lookup(le->hash, &slc->unique);
	else
		slo=NULL;
	if (!slo) {
		if (unique)
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

/**
 * @brief Get (next) result from a search.
 *
 * @param this_ search_list representing the search
 * @return next result
 */
struct search_list_result *
search_list_get_result(struct search_list *this_)
{
	struct search_list_level *le,*leu;
	int level=this_->level;
	struct attr attr2;
	int has_street_name=0;

	if (this_->use_address_results) {
		struct search_list_result *ret=NULL;
		if (this_->address_results_pos) {
			ret=this_->address_results_pos->data;
			this_->address_results_pos=g_list_next(this_->address_results_pos);
		}
		return ret;
	}

	//dbg(0,"enter\n");
	le=&this_->levels[level];
	//dbg(0,"le=%p\n", le);
	for (;;)
	{
		//dbg(0,"le->search=%p\n", le->search);
		if (! le->search)
		{
			//dbg(0,"partial=%d level=%d\n", le->partial, level);
			if (! level)
				le->parent=NULL;
			else
			{
				leu=&this_->levels[level-1];
				//dbg(0,"leu->curr=%p\n", leu->curr);
				for (;;)
				{
					//dbg(0,"*********########");

					struct search_list_common *slc;
					if (! leu->curr)
					{
						return NULL;
					}
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
			{
				//dbg(0,"mapset_search_new with item(%d,%d)\n", le->parent->item.id_hi, le->parent->item.id_lo);
			}
			//dbg(0,"############## attr=%s\n", attr_to_name(le->attr->type));
			le->search=mapset_search_new(this_->ms, &le->parent->item, le->attr, le->partial);
			le->hash=g_hash_table_new(search_item_hash_hash, search_item_hash_equal);
		}
		//dbg(0,"le->search=%p\n", le->search);
		if (!this_->item)
		{
			//dbg(0,"sssss 1");
			this_->item=mapset_search_get_item(le->search);
			//dbg(0,"sssss 1 %p\n",this_->item);
		}
		if (this_->item)
		{
			void *p=NULL;
			//dbg(0,"id_hi=%d id_lo=%d\n", this_->item->id_hi, this_->item->id_lo);
			if (this_->postal)
			{
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
			//dbg(0,"case x LEVEL start %d\n",level);
			switch (level)
			{
			case 0:
				//dbg(0,"case 0 COUNTRY");
				p=search_list_country_new(this_->item);
				this_->result.country=p;
				this_->result.country->common.parent=NULL;
				this_->result.town=NULL;
				this_->result.street=NULL;
				this_->result.house_number=NULL;
				this_->item=NULL;
				break;
			case 1:
				//dbg(0,"case 1 TOWN");
				p=search_list_town_new(this_->item);
				this_->result.town=p;
				this_->result.town->common.parent=this_->levels[0].last->data;
				this_->result.country=this_->result.town->common.parent;
				this_->result.c=this_->result.town->common.c;
				this_->result.street=NULL;
				this_->result.house_number=NULL;
				this_->item=NULL;
				break;
			case 2:
				//dbg(0,"case 2 STREET");
				p=search_list_street_new(this_->item);
				this_->result.street=p;
				this_->result.street->common.parent=this_->levels[1].last->data;
				this_->result.town=this_->result.street->common.parent;
				this_->result.country=this_->result.town->common.parent;
				this_->result.c=this_->result.street->common.c;
				this_->result.house_number=NULL;
				this_->item=NULL;
				break;
			case 3:
				dbg(1,"case 3 HOUSENUMBER\n");
				has_street_name=0;

				// if this housenumber has a streetname tag, set the name now
				if (item_attr_get(this_->item, attr_street_name, &attr2))
				{
					dbg(1,"streetname: %s\n",attr2.u.str);
					has_street_name=1;
				}

				p=search_list_house_number_new(this_->item, &this_->inter, le->attr->u.str, le->partial);
				if (!p)
				{
					house_number_interpolation_clear_all(&this_->inter);
					this_->item=NULL;
					continue;
				}

				this_->result.house_number=p;
				if (!this_->result.house_number->house_number_interpolation)
				{
					this_->item=NULL;
				} else {
					dbg(0,"interpolation!\n");
				}

				if(le->parent && has_street_name) {
					struct search_list_street *street=this_->levels[level-1].last->data;
					if(navit_utf8_strcasecmp(street->name, attr2.u.str)) {
						search_list_house_number_destroy(p);
						//this_->item=NULL;
						continue;
					}
				}


				this_->result.house_number->common.parent=this_->levels[2].last->data;
				this_->result.street=this_->result.house_number->common.parent;
				this_->result.town=this_->result.street->common.parent;
				this_->result.country=this_->result.town->common.parent;
				this_->result.c=this_->result.house_number->common.c;

#if 0
				if(!has_street_name) {
					static struct search_list_street null_street;
					this_->result.street=&null_street;
				}
#endif
			}
			if (p)
			{
				if (search_add_result(le, p))
				{
					this_->result.id++;
					return &this_->result;
				}
				else
				{
					search_list_result_destroy(level, p);
				}
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
