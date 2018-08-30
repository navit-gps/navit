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


#include "config.h"

#include <stdlib.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
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

/*prototypes*/
static int csv_coord_set(void *priv_data, struct coord *c, int count, enum change_mode mode);
static struct item * csv_create_item(struct map_rect_priv *mr, enum item_type it_type);
static void quadtree_item_free(void *mr, struct quadtree_item *qitem);
static void quadtree_item_free_do(void *qitem);


struct quadtree_data {
    enum item_type type;
    int id_lo;
    GList* attr_list;
    struct item *item;
};


struct quadtree_data *quadtree_data_dup(struct quadtree_data *qdata) {
    struct quadtree_data *ret=g_new0(struct quadtree_data,1);
    GList *l;
    ret->type=qdata->type;
    ret->id_lo=qdata->id_lo;
    ret->item=g_new(struct item,1);
    *(ret->item)=*(qdata->item);
    for(l=qdata->attr_list; l; l=g_list_next(l)) {
        ret->attr_list=g_list_prepend(ret->attr_list,attr_dup(l->data));
    }
    return ret;
}

static void save_map_csv(struct map_priv *m) {
    if(m->filename && m->dirty) {
        char* filename = g_strdup_printf("%s.tmp",m->filename);
        FILE* fp;
        int ferr = 0;
        char *csv_line = 0;
        char *tmpstr = 0;
        char *oldstr = 0;
        struct quadtree_iter *iter;
        struct quadtree_item *qitem;

        if( ! (fp=fopen(filename,"w+"))) {
            dbg(lvl_error, "Error opening csv file to write new entries");
            return;
        }
        /*query the world*/
        iter=quadtree_query(m->tree_root, -180, 180, -180, 180, quadtree_item_free, m);

        while((qitem = quadtree_item_next(iter))!=NULL) {
            int i;
            enum attr_type *at = m->attr_types;
            if(qitem->deleted)
                continue;
            csv_line = NULL;
            tmpstr = NULL;
            for(i=0; i<m->attr_cnt; ++i) {
                if(*at == attr_position_latitude) {
                    tmpstr = g_strdup_printf("%lf",qitem->latitude);
                } else if(*at == attr_position_longitude) {
                    tmpstr = g_strdup_printf("%lf",qitem->longitude);
                } else {
                    GList* attr_list = ((struct quadtree_data*)(qitem->data))->attr_list;
                    GList* attr_it = attr_list;
                    struct attr* found_attr = NULL;
                    /*search attributes*/
                    while(attr_it) {
                        if(((struct attr*)(attr_it->data))->type == *at) {
                            found_attr = attr_it->data;
                            break;
                        }
                        attr_it = g_list_next(attr_it);
                    }
                    if(found_attr) {
                        if(ATTR_IS_INT(*at)) {
                            tmpstr = g_strdup_printf("%d", (int)found_attr->u.num);
                        } else if(ATTR_IS_DOUBLE(*at)) {
                            tmpstr = g_strdup_printf("%lf", *found_attr->u.numd);
                        } else if(ATTR_IS_STRING(*at)) {
                            tmpstr = g_strdup(found_attr->u.str);
                        } else {
                            dbg(lvl_error,"Cant represent attribute %s",attr_to_name(*at));
                            tmpstr=g_strdup("");
                        }
                    } else {
                        dbg(lvl_debug,"No value defined for the attribute %s, assuming empty string",attr_to_name(*at));
                        tmpstr=g_strdup("");
                    }
                }
                if(i>0) {
                    oldstr = csv_line;
                    csv_line = g_strdup_printf("%s,%s",csv_line,tmpstr);
                    g_free(tmpstr);
                    g_free(oldstr);
                    tmpstr = NULL;
                } else {
                    csv_line=tmpstr;
                }
                ++at;
            }

            if(m->charset) {
                tmpstr=g_convert(csv_line, -1,m->charset,"utf-8",NULL,NULL,NULL);
                if(!tmpstr)
                    dbg(lvl_error,"Error converting '%s' to %s",csv_line, m->charset);
            } else
                tmpstr=csv_line;

            if(tmpstr && fprintf(fp,"%s\n", tmpstr)<0) {
                ferr = 1;
            }
            g_free(csv_line);
            if(m->charset)
                g_free(tmpstr);
        }

        if(fclose(fp)) {
            ferr = 1;
        }

        if(! ferr) {
            unlink(m->filename);
            rename(filename,m->filename);
            m->dirty = 0;
        }
        g_free(filename);
        quadtree_query_free(iter);

    }
}

