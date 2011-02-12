#include "quadtree.h"

#include <stdlib.h>
#include <glib.h>

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
 * tries to find closest item, first it descend into the quadtree as much as possible, then if no point is found
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
    }
    if(this_->ab) {
        quadtree_destroy(this_->ab);
    }
    if(this_->ba) {
        quadtree_destroy(this_->ba);
    }
    if(this_->bb) {
        quadtree_destroy(this_->bb);
    }
    free(this_);
}




