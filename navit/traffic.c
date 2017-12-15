/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2017 Navit Team
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
 * @brief Contains code related to processing traffic messages into map items.
 *
 * Currently the only map items supported are traffic distortions. More may be added in the future.
 *
 * Traffic distortions are used by Navit to route around traffic problems.
 */

#include <string.h>
#include <time.h>

#ifdef _POSIX_C_SOURCE
#include <sys/types.h>
#endif
#include "glib_slice.h"
#include "config.h"
#include "navit.h" // TODO see if we will still need that once we have a working map implementation
#include "util.h"
#include "coord.h"
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "route_protected.h"
#include "route.h"
#include "transform.h"
#include "xmlconfig.h"
#include "traffic.h"
#include "plugin.h"
#include "fib.h"
#include "event.h"
#include "callback.h"
#include "debug.h"

/** The penalty applied to an off-road link */
#define PENALTY_OFFROAD 2

/**
 * @brief A traffic plugin instance
 */
struct traffic {
	NAVIT_OBJECT
	struct navit *navit;         /**< The navit instance */
	struct traffic_priv *priv;   /**< Private data used by the plugin */
	struct traffic_methods meth; /**< Methods implemented by the plugin */
	struct callback * callback;  /**< The callback function for the idle loop */
	struct event_timeout * timeout; /**< The timeout event that triggers the loop function */
	struct mapset *ms;           /**< The mapset used for routing */
	struct route *rt;            /**< The route to notify of traffic changes */
};

/**
 * @brief Private data for the traffic map.
 */
struct map_priv {
	struct traffic_message ** messages; /**< Points to a list of pointers to all currently active messages */
	// TODO messages by ID, segments by message?
};

void tm_add_item(struct map_priv *priv, enum item_type type, int id_hi, int id_lo, struct attr **attrs,
		struct coord *c, int count);
void tm_destroy(struct map_priv *priv);
void traffic_loop(struct traffic * this_);
struct traffic * traffic_new(struct attr *parent, struct attr **attrs);
void traffic_message_dump(struct traffic_message * this_);

/**
 * @brief Adds an item to the map.
 *
 * Currently this method ignores the `priv` argument and simply writes the item to a textfile map named
 * `distortion.txt` in the default data folder. This map must be in the active mapset in order for the
 * distortions to be rendered on the map and considered for routing.
 *
 * All data passed to this method is safe to free after the method returns, and doing so is the
 * responsibility of the caller.
 *
 * @param priv The traffic map's private data
 * @param type Type of the item
 * @param id_hi First part of the ID of the item (item IDs have two parts)
 * @param id_lo Second part of the ID of the item
 * @param attrs The attributes for the item
 * @param c Points to an array of coordinates for the item
 * @param count Number of items in `c`
 */
void tm_add_item(struct map_priv *priv, enum item_type type, int id_hi, int id_lo, struct attr **attrs,
		struct coord *c, int count) {
	int i;
	char * attr_text;

	/* add the configuration directory to the name of the file to use */
	char *dist_filename = g_strjoin(NULL, navit_get_user_data_directory(TRUE),
									"/distortion.txt", NULL);
	if (dist_filename) {
		FILE *map = fopen(dist_filename,"a");
		if (map) {
			fprintf(map, "type=%s", item_to_name(type));
			while (*attrs) {
				attr_text = attr_to_text(*attrs, NULL, 0);
				/* FIXME this may not work properly for all attribute types */
				fprintf(map, " %s=%s", attr_to_name((*attrs)->type), attr_text);
				g_free(attr_text);
				attrs++;
			}
			fprintf(map, "\n");

			for (i = 0; i < count; i++) {
				fprintf(map,"0x%x 0x%x\n", c[i].x, c[i].y);
			}
			fclose(map);
		} else {
			dbg(lvl_error,"could not open file for distortions !!");

		} /* else - if (map) */
		g_free(dist_filename);			/* free the file name */
	} /* if (dist_filename) */
}

/**
 * @brief Destroys (closes) the traffic map.
 *
 * @param priv The private data for the traffic map instance
 */
void tm_destroy(struct map_priv *priv) {
	g_free(priv);
}

/**
 * @brief Opens a new map rectangle on the traffic map
 *
 * This function opens a new map rectangle on the route graph's map.
 *
 * @param priv The traffic graph map's private data
 * @param sel The map selection (to restrict search to a rectangle, order and/or item types)
 * @return A new map rect's private data
 */
static struct map_rect_priv * tm_rect_new(struct map_priv *priv, struct map_selection *sel) {
	// TODO
	return NULL;
}

/**
 * @brief Destroys a map rectangle on the traffic map
 */
static void tm_rect_destroy(struct map_rect_priv *mr) {
	// TODO
	return;
}

/**
 * @brief Returns the next item from the traffic map
 *
 * @param mr The map rect to search for items
 */
static struct item * tm_get_item(struct map_rect_priv *mr) {
	// TODO
	return NULL;
}

/**
 * @brief Returns the next item with the supplied ID from the traffic map
 *
 * @param mr The map rect to search for items
 * @param id_hi The high-order portion of the ID
 * @param id_lo The low-order portion of the ID
 */
static struct item * tm_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo) {
	// TODO
	return NULL;
}