static const int zoom_max = 18;

static void map_destroy_csv(struct map_priv *m) {
    dbg(lvl_debug,"map_destroy_csv");
    /*save if changed */
    save_map_csv(m);
    g_hash_table_destroy(m->qitem_hash);
    quadtree_destroy(m->tree_root);
    g_free(m->filename);
    g_free(m->charset);
    g_free(m->attr_types);
    g_free(m);
}

static void csv_coord_rewind(void *priv_data) {
}

static int csv_coord_get(void *priv_data, struct coord *c, int count) {
    struct map_rect_priv *mr=priv_data;
    if(mr) {
        *c = mr->c;
        return 1;
    } else {
        return 0;
    }
}

static void csv_attr_rewind(void *priv_data) {
    /*TODO implement if needed*/
}

static int csv_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr) {
    int i, bAttrFound = 0;
    GList* attr_list;
    struct map_rect_priv *mr=priv_data;
    enum attr_type *at;
    if( !mr || !mr->m || !mr->m->attr_types ) {
        return 0;
    }

    attr_list = ((struct quadtree_data*)(mr->qitem->data))->attr_list;

    if (attr_type == attr_any) {
        if (mr->at_iter==NULL) {	/*start iteration*/
            mr->at_iter = attr_list;
            if (mr->at_iter) {
                *attr = *(struct attr*)(mr->at_iter->data);
                return 1;
            } else {		/*empty attr list*/
                mr->at_iter = NULL;
                return 0;
            }
        } else {			/*continue iteration*/
            mr->at_iter = g_list_next(mr->at_iter);
            if(mr->at_iter) {
                *attr = *(struct attr*)mr->at_iter->data;
                return 1;
            } else {
                return 0;
            }
        }
        return 0;
    }

    at = mr->m->attr_types;

    for(i=0; i<mr->m->attr_cnt; ++i) {
        if(*at == attr_type) {
            bAttrFound = 1;
            break;
        }
        ++at;
    }

    if(!bAttrFound) {
        return 0;
    }

    while(attr_list) {
        if(((struct attr*)attr_list->data)->type == attr_type) {
            *attr = *(struct attr*)attr_list->data;
            return 1;
        }
        attr_list = g_list_next(attr_list);
    }
    return 0;
}

static int csv_attr_set(void *priv_data, struct attr *attr, enum change_mode mode) {
    struct map_rect_priv* mr;
    struct map_priv* m;
    int i, bFound;
    struct attr *attr_new;
    GList *attr_list, *curr_attr_list;
    enum attr_type *at;

    mr = (struct map_rect_priv*)priv_data;
    if(!mr || !mr->qitem) {
        return 0;
    }

    m = mr->m;
    bFound = 0;
    at = m->attr_types;

    /*if attribute is not supported by this csv map return 0*/
    for(i=0; i<m->attr_cnt; ++i) {
        if(*at==attr->type) {
            bFound = 1;
            break;
        }
        ++at;
    }
    if( ! bFound) {
        return 0;
    }
    m->dirty = 1;
    attr_new = attr_dup(attr);
    attr_list = ((struct quadtree_data*)(mr->qitem->data))->attr_list;
    curr_attr_list = attr_list;

    while(attr_list) {
        if(((struct attr*)attr_list->data)->type == attr->type) {
            switch(mode) {
            case change_mode_delete:
                attr_free((struct attr*)attr_list->data);
                curr_attr_list = g_list_delete_link(curr_attr_list,attr_list);
                m->dirty = 1;
                /* FIXME: To preserve consistency, may be the save_map_csv should be called here... */
                attr_free(attr_new);
                return 1;
            case change_mode_modify:
            case change_mode_prepend:
            case change_mode_append:
                /* replace existing attribute */
                if(attr_list->data) {
                    attr_free((struct attr*)attr_list->data);
                }
                attr_list->data = attr_new;
                m->dirty = 1;
                save_map_csv(m);
                return 1;
            default:
                attr_free(attr_new);
                return 0;
            }
        }
        attr_list = g_list_next(attr_list);
    }

    if( mode==change_mode_modify || mode==change_mode_prepend || mode==change_mode_append) {
        /* add new attribute */
        curr_attr_list = g_list_prepend(curr_attr_list, attr_new);
        ((struct quadtree_data*)(mr->qitem->data))->attr_list = curr_attr_list;
        m->dirty = 1;
        save_map_csv(m);
        return 1;
    }
    attr_free(attr_new);
    return 0;
}

