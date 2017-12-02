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
#include "navit.h" // TODO see if we will still need that in the long run
#include "util.h"
#include "coord.h"
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "route.h"
#include "transform.h"
#include "xmlconfig.h"
#include "traffic.h"
#include "plugin.h"
#include "event.h"
#include "callback.h"
#include "debug.h"

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
 * @param point The point to calculate the score for: 0 = `from`, 1 = `at`, 2 = `to`
 * @param item The map item
 *
 * @return The score
 */
int traffic_location_match_attributes(struct traffic_location * this_, int point, struct item *item) {
	int ret = 0;
	struct traffic_point *tp;
	struct attr attr;

	tp = (point == 0) ? this_->from : (point == 1) ? this_->at : (point == 2) ? this_->to : NULL;
	if (!tp)
		return ret;

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
	struct traffic_point * points[3];

	/* Corners of the enclosing rectangle, in WGS84 coordinates */
	struct coord_geo g1;
	struct coord_geo g2;

	/* Corners of the enclosing rectangle, in Mercator coordinates */
	struct coord c1, c2;

	/* Projected raw coordinates */
	struct coord c;

	struct mapset_handle *h;
	struct map *m;
	struct map_selection *sel;
	int max_dist = 1000;
	struct map_rect *mr;
	struct item *item;

	/* The matched point for the last item examined */
	struct coord lpnt;

	/* The matched point */
	struct pcoord * plpnt;

	/* The attribute matching score */
	int score, maxscore;

	/* Current and minimum distance between point and road */
	int dist, mindist;

	/* Street data for the item being examined */
	struct street_data *sd;

	if (!(this_->from || this_->at || this_->to))
		return 0;

	points[0] = this_->from;
	points[1] = this_->at;
	points[2] = this_->to;

	// FIXME this may fail if three points are given and `at` is outside the enclosing rectangle
	g1 = this_->from ? this_->from->coord : this_->at ? this_->at->coord : this_->to->coord;
	g2 = this_->to ? this_->to->coord : this_->at ? this_->at->coord : this_->from->coord;

	for (i = 0; i < 3; i++) {
		if (points[i]) {
			// TODO both directions if location is bidirectional
			/* match to map */
			plpnt = NULL;
			mindist = INT_MAX;
			maxscore = 0;

			h = mapset_open(ms);
			while ((m = mapset_next(h, 2))) {
				transform_from_geo(map_projection(m), &points[i]->coord, &c);
				transform_from_geo(map_projection(m), &g1, &c1);
				transform_from_geo(map_projection(m), &g2, &c2);
				sel = route_rect(18, &c1, &c2, 0, max_dist);
				if (!sel)
					continue;
				mr = map_rect_new(m, sel);
				if (!mr) {
					map_selection_destroy(sel);
					continue;
				}
				while ((item = map_rect_get_item(mr))) {
					if (item_get_default_flags(item->type)) {
						score = traffic_location_match_attributes(this_, i, item);
						sd = street_get_data(item);
						if (!sd)
							continue;
						dist = transform_distance_polyline_sq(sd->c, sd->count, &c, &lpnt, NULL);

						if ((score > maxscore) || ((score == maxscore) && (dist < mindist))) {
							maxscore = score;
							mindist = dist;

							if (!plpnt)
								plpnt = g_new0(struct pcoord, 1);
							plpnt->pro = map_projection(m);
							plpnt->x = lpnt.x;
							plpnt->y = lpnt.y;

							dbg(lvl_debug,"dist=%d id 0x%x 0x%x\n", dist, item->id_hi, item->id_lo);
						}
						street_data_free(sd);
					}
				}
				map_selection_destroy(sel);
				map_rect_destroy(mr);
			}
			mapset_close(h);
			if (plpnt)
				points[i]->map_coord = plpnt;
			else
				return 0;
		}
	}

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

		/* begin crude code */
		/* temporary solution to dump traffic report to a map, only good for visualization but not for
		 * any kind of routing */
		struct attr **attrs = g_new0(struct attr*, 2);
		struct coord coords[3];
		int coord_count = 0;
		attrs[0] = g_new0(struct attr, 1);
		attrs[0]->type = attr_maxspeed;
		attrs[0]->u.num = 20;
		if (messages[i]->location->from && messages[i]->location->from->map_coord) {
			coords[coord_count].x = messages[i]->location->from->map_coord->x;
			coords[coord_count].y = messages[i]->location->from->map_coord->y;
			coord_count++;
		}
		if (messages[i]->location->at && messages[i]->location->at->map_coord) {
			coords[coord_count].x = messages[i]->location->at->map_coord->x;
			coords[coord_count].y = messages[i]->location->at->map_coord->y;
			coord_count++;
		}
		if (messages[i]->location->to && messages[i]->location->to->map_coord) {
			coords[coord_count].x = messages[i]->location->to->map_coord->x;
			coords[coord_count].y = messages[i]->location->to->map_coord->y;
			coord_count++;
		}
		tm_add_item(NULL, type_traffic_distortion, 0, 0, attrs, &coords, coord_count);
		g_free(attrs[0]);
		g_free(attrs);
		/* end crude code */

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
	(object_func_iter_new)navit_object_attr_iter_new, // TODO or NULL
	(object_func_iter_destroy)navit_object_attr_iter_destroy, // TODO or NULL
	(object_func_set_attr)navit_object_set_attr, // TODO or NULL
	(object_func_add_attr)navit_object_add_attr, // TODO or NULL
	(object_func_remove_attr)navit_object_remove_attr, // TODO or NULL
	(object_func_init)NULL,
	(object_func_destroy)navit_object_destroy, // TODO or NULL
	(object_func_dup)NULL,
	(object_func_ref)navit_object_ref,
	(object_func_unref)navit_object_unref,
};
