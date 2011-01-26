/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2011 Navit Team
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
#include "transform.h"
#include "file.h"
#include "quadtree.h"

#include "csv.h"

static int map_id;


struct quadtree_data 
{
	struct item* item;
	GList* attr_list;
};

static void
map_destroy_csv(struct map_priv *m)
{
	dbg(1,"map_destroy_csv\n");
	g_free(m);
	g_hash_table_destroy(m->item_hash);
	quadtree_destroy(m->tree_root);
}

static void
csv_coord_rewind(void *priv_data)
{
}

static int
csv_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *mr=priv_data;
	if(mr) {
		*c = mr->c;
		return 1;
	}
	else {
		return 0;
	}
}

static void
csv_attr_rewind(void *priv_data)
{
	struct map_rect_priv *mr=priv_data;
	//TODO implement if needed
}

static int
csv_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{	
	struct map_rect_priv *mr=priv_data;

	GList* attr_list = ((struct quadtree_data*)(((struct quadtree_item*)(mr->curr_item->data))->data))->attr_list;
	if (attr_type == attr_any) {
	}

	while(attr_list) {
		if(((struct attr*)attr_list->data)->type == attr_type) {
			*attr = *(struct attr*)attr_list->data;
			return 1;
		}
		attr_list = g_list_next(attr_list);	
	}
	attr = NULL;
	return 0;
}

static struct item_methods methods_csv = {
        csv_coord_rewind,
        csv_coord_get,
        csv_attr_rewind,
        csv_attr_get,
};

static struct map_rect_priv *
map_rect_new_csv(struct map_priv *map, struct map_selection *sel)
{
	struct map_rect_priv *mr;
	struct coord_geo lu;
	struct coord_geo rl;
	GList*res = NULL;

	dbg(1,"map_rect_new_csv\n");
	mr=g_new0(struct map_rect_priv, 1);
	mr->m=map;
	mr->bStarted = 0;
	mr->sel=sel;
	if (map->flags & 1)
		mr->item.id_hi=1;
	else
		mr->item.id_hi=0;
	mr->item.id_lo=0;
	mr->item.meth=&methods_csv;
	mr->item.priv_data=mr;

	//convert selection to geo
	transform_to_geo(projection_mg, &sel->u.c_rect.lu, &lu);
	transform_to_geo(projection_mg, &sel->u.c_rect.rl, &rl);
	quadtree_find_rect_items(map->tree_root, lu.lng, rl.lng, rl.lat, lu.lat, &res);
	mr->query_result = res;
	mr->curr_item = res;
	return mr;
}


static void
map_rect_destroy_csv(struct map_rect_priv *mr)
{
        g_free(mr);
}

static struct item *
map_rect_get_item_csv(struct map_rect_priv *mr)
{
	if(mr->bStarted) {
		mr->curr_item = g_list_next(mr->curr_item);	
	}
	else {
		mr->bStarted = 1;
	}

	if(mr->curr_item) {
		struct item* ret;
		struct coord_geo cg;
		ret = ((struct quadtree_data*)(((struct quadtree_item*)(mr->curr_item->data))->data))->item;
		ret->priv_data=mr;
		if(mr->curr_item && mr->curr_item->data) {
			cg.lng = ((struct quadtree_item*)(mr->curr_item->data))->longitude;
			cg.lat = ((struct quadtree_item*)(mr->curr_item->data))->latitude;
			transform_from_geo(projection_mg, &cg, &mr->c);
			ret = ((struct quadtree_data*)(((struct quadtree_item*)(mr->curr_item->data))->data))->item;
			return ret;
		}

	}
	return NULL;
}

static struct item *
map_rect_get_item_byid_csv(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	//currently id_hi is ignored
	return g_hash_table_lookup(mr->m->item_hash,&id_lo);
}

static struct map_methods map_methods_csv = {
	projection_mg,
	"iso8859-1",
	map_destroy_csv,
	map_rect_new_csv,
	map_rect_destroy_csv,
	map_rect_get_item_csv,
	map_rect_get_item_byid_csv,
};

