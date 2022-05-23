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

#include <stdlib.h>
#include <glib.h>

#include "debug.h"

#include "quadtree.h"

#define QUADTREE_NODE_CAPACITY 10

#define MAX_DOUBLE 9999999
/* Check if two given line segments overlap (1-d case) */
#define segments_overlap(x11,x12, x21,x22) (((x11)<=(x21) && (x21)<=(x12)) || ((x21)<=(x11) && (x11)<=(x22)))
/* Check if two given rectanlgles overlap (2-d case) */
#define rects_overlap(x11,y11,x12,y12, x21,y21,x22,y22) (segments_overlap(x11,x12, x21,x22) && segments_overlap(y11,y12, y21,y22))

/* Structure describing quadtree iterative query */
struct quadtree_iter {
    /* List representing stack of quad_tree_iter_nodes referring to higher-level quadtree_nodes */
    GList *iter_nodes;
    double xmin,xmax,ymin,ymax;
    /* Current item pointer */
    struct quadtree_item *item;
    void (*item_free)(void *context, struct quadtree_item *qitem);
    void *item_free_context;
};

/* Structure describing one level of the quadtree iterative query */
struct quadtree_iter_node {
    struct quadtree_node *node;
    /* Number of subnode being analyzed (for non-leafs) */
    int subnode;
    /* Number of item being analyzed (for leafs) */
    int item;
    /* Number of subitems in items array (for leafs) */
    int node_num;
    /* If the node referenced was a leaf when it was analyzed */
    int is_leaf;
    struct quadtree_item *items[QUADTREE_NODE_CAPACITY];
};


static double dist_sq(double x1,double y1,double x2,double y2) {
    return (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);
}

struct quadtree_node*
quadtree_node_new(struct quadtree_node* parent, double xmin, double xmax, double ymin, double ymax ) {
    struct quadtree_node*ret = g_new0(struct quadtree_node,1);
    ret->xmin = xmin;
    ret->xmax = xmax;
    ret->ymin = ymin;
    ret->ymax = ymax;
    ret->is_leaf = 1;
    ret->parent = parent;
    return ret;
}

/*
 * searches all four subnodes recursively for the list of items within a rectangle
 */
void quadtree_find_rect_items(struct quadtree_node* this_, double dXMin, double dXMax, double dYMin, double dYMax,
                              GList**out) {

    struct quadtree_node* nodes[4] = { this_->aa, this_->ab, this_->ba, this_->bb };
    if( this_->is_leaf ) {
        int i;
        for(i=0; i<this_->node_num; ++i) {	//select only items within input rectangle
            if(dXMin<=this_->items[i]->longitude && this_->items[i]->longitude<=dXMax &&
                    dYMin<=this_->items[i]->latitude && this_->items[i]->latitude<=dYMax
              ) {
                *out=g_list_prepend(*out,this_->items[i]);
            }
        }
    } else {
        int i;
        for( i=0; i<4; ++i) {
            if(nodes[i] ) {
                //limit flooding
                if(nodes[i]->xmax<dXMin || dXMax<nodes[i]->xmin ||
                        nodes[i]->ymax<dYMin || dYMax<nodes[i]->ymin
                  ) {
                    continue;
                }
                //recurse into subtiles if there is at least one subtile corner within input rectangle
                quadtree_find_rect_items(nodes[i],dXMin,dXMax,dYMin,dYMax,out);
            }
        }
    }
}

/*
 * searches all four subnodes recursively for the closest item
 */
