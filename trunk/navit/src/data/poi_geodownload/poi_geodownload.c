#include <mdbtools.h>
#include "config.h"
#include "debug.h"
#include "coord.h"
#include "projection.h"
#include "map.h"
#include "item.h"
#include "plugin.h"


#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

struct map_priv
{
	char *filename;
	MdbHandle *h;
	MdbHandle *h_idx;
	MdbTableDef *table;
	GPtrArray *table_col;
	MdbColumn **cols;
	MdbIndex *idx;
	int idx_size;
	enum item_type type;
	int name_col;
};

struct map_rect_priv
{
	struct item item;
	struct map_priv *m;
	enum attr_type attr_next;
	int cidx;
	char buffer[4096];
};

#if 0
struct index_data {
	unsigned char data[15];
};
struct poi {
	char filename[1024];
	char icon[1024];
	long pos;
	MdbHandle *h;
	MdbHandle *h_idx;
	MdbTableDef *table;
	GPtrArray *table_col;
	MdbColumn **cols;
	MdbIndex *idx;
	int idx_size;
	struct index_data index_data;
	MdbIndexChain chain;
	struct poi *next;
} *poi_list;

struct poi_data {
	struct poi *poi;
	int page;
	int row;
};

	char poipath[256];
	char poibmp[256];


#endif

static void
print_col(MdbHandle *h, MdbColumn *col, char *buffer, int hex)
{
	char *s;
	dbg(1,"type=%d\n", col->col_type);
	switch (col->col_type) {
	case MDB_BOOL:
		strcpy(buffer, mdb_pg_get_byte(h, col->cur_value_start) ? "True" : "False");
		break;
	case MDB_BYTE:
		sprintf(buffer, "%d", mdb_pg_get_byte(h, col->cur_value_start));
		break;
	case MDB_LONGINT:
		if (hex)
			sprintf(buffer, "0x%lx", mdb_pg_get_int32(h, col->cur_value_start));
		else
			sprintf(buffer, "%ld", mdb_pg_get_int32(h, col->cur_value_start));
		break;
	case MDB_DOUBLE:
		sprintf(buffer, "%f", mdb_pg_get_double(h, col->cur_value_start));
		break;
	case MDB_TEXT:
		dbg(1,"pg_buf %p start %d len %d\n", h->pg_buf, col->cur_value_start, col->cur_value_len);
		if (col->cur_value_len) {
			s=mdb_col_to_string (h, h->pg_buf, col->cur_value_start, col->col_type, col->cur_value_len);
			dbg(1,"s=%p\n", s);
			sprintf(buffer, "%s", s);
		}
		break;
	default:
		sprintf(buffer, "unknown (%d)", col->col_type);
	}
}

#if 0

static void
setup_idx_data(struct index_data *idx, struct coord *c, unsigned int geoflags, int size)
{
	/* 7f 80 1c 91 0a 7f 80 5c  f5 41 7f 80 00 00 05 */
	idx->data[0]=0x7f;
	idx->data[1]=(c->x >> 24) ^ 0x80;
	idx->data[2]=c->x >> 16;
	idx->data[3]=c->x >> 8;
	idx->data[4]=c->x;
	idx->data[5]=0x7f;
	idx->data[6]=(c->y >> 24) ^ 0x80;
	idx->data[7]=c->y >> 16;
	idx->data[8]=c->y >> 8;
	idx->data[9]=c->y;
	idx->data[10]=0x7f;
	if (size > 12) {
		idx->data[11]=0x80 | (geoflags >> 24);
		idx->data[12]=geoflags >> 16;
		idx->data[13]=geoflags >> 8;
		idx->data[14]=geoflags;
	} else {
		idx->data[11]=geoflags;
	}
}

static void
setup_idx_rect(struct coord *rect, struct index_data *idx, int size)
{
	struct coord r[2];
	r[0].x=rect[0].x;
	r[0].y=rect[1].y;
	r[1].x=rect[1].x;
	r[1].y=rect[0].y;
#if 0
	printf("low 0x%x 0%x\n", r[0].x, r[0].y);
	printf("high 0x%x 0%x\n", r[1].x, r[1].y);
#endif
	setup_idx_data(idx, r, 0, size);
	setup_idx_data(idx+1, r+1, 0xffffffff, size);
}

