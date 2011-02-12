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
#include "glib_slice.h"
#include "config.h"
#include "point.h"
#include "graphics.h"
#include "profile.h"
#include "coord.h"
#include "projection.h"
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "route.h"
#include "track.h"
#include "transform.h"
#include "plugin.h"
#include "fib.h"
#include "event.h"
#include "callback.h"
#include "vehicle.h"
#include "vehicleprofile.h"
#include "roadprofile.h"
#include "debug.h"

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
	int flags;						/**< Flags for this point (eg traffic distortion) */
};

#define RP_TRAFFIC_DISTORTION 1
#define RP_TURN_RESTRICTION 2
#define RP_TURN_RESTRICTION_RESOLVED 4

/**
 * @brief A segment in the route graph or path
 *
 * This is a segment in the route graph or path. A segment represents a driveable way.
 */

struct route_segment_data {
	struct item item;							/**< The item (e.g. street) that this segment represents. */
	int flags;
	int len;									/**< Length of this segment */
	/*NOTE: After a segment, various fields may follow, depending on what flags are set. Order of fields:
				1.) maxspeed			Maximum allowed speed on this segment. Present if AF_SPEED_LIMIT is set.
				2.) offset				If the item is segmented (i.e. represented by more than one segment), this
										indicates the position of this segment in the item. Present if AF_SEGMENTED is set.
	 */
};


struct size_weight_limit {
	int width;
	int length;
	int height;
	int weight;
	int axle_weight;
};

#define RSD_OFFSET(x) *((int *)route_segment_data_field_pos((x), attr_offset))
#define RSD_MAXSPEED(x) *((int *)route_segment_data_field_pos((x), attr_maxspeed))
#define RSD_SIZE_WEIGHT(x) *((struct size_weight_limit *)route_segment_data_field_pos((x), attr_vehicle_width))
#define RSD_DANGEROUS_GOODS(x) *((int *)route_segment_data_field_pos((x), attr_vehicle_dangerous_goods))


struct route_graph_segment_data {
	struct item *item;
	int offset;
	int flags;
	int len;
	int maxspeed;
	struct size_weight_limit size_weight;
	int dangerous_goods;
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
	struct route_segment_data data;				/**< The segment data */
};

/**
 * @brief A traffic distortion
 *
 * This is distortion in the traffic where you can't drive as fast as usual or have to wait for some time
 */
struct route_traffic_distortion {
	int maxspeed;					/**< Maximum speed possible in km/h */
	int delay;					/**< Delay in tenths of seconds */
};

/**
 * @brief A segment in the route path
 *
 * This is a segment in the route path.
 */
struct route_path_segment {
	struct route_path_segment *next;	/**< Pointer to the next segment in the path */
	struct route_segment_data *data;	/**< The segment data */
	int direction;						/**< Order in which the coordinates are ordered. >0 means "First
										 *  coordinate of the segment is the first coordinate of the item", <=0 
										 *  means reverse. */
	unsigned ncoords;					/**< How many coordinates does this segment have? */
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
	int percent;			 /**< ratio of lenneg to lenght of whole street in percent */
	struct street_data *street; /**< The street lp is on */
	int street_direction;	/**< Direction of vehicle on street -1 = Negative direction, 1 = Positive direction, 0 = Unknown */
	int dir;		/**< Direction to take when following the route -1 = Negative direction, 1 = Positive direction */
};

/**
 * @brief A complete route path
 *
 * This structure describes a whole routing path
 */
struct route_path {
	int in_use;						/**< The path is in use and can not be updated */
	int update_required;					/**< The path needs to be updated after it is no longer in use */
	int updated;						/**< The path has only been updated */
	int path_time;						/**< Time to pass the path */
	int path_len;						/**< Length of the path */
	struct route_path_segment *path;			/**< The first segment in the path, i.e. the segment one should 
												 *  drive in next */
	struct route_path_segment *path_last;		/**< The last segment in the path */
	/* XXX: path_hash is not necessery now */
	struct item_hash *path_hash;				/**< A hashtable of all the items represented by this route's segements */
	struct route_path *next;				/**< Next route path in case of intermediate destinations */	
};

/**
 * @brief A complete route
 * 
 * This struct holds all information about a route.
 */
struct route {
	struct mapset *ms;			/**< The mapset this route is built upon */
	unsigned flags;
	struct route_info *pos;		/**< Current position within this route */
	GList *destinations;		/**< Destinations of the route */
	struct route_info *current_dst;	/**< Current destination */

	struct route_graph *graph;	/**< Pointer to the route graph */
	struct route_path *path2;	/**< Pointer to the route path */
	struct map *map;
	struct map *graph_map;
	struct callback * route_graph_done_cb ; /**< Callback when route graph is done */
	struct callback * route_graph_flood_done_cb ; /**< Callback when route graph flooding is done */
	struct callback_list *cbl2;	/**< Callback list to call when route changes */
	int destination_distance;	/**< Distance to the destination at which the destination is considered "reached" */
	struct vehicleprofile *vehicleprofile; /**< Routing preferences */
	int route_status;		/**< Route Status */
	int link_path;			/**< Link paths over multiple waypoints together */
	struct pcoord pc;
	struct vehicle *v;
};

/**
 * @brief A complete route graph
 *
 * This structure describes a whole routing graph
 */
struct route_graph {
	int busy;					/**< The graph is being built */
	struct map_selection *sel;			/**< The rectangle selection for the graph */
	struct mapset_handle *h;			/**< Handle to the mapset */	
	struct map *m;					/**< Pointer to the currently active map */	
	struct map_rect *mr;				/**< Pointer to the currently active map rectangle */
	struct vehicleprofile *vehicleprofile;		/**< The vehicle profile */
	struct callback *idle_cb;			/**< Idle callback to process the graph */
	struct callback *done_cb;			/**< Callback when graph is done */
	struct event_idle *idle_ev;			/**< The pointer to the idle event */
   	struct route_graph_segment *route_segments; /**< Pointer to the first route_graph_segment in the linked list of all segments */
#define HASH_SIZE 8192
	struct route_graph_point *hash[HASH_SIZE];	/**< A hashtable containing all route_graph_points in this graph */
};

#define HASHCOORD(c) ((((c)->x +(c)->y) * 2654435761UL) & (HASH_SIZE-1))

/**
 * @brief Iterator to iterate through all route graph segments in a route graph point
 *
 * This structure can be used to iterate through all route graph segments connected to a
 * route graph point. Use this with the rp_iterator_* functions.
 */
struct route_graph_point_iterator {
	struct route_graph_point *p;		/**< The route graph point whose segments should be iterated */
	int end;							/**< Indicates if we have finished iterating through the "start" segments */
	struct route_graph_segment *next;	/**< The next segment to be returned */
};

struct attr_iter {
	union {
		GList *list;
	} u;
};

static struct route_info * route_find_nearest_street(struct vehicleprofile *vehicleprofile, struct mapset *ms, struct pcoord *c);
static struct route_graph_point *route_graph_get_point(struct route_graph *this, struct coord *c);
static void route_graph_update(struct route *this, struct callback *cb, int async);
static void route_graph_build_done(struct route_graph *rg, int cancel);
static struct route_path *route_path_new(struct route_graph *this, struct route_path *oldpath, struct route_info *pos, struct route_info *dst, struct vehicleprofile *profile);
static void route_process_street_graph(struct route_graph *this, struct item *item, struct vehicleprofile *profile);
static void route_graph_destroy(struct route_graph *this);
static void route_path_update(struct route *this, int cancel, int async);
static int route_time_seg(struct vehicleprofile *profile, struct route_segment_data *over, struct route_traffic_distortion *dist);
static void route_graph_flood(struct route_graph *this, struct route_info *dst, struct vehicleprofile *profile, struct callback *cb);
static void route_graph_reset(struct route_graph *this);


/**
 * @brief Returns the projection used for this route
 *
 * @param route The route to return the projection for
 * @return The projection used for this route
 */
static enum projection route_projection(struct route *route)
{
	struct street_data *street;
	struct route_info *dst=route_get_dst(route);
	if (!route->pos && !dst)
		return projection_none;
	street = route->pos ? route->pos->street : dst->street;
	if (!street || !street->item.map)
		return projection_none;
	return map_projection(street->item.map);
}

/**
 * @brief Creates a new graph point iterator 
 *
 * This function creates a new route graph point iterator, that can be used to
 * iterate through all segments connected to the point.
 *
 * @param p The route graph point to create the iterator from
 * @return A new iterator.
 */
static struct route_graph_point_iterator
rp_iterator_new(struct route_graph_point *p)
{
	struct route_graph_point_iterator it;

	it.p = p;
	if (p->start) {
		it.next = p->start;
		it.end = 0;
	} else {
		it.next = p->end;
		it.end = 1;
	}

	return it;
}

/**
 * @brief Gets the next segment connected to a route graph point from an iterator
 *
 * @param it The route graph point iterator to get the segment from
 * @return The next segment or NULL if there are no more segments
 */
static struct route_graph_segment
*rp_iterator_next(struct route_graph_point_iterator *it) 
{
	struct route_graph_segment *ret;

	ret = it->next;
	if (!ret) {
		return NULL;
	}

	if (!it->end) {
		if (ret->start_next) {
			it->next = ret->start_next;
		} else {
			it->next = it->p->end;
			it->end = 1;
		}
	} else {
		it->next = ret->end_next;
	}

	return ret;
}

/**
 * @brief Checks if the last segment returned from a route_graph_point_iterator comes from the end
 *
 * @param it The route graph point iterator to be checked
 * @return 1 if the last segment returned comes from the end of the route graph point, 0 otherwise
 */
