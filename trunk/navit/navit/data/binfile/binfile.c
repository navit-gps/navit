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
#include "map.h"
#include "maptype.h"
#include "item.h"
#include "attr.h"
#include "coord.h"
#include "transform.h"
#include "file.h"
#include "zipfile.h"
#include "endianess.h"

static int map_id;


struct minmax {
	short min;
	short max;
};

struct tile {
	int *start;
	int *end;
	int *pos;
	int *pos_coord_start;
	int *pos_coord;
	int *pos_attr_start;
	int *pos_attr;
	int *pos_next;
	int zipfile_num;
};

struct map_priv {
	int id;
	char *filename;
	char *cachedir;
	struct file *fi;
	struct zip_cd *index_cd;
	int index_offset;
	int cde_size;
	struct zip_eoc *eoc;
	int zip_members;
	unsigned char *search_data;
	int search_offset;
	int search_size;
};

struct map_rect_priv {
	int *start;
	int *end;
	enum attr_type attr_last;
	int label;
	int *label_attr[2];
        struct map_selection *sel;
        struct map_priv *m;
        struct item item;
	int tile_depth;
	struct tile tiles[8];
	struct tile *t;
	int country_id;
	char *url;
};

struct map_search_priv {
	struct map_rect_priv *mr;
	struct attr *search;
	struct map_selection *ms;
	int partial;
	GHashTable *search_results;
};


static void minmax_to_cpu(struct minmax * mima) {
	g_assert(mima  != NULL);
	mima->min = le16_to_cpu(mima->min);
	mima->max = le16_to_cpu(mima->max);
}

static void lfh_to_cpu(struct zip_lfh *lfh) {
	g_assert(lfh != NULL);
	lfh->ziplocsig = le32_to_cpu(lfh->ziplocsig);
	lfh->zipver    = le16_to_cpu(lfh->zipver);
	lfh->zipgenfld = le16_to_cpu(lfh->zipgenfld);
	lfh->zipmthd   = le16_to_cpu(lfh->zipmthd);
	lfh->ziptime   = le16_to_cpu(lfh->ziptime);
	lfh->zipdate   = le16_to_cpu(lfh->zipdate);
	lfh->zipcrc    = le32_to_cpu(lfh->zipcrc);
	lfh->zipsize   = le32_to_cpu(lfh->zipsize);
	lfh->zipuncmp  = le32_to_cpu(lfh->zipuncmp);
	lfh->zipfnln   = le16_to_cpu(lfh->zipfnln);
	lfh->zipxtraln = le16_to_cpu(lfh->zipxtraln);
}

static void cd_to_cpu(struct zip_cd *zcd) {
	g_assert(zcd != NULL);
	zcd->zipccrc   = le32_to_cpu(zcd->zipccrc);
	zcd->zipcsiz   = le32_to_cpu(zcd->zipcsiz);
	zcd->zipcunc   = le32_to_cpu(zcd->zipcunc);
	zcd->zipcfnl   = le16_to_cpu(zcd->zipcfnl);
	zcd->zipcxtl   = le16_to_cpu(zcd->zipcxtl);
	zcd->zipccml   = le16_to_cpu(zcd->zipccml);
	zcd->zipdsk    = le16_to_cpu(zcd->zipdsk);
	zcd->zipint    = le16_to_cpu(zcd->zipint);
	zcd->zipext    = le32_to_cpu(zcd->zipext);
	zcd->zipofst   = le32_to_cpu(zcd->zipofst);
}

static void eoc_to_cpu(struct zip_eoc *eoc) {
	g_assert(eoc != NULL);
	eoc->zipesig   = le32_to_cpu(eoc->zipesig);
	eoc->zipedsk   = le16_to_cpu(eoc->zipedsk);
	eoc->zipecen   = le16_to_cpu(eoc->zipecen);
	eoc->zipenum   = le16_to_cpu(eoc->zipenum);
	eoc->zipecenn  = le16_to_cpu(eoc->zipecenn);
	eoc->zipecsz   = le32_to_cpu(eoc->zipecsz);
	eoc->zipeofst  = le32_to_cpu(eoc->zipeofst);
	eoc->zipecoml  = le16_to_cpu(eoc->zipecoml);
}

