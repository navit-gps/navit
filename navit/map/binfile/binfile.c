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

static int map_id;

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
	int mode;
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
	int version;
	int check_version;
	int map_version;
	GHashTable *changes;
};

struct map_rect_priv {
	int *start;
	int *end;
	enum attr_type attr_last;
	int label;
	int *label_attr[4];
        struct map_selection *sel;
        struct map_priv *m;
        struct item item;
	int tile_depth;
	struct tile tiles[8];
	struct tile *t;
	int country_id;
	char *url;
#ifdef DEBUG_SIZE
	int size;
#endif
};

struct map_search_priv {
	struct map_rect_priv *mr;
	struct attr *search;
	struct map_selection ms;
	int partial;
	GHashTable *search_results;
};


static void push_tile(struct map_rect_priv *mr, struct tile *t);
static void setup_pos(struct map_rect_priv *mr);

static void lfh_to_cpu(struct zip_lfh *lfh) {
	dbg_assert(lfh != NULL);
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
	dbg_assert(zcd != NULL);
	zcd->zipcensig = le32_to_cpu(zcd->zipcensig);
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
	dbg_assert(eoc != NULL);
	eoc->zipesig   = le32_to_cpu(eoc->zipesig);
	eoc->zipedsk   = le16_to_cpu(eoc->zipedsk);
	eoc->zipecen   = le16_to_cpu(eoc->zipecen);
	eoc->zipenum   = le16_to_cpu(eoc->zipenum);
	eoc->zipecenn  = le16_to_cpu(eoc->zipecenn);
	eoc->zipecsz   = le32_to_cpu(eoc->zipecsz);
	eoc->zipeofst  = le32_to_cpu(eoc->zipeofst);
	eoc->zipecoml  = le16_to_cpu(eoc->zipecoml);
}

static void binfile_check_version(struct map_priv *m);

static struct zip_eoc *
binfile_read_eoc(struct file *fi)
{
	struct zip_eoc *eoc;
	eoc=(struct zip_eoc *)file_data_read(fi,fi->size-sizeof(struct zip_eoc), sizeof(struct zip_eoc));
	if (eoc) {
		eoc_to_cpu(eoc);
		dbg(1,"sig 0x%x\n", eoc->zipesig);
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
		cd_to_cpu(cd);
		len=cd->zipcfnl+cd->zipcxtl;
		file_data_free(m->fi,(unsigned char *)cd);
	}
	cd=(struct zip_cd *)file_data_read(m->fi,m->eoc->zipeofst+offset, sizeof(*cd)+len);
	if (cd) {
		cd_to_cpu(cd);
		dbg(1,"sig 0x%x\n", cd->zipcensig);
		if (cd->zipcensig != zip_cd_sig) {
			file_data_free(m->fi,(unsigned char *)cd);
			cd=NULL;
		}
	}
	return cd;
}

static long long
binfile_cd_offset(struct zip_cd *cd)
{
	struct zip_cd_ext *ext;
	if (cd->zipofst != 0xffffffff || cd->zipcxtl != sizeof(*ext))
		return cd->zipofst;
	ext=(struct zip_cd_ext *)((unsigned char *)cd+sizeof(*cd)+cd->zipcfnl);
	if (ext->tag != 0x0001 || ext->size != 8)
		return cd->zipofst;
	return ext->zipofst;
}

static struct zip_lfh *
binfile_read_lfh(struct file *fi, unsigned int offset)
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
		      offset-m->search_offset+sizeof(*cd)+cd->zipcfnl+cd->zipcxtl > m->search_size
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
	int max,ret=0;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	max=(t->pos_attr_start-t->pos_coord)/2;
	if (count > max)
		count=max;
	memcpy(c, t->pos_coord, count*sizeof(struct coord));
	t->pos_coord+=count*2;
	ret=count;
#else
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
#endif
	return ret;
}

