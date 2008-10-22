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

/** @file
 * @brief Contains code related to finding a route from a position to a destination
 *
 * Routing uses segments, points and items. Items are items from the map: Streets, highways, etc.
 * Segments represent such items, or parts of it. Generally, a segment is a driveable path. An item
 * can be represented by more than one segment - in that case it is "segmented". Each segment has an
 * "offset" associated, that indicates at which position in a segmented item this segment is - a 
 * segment representing a not-segmented item always has the offset 1.
 * A point is located at the end of segments, often connecting several segments.
 * 
 * The code in this file will make navit find a route between a position and a destination.
 * It accomplishes this by first building a "route graph". This graph contains segments and
 * points.
 *
 * After building this graph in route_graph_build(), the function route_graph_flood() assigns every 
 * point and segment a "value" which represents the "costs" of traveling from this point to the
 * destination. This is done by Dijkstra's algorithm.
 *
 * When the graph is built a "route path" is created, which is a path in this graph from a given
 * position to the destination determined at time of building the graph.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if 0
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#endif
#include <glib.h>
#include "config.h"
#include "debug.h"
#include "profile.h"
#include "coord.h"
#include "projection.h"
#include "map.h"
#include "mapset.h"
#include "item.h"
#include "route.h"
#include "track.h"
#include "point.h"
#include "graphics.h"
#include "transform.h"
#include "plugin.h"
#include "fib.h"


struct map_priv {
	struct route *route;
};

int debug_route=0;

/**
 * @brief A point in the route graph
 *
 * This represents a point in the route graph. A point usually connects two or more segments,
 * but there are also points which don't do that (e.g. at the end of a dead-end).
 */
struct route_graph_point {
	struct route_graph_point *next;		 /**< Linked-list pointer to a list of all route_graph_points */
	struct route_graph_point *hash_next; /**< Pointer to a chained hashlist of all route_graph_points with this hash */
	struct route_graph_segment *start;	 /**< Pointer to a list of segments of which this point is the start. The links 
										  *  of this linked-list are in route_graph_segment->start_next.*/
	struct route_graph_segment *end;	 /**< Pointer to a list of segments of which this pointer is the end. The links
										  *  of this linked-list are in route_graph_segment->end_next. */
	struct route_graph_segment *seg;	 /**< Pointer to the segment one should use to reach the destination at
										  *  least costs */
	struct fibheap_el *el;				 /**< When this point is put on a Fibonacci heap, this is a pointer
										  *  to this point's heap-element */
	int value;							 /**< The cost at which one can reach the destination from this point on */
	struct coord c;						 /**< Coordinates of this point */
};

/**
 * @brief A segment in the route graph
 *
 * This is a segment in the route graph. A segment represents a driveable way.
 */
struct route_graph_segment {
	struct route_graph_segment *next;			/**< Linked-list pointer to a list of all route_graph_segments */
	struct route_graph_segment *start_next;		/**< Pointer to the next element in the list of segments that start at the 
												 *  same point. Start of this list is in route_graph_point->start. */
	struct route_graph_segment *end_next;		/**< Pointer to the next element in the list of segments that end at the
												 *  same point. Start of this list is in route_graph_point->end. */
	struct route_graph_point *start;			/**< Pointer to the point this segment starts at. */
	struct route_graph_point *end;				/**< Pointer to the point this segment ends at. */
	struct item item;							/**< The item (e.g. street) that this segment represents. */
	int flags;
	int len;									/**< Length of this segment */
	int offset;									/**< If the item represented by this segment is "segmented" (i.e. 
												 *  is represented by several segments instead of just one), this 
												 *  indicates the position of this segment in the item - for items
												 *  that are not segmented this should always be 1 */
};

/**
 * @brief A segment in the route path
 *
 * This is a segment in the route path.
 */
struct route_path_segment {
	struct route_path_segment *next;	/**< Pointer to the next segment in the path */
	struct item item;					/**< The item (e.g. street) this segment represents */
	int length;							/**< Length of the segment */
	int offset;							/**< Same as in route_graph_segment->offset */
	int direction;						/**< Order in which the coordinates are ordered. >0 means "First
										 *  coordinate of the segment is the first coordinate of the item", <=0 
										 *  means reverse. */
	unsigned ncoords;					/**< How many coordinates does this segment have? */
	struct attr **attrs;				/**< Attributes of this route path segment */
	struct coord c[0];					/**< Pointer to the ncoords coordinates of this segment */
	/* WARNING: There will be coordinates following here, so do not create new fields after c! */
};

/**
 * @brief Usually represents a destination or position
 *
 * This struct usually represents a destination or position
 */
struct route_info {
	struct coord c;			 /**< The actual destination / position */
	struct coord lp;		 /**< The nearest point on a street to c */
	int pos; 				 /**< The position of lp within the coords of the street */
	int lenpos; 			 /**< Distance between lp and the end of the street */
	int lenneg; 			 /**< Distance between lp and the start of the street */
	int lenextra;			 /**< Distance between lp and c */

	struct street_data *street; /**< The street lp is on */
};

/**
 * @brief A complete route path
 *
 * This structure describes a whole routing path
 */
struct route_path {
	struct route_path_segment *path;			/**< The first segment in the path, i.e. the segment one should 
												 *  drive in next */
	struct route_path_segment *path_last;		/**< The last segment in the path */
	/* XXX: path_hash is not necessery now */
	struct item_hash *path_hash;				/**< A hashtable of all the items represented by this route's segements */
};

#define RF_FASTEST	(1<<0)
#define RF_SHORTEST	(1<<1)
#define RF_AVOIDHW	(1<<2)
#define RF_AVOIDPAID	(1<<3)
#define RF_LOCKONROAD	(1<<4)
#define RF_SHOWGRAPH	(1<<5)

/**
 * @brief A complete route
 * 
 * This struct holds all information about a route.
 */
struct route {
	int version;				/**< Counts how many times this route got updated */
	struct mapset *ms;			/**< The mapset this route is built upon */
	unsigned flags;
	struct route_info *pos;		/**< Current position within this route */
	struct route_info *dst;		/**< Destination of the route */

	struct route_graph *graph;	/**< Pointer to the route graph */
	struct route_path *path2;	/**< Pointer to the route path */
	struct map *map;			
	struct map *graph_map;
	int destination_distance;	/**< Distance to the destination at which the destination is considered "reached" */
	int speedlist[route_item_last-route_item_first+1];	/**< The speedlist for this route */
};

/**
 * @brief A complete route graph
 *
 * This structure describes a whole routing graph
 */
struct route_graph {
	struct route_graph_point *route_points;		/**< Pointer to the first route_graph_point in the linked list of  all points */
   	struct route_graph_segment *route_segments; /**< Pointer to the first route_graph_segment in the linked list of all segments */
#define HASH_SIZE 8192
	struct route_graph_point *hash[HASH_SIZE];	/**< A hashtable containing all route_graph_points in this graph */
};

#define HASHCOORD(c) ((((c)->x +(c)->y) * 2654435761UL) & (HASH_SIZE-1))

static struct route_info * route_find_nearest_street(struct mapset *ms, struct pcoord *c);
static struct route_graph_point *route_graph_get_point(struct route_graph *this, struct coord *c);
static void route_graph_update(struct route *this);
static struct route_path *route_path_new(struct route_graph *this, struct route_path *oldpath, struct route_info *pos, struct route_info *dst, int *speedlist);
static void route_process_street_graph(struct route_graph *this, struct item *item);
static void route_graph_destroy(struct route_graph *this);
static void route_path_update(struct route *this);

/**
 * @brief Returns the projection used for this route
 *
 * @param route The route to return the projection for
 * @return The projection used for this route
 */
static enum projection route_projection(struct route *route)
{
	struct street_data *street;
	street = route->pos ? route->pos->street : route->dst->street;
	return map_projection(street->item.map);
}

/**
 * @brief Destroys a route_path
 *
 * @param this The route_path to be destroyed
 */
static void
route_path_destroy(struct route_path *this)
{
	struct route_path_segment *c,*n;
	if (! this)
		return;
	if (this->path_hash) {
		item_hash_destroy(this->path_hash);
		this->path_hash=NULL;
	}
	c=this->path;
	while (c) {
		n=c->next;
		if (c->attrs) { 
			attr_list_free(c->attrs);
		}
		g_free(c);
		c=n;
	}
	g_free(this);
}

/**
 * @brief Creates a completely new route structure
 *
 * @param attrs Not used
 * @return The newly created route
 */
