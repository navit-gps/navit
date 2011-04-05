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

#include "quadtree.h"

#define QUADTREE_NODE_CAPACITY 10

#define MAX_DOUBLE 9999999

static double 
dist_sq(double x1,double y1,double x2,double y2)
{
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
void 
quadtree_find_rect_items(struct quadtree_node* this_, double dXMin, double dXMax, double dYMin, double dYMax, GList**out) {

    struct quadtree_node* nodes[4] = { this_->aa, this_->ab, this_->ba, this_->bb };
    if( this_->is_leaf ) { 
        int i;
        //double distance_sq = current_max;
        for(i=0;i<this_->node_num;++i) {	//select only items within input rectangle
		if(dXMin<=this_->items[i].longitude && this_->items[i].longitude<=dXMax &&
		   dYMin<=this_->items[i].latitude && this_->items[i].latitude<=dYMax
		  ) {
		    *out=g_list_prepend(*out,&this_->items[i]);
		}
        }
    }
    else {
      int i;
      for( i=0;i<4;++i) {
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
quadtree_find_nearest_flood(struct quadtree_node* this_, struct quadtree_item* item, double current_max, struct quadtree_node* toSkip) {
    struct quadtree_node* nodes[4] = { this_->aa, this_->ab, this_->ba, this_->bb };
    struct quadtree_item*res = NULL;
    if( this_->is_leaf ) { 
        int i;
        double distance_sq = current_max;
        for(i=0;i<this_->node_num;++i) {
            double curr_dist_sq = dist_sq(item->longitude,item->latitude,this_->items[i].longitude,this_->items[i].latitude);
            if(curr_dist_sq<distance_sq) {
                distance_sq = curr_dist_sq;
                res = &this_->items[i];
            }
        }
    }
    else {
      int i;
      for( i=0;i<4;++i) {
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
        for(i=0;i<this_->node_num;++i) {
            //TODO equality check may not be correct on float values! maybe it can be replaced by range check with some tolerance
            if(item->longitude==this_->items[i].longitude && item->latitude==this_->items[i].latitude) {
                res = &this_->items[i];
                return res;
            }
        }
        return NULL;	
    }
    else {
        if(
	   this_->aa && 
           this_->aa->xmin<=item->longitude && item->longitude<this_->aa->xmax &&
           this_->aa->ymin<=item->latitude && item->latitude<this_->aa->ymax
           ) {
          res = quadtree_find_item(this_->aa,item);
        }
        else if(
	   this_->ab && 
           this_->ab->xmin<=item->longitude && item->longitude<this_->ab->xmax &&
           this_->ab->ymin<=item->latitude && item->latitude<this_->ab->ymax
           ) {
          res = quadtree_find_item(this_->ab,item);
        }
        else if(
	   this_->ba && 
           this_->ba->xmin<=item->longitude && item->longitude<this_->ba->xmax &&
           this_->ba->ymin<=item->latitude && item->latitude<this_->ba->ymax
           ) {
          res = quadtree_find_item(this_->ba,item);
        }
        else if(
	   this_->bb && 
           this_->bb->xmin<=item->longitude && item->longitude<this_->bb->xmax &&
           this_->bb->ymin<=item->latitude && item->latitude<this_->bb->ymax
           ) {
          res = quadtree_find_item(this_->bb,item);
        }
	else {
          return NULL;
        }
    }
  return res;
}

/*
 * returns the containing node for an item
 */
struct quadtree_node* 
quadtree_find_containing_node(struct quadtree_node* root, struct quadtree_item* item) 
{
    struct quadtree_node*res = NULL;

    if( ! root ) {
      return NULL;
    }

    if( root->is_leaf ) { 
        int i;
        for(i=0;i<root->node_num;++i) {
            if(item == &root->items[i]) {
                res = root;
            }
        }
    }
    else {
        if(
	   root->aa && 
           root->aa->xmin<=item->longitude && item->longitude<root->aa->xmax &&
           root->aa->ymin<=item->latitude && item->latitude<root->aa->ymax
           ) {
          res = quadtree_find_containing_node(root->aa,item);
        }
        else if(
	   root->ab && 
           root->ab->xmin<=item->longitude && item->longitude<root->ab->xmax &&
           root->ab->ymin<=item->latitude && item->latitude<root->ab->ymax
           ) {
          res = quadtree_find_containing_node(root->ab,item);
        }
        else if(
	   root->ba && 
           root->ba->xmin<=item->longitude && item->longitude<root->ba->xmax &&
           root->ba->ymin<=item->latitude && item->latitude<root->ba->ymax
           ) {
          res = quadtree_find_containing_node(root->ba,item);
        }
        else if(
	   root->bb && 
           root->bb->xmin<=item->longitude && item->longitude<root->bb->xmax &&
           root->bb->ymin<=item->latitude && item->latitude<root->bb->ymax
           ) {
          res = quadtree_find_containing_node(root->bb,item);
        }
	else {
          //this should not happen
        }
    }
  return res;
}


int  quadtree_delete_item(struct quadtree_node* root, struct quadtree_item* item)
{
  struct quadtree_node* qn = quadtree_find_containing_node(root,item);
  int i, bFound = 0;

  if(!qn || !qn->node_num) {
    return 0;
  }

  for(i=0;i<qn->node_num;++i) {
    if( &qn->items[i] == item) {
      bFound = 1;
    }
    if(bFound && i<qn->node_num-1) {
      qn->items[i] = qn->items[i+1];
    }
  }
  if(bFound) {
    --qn->node_num;
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
        for(i=0;i<this_->node_num;++i) {
            double curr_dist_sq = dist_sq(item->longitude,item->latitude,this_->items[i].longitude,this_->items[i].latitude);
            if(curr_dist_sq<distance_sq) {
                distance_sq = curr_dist_sq;
                res = &this_->items[i];
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
    }
    else {
        if(
	   this_->aa && 
           this_->aa->xmin<=item->longitude && item->longitude<this_->aa->xmax &&
           this_->aa->ymin<=item->latitude && item->latitude<this_->aa->ymax
           ) {
          res = quadtree_find_nearest(this_->aa,item);
        }
        else if(
	   this_->ab && 
           this_->ab->xmin<=item->longitude && item->longitude<this_->ab->xmax &&
           this_->ab->ymin<=item->latitude && item->latitude<this_->ab->ymax
           ) {
          res = quadtree_find_nearest(this_->ab,item);
        }
        else if(
	   this_->ba && 
           this_->ba->xmin<=item->longitude && item->longitude<this_->ba->xmax &&
           this_->ba->ymin<=item->latitude && item->latitude<this_->ba->ymax
           ) {
          res = quadtree_find_nearest(this_->ba,item);
        }
        else if(
	   this_->bb && 
           this_->bb->xmin<=item->longitude && item->longitude<this_->bb->xmax &&
           this_->bb->ymin<=item->latitude && item->latitude<this_->bb->ymax
           ) {
          res = quadtree_find_nearest(this_->bb,item);
        }
	else {
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

void
quadtree_add(struct quadtree_node* this_, struct quadtree_item* item) {
    if( this_->is_leaf ) {
        this_->items[this_->node_num++] = *item;
        if(QUADTREE_NODE_CAPACITY == this_->node_num) {
            quadtree_split(this_);
        }
    }
    else {
        if(
           this_->xmin<=item->longitude && item->longitude<this_->xmin+(this_->xmax-this_->xmin)/2.0 &&
           this_->ymin<=item->latitude && item->latitude<this_->ymin+(this_->ymax-this_->ymin)/2.0
           ) {
	    if(!this_->aa) {
              this_->aa = quadtree_node_new( this_, this_->xmin, this_->xmin+(this_->xmax-this_->xmin)/2.0 , this_->ymin, this_->ymin+(this_->ymax-this_->ymin)/2.0 );
	    }
          quadtree_add(this_->aa,item);
        }
        else if(
           this_->xmin+(this_->xmax-this_->xmin)/2.0<=item->longitude && item->longitude<this_->xmax &&
           this_->ymin<=item->latitude && item->latitude<this_->ymin+(this_->ymax-this_->ymin)/2.0
           ) {
	    if(!this_->ab) {
              this_->ab = quadtree_node_new( this_, this_->xmin+(this_->xmax-this_->xmin)/2.0, this_->xmax , this_->ymin, this_->ymin+(this_->ymax-this_->ymin)/2.0 );
	    }
          quadtree_add(this_->ab,item);
        }
        else if(
           this_->xmin<=item->longitude && item->longitude<this_->xmin+(this_->xmax-this_->xmin)/2.0 &&
           this_->ymin+(this_->ymax-this_->ymin)/2.0<=item->latitude && item->latitude<this_->ymax
           ) {
	    if(!this_->ba) {
              this_->ba = quadtree_node_new( this_, this_->xmin, this_->xmin+(this_->xmax-this_->xmin)/2.0 , this_->ymin+(this_->ymax-this_->ymin)/2.0 , this_->ymax);
	    }
          quadtree_add(this_->ba,item);
        }
        else if(
           this_->xmin+(this_->xmax-this_->xmin)/2.0<=item->longitude && item->longitude<this_->xmax &&
           this_->ymin+(this_->ymax-this_->ymin)/2.0<=item->latitude && item->latitude<this_->ymax
           ) {
	    if(!this_->bb) {
              this_->bb = quadtree_node_new( this_, this_->xmin+(this_->xmax-this_->xmin)/2.0, this_->xmax , this_->ymin+(this_->ymax-this_->ymin)/2.0 , this_->ymax);
	    }
          quadtree_add(this_->bb,item);
        }
    }
}

void
quadtree_split(struct quadtree_node* this_) {
    int i;
    this_->is_leaf = 0;
    for(i=0;i<this_->node_num;++i) {
        if(
           this_->xmin<=this_->items[i].longitude && this_->items[i].longitude<this_->xmin+(this_->xmax-this_->xmin)/2.0 &&
           this_->ymin<=this_->items[i].latitude && this_->items[i].latitude<this_->ymin+(this_->ymax-this_->ymin)/2.0
           ) {
	  if(!this_->aa) {
            this_->aa = quadtree_node_new( this_, this_->xmin, this_->xmin+(this_->xmax-this_->xmin)/2.0 , this_->ymin, this_->ymin+(this_->ymax-this_->ymin)/2.0 );
	  }
          quadtree_add(this_->aa,&this_->items[i]);
        }
        else if(
           this_->xmin+(this_->xmax-this_->xmin)/2.0<=this_->items[i].longitude && this_->items[i].longitude<this_->xmax &&
           this_->ymin<=this_->items[i].latitude && this_->items[i].latitude<this_->ymin+(this_->ymax-this_->ymin)/2.0
           ) {
	  if(!this_->ab) {
            this_->ab = quadtree_node_new( this_, this_->xmin+(this_->xmax-this_->xmin)/2.0, this_->xmax , this_->ymin, this_->ymin+(this_->ymax-this_->ymin)/2.0 );
	  }
          quadtree_add(this_->ab,&this_->items[i]);
        }
        else if(
           this_->xmin<=this_->items[i].longitude && this_->items[i].longitude<this_->xmin+(this_->xmax-this_->xmin)/2.0 &&
           this_->ymin+(this_->ymax-this_->ymin)/2.0<=this_->items[i].latitude && this_->items[i].latitude<this_->ymax
           ) {
	  if(!this_->ba) {
            this_->ba = quadtree_node_new( this_, this_->xmin, this_->xmin+(this_->xmax-this_->xmin)/2.0 , this_->ymin+(this_->ymax-this_->ymin)/2.0 , this_->ymax);
	  }
          quadtree_add(this_->ba,&this_->items[i]);
        }
        else if(
           this_->xmin+(this_->xmax-this_->xmin)/2.0<=this_->items[i].longitude && this_->items[i].longitude<this_->xmax &&
           this_->ymin+(this_->ymax-this_->ymin)/2.0<=this_->items[i].latitude && this_->items[i].latitude<this_->ymax
           ) {
	  if(!this_->bb) {
            this_->bb = quadtree_node_new( this_, this_->xmin+(this_->xmax-this_->xmin)/2.0, this_->xmax , this_->ymin+(this_->ymax-this_->ymin)/2.0 , this_->ymax);
	  }
          quadtree_add(this_->bb,&this_->items[i]);
        }
    }
    this_->node_num = 0;
}

void
quadtree_destroy(struct quadtree_node* this_) {
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