struct quadtree_item*
quadtree_find_nearest_flood(struct quadtree_node* this_, struct quadtree_item* item, double current_max,
                            struct quadtree_node* toSkip) {
    struct quadtree_node* nodes[4] = { this_->aa, this_->ab, this_->ba, this_->bb };
    struct quadtree_item*res = NULL;
    if( this_->is_leaf ) {
        int i;
        double distance_sq = current_max;
        for(i=0; i<this_->node_num; ++i) {
            double curr_dist_sq = dist_sq(item->longitude,item->latitude,this_->items[i]->longitude,this_->items[i]->latitude);
            if(curr_dist_sq<distance_sq) {
                distance_sq = curr_dist_sq;
                res = this_->items[i];
            }
        }
    } else {
        int i;
        for( i=0; i<4; ++i) {
            if(nodes[i] && nodes[i]!=toSkip) {
                //limit flooding
                struct quadtree_item*res_tmp = NULL;
                if(
                    dist_sq(nodes[i]->xmin,nodes[i]->ymin,item->longitude,item->latitude)<current_max ||
                    dist_sq(nodes[i]->xmax,nodes[i]->ymin,item->longitude,item->latitude)<current_max ||
                    dist_sq(nodes[i]->xmax,nodes[i]->ymax,item->longitude,item->latitude)<current_max ||
                    dist_sq(nodes[i]->xmin,nodes[i]->ymax,item->longitude,item->latitude)<current_max
                ) {
                    res_tmp = quadtree_find_nearest_flood(nodes[i],item,current_max,NULL);
                }
                if(res_tmp) {
                    double curr_dist_sq;
                    res = res_tmp;
                    curr_dist_sq = dist_sq(item->longitude,item->latitude,res->longitude,res->latitude);
                    if(curr_dist_sq<current_max) {
                        current_max = curr_dist_sq;
                    }
                }
            }
        }
    }
    return res;
}

/*
 * tries to find an item exactly
 */
struct quadtree_item*
quadtree_find_item(struct quadtree_node* this_, struct quadtree_item* item) {
    struct quadtree_item*res = NULL;
    if( ! this_ ) {
        return NULL;
    }
    if( this_->is_leaf ) {
        int i;
        for(i=0; i<this_->node_num; ++i) {
            //TODO equality check may not be correct on float values! maybe it can be replaced by range check with some tolerance
            if(item->longitude==this_->items[i]->longitude && item->latitude==this_->items[i]->latitude) {
                res = this_->items[i];
                return res;
            }
        }
        return NULL;
    } else {
        if(
            this_->aa &&
            this_->aa->xmin<=item->longitude && item->longitude<this_->aa->xmax &&
            this_->aa->ymin<=item->latitude && item->latitude<this_->aa->ymax
        ) {
            res = quadtree_find_item(this_->aa,item);
        } else if(
            this_->ab &&
            this_->ab->xmin<=item->longitude && item->longitude<this_->ab->xmax &&
            this_->ab->ymin<=item->latitude && item->latitude<this_->ab->ymax
        ) {
            res = quadtree_find_item(this_->ab,item);
        } else if(
            this_->ba &&
            this_->ba->xmin<=item->longitude && item->longitude<this_->ba->xmax &&
            this_->ba->ymin<=item->latitude && item->latitude<this_->ba->ymax
        ) {
            res = quadtree_find_item(this_->ba,item);
        } else if(
            this_->bb &&
            this_->bb->xmin<=item->longitude && item->longitude<this_->bb->xmax &&
            this_->bb->ymin<=item->latitude && item->latitude<this_->bb->ymax
        ) {
            res = quadtree_find_item(this_->bb,item);
        } else {
            return NULL;
        }
    }
    return res;
}

/*
 * returns the containing node for an item
 */