static struct zip_eoc *
binfile_read_eoc(struct file *fi)
{
	struct zip_eoc *eoc;
	eoc=(struct zip_eoc *)file_data_read(fi,fi->size-sizeof(struct zip_eoc), sizeof(struct zip_eoc));
	if (eoc) {
		eoc_to_cpu(eoc);
		dbg(0,"sig 0x%x\n", eoc->zipesig);
		if (eoc->zipesig != zip_eoc_sig) {
			file_data_free(fi,(unsigned char *)eoc);
			eoc=NULL;
		}
	}
	return eoc;
}

static struct zip_cd *
binfile_read_cd(struct map_priv *m, int offset, int len)
{
	struct zip_cd *cd;
	if (len == -1) {
		cd=(struct zip_cd *)file_data_read(m->fi,m->eoc->zipeofst+offset, sizeof(*cd));
		cd_to_cpu(m->index_cd);
		len=cd->zipcfnl;
		file_data_free(m->fi,(unsigned char *)cd);
	}
	cd=(struct zip_cd *)file_data_read(m->fi,m->eoc->zipeofst+offset, sizeof(*cd)+len);
	if (cd) {
		cd_to_cpu(cd);
		dbg(0,"sig 0x%x\n", cd->zipcensig);
		if (cd->zipcensig != zip_cd_sig) {
			file_data_free(m->fi,(unsigned char *)cd);
			cd=NULL;
		}
	}
	return cd;
}

static struct zip_lfh *
binfile_read_lfh(struct file *fi, int offset)
{
	struct zip_lfh *lfh;

	lfh=(struct zip_lfh *)(file_data_read(fi,offset,sizeof(struct zip_lfh)));
	if (lfh) {
		lfh_to_cpu(lfh);
		if (lfh->ziplocsig != zip_lfh_sig) {
			file_data_free(fi,(unsigned char *)lfh);
			lfh=NULL;
		}
	}
	return lfh;
}

static unsigned char *
binfile_read_content(struct file *fi, int offset, struct zip_lfh *lfh)
{
	offset+=sizeof(struct zip_lfh)+lfh->zipfnln+lfh->zipxtraln;
	switch (lfh->zipmthd) {
	case 0:
		return file_data_read(fi,offset, lfh->zipuncmp);
	case 8:
		return file_data_read_compressed(fi,offset, lfh->zipsize, lfh->zipuncmp);
	default:
		dbg(0,"Unknown compression method %d\n", lfh->zipmthd);
		return NULL;
	}
}

static int
binfile_search_cd(struct map_priv *m, int offset, char *name, int partial, int skip)
{
	int size=4096;
	int end=m->eoc->zipecsz;
	int len=strlen(name);
	struct zip_cd *cd;
#if 0
	dbg(0,"end=%d\n",end);
#endif
	while (offset < end) {
		cd=(struct zip_cd *)(m->search_data+offset-m->search_offset);
		if (! m->search_data || 
		      m->search_offset > offset || 
		      offset-m->search_offset+sizeof(*cd) > m->search_size ||
		      offset-m->search_offset+sizeof(*cd)+cd->zipcfnl > m->search_size
		   ) {
#if 0
			dbg(0,"reload %p %d %d\n", m->search_data, m->search_offset, offset);
#endif
			if (m->search_data)
				file_data_free(m->fi,m->search_data);
			m->search_offset=offset;
			m->search_size=end-offset;
			if (m->search_size > size)
				m->search_size=size;
			m->search_data=file_data_read(m->fi,m->eoc->zipeofst+m->search_offset,m->search_size);
			cd=(struct zip_cd *)m->search_data;
		}
#if 0
		dbg(0,"offset=%d search_offset=%d search_size=%d search_data=%p cd=%p\n", offset, m->search_offset, m->search_size, m->search_data, cd);
		dbg(0,"offset=%d fn='%s'\n",offset,cd->zipcfn);
#endif
		if (!skip && 
		    (partial || cd->zipcfnl == len) &&
		    !strncmp(cd->zipcfn, name, len)) 
			return offset;
		skip=0;
		offset+=sizeof(*cd)+cd->zipcfnl+cd->zipcxtl+cd->zipccml;
;
	}
	return -1;
}