static int csv_type_set(void *priv_data, enum item_type type) {
    struct map_rect_priv* mr = (struct map_rect_priv*)priv_data;
    dbg(lvl_debug,"Enter %d", type);

    if(!mr || !mr->qitem) {
        dbg(lvl_debug,"Nothing to do");
        return 0;
    }

    if(type!=type_none)
        return 0;

    mr->qitem->deleted=1;
    dbg(lvl_debug,"Item %p is deleted",mr->qitem);

    return 1;
}

static struct item_methods methods_csv = {
    csv_coord_rewind,
    csv_coord_get,
    csv_attr_rewind,
    csv_attr_get,
    NULL,
    csv_attr_set,
    csv_coord_set,
    csv_type_set
};


/*
 * Sets coordinate of an existing item (either on the new list or an item with coord )
 */
static int csv_coord_set(void *priv_data, struct coord *c, int count, enum change_mode mode) {
    struct quadtree_item query_item, *insert_item, *query_res;
    struct coord_geo cg;
    struct map_rect_priv* mr;
    struct map_priv* m;
    struct quadtree_item* qi;
    GList* new_it;
    dbg(lvl_debug,"Set coordinates %d %d", c->x, c->y);

    /* for now we only support coord modification only */
    if( ! change_mode_modify) {
        return 0;
    }
    /* csv driver supports one coord per record only */
    if( count != 1) {
        return 0;
    }

    /* get curr_item of given map_rect */
    mr = (struct map_rect_priv*)priv_data;
    m = mr->m;

    if(!mr->qitem) {
        return 0;
    }

    qi = mr->qitem;

    transform_to_geo(projection_mg, &c[0], &cg);

    /* if it is on the new list remove from new list and add it to the tree with the coord */
    new_it = m->new_items;
    while(new_it) {
        if(new_it->data==qi) {
            break;
        }
        new_it = g_list_next(new_it);
    }
    if(new_it) {
        qi->longitude = cg.lng;
        qi->latitude = cg.lat;
        quadtree_add( m->tree_root, qi, mr->qiter);
        dbg(lvl_debug,"Set coordinates %f %f", cg.lng, cg.lat);
        m->new_items = g_list_remove_link(m->new_items,new_it);
        m->dirty=1;
        save_map_csv(m);
        return 1;
    }

    /* else update quadtree item with the new coord
          remove item from the quadtree */
    query_item.longitude = cg.lng;
    query_item.latitude = cg.lat;
    query_res = quadtree_find_item(m->tree_root, &query_item);
    if(!query_res) {
        return 0;
    }
    quadtree_delete_item(m->tree_root, query_res);
    /*    add item to the tree with the new coord */
    insert_item=g_new0(struct quadtree_item,1);
    insert_item->data=quadtree_data_dup(query_res->data);
    insert_item->longitude = cg.lng;
    insert_item->latitude  = cg.lat;
    quadtree_add(m->tree_root, query_res, mr->qiter);

    mr->qitem->ref_count--;
    mr->qitem=insert_item;
    mr->qitem->ref_count++;

    m->dirty = 1;
    save_map_csv(m);
    return 1;
}

static void quadtree_item_free(void *this, struct quadtree_item *qitem) {
    struct map_priv* m=this;
    struct quadtree_data * qdata=qitem->data;
    if(m) {
        g_hash_table_remove(m->qitem_hash,&(qdata->item->id_lo));
    }
}