#endif

static int
load_row(struct map_priv *poi, int pg, int row)
{
	int row_start, row_end, offset;
	unsigned int num_fields, i;
	MdbField fields[256];
	MdbFormatConstants *fmt;
	int debug=0;

	fmt=poi->h->fmt;
	mdb_read_pg(poi->h, pg);
	dbg(1, "enter poi=%p pg=%d row=%d\n", poi, pg, row);
	dbg(1,"Page Type %d row_count_offset %d\n",poi->h->pg_buf[0], fmt->row_count_offset);
	for (i = 0; i <= row; i++) {
		offset=(fmt->row_count_offset + 2) + i * 2;
		dbg(1,"row %d %d 0x%x\n", i, offset, mdb_pg_get_int16(poi->h, offset));
	}
	row_start = mdb_pg_get_int16(poi->h, (fmt->row_count_offset + 2) + row * 2);
	if (row_start & 0x4000)
		return 1;
	row_end = mdb_find_end_of_row(poi->h, row);
	if (debug) {
		printf("start=0x%x end=0x%x\n", row_start, row_end);
		buffer_dump(poi->h->pg_buf, row_start, row_end);
	}

	poi->h->cur_pos=row_start & 0x1fff;
	poi->table->cur_row=row;
	num_fields = mdb_crack_row(poi->table, row_start & 0x1fff, row_end, fields);
	dbg(1,"num_fields=%d\n", num_fields);
	for (i = 0; i < num_fields; i++) {
		dbg(1,"i=%d/%d\n", i, num_fields);
		poi->cols[i]->cur_value_start=fields[i].start;
		poi->cols[i]->cur_value_len=fields[i].siz;
	}
	return 0;
}

#if 0

static MdbIndexPage *
index_next_row(MdbHandle *mdb, MdbIndex *idx, MdbIndexChain *chain)
{
	MdbIndexPage *ipg;

	ipg = mdb_index_read_bottom_pg(mdb, idx, chain);
	if (!mdb_index_find_next_on_page(mdb, ipg)) {
#if 0
		printf("no next\n");	
#endif
		if (!chain->clean_up_mode) {
#if 0
			printf("no cleanup\n");	
#endif
			if (!(ipg = mdb_index_unwind(mdb, idx, chain)))
				chain->clean_up_mode = 1;
		}
		if (chain->clean_up_mode) {
#if 0
			printf("cleanup\n");	
#endif
			//fprintf(stdout,"in cleanup mode\n");

			if (!chain->last_leaf_found) {
				printf("no last_leaf_found\n");
				return NULL;
			}
			mdb_read_pg(mdb, chain->last_leaf_found);
			chain->last_leaf_found =
			    mdb_pg_get_int24(mdb, 0x0c);
			//printf("next leaf %lu\n", chain->last_leaf_found);
			mdb_read_pg(mdb, chain->last_leaf_found);
			/* reuse the chain for cleanup mode */
			chain->cur_depth = 1;
			ipg = &chain->pages[0];
			mdb_index_page_init(ipg);
			ipg->pg = chain->last_leaf_found;
			//printf("next on page %d\n",
			if (!mdb_index_find_next_on_page(mdb, ipg)) {
#if 0
				printf("no find_next_on_page\n");
#endif
				return NULL;
			}
		}
	}
	return ipg;
}

