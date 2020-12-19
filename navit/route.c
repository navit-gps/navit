/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2018 Navit Team
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
 * Routing now relies on the Lifelong Planning A* (LPA*) algorithm, which builds upon the A* algorithm but allows for
 * partial updates after the cost of some segments has changed. (With A*, one would need to recalculate the entire
 * route graph from scratch.) A*, in turn, is an extension of the Dijkstra algorithm, with the added improvement that
 * A* introduces a heuristic (essentially, a lower boundary for the yet-to-be-calculated remainder of the route from a
 * given point onwards) and chooses the next point to analyze based on the sum of its cost and its heuristic, where
 * Dijkstra uses simply the cost of the node. This makes A* more efficient than Dijkstra in some scenarios. (Navit,
 * however, is not one of them, as we currently analyze only a subset of the entire map, and calculating the heuristic
 * for each node turned out to cost more than it saved in tests.)
 *
 * Wikipedia has articles on all three algorithms; refer to these for an in-depth discussion of the algorithms.
 *
 * If the heuristic is assumed to be zero in all cases, A* behaves exactly as Dijkstra would. Similarly, LPA* behaves
 * identically to A* if all segment costs are known prior to route calculation and do not change once route calculation
 * has started.
 *
 * Earlier versions of Navit used Dijkstra for routing. This was upgraded to LPA* when the traffic module was
 * introduced, as it became necessary to do fast partial recalculations of the route when the traffic situation
 * changes. Navit’s LPA* implementation differs from the canonical implementation in two important ways:
 *
 * \li The heuristic is not used (or assumed to be zero), for the reasons discussed above. However, this is not set in
 * stone and can be revisited if we find using a heuristic provides a true benefit.
 * \li Since the destination point may be off-road, Navit may initialize the route graph with multiple candidates for
 * the destination point, each of which will get a nonzero cost (which may still decrease if routing later determines
 * that it is cheaper to route from that candidate point to a different candidate point).
 *
 * The cost of a node is always the cost to reach the destination, and route calculation is done “backwards”, i.e.
 * starting at the destination and working its way to the current position (or previous waypoint). This is mandatory
 * in LPA*, while A* and Dijkstra can also work from the start to the destination. The latter is found in most textbook
 * descriptions of these algorithms. Navit has always calculated routes from destination to start, even with Dijkstra,
 * as this made it easier to react to changes in the vehicle position (the start of the route).
 *
 * A route graph first needs to be built with `route_graph_build()`, which fetches the segments from the map. Next
 * `route_graph_init()` is called to initialize the destination point candidates. Then
 * `route_graph_compute_shortest_path()` is called to assign a `value` to each node, which represents the cost of
 * traveling from this point to the destination. Each point is also assigned a “next segment” to take in order to reach
 * the destination from this point. Eventually a “route path” is created, i.e. the sequence of segments from the
 * current position to the destination are extracted from the route graph and turn instructions are added where
 * necessary.
 *
 * When segment costs change, `route_graph_point_update()` is called for each end point which may have changed. Then
 * `route_graph_compute_shortest_path()` is called to update parts of the route which may have changed, and finally the
 * route path is recreated. This is used by the traffic module when traffic reports change the cost of individual
 * segments.
 *
 * A completely new route can be created from an existing graph, which happens e.g. between sections of a route when
 * waypoints are used. This is done by calling `route_graph_reset()`, which resets all nodes to their initial state.
 * Then `route_graph_init()` is called, followed by `route_graph_compute_shortest_path()` and eventually creation of
 * the route path.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "navit_nls.h"
#include "glib_slice.h"
#include "config.h"
#include "point.h"
#include "graphics.h"
#include "profile.h"
#include "coord.h"
#include "projection.h"
#include "item.h"
#include "xmlconfig.h"
#include "map.h"
#include "mapset.h"
#include "route_protected.h"
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


#define RSD_OFFSET(x) *((int *)route_segment_data_field_pos((x), attr_offset))
#define RSD_SIZE_WEIGHT(x) *((struct size_weight_limit *)route_segment_data_field_pos((x), attr_vehicle_width))
#define RSD_DANGEROUS_GOODS(x) *((int *)route_segment_data_field_pos((x), attr_vehicle_dangerous_goods))


/**
 * @brief A traffic distortion
 *
 * Traffic distortions represent delays or closures on the route, which can occur for a variety of
 * reasons such as roadworks, accidents or heavy traffic. They are also used internally by Navit to
 * avoid using a particular segment.
 *
 * A traffic distortion can limit the speed on a segment, or introduce a delay. If both are given,
 * at the same time, they are cumulative.
 */
struct route_traffic_distortion {
    int maxspeed;					/**< Maximum speed possible in km/h. Use {@code INT_MAX} to
									     leave the speed unchanged, or 0 to mark the segment as
									     impassable. */
    int delay;					/**< Delay in tenths of seconds (0 for no delay) */
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
 * A route path is an ordered set of segments describing the route from the current position (or previous
 * destination) to the next destination.
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
    NAVIT_OBJECT
    struct mapset *ms;			/**< The mapset this route is built upon */
    enum route_path_flags flags;
    struct route_info *pos;		/**< Current position within this route */
    GList *destinations;		/**< Destinations of the route */
    int reached_destinations_count;	/**< Used as base to calculate waypoint numbers */
    struct route_info *current_dst;	/**< Current destination */

