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

static int map_id;

struct map_priv {
	int id;
	char *filename;
	struct file *f;
};

struct map_rect_priv {
	int *start;
	int *pos;
	int *pos_coord_start;
	int *pos_coord;
	int *pos_attr_start;
	int *pos_attr;
	int *pos_next;
	int *end;
	enum attr_type attr_last;
        struct map_selection *sel;
        struct map_priv *m;
        struct item item;
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
	mr->pos_coord=mr->pos_coord_start;
}

static int
binfile_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *mr=priv_data;
	int ret=0;
	dbg(1,"binfile_coord_get %d\n",count);
	while (count--) {
		dbg(1,"%p vs %p\n", mr->pos_coord, mr->pos_attr_start);
		if (mr->pos_coord >= mr->pos_attr_start)
			break;
		c->x=*(mr->pos_coord++);
		c->y=*(mr->pos_coord++);
		c++;
		ret++;
	}
	return ret;
}

static void
binfile_attr_rewind(void *priv_data)
{
	struct map_rect_priv *mr=priv_data;
	mr->pos_attr=mr->pos_attr_start;
	
}

static int
binfile_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{	
	struct map_rect_priv *mr=priv_data;
	enum attr_type type;
	int size;

	if (attr_type != mr->attr_last) {
		mr->pos_attr=mr->pos_attr_start;
		mr->attr_last=attr_type;
	}
	while (mr->pos_attr < mr->pos_next) {
		size=*(mr->pos_attr++);
		type=mr->pos_attr[0];
		if (type == attr_type || attr_type == attr_any) {
			if (attr_type == attr_any) {
				dbg(0,"pos %p attr %s size %d\n", mr->pos_attr-1, attr_to_name(type), size);
			}
			attr->type=type;
			attr_data_set(attr, mr->pos_attr+1);
			mr->pos_attr+=size;
			return 1;
		} else {
			mr->pos_attr+=size;
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

static struct map_rect_priv *
map_rect_new_binfile(struct map_priv *map, struct map_selection *sel)
{
	struct map_rect_priv *mr;

	dbg(1,"map_rect_new_binfile\n");
	mr=g_new0(struct map_rect_priv, 1);
	mr->m=map;
	mr->sel=sel;
	mr->start=mr->pos=mr->pos_next=map->f->begin;
	mr->end=map->f->end;
	mr->item.id_hi=0;
	mr->item.id_lo=0;
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
	size=*(mr->pos++);
	mr->pos_next=mr->pos+size;
	mr->item.type=*(mr->pos++);
	coord_size=*(mr->pos++);
	mr->pos_coord_start=mr->pos_coord=mr->pos;
	mr->pos_attr_start=mr->pos_attr=mr->pos_coord+coord_size;
}



static struct item *
map_rect_get_item_binfile(struct map_rect_priv *mr)
{
	mr->pos=mr->pos_next;
	if (mr->pos >= mr->end)
		return NULL;
	mr->item.id_hi=0;
	mr->item.id_lo=mr->pos-mr->start;
	setup_pos(mr);
	return &mr->item;
}

static struct item *
map_rect_get_item_byid_binfile(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	mr->pos=mr->start+id_lo;
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
	m->f=file_create(m->filename);
	file_wordexp_destroy(wexp);
	return m;
}

void
plugin_init(void)
{
	dbg(1,"binfile: plugin_init\n");
	plugin_register_map_type("binfile", map_new_binfile);
}