struct quadtree_node*
quadtree_find_containing_node(struct quadtree_node* root, struct quadtree_item* item) {
    struct quadtree_node*res = NULL;

    if( ! root ) {
        return NULL;
    }

    if( root->is_leaf ) {
        int i;
        for(i=0; i<root->node_num; ++i) {
            if(item == root->items[i]) {
                res = root;
            }
        }
    } else {
        if(
            root->aa &&
            root->aa->xmin<=item->longitude && item->longitude<root->aa->xmax &&
            root->aa->ymin<=item->latitude && item->latitude<root->aa->ymax
        ) {
            res = quadtree_find_containing_node(root->aa,item);
        } else if(
            root->ab &&
            root->ab->xmin<=item->longitude && item->longitude<root->ab->xmax &&
            root->ab->ymin<=item->latitude && item->latitude<root->ab->ymax
        ) {
            res = quadtree_find_containing_node(root->ab,item);
        } else if(
            root->ba &&
            root->ba->xmin<=item->longitude && item->longitude<root->ba->xmax &&
            root->ba->ymin<=item->latitude && item->latitude<root->ba->ymax
        ) {
            res = quadtree_find_containing_node(root->ba,item);
        } else if(
            root->bb &&
            root->bb->xmin<=item->longitude && item->longitude<root->bb->xmax &&
            root->bb->ymin<=item->latitude && item->latitude<root->bb->ymax
        ) {
            res = quadtree_find_containing_node(root->bb,item);
        } else {
            //this should not happen
        }
    }
    return res;
}


int  quadtree_delete_item(struct quadtree_node* root, struct quadtree_item* item) {

    struct quadtree_node* qn = quadtree_find_containing_node(root,item);
    int i, bFound=0;

    if(!qn || !qn->node_num) {
        return 0;
    }

    for(i=0; i<qn->node_num; ++i) {
        if( qn->items[i] == item) {
            qn->items[i]->deleted=1;
            bFound=1;
        }
    }
    return bFound;
}


/*
 * tries to find closest item, first it descend into the quadtree as much as possible, then if no point is found go up n levels and flood
 */
struct quadtree_item*
quadtree_find_nearest(struct quadtree_node* this_, struct quadtree_item* item) {
    struct quadtree_item*res = NULL;
    double distance_sq = MAX_DOUBLE;
    if( ! this_ ) {
        return NULL;
    }
    if( this_->is_leaf ) {
        int i;
        for(i=0; i<this_->node_num; ++i) {
            double curr_dist_sq = dist_sq(item->longitude,item->latitude,this_->items[i]->longitude,this_->items[i]->latitude);
            if(curr_dist_sq<distance_sq) {
                distance_sq = curr_dist_sq;
                res = this_->items[i];
            }
        }
        //go up n levels
        if(!res && this_->parent) {
            struct quadtree_item*res2 = NULL;
            struct quadtree_node* anchestor = this_->parent;
            int cnt = 0;
            while (anchestor->parent && cnt<4) {
                anchestor = anchestor->parent;
                ++cnt;
            }
            res2 = quadtree_find_nearest_flood(anchestor,item,distance_sq,NULL);
            if(res2) {
                res = res2;
            }
        }
    } else {
        if(
            this_->aa &&
            this_->aa->xmin<=item->longitude && item->longitude<this_->aa->xmax &&
            this_->aa->ymin<=item->latitude && item->latitude<this_->aa->ymax
        ) {
            res = quadtree_find_nearest(this_->aa,item);
        } else if(
            this_->ab &&
            this_->ab->xmin<=item->longitude && item->longitude<this_->ab->xmax &&
            this_->ab->ymin<=item->latitude && item->latitude<this_->ab->ymax
        ) {
            res = quadtree_find_nearest(this_->ab,item);
        } else if(
            this_->ba &&
            this_->ba->xmin<=item->longitude && item->longitude<this_->ba->xmax &&
            this_->ba->ymin<=item->latitude && item->latitude<this_->ba->ymax
        ) {
            res = quadtree_find_nearest(this_->ba,item);
        } else if(
            this_->bb &&
            this_->bb->xmin<=item->longitude && item->longitude<this_->bb->xmax &&
            this_->bb->ymin<=item->latitude && item->latitude<this_->bb->ymax
        ) {
            res = quadtree_find_nearest(this_->bb,item);
        } else {
            if(this_->parent) {
                //go up two levels
                struct quadtree_node* anchestor = this_->parent;
                int cnt = 0;
                while (anchestor->parent && cnt<4) {
                    anchestor = anchestor->parent;
                    ++cnt;
                }
                res = quadtree_find_nearest_flood(anchestor,item,distance_sq,NULL);
            }
        }
    }
    return res;

}