static void
binfile_attr_rewind(void *priv_data)
{
	struct map_rect_priv *mr=priv_data;
	struct tile *t=mr->t;
	t->pos_attr=t->pos_attr_start;
	mr->label=0;
	memset(mr->label_attr, 0, sizeof(mr->label_attr));
	
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
			lfh=binfile_read_lfh(m->fi, binfile_cd_offset(cd));
			start=binfile_read_content(m->fi, binfile_cd_offset(cd), lfh);
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
		if (type == attr_house_number)
			mr->label_attr[0]=t->pos_attr;
		if (type == attr_street_name)
			mr->label_attr[1]=t->pos_attr;
		if (type == attr_street_name_systematic)
			mr->label_attr[2]=t->pos_attr;
		if (type == attr_town_name && mr->item.type < type_line)
			mr->label_attr[3]=t->pos_attr;
		if (type == attr_type || attr_type == attr_any) {
			if (attr_type == attr_any) {
				dbg(1,"pos %p attr %s size %d\n", t->pos_attr-1, attr_to_name(type), size);
			}
			attr->type=type;			
			attr_data_set_le(attr, t->pos_attr+1); 
			if (type == attr_url_local) {
				g_free(mr->url);
				mr->url=binfile_extract(mr->m, mr->m->cachedir, attr->u.str, 1);
				attr->u.str=mr->url;
			}
			if (type == attr_flags && mr->m->map_version < 1) 
				attr->u.num |= AF_CAR;
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
				attr_data_set_le(attr,mr->label_attr[i]+1);
				return 1;
			}
		}
	}
	return 0;
}

struct binfile_hash_entry {
	struct item_id id;
	int flags;
	int data[0];
};

static guint
binfile_hash_entry_hash(gconstpointer key)
{
	const struct binfile_hash_entry *entry=key;
	return (entry->id.id_hi ^ entry->id.id_lo);
}

static gboolean
binfile_hash_entry_equal(gconstpointer a, gconstpointer b)
{
	const struct binfile_hash_entry *entry1=a,*entry2=b;
	return (entry1->id.id_hi==entry2->id.id_hi && entry1->id.id_lo == entry2->id.id_lo);
}

static int *
binfile_item_dup(struct map_priv *m, struct item *item, struct tile *t, int extend)
{
	int size=le32_to_cpu(t->pos[0]);
	struct binfile_hash_entry *entry=g_malloc(sizeof(struct binfile_hash_entry)+(size+1+extend)*sizeof(int));
	void *ret=entry->data;
	entry->id.id_hi=item->id_hi;
	entry->id.id_lo=item->id_lo;
	entry->flags=1;
	dbg(0,"id 0x%x,0x%x\n",entry->id.id_hi,entry->id.id_lo);
	
	memcpy(ret, t->pos, (size+1)*sizeof(int));
	if (!m->changes) 
		m->changes=g_hash_table_new_full(binfile_hash_entry_hash, binfile_hash_entry_equal, g_free, NULL);
	g_hash_table_replace(m->changes, entry, entry);
	dbg(0,"ret %p\n",ret);
	return ret;
}

static int
binfile_coord_set(void *priv_data, struct coord *c, int count, enum change_mode mode)
{
	struct map_rect_priv *mr=priv_data;
	struct tile *t=mr->t,*tn,new;
	int i,delta,move_len;
	int write_offset,move_offset,aoffset,coffset,clen;
	int *data;

	{
		int *i=t->pos,j=0;
		dbg(0,"Before: pos_coord=%d\n",t->pos_coord-i);
		while (i < t->pos_next) 
			dbg(0,"%d:0x%x\n",j++,*i++);
		
	}
	aoffset=t->pos_attr-t->pos_attr_start;
	coffset=t->pos_coord-t->pos_coord_start-2;
	clen=t->pos_attr_start-t->pos_coord+2;
	dbg(0,"coffset=%d clen=%d\n",coffset,clen);
	switch (mode) {
	case change_mode_delete:
		if (count*2 > clen)
			count=clen/2;
		delta=-count*2;
		move_offset=coffset+count*2;
		move_len=t->pos_next-t->pos_coord_start-move_offset;
		write_offset=0;
		break;
	case change_mode_modify:
		write_offset=coffset;
		if (count*2 > clen) {
			delta=count*2-clen;
			move_offset=t->pos_attr_start-t->pos_coord_start;
			move_len=t->pos_next-t->pos_coord_start-move_offset;
		} else {
			move_len=0;
			move_offset=0;
			delta=0;
		}
		break;
	case change_mode_prepend:
		delta=count*2;
		move_offset=coffset-2;
		move_len=t->pos_next-t->pos_coord_start-move_offset;
		write_offset=coffset-2;
		break;
	case change_mode_append:
		delta=count*2;
		move_offset=coffset;
		move_len=t->pos_next-t->pos_coord_start-move_offset;
		write_offset=coffset;
		break;
	default:
		return 0;
	}
	dbg(0,"delta %d\n",delta);
	data=binfile_item_dup(mr->m, &mr->item, t, delta > 0 ? delta:0);
	data[0]=cpu_to_le32(le32_to_cpu(data[0])+delta);
	data[2]=cpu_to_le32(le32_to_cpu(data[2])+delta);
	new.pos=new.start=data;
	new.zipfile_num=t->zipfile_num;
	new.mode=2;
	push_tile(mr, &new);
	setup_pos(mr);
	tn=mr->t;
	tn->pos_coord=tn->pos_coord_start+coffset;
	tn->pos_attr=tn->pos_attr_start+aoffset;
	dbg(0,"moving %d ints from offset %d to %d\n",move_len,tn->pos_coord_start+move_offset-data,tn->pos_coord_start+move_offset+delta-data);
	memmove(tn->pos_coord_start+move_offset+delta, tn->pos_coord_start+move_offset, move_len*4);
	{
		int *i=tn->pos,j=0;
		dbg(0,"After move: pos_coord=%d\n",tn->pos_coord-i);
		while (i < tn->pos_next) 
			dbg(0,"%d:0x%x\n",j++,*i++);
	}
	if (mode != change_mode_append)
		tn->pos_coord+=move_offset;
	if (mode != change_mode_delete) {
		dbg(0,"writing %d ints at offset %d\n",count*2,write_offset+tn->pos_coord_start-data);
		for (i = 0 ; i < count ; i++) {
			tn->pos_coord_start[write_offset++]=c[i].x;
			tn->pos_coord_start[write_offset++]=c[i].y;
		}
			
	}
	{
		int *i=tn->pos,j=0;
		dbg(0,"After: pos_coord=%d\n",tn->pos_coord-i);
		while (i < tn->pos_next) 
			dbg(0,"%d:0x%x\n",j++,*i++);
	}
	return 1;
}

