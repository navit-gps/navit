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

#include <stdlib.h>
#include <glib.h>
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

struct filter_entry {
    enum item_type first,last;
    enum attr_type cond_attr;
    char *cond_str;
};

struct filter {
    GList *old;
    GList *new;
};

struct map_priv {
    struct map *parent;
    GList *filters;
};

struct map_rect_priv {
    struct map_selection *sel;
    struct map_priv *m;
    struct map_rect *parent;
    struct item item,*parent_item;
};

struct map_search_priv {
    struct map_rect_priv *mr;
};

static enum item_type filter_type(struct map_priv *m, struct item *item) {
    GList *filters=m->filters;
    struct filter_entry *entry;
    while (filters) {
        struct filter *filter=filters->data;
        int pos=0,count=0;
        GList *old,*new;
        old=filter->old;
        while (old) {
            entry=old->data;
            if (item->type >= entry->first && item->type <= entry->last)
                break;
            pos+=entry->last-entry->first+1;
            old=g_list_next(old);
        }
        if (old && entry && entry->cond_attr != attr_none) {
            struct attr attr;
            if (entry->cond_attr == attr_id) {
                char idstr[64];
                sprintf(idstr,"0x%x 0x%x",item->id_hi,item->id_lo);
                if (strcmp(entry->cond_str, idstr))
                    old=NULL;
            } else if (!item_attr_get(item, entry->cond_attr, &attr)) {
                old=NULL;
            } else {
                char *wildcard=strchr(entry->cond_str,'*');
                int len;
                if (!wildcard)
                    len=strlen(entry->cond_str)+1;
                else
                    len=wildcard-entry->cond_str;
                if (strncmp(entry->cond_str, attr.u.str, len))
                    old=NULL;
            }
            item_attr_rewind(item);
        }
        if (old) {
            new=filter->new;
            if (!new)
                return item->type;
            while (new) {
                struct filter_entry *entry=new->data;
                count+=entry->last-entry->first+1;
                new=g_list_next(new);
            }
            pos%=count;
            new=filter->new;
            while (new) {
                struct filter_entry *entry=new->data;
                if (pos <= entry->last-entry->first)
                    return entry->first+pos;
                pos-=entry->last-entry->first+1;
                new=g_list_next(new);
            }
        }
        filters=g_list_next(filters);
    }
    return item->type;
}

/* user_data is for compatibility with GFunc */
static void free_filter_entry(struct filter_entry *filter, void * user_data) {
    g_free(filter->cond_str);
    g_free(filter);
}

/* user_data is for compatibility with GFunc */
static void free_filter(struct filter *filter, void * user_data) {
    g_list_foreach(filter->old, (GFunc)free_filter_entry, NULL);
    g_list_free(filter->old);
    filter->old=NULL;
    g_list_foreach(filter->new, (GFunc)free_filter_entry, NULL);
    g_list_free(filter->new);
    filter->new=NULL;
}

static void free_filters(struct map_priv *m) {
    g_list_foreach(m->filters, (GFunc)free_filter, NULL);
    g_list_free(m->filters);
    m->filters=NULL;
}

static GList *parse_filter(char *filter) {
    GList *ret=NULL;
    for (;;) {
        char *condition,*range,*next=strchr(filter,',');
        struct filter_entry *entry=g_new0(struct filter_entry, 1);
        if (next)
            *next++='\0';
        condition=strchr(filter,'[');
        if (condition)
            *condition++='\0';
        range=strchr(filter,'-');
        if (range)
            *range++='\0';
        if (!strcmp(filter,"*") && !range) {
            entry->first=type_none;
            entry->last=type_last;
        } else {
            entry->first=item_from_name(filter);
            if (range)
                entry->last=item_from_name(range);
            else
                entry->last=entry->first;
        }
        if (condition) {
            char *end=strchr(condition,']');
            char *eq=strchr(condition,'=');
            if (end && eq && eq < end) {
                *end='\0';
                *eq++='\0';
                if (eq[0] == '"' || eq[0] == '\'') {
                    char *quote=strchr(eq+1,eq[0]);
                    if (quote) {
                        eq++;
                        *quote='\0';
                    }
                }
                entry->cond_attr=attr_from_name(condition);
                entry->cond_str=g_strdup(eq);
            }
        }
        ret=g_list_append(ret, entry);
        if (!next)
            break;
        filter=next;
    }
    return ret;
}

static void parse_filters(struct map_priv *m, char *filter) {
    char *filter_copy=g_strdup(filter);
    char *str=filter_copy;

    free_filters(m);
    for (;;) {
        char *pos,*bracket,*eq,*next=strchr(str,';');
        struct filter *filter=g_new0(struct filter, 1);
        if (next)
            *next++='\0';
        pos=str;
        for (;;) {
            eq=strchr(pos,'=');
            if (eq) {
                bracket=strchr(pos,'[');
                if (bracket && bracket < eq) {
                    bracket=strchr(pos,']');
                    if (bracket)
                        pos=bracket+1;
                    else {
                        eq=NULL;
                        break;
                    }
                } else {
                    *eq++='\0';
                    break;
                }
            } else
                break;
        }
        filter->old=parse_filter(str);
        if (eq)
            filter->new=parse_filter(eq);
        m->filters=g_list_append(m->filters,filter);
        if (!next)
            break;
        str=next;
    }
    g_free(filter_copy);
}

