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

#include <stdio.h>
#include <glib.h>

#include "attr.h"
#include "coord.h"
#include "quadtree.h"


struct map_priv {
	int id;
	struct quadtree_node* tree_root;
	int flags;
	GHashTable*qitem_hash;
	char* filename;
	/*need to write map file on exit*/
	int dirty;
	int attr_cnt;
	enum attr_type *attr_types;
	int next_item_idx;
	enum item_type item_type;
	/*list of quadtree items that have no coord set yet ()*/
	GList* new_items;
	char *charset;
};

struct map_rect_priv {
	struct map_selection *sel;
	struct quadtree_iter *qiter;
	struct quadtree_item *qitem;
	struct coord c;
	int bStarted;
	struct item item;
	struct map_priv *m;
	GList* at_iter;
};