static int
index_next(struct poi *poi, struct index_data *idx)
{
	MdbIndexPage *ipg;
	MdbIndexChain *chain = &poi->chain;
	int row;
	int pg;
	int offset;
	char *cmp, *low, *high;
	int debug=0;


	for(;;) {
	for(;;) {
	ipg=index_next_row(poi->h_idx, poi->idx, chain);
	if (! ipg)
		return 0;
	row = poi->h_idx->pg_buf[ipg->offset + ipg->len - 1];
	pg = mdb_pg_get_int24_msb(poi->h_idx, ipg->offset + ipg->len - 4);

	offset=poi->idx_size+4-ipg->len;
	memcpy(poi->index_data.data+offset, poi->h_idx->pg_buf+ipg->offset, ipg->len - 4);
	cmp=poi->index_data.data;
	low=idx[0].data;
	high=idx[1].data;
	if (debug > 1) {
		buffer_dump(low, 0, poi->idx_size-1);
		buffer_dump(cmp, 0, poi->idx_size-1);
		buffer_dump(high, 0, poi->idx_size-1);
		printf("%d %d %d\n", memcmp(cmp, low, poi->idx_size), memcmp(cmp, high, poi->idx_size), offset);
	}
#if 0
	buffer_dump(poi->h_idx->pg_buf, ipg->offset, ipg->offset+ipg->len-1);
#endif
	ipg->offset += ipg->len;
	if (memcmp(cmp, low, poi->idx_size) >= 0) {
		if (memcmp(cmp, high, poi->idx_size) <=0 ) {
			if (debug) {
				printf("match\n");
				buffer_dump(low, 0, poi->idx_size-1);
				buffer_dump(cmp, 0, poi->idx_size-1);
				buffer_dump(high, 0, poi->idx_size-1);
				printf("%d %d %d\n", memcmp(cmp, low, poi->idx_size), memcmp(cmp, high, poi->idx_size), offset);
			}
			break;
		} else {
			return 0;
		}
	}
	if (debug > 1)
		printf("row=0x%x pg=0x%x len=%d\n", row, pg, ipg->len);
	}
	if (debug)
		printf("match: row=0x%x pg=0x%x len=%d\n", row, pg, ipg->len);
		if (!load_row(poi, pg, row))
			break;
	}
	return 1;
}

#endif

static int
load_poi_table(struct map_priv *m, MdbCatalogEntry *entry)
{
	int j;
	MdbIndex *idx;

	m->h_idx=NULL;
	m->table = mdb_read_table(entry);
	m->table_col = mdb_read_columns(m->table);
	mdb_read_indices(m->table);
	m->cols = (MdbColumn **) (m->table_col->pdata);
	if (m->table_col->len < 4 || strcasecmp(m->cols[0]->name, "X") ||
		strcasecmp(m->cols[1]->name, "Y") || strcasecmp(m->cols[3]->name, "GEOFLAGS")) 
			return 1;
	m->name_col=-1;
	for (j = 0; j < m->table_col->len ; j++) {
		if (!strcasecmp(m->cols[j]->name, "NAME"))
			m->name_col=j;
	}
	for (j = 0; j < m->table->num_idxs; j++) {
		idx = m->table->indices->pdata[j];
		if (idx->num_keys == 3 && idx->key_col_num[0] == 1 &&
		    idx->key_col_num[1] == 2 && idx->key_col_num[2] == 4) {
			m->idx = idx;
			m->idx_size=3+m->cols[0]->col_size+m->cols[1]->col_size+m->cols[3]->col_size;
			m->h_idx=mdb_clone_handle(m->h);
		}
	}
	return 0;	
}

#if 0

static void
load_poi(char *filename, char *icon, int type)
{
	int i;
	MdbCatalogEntry *entry;
	GPtrArray *catalog;
	struct poi *new = g_new0(struct poi, 1);

       FILE *fp = fopen(filename,"r");
       if( fp ) {
	       fclose(fp);
       } else {
               printf("ERR : POI file %s does not exists!\n",filename);
		exit(0);
               return -1;
       }


       fp = fopen(icon,"r");
       if( fp ) {
	       fclose(fp);
       } else {
               printf("ERR : WARNING INCORRECT PICTURE! %s!\n",icon);
		exit(0);
               return -1;
       }

	strcpy(new->filename,filename);
	strcpy(new->icon,icon);
	new->type = type;


	if (type == 0) {
		new->h = mdb_open(filename, MDB_NOFLAGS);
		catalog = mdb_read_catalog(new->h, MDB_TABLE);
		for (i = 0; i < catalog->len; i++) {
			entry = catalog->pdata[i];
			if (!strcasecmp(entry->object_name, "_INDEXDATA")) {
				if (load_poi_table(new, entry)) {
					printf("%s invalid\n", filename);
					g_free(new);
					new=NULL;
				}
			}
		}
		g_ptr_array_free(catalog, 1);
	}
	if (new) {
		new->next = poi_list;
		poi_list = new;
	}
}

