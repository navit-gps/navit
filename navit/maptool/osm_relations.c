/*
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
#include <stdio.h>
#include <string.h>
#include "maptool.h"
#include "attr.h"

/** Information about all members of a relation type and how to process them. */
struct relations {
    /** Hashes for nodes, ways and relations which are members. */
    GHashTable *member_hash[3];
    /** Default entries for processing items which are not a member of any relation. */
    GList *default_members;
};

struct relations_func {
    void (*func)(void *func_priv, void *relation_priv, struct item_bin *member, void *member_priv);
    void *func_priv;
};

struct relations_member {
    osmid memberid;
    void *relation_priv,*member_priv;
    struct relations_func *func;
};

static guint relations_member_hash(gconstpointer key) {
    const struct relations_member *memb=key;
    return (memb->memberid >> 32)^(memb->memberid & 0xffffffff);
}

static gboolean relations_member_equal(gconstpointer a, gconstpointer b) {
    const struct relations_member *memba=a;
    const struct relations_member *membb=b;
    return (memba->memberid == membb->memberid);
}

struct relations *
relations_new(void) {
    struct relations *ret=g_new0(struct relations, 1);
    int i;

    for (i = 0 ; i < 3 ; i++)
        ret->member_hash[i]=g_hash_table_new(relations_member_hash, relations_member_equal);
    return ret;
}

struct relations_func *
relations_func_new(void (*func)(void *func_priv, void *relation_priv, struct item_bin *member, void *member_priv),
                   void *func_priv) {
    struct relations_func *relations_func=g_new(struct relations_func, 1);
    relations_func->func=func;
    relations_func->func_priv=func_priv;
    return relations_func;
}

static struct relations_member *relations_member_new(struct relations_func *func, void *relation_priv,
        void *member_priv, osmid id) {
    struct relations_member *memb=g_new(struct relations_member, 1);
    memb->memberid=id;
    memb->relation_priv=relation_priv;
    memb->member_priv=member_priv;
    memb->func=func;
    return memb;
}
/*
 * @brief Add an entry for a relation member to the relations collection.
 * This function fills the relations collection, which is then passed to relations_process for
 * processing.
 * @param in rel relations collection to add the new member to
 * @param in func structure defining function to call when this member is read
 * @param in relation_priv parameter describing relation, or NULL. Will be passed to func function.
 * @param in member_priv parameter describing relation member, or NULL. Will be passed to func function.
 * @param in type Type of this member (node, way etc.).
 * @param in id OSM ID of relation member.
 */
void relations_add_relation_member_entry(struct relations *rel, struct relations_func *func, void
        *relation_priv, void *member_priv, enum relation_member_type type, osmid id) {
    struct relations_member *memb=relations_member_new(func, relation_priv, member_priv, id);
    GHashTable *member_hash=rel->member_hash[type-1];
    // The real key is the OSM ID, but we recycle "memb" as key to avoid a second allocating for the key.
    g_hash_table_insert(member_hash, memb, g_list_append(g_hash_table_lookup(member_hash, memb), memb));
}

/*
 * @brief Add a default entry to the relations collection.
 * Put a default entry into the relations collection, which is then passed to
 * relations_process for processing. The default entry is used for map items which are not a
 * member of any relation.
 * @param in rel relations collection to add the new member to
 * @param in func structure defining function to call when this member is read
 */
void relations_add_relation_default_entry(struct relations *rel, struct relations_func *func) {
    struct relations_member *memb=relations_member_new(func, NULL, NULL, 0);
    rel->default_members=g_list_append(rel->default_members, memb);
}


/*
 * @brief The actual relations processing: Loop through raw data and process any relations members.
 * This function reads through all nodes and ways passed in, and looks up each item in the
 * relations collection. For each relation member found, its processing function is called.
 * @param in rel relations collection storing pre-processed relations. Built using relations_add_relation_member_entry.
 * @param in nodes file containing nodes in "coords.tmp" format
 * @param in ways file containing items in item_bin format. This file may contain both nodes, ways, and relations in that format.
 */