struct route *
route_new(struct attr **attrs)
{
	struct route *this=g_new0(struct route, 1);
	struct attr dest_attr;

	if (!this) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return NULL;
	}

	if (attr_generic_get_attr(attrs, NULL, attr_destination_distance, &dest_attr, NULL)) {
		this->destination_distance = dest_attr.u.num;
	} else {
		this->destination_distance = 50; // Default value
	}

	return this;
}

/**
 * @brief Sets the mapset of the route passwd
 *
 * @param this The route to set the mapset for
 * @param ms The mapset to set for this route
 */
void
route_set_mapset(struct route *this, struct mapset *ms)
{
	this->ms=ms;
}

/**
 * @brief Returns the mapset of the route passed
 *
 * @param this The route to get the mapset of
 * @return The mapset of the route passed
 */
struct mapset *
route_get_mapset(struct route *this)
{
	return this->ms;
}

/**
 * @brief Returns the current position within the route passed
 *
 * @param this The route to get the position for
 * @return The position within the route passed
 */
struct route_info *
route_get_pos(struct route *this)
{
	return this->pos;
}

/**
 * @brief Returns the destination of the route passed
 *
 * @param this The route to get the destination for
 * @return The destination of the route passed
 */
struct route_info *
route_get_dst(struct route *this)
{
	return this->dst;
}

/**
 * @brief Returns the speedlist of the route passed
 *
 * @param this The route to get the speedlist for
 * @return The speedlist of the route passed
 */
int *
route_get_speedlist(struct route *this)
{
	return this->speedlist;
}

/**
 * @brief Checks if the path is calculated for the route passed
 *
 * @param this The route to check
 * @return True if the path is calculated, false if not
 */
int
route_get_path_set(struct route *this)
{
	return this->path2 != NULL;
}

/**
 * @brief Sets the driving speed for a certain itemtype
 *
 * @param this The route to set the speed for
 * @param type The itemtype to set the speed for
 * @param value The speed that should be set
 * @return True on success, false if the itemtype does not exist
 */
int
route_set_speed(struct route *this, enum item_type type, int value)
{
	if (type < route_item_first || type > route_item_last) {
		dbg(0,"street type %d out of range [%d,%d]", type, route_item_first, route_item_last);
		return 0;
	}
	this->speedlist[type-route_item_first]=value;
	return 1;
}

/**
 * @brief Checks if the route passed contains a certain item within the route path
 *
 * This function checks if a certain items exists in the path that navit will guide
 * the user to his destination. It does *not* check if this item exists in the route 
 * graph!
 *
 * @param this The route to check for this item
 * @param item The item to search for
 * @return True if the item was found, false if the item was not found or the route was not calculated
 */
int
route_contains(struct route *this, struct item *item)
{
	if (! this->path2 || !this->path2->path_hash)
		return 0;
	return  (int)item_hash_lookup(this->path2->path_hash, item);
}

/**
 * @brief Checks if the current position in a route is a certain item
 *
 * @param this The route to check for this item
 * @param item The item to search for
 * @return True if the current position is this item, false otherwise
 */
int
route_pos_contains(struct route *this, struct item *item)
{
	if (! this->pos)
		return 0;
	return item_is_equal(this->pos->street->item, *item);
}

/**
 * @brief Checks if a route has reached its destination
 *
 * @param this The route to be checked
 * @return True if the destination is "reached", false otherwise.
 */
int
route_destination_reached(struct route *this)
{
	struct street_data *sd = NULL;

	if (!this->pos)
		return 0;

	sd = this->pos->street;

	if (!this->path2) {
		return 0;
	}

	if (!item_is_equal(this->pos->street->item, this->dst->street->item)) { 
		return 0;
	}

	if ((sd->flags & AF_ONEWAY) && (this->pos->lenneg >= this->dst->lenneg)) { // We would have to drive against the one-way road
		return 0;
	}
	if ((sd->flags & AF_ONEWAYREV) && (this->pos->lenpos >= this->dst->lenpos)) {
		return 0;
	}
	 
	if (transform_distance(route_projection(this), &this->pos->c, &this->dst->lp) > this->destination_distance) {
		return 0;
	}
	
	return 1;
}

/**
 * @brief Updates the route graph and the route path if something changed with the route
 *
 * This will update the route graph and the route path of the route if some of the
 * route's settings (destination, position) have changed. 
 * 
 * @attention For this to work the route graph has to be destroyed if the route's 
 * @attention destination is changed somewhere!
 *
 * @param this The route to update
 */
static void
route_path_update(struct route *this)
{
	struct route_path *oldpath = NULL;
	if (! this->pos || ! this->dst) {
		route_path_destroy(this->path2);
		this->path2 = NULL;
		return;
	}
	/* the graph is destroyed when setting the destination */
	if (this->graph && this->pos && this->dst && this->path2) {
		// we can try to update
		oldpath = this->path2;
		this->path2 = NULL;
	}
	if (! this->graph || !(this->path2=route_path_new(this->graph, oldpath, this->pos, this->dst, this->speedlist))) {
		profile(0,NULL);
		route_graph_update(this);
		this->path2=route_path_new(this->graph, oldpath, this->pos, this->dst, this->speedlist);
		profile(1,"route_path_new");
		profile(0,"end");
	}

	if (oldpath) {
		/* Destroy what's left */
		route_path_destroy(oldpath);
	}
}

/** 
 * @brief This will calculate all the distances stored in a route_info
 *
 * @param ri The route_info to calculate the distances for
 * @param pro The projection used for this route
 */
static void
route_info_distances(struct route_info *ri, enum projection pro)
{
	int npos=ri->pos+1;
	struct street_data *sd=ri->street;
	/* 0 1 2 X 3 4 5 6 pos=2 npos=3 count=7 0,1,2 3,4,5,6*/
	ri->lenextra=transform_distance(pro, &ri->lp, &ri->c);
	ri->lenneg=transform_polyline_length(pro, sd->c, npos)+transform_distance(pro, &sd->c[ri->pos], &ri->lp);
	ri->lenpos=transform_polyline_length(pro, sd->c+npos, sd->count-npos)+transform_distance(pro, &sd->c[npos], &ri->lp);
}

/**
 * @brief This sets the current position of the route passed
 *
 * This will set the current position of the route passed to the street that is nearest to the
 * passed coordinates. It also automatically updates the route.
 *
 * @param this The route to set the position of
 * @param pos Coordinates to set as position
 */
void
route_set_position(struct route *this, struct pcoord *pos)
{
	if (this->pos)
		route_info_free(this->pos);
	this->pos=NULL;
	this->pos=route_find_nearest_street(this->ms, pos);
	dbg(1,"this->pos=%p\n", this->pos);
	if (! this->pos)
		return;
	route_info_distances(this->pos, pos->pro);
	if (this->dst) 
		route_path_update(this);
}

/**
 * @brief Sets a route's current position based on coordinates from tracking
 *
 * @param this The route to set the current position of
 * @param tracking The tracking to get the coordinates from
 */
void
route_set_position_from_tracking(struct route *this, struct tracking *tracking)
{
	struct coord *c;
	struct route_info *ret;

	dbg(2,"enter\n");
	c=tracking_get_pos(tracking);
	ret=g_new0(struct route_info, 1);
	if (!ret) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return;
	}
	if (this->pos)
		route_info_free(this->pos);
	this->pos=NULL;
	ret->c=*c;
	ret->lp=*c;
	ret->pos=tracking_get_segment_pos(tracking);
	ret->street=street_data_dup(tracking_get_street_data(tracking));
	route_info_distances(ret, projection_mg);
	dbg(3,"c->x=0x%x, c->y=0x%x pos=%d item=(0x%x,0x%x)\n", c->x, c->y, ret->pos, ret->street->item.id_hi, ret->street->item.id_lo);
	dbg(3,"street 0=(0x%x,0x%x) %d=(0x%x,0x%x)\n", ret->street->c[0].x, ret->street->c[0].y, ret->street->count-1, ret->street->c[ret->street->count-1].x, ret->street->c[ret->street->count-1].y);
	this->pos=ret;
	if (this->dst) 
		route_path_update(this);
	dbg(2,"ret\n");
}

/* Used for debuging of route_rect, what routing sees */
struct map_selection *route_selection;

/**
 * @brief Returns a single map selection
 */