static struct map_methods traffic_map_meth = {
	projection_mg,    /* pro: The projection used for that type of map */
	"utf-8",          /* charset: The charset this map uses. */
	tm_destroy,       /* map_destroy: Destroy ("close") a map. */
	tm_rect_new,      /* map_rect_new: Create a new map rect on the map. */
	tm_rect_destroy,  /* map_rect_destroy: Destroy a map rect */
	tm_get_item,      /* map_rect_get_item: Return the next item from a map rect */
	tm_get_item_byid, /* map_rect_get_item_byid: Get an item with a specific ID from a map rect, can be NULL */
	NULL,             /* map_search_new: Start a new search on the map, can be NULL */
	NULL,             /* map_search_destroy: Destroy a map search struct, ignored if `map_search_new` is NULL */
	NULL,             /* map_search_get_item: Get the next item of a search on the map */
	NULL,             /* map_rect_create_item: Create a new item in the map */
	NULL,             /* map_get_attr */
	NULL,             /* map_set_attr */
};

/**
 * @brief Determines the degree to which the attributes of a location and a map item match.
 *
 * The result of this method is used to match a location to a map item. Its result is a score—the higher
 * the score, the better the match.
 *
 * To calculate the score, all supplied attributes are examined. An exact match adds 4 to the score, a
 * partial match adds 2. Values of 1 are subtracted where additional granularity is needed. Undefined
 * attributes are not considered a match.
 *
 * @param this_ The location
 * @param item The map item
 *
 * @return The score
 */
int traffic_location_match_attributes(struct traffic_location * this_, struct item *item) {
	int ret = 0;
	struct attr attr;

	/* road type */
	if ((this_->road_type != type_line_unspecified) && item_attr_get(item, attr_type, &attr)) {
		if (attr.u.item_type == this_->road_type)
			ret +=4;
		else
			switch (this_->road_type) {
			/* motorway */
			case type_highway_land:
				if (attr.u.item_type == type_highway_city)
					ret += 3;
				else if (attr.u.item_type == type_street_n_lanes)
					ret += 2;
				break;
			case type_highway_city:
				if (attr.u.item_type == type_highway_land)
					ret += 3;
				else if (attr.u.item_type == type_street_n_lanes)
					ret += 2;
				break;
			/* trunk */
			case type_street_n_lanes:
				if ((attr.u.item_type == type_highway_land) || (attr.u.item_type == type_highway_city)
						|| (attr.u.item_type == type_street_4_land)
						|| (attr.u.item_type == type_street_4_city))
					ret += 2;
				break;
			/* primary */
			case type_street_4_land:
				if (attr.u.item_type == type_street_4_city)
					ret += 3;
				else if ((attr.u.item_type == type_street_n_lanes)
						|| (attr.u.item_type == type_street_3_land))
					ret += 2;
				else if (attr.u.item_type == type_street_3_city)
					ret += 1;
				break;
			case type_street_4_city:
				if (attr.u.item_type == type_street_4_land)
					ret += 3;
				else if ((attr.u.item_type == type_street_n_lanes)
						|| (attr.u.item_type == type_street_3_city))
					ret += 2;
				else if (attr.u.item_type == type_street_3_land)
					ret += 1;
				break;
			/* secondary */
			case type_street_3_land:
				if (attr.u.item_type == type_street_3_city)
					ret += 3;
				else if ((attr.u.item_type == type_street_4_land)
						|| (attr.u.item_type == type_street_2_land))
					ret += 2;
				else if ((attr.u.item_type == type_street_4_city)
						|| (attr.u.item_type == type_street_2_city))
					ret += 1;
				break;
			case type_street_3_city:
				if (attr.u.item_type == type_street_3_land)
					ret += 3;
				else if ((attr.u.item_type == type_street_4_city)
						|| (attr.u.item_type == type_street_2_city))
					ret += 2;
				else if ((attr.u.item_type == type_street_4_land)
						|| (attr.u.item_type == type_street_2_land))
					ret += 1;
				break;
			/* tertiary */
			case type_street_2_land:
				if (attr.u.item_type == type_street_2_city)
					ret += 3;
				else if ((attr.u.item_type == type_street_3_land))
					ret += 2;
				else if ((attr.u.item_type == type_street_3_city))
					ret += 1;
				break;
			case type_street_2_city:
				if (attr.u.item_type == type_street_2_land)
					ret += 3;
				else if ((attr.u.item_type == type_street_3_city))
					ret += 2;
				else if ((attr.u.item_type == type_street_3_land))
					ret += 1;
				break;
			default:
				break;
			}
	}

	/* road_ref */
	if (this_->road_ref && item_attr_get(item, attr_street_name_systematic, &attr)) {
		// TODO give partial score for partial matches
		if (!compare_name_systematic(this_->road_ref, attr.u.str))
			ret += 4;
	}

	/* road_name */
	if (this_->road_name && item_attr_get(item, attr_street_name, &attr)) {
		// TODO crude comparison in need of refinement
		if (!strcmp(this_->road_name, attr.u.str))
			ret += 4;
	}

	// TODO point->junction_ref
	// TODO point->junction_name

	// TODO direction
	// TODO destination

	// TODO tmc_table, point->tmc_id

	// TODO ramps

	return ret;
}

/**
 * @brief Returns the cost of the segment in the given direction.
 *
 * Currently the cost of a segment (for the purpose of matching traffic locations) is simply its length,
 * as only the best matching roads are used. Future versions may change this by considering all roads
 * and factoring match quality into the cost calculation.
 *
 * @param over The segment
 * @param dir The direction (positive numbers indicate positive direction)
 *
 * @return The cost of the segment, or `INT_MAX` if the segment is impassable in direction `dir`
 */