static void
map_destroy_binfile(struct map_priv *m)
{
	dbg(1,"map_destroy_binfile\n");
	g_free(m);
}

static void
binfile_coord_rewind(void *priv_data)
{
	struct map_rect_priv *mr=priv_data;
	struct tile *t=mr->t;
	t->pos_coord=t->pos_coord_start;
}

static int
binfile_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *mr=priv_data;
	struct tile *t=mr->t;
	int ret=0;
	dbg(2,"binfile_coord_get %d\n",count);
	while (count--) {
		dbg(2,"%p vs %p\n", t->pos_coord, t->pos_attr_start);
		if (t->pos_coord >= t->pos_attr_start)
			break;
		c->x=le32_to_cpu(*(t->pos_coord++));
		c->y=le32_to_cpu(*(t->pos_coord++));
		c++;
		ret++;
	}
	return ret;
}

static void
binfile_attr_rewind(void *priv_data)
{
	struct map_rect_priv *mr=priv_data;
	struct tile *t=mr->t;
	t->pos_attr=t->pos_attr_start;
	
}

static char *
binfile_extract(struct map_priv *m, char *dir, char *filename, int partial)
{
	char *full,*fulld,*sep;
	unsigned char *start;
	int len,offset=m->index_offset;
	struct zip_cd *cd;
	struct zip_lfh *lfh;
	FILE *f;

	for (;;) {
		offset=binfile_search_cd(m, offset, filename, partial, 1);
		if (offset == -1)
			break;
		cd=binfile_read_cd(m, offset, -1);
		len=strlen(dir)+1+cd->zipcfnl+1;
		full=g_malloc(len);
		strcpy(full,dir);
		strcpy(full+strlen(full),"/");
		strncpy(full+strlen(full),cd->zipcfn,cd->zipcfnl);
		full[len-1]='\0';
		fulld=g_strdup(full);
		sep=strrchr(fulld, '/');
		if (sep) {
			*sep='\0';
			file_mkdir(fulld, 1);
		}
		if (full[len-2] != '/') {
			lfh=binfile_read_lfh(m->fi, cd->zipofst);
			start=binfile_read_content(m->fi, cd->zipofst, lfh);
			dbg(0,"fopen '%s'\n", full);
			f=fopen(full,"w");
			fwrite(start, lfh->zipuncmp, 1, f);
			fclose(f);
			file_data_free(m->fi, start);
			file_data_free(m->fi, (unsigned char *)lfh);
		}
		file_data_free(m->fi, (unsigned char *)cd);
		g_free(fulld);
		g_free(full);
		if (! partial)
			break;
	}
	
	return g_strdup_printf("%s/%s",dir,filename);
}