/**
 * @brief Free space occupied by deleted unreferenced items.
 * @param node pointer to the quadtree node
 * @param iter Quadtree iteration context.
 * @return nothing
 */
void quadtree_node_drop_garbage(struct quadtree_node* node, struct quadtree_iter *iter) {
    int i,j;
    int node_num=node->node_num;
    dbg(lvl_debug,"Processing unreferenced subnode children...");
    for(i=0,j=0; i<node_num; i++) {
        if(node->items[i]->deleted && !node->items[i]->ref_count) {
            if(iter->item_free) {
                (iter->item_free)(iter->item_free_context, node->items[i]);
            } else {
                g_free(node->items[i]);
            }
            node->node_num--;
            node->items[i]=NULL;
        } else {
            node->items[j++]=node->items[i];
        }
        if(i>j)
            node->items[i]=NULL;
    }
}

/**
 * @brief Add new node to quadtree.
 * @param this_ pointer to the quadtree (root) node
 * @param item item to add
 * @param iter Quadtree iteration context. Can be NULL if no garbage collection is needed.
 * @return nothing
 */
void quadtree_add(struct quadtree_node* this_, struct quadtree_item* item, struct quadtree_iter *iter) {
    if( this_->is_leaf ) {
        int bSame = 1;
        int i;

        if(iter)
            quadtree_node_drop_garbage(this_, iter);

        if(QUADTREE_NODE_CAPACITY-1 == this_->node_num) {
            double lon, lat;
            //avoid infinite recursion when all elements have the same coordinate
            lon = this_->items[0]->longitude;
            lat = this_->items[0]->latitude;
            for(i=1; i<this_->node_num; ++i) {
                if (lon != this_->items[i]->longitude || lat != this_->items[i]->latitude) {
                    bSame = 0;
                    break;
                }
            }
            if (bSame) {
                //FIXME: memleak and items thrown away if more than QUADTREE_NODE_CAPACITY-1 items with same coordinates added.
                dbg(lvl_error,"Unable to add another item with same coordinates. Throwing item to the ground. Will leak %p.",item);
                return;
            }
            this_->items[this_->node_num++] = item;
            quadtree_split(this_);
        } else {
            this_->items[this_->node_num++] = item;
        }
    } else {
        if(
            this_->xmin<=item->longitude && item->longitude<this_->xmin+(this_->xmax-this_->xmin)/2.0 &&
            this_->ymin<=item->latitude && item->latitude<this_->ymin+(this_->ymax-this_->ymin)/2.0
        ) {
            if(!this_->aa) {
                this_->aa = quadtree_node_new( this_, this_->xmin, this_->xmin+(this_->xmax-this_->xmin)/2.0, this_->ymin,
                                               this_->ymin+(this_->ymax-this_->ymin)/2.0 );
            }
            quadtree_add(this_->aa,item,iter);
        } else if(
            this_->xmin+(this_->xmax-this_->xmin)/2.0<=item->longitude && item->longitude<this_->xmax &&
            this_->ymin<=item->latitude && item->latitude<this_->ymin+(this_->ymax-this_->ymin)/2.0
        ) {
            if(!this_->ab) {
                this_->ab = quadtree_node_new( this_, this_->xmin+(this_->xmax-this_->xmin)/2.0, this_->xmax, this_->ymin,
                                               this_->ymin+(this_->ymax-this_->ymin)/2.0 );
            }
            quadtree_add(this_->ab,item,iter);
        } else if(
            this_->xmin<=item->longitude && item->longitude<this_->xmin+(this_->xmax-this_->xmin)/2.0 &&
            this_->ymin+(this_->ymax-this_->ymin)/2.0<=item->latitude && item->latitude<this_->ymax
        ) {
            if(!this_->ba) {
                this_->ba = quadtree_node_new( this_, this_->xmin, this_->xmin+(this_->xmax-this_->xmin)/2.0,
                                               this_->ymin+(this_->ymax-this_->ymin)/2.0, this_->ymax);
            }
            quadtree_add(this_->ba,item,iter);
        } else if(
            this_->xmin+(this_->xmax-this_->xmin)/2.0<=item->longitude && item->longitude<this_->xmax &&
            this_->ymin+(this_->ymax-this_->ymin)/2.0<=item->latitude && item->latitude<this_->ymax
        ) {
            if(!this_->bb) {
                this_->bb = quadtree_node_new( this_, this_->xmin+(this_->xmax-this_->xmin)/2.0, this_->xmax,
                                               this_->ymin+(this_->ymax-this_->ymin)/2.0, this_->ymax);
            }
            quadtree_add(this_->bb,item,iter);
        }
    }
}

