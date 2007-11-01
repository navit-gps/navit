#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
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
};

struct map_priv {
	int id;
	char *filename;
	struct file *fi;
	struct zip_cd *index_cd;
	int cde_size;
	struct zip_eoc *eoc;
};

struct map_rect_priv {
	int *start;
	int *end;
	enum attr_type attr_last;
        struct map_selection *sel;
        struct map_priv *m;
        struct item item;
	int tile_depth;
	struct tile tiles[8];
	struct tile *t;
};


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
	dbg(1,"binfile_coord_get %d\n",count);
	while (count--) {
		dbg(1,"%p vs %p\n", t->pos_coord, t->pos_attr_start);
		if (t->pos_coord >= t->pos_attr_start)
			break;
		c->x=*(t->pos_coord++);
		c->y=*(t->pos_coord++);
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

static int
binfile_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{	
	struct map_rect_priv *mr=priv_data;
	struct tile *t=mr->t;
	enum attr_type type;
	int size;

	if (attr_type != mr->attr_last) {
		t->pos_attr=t->pos_attr_start;
		mr->attr_last=attr_type;
	}
	while (t->pos_attr < t->pos_next) {
		size=*(t->pos_attr++);
		type=t->pos_attr[0];
		if (type == attr_type || attr_type == attr_any) {
			if (attr_type == attr_any) {
				dbg(0,"pos %p attr %s size %d\n", t->pos_attr-1, attr_to_name(type), size);
			}
			attr->type=type;
			attr_data_set(attr, t->pos_attr+1);
			t->pos_attr+=size;
			return 1;
		} else {
			t->pos_attr+=size;
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
	mr->t=&mr->tiles[mr->tile_depth++];
	*(mr->t)=*t;
	mr->t->pos=mr->t->pos_next=mr->t->start;
}

static int
pop_tile(struct map_rect_priv *mr)
{
	if (mr->tile_depth <= 1)
		return 0;
	mr->t=&mr->tiles[--mr->tile_depth-1];
	return 1;
}


static void
zipfile_to_tile(struct file *f, struct zip_cd *cd, struct tile *t)
{
	char buffer[1024];
	struct zip_lfh *lfh;
	lfh=(struct zip_lfh *)(f->begin+cd->zipofst);
	strncpy(buffer, lfh->zipname, lfh->zipfnln);
	buffer[lfh->zipfnln]='\0';
	dbg(0,"0x%x '%s' %d\n", lfh->ziplocsig, buffer, sizeof(*cd)+cd->zipcfnl);
	t->start=(int *)(f->begin+cd->zipofst+sizeof(struct zip_lfh)+lfh->zipfnln);
	t->end=t->start+lfh->zipuncmp/4;
}

static void
push_zipfile_tile(struct map_rect_priv *mr, int zipfile)
{
        struct map_priv *m=mr->m;
	struct file *f=m->fi;
	struct tile t;
	struct zip_cd *cd=(struct zip_cd *)(f->begin + m->eoc->zipeofst + zipfile*m->cde_size);
	t.zipfile_num=zipfile;
	zipfile_to_tile(f, cd, &t);
	push_tile(mr, &t);
}

static struct map_rect_priv *
map_rect_new_binfile(struct map_priv *map, struct map_selection *sel)
{
	struct map_rect_priv *mr;
	struct tile *t;

	dbg(1,"map_rect_new_binfile\n");
	mr=g_new0(struct map_rect_priv, 1);
	mr->m=map;
	mr->sel=sel;
	mr->item.id_hi=0;
	mr->item.id_lo=0;
	if (map->eoc) 
		push_zipfile_tile(mr, map->eoc->zipecenn-1);
	else {
		t->start=(int *)(map->fi->begin);
		t->end=(int *)(map->fi->end);
		t->zipfile_num=0;
		push_tile(mr, t);
	}
	mr->item.meth=&methods_binfile;
	mr->item.priv_data=mr;
	return mr;
}


static void
map_rect_destroy_binfile(struct map_rect_priv *mr)
{
        g_free(mr);
}

static void
setup_pos(struct map_rect_priv *mr)
{
	int size,coord_size;
	struct tile *t=mr->t;
	size=*(t->pos++);
	if (size > 1024*1024 || size < 0) {
		fprintf(stderr,"size=0x%x\n", size);
#if 0
		fprintf(stderr,"offset=%d\n", (unsigned char *)(mr->pos)-mr->m->f->begin);
#endif
		g_error("size error");
	}
	t->pos_next=t->pos+size;
	mr->item.type=*(t->pos++);
	coord_size=*(t->pos++);
	t->pos_coord_start=t->pos_coord=t->pos;
	t->pos_attr_start=t->pos_attr=t->pos_coord+coord_size;
}

static int
selection_contains(struct map_selection *sel, struct coord_rect *r)
{
	if (! sel)
		return 1;
	while (sel) {
		if (coord_rect_overlap(r, &sel->rect))
			return 1;
		sel=sel->next;
	}
	return 0;
}



static struct item *
map_rect_get_item_binfile(struct map_rect_priv *mr)
{
	struct tile *t;
	for (;;) {
		t=mr->t;
		t->pos=t->pos_next;
		if (t->pos >= t->end) {
			if (pop_tile(mr))
				continue;
			return NULL;
		}
		mr->item.id_hi=t->zipfile_num;
		mr->item.id_lo=t->pos-t->start;
		setup_pos(mr);
		if (mr->item.type == type_submap) {
			struct coord_rect r;
			r.lu.x=t->pos_coord[0];
			r.lu.y=t->pos_coord[3];
			r.rl.x=t->pos_coord[2];
			r.rl.y=t->pos_coord[1];
			if (!mr->m->eoc || !selection_contains(mr->sel, &r)) {
				continue;
			}
			dbg(1,"0x%x\n", t->pos_attr[5]);
			push_zipfile_tile(mr, t->pos_attr[5]);
			continue;
				
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
	setup_pos(mr);
	return &mr->item;
}

static struct map_methods map_methods_binfile = {
	projection_mg,
	"utf-8",
	map_destroy_binfile,
	map_rect_new_binfile,
	map_rect_destroy_binfile,
	map_rect_get_item_binfile,
	map_rect_get_item_byid_binfile,
};

static struct map_priv *
map_new_binfile(struct map_methods *meth, struct attr **attrs)
{
	struct map_priv *m;
	struct attr *data=attr_search(attrs, NULL, attr_data);
	struct file_wordexp *wexp;
	char **wexp_data;
	int *magic,cde_index_size;
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
	file_wordexp_destroy(wexp);
	magic=(int *)(m->fi->begin);
	if (*magic == 0x04034b50) {
		cde_index_size=sizeof(struct zip_cd)+sizeof("index")-1;
		m->eoc=(struct zip_eoc *)(m->fi->end-sizeof(struct zip_eoc));
		m->index_cd=(struct zip_cd *)((char *)m->eoc-cde_index_size);
		printf("length %d\n", m->eoc->zipecsz);
		printf("entries %d\n", m->eoc->zipecenn);
		m->cde_size=(m->eoc->zipecsz-cde_index_size)/(m->eoc->zipecenn-1);
		printf("cde_size %d\n", m->cde_size);
		printf("length %d\n",m->cde_size*(m->eoc->zipecenn-1)+cde_index_size);
		printf("0x%x\n", m->eoc->zipesig);
		printf("0x%x\n", m->index_cd->zipcensig);
	}
	return m;
}

void
plugin_init(void)
{
	dbg(1,"binfile: plugin_init\n");
	plugin_register_map_type("binfile", map_new_binfile);
}