static int
binfile_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{	
	struct map_rect_priv *mr=priv_data;
	struct tile *t=mr->t;
	enum attr_type type;
	int i,size;

	if (attr_type != mr->attr_last) {
		t->pos_attr=t->pos_attr_start;
		mr->attr_last=attr_type;
	}
	while (t->pos_attr < t->pos_next) {
		size=le32_to_cpu(*(t->pos_attr++));
		type=le32_to_cpu(t->pos_attr[0]);
		if (type == attr_label) 
			mr->label=1;
		if (type == attr_town_name)
			mr->label_attr[0]=t->pos_attr;
		if (type == attr_street_name)
			mr->label_attr[0]=t->pos_attr;
		if (type == attr_street_name_systematic)
			mr->label_attr[1]=t->pos_attr;
		if (type == attr_type || attr_type == attr_any) {
			if (attr_type == attr_any) {
				dbg(1,"pos %p attr %s size %d\n", t->pos_attr-1, attr_to_name(type), size);
			}
			attr->type=type;
			attr_data_set(attr, t->pos_attr+1);
			if (type == attr_url_local) {
				g_free(mr->url);
				mr->url=binfile_extract(mr->m, mr->m->cachedir, attr->u.str, 1);
				attr->u.str=mr->url;
			}
			t->pos_attr+=size;
			return 1;
		} else {
			t->pos_attr+=size;
		}
	}
	if (!mr->label && (attr_type == attr_any || attr_type == attr_label)) {
		for (i = 0 ; i < sizeof(mr->label_attr)/sizeof(int *) ; i++) {
			if (mr->label_attr[i]) {
				mr->label=1;
				attr->type=attr_label;
				attr_data_set(attr, mr->label_attr[i]+1);
				return 1;
			}
		}
	}
	return 0;
}

static struct item_methods methods_binfile = {
        binfile_coord_rewind,
        binfile_coord_get,
        binfile_attr_rewind,
        binfile_attr_get,
};

static void
push_tile(struct map_rect_priv *mr, struct tile *t)
{
	g_assert(mr->tile_depth < 8);
	mr->t=&mr->tiles[mr->tile_depth++];
	*(mr->t)=*t;
	mr->t->pos=mr->t->pos_next=mr->t->start;
}

static int
pop_tile(struct map_rect_priv *mr)
{
	if (mr->tile_depth <= 1)
		return 0;
	file_data_free(mr->m->fi, (unsigned char *)(mr->t->start));
	mr->t=&mr->tiles[--mr->tile_depth-1];
	return 1;
}


static int
zipfile_to_tile(struct file *f, struct zip_cd *cd, struct tile *t)
{
	char buffer[1024];
	struct zip_lfh *lfh;
	char *zipfn;
	dbg(1,"enter %p %p %p\n", f, cd, t);
	dbg(1,"cd->zipofst=0x%x\n", cd->zipofst);
	t->start=NULL;
	lfh=binfile_read_lfh(f, cd->zipofst);
	zipfn=(char *)(file_data_read(f,cd->zipofst+sizeof(struct zip_lfh), lfh->zipfnln));
	strncpy(buffer, zipfn, lfh->zipfnln);
	buffer[lfh->zipfnln]='\0';
	t->start=(int *)binfile_read_content(f, cd->zipofst, lfh);
	t->end=t->start+lfh->zipuncmp/4;
	dbg(1,"0x%x '%s' %d %d,%d\n", lfh->ziplocsig, buffer, sizeof(*cd)+cd->zipcfnl, lfh->zipsize, lfh->zipuncmp);
	file_data_free(f, (unsigned char *)zipfn);
	file_data_free(f, (unsigned char *)lfh);
	return t->start != NULL;
}

static void
push_zipfile_tile(struct map_rect_priv *mr, int zipfile)
{
        struct map_priv *m=mr->m;
	struct file *f=m->fi;
	struct tile t;
	struct zip_cd *cd=(struct zip_cd *)(file_data_read(f, m->eoc->zipeofst + zipfile*m->cde_size, sizeof(struct zip_cd)));
	cd_to_cpu(cd);
	dbg(1,"enter %p %d\n", mr, zipfile);
	t.zipfile_num=zipfile;
	if (zipfile_to_tile(f, cd, &t))
		push_tile(mr, &t);
	file_data_free(f, (unsigned char *)cd);
}