void quadtree_split(struct quadtree_node* this_) {
    int i;
    this_->is_leaf = 0;
    for(i=0; i<this_->node_num; ++i) {
        if(
            this_->xmin<=this_->items[i]->longitude && this_->items[i]->longitude<this_->xmin+(this_->xmax-this_->xmin)/2.0 &&
            this_->ymin<=this_->items[i]->latitude && this_->items[i]->latitude<this_->ymin+(this_->ymax-this_->ymin)/2.0
        ) {
            if(!this_->aa) {
                this_->aa = quadtree_node_new( this_, this_->xmin, this_->xmin+(this_->xmax-this_->xmin)/2.0, this_->ymin,
                                               this_->ymin+(this_->ymax-this_->ymin)/2.0 );
            }
            quadtree_add(this_->aa,this_->items[i],NULL);
        } else if(
            this_->xmin+(this_->xmax-this_->xmin)/2.0<=this_->items[i]->longitude && this_->items[i]->longitude<this_->xmax &&
            this_->ymin<=this_->items[i]->latitude && this_->items[i]->latitude<this_->ymin+(this_->ymax-this_->ymin)/2.0
        ) {
            if(!this_->ab) {
                this_->ab = quadtree_node_new( this_, this_->xmin+(this_->xmax-this_->xmin)/2.0, this_->xmax, this_->ymin,
                                               this_->ymin+(this_->ymax-this_->ymin)/2.0 );
            }
            quadtree_add(this_->ab,this_->items[i],NULL);
        } else if(
            this_->xmin<=this_->items[i]->longitude && this_->items[i]->longitude<this_->xmin+(this_->xmax-this_->xmin)/2.0 &&
            this_->ymin+(this_->ymax-this_->ymin)/2.0<=this_->items[i]->latitude && this_->items[i]->latitude<this_->ymax
        ) {
            if(!this_->ba) {
                this_->ba = quadtree_node_new( this_, this_->xmin, this_->xmin+(this_->xmax-this_->xmin)/2.0,
                                               this_->ymin+(this_->ymax-this_->ymin)/2.0, this_->ymax);
            }
            quadtree_add(this_->ba,this_->items[i],NULL);
        } else if(
            this_->xmin+(this_->xmax-this_->xmin)/2.0<=this_->items[i]->longitude && this_->items[i]->longitude<this_->xmax &&
            this_->ymin+(this_->ymax-this_->ymin)/2.0<=this_->items[i]->latitude && this_->items[i]->latitude<this_->ymax
        ) {
            if(!this_->bb) {
                this_->bb = quadtree_node_new( this_, this_->xmin+(this_->xmax-this_->xmin)/2.0, this_->xmax,
                                               this_->ymin+(this_->ymax-this_->ymin)/2.0, this_->ymax);
            }
            quadtree_add(this_->bb,this_->items[i],NULL);
        }
        this_->items[i]=NULL;
    }
    this_->node_num = 0;
}