static int
binfile_attr_set(void *priv_data, struct attr *attr, enum change_mode mode)
{
	struct map_rect_priv *mr=priv_data;
	struct tile *t=mr->t,*tn,new;
	int extend,offset,delta,move_len;
	int write_offset,move_offset,naoffset,coffset,oattr_len;
	int nattr_size,nattr_len,pad;
	int *data;

	{
		int *i=t->pos,j=0;
		dbg(0,"Before: pos_attr=%d\n",t->pos_attr-i);
		while (i < t->pos_next) 
			dbg(0,"%d:0x%x\n",j++,*i++);
		
	}

	write_offset=0;	
	naoffset=t->pos_attr-t->pos_attr_start;
	coffset=t->pos_coord-t->pos_coord_start;
	offset=0;
	oattr_len=0;
	if (!naoffset) {
		if (mode == change_mode_delete || mode == change_mode_modify) {
			dbg(0,"no attribute selected\n");
			return 0;
		}
		if (mode == change_mode_append)
			naoffset=t->pos_next-t->pos_attr_start;
	}
	while (offset < naoffset) {
		oattr_len=le32_to_cpu(t->pos_attr_start[offset])+1;
		dbg(0,"len %d\n",oattr_len);
		write_offset=offset;
		offset+=oattr_len;
	}
	move_len=t->pos_next-t->pos_attr_start-offset;
	move_offset=offset;
	switch (mode) {
	case change_mode_delete:
		nattr_size=0;
		nattr_len=0;
		pad=0;
		extend=0;
		break;
	case change_mode_modify:
	case change_mode_prepend:
	case change_mode_append:
		nattr_size=attr_data_size(attr);
		pad=(4-(nattr_size%4))%4;
		nattr_len=(nattr_size+pad)/4+2;
		if (mode == change_mode_prepend) {
			move_offset=write_offset;
			move_len+=oattr_len;
		}
		if (mode == change_mode_append) {
			write_offset=move_offset;
		}
		break;
	default:
		return 0;
	}
	if (mode == change_mode_delete || mode == change_mode_modify) 
		delta=nattr_len-oattr_len;
	else
		delta=nattr_len;
	dbg(0,"delta %d oattr_len %d nattr_len %d\n",delta,oattr_len, nattr_len);
	data=binfile_item_dup(mr->m, &mr->item, t, delta > 0 ? delta:0);
	data[0]=cpu_to_le32(le32_to_cpu(data[0])+delta);
	new.pos=new.start=data;
	new.zipfile_num=t->zipfile_num;
	new.mode=2;
	push_tile(mr, &new);
	setup_pos(mr);
	tn=mr->t;
	tn->pos_coord=tn->pos_coord_start+coffset;
	tn->pos_attr=tn->pos_attr_start+offset;
	dbg(0,"attr start %d offset %d\n",tn->pos_attr_start-data,offset);
	dbg(0,"moving %d ints from offset %d to %d\n",move_len,tn->pos_attr_start+move_offset-data,tn->pos_attr_start+move_offset+delta-data);
	memmove(tn->pos_attr_start+move_offset+delta, tn->pos_attr_start+move_offset, move_len*4);
	if (mode != change_mode_append)
		tn->pos_attr+=delta;
	{
		int *i=tn->pos,j=0;
		dbg(0,"After move: pos_attr=%d\n",tn->pos_attr-i);
		while (i < tn->pos_next) 
			dbg(0,"%d:0x%x\n",j++,*i++);
	}
	if (nattr_len) {
		int *nattr=tn->pos_attr_start+write_offset;
		dbg(0,"writing %d ints at %d\n",nattr_len,nattr-data);
		nattr[0]=cpu_to_le32(nattr_len-1);
		nattr[1]=cpu_to_le32(attr->type);
		memcpy(nattr+2, attr_data_get(attr), nattr_size);
		memset((unsigned char *)(nattr+2)+nattr_size, 0, pad);
	}
	{
		int *i=tn->pos,j=0;
		dbg(0,"After: pos_attr=%d\n",tn->pos_attr-i);
		while (i < tn->pos_next) 
			dbg(0,"%d:0x%x\n",j++,*i++);
	}
	return 1;
}