static struct map_rect_priv *
map_rect_new_binfile(struct map_priv *map, struct map_selection *sel)
{
	struct map_rect_priv *mr;
	struct tile t;

	dbg(1,"map_rect_new_binfile\n");
	mr=g_new0(struct map_rect_priv, 1);
	mr->m=map;
	mr->sel=sel;
	mr->item.id_hi=0;
	mr->item.id_lo=0;
	dbg(1,"zip_members=%d\n", map->zip_members);
	if (map->eoc) 
		push_zipfile_tile(mr, map->zip_members-1);
	else {
		unsigned char *d=file_data_read(map->fi, 0, map->fi->size);
		t.start=(int *)d;
		t.end=(int *)(d+map->fi->size);
		t.zipfile_num=0;
		push_tile(mr, &t);
	}
	mr->item.meth=&methods_binfile;
	mr->item.priv_data=mr;
	return mr;
}


static void
map_rect_destroy_binfile(struct map_rect_priv *mr)
{
	while (pop_tile(mr));
	file_data_free(mr->m->fi, (unsigned char *)(mr->tiles[0].start));
	g_free(mr->url);
        g_free(mr);
}

static void
setup_pos(struct map_rect_priv *mr)
{
	int size,coord_size;
	struct tile *t=mr->t;
	size=le32_to_cpu(*(t->pos++));
	if (size > 1024*1024 || size < 0) {
		dbg(0,"size=0x%x\n", size);
#if 0
		fprintf(stderr,"offset=%d\n", (unsigned char *)(mr->pos)-mr->m->f->begin);
#endif
		g_error("size error");
	}
	t->pos_next=t->pos+size;
	mr->item.type=le32_to_cpu(*(t->pos++));
	coord_size=le32_to_cpu(*(t->pos++));
	t->pos_coord_start=t->pos_coord=t->pos;
	t->pos_attr_start=t->pos_attr=t->pos_coord+coord_size;
}

static int
selection_contains(struct map_selection *sel, struct coord_rect *r, struct minmax *mima)
{
	int order;
	if (! sel)
		return 1;
	while (sel) {
		if (coord_rect_overlap(r, &sel->u.c_rect)) {
			order=sel->order[0];
			if (sel->order[1] > order)
				order=sel->order[1];
			if (sel->order[2] > order)
				order=sel->order[2];
			dbg(1,"min %d max %d order %d\n", mima->min, mima->max, order);
			if (!mima->min && !mima->max)
				return 1;
			if (order >= mima->min && order <= mima->max)
				return 1;
		}
		sel=sel->next;
	}
	return 0;
}

static void
map_parse_country_binfile(struct map_rect_priv *mr)
{
	struct attr at;
	if (binfile_attr_get(mr->item.priv_data, attr_country_id, &at)) {
		if (at.u.num == mr->country_id)
		{
			if (binfile_attr_get(mr->item.priv_data, attr_zipfile_ref, &at))
			{
				push_zipfile_tile(mr, at.u.num);
			}
		}
	}
}

static struct item *
map_rect_get_item_binfile(struct map_rect_priv *mr)
{
	struct tile *t;
	struct minmax *mima;
	for (;;) {
		t=mr->t;
		if (! t)
			return NULL;
		t->pos=t->pos_next;
		if (t->pos >= t->end) {
			if (pop_tile(mr))
				continue;
			return NULL;
		}
		mr->item.id_hi=t->zipfile_num;
		mr->item.id_lo=t->pos-t->start;
		mr->label=0;
		memset(mr->label_attr, 0, sizeof(mr->label_attr));
		setup_pos(mr);
		if ((mr->item.type == type_submap) && (!mr->country_id)) {
			struct coord_rect r;
			r.lu.x=le32_to_cpu(t->pos_coord[0]);
			r.lu.y=le32_to_cpu(t->pos_coord[3]);
			r.rl.x=le32_to_cpu(t->pos_coord[2]);
			r.rl.y=le32_to_cpu(t->pos_coord[1]);
			mima=(struct minmax *)(t->pos_attr+2);
			minmax_to_cpu(mima);
			if (!mr->m->eoc || !selection_contains(mr->sel, &r, mima)) {
				continue;
			}
			dbg(1,"pushing zipfile %d from %d\n", le32_to_cpu(t->pos_attr[5]), t->zipfile_num);
			push_zipfile_tile(mr, le32_to_cpu(t->pos_attr[5]));
			continue;
				
		}
		if (mr->country_id)
		{
			if (mr->item.type == type_countryindex) {
				map_parse_country_binfile(mr);
			}
			if (item_is_town(mr->item))
			{
				return &mr->item;
			} else {
				continue;
			}
		}
		return &mr->item;
	}
}

