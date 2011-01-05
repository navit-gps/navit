#ifndef QUADTREE_H
#define QUADTREE_H

#include <glib.h>

#define QUADTREE_NODE_CAPACITY 10

struct quadtree_item {
    double longitude;
    double latitude;
    void*  data;
};

struct quadtree_node {
    int node_num;
    struct quadtree_item items[QUADTREE_NODE_CAPACITY];
    struct quadtree_node *aa;
    struct quadtree_node *ab;
    struct quadtree_node *ba;
    struct quadtree_node *bb;
    double xmin, xmax, ymin, ymax;
    int is_leaf;
    struct quadtree_node*parent;
};

struct quadtree_node* quadtree_node_new(struct quadtree_node* parent, double xmin, double xmax, double ymin, double ymax );
struct quadtree_item* quadtree_find_nearest_flood(struct quadtree_node* this_, struct quadtree_item* item, double current_max, struct quadtree_node* toSkip);
struct quadtree_item* quadtree_find_nearest(struct quadtree_node* this_, struct quadtree_item* item);
void quadtree_find_rect_items(struct quadtree_node* this_, double dXMin, double dXMax, double dYMin, double dYMax, GList**out);
void quadtree_split(struct quadtree_node* this_);
void quadtree_add(struct quadtree_node* this_, struct quadtree_item* item);
void quadtree_destroy(struct quadtree_node* this_);



#endif