static struct item_methods methods_binfile = {
        binfile_coord_rewind,
        binfile_coord_get,
        binfile_attr_rewind,
        binfile_attr_get,
	NULL,
        binfile_attr_set,
        binfile_coord_set,
};

static void
push_tile(struct map_rect_priv *mr, struct tile *t)
{
	dbg_assert(mr->tile_depth < 8);
	mr->t=&mr->tiles[mr->tile_depth++];
	*(mr->t)=*t;
	mr->t->pos=mr->t->pos_next=mr->t->start;
}

static int
pop_tile(struct map_rect_priv *mr)
{
	if (mr->tile_depth <= 1)
		return 0;
	if (mr->t->mode < 2)
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
	dbg(1,"cd->zipofst=0x%x\n", binfile_cd_offset(cd));
	t->start=NULL;
	t->mode=1;
	lfh=binfile_read_lfh(f, binfile_cd_offset(cd));
	zipfn=(char *)(file_data_read(f,binfile_cd_offset(cd)+sizeof(struct zip_lfh), lfh->zipfnln));
	strncpy(buffer, zipfn, lfh->zipfnln);
	buffer[lfh->zipfnln]='\0';
	t->start=(int *)binfile_read_content(f, binfile_cd_offset(cd), lfh);
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
	struct zip_cd *cd=(struct zip_cd *)(file_data_read(f, m->eoc->zipeofst + zipfile*m->cde_size, m->cde_size));
	cd_to_cpu(cd);
#if 0
	if (!cd->zipcunc) {
		char tilename[cd->zipcfnl+1];
		char *url;
		long long offset;
		struct file *tile;
		struct zip_lfh *lfh;
		struct zip_cd *cd_copy=g_malloc(m->cde_size);
		struct zip_eoc *eoc,*eoc_copy=g_malloc(sizeof(struct zip_eoc));
		memcpy(cd_copy, cd, m->cde_size);
		file_data_free(f, (unsigned char *)cd);
		cd=NULL;
		strncpy(tilename,(char *)(cd_copy+1),cd_copy->zipcfnl);
		tilename[cd_copy->zipcfnl]='\0';
		url=g_strdup_printf("http://maps.navit-project.org/api/map/?memberid=%d",zipfile);
		offset=file_size(f);
		offset-=sizeof(struct zip_eoc);
		eoc=(struct zip_eoc *)file_data_read(f, offset, sizeof(struct zip_eoc));
		memcpy(eoc_copy, eoc, sizeof(struct zip_eoc));
		file_data_free(f, (unsigned char *)eoc);
		dbg(0,"encountered missing tile %s(%s), downloading at %Ld\n",url,tilename,offset);
		cd_copy->zipofst=offset;
		tile=file_create_url(url);
		for (;;) {
			int size=64*1024,size_ret;
			unsigned char *data;
			data=file_data_read_special(tile, size, &size_ret);
			dbg(1,"got %d bytes\n",size_ret);
			if (size_ret <= 0)
				break;
			file_data_write(f, offset, size_ret, data);
			offset+=size_ret;
		}
		file_data_write(f, offset, sizeof(struct zip_eoc), eoc_copy);
		lfh=(char *)(file_data_read(f,cd_copy->zipofst, sizeof(struct zip_lfh)));
		cd_copy->zipcsiz=lfh->zipsize;
		cd_copy->zipcunc=lfh->zipuncmp;
		file_data_write(f, m->eoc->zipeofst + zipfile*m->cde_size, m->cde_size, cd_copy);
		g_free(url);
		g_free(cd_copy);
		cd=(struct zip_cd *)(file_data_read(f, m->eoc->zipeofst + zipfile*m->cde_size, m->cde_size));
		dbg(0,"Offset %d\n",cd->zipofst);
	}
#endif
	dbg(1,"enter %p %d\n", mr, zipfile);
#ifdef DEBUG_SIZE
	mr->size+=cd->zipcunc;
#endif
	t.zipfile_num=zipfile;
	if (zipfile_to_tile(f, cd, &t))
		push_tile(mr, &t);
	file_data_free(f, (unsigned char *)cd);
}