static void quadtree_item_free_do(void *data) {
    struct quadtree_item *qitem=data;
    GList* attr_it;
    struct attr* attr;
    struct quadtree_data * qdata=qitem->data;
    if(qdata) {
        for(attr_it = qdata->attr_list; attr_it; attr_it = g_list_next(attr_it)) {
            attr = attr_it->data;
            attr_free(attr);
        }
        g_list_free(qdata->attr_list);
        g_free(qdata->item);
        g_free(qitem->data);
    }
    g_free(data);
}

static void map_csv_debug_dump_hash_item(gpointer key, gpointer value, gpointer user_data) {
    struct quadtree_item *qi=value;
    GList *attrs;
    dbg(lvl_debug,"%p del=%d ref=%d", qi,qi->deleted, qi->ref_count);
    attrs=((struct quadtree_data *)qi->data)->attr_list;
    while(attrs) {
        if(((struct attr*)attrs->data)->type==attr_label)
            dbg(lvl_debug,"... %s",((struct attr*)attrs->data)->u.str);
        attrs=g_list_next(attrs);
    }
}

/**
 * Dump all map data (including deleted items) to the log.
 */
static void map_csv_debug_dump(struct map_priv *map) {
    g_hash_table_foreach(map->qitem_hash, map_csv_debug_dump_hash_item, NULL);
}


static struct map_rect_priv *map_rect_new_csv(struct map_priv *map, struct map_selection *sel) {
    struct map_rect_priv *mr;
    struct coord_geo lu;
    struct coord_geo rl;
    struct quadtree_iter *res = NULL;
    dbg(lvl_debug,"map_rect_new_csv");
    if(debug_level_get("map_csv")>2) {
        map_csv_debug_dump(map);
    }
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

    if(!sel) {
        lu.lng=-180;
        lu.lat=180;
        rl.lng=180;
        rl.lat=-180;
    } else {
        transform_to_geo(projection_mg, &sel->u.c_rect.lu, &lu);
        transform_to_geo(projection_mg, &sel->u.c_rect.rl, &rl);
    }
    res=quadtree_query(map->tree_root, lu.lng, rl.lng, rl.lat, lu.lat, quadtree_item_free, mr->m);
    mr->qiter = res;
    mr->qitem = NULL;
    return mr;
}

static void map_rect_destroy_csv(struct map_rect_priv *mr) {
    if(mr->qitem)
        mr->qitem->ref_count--;

    if(mr->qiter)
        quadtree_query_free(mr->qiter);

    g_free(mr);
}

static struct item *map_rect_get_item_csv(struct map_rect_priv *mr) {

    if(mr->qitem)
        mr->qitem->ref_count--;

    mr->qitem=quadtree_item_next(mr->qiter);

    if(mr->qitem) {
        struct item* ret=&(mr->item);
        struct coord_geo cg;
        mr->qitem->ref_count++;
        mr->item = *(((struct quadtree_data*)(mr->qitem->data))->item);
        ret->priv_data=mr;
        cg.lng = mr->qitem->longitude;
        cg.lat = mr->qitem->latitude;
        transform_from_geo(projection_mg, &cg, &mr->c);
        return ret;
    }
    return NULL;
}

static struct item *map_rect_get_item_byid_csv(struct map_rect_priv *mr, int id_hi, int id_lo) {
    /*currently id_hi is ignored*/

    struct quadtree_item *qit = g_hash_table_lookup(mr->m->qitem_hash,&id_lo);

    if(mr->qitem )
        mr->qitem->ref_count--;

    if(qit) {
        mr->qitem = qit;
        mr->qitem->ref_count++;
        mr->item=*(((struct quadtree_data*)(qit->data))->item);
        mr->item.priv_data=mr;
        return &(mr->item);
    } else {
        mr->qitem = NULL;
        return NULL;
    }
}

static int csv_get_attr(struct map_priv *m, enum attr_type type, struct attr *attr) {
    return 0;
}

static struct item *csv_create_item(struct map_rect_priv *mr, enum item_type it_type) {
    struct map_priv* m;
    struct quadtree_data* qd;
    struct quadtree_item* qi;
    struct item* curr_item;
    int* pID;
    if(mr && mr->m) {
        m = mr->m;
    } else {
        return NULL;
    }