static void
get_coord(struct poi *p, struct coord *c)
{
	c->x=mdb_pg_get_int32(p->h, p->cols[0]->cur_value_start);
	c->y=mdb_pg_get_int32(p->h, p->cols[1]->cur_value_start);
}

static void
poi_info(struct display_list *list, struct popup_item **popup)
{
	struct poi_data *data=list->data;
	struct poi *poi=data->poi;
	struct popup_item *popup_last, *popup_val_last;
	char *text,buffer[4096];
	int j;
	MdbColumn *col;
	char *v;

	popup_last = *popup;

	popup_val_last = NULL;
	sprintf(buffer,"File:%s", poi->filename);
	popup_item_new_text(&popup_val_last, buffer, 1);
	sprintf(buffer,"Icon:%s", poi->icon);
	popup_item_new_text(&popup_val_last, buffer, 2);
	if (poi->type == 0) {
		printf("poi_info pg=%d row=%d\n", data->page, data->row);
		load_row(poi, data->page, data->row);
		sprintf(buffer,"Page:%d", data->page);
		popup_item_new_text(&popup_val_last, buffer, 3);
		sprintf(buffer,"Row:%d", data->row);
		popup_item_new_text(&popup_val_last, buffer, 4);
		for (j = 0; j < poi->table_col->len; j++) {
			col = poi->table_col->pdata[j];
	#if 0
			printf("start: %d type:%d\n", col->cur_value_start, col->col_type);
	#endif
			sprintf(buffer, "%s:", col->name);
			v = buffer + strlen(buffer);
			if (!strcasecmp(col->name,"X") || !strcasecmp(col->name,"Y")) 
				print_col(poi->h, col, v, 1);
			else
				print_col(poi->h, col, v, 0);
	#if 0
			printf("%s\n", buffer);
	#endif
			text=g_convert(buffer,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
			popup_item_new_text(&popup_val_last, buffer, j+10);
			g_free(text);
		}
	}
	popup_item_new_text(&popup_last, "POI", 20)->submenu = popup_val_last;
	*popup=popup_last;
}

static void
draw_poi(struct poi *p, struct container *co, struct point *pnt)
{
	struct poi_data data;
	data.poi=p;
	if (p->type == 0) {
		data.page=p->h->cur_pg;
		data.row=p->table->cur_row-1;
	}
	if (p->type == 1) {
		data.row=p->pos;
	}
	display_add(&co->disp[display_poi], 5, 0, p->icon, 1, pnt, poi_info, &data, sizeof(data));
}

static void
plugin_draw(struct container *co)
{
	struct coord c;
	struct point pnt;
	struct poi *p;
	struct index_data idx[2];
	int use_index=0;
	int debug=1;

	p = poi_list;

 	if (co->trans->scale > 1024)
 		return;	
	if (debug) {
		printf("scale=%ld\n", co->trans->scale);
		printf("rect 0x%lx,0%lx-0x%lx,0x%lx\n", co->trans->rect[0].x, co->trans->rect[0].y, co->trans->rect[1].x, co->trans->rect[1].y);
	}
	while (p) {
		if (p->type == 0) {
			if (use_index)
				setup_idx_rect(co->trans->rect, idx, p->idx_size);
			if (! use_index) {
				printf("rewind %s %p\n", p->filename, p->table);
				mdb_rewind_table(p->table);
				while (mdb_fetch_row(p->table)) {
					get_coord(p, &c);
					if (transform(co->trans, &c, &pnt)) {
						if (debug)
							printf("coord 0x%lx,0x%lx pg %d row %d\n", c.x, c.y, p->h->cur_pg, p->table->cur_row);
						draw_poi(p, co, &pnt);
					}
				}
			}  else {
				memset(&p->chain, 0, sizeof(p->chain));
				while (index_next(p, idx)) {
					get_coord(p, &c);
					if (transform(co->trans, &c, &pnt)) {
						if (debug)
							printf("coord 0x%lx,0x%lx pg %d row %d\n", c.x, c.y, p->h->cur_pg, p->table->cur_row);
						draw_poi(p, co, &pnt);
					}
				}
			}
		}
		if (p->type == 1) {
			FILE *f;
			char line[1024];
			struct text_poi tpoi;
			if(!(f=fopen(p->filename, "r"))){
				printf("can't open poi file for drawing!\n");
				exit(0);
			}
#if 0
			printf("opened poi file %s for drawing!\n",p->filename);
#endif
 			p->pos=ftell(f);
			fgets(line, 1024, f);
			while (!feof(f)) {
				if (strlen(line)) {
					line[strlen(line)-1]='\0';
				}
				if (parse_text_poi(line, &tpoi)) {
					transform_mercator(&tpoi.lat,&tpoi.lng,&c);
// 					printf("%ld %ld\n", c.x, c.y);
					if (transform(co->trans, &c, &pnt)) {
						draw_poi(p, co, &pnt);
					}
				}
				p->pos=ftell(f);
				fgets(line, 1024, f);
			}
			fclose(f);			
		}
		p = p->next;
	}

}

#endif

static void
map_destroy_poi_geodownload(struct map_priv *m)
{
        dbg(1,"enter\n");
        g_free(m);
}

static void
poi_geodownload_coord_rewind(void *priv_data)
{
	struct map_rect_priv *mr=priv_data;
	mr->cidx=0;
}


static int
poi_geodownload_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *mr=priv_data;
	dbg(1,"enter\n");
	if (mr->cidx || !count)
		return 0;
	c->x=mdb_pg_get_int32(mr->m->h, mr->m->cols[0]->cur_value_start);
	c->y=mdb_pg_get_int32(mr->m->h, mr->m->cols[1]->cur_value_start);
	dbg(1,"x=0x%x y=0x%x\n", c->x, c->y);
	return 1;
}

