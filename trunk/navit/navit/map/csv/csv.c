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
#include <unistd.h>
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

//prototype
static void
csv_coord_set(void *priv_data, struct coord *c, int count, enum change_mode mode);

struct quadtree_data 
{
	struct item* item;
	GList* attr_list;
};

static void 
save_map_csv(struct map_priv *m)
{
	if(m->dirty) {
		GList *res = NULL, *item_it;
		char* filename = g_strdup_printf("%s.tmp",m->filename); 
		FILE* fp;
		int ferr = 0;
		char *csv_line = 0;
		char *tmpstr = 0;

		if( ! (fp=fopen(filename,"w+"))) {
			dbg(1, "Error opening csv file to write new entries");
			return;
		}
		//query the world
		quadtree_find_rect_items(m->tree_root, -90, 90, -90, 90, &res);

		item_it = res;
		while(item_it) {
			int i;
			enum attr_type *at = m->attr_types;
			csv_line = g_strdup_printf("");
			tmpstr = g_strdup_printf("");
			for(i=0;i<m->attr_cnt;++i) {
				if(at != m->attr_types) {
					csv_line = g_strdup_printf("%s,",tmpstr);
				}
				g_free(tmpstr);
				tmpstr = NULL;

				if(*at == attr_position_latitude) {
					tmpstr = g_strdup_printf("%lf",((struct quadtree_item*)(item_it->data))->latitude);
				}			
				else if(*at == attr_position_longitude) {
					tmpstr = g_strdup_printf("%lf",((struct quadtree_item*)(item_it->data))->longitude);
				}			
				else {
					GList* attr_list = ((struct quadtree_data*)(((struct quadtree_item*)(item_it->data))->data))->attr_list;
					GList* attr_it = attr_list;
					struct attr* found_attr = NULL;
					//search attributes
					while(attr_it) {
						if(((struct attr*)(attr_it->data))->type == *at) {
							found_attr = attr_it->data;
							break;
						}
						attr_it = g_list_next(attr_it);
					}
					if(found_attr) {
						if(ATTR_IS_INT(*at)) {
							tmpstr = g_strdup_printf("%d", found_attr->u.num);
						}
						else if(ATTR_IS_DOUBLE(*at)) {
							tmpstr = g_strdup_printf("%lf", *found_attr->u.numd);
						}
						else if(ATTR_IS_STRING(*at)) {
							tmpstr = g_strdup_printf("%s", found_attr->u.str);
						}
					}
					else {	//TODO handle this error
					}

				}
				csv_line = g_strdup_printf("%s%s",csv_line,tmpstr);
				g_free(tmpstr);
				tmpstr = csv_line;
				++at;
			}
			if(fprintf(fp,"%s\n", csv_line)<0) {
				ferr = 1;
			}
			item_it = g_list_next(item_it);
			tmpstr=g_strdup_printf("");
		}

		if(fclose(fp)) {
			ferr = 1;
		}

		if(! ferr) {
			unlink(m->filename);
			rename(filename,m->filename);
		}
		g_free(filename);
	}
}

static const int zoom_max = 18;

static void
map_destroy_csv(struct map_priv *m)
{
	dbg(1,"map_destroy_csv\n");
	//save if changed 
	save_map_csv(m);	
	g_free(m->attr_types);
	g_free(m);
	g_hash_table_destroy(m->item_hash);
	quadtree_destroy(m->tree_root);
	g_free(m->filename);
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
//	struct map_rect_priv *mr=priv_data;
	//TODO implement if needed
}

