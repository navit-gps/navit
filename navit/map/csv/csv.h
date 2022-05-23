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
	struct quadtree_node* tree_root; /**< Root of the quadtree from which items can be retrieved by their coordinates */
	int flags;
	GHashTable*qitem_hash;           /**< Hash table to retrieve items by their ID */
	char* filename;                  /**< Name of the file in which the map is stored */
	int dirty;                       /**< Need to write map file on exit */
	int attr_cnt;                    /**< Number of elements in `attr_types` */
	enum attr_type *attr_types;      /**< Array of attribute types supported by this map */
	int next_item_idx;               /**< Zero-based index (`id_lo`) for the next item to be added */
	enum item_type item_type;        /**< Item type stored in this map */
	GList* new_items;                /**< List of quadtree items that have no coord set yet */
	char *charset;                   /**< Identifier for the character set of this map */
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