struct map_selection *
route_rect(int order, struct coord *c1, struct coord *c2, int rel, int abs)
{
	int dx,dy,sx=1,sy=1,d,m;
	struct map_selection *sel=g_new(struct map_selection, 1);
	if (!sel) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return sel;
	}
	sel->order[layer_town]=0;
	sel->order[layer_poly]=0;
	sel->order[layer_street]=order;
	dbg(1,"%p %p\n", c1, c2);
	dx=c1->x-c2->x;
	dy=c1->y-c2->y;
	if (dx < 0) {
		sx=-1;
		sel->u.c_rect.lu.x=c1->x;
		sel->u.c_rect.rl.x=c2->x;
	} else {
		sel->u.c_rect.lu.x=c2->x;
		sel->u.c_rect.rl.x=c1->x;
	}
	if (dy < 0) {
		sy=-1;
		sel->u.c_rect.lu.y=c2->y;
		sel->u.c_rect.rl.y=c1->y;
	} else {
		sel->u.c_rect.lu.y=c1->y;
		sel->u.c_rect.rl.y=c2->y;
	}
	if (dx*sx > dy*sy) 
		d=dx*sx;
	else
		d=dy*sy;
	m=d*rel/100+abs;	
	sel->u.c_rect.lu.x-=m;
	sel->u.c_rect.rl.x+=m;
	sel->u.c_rect.lu.y+=m;
	sel->u.c_rect.rl.y-=m;
	sel->next=NULL;
	return sel;
}

/**
 * @brief Returns a list of map selections useable to create a route graph
 *
 * Returns a list of  map selections useable to get a  map rect from which items can be
 * retrieved to build a route graph. The selections are a rectangle with
 * c1 and c2 as two corners.
 *
 * @param c1 Corner 1 of the rectangle
 * @param c2 Corder 2 of the rectangle
 */
static struct map_selection *
route_calc_selection(struct coord *c1, struct coord *c2)
{
	struct map_selection *ret,*sel;
	sel=route_rect(4, c1, c2, 25, 0);
	ret=sel;
	sel->next=route_rect(8, c1, c1, 0, 40000);
	sel=sel->next;
	sel->next=route_rect(18, c1, c1, 0, 10000);
	sel=sel->next;
	sel->next=route_rect(8, c2, c2, 0, 40000);
	sel=sel->next;
	sel->next=route_rect(18, c2, c2, 0, 10000);
	/* route_selection=ret; */
	return ret;
}

/**
 * @brief Destroys a list of map selections
 *
 * @param sel Start of the list to be destroyed
 */
static void
route_free_selection(struct map_selection *sel)
{
	struct map_selection *next;
	while (sel) {
		next=sel->next;
		g_free(sel);
		sel=next;
	}
}


/**
 * @brief Sets the destination of a route
 *
 * This sets the destination of a route to the street nearest to the coordinates passed
 * and updates the route.
 *
 * @param this The route to set the destination for
 * @param dst Coordinates to set as destination
 */
void
route_set_destination(struct route *this, struct pcoord *dst)
{
	profile(0,NULL);
	if (this->dst)
		route_info_free(this->dst);
	this->dst=NULL;
	if (dst) {
		this->dst=route_find_nearest_street(this->ms, dst);
		if(this->dst)
		route_info_distances(this->dst, dst->pro);
	}
	profile(1,"find_nearest_street");

	/* The graph has to be destroyed and set to NULL, otherwise route_path_update() doesn't work */
	route_graph_destroy(this->graph);
	this->graph=NULL;
	route_path_update(this);
	profile(0,"end");
}

/**
 * @brief Gets the route_graph_point with the specified coordinates
 *
 * @param this The route in which to search
 * @param c Coordinates to search for
 * @return The point at the specified coordinates or NULL if not found
 */
static struct route_graph_point *
route_graph_get_point(struct route_graph *this, struct coord *c)
{
	struct route_graph_point *p;
	int hashval=HASHCOORD(c);
	p=this->hash[hashval];
	while (p) {
		if (p->c.x == c->x && p->c.y == c->y) 
			return p;
		p=p->hash_next;
	}
	return NULL;
}

/**
 * @brief Inserts a point into the route graph at the specified coordinates
 *
 * This will insert a point into the route graph at the coordinates passed in f.
 * Note that the point is not yet linked to any segments.
 *
 * @param this The route to insert the point into
 * @param f The coordinates at which the point should be inserted
 * @return The point inserted or NULL on failure
 */
static struct route_graph_point *
route_graph_add_point(struct route_graph *this, struct coord *f)
{
	int hashval;
	struct route_graph_point *p;

	p=route_graph_get_point(this,f);
	if (!p) {
		hashval=HASHCOORD(f);
		if (debug_route)
			printf("p (0x%x,0x%x)\n", f->x, f->y);
		p=g_new(struct route_graph_point,1);
		if (!p) {
			printf("%s:Out of memory\n", __FUNCTION__);
			return p;
		}
		p->hash_next=this->hash[hashval];
		this->hash[hashval]=p;
		p->next=this->route_points;
		p->el=NULL;
		p->start=NULL;
		p->end=NULL;
		p->seg=NULL;
		p->value=INT_MAX;
		p->c=*f;
		this->route_points=p;
	}
	return p;
}

/**
 * @brief Frees all the memory used for points in the route graph passed
 *
 * @param this The route graph to delete all points from
 */
static void
route_graph_free_points(struct route_graph *this)
{
	struct route_graph_point *curr,*next;
	curr=this->route_points;
	while (curr) {
		next=curr->next;
		g_free(curr);
		curr=next;
	}
	this->route_points=NULL;
	memset(this->hash, 0, sizeof(this->hash));
}

/**
 * @brief Inserts a new segment into the route graph
 *
 * This function performs a check if a segment for the item specified already exists, and inserts
 * a new segment representing this item if it does not.
 *
 * @param this The route graph to insert the segment into
 * @param start The graph point which should be connected to the start of this segment
 * @param end The graph point which should be connected to the end of this segment
 * @param len The length of this segment
 * @param item The item that is represented by this segment
 * @param flags Flags for this segment
 * @param offset If the item passed in "item" is segmented (i.e. divided into several segments), this indicates the position of this segment within the item
 */
static void
route_graph_add_segment(struct route_graph *this, struct route_graph_point *start,
			struct route_graph_point *end, int len, struct item *item,
			int flags, int offset)
{
	struct route_graph_segment *s;
/*	
	FIXME: commented out becouse
	it is possible to have one item with two different
	offsets as segments 
	s=start->start;
	while (s) {
		if (item_is_equal(*item, s->item)) 
			return;
		s=s->start_next;
	} 
*/
	s = g_new0(struct route_graph_segment, 1);
	if (!s) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return;
	}
	s->start=start;
	s->start_next=start->start;
	start->start=s;
	s->end=end;
	s->end_next=end->end;
	end->end=s;
	dbg_assert(len >= 0);
	s->len=len;
	s->item=*item;
	s->flags=flags;
	s->offset = offset;
	s->next=this->route_segments;
	this->route_segments=s;
	if (debug_route)
		printf("l (0x%x,0x%x)-(0x%x,0x%x)\n", start->c.x, start->c.y, end->c.x, end->c.y);
}

/**
 * @brief Gets all the coordinates of an item
 *
 * This will get all the coordinates of the item i and return them in c,
 * up to max coordinates. Additionally it is possible to limit the coordinates
 * returned to all the coordinates of the item between the two coordinates
 * start end end.
 *
 * @important Make shure that whatever c points to has enough memory allocated
 * @important to hold max coordinates!
 *
 * @param i The item to get the coordinates of
 * @param c Pointer to memory allocated for holding the coordinates
 * @param max Maximum number of coordinates to return
 * @param start First coordinate to get
 * @param end Last coordinate to get
 */
static int get_item_seg_coords(struct item *i, struct coord *c, int max,
		struct coord *start, struct coord *end)
{
	struct map_rect *mr;
	struct item *item;
	int rc = 0, p = 0;
	struct coord c1;
	mr=map_rect_new(i->map, NULL);
	if (!mr)
		return 0;
	item = map_rect_get_item_byid(mr, i->id_hi, i->id_lo);
	if (item) {
		rc = item_coord_get(item, &c1, 1);
		while (rc && (c1.x != start->x || c1.y != start->y)) {
			rc = item_coord_get(item, &c1, 1);
		}
		while (rc && p < max) {
			c[p++] = c1;
			if (c1.x == end->x && c1.y == end->y)
				break;
			rc = item_coord_get(item, &c1, 1);
		}
	}
	map_rect_destroy(mr);
	return p;
}

/**
 * @brief Returns and removes one segment from a path
 *
 * @param path The path to take the segment from
 * @param item The item whose segment to remove
 * @param offset Offset of the segment within the item to remove. If the item is not segmented this should be 1.
 * @return The segment removed
 */
static struct route_path_segment *
route_extract_segment_from_path(struct route_path *path, struct item *item,
						 int offset)
{
	struct route_path_segment *sp = NULL, *s;
	s = path->path;
	while (s) {
		if (s->offset == offset && item_is_equal(s->item,*item)) {
			if (sp) {
				sp->next = s->next;
				break;
			} else {
				path->path = s->next;
				break;
			}
		}
		sp = s;
		s = s->next;
	}
	if (s)
		item_hash_remove(path->path_hash, item);
	return s;
}