static int
csv_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{	
	GList* attr_list;
	struct map_rect_priv *mr=priv_data;

	if(!mr || !mr->curr_item || !mr->curr_item->data) {
		attr = NULL;
		return 0;
	}

	attr_list = ((struct quadtree_data*)(((struct quadtree_item*)(mr->curr_item->data))->data))->attr_list;

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

static int
csv_attr_set(void *priv_data, struct attr *attr, enum change_mode mode)
{
	struct map_rect_priv* mr = (struct map_rect_priv*)priv_data;
	struct map_priv* m = mr->m;
	int i, bFound = 0;
	struct attr *attr_new;
	GList *attr_list, *curr_attr_list;
	//if attribute is not supported by this csv map return 0
	for(i=0;i<m->attr_cnt;++i) {
		if(attr_type==attr->type) {
			bFound = 1;
			break;
		}
	}
	if( ! bFound) {
		return 0;
	}
	m->dirty = 1;
	attr_new = attr_dup(attr);
	attr_list = ((struct quadtree_data*)(((struct quadtree_item*)(mr->curr_item->data))->data))->attr_list;
	curr_attr_list = attr_list;

	while(attr_list) {
		if(((struct attr*)attr_list->data)->type == attr->type) {
			switch(mode) {
				case change_mode_delete:
					curr_attr_list = g_list_delete_link(curr_attr_list,attr_list->data);
					return 1;
				case change_mode_modify:
				case change_mode_prepend:
				case change_mode_append:
					//replace existing attribute
					if((struct attr*)attr_list->data) {
						g_free((struct attr*)attr_list->data);
					}
					attr_list->data = attr_new;
					return 1;
				default:
					g_free(attr_new);
					return 0;
			}
		}
		attr_list = g_list_next(attr_list);	
	}

	if( mode==change_mode_modify || mode==change_mode_prepend || mode==change_mode_append) {
		//add new attribute
		curr_attr_list = g_list_prepend(curr_attr_list, attr_new);	
		return 1;
	}
	g_free(attr_new);
	return 0;
}

static struct item_methods methods_csv = {
        csv_coord_rewind,
        csv_coord_get,
        csv_attr_rewind,
        csv_attr_get,
	NULL,
        csv_attr_set,
        csv_coord_set,
};

static void
csv_coord_set(void *priv_data, struct coord *c, int count, enum change_mode mode)
{
	struct map_rect_priv* mr = (struct map_rect_priv*)priv_data;
	struct map_priv* m = mr->m;
	struct coord_geo cg;
	struct quadtree_item *item;
	struct quadtree_item query_item;
	int i;

	for (i=0;i<count;++i) {
		struct item *curr_item;
		struct quadtree_data* qd;
		struct quadtree_item* qi;
		int* pID;

		transform_to_geo(projection_mg, &c[i], &cg);
		query_item.longitude = cg.lng;
		query_item.latitude  = cg.lat;
		item = quadtree_find_item(m->tree_root, &query_item);

		if(item) {  //already exists skip 
			continue;
		}
		m->dirty = 1;
		//add item to the map

		curr_item = item_new("",zoom_max);
		curr_item->type = m->item_type;
		curr_item->id_lo = m->next_item_idx;
		if (m->flags & 1)
			curr_item->id_hi=1;
		else
			curr_item->id_hi=0;
		curr_item->meth=&methods_csv;

		qd = g_new0(struct quadtree_data,1);
		qi = g_new (struct quadtree_item,1);
		pID = g_new(int,1);
		qd->item = curr_item;
		qd->attr_list = NULL;
		qi->data = qd;
		qi->longitude = cg.lng;
		qi->latitude  = cg.lat;
		quadtree_add(m->tree_root, qi);
		*pID = m->next_item_idx;
		g_hash_table_insert(m->item_hash, pID,curr_item);
		++m->next_item_idx;
	}
}

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
	if(sel) {   	
		transform_to_geo(projection_mg, &sel->u.c_rect.lu, &lu);
		transform_to_geo(projection_mg, &sel->u.c_rect.rl, &rl);
		quadtree_find_rect_items(map->tree_root, lu.lng, rl.lng, rl.lat, lu.lat, &res);
	}
	mr->query_result = res;
	mr->curr_item = res;
	return mr;
}


static void
map_rect_destroy_csv(struct map_rect_priv *mr)
{
	g_list_free(mr->query_result);
        g_free(mr);
}

static struct item *
map_rect_get_item_csv(struct map_rect_priv *mr)
{
	if(mr->bStarted) {
		if(mr->curr_item) {
			mr->curr_item = g_list_next(mr->curr_item);	
		}
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

static int
csv_get_attr(struct map_priv *m, enum attr_type type, struct attr *attr)
{
	return 0;
}


static struct map_methods map_methods_csv = {
	projection_mg,
	"iso8859-1",
	map_destroy_csv,
	map_rect_new_csv,
	map_rect_destroy_csv,
	map_rect_get_item_csv,
	map_rect_get_item_byid_csv,
	NULL,
	NULL,
	NULL,
	NULL,
	csv_get_attr,
};

static struct map_priv *
map_new_csv(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl)
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
		m->attr_cnt = attr_cnt;
		m->attr_types = attr_type_list;	
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

	m->item_type = item_type_attr->u.item_type;

	data=attr_search(attrs, NULL, attr_data);

	if(data) {
	  //load csv file into quadtree structure
	  //if column number is wrong skip
	  FILE*fp;
	  if((fp=fopen(data->u.str,"rt"))) {
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
					GList* attr_list = NULL;
					int bAddSum = 1;	
					double longitude = 0.0, latitude=0.0;
					struct item *curr_item = item_new("",zoom_max);//does not use parameters
					curr_item->type = item_type_attr->u.item_type;
					curr_item->id_lo = m->next_item_idx;
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
						*pID = m->next_item_idx;
						g_hash_table_insert(m->item_hash, pID,curr_item);
						++m->next_item_idx;
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
		m->filename = g_strdup(data->u.str);
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

