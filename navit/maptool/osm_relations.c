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
	struct relations *ret=g_new(struct relations, 1);
	int i;

	for (i = 0 ; i < 3 ; i++)
		ret->member_hash[i]=g_hash_table_new_full(relations_member_hash, relations_member_equal, NULL, NULL);
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

void
relations_add_func(struct relations *rel, struct relations_func *func, void *relation_priv, void *member_priv, int type, osmid id)
{
	GHashTable *member_hash=rel->member_hash[type-1];
	struct relations_member *memb=g_new(struct relations_member, 1);
	
	memb->memberid=id;
	memb->relation_priv=relation_priv;
	memb->member_priv=member_priv;
	memb->func=func;
	g_hash_table_insert(member_hash, memb, g_list_append(g_hash_table_lookup(member_hash, memb), memb));
}

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
			id=item_bin_get_attr(ib, attr_osm_wayid, NULL);
			if (id) {
				l=g_hash_table_lookup(rel->member_hash[1], id);
				while (l) {
					struct relations_member *memb=l->data;
					memb->func->func(memb->func->func_priv, memb->relation_priv, ib, memb->member_priv);
					l=g_list_next(l);
				}
			}
		}
	}
}