static struct map_priv *
map_new_csv(struct map_methods *meth, struct attr **attrs)
{
	struct map_priv *m = NULL;
	struct attr *item_type;
	struct attr *attr_types;
	struct attr *item_type_attr;
	struct attr *data;
	struct attr *flags;
	int bLonFound = 0;
	int bLatFound = 0;
	int attr_cnt = 0;
	enum attr_type* attr_type_list = NULL;	
	struct quadtree_node* tree_root = quadtree_node_new(NULL,-90,90,-90,90);
	m = g_new0(struct map_priv, 1);
	m->id = ++map_id;
	m->item_hash = g_hash_table_new_full(g_int_hash, g_int_equal,g_free,g_free);

	item_type  = attr_search(attrs, NULL, attr_item_type);
	attr_types = attr_search(attrs, NULL, attr_attr_types);
	if(attr_types) {
		enum attr_type* at = attr_types->u.attr_types; 
		while(*at != attr_none) {
			attr_type_list = g_realloc(attr_type_list,sizeof(enum attr_type)*(attr_cnt+1));
			attr_type_list[attr_cnt] = *at;
			if(*at==attr_position_latitude) {
				bLatFound = 1;
			}
			else if(*at==attr_position_longitude) {
				bLonFound = 1;
			}
			++attr_cnt;
			++at;
		}
	}
	else {
		return NULL;
	}

	if(bLonFound==0 || bLatFound==0) {
		return NULL;
	}
	
	item_type_attr=attr_search(attrs, NULL, attr_item_type);

	if( !item_type_attr || item_type_attr->u.item_type==type_none) {
		return NULL;
	}

	data=attr_search(attrs, NULL, attr_data);

	if(data) {
	  //load csv file into quadtree structure
	  //if column number is wrong skip
	  FILE*fp;
	  if((fp=fopen(data->u.str,"rt"))) {
		int item_idx = 0;
		const int max_line_len = 256;
		char *line=g_alloca(sizeof(char)*max_line_len);
	  	while(!feof(fp)) {
			if(fgets(line,max_line_len,fp)) {
				char*line2 = g_strdup(line);
				//count columns	
				//TODO make delimiter parameter
				char* delim = ",";
				int col_cnt=0;
				char*tok;
				while((tok=strtok( (col_cnt==0)?line:NULL , delim))) {
					++col_cnt;
				}

				if(col_cnt==attr_cnt) {
					int cnt = 0;	//idx of current attr
					char*tok;
					const int zoom_max = 18;
					GList* attr_list = NULL;
					int bAddSum = 1;	
					double longitude = 0.0, latitude=0.0;
					struct item *curr_item = item_new("",zoom_max);//does not use parameters
					curr_item->type = item_type_attr->u.item_type;
					curr_item->id_lo = item_idx;
					if (m->flags & 1)
						curr_item->id_hi=1;
					else
						curr_item->id_hi=0;
					curr_item->meth=&methods_csv;

					
					while((tok=strtok( (cnt==0)?line2:NULL , delim))) {
						struct attr*curr_attr = g_new0(struct attr,1);
						int bAdd = 1;	
						curr_attr->type = attr_types->u.attr_types[cnt];
						if(ATTR_IS_STRING(attr_types->u.attr_types[cnt])) {
							curr_attr->u.str = g_strdup(tok);
						}
						else if(ATTR_IS_INT(attr_types->u.attr_types[cnt])) {
							curr_attr->u.num = atoi(tok);
						}
						else if(ATTR_IS_DOUBLE(attr_types->u.attr_types[cnt])) {
							double *d = g_new(double,1);
							*d = atof(tok);
							curr_attr->u.numd = d;
							if(attr_types->u.attr_types[cnt] == attr_position_longitude) {
								longitude = *d;
							}
							if(attr_types->u.attr_types[cnt] == attr_position_latitude) {
								latitude = *d;
							}
						}
						else {
							//unknown attribute
							bAddSum = bAdd = 0;
							g_free(curr_attr);
						}

						if(bAdd) {
							attr_list = g_list_prepend(attr_list, curr_attr);
						}
						++cnt;
					}
					if(bAddSum && (longitude!=0.0 || latitude!=0.0)) {
						struct quadtree_data* qd = g_new0(struct quadtree_data,1);
						struct quadtree_item* qi =g_new(struct quadtree_item,1);
						int* pID = g_new(int,1);
						qd->item = curr_item;
						qd->attr_list = attr_list;
						qi->data = qd;
						qi->longitude = longitude;
						qi->latitude = latitude;
						quadtree_add(tree_root, qi);
						*pID = item_idx;
						g_hash_table_insert(m->item_hash, pID,curr_item);
						++item_idx;
					}
					else {
						g_free(curr_item);
					}
					
				}
				else {
	  				//printf("ERROR: Non-matching attr count and column count: %d %d  SKIPPING line: %s\n",col_cnt, attr_cnt,line);
				}
				g_free(line2);
			}
		}
	  	fclose(fp);
	  }
	  else {
	  	return NULL;
	  }
	}

	*meth = map_methods_csv;
	m->tree_root = tree_root;
	flags=attr_search(attrs, NULL, attr_flags);
	if (flags) 
		m->flags=flags->u.num;
	return m;
}

void
plugin_init(void)
{
	dbg(1,"csv: plugin_init\n");
	plugin_register_map_type("csv", map_new_csv);
}