static int traffic_route_get_seg_cost(struct route_graph_segment *over, int dir) {
	if (over->data.flags & (dir >= 0 ? AF_ONEWAYREV : AF_ONEWAY))
		return INT_MAX;

	return over->data.len;
}

/**
 * @brief Populates a route graph.
 *
 * This method can operate in two modes: In “initial” mode the route graph is populated with the best
 * matching segments, which may not include any ramps. In “add ramps” mode, all ramps within the
 * enclosing rectangle are added, which can be done even after flooding the route graph.
 *
 * @param rg The route graph
 * @param ms The mapset to read the ramps from
 * @param mode 0 to initially populate the route graph, 1 to add ramps
 */
void traffic_location_populate_route_graph(struct traffic_location * this_, struct route_graph * rg,
		struct mapset * ms, int mode) {
	/* Corners of the enclosing rectangle, in Mercator coordinates */
	struct coord c1, c2;

	/* buffer zone around the rectangle */
	int max_dist = 1000;

	/* The item being processed */
	struct item *item;

	/* Mercator coordinates of current and previous point */
	struct coord c, l;

	/* The attribute matching score (current and maximum) */
	int score, maxscore = 0;

	/* Data for the route graph segment */
	struct route_graph_segment_data data;

	/* The length of the current segment */
#ifdef AVOID_FLOAT
	int len;
#else
	double len;
#endif

	/* Whether the current item is segmented */
	int segmented;

	/* Holds an attribute retrieved from the current item */
	struct attr attr;

	/* Start and end point of the current way or segment */
	struct route_graph_point *s_pnt, *e_pnt;

	if (!(this_->sw && this_->ne))
		return;

	rg->h = mapset_open(ms);

	while ((rg->m = mapset_next(rg->h, 2))) {
		transform_from_geo(map_projection(rg->m), this_->sw, &c1);
		transform_from_geo(map_projection(rg->m), this_->ne, &c2);

		rg->sel = route_rect(18, &c1, &c2, 0, max_dist);

		if (!rg->sel)
			continue;
		rg->mr = map_rect_new(rg->m, rg->sel);
		if (!rg->mr) {
			map_selection_destroy(rg->sel);
			rg->sel = NULL;
			continue;
		}
		while ((item = map_rect_get_item(rg->mr))) {
			/* TODO we might need turn restrictions in mode 1 as well */
			if ((mode == 1) && (item->type != type_ramp))
				continue;
			/* TODO are there any non-routable line types which we can exclude? */
			if ((item->type < type_line) || (item->type >= type_area))
				continue;
			if (item_get_default_flags(item->type)) {

				if (item_coord_get(item, &l, 1)) {
					if (mode == 0) {
						score = traffic_location_match_attributes(this_, item);

						if (score < maxscore)
							continue;
						if (score > maxscore) {
							/* we have found a better match, drop the previous route graph */
							route_graph_free_points(rg);
							route_graph_free_segments(rg);
							maxscore = score;
						}
					}

					data.flags=0;
					data.offset=1;
					data.maxspeed=-1;
					data.item=item;
					len = 0;
					segmented = 0;

					int default_flags_value = AF_ALL;
					int *default_flags = item_get_default_flags(item->type);
					if (!default_flags)
						default_flags = &default_flags_value;
					if (item_attr_get(item, attr_flags, &attr)) {
						data.flags = attr.u.num;
						if (data.flags & AF_SEGMENTED)
							segmented = 1;
					} else
						data.flags = *default_flags;

					s_pnt = route_graph_add_point(rg, &l);

					if (!segmented) {
						while (item_coord_get(item, &c, 1)) {
							len += transform_distance(map_projection(item->map), &l, &c);
							l = c;
						}
						e_pnt = route_graph_add_point(rg, &l);
						dbg_assert(len >= 0);
						data.len=len;
						if (!route_graph_segment_is_duplicate(s_pnt, &data))
							route_graph_add_segment(rg, s_pnt, e_pnt, &data);
					} else {
						int isseg, rc;
						int sc = 0;
						do {
							isseg = item_coord_is_node(item);
							rc = item_coord_get(item, &c, 1);
							if (rc) {
								len += transform_distance(map_projection(item->map), &l, &c);
								l = c;
								if (isseg) {
									e_pnt = route_graph_add_point(rg, &l);
									data.len = len;
									if (!route_graph_segment_is_duplicate(s_pnt, &data))
										route_graph_add_segment(rg, s_pnt, e_pnt, &data);
									data.offset++;
									s_pnt = route_graph_add_point(rg, &l);
									len = 0;
								}
							}
						} while(rc);
						e_pnt = route_graph_add_point(rg, &l);
						dbg_assert(len >= 0);
						sc++;
						data.len = len;
						if (!route_graph_segment_is_duplicate(s_pnt, &data))
							route_graph_add_segment(rg, s_pnt, e_pnt, &data);
					}
				}
			}
		}
		map_rect_destroy(rg->mr);
		rg->mr = NULL;
	}
	route_graph_build_done(rg, 1);
}

/**
 * @brief Builds a new route graph for traffic location matching.
 *
 * Traffic location matching is done by using a modified routing algorithm to identify the segments
 * affected by a traffic message.
 *
 * @param this_ The location to match to the map
 * @param ms The mapset to use for the route graph
 *
 * @return A route graph. The caller is responsible for destroying the route graph and all related data
 * when it is no longer needed.
 */
struct route_graph * traffic_location_get_route_graph(struct traffic_location * this_, struct mapset * ms) {
	struct route_graph *rg;