void quadtree_destroy(struct quadtree_node* this_) {
    if(this_->aa) {
        quadtree_destroy(this_->aa);
        this_->aa = NULL;
    }
    if(this_->ab) {
        quadtree_destroy(this_->ab);
        this_->ab = NULL;
    }
    if(this_->ba) {
        quadtree_destroy(this_->ba);
        this_->ba = NULL;
    }
    if(this_->bb) {
        quadtree_destroy(this_->bb);
        this_->bb = NULL;
    }
    free(this_);
}


/*
 * @brief Start iterating over quadtree items not marked for deletion.
 * @param this_ Pointer to the quadtree root node.
 * @param dXMin,dXMax,dYMin,dYMax boundig box of area of interest.
 * @param item_free function to be called to free the qitem, first parameter would be context, second parameter - actual qitem pointer to free
 *		if this parameter is NULL, nothing would be called to free qitem pointer (possible massive memleak unless you store each qitem pointer in some
 *		alternate storage)
 * @param item_free_context data to be passed as a first parameter to item_free function
 * @return pointer to the quad tree iteration structure.
 */
struct quadtree_iter *quadtree_query(struct quadtree_node *this_, double dXMin, double dXMax, double dYMin,
                                     double dYMax,void (*item_free)(void *context, struct quadtree_item *qitem), void *item_free_context) {
    struct quadtree_iter *ret=g_new0(struct quadtree_iter,1);
    struct quadtree_iter_node *n=g_new0(struct quadtree_iter_node,1);
    ret->xmin=dXMin;
    ret->xmax=dXMax;
    ret->ymin=dYMin;
    ret->ymax=dYMax;
    dbg(lvl_debug,"%f %f %f %f",dXMin,dXMax,dYMin,dYMax)
    ret->item_free=item_free;
    ret->item_free_context=item_free_context;
    n->node=this_;
    ret->iter_nodes=g_list_prepend(ret->iter_nodes,n);
    n->is_leaf=this_->is_leaf;
    if(this_->is_leaf) {
        int i;
        n->node_num=this_->node_num;
        for(i=0; i<n->node_num; i++) {
            n->items[i]=this_->items[i];
            n->items[i]->ref_count++;
        }
    }

    this_->ref_count++;
    dbg(lvl_debug,"Query %p ",this_)
    return ret;

}

/*
 * @brief End iteration.
 * @param iter Pointer to the quadtree iteration structure.
 */
void quadtree_query_free(struct quadtree_iter *iter) {
    //Get the rest of the data to collect garbage and dereference all the items/nodes
    //TODO: No need to iterate the whole tree here. Just dereference nodes/items (if any).
    while(quadtree_item_next(iter));
    g_free(iter);
}


/*
 * @brief Mark current item of iterator for deletion.
 * @param iter Pointer to the quadtree iteration structure.
 */
void quadtree_item_delete(struct quadtree_iter *iter) {
    if(iter->item)
        iter->item->deleted=1;
}

/*
 * @brief Get next item from the quadtree. Any  met unreferenced items/nodes marked for deletion will be freed here.
 * @param iter Pointer to the quadtree iteration structure.
 * @return pointer to the next item, or NULL if no items are left.
 */
struct quadtree_item * quadtree_item_next(struct quadtree_iter *iter) {
    struct quadtree_iter_node *iter_node;
    struct quadtree_node *subnode;

    if(iter->item) {
        iter->item->ref_count--;
        iter->item=NULL;
    }