/**
 * @brief Adds a segment and the end of a path
 *
 * @param this The path to add the segment to
 * @param segment The segment to add
 */
static void
route_path_add_segment(struct route_path *this, struct route_path_segment *segment)
{
	if (!this->path)
		this->path=segment;
	if (this->path_last)
		this->path_last->next=segment;
	this->path_last=segment;
}

/**
 * @brief Adds a new item to a path
 *
 * This adds a new item to a path, creating a new segment for it. Please note that this function does not check
 * if the item passed is segmented - it will create exactly one segment.
 *
 * @param this The path to add the item to
 * @param item The item to add
 * @param len The length of the item
 * @param first (Optional) coordinate to add to the start of the item. If none should be added, make this NULL.
 * @param c Pointer to count coordinates of the item.
 * @param cound Number of coordinates in c
 * @param last (Optional) coordinate to add to the end of the item. If none should be added, make this NULL.
 * @param dir Direction to add the coordinates in. Greater than zero means "start with the first coordinate in c", all other values mean "start with the last coordinate in c"
 */
static void
route_path_add_item(struct route_path *this, struct item *item, int len, struct coord *first, struct coord *c, int count, struct coord *last, int dir)
{
	int i,idx=0,ccount=count + (first ? 1:0) + (last ? 1:0);
	struct route_path_segment *segment;
	
	segment=g_malloc0(sizeof(*segment) + sizeof(struct coord) * ccount);
	segment->ncoords=ccount;
	segment->direction=dir;
	if (first)
		segment->c[idx++]=*first;
	if (dir > 0) {
		for (i = 0 ; i < count ; i++)
			segment->c[idx++]=c[i];
	} else {
		for (i = 0 ; i < count ; i++)
			segment->c[idx++]=c[count-i-1];
	}
	if (last)
		segment->c[idx++]=*last;
	segment->length=len;
	if (item)
		segment->item=*item;
	route_path_add_segment(this, segment);
}

/**
 * @brief Inserts a new item into the path
 * 
 * This function does almost the same as "route_apth_add_item()", but identifies
 * the item to add by a segment from the route graph. Another difference is that it "copies" the
 * segment from the route graph, i.e. if the item is segmented, only the segment passed in rgs will
 * be added to the route path, not all segments of the item. 
 *
 * The function can be sped up by passing an old path already containing this segment in oldpath - 
 * the segment will then be extracted from this old path. Please note that in this case the direction
 * parameter has no effect.
 *
 * @param this The path to add the item to
 * @param oldpath Old path containing the segment to be added. Speeds up the function, but can be NULL.
 * @param rgs Segment of the route graph that should be "copied" to the route path
 * @param len Length of the item to be added
 * @param offset Offset of rgs within the item it represents
 * @param dir Order in which to add the coordinates. See route_path_add_item()
 * @param straight Indicates if this segment is being entered "straight". See route_check_straight().
 */
static void
route_path_add_item_from_graph(struct route_path *this, struct route_path *oldpath,
				   struct route_graph_segment *rgs, int len, int offset, int dir, int straight)
{
	struct route_path_segment *segment;
	int i,ccnt = 0;
	struct coord ca[2048];
	struct attr straight_attr;

	if (oldpath) {
		ccnt = (int)item_hash_lookup(oldpath->path_hash, &rgs->item);
		if (ccnt) {
			segment = route_extract_segment_from_path(oldpath,
							 &rgs->item, offset);
			
			if (segment)
				goto linkold;
		}
	}

	ccnt = get_item_seg_coords(&rgs->item, ca, 2047, &rgs->start->c, &rgs->end->c);
	segment= g_malloc0(sizeof(*segment) + sizeof(struct coord) * ccnt);
	if (!segment) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return;
	}
	segment->direction=dir;
	if (dir > 0) {
		for (i = 0 ; i < ccnt ; i++)
			segment->c[i]=ca[i];
	} else {
		for (i = 0 ; i < ccnt ; i++)
			segment->c[i]=ca[ccnt-i-1];
	}
	segment->ncoords = ccnt;
	segment->item=rgs->item;
	segment->offset = offset;
linkold:
	segment->length=len;
	segment->next=NULL;
	item_hash_insert(this->path_hash,  &rgs->item, (void *)ccnt);

	straight_attr.type = attr_route_follow_straight;
	straight_attr.u.num = straight;

	segment->attrs = attr_generic_set_attr(segment->attrs, &straight_attr);

	route_path_add_segment(this, segment);
}

/**
 * @brief Destroys all segments of a route graph
 *
 * @param this The graph to destroy all segments from
 */
static void
route_graph_free_segments(struct route_graph *this)
{
	struct route_graph_segment *curr,*next;
	curr=this->route_segments;
	while (curr) {
		next=curr->next;
		g_free(curr);
		curr=next;
	}
	this->route_segments=NULL;
}

/**
 * @brief Destroys a route graph
 * 
 * @param this The route graph to be destroyed
 */
static void
route_graph_destroy(struct route_graph *this)
{
	if (this) {
		route_graph_free_points(this);
		route_graph_free_segments(this);
		g_free(this);
	}
}

/**
 * @brief Returns the time needed to drive len on item
 *
 * @param speedlist The speedlist that should be used
 * @param item The item to be driven on
 * @param len The length to drive
 * @return The time needed to drive len on item
 */
int
route_time(int *speedlist, struct item *item, int len)
{
	if (item->type < route_item_first || item->type > route_item_last) {
		dbg(0,"street type %d out of range [%d,%d]\n", item->type, route_item_first, route_item_last);
		return len*36;
	}
	if (!speedlist[item->type-route_item_first]) {
		dbg(0,"street type %d speed is zero\n", item->type);
		return len*36;
	}
	return len*36/speedlist[item->type-route_item_first];
}

/**
 * @brief Returns the "costs" of driving len on item
 *
 * @param speedlist The speedlist that should be used
 * @param item The item to be driven on
 * @param len The length to drive
 * @return The "costs" needed to drive len on item
 */  
static int
route_value(int *speedlist, struct item *item, int len)
{
	int ret;
	if (len < 0) {
		printf("len=%d\n", len);
	}
	dbg_assert(len >= 0);
	ret=route_time(speedlist, item, len);
	dbg(1, "route_value(0x%x, %d)=%d\n", item->type, len, ret);
	return ret;
}

/**
 * @brief Adds an item to the route graph
 *
 * This adds an item (e.g. a street) to the route graph, creating as many segments as needed for a
 * segmented item.
 *
 * @param this The route graph to add to
 * @param item The item to add
 */
static void
route_process_street_graph(struct route_graph *this, struct item *item)
{
#ifdef AVOID_FLOAT
	int len=0;
#else
	double len=0;
#endif
	struct route_graph_point *s_pnt,*e_pnt;
	struct coord c,l;
	struct attr attr;
	int flags = 0;
	int segmented = 0;
	int offset = 1;

	if (item_coord_get(item, &l, 1)) {
		if (item_attr_get(item, attr_flags, &attr)) {
			flags = attr.u.num;
			if (flags & AF_SEGMENTED)
				segmented = 1;
		}
		s_pnt=route_graph_add_point(this,&l);
		if (!segmented) {
			while (item_coord_get(item, &c, 1)) {
				len+=transform_distance(map_projection(item->map), &l, &c);
				l=c;
			}
			e_pnt=route_graph_add_point(this,&l);
			dbg_assert(len >= 0);
			route_graph_add_segment(this, s_pnt, e_pnt, len, item, flags, offset);
		} else {
			int isseg,rc;
			int sc = 0;
			do {
				isseg = item_coord_is_node(item);
				rc = item_coord_get(item, &c, 1);
				if (rc) {
					len+=transform_distance(map_projection(item->map), &l, &c);
					l=c;
					if (isseg) {
						e_pnt=route_graph_add_point(this,&l);
						route_graph_add_segment(this, s_pnt, e_pnt, len, item, flags, offset);
						offset++;
						s_pnt=route_graph_add_point(this,&l);
						len = 0;
					}
				}
			} while(rc);
			e_pnt=route_graph_add_point(this,&l);
			dbg_assert(len >= 0);
			sc++;
			route_graph_add_segment(this, s_pnt, e_pnt, len, item, flags, offset);
		}
	}
}

/**
 * @brief Compares the costs of reaching the destination from two points on
 * 
 * @important Do not pass anything other than route_graph_points in v1 and v2!
 *
 * @param v1 Point1 
 * @param v2 Point2
 * @return The additional costs of v1 compared to v2 (may be negative)
 */
