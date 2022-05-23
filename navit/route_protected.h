/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

/**
 * @file route_private.h
 *
 * @brief Contains protected exports for route.c
 *
 * This file contains code that is exported by route.c. Unlike the exports in route.h, exports in this
 * file are intended only for use by modules which are closely related to routing. They are not part of
 * the public route API.
 */

#ifndef NAVIT_ROUTE_PROTECTED_H
#define NAVIT_ROUTE_PROTECTED_H

#ifdef __cplusplus
extern "C" {
#endif


#define RP_TRAFFIC_DISTORTION 1
#define RP_TURN_RESTRICTION 2
#define RP_TURN_RESTRICTION_RESOLVED 4

#define RSD_MAXSPEED(x) *((int *)route_segment_data_field_pos((x), attr_maxspeed))

/**
 * @brief A point in the route graph
 *
 * This represents a point in the route graph. A point usually connects two or more segments,
 * but there are also points which don't do that (e.g. at the end of a dead-end).
 */
struct route_graph_point {
	struct route_graph_point *hash_next; /**< Pointer to a chained hashlist of all route_graph_points with this hash */
	struct route_graph_segment *start;   /**< Pointer to a list of segments of which this point is the start. The links
	                                      *  of this linked-list are in route_graph_segment->start_next.*/
	struct route_graph_segment *end;     /**< Pointer to a list of segments of which this pointer is the end. The links
	                                      *  of this linked-list are in route_graph_segment->end_next. */
	struct route_graph_segment *seg;     /**< Pointer to the segment one should use to reach the destination at
	                                      *  least costs */
	struct fibheap_el *el;				 /**< When this point is put on a Fibonacci heap, this is a pointer
	                                      *  to this point's heap element */
	int value;                           /**< The cost at which one can reach the destination from this point on.
	                                      *  {@code INT_MAX} indicates that the destination is unreachable from this
	                                      *  point, or that this point has not yet been examined. */
	int rhs;                             /**< Lookahead value based on neighbors’ `value`; used for recalculation and
	                                      *   equal to `value` after the route graph has been flooded. */
	int dst_val;                         /**< For points close to the destination, this is the cost of the point if it
	                                      *   is the last in the graph; `INT_MAX` for all other points. */
    struct route_graph_segment *dst_seg; /**< For points close to the destination, this is the segment over which the
                                          * destination can be reached directly */
	struct coord c;                      /**< Coordinates of this point */
	int flags;                           /**< Flags for this point (e.g. traffic distortion) */
};

/**
 * @brief A segment in the route graph or path
 *
 * This is a segment in the route graph or path. A segment represents a driveable way.
 */
struct route_segment_data {
	struct item item;                    /**< The item (e.g. street) that this segment represents. */
	int flags;                           /**< Flags e.g. for access, restrictions, segmentation or roundabouts. */
	int len;                             /**< Length of this segment, in meters */
	int score;                           /**< Used by the traffic module to give preference to some
	                                      *   segments over others */
	/*NOTE: After a segment, various fields may follow, depending on what flags are set. Order of fields:
				1.) maxspeed			Maximum allowed speed on this segment. Present if AF_SPEED_LIMIT is set.
				2.) offset				If the item is segmented (i.e. represented by more than one segment), this
										indicates the position of this segment in the item. Present if AF_SEGMENTED is set.
	 */
};

/**
 * @brief Size and weight limits for a route segment
 */
struct size_weight_limit {
	int width;
	int length;
	int height;
	int weight;
	int axle_weight;
};

/**
 * @brief Data for a segment in the route graph
 */
struct route_graph_segment_data {
	struct item *item;                    /**< The item which this segment is part of */
	int offset;                           /**< If the item passed in "item" is segmented (i.e. divided
	                                       *   into several segments), this indicates the position of
	                                       *   this segment within the item */
	int flags;                            /**< Flags for this segment */
	int len;                              /**< The length of this segment */
	int maxspeed;                         /**< The maximum speed allowed on this segment in km/h,
	                                       *   -1 if not known */
	struct size_weight_limit size_weight; /**< Size and weight limits for this segment */
	int dangerous_goods;
	int score;                            /**< Used by the traffic module to give preference to some
	                                       *   segments over others */
};

/**
 * @brief A segment in the route graph
 *
 * This is a segment in the route graph. A segment represents a driveable way.
 */
struct route_graph_segment {
	struct route_graph_segment *next;		/**< Linked-list pointer to a list of all route_graph_segments */
	struct route_graph_segment *start_next;	/**< Pointer to the next element in the list of segments that start at the
	                                         *  same point. Start of this list is in route_graph_point->start. */
	struct route_graph_segment *end_next;	/**< Pointer to the next element in the list of segments that end at the
	                                         *  same point. Start of this list is in route_graph_point->end. */
	struct route_graph_point *start;		/**< Pointer to the point this segment starts at. */
	struct route_graph_point *end;			/**< Pointer to the point this segment ends at. */
	struct route_segment_data data;			/**< The segment data */
};

/**
 * @brief A complete route graph
 *
 * The route graph holds all routable segments along with the connections between them and the cost of
 * each segment.
 */
struct route_graph {
	int busy;                                   /**< Route calculation is in progress: the graph is being built,
	                                             *   flooded or the path is being built (a more detailed status can be
	                                             *   obtained from the route’s status attribute) */
	struct map_selection *sel;                  /**< The rectangle selection for the graph */
	struct mapset_handle *h;                    /**< Handle to the mapset */
	struct map *m;                              /**< Pointer to the currently active map */
	struct map_rect *mr;                        /**< Pointer to the currently active map rectangle */
	struct vehicleprofile *vehicleprofile;      /**< The vehicle profile */
	struct callback *idle_cb;                   /**< Idle callback to process the graph */
	struct callback *done_cb;                   /**< Callback when graph is done */
	struct event_idle *idle_ev;                 /**< The pointer to the idle event */
	struct route_graph_segment *route_segments; /**< Pointer to the first route_graph_segment in the linked list of all segments */
	struct route_graph_segment *avoid_seg;      /**< Segment to which a turnaround penalty (if active) applies */
	struct fibheap *heap;                       /**< Priority queue for points to be expanded */
#define HASH_SIZE 8192
	struct route_graph_point *hash[HASH_SIZE];  /**< A hashtable containing all route_graph_points in this graph */
};


/* prototypes */
struct route_graph * route_get_graph(struct route *this_);
struct map_selection * route_get_selection(struct route * this_);
void route_free_selection(struct map_selection *sel);
void route_add_traffic_distortion(struct route *this_, struct item *item);
void route_remove_traffic_distortion(struct route *this_, struct item *item);
void route_change_traffic_distortion(struct route *this_, struct item *item);
struct route_graph_point * route_graph_add_point(struct route_graph *this, struct coord *f);
void route_graph_add_turn_restriction(struct route_graph *this, struct item *item);
void route_graph_free_points(struct route_graph *this);
struct route_graph_point *route_graph_get_point(struct route_graph *this, struct coord *c);
struct route_graph_point *route_graph_get_point_next(struct route_graph *this, struct coord *c,
        struct route_graph_point *last);
void route_graph_add_segment(struct route_graph *this, struct route_graph_point *start,
		struct route_graph_point *end, struct route_graph_segment_data *data);
int route_graph_segment_is_duplicate(struct route_graph_point *start, struct route_graph_segment_data *data);
void route_graph_free_segments(struct route_graph *this);
void route_graph_build_done(struct route_graph *rg, int cancel);
void route_recalculate_partial(struct route *this_);
void * route_segment_data_field_pos(struct route_segment_data *seg, enum attr_type type);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif

