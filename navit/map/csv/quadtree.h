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

#ifndef QUADTREE_H
#define QUADTREE_H

#include <glib.h>

#define QUADTREE_NODE_CAPACITY 10

struct quadtree_item {
    double longitude;
    double latitude;
    int ref_count;
    int deleted;
    void *data;
};

struct quadtree_node {
    int node_num;
    struct quadtree_item *items[QUADTREE_NODE_CAPACITY];
    struct quadtree_node *aa;
    struct quadtree_node *ab;
    struct quadtree_node *ba;
    struct quadtree_node *bb;
    double xmin, xmax, ymin, ymax;
    int is_leaf;
    struct quadtree_node*parent;
    int ref_count;
};

struct quadtree_iter;

struct quadtree_node* quadtree_node_new(struct quadtree_node* parent, double xmin, double xmax, double ymin, double ymax );
struct quadtree_item* quadtree_find_nearest_flood(struct quadtree_node* this_, struct quadtree_item* item, double current_max, struct quadtree_node* toSkip);
struct quadtree_item* quadtree_find_nearest(struct quadtree_node* this_, struct quadtree_item* item);
struct quadtree_item* quadtree_find_item(struct quadtree_node* this_, struct quadtree_item* item);
struct quadtree_node* quadtree_find_containing_node(struct quadtree_node* root, struct quadtree_item* item);
int  quadtree_delete_item(struct quadtree_node* root, struct quadtree_item* item);
void quadtree_find_rect_items(struct quadtree_node* this_, double dXMin, double dXMax, double dYMin, double dYMax, GList**out);
void quadtree_split(struct quadtree_node* this_);
void quadtree_add(struct quadtree_node* this_, struct quadtree_item* item, struct quadtree_iter* iter);
void quadtree_destroy(struct quadtree_node* this_);
struct quadtree_iter *quadtree_query(struct quadtree_node *this_, double dXMin, double dXMax, double dYMin, double dYMax,void (*item_free)(void *context, struct quadtree_item *qitem), void *context);
struct quadtree_item * quadtree_item_next(struct quadtree_iter *iter);
void quadtree_query_free(struct quadtree_iter *iter);
void quadtree_item_delete(struct quadtree_iter *iter);
struct quadtree_data *quadtree_data_dup(struct quadtree_data *qdata);
void quadtree_node_drop_garbage(struct quadtree_node* node, struct quadtree_iter *iter);

#endif