	if (!(this_->sw && this_->ne))
		return NULL;

	rg = g_new0(struct route_graph, 1);

	rg->done_cb = NULL;
	rg->busy = 1;

	/* build the route graph */
	traffic_location_populate_route_graph(this_, rg, ms, 0);

	return rg;
}

/**
 * @brief Determines the path between two reference points in a route graph.
 *
 * The reference points `from` and `to` are the beginning and end of the path and do not necessarily
 * coincide with the `from` and `to` members of the location. For a point location with an auxiliary
 * point, one will instead be the `at` member of the location; when examining the opposite direction of
 * a bidirectional location, `from` and `to` will be swapped with respect to the location.
 *
 * The coordinates contained in the reference points are typically approximate, i.e. they do not
 * precisely coincide with a point in the route graph.
 *
 * When this function returns, the route graph will be flooded, i.e. every point will have a cost
 * assigned to it and the `seg` member for each point will be set, indicating the next segment on which
 * to proceed in order to reach the destination. For the last point in the graph, `seg` will be `NULL`.
 * Unlike in common routing, the last point will have a nonzero cost if `to` does not coincide with a
 * point in the route graph.
 *
 * The cost of each node represents the cost to reach `to`. Currently distance is used for cost, with a
 * penalty applied to the offroad connection from the last point in the graph to `to`. Future versions
 * may calculate segment cost differently.
 *
 * To obtain the path, start with the return value. Its `seg` member points to the next segment. Either
 * the `start` or the `end` value of that segment will coincide with the point currently being examined;
 * the other of the two is the point at the other end. Repeat this until you reach a point whose `seg`
 * member is `NULL`.
 *
 * This function can be run multiple times against the same route graph but with different reference
 * points. It is safe to call with `NULL` passed for one or both reference points, in which case `NULL`
 * will be returned.
 *
 * The caller is responsible for freeing up the data structures passed to this function when they are no
 * longer needed.
 *
 * @param rg The route graph
 * @param from Start location
 * @param to Destination location
 *
 * @return The point in the route graph at which the path begins, or `NULL` if no path was found.
 */
struct route_graph_point * traffic_route_flood_graph(struct route_graph * rg,
		struct traffic_point * from, struct traffic_point * to) {
	struct route_graph_point * ret;

	/* Projected coordinates of start and destination point */
	struct coord c_start, c_dst;

	int i;

	/* This heap will hold all points with "temporarily" calculated costs */
	struct fibheap *heap;

	/* Cost of the start position */
	int start_value;

	/* The point currently being examined */
	struct route_graph_point *p;

	/* Cost of point being examined, other end of segment being examined, segment */
	int min, new, val;

	/* The segment currently being examined */
	struct route_graph_segment *s = NULL;

	if (!from || !to)
		return NULL;

	/* transform coordinates */
	transform_from_geo(projection_mg, &to->coord, &c_dst);
	transform_from_geo(projection_mg, &from->coord, &c_start);

	/* prime the route graph */
	heap = fh_makekeyheap();

	start_value = PENALTY_OFFROAD * transform_distance(projection_mg, &c_start, &c_dst);
	ret = NULL;

	dbg(lvl_error, "start flooding route graph, start_value=%d\n", start_value);

	for (i = 0; i < HASH_SIZE; i++) {
		p = rg->hash[i];
		while (p) {
			p->value = PENALTY_OFFROAD * transform_distance(projection_mg, &p->c, &c_dst);
			p->el = fh_insertkey(heap, p->value, p);
			p->seg = NULL;
			p = p->hash_next;
		}
	}

	/* flood the route graph */
	for (;;) {
		p = fh_extractmin(heap); /* Starting Dijkstra by selecting the point with the minimum costs on the heap */
		if (!p) /* There are no more points with temporarily calculated costs, Dijkstra has finished */
			break;

		dbg(lvl_error, "p=0x%x, value=%d\n", p, p->value);

		min = p->value;
		p->el = NULL; /* This point is permanently calculated now, we've taken it out of the heap */
		s = p->start;
		while (s) { /* Iterating all the segments leading away from our point to update the points at their ends */
			val = traffic_route_get_seg_cost(s, -1);

			dbg(lvl_error, "  negative segment, val=%d\n", val);

			if (val != INT_MAX) {
				new = min + val;
				if (new < s->end->value) { /* We've found a less costly way to reach the end of s, update it */
					s->end->value = new;
					s->end->seg = s;
					if (!s->end->el) {
						s->end->el = fh_insertkey(heap, new, s->end);
					} else {
						fh_replacekey(heap, s->end->el, new);
					}
					new += PENALTY_OFFROAD * transform_distance(projection_mg, &s->end->c, &c_start);
					if (new < start_value) { /* We've found a less costly way from the start point, update */
						start_value = new;
						ret = s->end;
					}
				}
			}
			s = s->start_next;
		}
		s = p->end;
		while (s) { /* Doing the same as above with the segments leading towards our point */
			val = traffic_route_get_seg_cost(s, 1);

			dbg(lvl_error, "  positive segment, val=%d\n", val);

			if (val != INT_MAX) {
				new = min + val;
				if (new < s->start->value) {
					s->start->value = new;
					s->start->seg = s;
					if (!s->start->el) {
						s->start->el = fh_insertkey(heap, new, s->start);
					} else {
						fh_replacekey(heap, s->start->el, new);
					}
					new += PENALTY_OFFROAD * transform_distance(projection_mg, &s->start->c, &c_start);
					if (new < start_value) {
						start_value = new;
						ret = s->start;
					}
				}
			}
			s = s->end_next;
		}
	}

	fh_deleteheap(heap);
	return ret;
}

/**
 * @brief Matches a location to a map.
 *
 * This takes the approximate coordinates contained in the `from`, `at` and `to` members of the location
 * and matches them to an exact position on the map, using both the raw coordinates and the auxiliary
 * information contained in the location.
 *
 * @param this_ The location to match to the map
 * @param ms The mapset to use for matching
 *
 * @return `true` if the locations were matched successfully, `false` if there was a failure.
 */
int traffic_location_match_to_map(struct traffic_location * this_, struct mapset * ms) {
	int i;

	/* Corners of the enclosing rectangle, in WGS84 coordinates */
	struct coord_geo * sw;
	struct coord_geo * ne;

	struct coord_geo * coords[] = {&this_->from->coord, &this_->at->coord, &this_->to->coord};

	/* The direction (positive or negative) */
	int dir = 1;

	/* Next point after start position */
	struct route_graph_point * start_next;

	struct route_graph_segment *s = NULL;

	/* point at which the next segment starts, i.e. up to which the path is complete */
	struct route_graph_point *start;

	/* route graph for simplified routing */
	struct route_graph *rg;

	/* Coordinate count for matched segment */
	int ccnt;

	/* Coordinates of matched segment and pointer into it, order as read from map */
	struct coord *c, ca[2048];

	/* Coordinates of matched segment, sorted */
	struct coord *cd, *cs;

	/* Attributes for traffic distortion */
	struct attr **attrs;

	/* calculate enclosing rectangle, if not yet present */
	if (!this_->sw) {
		sw = g_new0(struct coord_geo, 1);
		sw->lat = INT_MAX;
		sw->lng = INT_MAX;
		for (i = 0; i < 3; i++)
			if (coords[i]) {
				if (coords[i]->lat < sw->lat)
					sw->lat = coords[i]->lat;
				if (coords[i]->lng < sw->lng)
					sw->lng = coords[i]->lng;
			}
		this_->sw = sw;
	}

	if (!this_->ne) {
		ne = g_new0(struct coord_geo, 1);
		ne->lat = -INT_MAX;
		ne->lng = -INT_MAX;
		for (i = 0; i < 3; i++)
			if (coords[i]) {
				if (coords[i]->lat > ne->lat)
					ne->lat = coords[i]->lat;
				if (coords[i]->lng > ne->lng)
					ne->lng = coords[i]->lng;
			}
		this_->ne = ne;
	}

	if (this_->at)
		/* TODO Point location, not supported yet */
		return 0;

	if (this_->ramps != location_ramps_none)
		/* TODO Ramps, not supported yet */
		return 0;

	/* Line location, main carriageway */

	rg = traffic_location_get_route_graph(this_, ms);

	/* determine segments, once for each direction */
	while (1) {
		if (dir > 0)
			start_next = traffic_route_flood_graph(rg, this_->from ? this_->from : this_->at,
					this_->to ? this_->to : this_->at);
		else
			start_next = traffic_route_flood_graph(rg, this_->to ? this_->to : this_->at,
					this_->from ? this_->from : this_->at);

		/* calculate route */
		if (start_next)
			s = start_next->seg;
		else
			s = NULL;
		start = start_next;

		if (!s)
			dbg(lvl_error, "no segments\n");

		while (s) {
			ccnt = item_coord_get_within_range(&s->data.item, ca, 2047, &s->start->c, &s->end->c);
			c = ca;
			cs = g_new0(struct coord, ccnt);
			cd = cs;

			attrs = g_new0(struct attr*, 2);
			attrs[0] = g_new0(struct attr, 1);
			attrs[0]->type = attr_maxspeed;
			attrs[0]->u.num = 20;

			if (s->start == start) {
				/* forward direction, maintain order of coordinates */
				for (i = 0; i < ccnt; i++) {
					*cd++ = *c++;
				}
				start = s->end;
			} else {
				/* backward direction, reverse order of coordinates */
				c += ccnt-1;
				for (i = 0; i < ccnt; i++) {
					*cd++ = *c--;
				}
				start = s->start;
			}

			tm_add_item(NULL, type_traffic_distortion, 0, 0, attrs, cs, ccnt);
			g_free(attrs[0]);
			g_free(attrs);
			/* TODO decide who frees cs */

			s = start->seg;
		}

		/* TODO tweak ends (find the point where the ramp touches the main road) */

		if ((this_->directionality == location_dir_one) || (dir < 0))
			break;

		dir = -1;
	}

	route_graph_free_points(rg);
	route_graph_free_segments(rg);
	g_free(rg);

	return 1;
}

/**
 * @brief Prints a dump of a message to debug output.
 *
 * @param this_ The message to dump
 */
