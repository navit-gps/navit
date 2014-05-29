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
#include <stdio.h>
#include <string.h>
#include "maptool.h"
#include "attr.h"

struct relations {
	GHashTable *member_hash[3];
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

static guint
relations_member_hash(gconstpointer key)
{
	const struct relations_member *memb=key;
	return (memb->memberid >> 32)^(memb->memberid & 0xffffffff);
}

static gboolean
relations_member_equal(gconstpointer a, gconstpointer b)
{
	const struct relations_member *memba=a;
	const struct relations_member *membb=b;
	return (memba->memberid == membb->memberid);
}

struct relations *
relations_new(void)
{
	struct relations *ret=g_new0(struct relations, 1);
	int i;

	for (i = 0 ; i < 3 ; i++)
		ret->member_hash[i]=g_hash_table_new(relations_member_hash, relations_member_equal);
	return ret;
}

struct relations_func *
relations_func_new(void (*func)(void *func_priv, void *relation_priv, struct item_bin *member, void *member_priv), void *func_priv)
{
	struct relations_func *relations_func=g_new(struct relations_func, 1);
	relations_func->func=func;
	relations_func->func_priv=func_priv;
	return relations_func;
}

/*
 * @brief Add a relation member to relations collection.
 * @param in rel relations collection to add the new member to.
 * @param in funct structure defining function to call when this member is read
 * @param in relation_priv parameter describing relation. Will be passed to funct function
 * @param in member_priv parameter describing member function. Will be passed to funct function
 * @param in type This member type: 1 - node, 2 - way, 3 - relation. 
 * 			Set to -1 to add a default member action which matches any item of any type which is not a member of any relation.
 * @param in osmid This member id
 * @param unused relations
 */
void
relations_add_func(struct relations *rel, struct relations_func *func, void *relation_priv, void *member_priv, int type, osmid id)
{
	struct relations_member *memb=g_new(struct relations_member, 1);

	memb->memberid=id;
	memb->relation_priv=relation_priv;
	memb->member_priv=member_priv;
	memb->func=func;
	if(type>0) {
		GHashTable *member_hash=rel->member_hash[type-1];
		g_hash_table_insert(member_hash, memb, g_list_append(g_hash_table_lookup(member_hash, memb), memb));
	} else
		rel->default_members=g_list_append(rel->default_members, memb);
}

/*
 * @brief Process relations members from the file.
 * @param in rel struct relations storing pre-processed relations info
 * @param in nodes file containing nodes in "coords.tmp" format
 * @param in ways file containing items in item_bin format. This file may contain both nodes, ways, and relations in that format.
 * @param unused relations
 */
void
relations_process(struct relations *rel, FILE *nodes, FILE *ways, FILE *relations)
{
	char buffer[128];
	struct item_bin *ib=(struct item_bin *)buffer;
	long long *id;
	struct coord *c=(struct coord *)(ib+1),cn={0,0};
	struct node_item *ni;
	GList *l;

	if (nodes) {
		item_bin_init(ib, type_point_unkn);
		item_bin_add_coord(ib, &cn, 1);
		item_bin_add_attr_longlong(ib, attr_osm_nodeid, 0);
		id=item_bin_get_attr(ib, attr_osm_nodeid, NULL);
		while ((ni=read_node_item(nodes))) {
			*id=ni->id;
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

static void
relations_destroy_func(void *key, GList *l, void *data)
{
	GList *ll=l;
	while (ll) {
		g_free(ll->data);
		ll=g_list_next(ll);
	}
	g_list_free(l);
}

void
relations_destroy(struct relations *relations)
{
	int i;

	for (i = 0 ; i < 3 ; i++) {
		g_hash_table_foreach(relations->member_hash[i], (GHFunc)relations_destroy_func, NULL);
		g_hash_table_destroy(relations->member_hash[i]);
	}
	g_free(relations);
}