static void
poi_geodownload_attr_rewind(void *priv_data)
{
	struct map_rect_priv *mr=priv_data;
	mr->attr_next=attr_label;
}

static int
poi_geodownload_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct map_rect_priv *mr=priv_data;
	struct map_priv *m=mr->m;
	MdbColumn *col;
	char *v;
	int j;

	dbg(1,"enter\n");
	attr->type=attr_type;
	switch (attr_type) {
	case attr_any:
		while (mr->attr_next != attr_none) {
			if (poi_geodownload_attr_get(mr, mr->attr_next, attr))
				return 1;
                }
                return 0;
	case attr_label:
		mr->attr_next=attr_debug;
		if (m->name_col == -1)
			return 0;
		col=m->cols[m->name_col];
		if (col->cur_value_len) 
			attr->u.str=mdb_col_to_string (m->h, m->h->pg_buf, col->cur_value_start, col->col_type, col->cur_value_len);
		else
			attr->u.str="";
		return 1;
	case attr_debug:
		mr->attr_next=attr_none;
		v=mr->buffer;
		*v='\0';
		for (j = 0; j < mr->m->table_col->len; j++) {
			col = mr->m->table_col->pdata[j];
			printf("start: %d type:%d\n", col->cur_value_start, col->col_type);
			sprintf(v, "%s:", col->name);
			v += strlen(v);
			if (!strcasecmp(col->name,"X") || !strcasecmp(col->name,"Y")) 
				print_col(mr->m->h, col, v, 1);
			else
				print_col(mr->m->h, col, v, 0);
			v += strlen(v);
			*v++='\n';
			*v='\0';
		}
		attr->u.str=mr->buffer;
		return 1;
	default:
		break;
	}
	return 0;
}