static struct item *
map_rect_get_item_byid_binfile(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	struct tile *t;
	if (mr->m->eoc) 
		push_zipfile_tile(mr, id_hi);
	t=mr->t;
	t->pos=t->start+id_lo;
	mr->item.id_hi=id_hi;
	mr->item.id_lo=id_lo;
	mr->label=0;
	memset(mr->label_attr, 0, sizeof(mr->label_attr));
	setup_pos(mr);
	return &mr->item;
}

static struct map_search_priv *
binmap_search_new(struct map_priv *map, struct item *item, struct attr *search, int partial)
{
	struct map_rect_priv *map_rec;
	struct map_search_priv *msp;
	struct map_selection *ms;
	struct item *town;
	int i;
	
	switch (search->type) {
		case attr_country_name:
			break;
		case attr_town_name:
			msp = g_new(struct map_search_priv, 1);
			msp->search_results = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
			map_rec = map_rect_new_binfile(map, NULL);
			map_rec->country_id = item->id_lo;
			msp->mr = map_rec;
			msp->search = search;
			msp->partial = partial;
			return msp;
			break;
		case attr_town_postal:
			break;
		case attr_street_name:
			if (! item->map)
				break;
			if (!map_priv_is(item->map, map))
				break;
			ms = g_new(struct map_selection, 1);
			ms->next = NULL;
			for (i = 0; i < layer_end; i++)
			{
				ms->order[i] = 18;
			}
			map_rec = map_rect_new_binfile(map, ms);
			town = map_rect_get_item_byid_binfile(map_rec, item->id_hi, item->id_lo);
			if (town) {
				struct map_search_priv *msp = g_new(struct map_search_priv, 1);
				struct coord *c = g_new(struct coord, 1);
				int size = 10000;
				switch (town->type) {
					case type_town_label_2e5:
						size = 10000;
						break;
					case type_town_label_2e4:
						size = 5000;
						break;
					case type_town_label_2e3:
						size = 2500;
						break;
					case type_town_label_2e2:
						size = 1000;
						break;
					default:
						break;
				}
				item_coord_get(town, c, 1);
				ms->u.c_rect.lu.x = c->x-size;
				ms->u.c_rect.lu.y = c->y+size;
				ms->u.c_rect.rl.x = c->x+size;
				ms->u.c_rect.rl.y = c->y-size;
				
				map_rect_destroy_binfile(map_rec);
				map_rec = map_rect_new_binfile(map, ms);
				msp->mr = map_rec;
				msp->search = search;
				msp->partial = partial;
				msp->search_results = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
				return msp;
			}
			map_rect_destroy_binfile(map_rec);
			g_free(ms);
			break;
		default:
			break;
	}
	return NULL;
}
static int
ascii_cmp(char *name, char *match, int partial)
{
	if (partial)
		return g_ascii_strncasecmp(name, match, strlen(match));
	else
		return g_ascii_strcasecmp(name, match);
}

static struct item *
binmap_search_get_item(struct map_search_priv *map_search)
{
	struct item* it;
	while ((it  = map_rect_get_item_binfile(map_search->mr))) {
		if (map_search->search->type == attr_town_name) {
			if (item_is_town(*it)) {
				struct attr at;
				if (binfile_attr_get(it->priv_data, attr_label, &at)) {
					if (!ascii_cmp(at.u.str, map_search->search->u.str, map_search->partial)) {
						return it;
					}
				}
			}
		} else if (map_search->search->type == attr_street_name) {
			if ((it->type == type_street_3_city) || (it->type == type_street_2_city) || (it->type == type_street_1_city)) {
				struct attr at;
				if (map_selection_contains_item_rect(map_search->mr->sel, it) && binfile_attr_get(it->priv_data, attr_label, &at)) {
					if (!ascii_cmp(at.u.str, map_search->search->u.str, map_search->partial)) {
						if (!g_hash_table_lookup(map_search->search_results, at.u.str)) {
							item_coord_rewind(it);
							item_attr_rewind(it);
							g_hash_table_insert(map_search->search_results, g_strdup(at.u.str), "");
							return it;
						}
					}
				}
			}
		}
	}
	return NULL;
}