static int
rp_iterator_end(struct route_graph_point_iterator *it) {
	if (it->end && (it->next != it->p->end)) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * @brief Destroys a route_path
 *
 * @param this The route_path to be destroyed
 */
static void
route_path_destroy(struct route_path *this, int recurse)
{
	struct route_path_segment *c,*n;
	struct route_path *next;
	while (this) {
		next=this->next;
		if (this->path_hash) {
			item_hash_destroy(this->path_hash);
			this->path_hash=NULL;
		}
		c=this->path;
		while (c) {
			n=c->next;
			g_free(c);
			c=n;
		}
		g_free(this);
		if (!recurse)
			break;
		this=next;
	}
}

/**
 * @brief Creates a completely new route structure
 *
 * @param attrs Not used
 * @return The newly created route
 */
struct route *
route_new(struct attr *parent, struct attr **attrs)
{
	struct route *this=g_new0(struct route, 1);
	struct attr dest_attr;

	if (attr_generic_get_attr(attrs, NULL, attr_destination_distance, &dest_attr, NULL)) {
		this->destination_distance = dest_attr.u.num;
	} else {
		this->destination_distance = 50; // Default value
	}
	this->cbl2=callback_list_new();

	return this;
}

/**
 * @brief Checks if a segment is part of a roundabout
 *
 * This function checks if a segment is part of a roundabout.
 *
 * @param seg The segment to be checked
 * @param level How deep to scan the route graph
 * @param direction Set this to 1 if we're entering the segment through its end, to 0 otherwise
 * @param origin Used internally, set to NULL
 * @return 1 If a roundabout was detected, 0 otherwise
 */
static int
route_check_roundabout(struct route_graph_segment *seg, int level, int direction, struct route_graph_segment *origin)
{
	struct route_graph_point_iterator it,it2;
	struct route_graph_segment *cur;
	int count=0;

	if (!level) {
		return 0;
	}
	if (!direction && !(seg->data.flags & AF_ONEWAY)) {
		return 0;
	}
	if (direction && !(seg->data.flags & AF_ONEWAYREV)) {
		return 0;
	}
	if (seg->data.flags & AF_ROUNDABOUT_VALID)
		return 0;
	
	if (!origin) {
		origin = seg;
	}

	if (!direction) {
		it = rp_iterator_new(seg->end);
	} else {
		it = rp_iterator_new(seg->start);
	}
	it2=it;
	
	while ((cur = rp_iterator_next(&it2)))
		count++;

	if (count > 3)
		return 0;
	cur = rp_iterator_next(&it);
	while (cur) {
		if (cur == seg) {
			cur = rp_iterator_next(&it);
			continue;
		}

		if (cur->data.item.type != origin->data.item.type) {
			// This street is of another type, can't be part of the roundabout
			cur = rp_iterator_next(&it);
			continue;
		}

		if (cur == origin) {
			seg->data.flags |= AF_ROUNDABOUT;
			return 1;
		}

		if (route_check_roundabout(cur, (level-1), rp_iterator_end(&it), origin)) {
			seg->data.flags |= AF_ROUNDABOUT;
			return 1;
		}

		cur = rp_iterator_next(&it);
	}

	return 0;
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
 * @brief Sets the vehicle profile of a route
 *
 * @param this The route to set the profile for
 * @param prof The vehicle profile
 */

void
route_set_profile(struct route *this, struct vehicleprofile *prof)
{
	if (this->vehicleprofile != prof) {
		this->vehicleprofile=prof;
		route_path_update(this, 1, 1);
	}
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
	struct route_info *dst=NULL;

	if (this->destinations)
		dst=g_list_last(this->destinations)->data;
	return dst;
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
	if (item_hash_lookup(this->path2->path_hash, item))
		return 1;
	if (! this->pos || !this->pos->street)
		return 0;
	return item_is_equal(this->pos->street->item, *item);

}

static struct route_info *
route_next_destination(struct route *this)
{
	if (!this->destinations)
		return NULL;
	return this->destinations->data;
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
	enum projection pro;
	struct route_info *dst=route_next_destination(this);

	if (!this->pos)
		return 0;
	if (!dst)
		return 0;
	
	sd = this->pos->street;

	if (!this->path2) {
		return 0;
	}

	if (!item_is_equal(this->pos->street->item, dst->street->item)) { 
		return 0;
	}

	if ((sd->flags & AF_ONEWAY) && (this->pos->lenneg >= dst->lenneg)) { // We would have to drive against the one-way road
		return 0;
	}
	if ((sd->flags & AF_ONEWAYREV) && (this->pos->lenpos >= dst->lenpos)) {
		return 0;
	}
	pro=route_projection(this);
	if (pro == projection_none)
		return 0;
	 
	if (transform_distance(pro, &this->pos->c, &dst->lp) > this->destination_distance) {
		return 0;
	}

	if (g_list_next(this->destinations))	
		return 1;
	else
		return 2;
}

static struct route_info *
route_previous_destination(struct route *this)
{
	GList *l=g_list_find(this->destinations, this->current_dst);
	if (!l)
		return this->pos;
	l=g_list_previous(l);
	if (!l)
		return this->pos;
	return l->data;
}

static void
route_path_update_done(struct route *this, int new_graph)
{
	struct route_path *oldpath=this->path2;
	struct attr route_status;
	struct route_info *prev_dst;
	route_status.type=attr_route_status;
	if (this->path2 && this->path2->in_use) {
		this->path2->update_required=1+new_graph;
		return;
	}
	route_status.u.num=route_status_building_path;
	route_set_attr(this, &route_status);
	prev_dst=route_previous_destination(this);
	if (this->link_path) {
		this->path2=route_path_new(this->graph, NULL, prev_dst, this->current_dst, this->vehicleprofile);
		if (this->path2)
		    this->path2->next=oldpath;
	} else {
		this->path2=route_path_new(this->graph, oldpath, prev_dst, this->current_dst, this->vehicleprofile);
		if (oldpath && this->path2) {
			this->path2->next=oldpath->next;
			route_path_destroy(oldpath,0);
		}
	}
	if (this->path2) {
		struct route_path_segment *seg=this->path2->path;
		int path_time=0,path_len=0;
		while (seg) {
			/* FIXME */
			int seg_time=route_time_seg(this->vehicleprofile, seg->data, NULL);
			if (seg_time == INT_MAX) {
				dbg(1,"error\n");
			} else
				path_time+=seg_time;
			path_len+=seg->data->len;
			seg=seg->next;
		}
		this->path2->path_time=path_time;
		this->path2->path_len=path_len;
		if (prev_dst != this->pos) {
			this->link_path=1;
			this->current_dst=prev_dst;
			route_graph_reset(this->graph);
			route_graph_flood(this->graph, this->current_dst, this->vehicleprofile, this->route_graph_flood_done_cb);
			return;
		}
		if (!new_graph && this->path2->updated)
			route_status.u.num=route_status_path_done_incremental;
		else
			route_status.u.num=route_status_path_done_new;
	} else 
		route_status.u.num=route_status_not_found;
	this->link_path=0;
	route_set_attr(this, &route_status);
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
route_path_update(struct route *this, int cancel, int async)
{
	dbg(1,"enter %d\n", cancel);
	if (! this->pos || ! this->destinations) {
		dbg(1,"destroy\n");
		route_path_destroy(this->path2,1);
		this->path2 = NULL;
		return;
	}
	if (cancel) {
		route_graph_destroy(this->graph);
		this->graph=NULL;
	}
	/* the graph is destroyed when setting the destination */
	if (this->graph) {
		if (this->graph->busy) {
			dbg(1,"busy building graph\n");
			return;
		}
		// we can try to update
		dbg(1,"try update\n");
		route_path_update_done(this, 0);
	} else {
		route_path_destroy(this->path2,1);
		this->path2 = NULL;
	}
	if (!this->graph || !this->path2) {
		dbg(1,"rebuild graph\n");
		if (! this->route_graph_flood_done_cb)
			this->route_graph_flood_done_cb=callback_new_2(callback_cast(route_path_update_done), this, (long)1);
		dbg(1,"route_graph_update\n");
		route_graph_update(this, this->route_graph_flood_done_cb, async);
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
	if (ri->lenneg || ri->lenpos)
		ri->percent=(ri->lenneg*100)/(ri->lenneg+ri->lenpos);
	else
		ri->percent=50;
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
	this->pos=route_find_nearest_street(this->vehicleprofile, this->ms, pos);

	// If there is no nearest street, bail out.
	if (!this->pos) return;

	this->pos->street_direction=0;
	dbg(1,"this->pos=%p\n", this->pos);
	route_info_distances(this->pos, pos->pro);
	route_path_update(this, 0, 1);
}

/**
 * @brief Sets a route's current position based on coordinates from tracking
 *
 * @param this The route to set the current position of
 * @param tracking The tracking to get the coordinates from
 */
void
route_set_position_from_tracking(struct route *this, struct tracking *tracking, enum projection pro)
{
	struct coord *c;
	struct route_info *ret;
	struct street_data *sd;

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
	ret->street_direction=tracking_get_street_direction(tracking);
	sd=tracking_get_street_data(tracking);
	if (sd) {
		ret->street=street_data_dup(sd);
		route_info_distances(ret, pro);
	}
	dbg(3,"position 0x%x,0x%x item 0x%x,0x%x direction %d pos %d lenpos %d lenneg %d\n",c->x,c->y,sd?sd->item.id_hi:0,sd?sd->item.id_lo:0,ret->street_direction,ret->pos,ret->lenpos,ret->lenneg);
	dbg(3,"c->x=0x%x, c->y=0x%x pos=%d item=(0x%x,0x%x)\n", c->x, c->y, ret->pos, ret->street?ret->street->item.id_hi:0, ret->street?ret->street->item.id_lo:0);
	dbg(3,"street 0=(0x%x,0x%x) %d=(0x%x,0x%x)\n", ret->street?ret->street->c[0].x:0, ret->street?ret->street->c[0].y:0, ret->street?ret->street->count-1:0, ret->street?ret->street->c[ret->street->count-1].x:0, ret->street?ret->street->c[ret->street->count-1].y:0);
	this->pos=ret;
	if (this->destinations) 
		route_path_update(this, 0, 1);
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
	sel->order=order;
	sel->range.min=route_item_first;
	sel->range.max=route_item_last;
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
route_calc_selection(struct coord *c, int count)
{
	struct map_selection *ret,*sel;
	int i;
	struct coord_rect r;

	if (!count)
		return NULL;
	r.lu=c[0];
	r.rl=c[0];
	for (i = 1 ; i < count ; i++)
		coord_rect_extend(&r, &c[i]);
	sel=route_rect(4, &r.lu, &r.rl, 25, 0);
	ret=sel;
	for (i = 0 ; i < count ; i++) {
		sel->next=route_rect(8, &c[i], &c[i], 0, 40000);
		sel=sel->next;
		sel->next=route_rect(18, &c[i], &c[i], 0, 10000);
		sel=sel->next;
	}
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


static void
route_clear_destinations(struct route *this_)
{
	g_list_foreach(this_->destinations, (GFunc)route_info_free, NULL);
	g_list_free(this_->destinations);
	this_->destinations=NULL;
}

/**
 * @brief Sets the destination of a route
 *
 * This sets the destination of a route to the street nearest to the coordinates passed
 * and updates the route.
 *
 * @param this The route to set the destination for
 * @param dst Coordinates to set as destination
 * @param count: Number of destinations (last one is final)
 * @param async: If set, do routing asynchronously
 */

void
route_set_destinations(struct route *this, struct pcoord *dst, int count, int async)
{
	struct attr route_status;
	struct route_info *dsti;
	int i;
	route_status.type=attr_route_status;

	profile(0,NULL);
	route_clear_destinations(this);
	if (dst && count) {
		for (i = 0 ; i < count ; i++) {
			dsti=route_find_nearest_street(this->vehicleprofile, this->ms, &dst[i]);
			if(dsti) {
				route_info_distances(dsti, dst->pro);
				this->destinations=g_list_append(this->destinations, dsti);
			}
		}
		route_status.u.num=route_status_destination_set;
	} else  
		route_status.u.num=route_status_no_destination;
	callback_list_call_attr_1(this->cbl2, attr_destination, this);
	route_set_attr(this, &route_status);
	profile(1,"find_nearest_street");

	/* The graph has to be destroyed and set to NULL, otherwise route_path_update() doesn't work */
	route_graph_destroy(this->graph);
	this->graph=NULL;
	this->current_dst=route_get_dst(this);
	route_path_update(this, 1, async);
	profile(0,"end");
}

int
route_get_destinations(struct route *this, struct pcoord *pc, int count)
{
	int ret=0;
	GList *l=this->destinations;
	while (l && ret < count) {
		struct route_info *dst=l->data;
		pc->x=dst->c.x;
		pc->y=dst->c.y;
		pc->pro=projection_mg; /* FIXME */
		pc++;
		ret++;
		l=g_list_next(l);
	}
	return ret;
}
 
void
route_set_destination(struct route *this, struct pcoord *dst, int async)
{
	route_set_destinations(this, dst, dst?1:0, async);
}

void
route_remove_waypoint(struct route *this)
{
	struct route_path *path=this->path2;
	this->destinations=g_list_remove(this->destinations,this->destinations->data);
	this->path2=this->path2->next;
	route_path_destroy(path,0);
	if (!this->destinations)
		return;
	route_graph_reset(this->graph);
	this->current_dst=this->destinations->data;
	route_graph_flood(this->graph, this->current_dst, this->vehicleprofile, this->route_graph_flood_done_cb);
	
}

/**
 * @brief Gets the route_graph_point with the specified coordinates
 *
 * @param this The route in which to search
 * @param c Coordinates to search for
 * @param last The last route graph point returned to iterate over multiple points with the same coordinates
 * @return The point at the specified coordinates or NULL if not found
 */
static struct route_graph_point *
route_graph_get_point_next(struct route_graph *this, struct coord *c, struct route_graph_point *last)
{
	struct route_graph_point *p;
	int seen=0,hashval=HASHCOORD(c);
	p=this->hash[hashval];
	while (p) {
		if (p->c.x == c->x && p->c.y == c->y) {
			if (!last || seen)
				return p;
			if (p == last)
				seen=1;
		}
		p=p->hash_next;
	}
	return NULL;
}

static struct route_graph_point *
route_graph_get_point(struct route_graph *this, struct coord *c)
{
	return route_graph_get_point_next(this, c, NULL);
}

/**
 * @brief Gets the last route_graph_point with the specified coordinates 
 *
 * @param this The route in which to search
 * @param c Coordinates to search for
 * @return The point at the specified coordinates or NULL if not found
 */
static struct route_graph_point *
route_graph_get_point_last(struct route_graph *this, struct coord *c)
{
	struct route_graph_point *p,*ret=NULL;
	int hashval=HASHCOORD(c);
	p=this->hash[hashval];
	while (p) {
		if (p->c.x == c->x && p->c.y == c->y)
			ret=p;
		p=p->hash_next;
	}
	return ret;
}



/**
 * @brief Create a new point for the route graph with the specified coordinates
 *
 * @param this The route to insert the point into
 * @param f The coordinates at which the point should be created
 * @return The point created
 */

static struct route_graph_point *
route_graph_point_new(struct route_graph *this, struct coord *f)
{
	int hashval;
	struct route_graph_point *p;

	hashval=HASHCOORD(f);
	if (debug_route)
		printf("p (0x%x,0x%x)\n", f->x, f->y);
	p=g_slice_new0(struct route_graph_point);
	p->hash_next=this->hash[hashval];
	this->hash[hashval]=p;
	p->value=INT_MAX;
	p->c=*f;
	return p;
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
	struct route_graph_point *p;

	p=route_graph_get_point(this,f);
	if (!p)
		p=route_graph_point_new(this,f);
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
	int i;
	for (i = 0 ; i < HASH_SIZE ; i++) {
		curr=this->hash[i];
		while (curr) {
			next=curr->hash_next;
			g_slice_free(struct route_graph_point, curr);
			curr=next;
		}
		this->hash[i]=NULL;
	}
}

/**
 * @brief Resets all nodes
 *
 * @param this The route graph to reset
 */
static void
route_graph_reset(struct route_graph *this)
{
	struct route_graph_point *curr;
	int i;
	for (i = 0 ; i < HASH_SIZE ; i++) {
		curr=this->hash[i];
		while (curr) {
			curr->value=INT_MAX;
			curr->seg=NULL;
			curr->el=NULL;
			curr=curr->hash_next;
		}
	}
}

/**
 * @brief Returns the position of a certain field appended to a route graph segment
 *
 * This function returns a pointer to a field that is appended to a route graph
 * segment.
 *
 * @param seg The route graph segment the field is appended to
 * @param type Type of the field that should be returned
 * @return A pointer to a field of a certain type, or NULL if no such field is present
 */
static void *
route_segment_data_field_pos(struct route_segment_data *seg, enum attr_type type)
{
	unsigned char *ptr;
	
	ptr = ((unsigned char*)seg) + sizeof(struct route_segment_data);

	if (seg->flags & AF_SPEED_LIMIT) {
		if (type == attr_maxspeed) 
			return (void*)ptr;
		ptr += sizeof(int);
	}
	if (seg->flags & AF_SEGMENTED) {
		if (type == attr_offset) 
			return (void*)ptr;
		ptr += sizeof(int);
	}
	if (seg->flags & AF_SIZE_OR_WEIGHT_LIMIT) {
		if (type == attr_vehicle_width)
			return (void*)ptr;
		ptr += sizeof(struct size_weight_limit);
	}
	if (seg->flags & AF_DANGEROUS_GOODS) {
		if (type == attr_vehicle_dangerous_goods)
			return (void*)ptr;
		ptr += sizeof(int);
	}
	return NULL;
}

/**
 * @brief Calculates the size of a route_segment_data struct with given flags
 *
 * @param flags The flags of the route_segment_data
 */

static int
route_segment_data_size(int flags)
{
	int ret=sizeof(struct route_segment_data);
	if (flags & AF_SPEED_LIMIT)
		ret+=sizeof(int);
	if (flags & AF_SEGMENTED)
		ret+=sizeof(int);
	if (flags & AF_SIZE_OR_WEIGHT_LIMIT)
		ret+=sizeof(struct size_weight_limit);
	if (flags & AF_DANGEROUS_GOODS)
		ret+=sizeof(int);
	return ret;
}


static int
route_graph_segment_is_duplicate(struct route_graph_point *start, struct route_graph_segment_data *data)
{
	struct route_graph_segment *s;
	s=start->start;
	while (s) {
		if (item_is_equal(*data->item, s->data.item)) {
			if (data->flags & AF_SEGMENTED) {
				if (RSD_OFFSET(&s->data) == data->offset) {
					return 1;
				}
			} else
				return 1;
		}
		s=s->start_next;
	} 
	return 0;
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
 * @param maxspeed The maximum speed allowed on this segment in km/h. -1 if not known.
 */
static void
route_graph_add_segment(struct route_graph *this, struct route_graph_point *start,
			struct route_graph_point *end, struct route_graph_segment_data *data)
{
	struct route_graph_segment *s;
	int size;

	size = sizeof(struct route_graph_segment)-sizeof(struct route_segment_data)+route_segment_data_size(data->flags);
	s = g_slice_alloc0(size);
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
	dbg_assert(data->len >= 0);
	s->data.len=data->len;
	s->data.item=*data->item;
	s->data.flags=data->flags;

	if (data->flags & AF_SPEED_LIMIT) 
		RSD_MAXSPEED(&s->data)=data->maxspeed;
	if (data->flags & AF_SEGMENTED) 
		RSD_OFFSET(&s->data)=data->offset;
	if (data->flags & AF_SIZE_OR_WEIGHT_LIMIT) 
		RSD_SIZE_WEIGHT(&s->data)=data->size_weight;
	if (data->flags & AF_DANGEROUS_GOODS) 
		RSD_DANGEROUS_GOODS(&s->data)=data->dangerous_goods;

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
 * @important Make sure that whatever c points to has enough memory allocated
 * @important to hold max coordinates!
 *
 * @param i The item to get the coordinates of
 * @param c Pointer to memory allocated for holding the coordinates
 * @param max Maximum number of coordinates to return
 * @param start First coordinate to get
 * @param end Last coordinate to get
 * @return The number of coordinates returned
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
	int soffset;
	struct route_path_segment *sp = NULL, *s;
	s = path->path;
	while (s) {
		if (item_is_equal(s->data->item,*item)) {
			if (s->data->flags & AF_SEGMENTED)
			 	soffset=RSD_OFFSET(s->data);
			else
				soffset=1;
			if (soffset == offset) {
				if (sp) {
					sp->next = s->next;
					break;
				} else {
					path->path = s->next;
					break;
				}
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
 * @brief Adds a two coordinate line to a path
 *
 * This adds a new line to a path, creating a new segment for it.
 *
 * @param this The path to add the item to
 * @param start coordinate to add to the start of the item. If none should be added, make this NULL.
 * @param end coordinate to add to the end of the item. If none should be added, make this NULL.
 * @param len The length of the item
 */
static void
route_path_add_line(struct route_path *this, struct coord *start, struct coord *end, int len)
{
	int ccnt=2;
	struct route_path_segment *segment;
	int seg_size,seg_dat_size;

	dbg(1,"line from 0x%x,0x%x-0x%x,0x%x\n", start->x, start->y, end->x, end->y);
	seg_size=sizeof(*segment) + sizeof(struct coord) * ccnt;
        seg_dat_size=sizeof(struct route_segment_data);
        segment=g_malloc0(seg_size + seg_dat_size);
        segment->data=(struct route_segment_data *)((char *)segment+seg_size);
	segment->ncoords=ccnt;
	segment->direction=0;
	segment->c[0]=*start;
	segment->c[1]=*end;
	segment->data->len=len;
	route_path_add_segment(this, segment);
}

/**
 * @brief Inserts a new item into the path
 * 
 * This function does almost the same as "route_path_add_item()", but identifies
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
 * @param dir Order in which to add the coordinates. See route_path_add_item()
 * @param pos  Information about start point if this is the first segment
 * @param dst  Information about end point if this is the last segment
 */

static int
route_path_add_item_from_graph(struct route_path *this, struct route_path *oldpath, struct route_graph_segment *rgs, int dir, struct route_info *pos, struct route_info *dst)
{
	struct route_path_segment *segment;
	int i, ccnt, extra=0, ret=0;
	struct coord *c,*cd,ca[2048];
	int offset=1;
	int seg_size,seg_dat_size;
	int len=rgs->data.len;
	if (rgs->data.flags & AF_SEGMENTED) 
		offset=RSD_OFFSET(&rgs->data);

	dbg(1,"enter (0x%x,0x%x) dir=%d pos=%p dst=%p\n", rgs->data.item.id_hi, rgs->data.item.id_lo, dir, pos, dst);
	if (oldpath) {
		segment=item_hash_lookup(oldpath->path_hash, &rgs->data.item);
		if (segment && segment->direction == dir) {
			segment = route_extract_segment_from_path(oldpath, &rgs->data.item, offset);
			if (segment) {
				ret=1;
				if (!pos)
					goto linkold;
			}
		}
	}

	if (pos) {
		if (dst) {
			extra=2;
			if (dst->lenneg >= pos->lenneg) {
				dir=1;
				ccnt=dst->pos-pos->pos;
				c=pos->street->c+pos->pos+1;
				len=dst->lenneg-pos->lenneg;
			} else {
				dir=-1;
				ccnt=pos->pos-dst->pos;
				c=pos->street->c+dst->pos+1;
				len=pos->lenneg-dst->lenneg;
			}
		} else {
			extra=1;
			dbg(1,"pos dir=%d\n", dir);
			dbg(1,"pos pos=%d\n", pos->pos);
			dbg(1,"pos count=%d\n", pos->street->count);
			if (dir > 0) {
				c=pos->street->c+pos->pos+1;
				ccnt=pos->street->count-pos->pos-1;
				len=pos->lenpos;
			} else {
				c=pos->street->c;
				ccnt=pos->pos+1;
				len=pos->lenneg;
			}
		}
		pos->dir=dir;
	} else 	if (dst) {
		extra=1;
		dbg(1,"dst dir=%d\n", dir);
		dbg(1,"dst pos=%d\n", dst->pos);
		if (dir > 0) {
			c=dst->street->c;
			ccnt=dst->pos+1;
			len=dst->lenpos;
		} else {
			c=dst->street->c+dst->pos+1;
			ccnt=dst->street->count-dst->pos-1;
			len=dst->lenneg;
		}
	} else {
		ccnt=get_item_seg_coords(&rgs->data.item, ca, 2047, &rgs->start->c, &rgs->end->c);
		c=ca;
	}
	seg_size=sizeof(*segment) + sizeof(struct coord) * (ccnt + extra);
	seg_dat_size=route_segment_data_size(rgs->data.flags);
	segment=g_malloc0(seg_size + seg_dat_size);
	segment->data=(struct route_segment_data *)((char *)segment+seg_size);
	segment->direction=dir;
	cd=segment->c;
	if (pos && (c[0].x != pos->lp.x || c[0].y != pos->lp.y))
		*cd++=pos->lp;
	if (dir < 0)
		c+=ccnt-1;
	for (i = 0 ; i < ccnt ; i++) {
		*cd++=*c;
		c+=dir;	
	}
	segment->ncoords+=ccnt;
	if (dst && (cd[-1].x != dst->lp.x || cd[-1].y != dst->lp.y)) 
		*cd++=dst->lp;
	segment->ncoords=cd-segment->c;
	if (segment->ncoords <= 1) {
		g_free(segment);
		return 1;
	}

	/* We check if the route graph segment is part of a roundabout here, because this
	 * only matters for route graph segments which form parts of the route path */
	if (!(rgs->data.flags & AF_ROUNDABOUT)) { // We identified this roundabout earlier
		route_check_roundabout(rgs, 13, (dir < 1), NULL);
	}

	memcpy(segment->data, &rgs->data, seg_dat_size);
linkold:
	segment->data->len=len;
	segment->next=NULL;
	item_hash_insert(this->path_hash,  &rgs->data.item, segment);

	route_path_add_segment(this, segment);

	return ret;
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
	int size;
	curr=this->route_segments;
	while (curr) {
		next=curr->next;
		size = sizeof(struct route_graph_segment)-sizeof(struct route_segment_data)+route_segment_data_size(curr->data.flags);
		g_slice_free1(size, curr);
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
		route_graph_build_done(this, 1);
		route_graph_free_points(this);
		route_graph_free_segments(this);
		g_free(this);
	}
}

/**
 * @brief Returns the time needed to drive len on item
 *
 * This function returns the time needed to drive len meters on 
 * the item passed in item in tenth of seconds.
 *
 * @param profile The routing preferences
 * @param over The segment which is passed
 * @return The time needed to drive len on item in thenth of senconds
 */

static int
route_time_seg(struct vehicleprofile *profile, struct route_segment_data *over, struct route_traffic_distortion *dist)
{
	struct roadprofile *roadprofile=vehicleprofile_get_roadprofile(profile, over->item.type);
	int speed,maxspeed;
	if (!roadprofile || !roadprofile->route_weight)
		return INT_MAX;
	/* maxspeed_handling: 0=always, 1 only if maxspeed restricts the speed, 2 never */
	speed=roadprofile->route_weight;
	if (profile->maxspeed_handling != 2) {
		if (over->flags & AF_SPEED_LIMIT) {
			maxspeed=RSD_MAXSPEED(over);
			if (!profile->maxspeed_handling)
				speed=maxspeed;
		} else
			maxspeed=INT_MAX;
		if (dist && maxspeed > dist->maxspeed)
			maxspeed=dist->maxspeed;
		if (maxspeed != INT_MAX && (profile->maxspeed_handling != 1 || maxspeed < speed))
			speed=maxspeed;
	}
	if (over->flags & AF_DANGEROUS_GOODS) {
		if (profile->dangerous_goods & RSD_DANGEROUS_GOODS(over))
			return INT_MAX;
	}
	if (over->flags & AF_SIZE_OR_WEIGHT_LIMIT) {
		struct size_weight_limit *size_weight=&RSD_SIZE_WEIGHT(over);
		if (size_weight->width != -1 && profile->width != -1 && profile->width > size_weight->width)
			return INT_MAX;
		if (size_weight->height != -1 && profile->height != -1 && profile->height > size_weight->height)
			return INT_MAX;
		if (size_weight->length != -1 && profile->length != -1 && profile->length > size_weight->length)
			return INT_MAX;
		if (size_weight->weight != -1 && profile->weight != -1 && profile->weight > size_weight->weight)
			return INT_MAX;
		if (size_weight->axle_weight != -1 && profile->axle_weight != -1 && profile->axle_weight > size_weight->axle_weight)
			return INT_MAX;
	}
	if (!speed)
		return INT_MAX;
	return over->len*36/speed+(dist ? dist->delay : 0);
}

static int
route_get_traffic_distortion(struct route_graph_segment *seg, struct route_traffic_distortion *ret)
{
	struct route_graph_point *start=seg->start;
	struct route_graph_point *end=seg->end;
	struct route_graph_segment *tmp,*found=NULL;
	tmp=start->start;
	while (tmp && !found) {
		if (tmp->data.item.type == type_traffic_distortion && tmp->start == start && tmp->end == end)
			found=tmp;
		tmp=tmp->start_next;
	}
	tmp=start->end;
	while (tmp && !found) {
		if (tmp->data.item.type == type_traffic_distortion && tmp->end == start && tmp->start == end) 
			found=tmp;
		tmp=tmp->end_next;
	}
	if (found) {
		ret->delay=found->data.len;
		if (found->data.flags & AF_SPEED_LIMIT)
			ret->maxspeed=RSD_MAXSPEED(&found->data);
		else
			ret->maxspeed=INT_MAX;
		return 1;
	}
	return 0;
}

static int
route_through_traffic_allowed(struct vehicleprofile *profile, struct route_graph_segment *seg)
{
	return (seg->data.flags & AF_THROUGH_TRAFFIC_LIMIT) == 0;
}

/**
 * @brief Returns the "costs" of driving from point from over segment over in direction dir
 *
 * @param profile The routing preferences
 * @param from The point where we are starting
 * @param over The segment we are using
 * @param dir The direction of segment which we are driving
 * @return The "costs" needed to drive len on item
 */  

static int
route_value_seg(struct vehicleprofile *profile, struct route_graph_point *from, struct route_graph_segment *over, int dir)
{
	int ret;
	struct route_traffic_distortion dist,*distp=NULL;
#if 0
	dbg(0,"flags 0x%x mask 0x%x flags 0x%x\n", over->flags, dir >= 0 ? profile->flags_forward_mask : profile->flags_reverse_mask, profile->flags);
#endif
	if ((over->data.flags & (dir >= 0 ? profile->flags_forward_mask : profile->flags_reverse_mask)) != profile->flags)
		return INT_MAX;
	if (dir > 0 && (over->start->flags & RP_TURN_RESTRICTION))
		return INT_MAX;
	if (dir < 0 && (over->end->flags & RP_TURN_RESTRICTION))
		return INT_MAX;
	if (from && from->seg == over)
		return INT_MAX;
	if ((over->start->flags & RP_TRAFFIC_DISTORTION) && (over->end->flags & RP_TRAFFIC_DISTORTION) && 
		route_get_traffic_distortion(over, &dist)) {
			distp=&dist;
	}
	ret=route_time_seg(profile, &over->data, distp);
	if (ret == INT_MAX)
		return ret;
	if (!route_through_traffic_allowed(profile, over) && from && route_through_traffic_allowed(profile, from->seg)) 
		ret+=profile->through_traffic_penalty;
	return ret;
}

/**
 * @brief Adds a route distortion item to the route graph
 *
 * @param this The route graph to add to
 * @param item The item to add
 */
static void
route_process_traffic_distortion(struct route_graph *this, struct item *item)
{
	struct route_graph_point *s_pnt,*e_pnt;
	struct coord c,l;
	struct attr delay_attr, maxspeed_attr;
	struct route_graph_segment_data data;

	data.item=item;
	data.len=0;
	data.flags=0;
	data.offset=1;
	data.maxspeed = INT_MAX;

	if (item_coord_get(item, &l, 1)) {
		s_pnt=route_graph_add_point(this,&l);
		while (item_coord_get(item, &c, 1)) {
			l=c;
		}
		e_pnt=route_graph_add_point(this,&l);
		s_pnt->flags |= RP_TRAFFIC_DISTORTION;
		e_pnt->flags |= RP_TRAFFIC_DISTORTION;
		if (item_attr_get(item, attr_maxspeed, &maxspeed_attr)) {
			data.flags |= AF_SPEED_LIMIT;
			data.maxspeed=maxspeed_attr.u.num;
		}
		if (item_attr_get(item, attr_delay, &delay_attr))
			data.len=delay_attr.u.num;
		route_graph_add_segment(this, s_pnt, e_pnt, &data);
	}
}

/**
 * @brief Adds a route distortion item to the route graph
 *
 * @param this The route graph to add to
 * @param item The item to add
 */
static void
route_process_turn_restriction(struct route_graph *this, struct item *item)
{
	struct route_graph_point *pnt[4];
	struct coord c[5];
	int i,count;
	struct route_graph_segment_data data;

	count=item_coord_get(item, c, 5);
	if (count != 3 && count != 4) {
		dbg(0,"wrong count %d\n",count);
		return;
	}
	if (count == 4)
		return;
	for (i = 0 ; i < count ; i++) 
		pnt[i]=route_graph_add_point(this,&c[i]);
	dbg(1,"%s: (0x%x,0x%x)-(0x%x,0x%x)-(0x%x,0x%x) %p-%p-%p\n",item_to_name(item->type),c[0].x,c[0].y,c[1].x,c[1].y,c[2].x,c[2].y,pnt[0],pnt[1],pnt[2]);
	data.item=item;
	data.flags=0;
	data.len=0;
	route_graph_add_segment(this, pnt[0], pnt[1], &data);
	route_graph_add_segment(this, pnt[1], pnt[2], &data);
#if 1
	if (count == 4) {
		pnt[1]->flags |= RP_TURN_RESTRICTION;
		pnt[2]->flags |= RP_TURN_RESTRICTION;
		route_graph_add_segment(this, pnt[2], pnt[3], &data);
	} else 
		pnt[1]->flags |= RP_TURN_RESTRICTION;
#endif	
}

/**
 * @brief Adds an item to the route graph
 *
 * This adds an item (e.g. a street) to the route graph, creating as many segments as needed for a
 * segmented item.
 *
 * @param this The route graph to add to
 * @param item The item to add
 * @param profile		The vehicle profile currently in use
 */
static void
route_process_street_graph(struct route_graph *this, struct item *item, struct vehicleprofile *profile)
{
#ifdef AVOID_FLOAT
	int len=0;
#else
	double len=0;
#endif
	int segmented = 0;
	struct roadprofile *roadp;
	struct route_graph_point *s_pnt,*e_pnt;
	struct coord c,l;
	struct attr attr;
	struct route_graph_segment_data data;
	data.flags=0;
	data.offset=1;
	data.maxspeed=-1;
	data.item=item;

	roadp = vehicleprofile_get_roadprofile(profile, item->type);
	if (!roadp) {
		// Don't include any roads that don't have a road profile in our vehicle profile
		return;
	}

	if (item_coord_get(item, &l, 1)) {
		int *default_flags=item_get_default_flags(item->type);
		if (! default_flags)
			return;
		if (item_attr_get(item, attr_flags, &attr)) {
			data.flags = attr.u.num;
			if (data.flags & AF_SEGMENTED)
				segmented = 1;
		} else
			data.flags = *default_flags;
		

		if (data.flags & AF_SPEED_LIMIT) {
			if (item_attr_get(item, attr_maxspeed, &attr)) 
				data.maxspeed = attr.u.num;
		}
		if (data.flags & AF_DANGEROUS_GOODS) {
			if (item_attr_get(item, attr_vehicle_dangerous_goods, &attr)) 
				data.dangerous_goods = attr.u.num;
			else 
				data.flags &= ~AF_DANGEROUS_GOODS;
		}
		if (data.flags & AF_SIZE_OR_WEIGHT_LIMIT) {
			if (item_attr_get(item, attr_vehicle_width, &attr))
				data.size_weight.width=attr.u.num;
			else
				data.size_weight.width=-1;
			if (item_attr_get(item, attr_vehicle_height, &attr))
				data.size_weight.height=attr.u.num;
			else
				data.size_weight.height=-1;
			if (item_attr_get(item, attr_vehicle_length, &attr))
				data.size_weight.length=attr.u.num;
			else
				data.size_weight.length=-1;
			if (item_attr_get(item, attr_vehicle_weight, &attr))
				data.size_weight.weight=attr.u.num;
			else
				data.size_weight.weight=-1;
			if (item_attr_get(item, attr_vehicle_axle_weight, &attr))
				data.size_weight.axle_weight=attr.u.num;
			else
				data.size_weight.axle_weight=-1;
		}

		s_pnt=route_graph_add_point(this,&l);
		if (!segmented) {
			while (item_coord_get(item, &c, 1)) {
				len+=transform_distance(map_projection(item->map), &l, &c);
				l=c;
			}
			e_pnt=route_graph_add_point(this,&l);
			dbg_assert(len >= 0);
			data.len=len;
			if (!route_graph_segment_is_duplicate(s_pnt, &data))
				route_graph_add_segment(this, s_pnt, e_pnt, &data);
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
						data.len=len;
						if (!route_graph_segment_is_duplicate(s_pnt, &data))
							route_graph_add_segment(this, s_pnt, e_pnt, &data);
						data.offset++;
						s_pnt=route_graph_add_point(this,&l);
						len = 0;
					}
				}
			} while(rc);
			e_pnt=route_graph_add_point(this,&l);
			dbg_assert(len >= 0);
			sc++;
			data.len=len;
			if (!route_graph_segment_is_duplicate(s_pnt, &data))
				route_graph_add_segment(this, s_pnt, e_pnt, &data);
		}
	}
}

static struct route_graph_segment *
route_graph_get_segment(struct route_graph *graph, struct street_data *sd, struct route_graph_segment *last)
{
	struct route_graph_point *start=NULL;
	struct route_graph_segment *s;
	int seen=0;

	while ((start=route_graph_get_point_next(graph, &sd->c[0], start))) {
		s=start->start;
		while (s) {
			if (item_is_equal(sd->item, s->data.item)) {
				if (!last || seen)
					return s;
				if (last == s)
					seen=1;
			}
			s=s->start_next;
		}
	}
	return NULL;
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
route_graph_flood(struct route_graph *this, struct route_info *dst, struct vehicleprofile *profile, struct callback *cb)
{
	struct route_graph_point *p_min;
	struct route_graph_segment *s=NULL;
	int min,new,old,val;
	struct fibheap *heap; /* This heap will hold all points with "temporarily" calculated costs */

	heap = fh_makekeyheap();   

	while ((s=route_graph_get_segment(this, dst->street, s))) {
		val=route_value_seg(profile, NULL, s, -1);
		if (val != INT_MAX) {
			val=val*(100-dst->percent)/100;
			s->end->seg=s;
			s->end->value=val;
			s->end->el=fh_insertkey(heap, s->end->value, s->end);
		}
		val=route_value_seg(profile, NULL, s, 1);
		if (val != INT_MAX) {
			val=val*dst->percent/100;
			s->start->seg=s;
			s->start->value=val;
			s->start->el=fh_insertkey(heap, s->start->value, s->start);
		}
	}
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
			val=route_value_seg(profile, p_min, s, -1);
			if (val != INT_MAX && !item_is_equal(s->data.item,p_min->seg->data.item)) {
				new=min+val;
				if (debug_route)
					printf("begin %d len %d vs %d (0x%x,0x%x)\n",new,val,s->end->value, s->end->c.x, s->end->c.y);
				if (new < s->end->value) { /* We've found a less costly way to reach the end of s, update it */
					s->end->value=new;
					s->end->seg=s;
					if (! s->end->el) {
						if (debug_route)
							printf("insert_end p=%p el=%p val=%d ", s->end, s->end->el, s->end->value);
						s->end->el=fh_insertkey(heap, new, s->end);
						if (debug_route)
							printf("el new=%p\n", s->end->el);
					}
					else {
						if (debug_route)
							printf("replace_end p=%p el=%p val=%d\n", s->end, s->end->el, s->end->value);
						fh_replacekey(heap, s->end->el, new);
					}
				}
				if (debug_route)
					printf("\n");
			}
			s=s->start_next;
		}
		s=p_min->end;
		while (s) { /* Doing the same as above with the segments leading towards our point */
			val=route_value_seg(profile, p_min, s, 1);
			if (val != INT_MAX && !item_is_equal(s->data.item,p_min->seg->data.item)) {
				new=min+val;
				if (debug_route)
					printf("end %d len %d vs %d (0x%x,0x%x)\n",new,val,s->start->value,s->start->c.x, s->start->c.y);
				if (new < s->start->value) {
					old=s->start->value;
					s->start->value=new;
					s->start->seg=s;
					if (! s->start->el) {
						if (debug_route)
							printf("insert_start p=%p el=%p val=%d ", s->start, s->start->el, s->start->value);
						s->start->el=fh_insertkey(heap, new, s->start);
						if (debug_route)
							printf("el new=%p\n", s->start->el);
					}
					else {
						if (debug_route)
							printf("replace_start p=%p el=%p val=%d\n", s->start, s->start->el, s->start->value);
						fh_replacekey(heap, s->start->el, new);
					}
				}
				if (debug_route)
					printf("\n");
			}
			s=s->end_next;
		}
	}
	fh_deleteheap(heap);
	callback_call_0(cb);
	dbg(1,"return\n");
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
route_path_new_offroad(struct route_graph *this, struct route_info *pos, struct route_info *dst)
{
	struct route_path *ret;

	ret=g_new0(struct route_path, 1);
	ret->path_hash=item_hash_new();
	route_path_add_line(ret, &pos->c, &dst->c, pos->lenextra+dst->lenextra);
	ret->updated=1;

	return ret;
}

/**
 * @brief Returns a coordinate at a given distance
 *
 * This function returns the coordinate, where the user will be if he
 * follows the current route for a certain distance.
 *
 * @param this_ The route we're driving upon
 * @param dist The distance in meters
 * @return The coordinate where the user will be in that distance
 */
struct coord 
route_get_coord_dist(struct route *this_, int dist)
{
	int d,l,i,len;
	int dx,dy;
	double frac;
	struct route_path_segment *cur;
	struct coord ret;
	enum projection pro=route_projection(this_);
	struct route_info *dst=route_get_dst(this_);

	d = dist;

	if (!this_->path2 || pro == projection_none) {
		return this_->pos->c;
	}
	
	ret = this_->pos->c;
	cur = this_->path2->path;
	while (cur) {
		if (cur->data->len < d) {
			d -= cur->data->len;
		} else {
			for (i=0; i < (cur->ncoords-1); i++) {
				l = d;
				len = (int)transform_polyline_length(pro, (cur->c + i), 2);
				d -= len;
				if (d <= 0) { 
					// We interpolate a bit here...
					frac = (double)l / len;
					
					dx = (cur->c + i + 1)->x - (cur->c + i)->x;
					dy = (cur->c + i + 1)->y - (cur->c + i)->y;
					
					ret.x = (cur->c + i)->x + (frac * dx);
					ret.y = (cur->c + i)->y + (frac * dy);
					return ret;
				}
			}
			return cur->c[(cur->ncoords-1)];
		}
		cur = cur->next;
	}

	return dst->c;
}

/**
 * @brief Creates a new route path
 * 
 * This creates a new non-trivial route. It therefore needs the routing information created by route_graph_flood, so
 * make sure to run route_graph_flood() after changing the destination before using this function.
 *
 * @param this The route graph to create the route from
 * @param oldpath (Optional) old path which may contain parts of the new part - this speeds things up a bit. May be NULL.
 * @param pos The starting position of the route
 * @param dst The destination of the route
 * @param preferences The routing preferences
 * @return The new route path
 */
static struct route_path *
route_path_new(struct route_graph *this, struct route_path *oldpath, struct route_info *pos, struct route_info *dst, struct vehicleprofile *profile)
{
	struct route_graph_segment *first,*s=NULL,*s1=NULL,*s2=NULL;
	struct route_graph_point *start;
	struct route_info *posinfo, *dstinfo;
	int segs=0;
	int val1=INT_MAX,val2=INT_MAX;
	int val,val1_new,val2_new;
	struct route_path *ret;

	if (! pos->street || ! dst->street) {
		dbg(0,"pos or dest not set\n");
		return NULL;
	}

	if (profile->mode == 2 || (profile->mode == 0 && pos->lenextra + dst->lenextra > transform_distance(map_projection(pos->street->item.map), &pos->c, &dst->c)))
		return route_path_new_offroad(this, pos, dst);
	while ((s=route_graph_get_segment(this, pos->street, s))) {
		val=route_value_seg(profile, NULL, s, 1);
		if (val != INT_MAX && s->end->value != INT_MAX) {
			val=val*(100-pos->percent)/100;
			val1_new=s->end->value+val;
			if (val1_new < val1) {
				val1=val1_new;
				s1=s;
			}
		}
		val=route_value_seg(profile, NULL, s, -1);
		if (val != INT_MAX && s->start->value != INT_MAX) {
			val=val*pos->percent/100;
			val2_new=s->start->value+val;
			if (val2_new < val2) {
				val2=val2_new;
				s2=s;
			}
		}
	}
	if (val1 == INT_MAX && val2 == INT_MAX) {
		dbg(0,"no route found, pos blocked\n");
		return NULL;
	}
	if (val1 == val2) {
		val1=s1->end->value;
		val2=s2->start->value;
	}
	if (val1 < val2) {
		start=s1->start;
		s=s1;
	} else {
		start=s2->end;
		s=s2;
	}
	ret=g_new0(struct route_path, 1);
	ret->updated=1;
	if (pos->lenextra) 
		route_path_add_line(ret, &pos->c, &pos->lp, pos->lenextra);
	ret->path_hash=item_hash_new();
	dstinfo=NULL;
	posinfo=pos;
	first=s;
	while (s && !dstinfo) { /* following start->seg, which indicates the least costly way to reach our destination */
		segs++;
#if 0
		printf("start->value=%d 0x%x,0x%x\n", start->value, start->c.x, start->c.y);
#endif
		if (s->start == start) {		
			if (item_is_equal(s->data.item, dst->street->item) && (s->end->seg == s || !posinfo))
				dstinfo=dst;
			if (!route_path_add_item_from_graph(ret, oldpath, s, 1, posinfo, dstinfo))
				ret->updated=0;
			start=s->end;
		} else {
			if (item_is_equal(s->data.item, dst->street->item) && (s->start->seg == s || !posinfo))
				dstinfo=dst;
			if (!route_path_add_item_from_graph(ret, oldpath, s, -1, posinfo, dstinfo))
				ret->updated=0;
			start=s->start;
		}
		posinfo=NULL;
		s=start->seg;
	}
	if (dst->lenextra) 
		route_path_add_line(ret, &dst->lp, &dst->c, dst->lenextra);
	dbg(1, "%d segments\n", segs);
	return ret;
}

static int
route_graph_build_next_map(struct route_graph *rg)
{
	do {
		rg->m=mapset_next(rg->h, 2);
		if (! rg->m)
			return 0;
		map_rect_destroy(rg->mr);
		rg->mr=map_rect_new(rg->m, rg->sel);
	} while (!rg->mr);
		
	return 1;
}


static int
is_turn_allowed(struct route_graph_point *p, struct route_graph_segment *from, struct route_graph_segment *to)
{
	struct route_graph_point *prev,*next;
	struct route_graph_segment *tmp1,*tmp2;
	if (item_is_equal(from->data.item, to->data.item))
		return 0;
	if (from->start == p)
		prev=from->end;
	else
		prev=from->start;
	if (to->start == p)
		next=to->end;
	else
		next=to->start;
	tmp1=p->end;
	while (tmp1) {
		if (tmp1->start->c.x == prev->c.x && tmp1->start->c.y == prev->c.y &&
			(tmp1->data.item.type == type_street_turn_restriction_no ||
			tmp1->data.item.type == type_street_turn_restriction_only)) {
			tmp2=p->start;
			dbg(1,"found %s (0x%x,0x%x) (0x%x,0x%x)-(0x%x,0x%x) %p-%p\n",item_to_name(tmp1->data.item.type),tmp1->data.item.id_hi,tmp1->data.item.id_lo,tmp1->start->c.x,tmp1->start->c.y,tmp1->end->c.x,tmp1->end->c.y,tmp1->start,tmp1->end);
			while (tmp2) {
				dbg(1,"compare %s (0x%x,0x%x) (0x%x,0x%x)-(0x%x,0x%x) %p-%p\n",item_to_name(tmp2->data.item.type),tmp2->data.item.id_hi,tmp2->data.item.id_lo,tmp2->start->c.x,tmp2->start->c.y,tmp2->end->c.x,tmp2->end->c.y,tmp2->start,tmp2->end);
				if (item_is_equal(tmp1->data.item, tmp2->data.item)) 
					break;
				tmp2=tmp2->start_next;
			}
			dbg(1,"tmp2=%p\n",tmp2);
			if (tmp2) {
				dbg(1,"%s tmp2->end=%p next=%p\n",item_to_name(tmp1->data.item.type),tmp2->end,next);
			}
			if (tmp1->data.item.type == type_street_turn_restriction_no && tmp2 && tmp2->end->c.x == next->c.x && tmp2->end->c.y == next->c.y) {
				dbg(1,"from 0x%x,0x%x over 0x%x,0x%x to 0x%x,0x%x not allowed (no)\n",prev->c.x,prev->c.y,p->c.x,p->c.y,next->c.x,next->c.y);
				return 0;
			}
			if (tmp1->data.item.type == type_street_turn_restriction_only && tmp2 && (tmp2->end->c.x != next->c.x || tmp2->end->c.y != next->c.y)) {
				dbg(1,"from 0x%x,0x%x over 0x%x,0x%x to 0x%x,0x%x not allowed (only)\n",prev->c.x,prev->c.y,p->c.x,p->c.y,next->c.x,next->c.y);
				return 0;
			}
		}
		tmp1=tmp1->end_next;
	}
	dbg(1,"from 0x%x,0x%x over 0x%x,0x%x to 0x%x,0x%x allowed\n",prev->c.x,prev->c.y,p->c.x,p->c.y,next->c.x,next->c.y);
	return 1;
}

static void
route_graph_clone_segment(struct route_graph *this, struct route_graph_segment *s, struct route_graph_point *start, struct route_graph_point *end, int flags)
{
	struct route_graph_segment_data data;
	data.flags=s->data.flags|flags;
	data.offset=1;
	data.maxspeed=-1;
	data.item=&s->data.item;
	data.len=s->data.len+1;
	if (s->data.flags & AF_SPEED_LIMIT)
		data.maxspeed=RSD_MAXSPEED(&s->data);
	if (s->data.flags & AF_SEGMENTED) 
		data.offset=RSD_OFFSET(&s->data);
	dbg(1,"cloning segment from %p (0x%x,0x%x) to %p (0x%x,0x%x)\n",start,start->c.x,start->c.y, end, end->c.x, end->c.y);
	route_graph_add_segment(this, start, end, &data);
}

static void
route_graph_process_restriction_segment(struct route_graph *this, struct route_graph_point *p, struct route_graph_segment *s, int dir)
{
	struct route_graph_segment *tmp;
	struct route_graph_point *pn;
	struct coord c=p->c;
	int dx=0;
	int dy=0;
	c.x+=dx;
	c.y+=dy;
	dbg(1,"From %s %d,%d\n",item_to_name(s->data.item.type),dx,dy);
	pn=route_graph_point_new(this, &c);
	if (dir > 0) { /* going away */
		dbg(1,"other 0x%x,0x%x\n",s->end->c.x,s->end->c.y);
		if (s->data.flags & AF_ONEWAY) {
			dbg(1,"Not possible\n");
			return;
		}
		route_graph_clone_segment(this, s, pn, s->end, AF_ONEWAYREV);
	} else { /* coming in */
		dbg(1,"other 0x%x,0x%x\n",s->start->c.x,s->start->c.y);
		if (s->data.flags & AF_ONEWAYREV) {
			dbg(1,"Not possible\n");
			return;
		}
		route_graph_clone_segment(this, s, s->start, pn, AF_ONEWAY);
	}
	tmp=p->start;
	while (tmp) {
		if (tmp != s && tmp->data.item.type != type_street_turn_restriction_no &&
			tmp->data.item.type != type_street_turn_restriction_only &&
			!(tmp->data.flags & AF_ONEWAYREV) && is_turn_allowed(p, s, tmp)) {
			route_graph_clone_segment(this, tmp, pn, tmp->end, AF_ONEWAY);
			dbg(1,"To start %s\n",item_to_name(tmp->data.item.type));
		}
		tmp=tmp->start_next;
	}
	tmp=p->end;
	while (tmp) {
		if (tmp != s && tmp->data.item.type != type_street_turn_restriction_no &&
			tmp->data.item.type != type_street_turn_restriction_only &&
			!(tmp->data.flags & AF_ONEWAY) && is_turn_allowed(p, s, tmp)) {
			route_graph_clone_segment(this, tmp, tmp->start, pn, AF_ONEWAYREV);
			dbg(1,"To end %s\n",item_to_name(tmp->data.item.type));
		}
		tmp=tmp->end_next;
	}
}

static void
route_graph_process_restriction_point(struct route_graph *this, struct route_graph_point *p)
{
	struct route_graph_segment *tmp;
	tmp=p->start;
	dbg(1,"node 0x%x,0x%x\n",p->c.x,p->c.y);
	while (tmp) {
		if (tmp->data.item.type != type_street_turn_restriction_no &&
			tmp->data.item.type != type_street_turn_restriction_only)
			route_graph_process_restriction_segment(this, p, tmp, 1);
		tmp=tmp->start_next;
	}
	tmp=p->end;
	while (tmp) {
		if (tmp->data.item.type != type_street_turn_restriction_no &&
			tmp->data.item.type != type_street_turn_restriction_only)
			route_graph_process_restriction_segment(this, p, tmp, -1);
		tmp=tmp->end_next;
	}
	p->flags |= RP_TURN_RESTRICTION_RESOLVED;
}

static void
route_graph_process_restrictions(struct route_graph *this)
{
	struct route_graph_point *curr;
	int i;
	dbg(1,"enter\n");
	for (i = 0 ; i < HASH_SIZE ; i++) {
		curr=this->hash[i];
		while (curr) {
			if (curr->flags & RP_TURN_RESTRICTION) 
				route_graph_process_restriction_point(this, curr);
			curr=curr->hash_next;
		}
	}
}

static void
route_graph_build_done(struct route_graph *rg, int cancel)
{
	dbg(1,"cancel=%d\n",cancel);
	if (rg->idle_ev)
		event_remove_idle(rg->idle_ev);
	if (rg->idle_cb)
		callback_destroy(rg->idle_cb);
	map_rect_destroy(rg->mr);
        mapset_close(rg->h);
	route_free_selection(rg->sel);
	rg->idle_ev=NULL;
	rg->idle_cb=NULL;
	rg->mr=NULL;
	rg->h=NULL;
	rg->sel=NULL;
	if (! cancel) {
		route_graph_process_restrictions(rg);
		callback_call_0(rg->done_cb);
	}
	rg->busy=0;
}

static void
route_graph_build_idle(struct route_graph *rg, struct vehicleprofile *profile)
{
	int count=1000;
	struct item *item;

	while (count > 0) {
		for (;;) {	
			item=map_rect_get_item(rg->mr);
			if (item)
				break;
			if (!route_graph_build_next_map(rg)) {
				route_graph_build_done(rg, 0);
				return;
			}
		}
		if (item->type == type_traffic_distortion)
			route_process_traffic_distortion(rg, item);
		else if (item->type == type_street_turn_restriction_no || item->type == type_street_turn_restriction_only)
			route_process_turn_restriction(rg, item);
		else
			route_process_street_graph(rg, item, profile);
		count--;
	}
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
 * @param done_cb The callback which will be called when graph is complete
 * @return The new route graph.
 */
static struct route_graph *
route_graph_build(struct mapset *ms, struct coord *c, int count, struct callback *done_cb, int async, struct vehicleprofile *profile)
{
	struct route_graph *ret=g_new0(struct route_graph, 1);

	dbg(1,"enter\n");

	ret->sel=route_calc_selection(c, count);
	ret->h=mapset_open(ms);
	ret->done_cb=done_cb;
	ret->busy=1;
	if (route_graph_build_next_map(ret)) {
		if (async) {
			ret->idle_cb=callback_new_2(callback_cast(route_graph_build_idle), ret, profile);
			ret->idle_ev=event_add_idle(50, ret->idle_cb);
		}
	} else
		route_graph_build_done(ret, 0);

	return ret;
}

static void
route_graph_update_done(struct route *this, struct callback *cb)
{
	route_graph_flood(this->graph, this->current_dst, this->vehicleprofile, cb);
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
route_graph_update(struct route *this, struct callback *cb, int async)
{
	struct attr route_status;
	struct coord *c=g_alloca(sizeof(struct coord)*(1+g_list_length(this->destinations)));
	int i=0;
	GList *tmp;

	route_status.type=attr_route_status;
	route_graph_destroy(this->graph);
	this->graph=NULL;
	callback_destroy(this->route_graph_done_cb);
	this->route_graph_done_cb=callback_new_2(callback_cast(route_graph_update_done), this, cb);
	route_status.u.num=route_status_building_graph;
	route_set_attr(this, &route_status);
	c[i++]=this->pos->c;
	tmp=this->destinations;
	while (tmp) {
		struct route_info *dst=tmp->data;
		c[i++]=dst->c;
		tmp=g_list_next(tmp);
	}
	this->graph=route_graph_build(this->ms, c, i, this->route_graph_done_cb, async, this->vehicleprofile);
	if (! async) {
		while (this->graph->busy) 
			route_graph_build_idle(this->graph, this->vehicleprofile);
	}
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
	int count=0,*flags;
	struct street_data *ret = NULL, *ret1;
	struct attr flags_attr, maxspeed_attr;
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

	ret1=g_realloc(ret, sizeof(struct street_data)+count*sizeof(struct coord));
	if (ret1)
		ret = ret1;
	ret->item=*item;
	ret->count=count;
	if (item_attr_get(item, attr_flags, &flags_attr)) 
		ret->flags=flags_attr.u.num;
	else {
		flags=item_get_default_flags(item->type);
		if (flags)
			ret->flags=*flags;
		else
			ret->flags=0;
	}

	ret->maxspeed = -1;
	if (ret->flags & AF_SPEED_LIMIT) {
		if (item_attr_get(item, attr_maxspeed, &maxspeed_attr)) {
			ret->maxspeed = maxspeed_attr.u.num;
		}
	}

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
route_find_nearest_street(struct vehicleprofile *vehicleprofile, struct mapset *ms, struct pcoord *pc)
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

	ret=g_new0(struct route_info, 1);
	mindist = INT_MAX;

	h=mapset_open(ms);
	while ((m=mapset_next(h,2))) {
		c.x = pc->x;
		c.y = pc->y;
		if (map_projection(m) != pc->pro) {
			transform_to_geo(pc->pro, &c, &g);
			transform_from_geo(map_projection(m), &g, &c);
		}
		sel = route_rect(18, &c, &c, 0, max_dist);
		if (!sel)
			continue;
		mr=map_rect_new(m, sel);
		if (!mr) {
			map_selection_destroy(sel);
			continue;
		}
		while ((item=map_rect_get_item(mr))) {
			if (item_get_default_flags(item->type)) {
				sd=street_get_data(item);
				if (!sd)
					continue;
				dist=transform_distance_polyline_sq(sd->c, sd->count, &c, &lp, &pos);
				if (dist < mindist && (
					(sd->flags & vehicleprofile->flags_forward_mask) == vehicleprofile->flags ||
					(sd->flags & vehicleprofile->flags_reverse_mask) == vehicleprofile->flags)) {
					mindist = dist;
					if (ret->street) {
						street_data_free(ret->street);
					}
					ret->c=c;
					ret->lp=lp;
					ret->pos=pos;
					ret->street=sd;
					dbg(1,"dist=%d id 0x%x 0x%x pos=%d\n", dist, item->id_hi, item->id_lo, pos);
				} else {
					street_data_free(sd);
				}
			}
		}
		map_selection_destroy(sel);
		map_rect_destroy(mr);
	}
	mapset_close(h);

	if (!ret->street || mindist > max_dist*max_dist) {
		if (ret->street) {
			street_data_free(ret->street);
			dbg(1,"Much too far %d > %d\n", mindist, max_dist);
		}
		g_free(ret);
		ret = NULL;
	}

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
	if (!inf)
		return;
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
	unsigned int last_coord;
	struct route_path *path;
	struct route_path_segment *seg,*seg_next;
	struct route_graph_point *point;
	struct route_graph_segment *rseg;
	char *str;
	int hash_bucket;
	struct coord *coord_sel;	/**< Set this to a coordinate if you want to filter for just a single route graph point */
	struct route_graph_point_iterator it;
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
	if (mr->item.type != type_street_route)
		return 0;
	attr->type=attr_type;
	switch (attr_type) {
		case attr_any:
			while (mr->attr_next != attr_none) {
				if (rm_attr_get(priv_data, mr->attr_next, attr))
					return 1;
			}
			return 0;
		case attr_maxspeed:
			mr->attr_next = attr_street_item;
			if (seg && seg->data->flags & AF_SPEED_LIMIT) {
				attr->u.num=RSD_MAXSPEED(seg->data);

			} else {
				return 0;
			}
			return 1;
		case attr_street_item:
			mr->attr_next=attr_direction;
			if (seg && seg->data->item.map)
				attr->u.item=&seg->data->item;
			else
				return 0;
			return 1;
		case attr_direction:
			mr->attr_next=attr_route;
			if (seg) 
				attr->u.num=seg->direction;
			else
				return 0;
			return 1;
		case attr_route:
			mr->attr_next=attr_length;
			attr->u.route = mr->mpriv->route;
			return 1;
		case attr_length:
			mr->attr_next=attr_time;
			if (seg)
				attr->u.num=seg->data->len;
			else
				return 0;
			return 1;
		case attr_time:
			mr->attr_next=attr_none;
			if (seg)
				attr->u.num=route_time_seg(route->vehicleprofile, seg->data, NULL);
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

	if (pro == projection_none)
		return 0;
	if (mr->item.type == type_route_start || mr->item.type == type_route_start_reverse || mr->item.type == type_route_end) {
		if (! count || mr->last_coord)
			return 0;
		mr->last_coord=1;
		if (mr->item.type == type_route_start || mr->item.type == type_route_start_reverse)
			c[0]=r->pos->c;
		else {
			c[0]=route_get_dst(r)->c;
		}	
		return 1;
	}
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
	struct route_graph_segment *seg = mr->rseg;
	struct route *route=mr->mpriv->route;

	attr->type=attr_type;
	switch (attr_type) {
	case attr_any: // works only with rg_points for now
		while (mr->attr_next != attr_none) {
			dbg(0,"querying %s\n", attr_to_name(mr->attr_next));
			if (rp_attr_get(priv_data, mr->attr_next, attr))
				return 1;
		}
		return 0;
	case attr_maxspeed:
		mr->attr_next = attr_label;
		if (mr->item.type != type_rg_segment) 
			return 0;
		if (seg && (seg->data.flags & AF_SPEED_LIMIT)) {
			attr->type = attr_maxspeed;
			attr->u.num=RSD_MAXSPEED(&seg->data);
			return 1;
		} else {
			return 0;
		}
	case attr_label:
		mr->attr_next=attr_street_item;
		if (mr->item.type != type_rg_point) 
			return 0;
		attr->type = attr_label;
		if (mr->str)
			g_free(mr->str);
		if (p->value != INT_MAX)
			mr->str=g_strdup_printf("%d", p->value);
		else
			mr->str=g_strdup("-");
		attr->u.str = mr->str;
		return 1;
	case attr_street_item:
		mr->attr_next=attr_flags;
		if (mr->item.type != type_rg_segment) 
			return 0;
		if (seg && seg->data.item.map)
			attr->u.item=&seg->data.item;
		else
			return 0;
		return 1;
	case attr_flags:
		mr->attr_next = attr_direction;
		if (mr->item.type != type_rg_segment)
			return 0;
		if (seg) {
			attr->u.num = seg->data.flags;
		} else {
			return 0;
		}
		return 1;
	case attr_direction:
		mr->attr_next = attr_debug;
		// This only works if the map has been opened at a single point, and in that case indicates if the
		// segment returned last is connected to this point via its start (1) or its end (-1)
		if (!mr->coord_sel || (mr->item.type != type_rg_segment))
			return 0;
		if (seg->start == mr->point) {
			attr->u.num=1;
		} else if (seg->end == mr->point) {
			attr->u.num=-1;
		} else {
			return 0;
		}
		return 1;
	case attr_debug:
		mr->attr_next=attr_none;
		if (mr->str)
			g_free(mr->str);
		switch (mr->item.type) {
		case type_rg_point:
		{
			struct route_graph_segment *tmp;
			int start=0;
			int end=0;
			tmp=p->start;
			while (tmp) {
				start++;
				tmp=tmp->start_next;
			}
			tmp=p->end;
			while (tmp) {
				end++;
				tmp=tmp->end_next;
			}
			mr->str=g_strdup_printf("%d %d %p (0x%x,0x%x)", start, end, p, p->c.x, p->c.y);
			attr->u.str = mr->str;
		}
			return 1;
		case type_rg_segment:
			if (! seg)
				return 0;
			mr->str=g_strdup_printf("len %d time %d start %p end %p",seg->data.len, route_time_seg(route->vehicleprofile, &seg->data, NULL), seg->start, seg->end);
			attr->u.str = mr->str;
			return 1;
		default:
			return 0;
		}
	default:
		mr->attr_next=attr_none;
		attr->type=attr_none;
		return 0;
	}
}

/**
 * @brief Returns the coordinates of a route graph item
 *
 * @param priv_data The route graph item's private data
 * @param c Pointer where to store the coordinates
 * @param count How many coordinates to get at a max?
 * @return The number of coordinates retrieved
 */
static int
rp_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *mr = priv_data;
	struct route_graph_point *p = mr->point;
	struct route_graph_segment *seg = mr->rseg;
	int rc = 0,i,dir;
	struct route *r = mr->mpriv->route;
	enum projection pro = route_projection(r);

	if (pro == projection_none)
		return 0;
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
rp_destroy(struct map_priv *priv)
{
	g_free(priv);
}

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
#if 0
	if (! route_get_pos(priv->route))
		return NULL;
	if (! route_get_dst(priv->route))
		return NULL;
#endif
#if 0
	if (! priv->route->path2)
		return NULL;
#endif
	mr=g_new0(struct map_rect_priv, 1);
	mr->mpriv = priv;
	mr->item.priv_data = mr;
	mr->item.type = type_none;
	mr->item.meth = &methods_route_item;
	if (priv->route->path2) {
		mr->path=priv->route->path2;
		mr->seg_next=mr->path->path;
		mr->path->in_use++;
	} else
		mr->seg_next=NULL;
	return mr;
}

/**
 * @brief Opens a new map rectangle on the route graph's map
 *
 * This function opens a new map rectangle on the route graph's map.
 * The "sel" parameter enables you to only search for a single route graph
 * point on this map (or exactly: open a map rectangle that only contains
 * this one point). To do this, pass here a single map selection, whose 
 * c_rect has both coordinates set to the same point. Otherwise this parameter
 * has no effect.
 *
 * @param priv The route graph map's private data
 * @param sel Here it's possible to specify a point for which to search. Please read the function's description.
 * @return A new map rect's private data
 */
static struct map_rect_priv * 
rp_rect_new(struct map_priv *priv, struct map_selection *sel)
{
	struct map_rect_priv * mr;

	dbg(1,"enter\n");
	if (! priv->route->graph)
		return NULL;
	mr=g_new0(struct map_rect_priv, 1);
	mr->mpriv = priv;
	mr->item.priv_data = mr;
	mr->item.type = type_rg_point;
	mr->item.meth = &methods_point_item;
	if (sel) {
		if ((sel->u.c_rect.lu.x == sel->u.c_rect.rl.x) && (sel->u.c_rect.lu.y == sel->u.c_rect.rl.y)) {
			mr->coord_sel = g_malloc(sizeof(struct coord));
			*(mr->coord_sel) = sel->u.c_rect.lu;
		}
	}
	return mr;
}

static void
rm_rect_destroy(struct map_rect_priv *mr)
{
	if (mr->str)
		g_free(mr->str);
	if (mr->coord_sel) {
		g_free(mr->coord_sel);
	}
	if (mr->path) {
		mr->path->in_use--;
		if (mr->path->update_required && !mr->path->in_use) 
			route_path_update_done(mr->mpriv->route, mr->path->update_required-1);
	}

	g_free(mr);
}

static struct item *
rp_get_item(struct map_rect_priv *mr)
{
	struct route *r = mr->mpriv->route;
	struct route_graph_point *p = mr->point;
	struct route_graph_segment *seg = mr->rseg;

	if (mr->item.type == type_rg_point) {
		if (mr->coord_sel) {
			// We are supposed to return only the point at one specified coordinate...
			if (!p) {
				p = route_graph_get_point_last(r->graph, mr->coord_sel);
				if (!p) {
					mr->point = NULL; // This indicates that no point has been found
				} else {
					mr->it = rp_iterator_new(p);
				}
			} else {
				p = NULL;
			}
		} else {
			if (!p) {
				mr->hash_bucket=0;
				p = r->graph->hash[0];
			} else 
				p=p->hash_next;
			while (!p) {
				mr->hash_bucket++;
				if (mr->hash_bucket >= HASH_SIZE)
					break;
				p = r->graph->hash[mr->hash_bucket];
			}
		}
		if (p) {
			mr->point = p;
			mr->item.id_lo++;
			rm_coord_rewind(mr);
			rp_attr_rewind(mr);
			return &mr->item;
		} else
			mr->item.type = type_rg_segment;
	}
	
	if (mr->coord_sel) {
		if (!mr->point) { // This means that no point has been found
			return NULL;
		}
		seg = rp_iterator_next(&(mr->it));
	} else {
		if (!seg)
			seg=r->graph->route_segments;
		else
			seg=seg->next;
	}
	
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
	struct route *route=mr->mpriv->route;
	dbg(1,"enter\n", mr->pos);

	switch (mr->item.type) {
	case type_none:
		if (route->pos && route->pos->street_direction && route->pos->street_direction != route->pos->dir)
			mr->item.type=type_route_start_reverse;
		else
			mr->item.type=type_route_start;
		if (route->pos) 
			break;
	default:
		mr->item.type=type_street_route;
		mr->seg=mr->seg_next;
		if (!mr->seg && mr->path && mr->path->next) {
			mr->path->in_use--;
			mr->path=mr->path->next;
			mr->path->in_use++;
			mr->seg=mr->path->path;
		}
		if (mr->seg) {
			mr->seg_next=mr->seg->next;
			break;
		}
		mr->item.type=type_route_end;
		if (mr->mpriv->route->destinations)
			break;
	case type_route_end:
		return NULL;
	}
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
	rp_destroy,
	rp_rect_new,
	rm_rect_destroy,
	rp_get_item,
	rp_get_item_byid,
	NULL,
	NULL,
	NULL,
};

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
route_map_new(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	return route_map_new_helper(meth, attrs, 0);
}

static struct map_priv *
route_graph_map_new(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	return route_map_new_helper(meth, attrs, 1);
}

static struct map *
route_get_map_helper(struct route *this_, struct map **map, char *type, char *description)
{
	struct attr *attrs[5];
	struct attr a_type,navigation,data,a_description;
	a_type.type=attr_type;
	a_type.u.str=type;
	navigation.type=attr_route;
	navigation.u.route=this_;
	data.type=attr_data;
	data.u.str="";
	a_description.type=attr_description;
	a_description.u.str=description;

	attrs[0]=&a_type;
	attrs[1]=&navigation;
	attrs[2]=&data;
	attrs[3]=&a_description;
	attrs[4]=NULL;

	if (! *map) 
		*map=map_new(NULL, attrs);
 
	return *map;
}

/**
 * @brief Returns a new map containing the route path
 *
 * This function returns a new map containing the route path.
 *
 * @important Do not map_destroy() this!
 *
 * @param this_ The route to get the map of
 * @return A new map containing the route path
 */
struct map *
route_get_map(struct route *this_)
{
	return route_get_map_helper(this_, &this_->map, "route","Route");
}


/**
 * @brief Returns a new map containing the route graph
 *
 * This function returns a new map containing the route graph.
 *
 * @important Do not map_destroy()  this!
 *
 * @param this_ The route to get the map of
 * @return A new map containing the route graph
 */
struct map *
route_get_graph_map(struct route *this_)
{
	return route_get_map_helper(this_, &this_->graph_map, "route_graph","Route Graph");
}

void
route_set_projection(struct route *this_, enum projection pro)
{
}

int
route_set_attr(struct route *this_, struct attr *attr)
{
	int attr_updated=0;
	switch (attr->type) {
	case attr_route_status:
		attr_updated = (this_->route_status != attr->u.num);
		this_->route_status = attr->u.num;
		break;
	case attr_destination:
		route_set_destination(this_, attr->u.pcoord, 1);
		return 1;
	case attr_vehicle:
		attr_updated = (this_->v != attr->u.vehicle);
		this_->v=attr->u.vehicle;
		if (attr_updated) {
			struct attr g;
			struct pcoord pc;
			struct coord c;
			if (vehicle_get_attr(this_->v, attr_position_coord_geo, &g, NULL)) {
				pc.pro=projection_mg;
				transform_from_geo(projection_mg, g.u.coord_geo, &c);
				pc.x=c.x;
				pc.y=c.y;
				route_set_position(this_, &pc);
			}
		}
		break;
	default:
		dbg(0,"unsupported attribute: %s\n",attr_to_name(attr->type));
		return 0;
	}
	if (attr_updated)
		callback_list_call_attr_2(this_->cbl2, attr->type, this_, attr);
	return 1;
}

int
route_add_attr(struct route *this_, struct attr *attr)
{
	switch (attr->type) {
	case attr_callback:
		callback_list_add(this_->cbl2, attr->u.callback);
		return 1;
	default:
		return 0;
	}
}

int
route_remove_attr(struct route *this_, struct attr *attr)
{
	dbg(0,"enter\n");
	switch (attr->type) {
	case attr_callback:
		callback_list_remove(this_->cbl2, attr->u.callback);
		return 1;
	case attr_vehicle:
		this_->v=NULL;
		return 1;
	default:
		return 0;
	}
}

int
route_get_attr(struct route *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	int ret=1;
	switch (type) {
	case attr_map:
		attr->u.map=route_get_map(this_);
		ret=(attr->u.map != NULL);
		break;
	case attr_destination:
		if (this_->destinations) {
			struct route_info *dst;
			if (iter) {
				if (iter->u.list) {
					iter->u.list=g_list_next(iter->u.list);
				} else {
					iter->u.list=this_->destinations;
				}
				if (!iter->u.list) {
					return 0;
				}
				dst = (struct route_info*)iter->u.list->data;				
			} else { //No iter handling
				dst=route_get_dst(this_);
			}
			attr->u.pcoord=&this_->pc;
			this_->pc.pro=projection_mg; /* fixme */
			this_->pc.x=dst->c.x;
			this_->pc.y=dst->c.y;
		} else
			ret=0;
		break;
	case attr_vehicle:
		attr->u.vehicle=this_->v;
		ret=(this_->v != NULL);
		dbg(0,"get vehicle %p\n",this_->v);
		break;
	case attr_vehicleprofile:
		attr->u.vehicleprofile=this_->vehicleprofile;
		ret=(this_->vehicleprofile != NULL);
		break;
	case attr_route_status:
		attr->u.num=this_->route_status;
		break;
	case attr_destination_time:
		if (this_->path2 && (this_->route_status == route_status_path_done_new || this_->route_status == route_status_path_done_incremental)) {

			attr->u.num=this_->path2->path_time;
			dbg(1,"path_time %d\n",attr->u.num);
		}
		else
			ret=0;
		break;
	case attr_destination_length:
		if (this_->path2 && (this_->route_status == route_status_path_done_new || this_->route_status == route_status_path_done_incremental))
			attr->u.num=this_->path2->path_len;
		else
			ret=0;
		break;
	default:
		return 0;
	}
	attr->type=type;
	return ret;
}

struct attr_iter *
route_attr_iter_new(void)
{
	return g_new0(struct attr_iter, 1);
}

void
route_attr_iter_destroy(struct attr_iter *iter)
{
	g_free(iter);
}

void
route_init(void)
{
	plugin_register_map_type("route", route_map_new);
	plugin_register_map_type("route_graph", route_graph_map_new);
}

void
route_destroy(struct route *this_)
{
	route_path_destroy(this_->path2,1);
	route_graph_destroy(this_->graph);
	route_clear_destinations(this_);
	route_info_free(this_->pos);
	map_destroy(this_->map);
	map_destroy(this_->graph_map);
	g_free(this_);
}