static int
compare(void *v1, void *v2)
{
	struct route_graph_point *p1=v1;
	struct route_graph_point *p2=v2;
#if 0
	if (debug_route)
		printf("compare %d (%p) vs %d (%p)\n", p1->value,p1,p2->value,p2);
#endif
	return p1->value-p2->value;
}

/**
 * @brief Calculates the routing costs for each point
 *
 * This function is the heart of routing. It assigns each point in the route graph a
 * cost at which one can reach the destination from this point on. Additionally it assigns
 * each point a segment one should follow from this point on to reach the destination at the
 * stated costs.
 * 
 * This function uses Dijkstra's algorithm to do the routing. To understand it you should have a look
 * at this algorithm.
 */
static void
route_graph_flood(struct route_graph *this, struct route_info *dst, int *speedlist)
{
	struct route_graph_point *p_min,*end=NULL;
	struct route_graph_segment *s;
	int min,new,old,val;
	struct fibheap *heap; /* This heap will hold all points with "temporarily" calculated costs */
	struct street_data *sd=dst->street;

	heap = fh_makeheap();   
	fh_setcmp(heap, compare);

	if (! (sd->flags & AF_ONEWAYREV)) { /* If we may drive in the direction of the coordinates of the item, the first coordinate is one starting point */
		end=route_graph_get_point(this, &sd->c[0]);
		dbg_assert(end != 0);
		end->value=route_value(speedlist, &sd->item, dst->lenneg);
		end->el=fh_insert(heap, end);
	}

	if (! (sd->flags & AF_ONEWAY)) { /* If we may drive against the direction of the coordinates, the last coordinate is another starting point */
		end=route_graph_get_point(this, &sd->c[sd->count-1]);
		dbg_assert(end != 0);
		end->value=route_value(speedlist, &sd->item, dst->lenpos);
		end->el=fh_insert(heap, end);
	}

	dbg(1,"0x%x,0x%x\n", end->c.x, end->c.y);
	for (;;) {
		p_min=fh_extractmin(heap); /* Starting Dijkstra by selecting the point with the minimum costs on the heap */
		if (! p_min) /* There are no more points with temporarily calculated costs, Dijkstra has finished */
			break;
		min=p_min->value;
		if (debug_route)
			printf("extract p=%p free el=%p min=%d, 0x%x, 0x%x\n", p_min, p_min->el, min, p_min->c.x, p_min->c.y);
		p_min->el=NULL; /* This point is permanently calculated now, we've taken it out of the heap */
		s=p_min->start;
		while (s) { /* Iterating all the segments leading away from our point to update the points at their ends */
			val=route_value(speedlist, &s->item, s->len);
#if 0
			val+=val*2*street_route_contained(s->str->segid);
#endif
			new=min+val;
			if (debug_route)
				printf("begin %d len %d vs %d (0x%x,0x%x)\n",new,val,s->end->value, s->end->c.x, s->end->c.y);
			if (new < s->end->value && !(s->flags & AF_ONEWAY)) { /* We've found a less costly way to reach the end of s, update it */
				s->end->value=new;
				s->end->seg=s;
				if (! s->end->el) {
					if (debug_route)
						printf("insert_end p=%p el=%p val=%d ", s->end, s->end->el, s->end->value);
					s->end->el=fh_insert(heap, s->end);
					if (debug_route)
						printf("el new=%p\n", s->end->el);
				}
				else {
					if (debug_route)
						printf("replace_end p=%p el=%p val=%d\n", s->end, s->end->el, s->end->value);
					fh_replacedata(heap, s->end->el, s->end);
				}
			}
			if (debug_route)
				printf("\n");
			s=s->start_next;
		}
		s=p_min->end;
		while (s) { /* Doing the same as above with the segments leading towards our point */
			val=route_value(speedlist, &s->item, s->len);
			new=min+val;
			if (debug_route)
				printf("end %d len %d vs %d (0x%x,0x%x)\n",new,val,s->start->value,s->start->c.x, s->start->c.y);
			if (new < s->start->value && !(s->flags & AF_ONEWAYREV)) {
				old=s->start->value;
				s->start->value=new;
				s->start->seg=s;
				if (! s->start->el) {
					if (debug_route)
						printf("insert_start p=%p el=%p val=%d ", s->start, s->start->el, s->start->value);
					s->start->el=fh_insert(heap, s->start);
					if (debug_route)
						printf("el new=%p\n", s->start->el);
				}
				else {
					if (debug_route)
						printf("replace_start p=%p el=%p val=%d\n", s->start, s->start->el, s->start->value);
					fh_replacedata(heap, s->start->el, s->start);
				}
			}
			if (debug_route)
				printf("\n");
			s=s->end_next;
		}
	}
	fh_deleteheap(heap);
}

/**
 * @brief Starts an "offroad" path
 *
 * This starts a path that is not located on a street. It creates a new route path
 * adding only one segment, that leads from pos to dest, and which is not associated with an item.
 *
 * @param this Not used
 * @param pos The starting position for the new path
 * @param dst The destination of the new path
 * @param dir Not used
 * @return The new path
 */
static struct route_path *
route_path_new_offroad(struct route_graph *this, struct route_info *pos, struct route_info *dst, int dir)
{
	struct route_path *ret;

	ret=g_new0(struct route_path, 1);
	ret->path_hash=item_hash_new();
	route_path_add_item(ret, NULL, pos->lenextra+dst->lenextra, &pos->c, NULL, 0, &dst->c, 1);

	return ret;
}

/**
 * @brief Creates a new "trivial" route
 * 
 * This function creates a new "trivial" route. A trivial route is a route that starts and ends on the same street,
 * so there is no routing needed. Depending on pos and dst it can optionally add some "offroad" part to the route.
 *
 * @param this The route graph to place the route on
 * @param pos The starting position for the new path
 * @param dst The destination of the new path
 * @param dir Direction of the coordinates to be added
 * @return The new path
 */
static struct route_path *
route_path_new_trivial(struct route_graph *this, struct route_info *pos, struct route_info *dst, int dir)
{
	struct street_data *sd=pos->street;
	struct route_path *ret;

	if (dir > 0) {
		if (pos->lenextra + dst->lenextra + pos->lenneg-dst->lenneg > transform_distance(map_projection(sd->item.map), &pos->c, &dst->c))
			return route_path_new_offroad(this, pos, dst, dir);
	} else {
		if (pos->lenextra + dst->lenextra + pos->lenpos-dst->lenpos > transform_distance(map_projection(sd->item.map), &pos->c, &dst->c))
			return route_path_new_offroad(this, pos, dst, dir);
	}
	ret=g_new0(struct route_path, 1);
	ret->path_hash=item_hash_new();
	if (pos->lenextra) 
		route_path_add_item(ret, NULL, pos->lenextra, &pos->c, NULL, 0, &pos->lp, 1);
	if (dir > 0)
		route_path_add_item(ret, &sd->item, pos->lenneg-dst->lenneg, &pos->lp, sd->c+pos->pos+1, dst->pos+pos->pos, &dst->lp, 1);
	else
		route_path_add_item(ret, &sd->item, pos->lenpos-dst->lenpos, &pos->lp, sd->c+dst->pos+1, pos->pos-dst->pos, &dst->lp, -1);
	if (dst->lenextra) 
		route_path_add_item(ret, NULL, dst->lenextra, &dst->lp, NULL, 0, &dst->c, 1);
	return ret;
}

/**
 * @brief Calculates of two coordinates' connection
 *
 * This function calculates the angle between coordinates, with north = 0
 * and east = 90.
 *
 * %FIXME This is a duplicate of road_angle() in navigation.c - combine them?
 *
 * @param c1 Coordinate 1
 * @param c2 Coordinate 2
 * @param dir Set to true if c1 is the prior, and c2 the later coordinate.
 * @return The angle of the coordinate's connection  
 */
static int
route_road_angle(struct coord *c1, struct coord *c2, int dir)
{
	int ret=transform_get_angle_delta(c1, c2, dir);
	dbg(1, "road_angle(0x%x,0x%x - 0x%x,0x%x)=%d\n", c1->x, c1->y, c2->x, c2->y, ret);
	return ret;
}

/**
 * @brief Checks if entering one segment from another is a "straight" road
 *
 * This checks if one can enter seg_to from seg_from by driving "straight" - i.e. 
 * if seg_to is the segment you can drive to from seg_from by steering less than
 * all to all other segments.
 *
 * This function returns true on failure, so we don't create maneuvers on every error.
 *
 * @param seg_from Segment we are driving from
 * @param seg_to Segment we are driving to
 * @return True if driving from seg_from to seg_to is "straight", false otherwise
 */