void traffic_message_dump(struct traffic_message * this_) {
	int i, j;
	char * point_names[3] = {"From", "At", "To"};
	struct traffic_point * points[3];

	if (!this_) {
		dbg(lvl_debug, "(null)\n");
		return;
	}

	points[0] = this_->location->from;
	points[1] = this_->location->at;
	points[2] = this_->location->to;

	dbg(lvl_debug, "id='%s', is_cancellation=%d, is_forecast=%d\n",
			this_->id, this_->is_cancellation, this_->is_forecast);
	/* TODO timestamps */

	/* dump replaced message IDs */
	dbg(lvl_debug, "  replaced_count=%d\n",
			this_->replaced_count);
	for (i = 0; i < this_->replaced_count; i++) {
		dbg(lvl_debug, "  Replaces: '%s'\n", this_->replaces[i]);
	}

	/* dump location */
	dbg(lvl_debug, "  Location: road_type='%s', road_ref='%s', road_name='%s'\n",
			item_to_name(this_->location->road_type), this_->location->road_ref,
			this_->location->road_name);
	dbg(lvl_debug, "    directionality=%d, destination='%s', direction='%s'\n",
			this_->location->directionality, this_->location->destination, this_->location->direction);
	dbg(lvl_debug, "    fuzziness=%d, ramps=%d, tmc_table='%s', tmc_direction=%+d\n",
			this_->location->fuzziness, this_->location->ramps, this_->location->tmc_table,
			this_->location->tmc_direction);
	for (i = 0; i < 3; i++) {
		if (points[i]) {
			dbg(lvl_debug, "    %s: lat=%.5f, lng=%.5f\n",
					point_names[i], points[i]->coord.lat, points[i]->coord.lng);
			dbg(lvl_debug, "      junction_name='%s', junction_ref='%s', tmc_id='%s'\n",
					points[i]->junction_name, points[i]->junction_ref, points[i]->tmc_id);

			if (points[i]->map_coord) {
				dbg(lvl_debug, "      Map-matched: pro=%d, x=%x, y=%x\n",
						points[i]->map_coord->pro, points[i]->map_coord->x, points[i]->map_coord->y);
			} else {
				dbg(lvl_debug, "      Map-matched: (null)\n");
			}

			if (points[i]->map_coord_backward) {
				dbg(lvl_debug, "      Map-matched backward: pro=%d, x=%x, y=%x\n",
						points[i]->map_coord_backward->pro, points[i]->map_coord_backward->x,
						points[i]->map_coord_backward->y);
			} else {
				dbg(lvl_debug, "      Map-matched backward: (null)\n");
			}
		} else {
			dbg(lvl_debug, "    %s: (null)\n",
					point_names[i]);
		}
	}

	/* dump events */
	dbg(lvl_debug, "  event_count=%d\n",
			this_->event_count);
	for (i = 0; i < this_->event_count; i++) {
		dbg(lvl_debug, "  Event: event_class=%d, type=%d, length=%d m, speed=%d km/h\n",
				this_->events[i]->event_class, this_->events[i]->type, this_->events[i]->length,
				this_->events[i]->speed);
		/* TODO quantifier */

		/* dump supplementary information */
		dbg(lvl_debug, "    si_count=%d\n",
				this_->events[i]->si_count);
		for (j = 0; j < this_->events[i]->si_count; j++) {
			dbg(lvl_debug, "    Supplementary Information: si_class=%d, type=%d\n",
					this_->events[i]->si[j]->si_class, this_->events[i]->si[j]->type);
			/* TODO quantifier */
		}
	}
}

/**
 * @brief The loop function for the traffic module.
 *
 * This function polls backends for new messages and processes them by inserting, removing or modifying
 * traffic distortions and triggering route recalculations as needed.
 */
void traffic_loop(struct traffic * this_) {
	int i;
	struct traffic_message ** messages;

	messages = this_->meth.get_messages(this_->priv);
	if (!messages)
		return;
	for (i = 0; messages[i] != NULL; i++) {
		traffic_location_match_to_map(messages[i]->location, this_->ms);

		traffic_message_dump(messages[i]);
	}
	dbg(lvl_debug, "received %d message(s)\n", i);
}

/**
 * @brief Instantiates the traffic plugin
 *
 * At a minimum, `attrs` must contain a `type` attribute matching one of the available traffic plugins.
 *
 * @param parent The parent, usually the Navit instance
 * @param attrs The attributes for the plugin
 *
 * @return A `traffic` instance.
 */
struct traffic * traffic_new(struct attr *parent, struct attr **attrs) {
	struct traffic *this_;
	struct traffic_priv *(*traffic_new)(struct navit *nav, struct traffic_methods *meth,
			struct attr **attrs, struct callback_list *cbl);
	struct attr *attr;

	attr = attr_search(attrs, NULL, attr_type);
	if (!attr) {
		dbg(lvl_error, "type missing\n");
		return NULL;
	}
	dbg(lvl_debug, "type='%s'\n", attr->u.str);
	traffic_new = plugin_get_category_traffic(attr->u.str);
	dbg(lvl_debug, "new=%p\n", traffic_new);
	if (!traffic_new) {
		dbg(lvl_error, "wrong type '%s'\n", attr->u.str);
		return NULL;
	}
	this_ = (struct traffic *) navit_object_new(attrs, &traffic_func, sizeof(struct traffic));
	if (parent->type == attr_navit)
		this_->navit = parent->u.navit;
	else {
		dbg(lvl_error, "wrong parent type '%s', only navit is permitted\n", attr_to_name(parent->type));
		navit_object_destroy((struct navit_object *) this_);
		return NULL;
	}

	this_->priv = traffic_new(parent->u.navit, &this_->meth, this_->attrs, NULL);
	dbg(lvl_debug, "get_messages=%p\n", this_->meth.get_messages);
	dbg(lvl_debug, "priv=%p\n", this_->priv);
	if (!this_->priv) {
		dbg(lvl_error, "plugin initialization failed\n");
		navit_object_destroy((struct navit_object *) this_);
		return NULL;
	}
	navit_object_ref((struct navit_object *) this_);
	dbg(lvl_debug,"return %p\n", this_);