    while(iter->iter_nodes)  {
        struct quadtree_node *nodes[4];
        int i;
        iter_node=iter->iter_nodes->data;

        if(iter_node->is_leaf) {
            /* Try to find undeleted item in the current node */
            dbg(lvl_debug,"find item %p %p ...",iter->iter_nodes,iter->iter_nodes->data);
            while(iter_node->item<iter_node->node_num) {
                dbg(lvl_debug,"%d %d",iter_node->item,iter_node->items[iter_node->item]->deleted);
                if(iter_node->items[iter_node->item]->deleted) {
                    iter_node->item++;
                    continue;
                }
                iter->item=iter_node->items[iter_node->item];
                iter_node->item++;
                dbg(lvl_debug,"Returning %p",iter->item);
                iter->item->ref_count++;
                return iter->item;
            }
            for(i=0; i<iter_node->node_num; i++) {
                iter_node->items[i]->ref_count--;
            }
        } else {
            /* No items left, try to find non-empty subnode */
            nodes[0]=iter_node->node->aa;
            nodes[1]=iter_node->node->ab;
            nodes[2]=iter_node->node->ba;
            nodes[3]=iter_node->node->bb;

            for(subnode=NULL; !subnode && iter_node->subnode<4; iter_node->subnode++) {
                i=iter_node->subnode;
                if(!nodes[i]
                        || !rects_overlap(nodes[i]->xmin, nodes[i]->ymin, nodes[i]->xmax, nodes[i]->ymax, iter->xmin, iter->ymin, iter->xmax,
                                          iter->ymax))
                    continue;
                dbg(lvl_debug,"%f %f %f %f",nodes[i]->xmin, nodes[i]->xmax, nodes[i]->ymin, nodes[i]->ymax)
                subnode=nodes[i];
            }

            if(subnode) {
                /* Go one level deeper */
                dbg(lvl_debug,"Go one level deeper...");
                iter_node=g_new0(struct quadtree_iter_node, 1);
                iter_node->node=subnode;
                iter_node->is_leaf=subnode->is_leaf;
                if(iter_node->is_leaf) {
                    int i;
                    iter_node->node_num=subnode->node_num;
                    for(i=0; i<iter_node->node_num; i++) {
                        iter_node->items[i]=subnode->items[i];
                        iter_node->items[i]->ref_count++;
                    }
                }
                subnode->ref_count++;
                iter->iter_nodes=g_list_prepend(iter->iter_nodes,iter_node);
                continue;
            }
        }

        /* No nodes and items left, fix up current subnode... */
        iter_node=iter->iter_nodes->data;
        subnode=iter_node->node;
        subnode->ref_count--;

        if(!subnode->aa && !subnode->ab && !subnode->ba && !subnode->bb)
            subnode->is_leaf=1;

        /*  1. free deleted unreferenced items */
        quadtree_node_drop_garbage(subnode, iter);

        /* 2. remove empty leaf subnode if it's unreferenced */

        if(!subnode->ref_count && !subnode->node_num && subnode->is_leaf ) {
            dbg(lvl_debug,"Going to delete an empty unreferenced leaf subnode...");

            if(subnode->parent) {
                if(subnode->parent->aa==subnode) {
                    subnode->parent->aa=NULL;
                } else if(subnode->parent->ab==subnode) {
                    subnode->parent->ab=NULL;
                } else if(subnode->parent->ba==subnode) {
                    subnode->parent->ba=NULL;
                } else if(subnode->parent->bb==subnode) {
                    subnode->parent->bb=NULL;
                } else {
                    dbg(lvl_error,"Found Quadtree structure corruption while trying to free an empty node.");
                }

                if(!subnode->parent->aa && !subnode->parent->ab && !subnode->parent->ba && !subnode->parent->bb )
                    subnode->parent->is_leaf=1;
                g_free(subnode);
            } else
                dbg(lvl_debug,"Quadtree is empty. NOT deleting the root subnode...");

        }

        /* Go one step towards root */
        dbg(lvl_info,"Going towards root...");
        g_free(iter->iter_nodes->data);
        iter->iter_nodes=g_list_delete_link(iter->iter_nodes,iter->iter_nodes);
    }

    iter->item=NULL;
    return NULL;
}