static int
route_check_straight(struct route_graph_segment *seg_from, struct route_graph_segment *seg_to)
{
	struct route_graph_segment *curr;
	struct route_graph_point *conn;
	int from_angle, to_angle, curr_angle, angle_diff; 
	int ccnt;
	struct coord ca[2048];

	if ((seg_from->end == seg_to->start) || (seg_from->end == seg_to->end)) {
		ccnt = get_item_seg_coords(&seg_from->item, ca, 2047, &seg_from->start->c, &seg_from->end->c);
		from_angle = route_road_angle(&ca[ccnt-2], &ca[ccnt-1],1);

		conn = seg_from->end;
	} else if ((seg_from->start == seg_to->start) || (seg_from->start == seg_to->end)) {
		ccnt = get_item_seg_coords(&seg_from->item, ca, 2, &seg_from->start->c, &seg_from->end->c);
		from_angle = route_road_angle(&ca[1], &ca[0],1);

		conn = seg_from->start;
	} else {
		// Not connected!
		return 1;
	}


	if (seg_to->end == conn) {
		ccnt = get_item_seg_coords(&seg_to->item, ca, 2047, &seg_to->start->c, &seg_to->end->c);
		to_angle = route_road_angle(&ca[ccnt-1], &ca[ccnt-2],1);
	} else {
		ccnt = get_item_seg_coords(&seg_to->item, ca, 2, &seg_to->start->c, &seg_to->end->c);
		to_angle = route_road_angle(&ca[0], &ca[1],1);
	}			
	
	
	angle_diff = from_angle - to_angle;
	if (angle_diff < 0) {
		angle_diff *= -1;
	}


	curr = conn->start;
	while (curr != NULL) {
		if (curr==seg_to) {
			curr = curr->start_next;
			continue;
		}
		
		ccnt = get_item_seg_coords(&curr->item, ca, 2, &curr->start->c, &curr->end->c);
		curr_angle = route_road_angle(&ca[0], &ca[1], 1);

		curr_angle = from_angle - curr_angle;

		if (curr_angle < 0) {
			curr_angle *= -1;
		}
		

		if (curr_angle < angle_diff) {
			return 0;
		}

		curr = curr->start_next;
	}

	curr = conn->end;
	while (curr != NULL) {
		if (curr==seg_to) {
			curr = curr->end_next;
			continue;
		}
		
		ccnt = get_item_seg_coords(&curr->item, ca, 2047, &curr->start->c, &curr->end->c);
		curr_angle = route_road_angle(&ca[ccnt-1], &ca[ccnt-2], 1);

		curr_angle = from_angle - curr_angle;

		if (curr_angle < 0) {
			curr_angle *= -1;
		}

		if (curr_angle < angle_diff) {
			return 0;
		}

		curr = curr->end_next;
	}

	return 1;
}

/**
 * @brief Creates a new route path
 * 
 * This creates a new non-trivial route. It therefore needs the routing information created by route_graph_flood, so
 * make shure to run route_graph_flood() after changing the destination before using this function.
 *
 * @param this The route graph to create the route from
 * @param oldpath (Optional) old path which may contain parts of the new part - this speeds things up a bit. May be NULL.
 * @param pos The starting position of the route
 * @param dst The destination of the route
 * @param speedlist The speedlist to use
 * @return The new route path
 */
static struct route_path *
route_path_new(struct route_graph *this, struct route_path *oldpath, struct route_info *pos, struct route_info *dst, int *speedlist)
{
	struct route_graph_point *start1=NULL,*start2=NULL,*start;
	struct route_graph_segment *s=NULL;
	struct route_graph_segment *lastseg = NULL;
	int is_straight;
	int len=0,segs=0;
	int seg_len;
#if 0
	int time=0,hr,min,sec
#endif
	unsigned int val1=0xffffffff,val2=0xffffffff;
	struct street_data *sd=pos->street;
	struct route_path *ret;

	if (item_is_equal(pos->street->item, dst->street->item)) { /* We probably don't have to leave this street and can use a trivial route */
		if (!(sd->flags & AF_ONEWAY) && pos->lenneg >= dst->lenneg) {
			return route_path_new_trivial(this, pos, dst, -1);
		}
		if (!(sd->flags & AF_ONEWAYREV) && pos->lenpos >= dst->lenpos) {
			return route_path_new_trivial(this, pos, dst, 1);
		}
	} 
	if (! (sd->flags & AF_ONEWAY)) { /* Using the start of the current segment as one starting point */
		start1=route_graph_get_point(this, &sd->c[0]);
		if (! start1)
			return NULL;
		val1=start1->value+route_value(speedlist, &sd->item, pos->lenneg);
		dbg(1,"start1: %d(route)+%d=%d\n", start1->value, val1-start1->value, val1);
	}
	if (! (sd->flags & AF_ONEWAYREV)) { /* Using the start of the current segment as an alternative starting point */
		start2=route_graph_get_point(this, &sd->c[sd->count-1]);
		if (! start2)
			return NULL;
		val2=start2->value+route_value(speedlist, &sd->item, pos->lenpos);
		dbg(1,"start2: %d(route)+%d=%d\n", start2->value, val2-start2->value, val2);
	}
	dbg(1,"val1=%d val2=%d\n", val1, val2);
	if (val1 == val2) {
		val1=start1->start->start->value;
		val2=start2->end->end->value;
	}
	ret=g_new0(struct route_path, 1);
	if (pos->lenextra) 
		route_path_add_item(ret, NULL, pos->lenextra, &pos->c, NULL, 0, &pos->lp, 1);
	if (start1 && (val1 < val2)) {
		start=start1;
		route_path_add_item(ret, &sd->item, pos->lenneg, &pos->lp, sd->c, pos->pos+1, NULL, -1);
	} else {
		if (start2) {
			start=start2;
			route_path_add_item(ret, &sd->item, pos->lenpos, &pos->lp, sd->c+pos->pos+1, sd->count-pos->pos-1, NULL, 1);
		} else {
			printf("no route found, pos blocked\n");
			return NULL;
		}
	}
	ret->path_hash=item_hash_new();
	while ((s=start->seg)) { /* following start->seg, which indicates the least costly way to reach our destination */
		segs++;
#if 0
		printf("start->value=%d 0x%x,0x%x\n", start->value, start->c.x, start->c.y);
#endif
		seg_len=s->len;
		len+=seg_len;
		
		if (lastseg) {
			is_straight = route_check_straight(lastseg,s);
		} else {
			is_straight = 0;
		}

		if (s->start == start) {		
			route_path_add_item_from_graph(ret, oldpath, s, seg_len, s->offset, 1, is_straight);
			start=s->end;
		} else {
			route_path_add_item_from_graph(ret, oldpath, s, seg_len, s->offset, -1, is_straight);
			start=s->start;
		}

		lastseg = s;
	}
	sd=dst->street;
	dbg(1,"start->value=%d 0x%x,0x%x\n", start->value, start->c.x, start->c.y);
	dbg(1,"dst sd->flags=%d sd->c[0]=0x%x,0x%x sd->c[sd->count-1]=0x%x,0x%x\n", sd->flags, sd->c[0].x,sd->c[0].y, sd->c[sd->count-1].x, sd->c[sd->count-1].y);
	if (start->c.x == sd->c[0].x && start->c.y == sd->c[0].y) { /* Adding a final segment to reach the destination within the destination street */
		route_path_add_item(ret, &sd->item, dst->lenneg, &dst->lp, sd->c, dst->pos+1, NULL, -1);
	} else if (start->c.x == sd->c[sd->count-1].x && start->c.y == sd->c[sd->count-1].y) {
		route_path_add_item(ret, &sd->item, dst->lenpos, &dst->lp, sd->c+dst->pos+1, sd->count-dst->pos-1, NULL, 1);
	} else {
		printf("no route found\n");
		route_path_destroy(ret);
		return NULL;
	}
	if (dst->lenextra) 
		route_path_add_item(ret, NULL, dst->lenextra, &dst->lp, NULL, 0, &dst->c, 1);
	dbg(1, "%d segments\n", segs);
	return ret;
}

/**
 * @brief Builds a new route graph from a mapset
 *
 * This function builds a new route graph from a map. Please note that this function does not
 * add any routing information to the route graph - this has to be done via the route_graph_flood()
 * function.
 *
 * The function does not create a graph covering the whole map, but only covering the rectangle
 * between c1 and c2.
 *
 * @param ms The mapset to build the route graph from
 * @param c1 Corner 1 of the rectangle to use from the map
 * @param c2 Corner 2 of the rectangle to use from the map
 * @return The new route graph.
 */
