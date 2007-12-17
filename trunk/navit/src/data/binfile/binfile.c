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
	int zip_members;
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
	dbg(2,"binfile_coord_get %d\n",count);
	while (count--) {
		dbg(2,"%p vs %p\n", t->pos_coord, t->pos_attr_start);
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
		if (type == attr_street_name || type == attr_street_name_systematic)
			type = attr_label;
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
	lfh=(struct zip_lfh *)(file_data_read(f,cd->zipofst,sizeof(struct zip_lfh)));
	zipfn=(char *)(file_data_read(f,cd->zipofst+sizeof(struct zip_lfh), lfh->zipfnln));
	strncpy(buffer, zipfn, lfh->zipfnln);
	buffer[lfh->zipfnln]='\0';
	dbg(0,"0x%x '%s' %d %d,%d\n", lfh->ziplocsig, buffer, sizeof(*cd)+cd->zipcfnl, lfh->zipsize, lfh->zipuncmp);
	switch (lfh->zipmthd) {
	case 0:
		t->start=(int *)(file_data_read(f,cd->zipofst+sizeof(struct zip_lfh)+lfh->zipfnln, lfh->zipuncmp));
		t->end=t->start+lfh->zipuncmp/4;
		break;
	case 8:
		t->start=(int *)(file_data_read_compressed(f,cd->zipofst+sizeof(struct zip_lfh)+lfh->zipfnln, lfh->zipsize, lfh->zipuncmp));
		t->end=t->start+lfh->zipuncmp/4;
		break;
	default:
		dbg(0,"Unknown compression method %d\n", lfh->zipmthd);
	}
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
		if (coord_rect_overlap(r, &sel->u.c_rect))
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
			dbg(1,"pushing zipfile %d from %d\n", t->pos_attr[5], t->zipfile_num);
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
	struct zip_cd *first_cd;
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
	if (! m->fi) {
		dbg(0,"Failed to load %s\n", m->filename);
		g_free(m);
		return NULL;
	}
	file_wordexp_destroy(wexp);
	magic=(int *)file_data_read(m->fi, 0, 4);
	if (*magic == 0x04034b50) {
		cde_index_size=sizeof(struct zip_cd)+sizeof("index")-1;
		m->eoc=(struct zip_eoc *)file_data_read(m->fi,m->fi->size-sizeof(struct zip_eoc), sizeof(struct zip_eoc));
		printf("magic 0x%x\n", m->eoc->zipesig);
		m->index_cd=(struct zip_cd *)file_data_read(m->fi,m->fi->size-sizeof(struct zip_eoc)-cde_index_size, cde_index_size);
		first_cd=(struct zip_cd *)file_data_read(m->fi,m->eoc->zipeofst, sizeof(struct zip_cd));
		m->cde_size=sizeof(struct zip_cd)+first_cd->zipcfnl;
		m->zip_members=(m->eoc->zipecsz-cde_index_size)/m->cde_size+1;
		printf("cde_size %d\n", m->cde_size);
		printf("members %d\n",m->zip_members);
		printf("0x%x\n", m->eoc->zipesig);
		printf("0x%x\n", m->index_cd->zipcensig);
		file_data_free(m->fi, (unsigned char *)first_cd);
	} else 
		file_mmap(m->fi);
	file_data_free(m->fi, (unsigned char *)magic);
	return m;
}

void
plugin_init(void)
{
	dbg(1,"binfile: plugin_init\n");
	plugin_register_map_type("binfile", map_new_binfile);
}