	// TODO do this once and cycle through all plugins
	this_->callback = callback_new_1(callback_cast(traffic_loop), this_);
	this_->timeout = event_add_timeout(1000, 1, this_->callback); // TODO make interval configurable

	return this_;
}

struct traffic_point * traffic_point_new(float lon, float lat, char * junction_name, char * junction_ref,
		char * tmc_id) {
	struct traffic_point * ret;

	ret = g_new0(struct traffic_point, 1);
	ret->coord.lat = lat;
	ret->coord.lng = lon;
	ret->map_coord = NULL;
	ret->map_coord_backward = NULL;
	ret->junction_name = junction_name ? g_strdup(junction_name) : NULL;
	ret->junction_ref = junction_ref ? g_strdup(junction_ref) : NULL;
	ret->tmc_id = tmc_id ? g_strdup(tmc_id) : NULL;
	return ret;
}

struct traffic_point * traffic_point_new_short(float lon, float lat) {
	return traffic_point_new(lon, lat, NULL, NULL, NULL);
}

void traffic_point_destroy(struct traffic_point * this_) {
	if (this_->map_coord)
		g_free(this_->map_coord);
	if (this_->map_coord_backward)
		g_free(this_->map_coord_backward);
	if (this_->junction_name)
		g_free(this_->junction_name);
	if (this_->junction_ref)
		g_free(this_->junction_ref);
	if (this_->tmc_id)
		g_free(this_->tmc_id);
	g_free(this_);
}

// TODO split CID/LTN?
struct traffic_location * traffic_location_new(struct traffic_point * at, struct traffic_point * from,
		struct traffic_point * to, char * destination, char * direction, enum location_dir directionality,
		enum location_fuzziness fuzziness, enum location_ramps ramps, enum item_type road_type,
		char * road_name, char * road_ref, char * tmc_table, int tmc_direction) {
	struct traffic_location * ret;

	ret = g_new0(struct traffic_location, 1);
	ret->at = at;
	ret->from = from;
	ret->to = to;
	ret->destination = destination ? g_strdup(destination) : NULL;
	ret->direction = direction ? g_strdup(direction) : NULL;
	ret->directionality = directionality;
	ret->fuzziness = fuzziness;
	ret->ramps = ramps;
	ret->road_type = road_type;
	ret->road_name = road_name ? g_strdup(road_name) : NULL;
	ret->road_ref = road_ref ? g_strdup(road_ref) : NULL;
	ret->tmc_table = tmc_table ? g_strdup(tmc_table) : NULL;
	ret->tmc_direction = tmc_direction;
	ret->sw = NULL;
	ret->ne = NULL;
	return ret;
}

struct traffic_location * traffic_location_new_short(struct traffic_point * at, struct traffic_point * from,
		struct traffic_point * to, enum location_dir directionality, enum location_fuzziness fuzziness) {
	return traffic_location_new(at, from, to, NULL, NULL, directionality, fuzziness, location_ramps_none,
			type_line_unspecified, NULL, NULL, NULL, 0);
}

void traffic_location_destroy(struct traffic_location * this_) {
	if (this_->at)
		traffic_point_destroy(this_->at);
	if (this_->from)
		traffic_point_destroy(this_->from);
	if (this_->to)
		traffic_point_destroy(this_->to);
	if (this_->destination)
		g_free(this_->destination);
	if (this_->direction)
		g_free(this_->direction);
	if (this_->road_name)
		g_free(this_->road_name);
	if (this_->road_ref)
		g_free(this_->road_ref);
	if (this_->tmc_table)
		g_free(this_->tmc_table);
	if (this_->sw)
		g_free(this_->sw);
	if (this_->ne)
		g_free(this_->ne);
	g_free(this_);
}

struct traffic_suppl_info * traffic_suppl_info_new(enum si_class si_class, enum si_type type,
		struct quantifier * quantifier) {
	struct traffic_suppl_info * ret;
	ret = g_new0(struct traffic_suppl_info, 1);
	ret->si_class = si_class;
	ret->type = type;
	ret->quantifier = quantifier ? g_memdup(quantifier, sizeof(struct quantifier)) : NULL;
	return ret;
}

void traffic_suppl_info_destroy(struct traffic_suppl_info * this_) {
	if (this_->quantifier)
		g_free(this_->quantifier);
	g_free(this_);
}

struct traffic_event * traffic_event_new(enum event_class event_class, enum event_type type,
		int length, int speed, struct quantifier * quantifier, int si_count, struct traffic_suppl_info ** si) {
	struct traffic_event * ret;

	ret = g_new0(struct traffic_event, 1);
	ret->event_class = event_class;
	ret->type = type;
	ret->length = length;
	ret->speed = speed;
	ret->quantifier = quantifier ? g_memdup(quantifier, sizeof(struct quantifier)) : NULL;
	if (si_count && si) {
		ret->si_count = si_count;
		ret->si = g_memdup(si, sizeof(struct traffic_suppl_info *) * si_count);
	} else {
		ret->si_count = 0;
		ret->si = NULL;
	}
	return ret;
}

struct traffic_event * traffic_event_new_short(enum event_class event_class, enum event_type type) {
	return traffic_event_new(event_class, type, -1, INT_MAX, NULL, 0, NULL);
}