static struct route_graph *
route_graph_build(struct mapset *ms, struct coord *c1, struct coord *c2)
{
	struct route_graph *ret=g_new0(struct route_graph, 1);
	struct map_selection *sel;
	struct mapset_handle *h;
	struct map_rect *mr;
	struct map *m;
	struct item *item;

	if (!ret) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return ret;
	}
	sel=route_calc_selection(c1, c2);
	h=mapset_open(ms);
	while ((m=mapset_next(h,1))) {
		mr=map_rect_new(m, sel);
		if (! mr)
			continue;
		while ((item=map_rect_get_item(mr))) {
			if (item->type >= type_street_0 && item->type <= type_ferry) {
				route_process_street_graph(ret, item);
			}
		}
		map_rect_destroy(mr);
        }
        mapset_close(h);
	route_free_selection(sel);

	return ret;
}

/**
 * @brief Updates the route graph
 *
 * This updates the route graph after settings in the route have changed. It also
 * adds routing information afterwards by calling route_graph_flood().
 * 
 * @param this The route to update the graph for
 */
static void
route_graph_update(struct route *this)
{
	route_graph_destroy(this->graph);
	profile(1,"graph_free");
	this->graph=route_graph_build(this->ms, &this->pos->c, &this->dst->c);
	profile(1,"route_graph_build");
	route_graph_flood(this->graph, this->dst, this->speedlist);
	profile(1,"route_graph_flood");
	this->version++;
}

/**
 * @brief Gets street data for an item
 *
 * @param item The item to get the data for
 * @return Street data for the item
 */
struct street_data *
street_get_data (struct item *item)
{
	int count=0;
	struct street_data *ret = NULL, *ret1;
	struct attr attr;
	const int step = 128;
	int c;

	do {
		ret1=g_realloc(ret, sizeof(struct street_data)+(count+step)*sizeof(struct coord));
		if (!ret1) {
			if (ret)
				g_free(ret);
			return NULL;
		}
		ret = ret1;
		c = item_coord_get(item, &ret->c[count], step);
		count += c;
	} while (c && c == step);
	if (!count) {
		g_free(ret);
		return NULL;
	}
	ret1=g_realloc(ret, sizeof(struct street_data)+count*sizeof(struct coord));
	if (ret1)
		ret = ret1;
	ret->item=*item;
	ret->count=count;
	if (item_attr_get(item, attr_flags, &attr)) 
		ret->flags=attr.u.num;
	else
		ret->flags=0;

	return ret;
}

/**
 * @brief Copies street data
 * 
 * @param orig The street data to copy
 * @return The copied street data
 */ 
struct street_data *
street_data_dup(struct street_data *orig)
{
	struct street_data *ret;
	int size=sizeof(struct street_data)+orig->count*sizeof(struct coord);

	ret=g_malloc(size);
	memcpy(ret, orig, size);

	return ret;
}

/**
 * @brief Frees street data
 *
 * @param sd Street data to be freed
 */
void
street_data_free(struct street_data *sd)
{
	g_free(sd);
}

/**
 * @brief Finds the nearest street to a given coordinate
 *
 * @param ms The mapset to search in for the street
 * @param pc The coordinate to find a street nearby
 * @return The nearest street
 */
static struct route_info *
route_find_nearest_street(struct mapset *ms, struct pcoord *pc)
{
	struct route_info *ret=NULL;
	int max_dist=1000;
	struct map_selection *sel;
	int dist,mindist=0,pos;
	struct mapset_handle *h;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	struct coord lp;
	struct street_data *sd;
	struct coord c;
	struct coord_geo g;

	c.x = pc->x;
	c.y = pc->y;
	/*
	 * This is not correct for two reasons:
	 * - You may need to go back first
	 * - Currently we allow mixing of mapsets
	 */
	sel = route_rect(18, &c, &c, 0, max_dist);
	h=mapset_open(ms);
	while ((m=mapset_next(h,1))) {
		c.x = pc->x;
		c.y = pc->y;
		if (map_projection(m) != pc->pro) {
			transform_to_geo(pc->pro, &c, &g);
			transform_from_geo(map_projection(m), &g, &c);
		}
		mr=map_rect_new(m, sel);
		if (! mr)
			continue;
		while ((item=map_rect_get_item(mr))) {
			if (item->type >= type_street_0 && item->type <= type_ferry) {
				sd=street_get_data(item);
				dist=transform_distance_polyline_sq(sd->c, sd->count, &c, &lp, &pos);
				if (!ret || dist < mindist) {
					if (ret) {
						street_data_free(ret->street);
						g_free(ret);
					}
					ret=g_new(struct route_info, 1);
					if (!ret) {
						printf("%s:Out of memory\n", __FUNCTION__);
						return ret;
					}
					ret->c=c;
					ret->lp=lp;
					ret->pos=pos;
					mindist=dist;
					ret->street=sd;
					dbg(1,"dist=%d id 0x%x 0x%x pos=%d\n", dist, item->id_hi, item->id_lo, pos);
				} else 
					street_data_free(sd);
			}
		}  
		map_rect_destroy(mr);
	}
	mapset_close(h);
	map_selection_destroy(sel);

	return ret;
}

/**
 * @brief Destroys a route_info
 *
 * @param info The route info to be destroyed
 */
void
route_info_free(struct route_info *inf)
{
	if (inf->street)
		street_data_free(inf->street);
	g_free(inf);
}


#include "point.h"

/**
 * @brief Returns street data for a route info 
 *
 * @param rinf The route info to return the street data for
 * @return Street data for the route info
 */
struct street_data *
route_info_street(struct route_info *rinf)
{
	return rinf->street;
}

#if 0
struct route_crossings *
route_crossings_get(struct route *this, struct coord *c)
{
      struct route_point *pnt;
      struct route_segment *seg;
      int crossings=0;
      struct route_crossings *ret;

       pnt=route_graph_get_point(this, c);
       seg=pnt->start;
       while (seg) {
		printf("start: 0x%x 0x%x\n", seg->item.id_hi, seg->item.id_lo);
               crossings++;
               seg=seg->start_next;
       }
       seg=pnt->end;
       while (seg) {
		printf("end: 0x%x 0x%x\n", seg->item.id_hi, seg->item.id_lo);
               crossings++;
               seg=seg->end_next;
       }
       ret=g_malloc(sizeof(struct route_crossings)+crossings*sizeof(struct route_crossing));
       ret->count=crossings;
       return ret;
}
#endif


struct map_rect_priv {
	struct route_info_handle *ri;
	enum attr_type attr_next;
	int pos;
	struct map_priv *mpriv;
	struct item item;
	int length;
	unsigned int last_coord;
	struct route_path_segment *seg,*seg_next;
	struct route_graph_point *point;
	struct route_graph_segment *rseg;
	char *str;
};

static void
rm_coord_rewind(void *priv_data)
{
	struct map_rect_priv *mr = priv_data;
	mr->last_coord = 0;
}

static void
rm_attr_rewind(void *priv_data)
{
	struct map_rect_priv *mr = priv_data;
	mr->attr_next = attr_street_item;
}

static int
rm_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct map_rect_priv *mr = priv_data;
	struct route_path_segment *seg=mr->seg;
	struct route *route=mr->mpriv->route;
	attr->type=attr_type;
	switch (attr_type) {
		case attr_any:
			while (mr->attr_next != attr_none) {
				if (rm_attr_get(priv_data, mr->attr_next, attr))
					return 1;
			}
			return 0;
		case attr_street_item:
			mr->attr_next=attr_route_follow_straight;
			if (seg && seg->item.map)
				attr->u.item=&seg->item;
			else
				return 0;
			return 1;
		case attr_route_follow_straight:
			mr->attr_next=attr_direction;
			if (seg) {
				return attr_generic_get_attr(seg->attrs,NULL,attr_route_follow_straight,attr,NULL);
			}
			return 0;
		case attr_direction:
			mr->attr_next=attr_length;
			if (seg) 
				attr->u.num=seg->direction;
			else
				return 0;
			return 1;
		case attr_length:
			if (seg)
				attr->u.num=seg->length;
			else
				attr->u.num=mr->length;
			mr->attr_next=attr_time;
			return 1;
		case attr_time:
			mr->attr_next=attr_none;
			if (seg)
				attr->u.num=route_time(route->speedlist, &seg->item, seg->length);
			else
				return 0;
			return 1;
		case attr_label:
			mr->attr_next=attr_none;
			return 0;
		default:
			mr->attr_next=attr_none;
			attr->type=attr_none;
			return 0;
	}
	return 0;
}