static void
binmap_search_destroy(struct map_search_priv *ms)
{
	g_hash_table_destroy(ms->search_results);
	g_free(ms->mr->sel);
	map_rect_destroy_binfile(ms->mr);
	g_free(ms);
}

static struct map_methods map_methods_binfile = {
	projection_mg,
	"utf-8",
	map_destroy_binfile,
	map_rect_new_binfile,
	map_rect_destroy_binfile,
	map_rect_get_item_binfile,
	map_rect_get_item_byid_binfile,
	binmap_search_new,
	binmap_search_destroy,
	binmap_search_get_item
};

static int
binfile_get_index(struct map_priv *m)
{
	int len=sizeof("index")-1;
	int cde_index_size=sizeof(struct zip_cd)+len;
	int offset=m->eoc->zipecsz-cde_index_size;
	struct zip_cd *cd=binfile_read_cd(m, offset, len);
	if (cd) {
		if (cd->zipcfnl == len && !strncmp(cd->zipcfn, "index", len)) {
			m->index_offset=offset;
			m->index_cd=cd;
			return 1;
		}
	}
	offset=binfile_search_cd(m, 0, "index", 0, 0);
	if (offset == -1)
		return 0;
	cd=binfile_read_cd(m, offset, len);
	if (!cd)
		return 0;
	m->index_offset=offset;
	m->index_cd=cd;
	return 1;
}


static struct map_priv *
map_new_binfile(struct map_methods *meth, struct attr **attrs)
{
	struct map_priv *m;
	struct attr *data=attr_search(attrs, NULL, attr_data);
	struct file_wordexp *wexp;
	struct zip_cd *first_cd;
	char **wexp_data;
	int *magic;
	if (! data)
		return NULL;

	wexp=file_wordexp_new(data->u.str);
	wexp_data=file_wordexp_get_array(wexp);
	dbg(1,"map_new_binfile %s\n", data->u.str);	
	*meth=map_methods_binfile;

	m=g_new0(struct map_priv, 1);
	m->id=++map_id;
	m->filename=g_strdup(wexp_data[0]);
	dbg(0,"file_create %s\n", m->filename);
	m->fi=file_create(m->filename);
	if (! m->fi) {
		dbg(0,"Failed to load '%s'\n", m->filename);
		g_free(m);
		return NULL;
	}
	file_wordexp_destroy(wexp);
	magic=(int *)file_data_read(m->fi, 0, 4);
	*magic = le32_to_cpu(*magic);
	if (*magic == zip_lfh_sig) {
		if ((m->eoc=binfile_read_eoc(m->fi)) && binfile_get_index(m) && (first_cd=binfile_read_cd(m, 0, 0))) {
			m->cde_size=sizeof(struct zip_cd)+first_cd->zipcfnl;
			m->zip_members=m->index_offset/m->cde_size+1;
			printf("cde_size %d\n", m->cde_size);
			printf("members %d\n",m->zip_members);
			file_data_free(m->fi, (unsigned char *)first_cd);
		} else {
			dbg(0,"invalid file format for '%s'\n", m->filename);
		}
	} else 
		file_mmap(m->fi);
	file_data_free(m->fi, (unsigned char *)magic);
	m->cachedir="/tmp/navit";
	return m;
}

void
plugin_init(void)
{
	dbg(1,"binfile: plugin_init\n");
	plugin_register_map_type("binfile", map_new_binfile);
}