static struct map_rect_priv *
map_rect_new_binfile(struct map_priv *map, struct map_selection *sel)
{
	struct map_rect_priv *mr;
	struct tile t={};

	binfile_check_version(map);
	dbg(1,"map_rect_new_binfile\n");
	if (!map->fi)
		return NULL;
	mr=g_new0(struct map_rect_priv, 1);
	mr->m=map;
	mr->sel=sel;
	mr->item.id_hi=0;
	mr->item.id_lo=0;
	dbg(1,"zip_members=%d\n", map->zip_members);
	if (map->eoc) 
		push_zipfile_tile(mr, map->zip_members-1);
	else {
		unsigned char *d;
		d=file_data_read(map->fi, 0, map->fi->size);
		t.start=(int *)d;
		t.end=(int *)(d+map->fi->size);
		t.zipfile_num=0;
		t.mode=0;
		push_tile(mr, &t);
	}
	mr->item.meth=&methods_binfile;
	mr->item.priv_data=mr;
	return mr;
}

static void
write_changes_do(gpointer key, gpointer value, gpointer user_data)
{
	struct binfile_hash_entry *entry=key;
	FILE *out=user_data;
	if (entry->flags) {
		entry->flags=0;
		fwrite(entry, sizeof(*entry)+(le32_to_cpu(entry->data[0])+1)*4, 1, out);
		dbg(0,"yes\n");
	}
}

static void
write_changes(struct map_priv *m)
{
	FILE *changes;
	char *changes_file;
	if (!m->changes)
		return;
	changes_file=g_strdup_printf("%s.log",m->filename);
	changes=fopen(changes_file,"ab");
	g_hash_table_foreach(m->changes, write_changes_do, changes);
	fclose(changes);
	g_free(changes_file);
}

static void
load_changes(struct map_priv *m)
{
	FILE *changes;
	char *changes_file;
	struct binfile_hash_entry entry,*e;
	int size;
	changes_file=g_strdup_printf("%s.log",m->filename);
	changes=fopen(changes_file,"rb");
	if (! changes)
		return;
	m->changes=g_hash_table_new_full(binfile_hash_entry_hash, binfile_hash_entry_equal, g_free, NULL);
	while (fread(&entry, sizeof(entry), 1, changes) == 1) {
		if (fread(&size, sizeof(size), 1, changes) != 1)
			break;
		e=g_malloc(sizeof(struct binfile_hash_entry)+(le32_to_cpu(size)+1)*4);
		*e=entry;
		e->data[0]=size;
		if (fread(e->data+1, le32_to_cpu(size)*4, 1, changes) != 1)
			break;
		g_hash_table_replace(m->changes, e, e);
	}
	fclose(changes);
	g_free(changes_file);
}


static void
map_rect_destroy_binfile(struct map_rect_priv *mr)
{
	write_changes(mr->m);
	while (pop_tile(mr));
#ifdef DEBUG_SIZE
	dbg(0,"size=%d kb\n",mr->size/1024);
#endif
	file_data_free(mr->m->fi, (unsigned char *)(mr->tiles[0].start));
	g_free(mr->url);
        g_free(mr);
}