    if( m->item_type != it_type) {
        return NULL;
    }

    m->dirty = 1;
    /*add item to the map*/
    curr_item = item_new("",zoom_max);
    curr_item->type = m->item_type;
    curr_item->meth=&methods_csv;

    curr_item->id_lo = m->next_item_idx;
    if (m->flags & 1)
        curr_item->id_hi=1;
    else
        curr_item->id_hi=0;

    qd = g_new0(struct quadtree_data,1);
    qi = g_new0(struct quadtree_item,1);
    qd->item = curr_item;
    qd->attr_list = NULL;
    qi->data = qd;
    /*we don`t have valid coord yet*/
    qi->longitude = 0;
    qi->latitude  = 0;
    /*add the coord less item to the new list*/
    m->new_items = g_list_prepend(m->new_items, qi);
    if(mr->qitem)
        mr->qitem->ref_count--;
    mr->qitem=qi;
    mr->item=*curr_item;
    mr->item.priv_data=mr;
    mr->qitem->ref_count++;
    /*don't add to the quadtree yet, wait until we have a valid coord*/
    pID = g_new(int,1);
    *pID = m->next_item_idx;
    g_hash_table_insert(m->qitem_hash, pID,qi);
    ++m->next_item_idx;
    return &mr->item;
}

static struct map_methods map_methods_csv = {
    projection_mg,
    "utf-8",
    map_destroy_csv,
    map_rect_new_csv,
    map_rect_destroy_csv,
    map_rect_get_item_csv,
    map_rect_get_item_byid_csv,
    NULL,
    NULL,
    NULL,
    csv_create_item,
    csv_get_attr,
};