static int
rm_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *mr = priv_data;
	struct route_path_segment *seg = mr->seg;
	int i,rc=0;
	struct route *r = mr->mpriv->route;
	enum projection pro = route_projection(r);

	if (! seg)
		return 0;
	for (i=0; i < count; i++) {
		if (mr->last_coord >= seg->ncoords)
			break;
		if (i >= seg->ncoords)
			break;
		if (pro != projection_mg)
			transform_from_to(&seg->c[mr->last_coord++], pro,
				&c[i],projection_mg);
		else
			c[i] = seg->c[mr->last_coord++];
		rc++;
	}
	dbg(1,"return %d\n",rc);
	return rc;
}

static struct item_methods methods_route_item = {
	rm_coord_rewind,
	rm_coord_get,
	rm_attr_rewind,
	rm_attr_get,
};

static void
rp_attr_rewind(void *priv_data)
{
	struct map_rect_priv *mr = priv_data;
	mr->attr_next = attr_label;
}

static int
rp_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct map_rect_priv *mr = priv_data;
	struct route_graph_point *p = mr->point;
	if (mr->item.type != type_rg_point) 
		return 0;
	switch (attr_type) {
	case attr_any:
		while (mr->attr_next != attr_none) {
			if (rp_attr_get(priv_data, mr->attr_next, attr))
				return 1;
		}
		return 0;
	case attr_label:
		attr->type = attr_label;
		if (mr->str)
			g_free(mr->str);
		if (p->value != INT_MAX)
			mr->str=g_strdup_printf("%d", p->value);
		else
			mr->str=g_strdup("-");
		attr->u.str = mr->str;
		mr->attr_next=attr_none;
		return 1;
	case attr_debug:
		attr->type = attr_debug;
		if (mr->str)
			g_free(mr->str);
		mr->str=g_strdup_printf("x=%d y=%d", p->c.x, p->c.y);
		attr->u.str = mr->str;
		mr->attr_next=attr_none;
		return 1;
	default:
		mr->attr_next=attr_none;
		attr->type=attr_none;
		return 0;
	}
}

static int
rp_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *mr = priv_data;
	struct route_graph_point *p = mr->point;
	struct route_graph_segment *seg = mr->rseg;
	int rc = 0,i,dir;
	struct route *r = mr->mpriv->route;
	enum projection pro = route_projection(r);

	for (i=0; i < count; i++) {
		if (mr->item.type == type_rg_point) {
			if (mr->last_coord >= 1)
				break;
			if (pro != projection_mg)
				transform_from_to(&p->c, pro,
					&c[i],projection_mg);
			else
				c[i] = p->c;
		} else {
			if (mr->last_coord >= 2)
				break;
			dir=0;
			if (seg->end->seg == seg)
				dir=1;
			if (mr->last_coord)
				dir=1-dir;
			if (dir) {
				if (pro != projection_mg)
					transform_from_to(&seg->end->c, pro,
						&c[i],projection_mg);
				else
					c[i] = seg->end->c;
			} else {
				if (pro != projection_mg)
					transform_from_to(&seg->start->c, pro,
						&c[i],projection_mg);
				else
					c[i] = seg->start->c;
			}
		}
		mr->last_coord++;
		rc++;
	}
	return rc;
}

static struct item_methods methods_point_item = {
	rm_coord_rewind,
	rp_coord_get,
	rp_attr_rewind,
	rp_attr_get,
};

static void
rm_destroy(struct map_priv *priv)
{
	g_free(priv);
}

static struct map_rect_priv * 
rm_rect_new(struct map_priv *priv, struct map_selection *sel)
{
	struct map_rect_priv * mr;
	dbg(1,"enter\n");
	if (! route_get_pos(priv->route))
		return NULL;
	if (! route_get_dst(priv->route))
		return NULL;
	if (! priv->route->path2)
		return NULL;
	mr=g_new0(struct map_rect_priv, 1);
	mr->mpriv = priv;
	mr->item.priv_data = mr;
	mr->item.type = type_street_route;
	mr->item.meth = &methods_route_item;
	mr->seg_next=priv->route->path2->path;
	return mr;
}

static struct map_rect_priv * 
rp_rect_new(struct map_priv *priv, struct map_selection *sel)
{
	struct map_rect_priv * mr;
	dbg(1,"enter\n");
	if (! priv->route->graph || ! priv->route->graph->route_points)
		return NULL;
	mr=g_new0(struct map_rect_priv, 1);
	mr->mpriv = priv;
	mr->item.priv_data = mr;
	mr->item.type = type_rg_point;
	mr->item.meth = &methods_point_item;
	return mr;
}

static void
rm_rect_destroy(struct map_rect_priv *mr)
{
	if (mr->str)
		g_free(mr->str);
	g_free(mr);
}

static struct item *
rp_get_item(struct map_rect_priv *mr)
{
	struct route *r = mr->mpriv->route;
	struct route_graph_point *p = mr->point;
	struct route_graph_segment *seg = mr->rseg;

	if (mr->item.type == type_rg_point) {
		if (!p)
			p = r->graph->route_points;
		else
			p = p->next;
		if (p) {
			mr->point = p;
			mr->item.id_lo++;
			rm_coord_rewind(mr);
			rp_attr_rewind(mr);
			return &mr->item;
		} else
			mr->item.type = type_rg_segment;
	}
	if (!seg)
		seg=r->graph->route_segments;
	else
		seg=seg->next;
	if (seg) {
		mr->rseg = seg;
		mr->item.id_lo++;
		rm_coord_rewind(mr);
		rp_attr_rewind(mr);
		return &mr->item;
	}
	return NULL;
	
}

static struct item *
rp_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	struct item *ret=NULL;
	while (id_lo-- > 0) 
		ret=rp_get_item(mr);
	return ret;
}


static struct item *
rm_get_item(struct map_rect_priv *mr)
{
	struct route *r = mr->mpriv->route;
	struct route_path_segment *seg = mr->seg;
	dbg(1,"enter\n", mr->pos);

	mr->seg=mr->seg_next;
	if (! mr->seg)
		return NULL;
	mr->seg_next=mr->seg->next;
	mr->last_coord = 0;
	mr->item.id_lo++;
	rm_attr_rewind(mr);
	return &mr->item;
}

static struct item *
rm_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	struct item *ret=NULL;
	while (id_lo-- > 0) 
		ret=rm_get_item(mr);
	return ret;
}

static struct map_methods route_meth = {
	projection_mg,
	"utf-8",
	rm_destroy,
	rm_rect_new,
	rm_rect_destroy,
	rm_get_item,
	rm_get_item_byid,
	NULL,
	NULL,
	NULL,
};

static struct map_methods route_graph_meth = {
	projection_mg,
	"utf-8",
	rm_destroy,
	rp_rect_new,
	rm_rect_destroy,
	rp_get_item,
	rp_get_item_byid,
	NULL,
	NULL,
	NULL,
};

void 
route_toggle_routegraph_display(struct route *route)
{
	if (route->flags & RF_SHOWGRAPH) {
		route->flags &= ~RF_SHOWGRAPH;
	} else {
		route->flags |= RF_SHOWGRAPH;
	}
}

static struct map_priv *
route_map_new_helper(struct map_methods *meth, struct attr **attrs, int graph)
{
	struct map_priv *ret;
	struct attr *route_attr;

	route_attr=attr_search(attrs, NULL, attr_route);
	if (! route_attr)
		return NULL;
	ret=g_new0(struct map_priv, 1);
	if (graph)
		*meth=route_graph_meth;
	else
		*meth=route_meth;
	ret->route=route_attr->u.route;

	return ret;
}

static struct map_priv *
route_map_new(struct map_methods *meth, struct attr **attrs)
{
	return route_map_new_helper(meth, attrs, 0);
}

static struct map_priv *
route_graph_map_new(struct map_methods *meth, struct attr **attrs)
{
	return route_map_new_helper(meth, attrs, 1);
}

static struct map *
route_get_map_helper(struct route *this_, struct map **map, char *type, char *description)
{
	if (! *map) 
		*map=map_new((struct attr*[]){
                                &(struct attr){attr_type,{type}},
                                &(struct attr){attr_route,.u.route=this_},
                                &(struct attr){attr_data,{""}},
                                &(struct attr){attr_description,{description}},
                                NULL});
	return *map;
}

struct map *
route_get_map(struct route *this_)
{
	return route_get_map_helper(this_, &this_->map, "route","Route");
}


struct map *
route_get_graph_map(struct route *this_)
{
	return route_get_map_helper(this_, &this_->graph_map, "route_graph","Route Graph");
}

void
route_set_projection(struct route *this_, enum projection pro)
{
}

void
route_init(void)
{
	plugin_register_map_type("route", route_map_new);
	plugin_register_map_type("route_graph", route_graph_map_new);
}