static void map_filter_coord_rewind(void *priv_data) {
    struct map_rect_priv *mr=priv_data;
    item_coord_rewind(mr->parent_item);
}

static int map_filter_coord_get(void *priv_data, struct coord *c, int count) {
    struct map_rect_priv *mr=priv_data;
    return item_coord_get(mr->parent_item, c, count);
}

static void map_filter_attr_rewind(void *priv_data) {
    struct map_rect_priv *mr=priv_data;
    item_attr_rewind(mr->parent_item);
}

static int map_filter_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr) {
    struct map_rect_priv *mr=priv_data;
    return item_attr_get(mr->parent_item, attr_type, attr);
}

static struct item_methods methods_filter = {
    map_filter_coord_rewind,
    map_filter_coord_get,
    map_filter_attr_rewind,
    map_filter_attr_get,
};

static struct map_rect_priv *map_filter_rect_new(struct map_priv *map, struct map_selection *sel) {
    struct map_rect_priv *mr=NULL;
    struct map_rect *parent;
    parent=map_rect_new(map->parent, sel);
    if (parent) {
        mr=g_new0(struct map_rect_priv, 1);
        mr->m=map;
        mr->sel=sel;
        mr->parent=parent;
        mr->item.meth=&methods_filter;
        mr->item.priv_data=mr;
    }
    return mr;
}

static void map_filter_rect_destroy(struct map_rect_priv *mr) {
    map_rect_destroy(mr->parent);
    g_free(mr);
}

static struct item *map_filter_rect_get_item(struct map_rect_priv *mr) {
    mr->parent_item=map_rect_get_item(mr->parent);
    if (!mr->parent_item)
        return NULL;
    mr->item.type=filter_type(mr->m,mr->parent_item);
    mr->item.id_lo=mr->parent_item->id_lo;
    mr->item.id_hi=mr->parent_item->id_hi;
    return &mr->item;
}

static struct item *map_filter_rect_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo) {
    dbg(lvl_debug,"enter");
    mr->parent_item=map_rect_get_item_byid(mr->parent, id_hi, id_lo);
    if (!mr->parent_item)
        return NULL;
    mr->item.type=filter_type(mr->m,mr->parent_item);
    mr->item.id_lo=mr->parent_item->id_lo;
    mr->item.id_hi=mr->parent_item->id_hi;
    return &mr->item;
}

static struct map_search_priv *map_filter_search_new(struct map_priv *map, struct item *item, struct attr *search,
        int partial) {
    dbg(lvl_debug,"enter");
    return NULL;
}

static struct item *map_filter_search_get_item(struct map_search_priv *map_search) {
    dbg(lvl_debug,"enter");
    return NULL;
}

static void map_filter_search_destroy(struct map_search_priv *ms) {
    dbg(lvl_debug,"enter");
}

static void map_filter_destroy(struct map_priv *m) {
    map_destroy(m->parent);
    g_free(m);
}

static int map_filter_set_attr(struct map_priv *m, struct attr *attr) {
    switch (attr->type) {
    case attr_filter:
        parse_filters(m,attr->u.str);
        return 1;
    default:
        return 0;
    }
}


static struct map_methods map_methods_filter = {
    projection_mg,
    "utf-8",
    map_filter_destroy,
    map_filter_rect_new,
    map_filter_rect_destroy,
    map_filter_rect_get_item,
    map_filter_rect_get_item_byid,
    map_filter_search_new,
    map_filter_search_destroy,
    map_filter_search_get_item,
    NULL,
    NULL,
    map_filter_set_attr,
};



static struct map_priv *map_filter_new(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl) {
    struct map_priv *m=NULL;
    struct attr **parent_attrs,type,*subtype=attr_search(attrs, NULL, attr_subtype),*filter=attr_search(attrs, NULL,
            attr_filter);
    struct map *map;
    int i,j;
    if (! subtype || !filter)
        return NULL;
    i=0;
    while (attrs[i])
        i++;
    parent_attrs=g_new(struct attr *,i+1);
    i=0;
    j=0;
    while (attrs[i]) {
        if (attrs[i]->type != attr_filter && attrs[i]->type != attr_type) {
            if (attrs[i]->type == attr_subtype) {
                type=*attrs[i];
                type.type = attr_type;
                parent_attrs[j]=&type;
            } else
                parent_attrs[j]=attrs[i];
            j++;
        }
        i++;
    }
    parent_attrs[j]=NULL;
    *meth=map_methods_filter;
    map=map_new(NULL, parent_attrs);
    if (map) {
        m=g_new0(struct map_priv, 1);
        m->parent=map;
        parse_filters(m,filter->u.str);
    }
    g_free(parent_attrs);
    return m;
}

void plugin_init(void) {
    dbg(lvl_debug,"filter: plugin_init");
    plugin_register_category_map("filter", map_filter_new);
}