    struct route_graph *graph;	/**< Pointer to the route graph */
    struct route_path *path2;	/**< Pointer to the route path */
    struct map *map;            /**< The map containing the route path */
    struct map *graph_map;      /**< The map containing the route graph */
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

static struct route_info * route_find_nearest_street(struct vehicleprofile *vehicleprofile, struct mapset *ms,
        struct pcoord *c);
static void route_graph_update(struct route *this, struct callback *cb, int async);
static struct route_path *route_path_new(struct route_graph *this, struct route_path *oldpath, struct route_info *pos,
        struct route_info *dst, struct vehicleprofile *profile);
static void route_graph_add_street(struct route_graph *this, struct item *item, struct vehicleprofile *profile);
static void route_graph_destroy(struct route_graph *this);
static void route_path_update(struct route *this, int cancel, int async);
static int route_time_seg(struct vehicleprofile *profile, struct route_segment_data *over,
                          struct route_traffic_distortion *dist);
static void route_graph_compute_shortest_path(struct route_graph * graph, struct vehicleprofile * profile,
        struct callback *cb);
static int route_graph_is_path_computed(struct route_graph *this_);
static struct route_graph_segment *route_graph_get_segment(struct route_graph *graph, struct street_data *sd,
        struct route_graph_segment *last);
static int route_value_seg(struct vehicleprofile *profile, struct route_graph_point *from,
                           struct route_graph_segment *over,
                           int dir);
static void route_graph_init(struct route_graph *this, struct route_info *dst, struct vehicleprofile *profile);
static void route_graph_reset(struct route_graph *this);


/**
 * @brief Returns the projection used for this route
 *
 * @param route The route to return the projection for
 * @return The projection used for this route
 */
static enum projection route_projection(struct route *route) {
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
static struct route_graph_point_iterator rp_iterator_new(struct route_graph_point *p) {
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
*rp_iterator_next(struct route_graph_point_iterator *it) {
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
static int rp_iterator_end(struct route_graph_point_iterator *it) {
    if (it->end && (it->next != it->p->end)) {
        return 1;
    } else {
        return 0;
    }
}

static void route_path_get_distances(struct route_path *path, struct coord *c, int count, int *distances) {
    int i;
    for (i = 0 ; i < count ; i++)
        distances[i]=INT_MAX;
    while (path) {
        struct route_path_segment *seg=path->path;
        while (seg) {
            for (i = 0 ; i < count ; i++) {
                int dist=transform_distance_polyline_sq(seg->c, seg->ncoords, &c[i], NULL, NULL);
                if (dist < distances[i])
                    distances[i]=dist;
            }
            seg=seg->next;
        }
        path=path->next;
    }
    for (i = 0 ; i < count ; i++) {
        if (distances[i] != INT_MAX)
            distances[i]=sqrt(distances[i]);
    }
}

void route_get_distances(struct route *this, struct coord *c, int count, int *distances) {
    return route_path_get_distances(this->path2, c, count, distances);
}

/**
 * @brief Destroys a route_path
 *
 * @param this The route_path to be destroyed
 */
static void route_path_destroy(struct route_path *this, int recurse) {
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
        this->in_use--;
        if (!this->in_use)
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
route_new(struct attr *parent, struct attr **attrs) {
    struct route *this=g_new0(struct route, 1);
    struct attr dest_attr;

    this->func=&route_func;
    navit_object_ref((struct navit_object *)this);

    if (attr_generic_get_attr(attrs, NULL, attr_destination_distance, &dest_attr, NULL)) {
        this->destination_distance = dest_attr.u.num;
    } else {
        this->destination_distance = 50; // Default value
    }
    this->cbl2=callback_list_new();

    return this;
}

/**
 * @brief Duplicates a route object
 *
 * @return The duplicated route
 */

struct route *
route_dup(struct route *orig) {
    struct route *this=g_new0(struct route, 1);
    this->func=&route_func;
    navit_object_ref((struct navit_object *)this);
    this->cbl2=callback_list_new();
    this->destination_distance=orig->destination_distance;
    this->ms=orig->ms;
    this->flags=orig->flags;
    this->vehicleprofile=orig->vehicleprofile;

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
static int route_check_roundabout(struct route_graph_segment *seg, int level, int direction,
                                  struct route_graph_segment *origin) {
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
void route_set_mapset(struct route *this, struct mapset *ms) {
    this->ms=ms;
}

/**
 * @brief Sets the vehicle profile of a route
 *
 * @param this The route to set the profile for
 * @param prof The vehicle profile
 */

void route_set_profile(struct route *this, struct vehicleprofile *prof) {
    if (this->vehicleprofile != prof) {
        int dest_count = g_list_length(this->destinations);
        struct pcoord *pc;
        this->vehicleprofile = prof;
        pc = g_alloca(dest_count*sizeof(struct pcoord));
        route_get_destinations(this, pc, dest_count);
        route_set_destinations(this, pc, dest_count, 1);
    }
}

/**
 * @brief Returns the mapset of the route passed
 *
 * @param this The route to get the mapset of
 * @return The mapset of the route passed
 */
struct mapset *
route_get_mapset(struct route *this) {
    return this->ms;
}

/**
 * @brief Returns the current position within the route passed
 *
 * @param this The route to get the position for
 * @return The position within the route passed
 */
struct route_info *
route_get_pos(struct route *this) {
    return this->pos;
}

/**
 * @brief Returns the destination of the route passed
 *
 * @param this The route to get the destination for
 * @return The destination of the route passed
 */
struct route_info *
route_get_dst(struct route *this) {
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
int route_get_path_set(struct route *this) {
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
int route_contains(struct route *this, struct item *item) {
    if (! this->path2 || !this->path2->path_hash)
        return 0;
    if (item_hash_lookup(this->path2->path_hash, item))
        return 1;
    if (! this->pos || !this->pos->street)
        return 0;
    return item_is_equal(this->pos->street->item, *item);

}

static struct route_info *route_next_destination(struct route *this) {
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
int route_destination_reached(struct route *this) {
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

/**
 * @brief Returns the position from which to route to the current destination of the route.
 *
 * This function examines the destination list of the route. If present, it returns the destination
 * which precedes the one indicated by the {@code current_dst} member of the route. Failing that,
 * the current position of the route is returned.
 *
 * @param this The route object
 * @return The previous destination or current position, see description
 */
static struct route_info *route_previous_destination(struct route *this) {
    GList *l=g_list_find(this->destinations, this->current_dst);
    if (!l)
        return this->pos;
    l=g_list_previous(l);
    if (!l)
        return this->pos;
    return l->data;
}

/**
 * @brief Updates or recreates the route graph.
 *
 * This function is called after the route graph has been changed or rebuilt and flooding has
 * completed. It then updates the route path to reflect these changes.
 *
 * If multiple destinations are set, this function will reset and re-flood the route graph for each
 * destination, thus recursively calling itself for each destination.
 *
 * @param this The route object
 * @param new_graph FIXME Whether the route graph has been rebuilt from scratch
 */
/* FIXME Should we rename this function to route_graph_flood_done, in order to avoid confusion? */
static void route_path_update_done(struct route *this, int new_graph) {
    struct route_path *oldpath=this->path2;
    struct attr route_status;
    struct route_info *prev_dst; /* previous destination or current position */
    route_status.type=attr_route_status;
    if (this->path2 && (this->path2->in_use>1)) {
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
        else
            route_path_destroy(oldpath,0);
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
                dbg(lvl_debug,"error");
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
            route_graph_init(this->graph, this->current_dst, this->vehicleprofile);
            route_graph_compute_shortest_path(this->graph, this->vehicleprofile, this->route_graph_flood_done_cb);
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
 * The behavior of this function can be controlled via flags:
 * <ul>
 * <li>{@code route_path_flag_cancel}: Cancel navigation, clear route graph and route path</li>
 * <li>{@code route_path_flag_async}: Perform operations asynchronously</li>
 * <li>{@code route_path_flag_no_rebuild}: Do not rebuild the route graph</li>
 * </ul>
 *
 * These flags will be stored in the {@code flags} member of the route object.
 *
 * @attention For this to work the route graph has to be destroyed if the route's
 * @attention destination is changed somewhere!
 *
 * @param this The route to update
 * @param flags Flags to control the behavior of this function, see description
 */
static void route_path_update_flags(struct route *this, enum route_path_flags flags) {
    dbg(lvl_debug,"enter %d", flags);
    this->flags = flags;
    if (! this->pos || ! this->destinations) {
        dbg(lvl_debug,"destroy");
        route_path_destroy(this->path2,1);
        this->path2 = NULL;
        return;
    }
    if (flags & route_path_flag_cancel) {
        route_graph_destroy(this->graph);
        this->graph=NULL;
    }
    /* the graph is destroyed when setting the destination */
    if (this->graph) {
        if (this->graph->busy) {
            dbg(lvl_debug,"busy building graph");
            return;
        }
        // we can try to update
        dbg(lvl_debug,"try update");
        route_path_update_done(this, 0);
    } else {
        route_path_destroy(this->path2,1);
        this->path2 = NULL;
    }
    if (!this->graph || (!this->path2 && !(flags & route_path_flag_no_rebuild))) {
        dbg(lvl_debug,"rebuild graph %p %p",this->graph,this->path2);
        if (! this->route_graph_flood_done_cb)
            this->route_graph_flood_done_cb=callback_new_2(callback_cast(route_path_update_done), this, (long)1);
        dbg(lvl_debug,"route_graph_update");
        route_graph_update(this, this->route_graph_flood_done_cb, !!(flags & route_path_flag_async));
    }
}

/**
 * @brief Updates the route graph and the route path if something changed with the route
 *
 * This function is a wrapper around {@link route_path_update_flags(route *, enum route_path)}.
 *
 * @param this The route to update
 * @param cancel If true, cancel navigation, clear route graph and route path
 * @param async If true, perform processing asynchronously
 */
static void route_path_update(struct route *this, int cancel, int async) {
    enum route_path_flags flags=(cancel ? route_path_flag_cancel:0)|(async ? route_path_flag_async:0);
    route_path_update_flags(this, flags);
}


/**
 * @brief This will calculate all the distances stored in a route_info
 *
 * @param ri The route_info to calculate the distances for
 * @param pro The projection used for this route
 */
static void route_info_distances(struct route_info *ri, enum projection pro) {
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
 * @param flags Flags to use for building the graph
 */

static int route_set_position_flags(struct route *this, struct pcoord *pos, enum route_path_flags flags) {
    if (this->pos)
        route_info_free(this->pos);
    this->pos=NULL;
    this->pos=route_find_nearest_street(this->vehicleprofile, this->ms, pos);

    // If there is no nearest street, bail out.
    if (!this->pos) return 0;

    this->pos->street_direction=0;
    dbg(lvl_debug,"this->pos=%p", this->pos);
    route_info_distances(this->pos, pos->pro);
    route_path_update_flags(this, flags);
    return 1;
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
void route_set_position(struct route *this, struct pcoord *pos) {
    route_set_position_flags(this, pos, route_path_flag_async);
}

/**
 * @brief Sets a route's current position based on coordinates from tracking
 *
 * @param this The route to set the current position of
 * @param tracking The tracking to get the coordinates from
 */
void route_set_position_from_tracking(struct route *this, struct tracking *tracking, enum projection pro) {
    struct coord *c;
    struct route_info *ret;
    struct street_data *sd;

    dbg(lvl_info,"enter");
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
    dbg(lvl_debug,"position 0x%x,0x%x item 0x%x,0x%x direction %d pos %d lenpos %d lenneg %d",c->x,c->y,sd?sd->item.id_hi:0,
        sd?sd->item.id_lo:0,ret->street_direction,ret->pos,ret->lenpos,ret->lenneg);
    dbg(lvl_debug,"c->x=0x%x, c->y=0x%x pos=%d item=(0x%x,0x%x)", c->x, c->y, ret->pos,
        ret->street?ret->street->item.id_hi:0, ret->street?ret->street->item.id_lo:0);
    dbg(lvl_debug,"street 0=(0x%x,0x%x) %d=(0x%x,0x%x)", ret->street?ret->street->c[0].x:0,
        ret->street?ret->street->c[0].y:0, ret->street?ret->street->count-1:0,
        ret->street?ret->street->c[ret->street->count-1].x:0, ret->street?ret->street->c[ret->street->count-1].y:0);
    this->pos=ret;
    if (this->destinations)
        route_path_update(this, 0, 1);
    dbg(lvl_info,"ret");
}

/* Used for debuging of route_rect, what routing sees */
struct map_selection *route_selection;

/**
 * @brief Returns a single map selection
 *
 * The boundaries of the selection are determined as follows: First a rectangle spanning `c1` and `c2` is
 * built (the two coordinates can be any two opposite corners of the rectangle). Then its maximum extension
 * (height or width) is determined and multiplied with the percentage specified by `rel`. The resulting
 * amount of padding is added to each edge. After that, the amount specified by `abs` is added to each edge.
 *
 * @param order Map order (deepest tile level) to select
 * @param c1 First coordinate
 * @param c2 Second coordinate
 * @param rel Relative padding to add to the selection rectangle, in percent
 * @param abs Absolute padding to add to the selection rectangle
 */
struct map_selection *
route_rect(int order, struct coord *c1, struct coord *c2, int rel, int abs) {
    int dx,dy,sx=1,sy=1,d,m;
    struct map_selection *sel=g_new(struct map_selection, 1);
    if (!sel) {
        printf("%s:Out of memory\n", __FUNCTION__);
        return sel;
    }
    sel->order=order;
    sel->range.min=route_item_first;
    sel->range.max=route_item_last;
    dbg(lvl_debug,"%p %p", c1, c2);
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
 * @brief Appends a map selection to the selection list. Selection list may be NULL.
 */
static struct map_selection *route_rect_add(struct map_selection *sel, int order, struct coord *c1, struct coord *c2,
        int rel, int abs) {
    struct map_selection *ret;
    ret=route_rect(order, c1, c2, rel, abs);
    ret->next=sel;
    return ret;
}

/**
 * @brief Returns a list of map selections useable to create a route graph
 *
 * Returns a list of  map selections useable to get a map rect from which items can be
 * retrieved to build a route graph.
 *
 * @param c Array containing route points, including start, intermediate and destination ones.
 * @param count number of route points
 * @param proifle vehicleprofile
 */
static struct map_selection *route_calc_selection(struct coord *c, int count, struct vehicleprofile *profile) {
    struct map_selection *ret=NULL;
    int i;
    struct coord_rect r;
    char *depth, *str, *tok;

    if (!count)
        return NULL;
    r.lu=c[0];
    r.rl=c[0];
    for (i = 1 ; i < count ; i++)
        coord_rect_extend(&r, &c[i]);

    depth=profile->route_depth;
    if (!depth)
        depth="4:25%,8:40000,18:10000";
    depth=str=g_strdup(depth);

    while((tok=strtok(str,","))!=NULL) {
        int order=0, dist=0;
        sscanf(tok,"%d:%d",&order,&dist);
        if(strchr(tok,'%'))
            ret=route_rect_add(ret, order, &r.lu, &r.rl, dist, 0);
        else
            for (i = 0 ; i < count ; i++) {
                ret=route_rect_add(ret, order, &c[i], &c[i], 0, dist);
            }
        str=NULL;
    }

    g_free(depth);

    return ret;
}

/**
 * @brief Retrieves the map selection for the route.
 */
struct map_selection * route_get_selection(struct route * this_) {
    struct coord *c = g_alloca(sizeof(struct coord) * (1 + g_list_length(this_->destinations)));
    int i = 0;
    GList *tmp;

    if (this_->pos)
        c[i++] = this_->pos->c;
    tmp = this_->destinations;
    while (tmp) {
        struct route_info *dst = tmp->data;
        c[i++] = dst->c;
        tmp = g_list_next(tmp);
    }
    return route_calc_selection(c, i, this_->vehicleprofile);
}

/**
 * @brief Destroys a list of map selections
 *
 * @param sel Start of the list to be destroyed
 */
void route_free_selection(struct map_selection *sel) {
    struct map_selection *next;
    while (sel) {
        next=sel->next;
        g_free(sel);
        sel=next;
    }
}


/* for compatibility to GFunc */
static void route_info_free_g(struct route_info *inf, void * unused) {
    route_info_free(inf);
}

static void route_clear_destinations(struct route *this_) {
    g_list_foreach(this_->destinations, (GFunc)route_info_free_g, NULL);
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
 * @param dst Points to an array of coordinates to set as destinations, which will be visited in the
 * order in which they appear in the array (the last one is the final destination)
 * @param count Number of items in {@code dst}, 0 to clear all destinations
 * @param async If set, do routing asynchronously
 */

void route_set_destinations(struct route *this, struct pcoord *dst, int count, int async) {
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
    } else  {
        this->reached_destinations_count=0;
        route_status.u.num=route_status_no_destination;
    }
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

/**
 * @brief Retrieves destinations from the route
 *
 * Prior to calling this method, you may want to retrieve the number of destinations by calling
 * {@link route_get_destination_count(struct route *)} and assigning a buffer of sufficient capacity.
 *
 * If the return value equals `count`, the buffer was either just large enough or too small to hold the
 * entire list of destinations; there is no way to tell from the result which is the case.
 *
 * @param this The route instance
 * @param pc Pointer to an array of projected coordinates which will receive the destination coordinates
 * @param count Capacity of `pc`
 * @return The number of destinations stored in `pc`, never greater than `count`
 */
int route_get_destinations(struct route *this, struct pcoord *pc, int count) {
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

/**
 * @brief Get the destinations count for the route
 *
 * @param this The route instance
 * @return destination count for the route
 */
int route_get_destination_count(struct route *this) {
    return g_list_length(this->destinations);
}

/**
 * @brief Returns a description for a waypoint as (type or street_name_systematic) + (label or WayID[osm_wayid])
 *
 * @param this The route instance
 * @param n The nth waypoint
 * @return The description
 */
char* route_get_destination_description(struct route *this, int n) {
    struct route_info *dst;
    struct map_rect *mr=NULL;
    struct item *item;
    struct attr attr;
    char *type=NULL;
    char *label=NULL;
    char *desc=NULL;

    if(!this->destinations)
        return NULL;

    dst=g_list_nth_data(this->destinations,n);
    mr=map_rect_new(dst->street->item.map, NULL);
    item = map_rect_get_item_byid(mr, dst->street->item.id_hi, dst->street->item.id_lo);

    type=g_strdup(item_to_name(dst->street->item.type));

    while(item_attr_get(item, attr_any, &attr)) {
        if (attr.type==attr_street_name_systematic ) {
            g_free(type);
            type=attr_to_text(&attr, item->map, 1);
        } else if (attr.type==attr_label) {
            g_free(label);
            label=attr_to_text(&attr, item->map, 1);
        } else if (attr.type==attr_osm_wayid && !label) {
            char *tmp=attr_to_text(&attr, item->map, 1);
            label=g_strdup_printf("WayID %s", tmp);
            g_free(tmp);
        }
    }

    if(!label && !type) {
        desc=g_strdup(_("unknown street"));
    } else if (!label || strcmp(type, label)==0) {
        desc=g_strdup(type);
    } else {
        desc=g_strdup_printf("%s %s", type, label);
    }

    g_free(label);
    g_free(type);

    if (mr)
        map_rect_destroy(mr);

    return desc;
}

/**
 * @brief Start a route given set of coordinates
 *
 * @param this The route instance
 * @param dst The coordinate to start routing to
 * @param async Set to 1 to do route calculation asynchronously
 * @return nothing
 */
void route_set_destination(struct route *this, struct pcoord *dst, int async) {
    route_set_destinations(this, dst, dst?1:0, async);
}

/**
 * @brief Append a waypoint to the route.
 *
 * This appends a waypoint to the current route, targetting the street
 * nearest to the coordinates passed, and updates the route.
 *
 * @param this The route to set the destination for
 * @param dst Coordinates of the new waypoint
 * @param async: If set, do routing asynchronously
 */
void route_append_destination(struct route *this, struct pcoord *dst, int async) {
    if (dst) {
        struct route_info *dsti;
        dsti=route_find_nearest_street(this->vehicleprofile, this->ms, &dst[0]);
        if(dsti) {
            route_info_distances(dsti, dst->pro);
            this->destinations=g_list_append(this->destinations, dsti);
        }
        /* The graph has to be destroyed and set to NULL, otherwise route_path_update() doesn't work */
        route_graph_destroy(this->graph);
        this->graph=NULL;
        this->current_dst=route_get_dst(this);
        route_path_update(this, 1, async);
    } else {
        route_set_destinations(this, NULL, 0, async);
    }
}

/**
 * @brief Remove the nth waypoint of the route
 *
 * @param this The route instance
 * @param n The waypoint to remove
 * @return nothing
 */
void route_remove_nth_waypoint(struct route *this, int n) {
    struct route_info *ri=g_list_nth_data(this->destinations, n);
    this->destinations=g_list_remove(this->destinations,ri);
    route_info_free(ri);
    /* The graph has to be destroyed and set to NULL, otherwise route_path_update() doesn't work */
    route_graph_destroy(this->graph);
    this->graph=NULL;
    this->current_dst=route_get_dst(this);
    route_path_update(this, 1, 1);
}

void route_remove_waypoint(struct route *this) {
    if (this->path2) {
        struct route_path *path = this->path2;
        struct route_info *ri = this->destinations->data;
        this->destinations = g_list_remove(this->destinations, ri);
        route_info_free(ri);
        this->path2 = this->path2->next;
        route_path_destroy(path, 0);
        if (!this->destinations) {
            this->route_status=route_status_no_destination;
            this->reached_destinations_count=0;
            return;
        }
        this->reached_destinations_count++;
        route_graph_reset(this->graph);
        this->current_dst = this->destinations->data;
        route_graph_init(this->graph, this->current_dst, this->vehicleprofile);
        route_graph_compute_shortest_path(this->graph, this->vehicleprofile, this->route_graph_flood_done_cb);
    }
}

/**
 * @brief Gets the next route_graph_point with the specified coordinates
 *
 * @param this The route in which to search
 * @param c Coordinates to search for
 * @param last The last route graph point returned to iterate over multiple points with the same coordinates,
 * or {@code NULL} to return the first point
 * @return The point at the specified coordinates or NULL if not found
 */
struct route_graph_point *route_graph_get_point_next(struct route_graph *this, struct coord *c,
        struct route_graph_point *last) {
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

/**
 * @brief Gets the first route_graph_point with the specified coordinates
 *
 * @param this The route in which to search
 * @param c Coordinates to search for
 * @return The point at the specified coordinates or NULL if not found
 */
struct route_graph_point * route_graph_get_point(struct route_graph *this, struct coord *c) {
    return route_graph_get_point_next(this, c, NULL);
}

/**
 * @brief Gets the last route_graph_point with the specified coordinates
 *
 * @param this The route in which to search
 * @param c Coordinates to search for
 * @return The point at the specified coordinates or NULL if not found
 */
static struct route_graph_point *route_graph_get_point_last(struct route_graph *this, struct coord *c) {
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

static struct route_graph_point *route_graph_point_new(struct route_graph *this, struct coord *f) {
    int hashval;
    struct route_graph_point *p;

    hashval=HASHCOORD(f);
    if (debug_route)
        printf("p (0x%x,0x%x)\n", f->x, f->y);
    p=g_slice_new0(struct route_graph_point);
    p->hash_next=this->hash[hashval];
    this->hash[hashval]=p;
    p->value=INT_MAX;
    p->dst_val = INT_MAX;
    p->c=*f;
    return p;
}

/**
 * @brief Inserts a point into the route graph at the specified coordinates
 *
 * This will insert a point into the route graph at the coordinates passed in f.
 * Note that the point is not yet linked to any segments.
 *
 * If the route graph already contains a point at the specified coordinates, the existing point
 * will be returned.
 *
 * @param this The route graph to insert the point into
 * @param f The coordinates at which the point should be inserted
 * @return The point inserted or NULL on failure
 */
struct route_graph_point * route_graph_add_point(struct route_graph *this, struct coord *f) {
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
void route_graph_free_points(struct route_graph *this) {
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
 * @brief Initializes potential destination nodes.
 *
 * This method is normally called after building a fresh route graph, or resetting an existing one. It iterates over
 * all potential destination nodes (i.e. all nodes which are part of the destination’s street) and initializes them:
 * The `dst_val` and `rhs` values are set according to their cost to reach the destination.
 *
 * @param this The route graph to initialize
 * @param dst The destination of the route
 * @param profile The vehicle profile to use for routing. This determines which ways are passable
 * and how their costs are calculated.
 */
static void route_graph_init(struct route_graph *this, struct route_info *dst, struct vehicleprofile *profile) {
    struct route_graph_segment *s = NULL;
    int val;

    while ((s = route_graph_get_segment(this, dst->street, s))) {
        val = route_value_seg(profile, NULL, s, -1);
        if (val != INT_MAX) {
            val = val*(100-dst->percent)/100;
            s->end->seg = s;
            s->end->dst_seg = s;
            s->end->rhs = val;
            s->end->dst_val = val;
            s->end->el = fh_insertkey(this->heap, MIN(s->end->rhs, s->end->value), s->end);
        }
        val = route_value_seg(profile, NULL, s, 1);
        if (val != INT_MAX) {
            val = val*dst->percent/100;
            s->start->seg = s;
            s->start->dst_seg = s;
            s->start->rhs = val;
            s->start->dst_val = val;
            s->start->el = fh_insertkey(this->heap, MIN(s->start->rhs, s->start->value), s->start);
        }
    }
}

/**
 * @brief Resets all nodes
 *
 * This iterates through all the points in the route graph, resetting them to their initial state.
 * The `value` (cost to reach the destination via `seg`) and `dst_val` (cost to destination if this point is the last
 * in the route) members of each point are reset to`INT_MAX`, the `seg` member (cheapest way to destination) is reset
 * to `NULL` and the `el` member (pointer to element in Fibonacci heap) is also reset to `NULL`.
 *
 * The Fibonacci heap is also cleared. Inconsistencies between `el` and Fibonacci heap membership are handled
 * gracefully, i.e. `el` is reset even if it is invalid, and points are removed from the heap regardless of their `el`
 * value.
 *
 * After this method returns, the caller should call
 * {@link route_graph_init(struct route_graph *, struct route_info *, struct vehicleprofile *)} to initialize potential
 * end points. After that a route can be calculated.
 *
 * References to elements of the route graph which were obtained prior to calling this function
 * remain valid after it returns.
 *
 * @param this The route graph to reset
 */
static void route_graph_reset(struct route_graph *this) {
    struct route_graph_point *curr;
    int i;

    for (i = 0 ; i < HASH_SIZE ; i++) {
        curr=this->hash[i];
        while (curr) {
            curr->value=INT_MAX;
            curr->dst_val = INT_MAX;
            curr->rhs = INT_MAX;
            curr->seg=NULL;
            curr->dst_seg = NULL;
            curr->el=NULL;
            curr=curr->hash_next;
        }
    }

    while (fh_extractmin(this->heap)) {
        // no operation, just remove all items (`el` has already been reset)
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
void * route_segment_data_field_pos(struct route_segment_data *seg, enum attr_type type) {
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

static int route_segment_data_size(int flags) {
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


/**
 * @brief Checks if the route graph already contains a particular segment.
 *
 * This function compares the item IDs of both segments. If the item is segmented, the segment offset is
 * also compared.
 *
 * @param start The starting point of the segment
 * @param data The data for the segment
 */
int route_graph_segment_is_duplicate(struct route_graph_point *start, struct route_graph_segment_data *data) {
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
 * @param this The route graph to insert the segment into
 * @param start The graph point which should be connected to the start of this segment
 * @param end The graph point which should be connected to the end of this segment
 * @param len The length of this segment
 * @param item The item that is represented by this segment
 * @param flags Flags for this segment
 * @param offset If the item passed in "item" is segmented (i.e. divided into several segments), this indicates the position of this segment within the item
 * @param maxspeed The maximum speed allowed on this segment in km/h. -1 if not known.
 */
void route_graph_add_segment(struct route_graph *this, struct route_graph_point *start,
                             struct route_graph_point *end, struct route_graph_segment_data *data) {
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
    s->data.score = data->score;

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
 * @brief Returns and removes one segment from a path
 *
 * @param path The path to take the segment from
 * @param item The item whose segment to remove
 * @param offset Offset of the segment within the item to remove. If the item is not segmented this should be 1.
 * @return The segment removed
 */
static struct route_path_segment *route_extract_segment_from_path(struct route_path *path, struct item *item,
        int offset) {
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
static void route_path_add_segment(struct route_path *this, struct route_path_segment *segment) {
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
static void route_path_add_line(struct route_path *this, struct coord *start, struct coord *end, int len) {
    int ccnt=2;
    struct route_path_segment *segment;
    int seg_size,seg_dat_size;

    dbg(lvl_debug,"line from 0x%x,0x%x-0x%x,0x%x", start->x, start->y, end->x, end->y);
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
 * @brief Inserts a new segment into the path
 *
 * This function adds a new segment to the route path. The segment is copied from the route graph. If
 * `rgs` is part of a segmented item, only `rgs` will be added to the route path, not the other segments.
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

static int route_path_add_item_from_graph(struct route_path *this, struct route_path *oldpath,
        struct route_graph_segment *rgs,
        int dir, struct route_info *pos, struct route_info *dst) {
    struct route_path_segment *segment=NULL;
    int i, ccnt, extra=0, ret=0;
    struct coord *c,*cd,ca[2048];
    int offset=1;
    int seg_size,seg_dat_size;
    int len=rgs->data.len;
    if (rgs->data.flags & AF_SEGMENTED)
        offset=RSD_OFFSET(&rgs->data);

    dbg(lvl_debug,"enter (0x%x,0x%x) dir=%d pos=%p dst=%p", rgs->data.item.id_hi, rgs->data.item.id_lo, dir, pos, dst);
    if (oldpath) {
        segment=item_hash_lookup(oldpath->path_hash, &rgs->data.item);
        if (segment && segment->direction == dir) {
            segment = route_extract_segment_from_path(oldpath, &rgs->data.item, offset);
            if (segment) {
                ret=1;
                if (!pos)
                    goto linkold;
            }
            g_free(segment);
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
            dbg(lvl_debug,"pos dir=%d", dir);
            dbg(lvl_debug,"pos pos=%d", pos->pos);
            dbg(lvl_debug,"pos count=%d", pos->street->count);
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
        dbg(lvl_debug,"dst dir=%d", dir);
        dbg(lvl_debug,"dst pos=%d", dst->pos);
        if (dir > 0) {
            c=dst->street->c;
            ccnt=dst->pos+1;
            len=dst->lenneg;
        } else {
            c=dst->street->c+dst->pos+1;
            ccnt=dst->street->count-dst->pos-1;
            len=dst->lenpos;
        }
    } else {
        ccnt=item_coord_get_within_range(&rgs->data.item, ca, 2047, &rgs->start->c, &rgs->end->c);
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
void route_graph_free_segments(struct route_graph *this) {
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
static void route_graph_destroy(struct route_graph *this) {
    if (this) {
        route_graph_build_done(this, 1);
        route_graph_free_points(this);
        route_graph_free_segments(this);
        fh_deleteheap(this->heap);
        g_free(this);
    }
}

/**
 * @brief Returns the estimated speed on a segment, or 0 for an impassable segment
 *
 * This function returns the estimated speed to be driven on a segment, calculated as follows:
 * <ul>
 * <li>Initially the route weight of the vehicle profile for the given item type is used. If the
 * item type does not have a route weight in the vehicle profile given, it is considered impassable
 * and 0 is returned.</li>
 * <li>If the {@code maxspeed} attribute of the segment's item is set, either it or the previous
 * speed estimate for the segment is used, as governed by the vehicle profile's
 * {@code maxspeed_handling} attribute.</li>
 * <li>If a traffic distortion is present, its {@code maxspeed} is taken into account in a similar
 * manner. Unlike the regular {@code maxspeed}, a {@code maxspeed} resulting from a traffic
 * distortion is always considered if it limits the speed, regardless of {@code maxspeed_handling}.
 * </li>
 * <li>Access restrictions for dangerous goods, size or weight are evaluated, and 0 is returned if
 * the given vehicle profile violates one of them.</li>
 * </ul>
 *
 * @param profile The routing preferences
 * @param over The segment which is passed
 * @param dist A traffic distortion if applicable, or {@code NULL}
 * @return The estimated speed in km/h, or 0 if the segment is impassable
 */
static int route_seg_speed(struct vehicleprofile *profile, struct route_segment_data *over,
                           struct route_traffic_distortion *dist) {
    struct roadprofile *roadprofile=vehicleprofile_get_roadprofile(profile, over->item.type);
    int speed,maxspeed;
    if (!roadprofile || !roadprofile->route_weight)
        return 0;
    speed=roadprofile->route_weight;
    if (profile->maxspeed_handling != maxspeed_ignore) {
        if (over->flags & AF_SPEED_LIMIT) {
            maxspeed=RSD_MAXSPEED(over);
            if (profile->maxspeed_handling == maxspeed_enforce)
                speed=maxspeed;
        } else
            maxspeed=INT_MAX;
        if (dist && maxspeed > dist->maxspeed)
            maxspeed=dist->maxspeed;
        if (maxspeed != INT_MAX && (profile->maxspeed_handling != maxspeed_restrict || maxspeed < speed))
            speed=maxspeed;
    }
    if (over->flags & AF_DANGEROUS_GOODS) {
        if (profile->dangerous_goods & RSD_DANGEROUS_GOODS(over))
            return 0;
    }
    if (over->flags & AF_SIZE_OR_WEIGHT_LIMIT) {
        struct size_weight_limit *size_weight=&RSD_SIZE_WEIGHT(over);
        if (size_weight->width != -1 && profile->width != -1 && profile->width > size_weight->width)
            return 0;
        if (size_weight->height != -1 && profile->height != -1 && profile->height > size_weight->height)
            return 0;
        if (size_weight->length != -1 && profile->length != -1 && profile->length > size_weight->length)
            return 0;
        if (size_weight->weight != -1 && profile->weight != -1 && profile->weight > size_weight->weight)
            return 0;
        if (size_weight->axle_weight != -1 && profile->axle_weight != -1 && profile->axle_weight > size_weight->axle_weight)
            return 0;
    }
    return speed;
}

/**
 * @brief Returns the time needed to travel along a segment, or {@code INT_MAX} if the segment is impassable.
 *
 * This function returns the time needed to travel along the entire length of {@code over} in
 * tenths of seconds. Restrictions for dangerous goods, weight or size are taken into account.
 * Traffic distortions are also taken into account if a valid {@code dist} argument is given.
 *
 * @param profile The vehicle profile (routing preferences)
 * @param over The segment which is passed
 * @param dist A traffic distortion if applicable, or {@code NULL}
 * @return The time needed in tenths of seconds
 */

static int route_time_seg(struct vehicleprofile *profile, struct route_segment_data *over,
                          struct route_traffic_distortion *dist) {
    int speed=route_seg_speed(profile, over, dist);
    if (!speed)
        return INT_MAX;
    return over->len*36/speed+(dist ? dist->delay : 0);
}

/**
 * @brief Returns the traffic distortion for a segment.
 *
 * If multiple traffic distortions match a segment, the return value will report the lowest speed limit
 * and greatest delay of all matching segments.
 *
 * @param seg The segment for which the traffic distortion is to be returned
 * @param dir The direction of `seg` for which to return traffic distortions. Positive values indicate
 * travel in the direction of the segment, negative values indicate travel against it.
 * @param profile The current vehicle profile
 * @param ret Points to a {@code struct route_traffic_distortion}, whose members will be filled with the
 * distortion data
 *
 * @return true if a traffic distortion was found, 0 if not
 */
static int route_get_traffic_distortion(struct route_graph_segment *seg, int dir, struct vehicleprofile *profile,
                                        struct route_traffic_distortion *ret) {
    struct route_graph_point *start=seg->start;
    struct route_graph_point *end=seg->end;
    struct route_graph_segment *tmp,*found=NULL;
    struct route_traffic_distortion result;

    if (!dir) {
        dbg(lvl_warning, "dir is zero, assuming positive");
        dir = 1;
    }

    result.delay = 0;
    result.maxspeed = INT_MAX;

    for (tmp = start->start; tmp; tmp = tmp->start_next) {
        if (tmp->data.item.type == type_traffic_distortion && tmp->start == start && tmp->end == end) {
            if ((tmp->data.flags & (dir > 0 ? profile->flags_forward_mask : profile->flags_reverse_mask)) != profile->flags)
                continue;
            if (tmp->data.len > result.delay)
                result.delay = tmp->data.len;
            if ((tmp->data.flags & AF_SPEED_LIMIT) && (RSD_MAXSPEED(&tmp->data) < result.maxspeed))
                result.maxspeed = RSD_MAXSPEED(&tmp->data);
            found=tmp;
        }
    }
    for (tmp = start->end; tmp; tmp = tmp->end_next) {
        if (tmp->data.item.type == type_traffic_distortion && tmp->end == start && tmp->start == end) {
            if ((tmp->data.flags & (dir < 0 ? profile->flags_forward_mask : profile->flags_reverse_mask)) != profile->flags)
                continue;
            if (tmp->data.len > result.delay)
                result.delay = tmp->data.len;
            if ((tmp->data.flags & AF_SPEED_LIMIT) && (RSD_MAXSPEED(&tmp->data) < result.maxspeed))
                result.maxspeed = RSD_MAXSPEED(&tmp->data);
            found=tmp;
        }
    }
    if (found) {
        ret->delay = result.delay;
        ret->maxspeed = result.maxspeed;
        return 1;
    }
    return 0;
}

static int route_through_traffic_allowed(struct vehicleprofile *profile, struct route_graph_segment *seg) {
    return (seg->data.flags & AF_THROUGH_TRAFFIC_LIMIT) == 0;
}

/**
 * @brief Returns the "cost" of traveling along segment `over` in direction `dir`
 *
 * Cost is relative to time, indicated in tenths of seconds.
 *
 * This function considers traffic distortions as well as penalties. If the segment is impassable due to traffic
 * distortions or restrictions, `INT_MAX` is returned in order to prevent use of this segment for routing.
 *
 * If `from` is specified, it must be the point at which we leave the segment (`over->end` if `dir` is positive,
 * `over->start` if `dir` is negative); anything else will produce invalid results. If `from` is non-NULL, additional
 * checks are done on `from->seg` (the next segment to follow after `over`):
 * \li If `from->seg` equals `over` (indicating that, after traversing `over` in direction `dir`, we would immediately
 * traverse it again in the opposite direction), `INT_MAX` is returned.
 * \li If `over` loops back to itself (i.e. its `start` and `end` members are equal), `INT_MAX` is returned.
 * \li Otherwise, if `over` does not allow through traffic but `from->seg` does, the through traffic penalty of the
 * vehicle profile (`profile`) is applied.
 *
 * @param profile The routing preferences
 * @param from The point currently being visited (or NULL), see description
 * @param over The segment we are using
 * @param dir The direction of segment which we are traveling. Positive values indicate we are traveling in the
 * direction of the segment (from `over->start` to `over->end`), negative values indicate we are traveling in the
 * opposite direction. Values of +2 or -2 cause the function to ignore traffic distortions.
 *
 * @return The "cost" needed to travel along the segment
 */
/* FIXME `from` as a name is highly misleading, find a better one */
static int route_value_seg(struct vehicleprofile *profile, struct route_graph_point *from,
                           struct route_graph_segment *over,
                           int dir) {
    int ret;
    struct route_traffic_distortion dist,*distp=NULL;
    if (!dir) {
        dbg(lvl_warning, "dir is zero, assuming positive");
        dir = 1;
    }
    if (from && (over->start == over->end))
        return INT_MAX;
    if ((over->data.flags & (dir >= 0 ? profile->flags_forward_mask : profile->flags_reverse_mask)) != profile->flags)
        return INT_MAX;
    if (dir > 0 && (over->start->flags & RP_TURN_RESTRICTION))
        return INT_MAX;
    if (dir < 0 && (over->end->flags & RP_TURN_RESTRICTION))
        return INT_MAX;
    if (from && from->seg == over)
        return INT_MAX;
    if (over->data.item.type == type_traffic_distortion)
        return INT_MAX;
    if ((over->start->flags & RP_TRAFFIC_DISTORTION) && (over->end->flags & RP_TRAFFIC_DISTORTION) &&
            route_get_traffic_distortion(over, dir, profile, &dist) && dir != 2 && dir != -2) {
        /* we have a traffic distortion */
        distp=&dist;
    }
    ret=route_time_seg(profile, &over->data, distp);
    if (ret == INT_MAX)
        return ret;
    if (!route_through_traffic_allowed(profile, over) && from && from->seg
            && route_through_traffic_allowed(profile, from->seg))
        ret+=profile->through_traffic_penalty;
    return ret;
}

/**
 * @brief Whether two route graph segments match.
 *
 * Two segments match if both start and end at the exact same points. Other points are not considered.
 *
 * @param s1 The first segment
 * @param s2 The second segment
 * @return true if both segments match, false if not
 */
static int route_graph_segment_match(struct route_graph_segment *s1, struct route_graph_segment *s2) {
    if (!s1 || !s2)
        return 0;
    return (s1->start->c.x == s2->start->c.x && s1->start->c.y == s2->start->c.y &&
            s1->end->c.x == s2->end->c.x && s1->end->c.y == s2->end->c.y);
}

/**
 * @brief Adds two route values with protection against integer overflows.
 *
 * Unlike regular addition, this function is safe to use if one of the two arguments is `INT_MAX`
 * (which Navit uses to express that a segment cannot be traversed or a point cannot be reached):
 * If any of the two arguments is `INT_MAX`, then `INT_MAX` is returned; else the sum of the two
 * arguments is returned.
 *
 * Note that this currently does not cover cases in which both arguments are less than `INT_MAX` but add
 * up to `val1 + val2 >= INT_MAX`. With Navit’s internal cost definition, `INT_MAX` (2^31) is equivalent
 * to approximately 7 years, making this unlikely to become a real issue.
 */
static int route_value_add(int val1, int val2) {
    if (val1 == INT_MAX)
        return INT_MAX;
    if (val2 == INT_MAX)
        return INT_MAX;
    return val1 + val2;
}

/**
 * @brief Updates the lookahead value of a point in the route graph and updates its heap membership.
 *
 * This recalculates the lookahead value (the `rhs` member) of point `p`, based on the `value` of each neighbor and the
 * cost to reach that neighbor. If the resulting `p->rhs` differs from `p->value`, `p` is inserted into `heap` using
 * the lower of the two as its key (if `p` is already a member of `heap`, its key is changed accordingly). If the
 * resulting `p->rhs` is equal to `p->value` and `p` is a member of `heap`, it is removed.
 *
 * This is part of a modified LPA* implementation.
 *
 * @param profile The vehicle profile to use for routing. This determines which ways are passable and how their costs
 * are calculated.
 * @param p The point to evaluate
 * @param heap The heap
 */
static void route_graph_point_update(struct vehicleprofile *profile, struct route_graph_point * p,
                                     struct fibheap * heap) {
    struct route_graph_segment *s = NULL;
    int new, val;

    p->rhs = p->dst_val;
    p->seg = p->dst_seg;

    for (s = p->start; s; s = s->start_next) { /* Iterate over all the segments leading away from our point */
        val = route_value_seg(profile, s->end, s, 1);
        if (val != INT_MAX && s->end->seg && item_is_equal(s->data.item, s->end->seg->data.item)) {
            if (profile->turn_around_penalty2)
                val += profile->turn_around_penalty2;
            else
                val = INT_MAX;
        }
        if (val != INT_MAX) {
            new = route_value_add(val, s->end->value);
            if (new < p->rhs) {
                p->rhs = new;
                p->seg = s;
            }
        }
    }

    for (s = p->end; s; s = s->end_next) { /* Iterate over all the segments leading towards our point */
        val = route_value_seg(profile, s->start, s, -1);
        if (val != INT_MAX && s->start->seg && item_is_equal(s->data.item, s->start->seg->data.item)) {
            if (profile->turn_around_penalty2)
                val += profile->turn_around_penalty2;
            else
                val = INT_MAX;
        }
        if (val != INT_MAX) {
            new = route_value_add(val, s->start->value);
            if (new < p->rhs) {
                p->rhs = new;
                p->seg = s;
            }
        }
    }

    if (p->el) {
        /* Due to a limitation of the Fibonacci heap implementation, which causes fh_replacekey() to fail if the new
         * key is greater than the current one, we always remove the point from the heap (and, if locally inconsistent,
         * re-add it afterwards). */
        fh_delete(heap, p->el);
        p->el = NULL;
    }

    if (p->rhs != p->value)
        /* The point is locally inconsistent, add (or re-add) it to the heap */
        p->el = fh_insertkey(heap, MIN(p->rhs, p->value), p);
}

/**
 * @brief Expands (i.e. calculates the costs for) the points on the route graph’s heap.
 *
 * This calculates the cost for every point on the route graph’s heap, as well as any neighbors affected by the cost
 * change, and sets the next segment.
 *
 * This is part of a modified LPA* implementation.
 *
 * @param graph The route graph
 * @param profile The vehicle profile to use for routing. This determines which ways are passable and how their costs
 * are calculated.
 * @param cb The callback function to call when flooding is complete (can be NULL)
 */
static void route_graph_compute_shortest_path(struct route_graph * graph, struct vehicleprofile * profile,
        struct callback *cb) {
    struct route_graph_point *p_min;
    struct route_graph_segment *s = NULL;

    while (!route_graph_is_path_computed(graph) && (p_min = fh_extractmin(graph->heap))) {
        p_min->el = NULL;
        if (p_min->value > p_min->rhs)
            /* cost has decreased, update point value */
            p_min->value = p_min->rhs;
        else {
            /* cost has increased, re-evaluate */
            p_min->value = INT_MAX;
            route_graph_point_update(profile, p_min, graph->heap);
        }

        /* in any case, update rhs of predecessors (nodes from which we can reach p_min via a single segment) */
        for (s = p_min->start; s; s = s->start_next)
            if ((s->start == s->end) || (s->data.item.type < route_item_first) || (s->data.item.type > route_item_last))
                continue;
            else if (route_value_seg(profile, NULL, s, -2) != INT_MAX)
                route_graph_point_update(profile, s->end, graph->heap);
        for (s = p_min->end; s; s = s->end_next)
            if ((s->start == s->end) || (s->data.item.type < route_item_first) || (s->data.item.type > route_item_last))
                continue;
            else if (route_value_seg(profile, NULL, s, 2) != INT_MAX)
                route_graph_point_update(profile, s->start, graph->heap);
    }
    if (cb)
        callback_call_0(cb);
}

/**
 * @brief Sets or clears a traffic distortion for a segment.
 *
 * This sets a delay (setting speed is not supported) or clears an existing traffic distortion.
 * Note that, although setting a speed is not supported, calling this function with a delay of 0
 * will also clear an existing speed constraint.
 *
 * @param this The route graph
 * @param seg The segment to which the traffic distortion applies
 * @param delay Delay in tenths of a second, or 0 to clear an existing traffic distortion
 */
static void route_graph_set_traffic_distortion(struct route_graph *this, struct route_graph_segment *seg, int delay) {
    struct route_graph_point *start=NULL;
    struct route_graph_segment *s;

    while ((start=route_graph_get_point_next(this, &seg->start->c, start))) {
        s=start->start;
        while (s) {
            if (route_graph_segment_match(s, seg)) {
                if (s->data.item.type != type_none && s->data.item.type != type_traffic_distortion && delay) {
                    struct route_graph_segment_data data;
                    struct item item;
                    memset(&data, 0, sizeof(data));
                    memset(&item, 0, sizeof(item));
                    item.type=type_traffic_distortion;
                    data.item=&item;
                    data.len=delay;
                    data.flags = seg->data.flags & AF_DISTORTIONMASK;
                    s->start->flags |= RP_TRAFFIC_DISTORTION;
                    s->end->flags |= RP_TRAFFIC_DISTORTION;
                    route_graph_add_segment(this, s->start, s->end, &data);
                } else if (s->data.item.type == type_traffic_distortion && !delay) {
                    s->data.item.type = type_none;
                }
            }
            s=s->start_next;
        }
    }
}

/**
 * @brief Adds a traffic distortion item to the route graph
 *
 * @param this The route graph to add to
 * @param profile The vehicle profile to use for cost calculations
 * @param item The item to add, must be of {@code type_traffic_distortion}
 * @param update Whether to update the point (true for LPA*, false for Dijkstra)
 */
static void route_graph_add_traffic_distortion(struct route_graph *this, struct vehicleprofile *profile,
        struct item *item, int update) {
    struct route_graph_point *s_pnt,*e_pnt;
    struct coord c,l;
    struct attr flags_attr, delay_attr, maxspeed_attr;
    struct route_graph_segment_data data;

    data.item=item;
    data.len=0;
    data.offset=1;
    data.maxspeed = INT_MAX;

    item_attr_rewind(item);
    if (item_attr_get(item, attr_flags, &flags_attr))
        data.flags = flags_attr.u.num & AF_DISTORTIONMASK;
    else
        data.flags = 0;

    item_coord_rewind(item);
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
        if (update) {
            if (!(data.flags & AF_ONEWAYREV))
                route_graph_point_update(profile, s_pnt, this->heap);
            if (!(data.flags & AF_ONEWAY))
                route_graph_point_update(profile, e_pnt, this->heap);
        }
    }
}

/**
 * @brief Removes a traffic distortion item from the route graph
 *
 * Removing a traffic distortion which is not in the graph is a no-op.
 *
 * @param this The route graph to remove from
 * @param profile The vehicle profile to use for cost calculations
 * @param item The item to remove, must be of {@code type_traffic_distortion}
 */
static void route_graph_remove_traffic_distortion(struct route_graph *this, struct vehicleprofile *profile,
        struct item *item) {
    struct route_graph_point *s_pnt = NULL, *e_pnt = NULL;
    struct coord c, l;
    struct route_graph_segment *curr;

    item_coord_rewind(item);
    if (item_coord_get(item, &l, 1)) {
        s_pnt = route_graph_get_point(this, &l);
        while (item_coord_get(item, &c, 1))
            l = c;
        e_pnt = route_graph_get_point(this, &l);
    }
    if (s_pnt && e_pnt) {
#if 1
        curr = s_pnt->start;
        s_pnt->flags &= ~RP_TRAFFIC_DISTORTION;
        for (curr = s_pnt->start; curr; curr = curr->start_next) {
            if ((curr->end == e_pnt) && item_is_equal(curr->data.item, *item))
                curr->data.item.type = type_none;
            else if (curr->data.item.type == type_traffic_distortion)
                s_pnt->flags |= RP_TRAFFIC_DISTORTION;
        }

        e_pnt->flags &= ~RP_TRAFFIC_DISTORTION;
        for (curr = e_pnt->end; curr; curr = curr->end_next)
            if (curr->data.item.type == type_traffic_distortion)
                e_pnt->flags |= RP_TRAFFIC_DISTORTION;
#else
        struct route_graph_segment *found = NULL, *prev;
        /* this frees up memory but is slower */
        /* remove from global list */
        curr = this->route_segments;
        prev = NULL;
        while (curr && !found) {
            if ((curr->start == s_pnt) && (curr->end == e_pnt) && (curr->data.item == item)) {
                if (prev)
                    prev->next = curr->next;
                else
                    this->route_segments = curr->next;
                found = curr;
            } else {
                prev = curr;
                curr = prev->next;
            }
        }

        if (!found)
            return;

        /* remove from s_pnt list */
        curr = s_pnt->start;
        prev = NULL;
        s_pnt->flags &= ~RP_TRAFFIC_DISTORTION;
        while (curr) {
            if (curr == found) {
                if (prev)
                    prev->start_next = curr->start_next;
                else
                    s_pnt->start = curr->start_next;
            } else {
                if (curr->data.item.type == type_traffic_distortion)
                    s_pnt->flags |= RP_TRAFFIC_DISTORTION;
                prev = curr;
            }
            curr = prev->start_next;
        }

        /* remove from e_pnt list */
        curr = e_pnt->end;
        prev = NULL;
        e_pnt->flags &= ~RP_TRAFFIC_DISTORTION;
        while (curr) {
            if (curr == found) {
                if (prev)
                    prev->end_next = curr->end_next;
                else
                    s_pnt->end = curr->end_next;
            } else {
                if (curr->data.item.type == type_traffic_distortion)
                    e_pnt->flags |= RP_TRAFFIC_DISTORTION;
                prev = curr;
            }
            curr = prev->end_next;
        }

        size = sizeof(struct route_graph_segment) - sizeof(struct route_segment_data)
               + route_segment_data_size(found->data.flags);
        g_slice_free1(size, found);
#endif

        /* TODO figure out if we need to update both points */
        route_graph_point_update(profile, s_pnt, this->heap);
        route_graph_point_update(profile, e_pnt, this->heap);
    }
}

/**
 * @brief Changes a traffic distortion item in the route graph
 *
 * Attempting to change an idem which is not in the route graph will add it.
 *
 * @param this The route graph to change
 * @param profile The vehicle profile to use for cost calculations
 * @param item The item to change, must be of {@code type_traffic_distortion}
 */
static void route_graph_change_traffic_distortion(struct route_graph *this, struct vehicleprofile *profile,
        struct item *item) {
    /* TODO is there a more elegant way of doing this? */
    route_graph_remove_traffic_distortion(this, profile, item);
    route_graph_add_traffic_distortion(this, profile, item, 1);
}

/**
 * @brief Adds a turn restriction item to the route graph
 *
 * @param this The route graph to add to
 * @param item The item to add, must be of `type_street_turn_restriction_no` or `type_street_turn_restriction_only`
 */
void route_graph_add_turn_restriction(struct route_graph *this, struct item *item) {
    struct route_graph_point *pnt[4];
    struct coord c[5];
    int i,count;
    struct route_graph_segment_data data;

    item_coord_rewind(item);
    count=item_coord_get(item, c, 5);
    if (count != 3 && count != 4) {
        dbg(lvl_debug,"wrong count %d",count);
        return;
    }
    if (count == 4)
        return;
    for (i = 0 ; i < count ; i++)
        pnt[i]=route_graph_add_point(this,&c[i]);
    dbg(lvl_debug,"%s: (0x%x,0x%x)-(0x%x,0x%x)-(0x%x,0x%x) %p-%p-%p",item_to_name(item->type),c[0].x,c[0].y,c[1].x,c[1].y,
        c[2].x,c[2].y,pnt[0],pnt[1],pnt[2]);
    data.item=item;
    data.flags=0;
    data.len=0;
    data.score = 0;
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
static void route_graph_add_street(struct route_graph *this, struct item *item, struct vehicleprofile *profile) {
#ifdef AVOID_FLOAT
    int len=0;
#else
    double len=0;
#endif
    int segmented = 0;
    struct roadprofile *roadp;
    int default_flags_value = AF_ALL;
    int *default_flags;
    struct route_graph_point *s_pnt,*e_pnt; /* Start and end point */
    struct coord c,l; /* Current and previous point */
    struct attr attr;
    struct route_graph_segment_data data;
    data.flags=0;
    data.offset=1;
    data.maxspeed=-1;
    data.item=item;

    roadp = vehicleprofile_get_roadprofile(profile, item->type);
    if (!roadp) {
        /* Don't include any roads that don't have a road profile in our vehicle profile */
        return;
    }

    item_coord_rewind(item);
    if (item_coord_get(item, &l, 1)) {
        if (!(default_flags = item_get_default_flags(item->type)))
            default_flags = &default_flags_value;
        if (item_attr_get(item, attr_flags, &attr)) {
            data.flags = attr.u.num;
            segmented = (data.flags & AF_SEGMENTED);
        } else
            data.flags = *default_flags;

        if ((data.flags & AF_SPEED_LIMIT) && (item_attr_get(item, attr_maxspeed, &attr)))
            data.maxspeed = attr.u.num;
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

/**
 * @brief Gets the next route_graph_segment belonging to the specified street
 *
 * @param graph The route graph in which to search
 * @param sd The street to search for
 * @param last The last route graph segment returned to iterate over multiple segments of the same
 * item. If {@code NULL}, the first matching segment will be returned.
 *
 * @return The route graph segment, or {@code NULL} if none was found.
 */
static struct route_graph_segment *route_graph_get_segment(struct route_graph *graph, struct street_data *sd,
        struct route_graph_segment *last) {
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
 * @brief Whether cost (re)calculation of route graph points has reached the start point
 *
 * This method serves as the exit criterion for cost calculation in our LPA* implementation. When it returns true, it
 * means that calculation of node cost has proceeded far enough to determine the cost of, and cheapest path from, the
 * start point.
 *
 * The current implementation returns true only when the heap is empty, i.e. all points have been calculated. This is
 * not optimal in terms of efficiency, as the cost of the start point and the cheapest path from there no longer
 * change during the last few cycles. Future versions may report true before the heap is completely empty, as soon as
 * the cost of the start point and the cheapest path are final. However, this needs to be considered for recalculations
 * which happen when the vehicle leaves the cheapest path: right now, any point in the route graph has its final cost
 * and cheapest path, thus no recalculation is needed if the vehicle leaves the cheapest path. In the future, however,
 * a (partial) recalculation may be needed if the vehicle deviates from the cheapest path.
 *
 * @param this_ The route graph
 *
 * @return true if calculation is complete, false if not
 */
static int route_graph_is_path_computed(struct route_graph *this_) {
    /* TODO refine exit criterion */
    if (!fh_min(this_->heap))
        return 1;
    else
        return 0;
}

/**
 * @brief Triggers partial recalculation of the route, based on the existing route graph.
 *
 * This is currently used when traffic distortions have been added, changed or removed. Future versions may also use
 * it if the current position has changed to a portion of the route graph which has not been flooded (which is
 * currently not necessary because the route graph is always flooded completely).
 *
 * This tends to be faster than full recalculation, as only a subset of all points in the graph needs to be evaluated.
 *
 * If segment costs have changed (as is the case with traffic distortions), all affected segments must have been added
 * to, removed from or updated in the route graph before this method is called.
 *
 * After recalculation, the route path is updated.
 *
 * The function uses a modified LPA* algorithm for recalculations. Most modifications were made for compatibility with
 * the algorithm used for the initial routing:
 * \li The `value` of a node represents the cost to reach the destination and thus decreases along the route
 * (eliminating the need for recalculations as the vehicle moves within the route graph)
 * \li The heuristic is always assumed to be zero (which would turn A* into Dijkstra, the basis of the main routing
 * algorithm, and makes our keys one-dimensional)
 * \li Currently, each pass evaluates all locally inconsistent points, leaving an empty heap at the end (though this
 * may change in the future).
 *
 * @param this_ The route
 */
/* TODO This is absolutely not thread-safe and will wreak havoc if run concurrently with route_graph_flood(). This is
 * not an issue as long as the two never overlap: Currently both this function and route_graph_flood() run without
 * interruption until they finish, and are both on the main thread. If that changes, we need to revisit this. */
void route_recalculate_partial(struct route *this_) {
    struct attr route_status;

    /* do nothing if we don’t have a route graph */
    if (!route_has_graph(this_))
        return;

    /* if the route graph is still being built, it will be calculated from scratch after that, nothing to do here */
    if (this_->graph->busy)
        return;

    /* exit if there is no need to recalculate */
    if (route_graph_is_path_computed(this_->graph))
        return;

    route_status.type = attr_route_status;

    route_status.u.num = route_status_building_graph;
    route_set_attr(this_, &route_status);

    printf("Expanding points which have changed\n");

    route_graph_compute_shortest_path(this_->graph, this_->vehicleprofile, NULL);

    printf("Point expansion complete, recalculating route path\n");

    route_path_update_done(this_, 0);
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
static struct route_path *route_path_new_offroad(struct route_graph *this, struct route_info *pos,
        struct route_info *dst) {
    struct route_path *ret;

    ret=g_new0(struct route_path, 1);
    ret->in_use=1;
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
route_get_coord_dist(struct route *this_, int dist) {
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
static struct route_path *route_path_new(struct route_graph *this, struct route_path *oldpath, struct route_info *pos,
        struct route_info *dst,
        struct vehicleprofile *profile) {
    struct route_graph_segment *s=NULL,*s1=NULL,*s2=NULL; /* candidate segments for cheapest path */
    struct route_graph_point *start; /* point at which the next segment starts, i.e. up to which the path is complete */
    struct route_info *posinfo, *dstinfo; /* same as pos and dst, but NULL if not part of current segment */
    int segs=0,dir; /* number of segments added to graph, direction of first segment */
    int val1=INT_MAX,val2=INT_MAX; /* total cost for s1 and s2, respectively */
    int val,val1_new,val2_new;
    struct route_path *ret;

    if (! pos->street || ! dst->street) {
        dbg(lvl_error,"pos or dest not set");
        return NULL;
    }

    if (profile->mode == 2 || (profile->mode == 0
                               && pos->lenextra + dst->lenextra > transform_distance(map_projection(pos->street->item.map), &pos->c, &dst->c)))
        return route_path_new_offroad(this, pos, dst);
    while ((s=route_graph_get_segment(this, pos->street, s))) {
        val=route_value_seg(profile, NULL, s, 2);
        if (val != INT_MAX && s->end->value != INT_MAX) {
            val=val*(100-pos->percent)/100;
            dbg(lvl_debug,"val1 %d",val);
            if (route_graph_segment_match(s,this->avoid_seg) && pos->street_direction < 0)
                val+=profile->turn_around_penalty;
            dbg(lvl_debug,"val1 %d",val);
            val1_new=s->end->value+val;
            dbg(lvl_debug,"val1 +%d=%d",s->end->value,val1_new);
            if (val1_new < val1) {
                val1=val1_new;
                s1=s;
            }
        }
        val=route_value_seg(profile, NULL, s, -2);
        if (val != INT_MAX && s->start->value != INT_MAX) {
            val=val*pos->percent/100;
            dbg(lvl_debug,"val2 %d",val);
            if (route_graph_segment_match(s,this->avoid_seg) && pos->street_direction > 0)
                val+=profile->turn_around_penalty;
            dbg(lvl_debug,"val2 %d",val);
            val2_new=s->start->value+val;
            dbg(lvl_debug,"val2 +%d=%d",s->start->value,val2_new);
            if (val2_new < val2) {
                val2=val2_new;
                s2=s;
            }
        }
    }
    if (val1 == INT_MAX && val2 == INT_MAX) {
        dbg(lvl_error,"no route found, pos blocked");
        return NULL;
    }
    if (val1 == val2) {
        val1=s1->end->value;
        val2=s2->start->value;
    }
    if (val1 < val2) {
        start=s1->start;
        s=s1;
        dir=1;
    } else {
        start=s2->end;
        s=s2;
        dir=-1;
    }
    if (pos->street_direction && dir != pos->street_direction && profile->turn_around_penalty) {
        if (!route_graph_segment_match(this->avoid_seg,s)) {
            dbg(lvl_debug,"avoid current segment");
            if (this->avoid_seg)
                route_graph_set_traffic_distortion(this, this->avoid_seg, 0);
            this->avoid_seg=s;
            route_graph_set_traffic_distortion(this, this->avoid_seg, profile->turn_around_penalty);
            route_graph_reset(this);
            route_graph_init(this, dst, profile);
            route_graph_compute_shortest_path(this, profile, NULL);
            return route_path_new(this, oldpath, pos, dst, profile);
        }
    }
    ret=g_new0(struct route_path, 1);
    ret->in_use=1;
    ret->updated=1;
    if (pos->lenextra)
        route_path_add_line(ret, &pos->c, &pos->lp, pos->lenextra);
    ret->path_hash=item_hash_new();
    dstinfo=NULL;
    posinfo=pos;
    while (s && !dstinfo) { /* following start->seg, which indicates the least costly way to reach our destination */
        segs++;
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
    dbg(lvl_debug, "%d segments", segs);
    return ret;
}

static int route_graph_build_next_map(struct route_graph *rg) {
    do {
        rg->m=mapset_next(rg->h, 2);
        if (! rg->m)
            return 0;
        map_rect_destroy(rg->mr);
        rg->mr=map_rect_new(rg->m, rg->sel);
    } while (!rg->mr);

    return 1;
}


static int is_turn_allowed(struct route_graph_point *p, struct route_graph_segment *from,
                           struct route_graph_segment *to) {
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
            dbg(lvl_debug,"found %s (0x%x,0x%x) (0x%x,0x%x)-(0x%x,0x%x) %p-%p",item_to_name(tmp1->data.item.type),
                tmp1->data.item.id_hi,tmp1->data.item.id_lo,tmp1->start->c.x,tmp1->start->c.y,tmp1->end->c.x,tmp1->end->c.y,tmp1->start,
                tmp1->end);
            while (tmp2) {
                dbg(lvl_debug,"compare %s (0x%x,0x%x) (0x%x,0x%x)-(0x%x,0x%x) %p-%p",item_to_name(tmp2->data.item.type),
                    tmp2->data.item.id_hi,tmp2->data.item.id_lo,tmp2->start->c.x,tmp2->start->c.y,tmp2->end->c.x,tmp2->end->c.y,tmp2->start,
                    tmp2->end);
                if (item_is_equal(tmp1->data.item, tmp2->data.item))
                    break;
                tmp2=tmp2->start_next;
            }
            dbg(lvl_debug,"tmp2=%p",tmp2);
            if (tmp2) {
                dbg(lvl_debug,"%s tmp2->end=%p next=%p",item_to_name(tmp1->data.item.type),tmp2->end,next);
            }
            if (tmp1->data.item.type == type_street_turn_restriction_no && tmp2 && tmp2->end->c.x == next->c.x
                    && tmp2->end->c.y == next->c.y) {
                dbg(lvl_debug,"from 0x%x,0x%x over 0x%x,0x%x to 0x%x,0x%x not allowed (no)",prev->c.x,prev->c.y,p->c.x,p->c.y,next->c.x,
                    next->c.y);
                return 0;
            }
            if (tmp1->data.item.type == type_street_turn_restriction_only && tmp2 && (tmp2->end->c.x != next->c.x
                    || tmp2->end->c.y != next->c.y)) {
                dbg(lvl_debug,"from 0x%x,0x%x over 0x%x,0x%x to 0x%x,0x%x not allowed (only)",prev->c.x,prev->c.y,p->c.x,p->c.y,
                    next->c.x,next->c.y);
                return 0;
            }
        }
        tmp1=tmp1->end_next;
    }
    dbg(lvl_debug,"from 0x%x,0x%x over 0x%x,0x%x to 0x%x,0x%x allowed",prev->c.x,prev->c.y,p->c.x,p->c.y,next->c.x,
        next->c.y);
    return 1;
}

static void route_graph_clone_segment(struct route_graph *this, struct route_graph_segment *s,
                                      struct route_graph_point *start,
                                      struct route_graph_point *end, int flags) {
    struct route_graph_segment_data data;
    data.item=&s->data.item;
    data.offset=1;
    data.flags=s->data.flags|flags;
    data.len=s->data.len+1;
    data.maxspeed=-1;
    data.dangerous_goods=0;
    data.score = s->data.score;
    if (s->data.flags & AF_SPEED_LIMIT)
        data.maxspeed=RSD_MAXSPEED(&s->data);
    if (s->data.flags & AF_SEGMENTED)
        data.offset=RSD_OFFSET(&s->data);
    dbg(lvl_debug,"cloning segment from %p (0x%x,0x%x) to %p (0x%x,0x%x)",start,start->c.x,start->c.y, end, end->c.x,
        end->c.y);
    route_graph_add_segment(this, start, end, &data);
}

static void route_graph_process_restriction_segment(struct route_graph *this, struct route_graph_point *p,
        struct route_graph_segment *s, int dir) {
    struct route_graph_segment *tmp;
    struct route_graph_point *pn;
    struct coord c=p->c;
    int dx=0;
    int dy=0;
    c.x+=dx;
    c.y+=dy;
    dbg(lvl_debug,"From %s %d,%d",item_to_name(s->data.item.type),dx,dy);
    pn=route_graph_point_new(this, &c);
    if (dir > 0) { /* going away */
        dbg(lvl_debug,"other 0x%x,0x%x",s->end->c.x,s->end->c.y);
        if (s->data.flags & AF_ONEWAY) {
            dbg(lvl_debug,"Not possible");
            return;
        }
        route_graph_clone_segment(this, s, pn, s->end, AF_ONEWAYREV);
    } else { /* coming in */
        dbg(lvl_debug,"other 0x%x,0x%x",s->start->c.x,s->start->c.y);
        if (s->data.flags & AF_ONEWAYREV) {
            dbg(lvl_debug,"Not possible");
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
            dbg(lvl_debug,"To start %s",item_to_name(tmp->data.item.type));
        }
        tmp=tmp->start_next;
    }
    tmp=p->end;
    while (tmp) {
        if (tmp != s && tmp->data.item.type != type_street_turn_restriction_no &&
                tmp->data.item.type != type_street_turn_restriction_only &&
                !(tmp->data.flags & AF_ONEWAY) && is_turn_allowed(p, s, tmp)) {
            route_graph_clone_segment(this, tmp, tmp->start, pn, AF_ONEWAYREV);
            dbg(lvl_debug,"To end %s",item_to_name(tmp->data.item.type));
        }
        tmp=tmp->end_next;
    }
}

static void route_graph_process_restriction_point(struct route_graph *this, struct route_graph_point *p) {
    struct route_graph_segment *tmp;
    tmp=p->start;
    dbg(lvl_debug,"node 0x%x,0x%x",p->c.x,p->c.y);
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

static void route_graph_process_restrictions(struct route_graph *this) {
    struct route_graph_point *curr;
    int i;
    dbg(lvl_debug,"enter");
    for (i = 0 ; i < HASH_SIZE ; i++) {
        curr=this->hash[i];
        while (curr) {
            if (curr->flags & RP_TURN_RESTRICTION)
                route_graph_process_restriction_point(this, curr);
            curr=curr->hash_next;
        }
    }
}

/**
 * @brief Releases all resources needed to build the route graph.
 *
 * If `cancel` is false, this function will start processing restrictions and ultimately call the route
 * graph's `done_cb` callback.
 *
 * The traffic module will always call this method with `cancel` set to true, as it does not process
 * restrictions and has no callback. Inside the routing module, `cancel` will be true if, and only if,
 * navigation has been aborted.
 *
 * @param rg Points to the route graph
 * @param cancel True if the process was aborted before completing, false if it completed normally
 */
void route_graph_build_done(struct route_graph *rg, int cancel) {
    dbg(lvl_debug,"cancel=%d",cancel);
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
        if (rg->done_cb)
            callback_call_0(rg->done_cb);
    }
    rg->busy=0;
}

static void route_graph_build_idle(struct route_graph *rg, struct vehicleprofile *profile) {
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
            route_graph_add_traffic_distortion(rg, profile, item, 0);
        else if (item->type == type_street_turn_restriction_no || item->type == type_street_turn_restriction_only)
            route_graph_add_turn_restriction(rg, item);
        else
            route_graph_add_street(rg, item, profile);
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
 * @param c The coordinates of the destination or next waypoint
 * @param c1 Corner 1 of the rectangle to use from the map
 * @param c2 Corner 2 of the rectangle to use from the map
 * @param done_cb The callback which will be called when graph is complete
 * @return The new route graph.
 */
// FIXME documentation does not match argument list
static struct route_graph *route_graph_build(struct mapset *ms, struct coord *c, int count, struct callback *done_cb,
        int async,
        struct vehicleprofile *profile) {
    struct route_graph *ret=g_new0(struct route_graph, 1);

    dbg(lvl_debug,"enter");

    ret->sel=route_calc_selection(c, count, profile);
    ret->h=mapset_open(ms);
    ret->done_cb=done_cb;
    ret->busy=1;
    ret->heap = fh_makekeyheap();
    if (route_graph_build_next_map(ret)) {
        if (async) {
            ret->idle_cb=callback_new_2(callback_cast(route_graph_build_idle), ret, profile);
            ret->idle_ev=event_add_idle(50, ret->idle_cb);
        }
    } else
        route_graph_build_done(ret, 0);

    return ret;
}

static void route_graph_update_done(struct route *this, struct callback *cb) {
    route_graph_init(this->graph, this->current_dst, this->vehicleprofile);
    route_graph_compute_shortest_path(this->graph, this->vehicleprofile, cb);
}

/**
 * @brief Updates the route graph
 *
 * This updates the route graph after settings in the route have changed. It also
 * adds routing information afterwards by calling route_graph_flood().
 *
 * @param this The route to update the graph for
 * @param cb The callback function to call when the route graph update is complete (used only in asynchronous mode)
 * @param async Set to nonzero in order to update the route graph asynchronously
 */
static void route_graph_update(struct route *this, struct callback *cb, int async) {
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
street_get_data (struct item *item) {
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
street_data_dup(struct street_data *orig) {
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
void street_data_free(struct street_data *sd) {
    g_free(sd);
}

/**
 * @brief Finds the nearest street to a given coordinate
 *
 * @param ms The mapset to search in for the street
 * @param pc The coordinate to find a street nearby
 * @return The nearest street
 */
static struct route_info *route_find_nearest_street(struct vehicleprofile *vehicleprofile, struct mapset *ms,
        struct pcoord *pc) {
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

    if(!vehicleprofile)
        return NULL;

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
                    dbg(lvl_debug,"dist=%d id 0x%x 0x%x pos=%d", dist, item->id_hi, item->id_lo, pos);
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
            dbg(lvl_debug,"Much too far %d > %d", mindist, max_dist);
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
void route_info_free(struct route_info *inf) {
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
route_info_street(struct route_info *rinf) {
    return rinf->street;
}


/**
 * @brief Implementation-specific map rect data
 */
struct map_rect_priv {
    struct route_info_handle *ri;
    enum attr_type attr_next;
    int pos;
    struct map_priv *mpriv;     /**< The map to which this map rect refers */
    struct item item;           /**< The current item, i.e. the last item returned by the `map_rect_get_item` method */
    unsigned int last_coord;
    struct route_path *path;
    struct route_path_segment *seg,*seg_next;
    struct route_graph_point *point;
    struct route_graph_segment *rseg;
    char *str;
    int hash_bucket;
    struct coord *coord_sel;	/**< Set this to a coordinate if you want to filter for just a single route graph point */
    struct route_graph_point_iterator it;
    /* Pointer to current waypoint element of route->destinations */
    GList *dest;
};

static void rm_coord_rewind(void *priv_data) {
    struct map_rect_priv *mr = priv_data;
    mr->last_coord = 0;
}

static void rm_attr_rewind(void *priv_data) {
    struct map_rect_priv *mr = priv_data;
    mr->attr_next = attr_street_item;
}

static int rm_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr) {
    struct map_rect_priv *mr = priv_data;
    struct route_path_segment *seg=mr->seg;
    struct route *route=mr->mpriv->route;
    if (mr->item.type != type_street_route && mr->item.type != type_waypoint && mr->item.type != type_route_end)
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
        if (seg && (seg->data->flags & AF_SPEED_LIMIT)) {
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
        /* TODO This ignores access flags on traffic distortions, but the attribute does not seem
         * to be used anywhere */
        mr->attr_next=attr_speed;
        if (seg)
            attr->u.num=route_time_seg(route->vehicleprofile, seg->data, NULL);
        else
            return 0;
        return 1;
    case attr_speed:
        /* TODO This ignores access flags on traffic distortions, but the attribute does not seem
         * to be used anywhere */
        mr->attr_next=attr_label;
        if (seg)
            attr->u.num=route_seg_speed(route->vehicleprofile, seg->data, NULL);
        else
            return 0;
        return 1;
    case attr_label:
        mr->attr_next=attr_none;
        if(mr->item.type==type_waypoint || mr->item.type == type_route_end) {
            if(mr->str)
                g_free(mr->str);
            /* Build the text displayed close to the destination cursor.
             * It will contain the sequence number of the waypoint (1, 2...) */
            mr->str=g_strdup_printf("%d",route->reached_destinations_count+g_list_position(route->destinations,mr->dest)+1);
            attr->u.str=mr->str;
            return 1;
        }
        return 0;
    default:
        mr->attr_next=attr_none;
        attr->type=attr_none;
        return 0;
    }
    return 0;
}

static int rm_coord_get(void *priv_data, struct coord *c, int count) {
    struct map_rect_priv *mr = priv_data;
    struct route_path_segment *seg = mr->seg;
    int i,rc=0;
    struct route *r = mr->mpriv->route;
    enum projection pro = route_projection(r);

    if (pro == projection_none)
        return 0;
    if (mr->item.type == type_route_start || mr->item.type == type_route_start_reverse || mr->item.type == type_route_end
            || mr->item.type == type_waypoint ) {
        if (! count || mr->last_coord)
            return 0;
        mr->last_coord=1;
        if (mr->item.type == type_route_start || mr->item.type == type_route_start_reverse)
            c[0]=r->pos->c;
        else if (mr->item.type == type_waypoint) {
            c[0]=((struct route_info *)mr->dest->data)->c;
        } else { /*type_route_end*/
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
    dbg(lvl_debug,"return %d",rc);
    return rc;
}

static struct item_methods methods_route_item = {
    rm_coord_rewind,
    rm_coord_get,
    rm_attr_rewind,
    rm_attr_get,
};

static void rp_attr_rewind(void *priv_data) {
    struct map_rect_priv *mr = priv_data;
    mr->attr_next = attr_label;
}

static int rp_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr) {
    struct map_rect_priv *mr = priv_data;
    struct route_graph_point *p = mr->point;
    struct route_graph_segment *seg = mr->rseg;
    struct route *route=mr->mpriv->route;

    attr->type=attr_type;
    switch (attr_type) {
    case attr_any: // works only with rg_points for now
        while (mr->attr_next != attr_none) {
            dbg(lvl_debug,"querying %s", attr_to_name(mr->attr_next));
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
        attr->type = attr_label;
        if (mr->str)
            g_free(mr->str);
        if (mr->item.type == type_rg_point) {
            if (p->value != INT_MAX)
                mr->str=g_strdup_printf("%d", p->value);
            else
                mr->str=g_strdup("-");
        } else {
            int len=seg->data.len;
            int speed=route_seg_speed(route->vehicleprofile, &seg->data, NULL);
            int time=route_time_seg(route->vehicleprofile, &seg->data, NULL);
            if (speed)
                mr->str=g_strdup_printf("%dm %dkm/h %d.%ds",len,speed,time/10,time%10);
            else if (len)
                mr->str=g_strdup_printf("%dm",len);
            else {
                mr->str=NULL;
                return 0;
            }
        }
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
        mr->str=NULL;
        switch (mr->item.type) {
        case type_rg_point: {
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
            mr->str=g_strdup_printf("len %d time %d start %p end %p",seg->data.len, route_time_seg(route->vehicleprofile,
                                    &seg->data, NULL), seg->start, seg->end);
            attr->u.str = mr->str;
            return 1;
            break;
        default:
            return 0;
        }
        break;
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
static int rp_coord_get(void *priv_data, struct coord *c, int count) {
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

static void rp_destroy(struct map_priv *priv) {
    g_free(priv);
}

static void rm_destroy(struct map_priv *priv) {
    g_free(priv);
}

static struct map_rect_priv *rm_rect_new(struct map_priv *priv, struct map_selection *sel) {
    struct map_rect_priv * mr;
    dbg(lvl_debug,"enter");
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
static struct map_rect_priv *rp_rect_new(struct map_priv *priv, struct map_selection *sel) {
    struct map_rect_priv * mr;

    dbg(lvl_debug,"enter");
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

static void rm_rect_destroy(struct map_rect_priv *mr) {
    if (mr->str)
        g_free(mr->str);
    if (mr->coord_sel) {
        g_free(mr->coord_sel);
    }
    if (mr->path) {
        mr->path->in_use--;
        if (mr->path->update_required && (mr->path->in_use==1)
                && (mr->mpriv->route->route_status & ~route_status_destination_set))
            route_path_update_done(mr->mpriv->route, mr->path->update_required-1);
        else if (!mr->path->in_use)
            g_free(mr->path);
    }

    g_free(mr);
}

static struct item *rp_get_item(struct map_rect_priv *mr) {
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
        if (!mr->point) { /* This means that no point has been found */
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

static struct item *rp_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo) {
    struct item *ret=NULL;
    while (id_lo-- > 0)
        ret=rp_get_item(mr);
    return ret;
}


static struct item *rm_get_item(struct map_rect_priv *mr) {
    struct route *route=mr->mpriv->route;
    void *id=0;

    switch (mr->item.type) {
    case type_none:
        if (route->pos && route->pos->street_direction && route->pos->street_direction != route->pos->dir)
            mr->item.type=type_route_start_reverse;
        else
            mr->item.type=type_route_start;
        if (route->pos) {
            id=route->pos;
            break;
        }
    /* FALLTHRU */

    case type_route_start:
    case type_route_start_reverse:
        mr->seg=NULL;
        mr->dest=mr->mpriv->route->destinations;
    /* FALLTHRU */
    default:
        if (mr->item.type == type_waypoint)
            mr->dest=g_list_next(mr->dest);
        mr->item.type=type_street_route;
        mr->seg=mr->seg_next;
        if (!mr->seg && mr->path && mr->path->next) {
            struct route_path *p=NULL;
            mr->path->in_use--;
            if (!mr->path->in_use)
                p=mr->path;
            mr->path=mr->path->next;
            mr->path->in_use++;
            mr->seg=mr->path->path;
            if (p)
                g_free(p);
            if (mr->dest) {
                id=mr->dest;
                mr->item.type=type_waypoint;
                mr->seg_next=mr->seg;
                break;
            }
        }
        if (mr->seg) {
            mr->seg_next=mr->seg->next;
            id=mr->seg;
            break;
        }
        if (mr->dest && g_list_next(mr->dest)) {
            id=mr->dest;
            mr->item.type=type_waypoint;
            break;
        }
        mr->item.type=type_route_end;
        id=&(mr->mpriv->route->destinations);
        if (mr->mpriv->route->destinations)
            break;
    /* FALLTHRU */
    case type_route_end:
        return NULL;
    }
    mr->last_coord = 0;
    item_id_from_ptr(&mr->item,id);
    rm_attr_rewind(mr);
    return &mr->item;
}

static struct item *rm_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo) {
    struct item *ret=NULL;
    do {
        ret=rm_get_item(mr);
    } while (ret && (ret->id_lo!=id_lo || ret->id_hi!=id_hi));
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

static struct map_priv *route_map_new_helper(struct map_methods *meth, struct attr **attrs, int graph) {
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

static struct map_priv *route_map_new(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl) {
    return route_map_new_helper(meth, attrs, 0);
}

static struct map_priv *route_graph_map_new(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl) {
    return route_map_new_helper(meth, attrs, 1);
}

static struct map *route_get_map_helper(struct route *this_, struct map **map, char *type, char *description) {
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

    if (! *map) {
        *map=map_new(NULL, attrs);
        navit_object_ref((struct navit_object *)*map);
    }

    return *map;
}

/**
 * @brief Adds a traffic distortion item to the route
 *
 * @param this_ The route
 * @param item The item to add, must be of {@code type_traffic_distortion}
 */
void route_add_traffic_distortion(struct route *this_, struct item *item) {
    if (route_has_graph(this_))
        route_graph_add_traffic_distortion(this_->graph, this_->vehicleprofile, item, 1);
}

/**
 * @brief Changes a traffic distortion item on the route
 *
 * Attempting to change an idem which is not in the route graph will add it.
 *
 * @param this_ The route
 * @param item The item to change, must be of {@code type_traffic_distortion}
 */
void route_change_traffic_distortion(struct route *this_, struct item *item) {
    if (route_has_graph(this_))
        route_graph_change_traffic_distortion(this_->graph, this_->vehicleprofile, item);
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
route_get_map(struct route *this_) {
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
route_get_graph_map(struct route *this_) {
    return route_get_map_helper(this_, &this_->graph_map, "route_graph","Route Graph");
}


/**
 * @brief Returns the flags for the route.
 */
enum route_path_flags route_get_flags(struct route *this_) {
    return this_->flags;
}

/**
 * @brief Retrieves the route graph.
 *
 * @return The route graph, or NULL if the route has no valid graph
 */
struct route_graph * route_get_graph(struct route *this_) {
    return this_->graph;
}

/**
 * @brief Whether the route has a valid graph.
 *
 * @return True if the route has a graph, false if not.
 */
int route_has_graph(struct route *this_) {
    return (this_->graph != NULL);
}

/**
 * @brief Removes a traffic distortion item from the route
 *
 * Removing a traffic distortion which is not in the route graph is a no-op.
 *
 * @param this_ The route
 * @param item The item to remove, must be of {@code type_traffic_distortion}
 */
void route_remove_traffic_distortion(struct route *this_, struct item *item) {
    if (route_has_graph(this_))
        route_graph_remove_traffic_distortion(this_->graph, this_->vehicleprofile, item);
}

void route_set_projection(struct route *this_, enum projection pro) {
}

int route_set_attr(struct route *this_, struct attr *attr) {
    int attr_updated=0;
    switch (attr->type) {
    case attr_route_status:
        attr_updated = (this_->route_status != attr->u.num);
        this_->route_status = attr->u.num;
        break;
    case attr_destination:
        route_set_destination(this_, attr->u.pcoord, 1);
        return 1;
    case attr_position:
        route_set_position_flags(this_, attr->u.pcoord, route_path_flag_async);
        return 1;
    case attr_position_test:
        return route_set_position_flags(this_, attr->u.pcoord, route_path_flag_no_rebuild);
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
        dbg(lvl_error,"unsupported attribute: %s",attr_to_name(attr->type));
        return 0;
    }
    if (attr_updated)
        callback_list_call_attr_2(this_->cbl2, attr->type, this_, attr);
    return 1;
}

int route_add_attr(struct route *this_, struct attr *attr) {
    switch (attr->type) {
    case attr_callback:
        callback_list_add(this_->cbl2, attr->u.callback);
        return 1;
    default:
        return 0;
    }
}

int route_remove_attr(struct route *this_, struct attr *attr) {
    dbg(lvl_debug,"enter");
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

int route_get_attr(struct route *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
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
        dbg(lvl_debug,"get vehicle %p",this_->v);
        break;
    case attr_vehicleprofile:
        attr->u.vehicleprofile=this_->vehicleprofile;
        ret=(this_->vehicleprofile != NULL);
        break;
    case attr_route_status:
        attr->u.num=this_->route_status;
        break;
    case attr_destination_time:
        if (this_->path2 && (this_->route_status == route_status_path_done_new
                             || this_->route_status == route_status_path_done_incremental)) {
            struct route_path *path=this_->path2;
            attr->u.num=0;
            while (path) {
                attr->u.num+=path->path_time;
                path=path->next;
            }
            dbg(lvl_debug,"path_time %ld",attr->u.num);
        } else
            ret=0;
        break;
    case attr_destination_length:
        if (this_->path2 && (this_->route_status == route_status_path_done_new
                             || this_->route_status == route_status_path_done_incremental)) {
            struct route_path *path=this_->path2;
            attr->u.num=0;
            while (path) {
                attr->u.num+=path->path_len;
                path=path->next;
            }
        } else
            ret=0;
        break;
    default:
        return 0;
    }
    attr->type=type;
    return ret;
}

struct attr_iter *
route_attr_iter_new(void) {
    return g_new0(struct attr_iter, 1);
}

void route_attr_iter_destroy(struct attr_iter *iter) {
    g_free(iter);
}

void route_init(void) {
    plugin_register_category_map("route", route_map_new);
    plugin_register_category_map("route_graph", route_graph_map_new);
}

void route_destroy(struct route *this_) {
    this_->refcount++; /* avoid recursion */
    route_path_destroy(this_->path2,1);
    route_graph_destroy(this_->graph);
    route_clear_destinations(this_);
    route_info_free(this_->pos);
    map_destroy(this_->map);
    map_destroy(this_->graph_map);
    g_free(this_);
}

struct object_func route_func = {
    attr_route,
    (object_func_new)route_new,
    (object_func_get_attr)route_get_attr,
    (object_func_iter_new)NULL,
    (object_func_iter_destroy)NULL,
    (object_func_set_attr)route_set_attr,
    (object_func_add_attr)route_add_attr,
    (object_func_remove_attr)route_remove_attr,
    (object_func_init)NULL,
    (object_func_destroy)route_destroy,
    (object_func_dup)route_dup,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};