static void
setup_pos(struct map_rect_priv *mr)
{
	int size,coord_size;
	struct tile *t=mr->t;
	size=le32_to_cpu(t->pos[0]);
	if (size > 1024*1024 || size < 0) {
		dbg(0,"size=0x%x\n", size);
#if 0
		fprintf(stderr,"offset=%d\n", (unsigned char *)(mr->pos)-mr->m->f->begin);
#endif
		dbg(0,"size error");
	}
	t->pos_next=t->pos+size+1;
	mr->item.type=le32_to_cpu(t->pos[1]);
	coord_size=le32_to_cpu(t->pos[2]);
	t->pos_coord_start=t->pos+3;
	t->pos_attr_start=t->pos_coord_start+coord_size;
}

static int
selection_contains(struct map_selection *sel, struct coord_rect *r, struct range *mima)
{
	int order;
	if (! sel)
		return 1;
	while (sel) {
		if (coord_rect_overlap(r, &sel->u.c_rect)) {
			order=sel->order;
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

static void
map_parse_submap(struct map_rect_priv *mr)
{
	struct coord_rect r;
	struct coord c[2];
	struct attr at;
	if (binfile_coord_get(mr->item.priv_data, c, 2) != 2)
		return;
	r.lu.x=c[0].x;
	r.lu.y=c[1].y;
	r.rl.x=c[1].x;
	r.rl.y=c[0].y;
	if (!binfile_attr_get(mr->item.priv_data, attr_order, &at))
		return;
	if (!mr->m->eoc || !selection_contains(mr->sel, &r, &at.u.range))
		return;
	if (!binfile_attr_get(mr->item.priv_data, attr_zipfile_ref, &at))
		return;
	dbg(1,"pushing zipfile %d from %d\n", at.u.num, mr->t->zipfile_num);
	push_zipfile_tile(mr, at.u.num);
}

static int
push_modified_item(struct map_rect_priv *mr)
{
	struct item_id id;
	struct binfile_hash_entry *entry;
	id.id_hi=mr->item.id_hi;
	id.id_lo=mr->item.id_lo;
	entry=g_hash_table_lookup(mr->m->changes, &id);
	if (entry) {
		struct tile tn;
		tn.pos_next=tn.pos=tn.start=entry->data;
		tn.zipfile_num=mr->item.id_hi;
		tn.mode=2;
		tn.end=tn.start+le32_to_cpu(entry->data[0])+1;
		push_tile(mr, &tn);
		return 1;
	}
	return 0;
}

static struct item *
map_rect_get_item_binfile(struct map_rect_priv *mr)
{
	struct tile *t;
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
		setup_pos(mr);
		binfile_coord_rewind(mr);
		binfile_attr_rewind(mr);
		if ((mr->item.type == type_submap) && (!mr->country_id)) {
			map_parse_submap(mr);
			continue;
		}
		if (t->mode != 2) {
			mr->item.id_hi=t->zipfile_num;
			mr->item.id_lo=t->pos-t->start;
			if (mr->m->changes && push_modified_item(mr))
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
	if (mr->m->eoc) {
		while (pop_tile(mr));
		push_zipfile_tile(mr, id_hi);
	}
	t=mr->t;
	t->pos=t->start+id_lo;
	mr->item.id_hi=id_hi;
	mr->item.id_lo=id_lo;
	if (mr->m->changes)
		push_modified_item(mr);
	setup_pos(mr);
	binfile_coord_rewind(mr);
	binfile_attr_rewind(mr);
	return &mr->item;
}

static struct map_search_priv *
binmap_search_new(struct map_priv *map, struct item *item, struct attr *search, int partial)
{
	struct map_rect_priv *map_rec;
	struct map_search_priv *msp;
	struct item *town;
	
	/*
     * NOTE: If you implement search for other attributes than attr_town_name and attr_street_name,
     * please update this comment and the documentation for map_search_new() in map.c
     */
	switch (search->type) {
		case attr_country_name:
			break;
		case attr_town_name:
		case attr_town_or_district_name:
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
			map_rec = map_rect_new_binfile(map, NULL);
			town = map_rect_get_item_byid_binfile(map_rec, item->id_hi, item->id_lo);
			if (town) {
				struct map_search_priv *msp = g_new(struct map_search_priv, 1);
				struct coord c;
				struct map_rect_priv *map_rec2;
				struct attr town_name, poly_town_name;
				struct item *place;
				int place_found=0;
				item_coord_get(town, &c, 1);
				if (item_attr_get(town, attr_label, &town_name)) {
					msp->ms.range = item_range_all;
					msp->ms.order = 18;
					msp->ms.next = NULL;
					msp->ms.u.c_rect.lu=c;
					msp->ms.u.c_rect.rl=c;
					map_rec2=map_rect_new_binfile(map, &msp->ms);
					while ((place=map_rect_get_item_binfile(map_rec2))) {
						if (item_is_poly_place(*place) &&
						    item_attr_get(place, attr_label, &poly_town_name) && 
						    !strcmp(poly_town_name.u.str,town_name.u.str)) {
							struct coord c[128];
							int i,count;
							place_found=1;
							while ((count=item_coord_get(place, c, 128))) {
								for (i = 0 ; i < count ; i++)
									coord_rect_extend(&msp->ms.u.c_rect, &c[i]);
							}
						}
					}
					map_rect_destroy_binfile(map_rec2);
				}
				if (!place_found) {
					int size = 10000;
					switch (town->type) {
						case type_town_label_1e7:
						case type_town_label_5e6:
						case type_town_label_2e6:
						case type_town_label_1e6:
						case type_town_label_5e5:
						case type_town_label_2e5:
							size = 10000;
							break;
						case type_town_label_1e5:
						case type_town_label_5e4:
						case type_town_label_2e4:
							size = 5000;
							break;
						case type_town_label_1e4:
						case type_town_label_5e3:
						case type_town_label_2e3:
							size = 2500;
							break;
						case type_town_label_1e3:
						case type_town_label_5e2:
						case type_town_label_2e2:
						case type_town_label_1e2:
						case type_town_label_5e1:
						case type_town_label_2e1:
						case type_town_label_1e1:
						case type_town_label_5e0:
						case type_town_label_2e0:
						case type_town_label_1e0:
						case type_town_label_0e0:
							size = 1000;
							break;
						default:
							break;
					}
					msp->ms.u.c_rect.lu.x = c.x-size;
					msp->ms.u.c_rect.lu.y = c.y+size;
					msp->ms.u.c_rect.rl.x = c.x+size;
					msp->ms.u.c_rect.rl.y = c.y-size;
				}
				map_rect_destroy_binfile(map_rec);
				map_rec = map_rect_new_binfile(map, &msp->ms);
				msp->mr = map_rec;
				msp->search = search;
				msp->partial = partial;
				msp->search_results = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
				return msp;
			}
			map_rect_destroy_binfile(map_rec);
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
		switch (map_search->search->type) {
		case attr_town_name:
		case attr_district_name:
		case attr_town_or_district_name:
			if (item_is_town(*it) && map_search->search->type != attr_district_name) {
				struct attr at;
				if (binfile_attr_get(it->priv_data, attr_town_name_match, &at) || binfile_attr_get(it->priv_data, attr_town_name, &at)) {
					if (!ascii_cmp(at.u.str, map_search->search->u.str, map_search->partial)) {
						return it;
					}
				}
			}
			if (item_is_district(*it) && map_search->search->type != attr_town_name) {
				struct attr at;
				if (binfile_attr_get(it->priv_data, attr_district_name_match, &at) || binfile_attr_get(it->priv_data, attr_district_name, &at)) {
					if (!ascii_cmp(at.u.str, map_search->search->u.str, map_search->partial)) {
						return it;
					}
				}
			}
			break;
		case attr_street_name:
			if ((it->type == type_street_3_city) || (it->type == type_street_2_city) || (it->type == type_street_1_city)) {
				struct attr at;
				if (map_selection_contains_item_rect(map_search->mr->sel, it) && binfile_attr_get(it->priv_data, attr_label, &at)) {
					int i,match=0;
					char *str=g_strdup(at.u.str);
					char *word=str;
					do {
						for (i = 0 ; i < 3 ; i++) {
							char *name=linguistics_expand_special(word,i);
							if (name && !ascii_cmp(name, map_search->search->u.str, map_search->partial))
								match=1;
							g_free(name);
							if (match)
								break;
						}
						if (match)
							break;
						word=linguistics_next_word(word);
					} while (word);
					g_free(str);
					if (match && !g_hash_table_lookup(map_search->search_results, at.u.str)) {
						item_coord_rewind(it);
						item_attr_rewind(it);
						g_hash_table_insert(map_search->search_results, g_strdup(at.u.str), "");
						return it;
					}
				}
			}
			break;
		default:
			return NULL;
		}
	}
	return NULL;
}

static void
binmap_search_destroy(struct map_search_priv *ms)
{
	g_hash_table_destroy(ms->search_results);
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
	int len;
	int cde_index_size;
	int offset;
	struct zip_cd *cd;

	len = strlen("index");
	cde_index_size = sizeof(struct zip_cd)+len;
	offset = m->eoc->zipecsz-cde_index_size;
	cd = binfile_read_cd(m, offset, len);

	if (!cd) {
		cde_index_size+=sizeof(struct zip_cd_ext);
		offset = m->eoc->zipecsz-cde_index_size;
		cd = binfile_read_cd(m, offset, len+sizeof(struct zip_cd_ext));
	}
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
	cd=binfile_read_cd(m, offset, -1);
	if (!cd)
		return 0;
	m->index_offset=offset;
	m->index_cd=cd;
	return 1;
}

static int
map_binfile_open(struct map_priv *m)
{
	int *magic;
	struct zip_cd *first_cd;
	struct map_rect_priv *mr;
	struct item *item;
	struct attr attr;

	dbg(1,"file_create %s\n", m->filename);
	m->fi=file_create(m->filename, 0);
	if (! m->fi) {
		dbg(0,"Failed to load '%s'\n", m->filename);
		return 0;
	}
	if (m->check_version)
		m->version=file_version(m->fi, m->check_version);
	magic=(int *)file_data_read(m->fi, 0, 4);
	if (!magic) {
		file_destroy(m->fi);
		m->fi=NULL;
		return 0;
	}
	*magic = le32_to_cpu(*magic);
	if (*magic == zip_lfh_sig) {
		if ((m->eoc=binfile_read_eoc(m->fi)) && binfile_get_index(m) && (first_cd=binfile_read_cd(m, 0, 0))) {
			m->cde_size=sizeof(struct zip_cd)+first_cd->zipcfnl+first_cd->zipcxtl;
			m->zip_members=m->index_offset/m->cde_size+1;
			dbg(1,"cde_size %d\n", m->cde_size);
			dbg(1,"members %d\n",m->zip_members);
			file_data_free(m->fi, (unsigned char *)first_cd);
		} else {
			dbg(0,"invalid file format for '%s'\n", m->filename);
			file_destroy(m->fi);
			m->fi=NULL;
			return 0;
		}
	} else
		file_mmap(m->fi);
	file_data_free(m->fi, (unsigned char *)magic);
	m->cachedir=g_strdup("/tmp/navit");
	m->map_version=0;
	mr=map_rect_new_binfile(m, NULL);
	if (mr) {
		item=map_rect_get_item_binfile(mr);
		if (item && item->type == type_map_information) 
			if (binfile_attr_get(item->priv_data, attr_version, &attr))
				m->map_version=attr.u.num;
		map_rect_destroy_binfile(mr);
		if (m->map_version >= 16) {
			dbg(0,"Warning: This map is incompatible with your navit version. Please update navit.\n");
			return 0;
		}
	}
	return 1;
}

static void
map_binfile_close(struct map_priv *m)
{
	file_data_free(m->fi, (unsigned char *)m->index_cd);
	file_data_free(m->fi, (unsigned char *)m->eoc);
	g_free(m->cachedir);
	file_destroy(m->fi);
}

static void
map_binfile_destroy(struct map_priv *m)
{
	g_free(m->filename);
	g_free(m);
}


static void
binfile_check_version(struct map_priv *m)
{
	int version=-1;
	if (!m->check_version)
		return;
	if (m->fi) 
		version=file_version(m->fi, m->check_version);
	if (version != m->version) {
		if (m->fi)
			map_binfile_close(m);
		map_binfile_open(m);
	}
}


static struct map_priv *
map_new_binfile(struct map_methods *meth, struct attr **attrs)
{
	struct map_priv *m;
	struct attr *data=attr_search(attrs, NULL, attr_data);
	struct attr *check_version;
	struct file_wordexp *wexp;
	char **wexp_data;
	if (! data)
		return NULL;

	wexp=file_wordexp_new(data->u.str);
	wexp_data=file_wordexp_get_array(wexp);
	dbg(1,"map_new_binfile %s\n", data->u.str);	
	*meth=map_methods_binfile;

	m=g_new0(struct map_priv, 1);
	m->id=++map_id;
	m->filename=g_strdup(wexp_data[0]);
	file_wordexp_destroy(wexp);
	check_version=attr_search(attrs, NULL, attr_check_version);
	if (check_version) 
		m->check_version=check_version->u.num;
	if (!map_binfile_open(m) && !m->check_version) {
		map_binfile_destroy(m);
		m=NULL;
	} else {
		load_changes(m);
	}
	return m;
}

void
plugin_init(void)
{
	dbg(1,"binfile: plugin_init\n");
	plugin_register_map_type("binfile", map_new_binfile);
}