void relations_process(struct relations *rel, FILE *nodes, FILE *ways) {
    char buffer[128];
    struct item_bin *ib=(struct item_bin *)buffer;
    osmid *id;
    struct coord *c=(struct coord *)(ib+1),cn= {0,0};
    struct node_item *ni;
    GList *l;

    if (nodes) {
        item_bin_init(ib, type_point_unkn);
        item_bin_add_coord(ib, &cn, 1);
        item_bin_add_attr_longlong(ib, attr_osm_nodeid, 0);
        id=item_bin_get_attr(ib, attr_osm_nodeid, NULL);
        while ((ni=read_node_item(nodes))) {
            *id=ni->nd_id;
            *c=ni->c;
            l=g_hash_table_lookup(rel->member_hash[0], id);
            while (l) {
                struct relations_member *memb=l->data;
                memb->func->func(memb->func->func_priv, memb->relation_priv, ib, memb->member_priv);
                l=g_list_next(l);
            }
        }
    }
    if (ways) {
        while ((ib=read_item(ways))) {
            l=NULL;
            if(NULL!=(id=item_bin_get_attr(ib, attr_osm_nodeid, NULL)))
                l=g_hash_table_lookup(rel->member_hash[0], id);
            else if(NULL!=(id=item_bin_get_attr(ib, attr_osm_wayid, NULL)))
                l=g_hash_table_lookup(rel->member_hash[1], id);
            else if(NULL!=(id=item_bin_get_attr(ib, attr_osm_relationid, NULL)))
                l=g_hash_table_lookup(rel->member_hash[2], id);
            if(!l)
                l=rel->default_members;
            while (l) {
                struct relations_member *memb=l->data;
                memb->func->func(memb->func->func_priv, memb->relation_priv, ib, memb->member_priv);
                l=g_list_next(l);
            }
        }
    }
}

/*
 * @brief The actual relations processing: Loop through raw data and process any relations members.
 * This function reads through all nodes and ways passed in, and looks up each item in the
 * relations collection. For each relation member found, its processing function is called.
 * @param in rel relations collection storing pre-processed relations. Built using relations_add_relation_member_entry.
 * @param in nodes file containing nodes in "coords.tmp" format
 * @param in ways file containing items in item_bin format. This file may contain both nodes, ways, and relations in that format.
 */
void relations_process_multi(struct relations **rel, int count, FILE *nodes, FILE *ways) {
    char buffer[128];
    struct item_bin *ib=(struct item_bin *)buffer;
    osmid *id;
    struct coord *c=(struct coord *)(ib+1),cn= {0,0};
    struct node_item *ni;
    GList *l;

    if(count <= 0)
        return;

    if (nodes) {
        item_bin_init(ib, type_point_unkn);
        item_bin_add_coord(ib, &cn, 1);
        item_bin_add_attr_longlong(ib, attr_osm_nodeid, 0);
        id=item_bin_get_attr(ib, attr_osm_nodeid, NULL);
        while ((ni=read_node_item(nodes))) {
            int i;
            *id=ni->nd_id;
            *c=ni->c;
            for(i=0; i < count; i ++) {
                l=g_hash_table_lookup(rel[i]->member_hash[0], id);
                while (l) {
                    struct relations_member *memb=l->data;
                    memb->func->func(memb->func->func_priv, memb->relation_priv, ib, memb->member_priv);
                    l=g_list_next(l);
                }
            }
        }
    }
    if (ways) {
        while ((ib=read_item(ways))) {
            int i;
            for(i=0; i < count; i ++) {
                l=NULL;
                if(NULL!=(id=item_bin_get_attr(ib, attr_osm_nodeid, NULL)))
                    l=g_hash_table_lookup(rel[i]->member_hash[0], id);
                else if(NULL!=(id=item_bin_get_attr(ib, attr_osm_wayid, NULL)))
                    l=g_hash_table_lookup(rel[i]->member_hash[1], id);
                else if(NULL!=(id=item_bin_get_attr(ib, attr_osm_relationid, NULL)))
                    l=g_hash_table_lookup(rel[i]->member_hash[2], id);
                if(!l)
                    l=rel[i]->default_members;
                while (l) {
                    struct relations_member *memb=l->data;
                    memb->func->func(memb->func->func_priv, memb->relation_priv, ib, memb->member_priv);
                    l=g_list_next(l);
                }
            }
        }
    }
}

static void relations_destroy_func(void *key, GList *l, void *data) {
    GList *ll=l;
    while (ll) {
        g_free(ll->data);
        ll=g_list_next(ll);
    }
    g_list_free(l);
}

void relations_destroy(struct relations *relations) {
    int i;

    for (i = 0 ; i < 3 ; i++) {
        g_hash_table_foreach(relations->member_hash[i], (GHFunc)relations_destroy_func, NULL);
        g_hash_table_destroy(relations->member_hash[i]);
    }
    if(relations->default_members != NULL) {
        GList *ll=relations->default_members;
        while (ll) {
            g_free(ll->data);
            ll=g_list_next(ll);
        }
        g_list_free(relations->default_members);
    }
    g_free(relations);
}