static struct map_priv *map_new_csv(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl) {
    struct map_priv *m = NULL;
    struct attr *attr_types;
    struct attr *item_type_attr;
    struct attr *data;
    struct attr *flags;
    struct attr *charset;
    int bLonFound = 0;
    int bLatFound = 0;
    int attr_cnt = 0;
    enum attr_type* attr_type_list = NULL;
    struct quadtree_node* tree_root = quadtree_node_new(NULL,-180,180,-180,180);
    m = g_new0(struct map_priv, 1);
    m->id = ++map_id;
    m->qitem_hash = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, quadtree_item_free_do);
    m->tree_root = tree_root;

    attr_types = attr_search(attrs, NULL, attr_attr_types);
    if(attr_types) {
        enum attr_type* at = attr_types->u.attr_types;
        while(*at != attr_none) {
            attr_type_list = g_realloc(attr_type_list,sizeof(enum attr_type)*(attr_cnt+1));
            attr_type_list[attr_cnt] = *at;
            if(*at==attr_position_latitude) {
                bLatFound = 1;
            } else if(*at==attr_position_longitude) {
                bLonFound = 1;
            }
            ++attr_cnt;
            ++at;
        }
        m->attr_cnt = attr_cnt;
        m->attr_types = attr_type_list;
    } else {
        m->attr_types = NULL;
        return NULL;
    }

    charset  = attr_search(attrs, NULL, attr_charset);
    if(charset) {
        dbg(lvl_debug,"charset:%s",charset->u.str);
        m->charset=g_strdup(charset->u.str);
    } else {
        m->charset=g_strdup(map_methods_csv.charset);
    }

    if(bLonFound==0 || bLatFound==0) {
        return NULL;
    }

    item_type_attr=attr_search(attrs, NULL, attr_item_type);

    if( !item_type_attr || item_type_attr->u.item_type==type_none) {
        return NULL;
    }

    m->item_type = item_type_attr->u.item_type;

    flags=attr_search(attrs, NULL, attr_flags);
    if (flags)
        m->flags=flags->u.num;

    *meth = map_methods_csv;

    data=attr_search(attrs, NULL, attr_data);

    if(data) {
        struct file_wordexp *wexp;
        char **wexp_data;
        FILE *fp;
        wexp=file_wordexp_new(data->u.str);
        wexp_data=file_wordexp_get_array(wexp);
        dbg(lvl_debug,"map_new_csv %s", data->u.str);
        m->filename=g_strdup(wexp_data[0]);
        file_wordexp_destroy(wexp);

        /*load csv file into quadtree structure*/
        /*if column number is wrong skip*/
        if((fp=fopen(m->filename,"rt"))) {
            const int max_line_len = 256;
            char *linebuf=g_alloca(sizeof(char)*max_line_len);
            while(!feof(fp)) {
                if(fgets(linebuf,max_line_len,fp)) {
                    char *line=g_convert(linebuf, -1,"utf-8",m->charset,NULL,NULL,NULL);
                    char *line2=NULL;
                    char *delim = ",";
                    int col_cnt=0;
                    char *tok;
                    if(!line) {
                        dbg(lvl_error,"Error converting '%s' to utf-8 from %s",linebuf, m->charset);
                        continue;
                    }
                    if(line[strlen(line)-1]=='\n' || line[strlen(line)-1]=='\r') {
                        line[strlen(line)-1] = '\0';
                    }
                    line2 = g_strdup(line);
                    while((tok=strtok( (col_cnt==0)?line:NULL, delim))) {
                        ++col_cnt;
                    }

                    if(col_cnt==attr_cnt) {
                        int cnt = 0;	/*index of current attr*/
                        char*tok;
                        GList* attr_list = NULL;
                        int bAddSum = 1;
                        double longitude = 0.0, latitude=0.0;
                        struct item *curr_item = item_new("",zoom_max);/*does not use parameters*/

                        curr_item->type = item_type_attr->u.item_type;
                        curr_item->id_lo = m->next_item_idx;
                        if (m->flags & 1)
                            curr_item->id_hi=1;
                        else
                            curr_item->id_hi=0;
                        curr_item->meth=&methods_csv;


                        while((tok=strtok( (cnt==0)?line2:NULL, delim))) {
                            struct attr*curr_attr = g_new0(struct attr,1);
                            int bAdd = 1;
                            curr_attr->type = attr_types->u.attr_types[cnt];
                            if(ATTR_IS_STRING(attr_types->u.attr_types[cnt])) {
                                curr_attr->u.str = g_strdup(tok);
                            } else if(ATTR_IS_INT(attr_types->u.attr_types[cnt])) {
                                curr_attr->u.num = atoi(tok);
                            } else if(ATTR_IS_DOUBLE(attr_types->u.attr_types[cnt])) {
                                double *d = g_new(double,1);
                                *d = atof(tok);
                                curr_attr->u.numd = d;
                                if(attr_types->u.attr_types[cnt] == attr_position_longitude) {
                                    longitude = *d;
                                }
                                if(attr_types->u.attr_types[cnt] == attr_position_latitude) {
                                    latitude = *d;
                                }
                            } else {
                                /*unknown attribute*/
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
                            struct quadtree_item* qi =g_new0(struct quadtree_item,1);
                            int* pID = g_new(int,1);
                            qd->item = curr_item;
                            qd->attr_list = attr_list;
                            qi->data = qd;
                            qi->longitude = longitude;
                            qi->latitude = latitude;
                            quadtree_add(tree_root, qi, NULL);
                            *pID = m->next_item_idx;
                            g_hash_table_insert(m->qitem_hash, pID,qi);
                            ++m->next_item_idx;
                            dbg(lvl_debug,"%s",line);
                        } else {
                            g_free(curr_item);
                        }

                    } else {
                        dbg(lvl_error,"ERROR: Non-matching attr count and column count: %d %d  SKIPPING line: %s",col_cnt, attr_cnt,line);
                    }
                    g_free(line);
                    g_free(line2);
                }
            }
            fclose(fp);
        } else {
            dbg(lvl_error,"Error opening csv map file '%s': %s", m->filename, strerror(errno));
            return NULL;
        }
    } else {
        dbg(lvl_debug,"No data attribute, starting with in-memory map");
    }

    dbg(lvl_info,"%p",tree_root);
    return m;
}

void plugin_init(void) {
    dbg(lvl_debug,"csv: plugin_init");
    plugin_register_category_map("csv", map_new_csv);
}