static struct item_methods methods_poi_geodownload = {
	poi_geodownload_coord_rewind,
	poi_geodownload_coord_get,
	poi_geodownload_attr_rewind,
	poi_geodownload_attr_get,
};


static struct map_rect_priv *
map_rect_new_poi_geodownload(struct map_priv *map, struct map_selection *sel)
{
        struct map_rect_priv *mr;

        dbg(1,"enter\n");
        mr=g_new0(struct map_rect_priv, 1);
	mr->item.meth=&methods_poi_geodownload;
	mr->item.id_hi=0;
	mr->item.id_lo=0;
	mr->item.priv_data=mr;
	mr->item.type=map->type;
	mr->m=map;
	mdb_rewind_table(map->table);
        return mr;
}


static void
map_rect_destroy_poi_geodownload(struct map_rect_priv *mr)
{
	g_free(mr);
}

static struct item *
map_rect_get_item_poi_geodownload(struct map_rect_priv *mr)
{
	dbg(1,"enter\n");
	if (mdb_fetch_row(mr->m->table)) {
		mr->item.id_hi=mr->m->table->cur_phys_pg;
		mr->item.id_lo=mr->m->table->cur_row-1;
		poi_geodownload_attr_rewind(mr);
		return &mr->item;
	}
	return NULL;
}

static struct item *
map_rect_get_item_byid_poi_geodownload(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	char *v, buffer[4096];
	int j;
	MdbColumn *col;

	dbg(1,"enter\n");
	load_row(mr->m, id_hi, id_lo);
	for (j = 0; j < mr->m->table_col->len; j++) {
		col = mr->m->table_col->pdata[j];
		printf("start: %d type:%d\n", col->cur_value_start, col->col_type);
		sprintf(buffer, "%s:", col->name);
		v = buffer + strlen(buffer);
		if (!strcasecmp(col->name,"X") || !strcasecmp(col->name,"Y")) 
			print_col(mr->m->h, col, v, 1);
		else
			print_col(mr->m->h, col, v, 0);
		printf("%s\n", buffer);
	}
	dbg(1,"ret=%p\n", &mr->item);
	poi_geodownload_attr_rewind(mr);
	return &mr->item;
}


static struct map_methods map_methods_poi_geodownload = {
	projection_mg,
	"iso8859-1",
	map_destroy_poi_geodownload,
	map_rect_new_poi_geodownload,
	map_rect_destroy_poi_geodownload,
	map_rect_get_item_poi_geodownload,
	map_rect_get_item_byid_poi_geodownload,
};

static struct map_priv *
map_new_poi_geodownload(struct map_methods *meth, struct attr **attrs)
{
        struct map_priv *m;
	MdbCatalogEntry *entry;
	GPtrArray *catalog;
	int i;
	struct attr *attr;
	struct attr *data=attr_search(attrs, NULL, attr_data);
	char *filename;
        if (! data)
		return NULL;
	filename=data->u.str;
        dbg(1,"filename %s\n",filename);
        *meth=map_methods_poi_geodownload;

        m=g_new(struct map_priv, 1);
        m->filename=g_strdup(filename);
	m->h = mdb_open(m->filename, MDB_NOFLAGS);
	m->type=type_none;
	dbg(1,"attr_search\n");
	attr=attr_search(attrs, NULL, attr_item_type);
	dbg(1,"attr_search result %p\n", attr);
	if (attr) 
		m->type=attr->u.item_type;
		
	
	catalog = mdb_read_catalog(m->h, MDB_TABLE);
	for (i = 0; i < catalog->len; i++) {
		entry = catalog->pdata[i];
		dbg(1,"object name '%s'\n", entry->object_name);
		if (!strcasecmp(entry->object_name, "_INDEXDATA")) {
			if (load_poi_table(m, entry)) {
				printf("%s invalid\n", filename);
				g_free(m);
				m=NULL;
			}
		}
	}
	g_ptr_array_free(catalog, 1);
        return m;
}

void
plugin_init(void)
{
	dbg(1,"plugin_init\n");
	plugin_register_map_type("poi_geodownload", map_new_poi_geodownload);
	mdb_init();
}