void traffic_event_destroy(struct traffic_event * this_) {
	int i;

	if (this_->quantifier)
		g_free(this_->quantifier);
	if (this_->si) {
		for (i = 0; i < this_->si_count; i++)
			traffic_suppl_info_destroy(this_->si[i]);
		g_free(this_->si);
	}
	g_free(this_);
}

void traffic_event_add_suppl_info(struct traffic_event * this_, struct traffic_suppl_info * si) {
	struct traffic_suppl_info ** si_new;

	if (this_->si_count && this_->si) {
		si_new = g_new0(struct traffic_suppl_info *, this_->si_count + 1);
		memcpy(si_new, this_->si, sizeof(struct traffic_suppl_info *) * this_->si_count);
		si_new[this_->si_count] = si;
		g_free(this_->si);
		this_->si = si_new;
		this_->si_count++;
	} else {
		this_->si = g_new0(struct traffic_suppl_info *, 1);
		this_->si[0] = si;
		this_->si_count = 1;
	}
}

struct traffic_suppl_info * traffic_event_get_suppl_info(struct traffic_event * this_, int index) {
	if (this_->si && (index < this_->si_count))
		return this_->si[index];
	else
		return NULL;
}

struct traffic_message * traffic_message_new(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, time_t start_time, time_t end_time, int is_cancellation, int is_forecast,
		int replaced_count, char ** replaces, struct traffic_location * location, int event_count,
		struct traffic_event ** events) {
	struct traffic_message * ret;

	ret = g_new0(struct traffic_message, 1);
	ret->id = g_strdup(id);
	ret->receive_time = receive_time;
	ret->update_time = update_time;
	ret->expiration_time = expiration_time;
	ret->start_time = start_time;
	ret->end_time = end_time;
	ret->is_cancellation = is_cancellation;
	ret->is_forecast = is_forecast;
	if (replaced_count && replaces) {
		ret->replaced_count = replaced_count;
		ret->replaces = g_memdup(replaces, sizeof(char *) * replaced_count);
	} else {
		ret->replaced_count = 0;
		ret->replaces = NULL;
	}
	ret->location = location;
	ret->event_count = event_count;
	ret->events = g_memdup(events, sizeof(struct traffic_event *) * event_count);
	return ret;
}

struct traffic_message * traffic_message_new_short(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, int is_cancellation, int is_forecast, struct traffic_location * location,
		int event_count, struct traffic_event ** events) {
	return traffic_message_new(id, receive_time, update_time, expiration_time, 0, 0, is_cancellation,
			is_forecast, 0, NULL, location, event_count, events);
}

struct traffic_message * traffic_message_new_single_event(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, int is_cancellation, int is_forecast, struct traffic_location * location,
		enum event_class event_class, enum event_type type) {
	struct traffic_event * event;
	struct traffic_event ** events;

	event = traffic_event_new_short(event_class, type);
	events = g_new0(struct traffic_event *, 1);
	events[0] = event;
	return traffic_message_new_short(id, receive_time, update_time, expiration_time, is_cancellation,
			is_forecast, location, 1, events);
	g_free(events);
}

void traffic_message_destroy(struct traffic_message * this_) {
	int i;

	g_free(this_->id);
	if (this_->replaces) {
		for (i = 0; i < this_->replaced_count; i++)
			g_free(this_->replaces[i]);
		g_free(this_->replaces);
	}
	traffic_location_destroy(this_->location);
	for (i = 0; i < this_->event_count; i++)
		traffic_event_destroy(this_->events[i]);
	g_free(this_->events);
}

void traffic_message_add_event(struct traffic_message * this_, struct traffic_event * event) {
	struct traffic_event ** events_new;

	events_new = g_new0(struct traffic_event *, this_->event_count + 1);
	memcpy(events_new, this_->events, sizeof(struct traffic_event *) * this_->event_count);
	events_new[this_->event_count] = event;
	g_free(this_->events);
	this_->events = events_new;
	this_->event_count++;
}

struct traffic_event * traffic_message_get_event(struct traffic_message * this_, int index) {
	if (this_->events && (index < this_->event_count))
		return this_->events[index];
	else
		return NULL;
}

/**
 * @brief Registers a new traffic map plugin
 *
 * @param meth Receives the map methods
 * @param attrs The attributes for the map
 * @param cbl
 *
 * @return A pointer to a {@code map_priv} structure for the map
 */
static struct map_priv * traffic_map_new(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl) {
	struct map_priv *ret;

	dbg(lvl_error, "enter\n");

	ret = g_new0(struct map_priv, 1);
	*meth = traffic_map_meth;

	return ret; // TODO
}

void traffic_init(void) {
	dbg(lvl_error, "enter\n");
	plugin_register_category_map("traffic", traffic_map_new);
}

void traffic_set_mapset(struct traffic *this_, struct mapset *ms) {
	this_->ms = ms;
}

void traffic_set_route(struct traffic *this_, struct route *rt) {
	this_->rt = rt;
}

struct object_func traffic_func = {
	attr_traffic,
	(object_func_new)traffic_new,
	(object_func_get_attr)navit_object_get_attr,
	(object_func_iter_new)navit_object_attr_iter_new,
	(object_func_iter_destroy)navit_object_attr_iter_destroy,
	(object_func_set_attr)navit_object_set_attr,
	(object_func_add_attr)navit_object_add_attr,
	(object_func_remove_attr)navit_object_remove_attr,
	(object_func_init)NULL,
	(object_func_destroy)navit_object_destroy,
	(object_func_dup)NULL,
	(object_func_ref)navit_object_ref,
	(object_func_unref)navit_object_unref,
};
