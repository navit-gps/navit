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
#include <stdlib.h>
#include <time.h>

#ifdef _POSIX_C_SOURCE
#include <sys/types.h>
#endif
#include <sys/time.h>
#include "glib_slice.h"
#include "config.h"
#include "navit.h"
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
#include "vehicleprofile.h"
#include "debug.h"

#undef TRAFFIC_DEBUG

/** Flag to indicate new messages have been received */
#define MESSAGE_UPDATE_MESSAGES 1 << 0

/** Flag to indicate segments have changed */
#define MESSAGE_UPDATE_SEGMENTS 1 << 1

/** The penalty applied to an off-road link */
#define PENALTY_OFFROAD 8

/** The penalty applied to segments with non-matching attributes */
#define PENALTY_SEGMENT_MATCH 4

/** The maximum penalty applied to points with non-matching attributes */
#define PENALTY_POINT_MATCH 24

/** Flag to indicate expired messages should be purged */
#define PROCESS_MESSAGES_PURGE_EXPIRED 1 << 0

/** Flag to indicate the message store should not be exported */
#define PROCESS_MESSAGES_NO_DUMP_STORE 1 << 1

/** The lowest order of items to consider (always the default order for the next-lower road type) */
#define ROUTE_ORDER(x) (((x == type_highway_land) || (x == type_highway_city) || (x == type_street_n_lanes)) \
        ? 10 : ((x == type_street_4_land) || (x == type_street_4_city)) ? 12 : 18)

/** The buffer zone around the enclosing rectangle used in route calculations, absolute distance */
#define ROUTE_RECT_DIST_ABS(x) ((x == location_fuzziness_low_res) ? 1000 : 100)

/** The buffer zone around the enclosing rectangle used in route calculations, relative to rect size */
#define ROUTE_RECT_DIST_REL(x) 0

/** Maximum textfile line size */
#define TEXTFILE_LINE_SIZE 512

/** Time slice for idle loops, in milliseconds */
#define TIME_SLICE 40

/** Default value assumed for access flags if we cannot get flags for the item, nor for the item type */
int item_default_flags_value = AF_ALL;

/**
 * @brief Private data shared between all traffic instances.
 */
struct traffic_shared_priv {
    GList * messages;           /**< Currently active messages */
    GList * message_queue;      /**< Queued messages, waiting to be processed */
    // TODO messages by ID?                 In a later phase…
    struct mapset *ms;          /**< The mapset used for routing */
    struct route *rt;           /**< The route to notify of traffic changes */
    struct map *map;            /**< The traffic map, in which traffic distortions are stored */
};

/**
 * @brief A traffic plugin instance.
 *
 * If multiple traffic plugins are loaded, each will have its own `struct traffic` instance.
 */
struct traffic {
    NAVIT_OBJECT
    struct navit *navit;         /**< The navit instance */
    struct traffic_shared_priv *shared; /**< Private data shared between all instances */
    struct traffic_priv *priv;   /**< Private data used by the plugin */
    struct traffic_methods meth; /**< Methods implemented by the plugin */
    struct callback * callback;  /**< The callback function for the idle loop */
    struct event_timeout * timeout; /**< The timeout event that triggers the loop function */
    struct callback *idle_cb;    /**< Idle callback to process new messages */
    struct event_idle *idle_ev;  /**< The pointer to the idle event */
};

struct traffic_location_priv {
    char * txt_data;                   /*!< Persisted location data in txtfile map format. */
    struct coord_geo * sw;             /*!< Southwestern corner of rectangle enclosing all points.
	                                    *   Calculated by Navit from the points of the location. */
    struct coord_geo * ne;             /*!< Northeastern corner of rectangle enclosing all points.
	                                    *   Calculated by Navit from the points of the location. */
};

struct traffic_message_priv {
    struct item **items;        /**< The items for this message in the traffic map */
};

/**
 * @brief Private data for the traffic map.
 *
 * If multiple traffic plugins are loaded, the map is shared between all of them.
 */
struct map_priv {
    GList * items;              /**< The map items */
    struct traffic_shared_priv *shared; /**< Private data shared between all instances */
    // TODO items by start/end coordinates? In a later phase…
};

/**
 * @brief Implementation-specific map rect data
 */
struct map_rect_priv {
    struct map_priv *mpriv;     /**< The map to which this map rect refers */
    struct item *item;          /**< The current item, i.e. the last item returned by the `map_rect_get_item` method */
    GList * next_item;          /**< `GList` entry for the next item to be returned by `map_rect_get_item` */
};

/**
 * @brief Message-specific map private data
 *
 * This structure is needed to handle segments referenced by multiple messages. When a message changes,
 * is cancelled or expires, the data of the remaining messages is used to determine the new attributes
 * for the segment.
 */
struct item_msg_priv {
    char * message_id;           /**< Message ID for the associated message */
    int speed;                   /**< The expected speed in km/h (`INT_MAX` for unlimited, 0 indicates
	                              *   that the road is closed) */
    int delay;                   /**< Expected delay for this segment, in 1/10 s */
    struct attr ** attrs;        /**< Additional attributes to add to the segment */
};

/**
 * @brief Implementation-specific item data for traffic map items
 */
struct item_priv {
    struct map_rect_priv * mr;  /**< The private data for the map rect from which the item was obtained */
    struct attr **attrs;        /**< The attributes for the item, `NULL`-terminated */
    struct coord *coords;       /**< The coordinates for the item */
    int coord_count;            /**< The number of elements in `coords` */
    int refcount;               /**< How many references to this item exist */
    GList * message_data;       /**< Message-specific data, see `struct item_msg_priv` */
    struct attr **next_attr;    /**< The next attribute of `item` to be returned by the `item_attr_get` method */
    unsigned int
    next_coord;    /**< The index of the next coordinate of `item` to be returned by the `item_coord_get` method */
    struct route *rt;           /**< The route to which the item has been added */
};

/**
 * @brief Parsed data for a cached item
 */
struct parsed_item {
    enum item_type type;        /**< The item type */
    int id_hi;                  /**< The high-order part of the identifier */
    int id_lo;                  /**< The low-order part of the identifier */
    int flags;                  /**< Access and other flags for the item */
    struct attr **attrs;        /**< The attributes for the item, `NULL`-terminated */
    struct coord *coords;       /**< The coordinates for the item */
    int coord_count;            /**< The number of elements in `coords` */
    int length;                 /**< The length of the segment in meters */
    int is_matched;             /**< Whether any of the maps has a matching item */
};

/**
 * @brief Data for segments affected by a traffic message.
 *
 * Speed can be specified in three different ways:
 * \li `speed` replaces the maximum speed of the segment, if lower
 * \li `speed_penalty` subtracts the specified amount from the maximum speed of the segment
 * \li `speed_factor` is the percentage of the maximum speed of the segment to be assumed
 *
 * Where more than one of these values is set, the lowest speed applies.
 */
struct seg_data {
    enum item_type type;         /**< The item type; currently only `type_traffic_distortion` is supported */
    int speed;                   /**< The expected speed in km/h (`INT_MAX` for unlimited, 0 indicates
	                              *   that the road is closed) */
    int speed_penalty;           /**< Difference between expected speed and the posted speed limit of
	                              *   the segment (0 for none); the resulting maximum speed is never
	                              *   less than 5 km/h */
    int speed_factor;            /**< Expected speed expressed as a percentage of the posted limit (100
	                              *   for full speed) */
    int delay;                   /**< Expected delay for all segments combined, in 1/10 s */
    enum location_dir dir;       /**< Directionality */
    int flags;                   /**< Access flags (modes of transportation to which the message applies) */
    struct attr ** attrs;        /**< Additional attributes to add to the segments */
};

struct point_data {
    struct route_graph_point * p; /**< The point in the route graph */
    int score;                   /**< The attribute matching score */
};

/**
 * @brief State for the XML parser.
 *
 * Several members of this struct are used to cache traffic data model objects until they can be
 * incorporated in a message.
 *
 * All `struct traffic_point` members are reset to NULL when the `location` member is set. Likewise, the
 * `si` member is reset to NULL when a new event is added. The `location` and `events` members are reset
 * to NULL when a message is created.
 */
struct xml_state {
    GList * messages;                   /**< Messages read so far */
    GList * tagstack;                   /**< Currently open tags (order is bottom to top) */
    int is_valid;                       /**< Whether `tagstack` represents a hierarchy of elements we recognize */
    int is_opened;                      /**< True if we have just opened an element;
	                                     *   false if child elements have been opened and closed since */
    struct traffic_point * at;          /**< The point for a point location, NULL for linear locations. */
    struct traffic_point * from;        /**< The start of a linear location, or a point before `at`. */
    struct traffic_point * to;          /**< The end of a linear location, or a point after `at`. */
    struct traffic_point * via;         /**< A point between `from` and `to`. Required on ring roads
	                                     *   unless `not_via` is used; cannot be used together with `at`. */
    struct traffic_point * not_via;     /**< A point NOT between `from` and `to`. Required on ring roads
	                                     *   unless `via` is used; cannot be used together with `at`. */
    struct traffic_location * location; /**< The location to which the next message refers. */
    char * location_txt_data;           /**< Persisted location data in txtfile map format. */
    GList * si;                         /**< Supplementary information items for the next event. */
    GList * events;                     /**< The events for the next message. */
};

/**
 * @brief Data for an XML element
 *
 * `names` and `values` are always two separate arrays for this struct, regardless of what is indicated by
 * `XML_ATTR_DISTANCE`.
 */
struct xml_element {
    char * tag_name;             /**< The tag name */
    char ** names;               /**< Attribute names */
    char ** values;              /**< Attribute values (indices correspond to `names`) */
    char * text;                 /**< Character data (NULL-terminated) */
};

static struct map_methods traffic_map_meth;

static struct seg_data * seg_data_new(void);
static struct item * tm_add_item(struct map *map, enum item_type type, int id_hi, int id_lo,
                                 int flags, struct attr **attrs, struct coord *c, int count, char * id);
#ifdef TRAFFIC_DEBUG
static void tm_dump_item_to_textfile(struct item * item);
#endif
static void tm_destroy(struct map_priv *priv);
static void tm_coord_rewind(void *priv_data);
static void tm_item_destroy(struct item * item);
static struct item * tm_item_ref(struct item * item);
static struct item * tm_item_unref(struct item * item);
static void tm_item_update_attrs(struct item * item, struct route * route);
static int tm_coord_get(void *priv_data, struct coord *c, int count);
static void tm_attr_rewind(void *priv_data);
static int tm_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr);
static int tm_type_set(void *priv_data, enum item_type type);
static struct map_selection * traffic_location_get_rect(struct traffic_location * this_, enum projection projection);
static struct route_graph * traffic_location_get_route_graph(struct traffic_location * this_,
        struct mapset * ms);
static int traffic_location_match_attributes(struct traffic_location * this_, struct item *item);
static int traffic_message_add_segments(struct traffic_message * this_, struct mapset * ms, struct seg_data * data,
                                        struct map *map, struct route * route);
static int traffic_message_restore_segments(struct traffic_message * this_, struct mapset * ms,
        struct map *map, struct route * route);
static void traffic_location_populate_route_graph(struct traffic_location * this_, struct route_graph * rg,
        struct mapset * ms);
static void traffic_location_set_enclosing_rect(struct traffic_location * this_, struct coord_geo ** coords);
static void traffic_dump_messages_to_xml(struct traffic_shared_priv * shared);
static void traffic_loop(struct traffic * this_);
static struct traffic * traffic_new(struct attr *parent, struct attr **attrs);
static int traffic_process_messages_int(struct traffic * this_, int flags);
static void traffic_message_dump_to_stderr(struct traffic_message * this_);
static struct seg_data * traffic_message_parse_events(struct traffic_message * this_);
static struct route_graph_point * traffic_route_flood_graph(struct route_graph * rg, struct seg_data * data,
        struct coord * c_start, struct coord * c_dst, struct route_graph_point * start_existing);
static void traffic_message_remove_item_data(struct traffic_message * old, struct traffic_message * new,
        struct route * route);

static struct item_methods methods_traffic_item = {
    tm_coord_rewind,
    tm_coord_get,
    tm_attr_rewind,
    tm_attr_get,
    NULL,
    NULL,
    NULL,
    tm_type_set,
};

/**
 * @brief Creates a Boolean value from its string representation.
 *
 * If the string equals `true`, `yes` or can be parsed to a nonzero integer, the result is true.
 *
 * If the string equals `false`, `no` or begins with the digit 0 and returns zero when parsed to an
 * integer, the result is false.
 *
 * If NULL is supplied, or if the string does not match any known value, the result is the default value.
 *
 * String comparison is case-insensitive.
 *
 * Since true is always represented by a return value of 1, passing a `deflt` other than 0 or 1 allows
 * the caller to determine if the string could be parsed correctly.
 *
 * @param string The string representation
 * @param deflt The default value to return if `string` is not a valid representation of a Boolean value.
 *
 * @return The corresponding `enum event_class`, or `event_class_invalid` if `string` does not match a
 * known identifier
 */
static int boolean_new(const char * string, int deflt) {
    if (!string)
        return deflt;
    if (!g_ascii_strcasecmp(string, "yes") || !g_ascii_strcasecmp(string, "true") || atoi(string))
        return 1;
    if (!g_ascii_strcasecmp(string, "no") || !g_ascii_strcasecmp(string, "false") || ((string[0] == '0') && !atoi(string)))
        return 0;
    return deflt;
}

/**
 * @brief Destructor for `struct parsed_item`
 *
 * This frees up the `struct parsed_item` and all associated data. The pointer passed to this function will
 * be invalid after it returns.
 */
static void parsed_item_destroy(struct parsed_item * this_) {
    g_free(this_->coords);
    attr_list_free(this_->attrs);
    g_free(this_);
}

/**
 * @brief Creates a new `struct seg_data` and initializes it with default values.
 */
static struct seg_data * seg_data_new(void) {
    struct seg_data * ret = g_new0(struct seg_data, 1);
    ret->type = type_traffic_distortion;
    ret->speed = INT_MAX;
    ret->speed_factor = 100;
    return ret;
}

/**
 * @brief Creates a timestamp from its ISO8601 representation.
 *
 * @param string The ISO8601 timestamp
 *
 * @return The timestamp, or 0 if `string` is NULL.
 */
static time_t time_new(char * string) {
    if (!string)
        return 0;
    return iso8601_to_time(string);
}

/**
 * @brief Whether two `struct seg_data` contain the same data.
 *
 * @return true if `l` and `r` are equal, false if not. Two NULL values are considered equal; a NULL value and a
 * non-NULL value are not.
 */
static int seg_data_equals(struct seg_data * l, struct seg_data * r) {
    struct attr ** attrs;
    struct attr * attr;

    if (!l && !r)
        return 0;
    else if (!l || !r)
        return 1;
    if (l->type != r->type)
        return 0;
    if (l->speed != r->speed)
        return 0;
    if (l->speed_penalty != r->speed_penalty)
        return 0;
    if (l->speed_factor != r->speed_factor)
        return 0;
    if (l->delay != r->delay)
        return 0;
    if (l->dir != r->dir)
        return 0;
    if (l->flags != r->flags)
        return 0;
    if (!l->attrs && !r->attrs)
        return 1;
    if (!l->attrs || !r->attrs)
        return 0;
    /* FIXME this will break if multiple attributes of the same type are present and have different values */
    for (attrs = l->attrs; attrs; attrs++) {
        attr = attr_search(r->attrs, (*attrs)->type);
        if (!attr || (attr->u.data != (*attrs)->u.data))
            return 0;
    }
    for (attrs = r->attrs; attrs; attrs++) {
        attr = attr_search(l->attrs, (*attrs)->type);
        if (!attr || (attr->u.data != (*attrs)->u.data))
            return 0;
    }
    return 1;
}

/**
 * @brief Adds message data to a traffic map item.
 *
 * This method checks if the item already has data for the message specified in `msgid`. If so, the
 * existing data is updated, else a new entry is added.
 *
 * Data changes also trigger an update of the affected item’s attributes.
 *
 * @param item The item (its `priv_data` member must point to a `struct item_priv`)
 * @param msgid The message ID
 * @param speed The maximum speed for the segment (`INT_MAX` if none given)
 * @param delay The delay for the segment, in tenths of seconds (0 for none)
 * @param attrs Additional attributes specified by the message, can be NULL
 * @param route The route affected by the changes
 *
 * @return true if data was changed, false if not
 */
static int tm_item_add_message_data(struct item * item, char * msgid, int speed, int delay, struct attr ** attrs,
                                    struct route * route) {
    int ret = 0;
    struct item_priv * priv_data = item->priv_data;
    GList * msglist;
    struct item_msg_priv * msgdata;

    for (msglist = priv_data->message_data; msglist; msglist = g_list_next(msglist)) {
        msgdata = (struct item_msg_priv *) msglist->data;
        if (!strcmp(msgdata->message_id, msgid))
            break;
    }

    if (msglist) {
        /* we have an existing item, update it */
        ret |= ((msgdata->speed != speed) || (msgdata->delay != delay));
        msgdata->speed = speed;
        msgdata->delay = delay;
        /* TODO attrs */
    } else {
        ret = 1;
        /* we need to insert a new item */
        msgdata = g_new0(struct item_msg_priv, 1);
        msgdata->message_id = g_strdup(msgid);
        msgdata->speed = speed;
        msgdata->delay = delay;
        /* TODO attrs */
        priv_data->message_data = g_list_append(priv_data->message_data, msgdata);
    }

    if (ret)
        tm_item_update_attrs(item, route);

    return ret;
}

/**
 * @brief Destroys a traffic map item.
 *
 * This function should never be called directly. Instead, be sure to obtain all references by calling
 * `tm_item_ref()` and destroying them by calling `tm_item_unref()`.
 *
 * @param item The item (its `priv_data` member must point to a `struct item_priv`)
 */
static void tm_item_destroy(struct item * item) {
    struct item_priv * priv_data = item->priv_data;
    GList * msglist;
    struct item_msg_priv * msgdata;

    attr_list_free(priv_data->attrs);
    g_free(priv_data->coords);

    for (msglist = priv_data->message_data; msglist; msglist = g_list_remove(msglist, msglist->data)) {
        msgdata = (struct item_msg_priv *) msglist->data;
        g_free(msgdata->message_id);
        attr_list_free(msgdata->attrs);
        g_free(msgdata);
    }

    g_free(item->priv_data);
    g_free(item);
}

/**
 * @brief References a traffic map item.
 *
 * Storing a reference to a traffic map item should always be done by calling this function, passing the
 * item as its argument. This will return the item and increase its reference count by one.
 *
 * Never store a pointer to a traffic item not obtained via this function. Doing so may have undesired
 * side effects as the item will not be aware of the reference to it, and the reference may unexpectedly
 * become invalid, leading to a segmentation fault.
 *
 * @param item The item (its `priv_data` member must point to a `struct item_priv`)
 *
 * @return The item. `NULL` will be returned if the argument is `NULL` or points to an item whose
 * `priv_data` member is `NULL`.
 */
static struct item * tm_item_ref(struct item * item) {
    if (!item)
        return NULL;
    if (!item->priv_data)
        return NULL;
    ((struct item_priv *) item->priv_data)->refcount++;
    return item;
}

/**
 * @brief Unreferences a traffic map item.
 *
 * This must be called when destroying a reference to a traffic map item. It will decrease the reference
 * count of the item by one, and destroy the item if the last reference to is is removed.
 *
 * The map itself (and only the map) holds weak references to its items, which are not considered in the
 * reference count. Consequently, when the reference count reaches zero, the item is also removed from
 * the map.
 *
 * Unreferencing an item with a zero reference count (which is only possible for an item which has never
 * been referenced since its creation) is equivalent to dropping the last reference, i.e. it will destroy
 * the item.
 *
 * When the last reference is removed (or an item with a zero reference count is unreferenced) and the item’s `rt`
 * member is set (indicating the route to which the item was added), the item is removed from that route.
 *
 * If the unreference operation is successful, this function returns `NULL`. This allows one-line
 * operations such as:
 *
 * {@code some_item = tm_item_unref(some_item);}
 *
 * @param item The item (its `priv_data` member must point to a `struct item_priv`)
 *
 * @return `NULL` if the item was unreferenced successfully, `item` if it points to an item whose
 * `priv_data` member is `NULL`.
 */
static struct item * tm_item_unref(struct item * item) {
    struct item_priv * priv_data;
    struct map_rect * mr;
    struct item * mapitem;
    if (!item)
        return item;
    if (!item->priv_data)
        return item;
    priv_data = (struct item_priv *) item->priv_data;
    priv_data->refcount--;
    if (priv_data->refcount <= 0) {
        if (priv_data->rt)
            route_remove_traffic_distortion(priv_data->rt, item);
        mr = map_rect_new(item->map, NULL);
        do {
            mapitem = map_rect_get_item(mr);
        } while (mapitem && (mapitem != item));
        if (mapitem)
            item_type_set(mapitem, type_none);
        map_rect_destroy(mr);
        tm_item_destroy(item);
    }
    return NULL;
}

/**
 * @brief Updates the attributes of an item.
 *
 * This method must be called after changing the message data associated with an item, i.e. adding,
 * removing or modifying message data.
 *
 * @param item The item
 * @param route The route affected by the changes
 */
static void tm_item_update_attrs(struct item * item, struct route * route) {
    struct item_priv * priv_data = (struct item_priv *) item->priv_data;
    GList * msglist;
    struct item_msg_priv * msgdata;
    int speed = INT_MAX;
    int delay = 0;
    struct attr * attr = NULL;
    int has_changes = 0;

    for (msglist = priv_data->message_data; msglist; msglist = g_list_next(msglist)) {
        msgdata = (struct item_msg_priv *) msglist->data;
        if (msgdata->speed < speed)
            speed = msgdata->speed;
        if (msgdata->delay > delay)
            delay = msgdata->delay;
        /* TODO attrs */
    }

    if (!priv_data->attrs)
        priv_data->attrs = g_new0(struct attr *, 1);

    /* TODO maxspeed vs. delay:
     * Currently both values are interpreted as being cumulative, which may give erroneous results.
     * Consider a segment with a length of 1000 m and a maxspeed of 120 km/h, thus having a cost of 30 s.
     * One message reports a maxspeed of 60 km/h and no delay, increasing the cost to 60 s. A second
     * message reports no maxspeed but a delay of 30 s, also increasing the cost to 60 s. Both messages
     * together would be interpreted as reducing the maxspeed to 60 km/h and adding a delay of 30 s,
     * resulting in a cost of 90 s for the segment.
     */
    if (speed < INT_MAX) {
        attr = attr_search(priv_data->attrs, attr_maxspeed);
        if (!attr) {
            attr = g_new0(struct attr, 1);
            attr->type = attr_maxspeed;
            attr->u.num = speed;
            priv_data->attrs = attr_generic_add_attr(priv_data->attrs, attr);
            g_free(attr);
            attr = NULL;
            has_changes = 1;
        } else if (speed < attr->u.num) {
            has_changes = 1;
            attr->u.num = speed;
        } else if (speed > attr->u.num) {
            has_changes = 1;
            attr->u.num = speed;
        }
    } else {
        while ((attr = attr_search(priv_data->attrs, attr_maxspeed)))
            priv_data->attrs = attr_generic_remove_attr(priv_data->attrs, attr);
    }

    if (delay) {
        attr = attr_search(priv_data->attrs, attr_delay);
        if (!attr) {
            attr = g_new0(struct attr, 1);
            attr->type = attr_delay;
            attr->u.num = delay;
            priv_data->attrs = attr_generic_add_attr(priv_data->attrs, attr);
            g_free(attr);
            attr = NULL;
            has_changes = 1;
        } else if (delay > attr->u.num) {
            has_changes = 1;
            attr->u.num = delay;
        } else if (delay < attr->u.num) {
            has_changes = 1;
            attr->u.num = delay;
        }
    } else {
        while (1) {
            attr = attr_search(priv_data->attrs, attr_delay);
            if (!attr)
                break;
            priv_data->attrs = attr_generic_remove_attr(priv_data->attrs, attr);
        }
    }

    if (has_changes) {
        if (!priv_data->rt) {
            priv_data->rt = route;
            route_add_traffic_distortion(priv_data->rt, item);
        } else
            route_change_traffic_distortion(priv_data->rt, item);
    }
}

/**
 * @brief Returns an item from the map which matches the supplied data.
 *
 * Comparison criteria are as follows:
 *
 * \li The item type must match
 * \li Start and end coordinates must match (inverted coordinates will also match)
 * \li If `attr_flags` is supplied in `attrs`, the item must have this attribute and the rules listed
 * below are applied
 * \li Flags in `AF_ALL` must match
 * \li Flags in `AF_ONEWAYMASK` must be set either on both sides or neither side
 * \li If set, flags in `AF_ONEWAYMASK` must effectively match (equal for same direction, inverted for
 * opposite directions)
 * \li Other attributes are currently ignored
 *
 * This is due to the way different reports for the same segment are handled:
 *
 * \li If multiple reports with the same access flags exist, one item is created; speed and delay are
 * evaluated across all currently active reports in `tm_item_update_attrs()` (lowest speed and longest
 * delay wins)
 * \li If multiple reports exist and access flags differ, one item is created for each set of flags;
 * items are deduplicated in `route_get_traffic_distortion()`
 *
 * @param mr A map rectangle in the traffic map
 * @param type Type of the item
 * @param attrs The attributes for the item
 * @param c Points to an array of coordinates for the item
 * @param count Number of items in `c`
 */
static struct item * tm_find_item(struct map_rect *mr, enum item_type type, struct attr **attrs,
                                  struct coord *c, int count) {
    struct item * ret = NULL;
    struct item * curr;
    struct item_priv * curr_priv;
    struct attr wanted_flags_attr, curr_flags_attr;

    while ((curr = map_rect_get_item(mr)) && !ret) {
        if (curr->type != type)
            continue;
        if (attr_generic_get_attr(attrs, NULL, attr_flags, &wanted_flags_attr, NULL)) {
            if (!item_attr_get(curr, attr_flags, &curr_flags_attr))
                continue;
            item_attr_rewind(curr);
            if ((wanted_flags_attr.u.num & AF_ALL) != (curr_flags_attr.u.num & AF_ALL))
                continue;
            continue;
        } else
            wanted_flags_attr.type = attr_none;
        curr_priv = curr->priv_data;
        if (curr_priv->coords[0].x == c[0].x && curr_priv->coords[0].y == c[0].y
                && curr_priv->coords[curr_priv->coord_count-1].x == c[count-1].x
                && curr_priv->coords[curr_priv->coord_count-1].y == c[count-1].y) {
            if (wanted_flags_attr.type == attr_none) {
                /* no flag comparison, match */
            } else if ((wanted_flags_attr.u.num & AF_ONEWAYMASK) != (curr_flags_attr.u.num & AF_ONEWAYMASK))
                /* different oneway restrictions, no match */
                continue;
            ret = curr;
        } else if (curr_priv->coords[0].x == c[count-1].x
                   && curr_priv->coords[0].y == c[count-1].y
                   && curr_priv->coords[curr_priv->coord_count-1].x == c[0].x
                   && curr_priv->coords[curr_priv->coord_count-1].y == c[0].y) {
            if (wanted_flags_attr.type == attr_none) {
                /* no flag comparison, match */
            } else if (!(wanted_flags_attr.u.num & AF_ONEWAYMASK) && !(curr_flags_attr.u.num & AF_ONEWAYMASK)) {
                /* two bidirectional distortions, match */
            } else if (wanted_flags_attr.u.num & curr_flags_attr.u.num & AF_ONEWAYMASK) {
                /* oneway in opposite directions, no match */
                continue;
            } else if ((wanted_flags_attr.u.num ^ AF_ONEWAYMASK) & curr_flags_attr.u.num & AF_ONEWAYMASK) {
                /* oneway in same direction, match */
            } else {
                continue;
            }
            ret = curr;
        }
    }
    return ret;
}

/**
 * @brief Dumps an item to a file in textfile format.
 *
 * All data passed to this method is safe to free after the method returns, and doing so is the
 * responsibility of the caller.
 *
 * @param item The item
 * @param f The file to write to
 */
static void tm_item_dump_to_file(struct item * item, FILE * f) {
    struct item_priv * ip = (struct item_priv *) item->priv_data;
    struct attr **attrs = ip->attrs;
    struct coord *c = ip->coords;
    int i;
    char * attr_text;

    fprintf(f, "type=%s", item_to_name(item->type));
    fprintf(f, " id=0x%x,0x%x", item->id_hi, item->id_lo);
    while (*attrs) {
        if ((*attrs)->type == attr_flags) {
            /* special handling for flags */
            fprintf(f, " flags=0x%x", (unsigned int)(*attrs)->u.num);
        } else {
            attr_text = attr_to_text(*attrs, NULL, 0);
            /* FIXME this may not work properly for all attribute types */
            fprintf(f, " %s=%s", attr_to_name((*attrs)->type), attr_text);
            g_free(attr_text);
        }
        attrs++;
    }
    fprintf(f, "\n");

    for (i = 0; i < ip->coord_count; i++) {
        fprintf(f,"0x%x 0x%x\n", c[i].x, c[i].y);
    }
}

#ifdef TRAFFIC_DEBUG
/**
 * @brief Dumps an item to a textfile map.
 *
 * This method writes the item to a textfile map named `distortion.txt` in the default data folder.
 * This map can be added to the active mapset in order for the distortions to be rendered on the map and
 * considered for routing.
 *
 * All data passed to this method is safe to free after the method returns, and doing so is the
 * responsibility of the caller.
 *
 * @param item The item
 */
static void tm_dump_item_to_textfile(struct item * item) {
    struct item_priv * ip = (struct item_priv *) item->priv_data;
    struct attr **attrs = ip->attrs;
    struct coord *c = ip->coords;
    int i;
    char * attr_text;

    /* add the configuration directory to the name of the file to use */
    char *dist_filename = g_strjoin(NULL, navit_get_user_data_directory(TRUE),
                                    "/distortion.txt", NULL);
    if (dist_filename) {
        FILE *map = fopen(dist_filename,"a");
        if (map) {
            tm_item_dump_to_file(item, map);
            fclose(map);
        } else {
            dbg(lvl_error,"could not open file for distortions !!");

        } /* else - if (map) */
        g_free(dist_filename);			/* free the file name */
    } /* if (dist_filename) */
}

/**
 * @brief Dumps the traffic map to a textfile map.
 *
 * This method writes all items to a textfile map named `distortion.txt` in the default data folder.
 * This map can be added to the active mapset in order for the distortions to be rendered on the map and
 * considered for routing.
 *
 * @param map The traffic map
 */
static void tm_dump_to_textfile(struct map * map) {
    /* external method, verifies the public API as well as internal structure */
    struct map_rect * mr;
    struct item * item;

    mr = map_rect_new(map, NULL);
    while ((item = map_rect_get_item(mr)))
        tm_dump_item_to_textfile(item);
    map_rect_destroy(mr);
}
#endif

/**
 * @brief Adds an item to the map.
 *
 * If a matching item is already in the map, that item will be returned.
 *
 * All data passed to this method is safe to free after the method returns, and doing so is the
 * responsibility of the caller.
 *
 * @param map The traffic map
 * @param type Type of the item
 * @param id_hi First part of the ID of the item (item IDs have two parts)
 * @param id_lo Second part of the ID of the item
 * @param flags Flags used as a matching criterion, and added to newly-created items
 * @param attrs The attributes for the item
 * @param c Points to an array of coordinates for the item
 * @param count Number of items in `c`
 * @param id Message ID for the associated message
 *
 * @return The map item
 */
static struct item * tm_add_item(struct map *map, enum item_type type, int id_hi, int id_lo,
                                 int flags, struct attr **attrs, struct coord *c, int count, char * id) {
    struct item * ret = NULL;
    struct item_priv * priv_data;
    struct map_rect * mr;
    struct attr ** int_attrs = NULL;
    struct attr flags_attr;

    flags_attr.type = attr_flags;
    flags_attr.u.num = flags;
    int_attrs = attr_generic_set_attr(attr_list_dup(attrs), &flags_attr);

    mr = map_rect_new(map, NULL);
    ret = tm_find_item(mr, type, int_attrs, c, count);
    if (!ret) {
        ret = map_rect_create_item(mr, type);
        ret->id_hi = id_hi;
        ret->id_lo = id_lo;
        ret->map = map;
        ret->meth = &methods_traffic_item;
        priv_data = (struct item_priv *) ret->priv_data;
        priv_data->attrs = int_attrs;
        priv_data->coords = g_memdup(c, sizeof(struct coord) * count);
        priv_data->coord_count = count;
        priv_data->next_attr = int_attrs;
        priv_data->next_coord = 0;
    } else if (int_attrs) {
        /* free up our copy of the attribute list if we’re not attaching it to a new item */
        attr_list_free(int_attrs);
    }
    map_rect_destroy(mr);
    //tm_dump_item(ret);
    return ret;
}

/**
 * @brief Destroys (closes) the traffic map.
 *
 * @param priv The private data for the traffic map instance
 */
static void tm_destroy(struct map_priv *priv) {
    g_free(priv);
}

/**
 * @brief Opens a new map rectangle on the traffic map.
 *
 * This function opens a new map rectangle on the route graph's map.
 *
 * @param priv The traffic graph map's private data
 * @param sel The map selection (to restrict search to a rectangle, order and/or item types)
 * @return A new map rect's private data
 */
static struct map_rect_priv * tm_rect_new(struct map_priv *priv, struct map_selection *sel) {
    struct map_rect_priv * mr;

    /* Iterator over active messages */
    GList * msgiter;

    /* Current message */
    struct traffic_message * message;

    /* Map selection for current message, current map rect selection */
    struct map_selection * msg_sel, * rect_sel;

    /* Attributes for traffic distortions generated from the current traffic message */
    struct seg_data * data;

    /* Whether new segments have been added */
    int dirty = 0;

    dbg(lvl_debug,"enter");

    /* lazy location matching */
    if (sel != NULL)
        /* TODO experimental: if no selection is passed, do not resolve any locations */
        for (msgiter = priv->shared->messages; msgiter; msgiter = g_list_next(msgiter)) {
            message = (struct traffic_message *) msgiter->data;
            if (message->priv->items == NULL) {
                traffic_location_set_enclosing_rect(message->location, NULL);
                msg_sel = traffic_location_get_rect(message->location, traffic_map_meth.pro);
                for (rect_sel = sel; rect_sel; rect_sel = rect_sel->next)
                    if (coord_rect_overlap(&(msg_sel->u.c_rect), &(rect_sel->u.c_rect))) {
                        /* TODO do this in an idle loop, not here */
                        /* lazy cache restore */
                        if (message->location->priv->txt_data) {
                            dbg(lvl_debug, "location has txt_data, trying to restore");
                            traffic_message_restore_segments(message, priv->shared->ms,
                                                             priv->shared->map, priv->shared->rt);
                        } else {
                            dbg(lvl_debug, "location has no txt_data, nothing to restore");
                        }
                        /* if cache restore yielded no items, expand from scratch */
                        if (message->priv->items == NULL) {
                            data = traffic_message_parse_events(message);
                            traffic_message_add_segments(message, priv->shared->ms, data, priv->shared->map, priv->shared->rt);
                            g_free(data);
                        }
                        dirty = 1;
                        break;
                    }
                map_selection_destroy(msg_sel);
            }
        }

    mr=g_new0(struct map_rect_priv, 1);
    mr->mpriv = priv;
    mr->next_item = priv->items;
    /* all other pointers are initially NULL */

    if (dirty)
        /* dump message store if new messages have been received */
        traffic_dump_messages_to_xml(priv->shared);
    return mr;
}

/**
 * @brief Destroys a map rectangle on the traffic map.
 */
static void tm_rect_destroy(struct map_rect_priv *mr) {
    /* just free the map_rect_priv, all its members are pointers to data "owned" by others */
    g_free(mr);
}

/**
 * @brief Returns the next item from the traffic map
 *
 * @param mr The map rect to search for items
 *
 * @return The next item, or `NULL` if the last item has already been retrieved.
 */
static struct item * tm_get_item(struct map_rect_priv *mr) {
    struct item * ret = NULL;
    struct item_priv * ip;

    if (mr->item) {
        ip = (struct item_priv *) mr->item->priv_data;
        ip->mr = NULL;
    }
    if (mr->next_item) {
        ret = (struct item *) mr->next_item->data;
        ip = (struct item_priv *) ret->priv_data;
        ip->mr = mr;
        tm_attr_rewind(ret->priv_data);
        tm_coord_rewind(ret->priv_data);
        mr->next_item = g_list_next(mr->next_item);
    }

    mr->item = ret;
    return ret;
}

/**
 * @brief Returns the next item with the supplied ID from the traffic map
 *
 * @param mr The map rect to search for items
 * @param id_hi The high-order portion of the ID
 * @param id_lo The low-order portion of the ID
 *
 * @return The next item matching the ID; `NULL` if there are no matching items or the last matching
 * item has already been retrieved.
 */
static struct item * tm_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo) {
    struct item *ret = NULL;
    do {
        ret = tm_get_item(mr);
    } while (ret && (ret->id_lo != id_lo || ret->id_hi != id_hi));
    return ret;
}

/**
 * @brief Creates a new item of the specified type and inserts it into the map.
 *
 * @param mr The map rect in which to create the item
 * @param type The type of item to create
 *
 * @return The new item. The item is of type `type` and has an allocated `priv_data` member; all other
 * members of both structs are `NULL`.
 */
static struct item * tm_rect_create_item(struct map_rect_priv *mr, enum item_type type) {
    struct map_priv * map_priv = mr->mpriv;
    struct item * ret = NULL;
    struct item_priv * priv_data;

    priv_data = g_new0(struct item_priv, 1);

    ret = g_new0(struct item, 1);
    ret->type = type;
    ret->priv_data = priv_data;
    map_priv->items = g_list_append(map_priv->items, ret);

    return ret;
}


/**
 * @brief Gets an attribute from the traffic map
 *
 * This only supports the `attr_traffic` attribute, which is currently only used for the purpose of
 * identifying the map as a traffic map. Note, however, that for now the attribute will have a null pointer.
 *
 * @param map_priv Private data of the traffic map
 * @param type The type of the attribute to be read
 * @param attr Pointer to an attrib-structure where the attribute should be written to
 * @return True if the attribute type was found, false if not
 */
static int tm_get_attr(struct map_priv *priv, enum attr_type type, struct attr *attr) {
    if (attr_type == attr_traffic) {
        attr->type = attr_traffic;
        attr->u.traffic = NULL;
        return 1;
    } else
        return 0;
}

/**
 * @brief Rewinds the coordinates of the currently selected item.
 *
 * After rewinding, the next call to the `tm_coord_get()` will return the first coordinate of the
 * current item.
 *
 * @param priv_data The item's private data
 */
static void tm_coord_rewind(void *priv_data) {
    struct item_priv * ip = priv_data;

    ip->next_coord = 0;
}

/**
 * @brief Returns the coordinates of a traffic item.
 *
 * @param priv_data The item's private data
 * @param c Pointer to a `struct coord` array where coordinates will be stored
 * @param count The maximum number of coordinates to retrieve (must be less than or equal to the number
 * of items `c` can hold)
 * @return The number of coordinates retrieved
 */
static int tm_coord_get(void *priv_data, struct coord *c, int count) {
    struct item_priv * ip = priv_data;
    int ret = count;

    if (!ip)
        return 0;
    if (ip->next_coord >= ip->coord_count)
        return 0;
    if (ip->next_coord + count > ip->coord_count)
        ret = ip->coord_count - ip->next_coord;
    memcpy(c, &ip->coords[ip->next_coord], ret * sizeof(struct coord));
    ip->next_coord += ret;
    return ret;
}

/**
 * @brief Rewinds the attributes of the currently selected item.
 *
 * After rewinding, the next call to `tm_attr_get()` will return the first attribute.
 *
 * @param priv_data The item's private data
 */
static void tm_attr_rewind(void *priv_data) {
    struct item_priv * ip = priv_data;

    ip->next_attr = ip->attrs;
}

/**
 * @brief Returns the next attribute of a traffic item which matches the specified type.
 *
 * @param priv_data The item's private data
 * @param attr_type The attribute type to retrieve, or `attr_any` to retrieve the next attribute,
 * regardless of type
 * @param attr Receives the attribute
 *
 * @return True on success, false on failure
 */
static int tm_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr) {
    struct item_priv * ip = priv_data;
    int ret = 0;

    if (!ip->next_attr)
        return 0;
    while (*(ip->next_attr) && !ret) {
        ret = (attr_type == attr_any) || (attr_type == (*(ip->next_attr))->type);
        if (ret)
            attr_dup_content(*(ip->next_attr), attr);
        ip->next_attr++;
    }
    return ret;
}

/**
 * @brief Sets the type of a traffic item.
 *
 * @param priv_data The item's private data
 * @param type The new type for the item. Setting it to `type_none` deletes the item from the map.
 *
 * @return 0 on failure, nonzero on success
 */
static int tm_type_set(void *priv_data, enum item_type type) {
    struct item_priv * ip = priv_data;

    if (!ip->mr || !ip->mr->item || (ip->mr->item->priv_data != priv_data)) {
        dbg(lvl_error, "this function can only be called for the last item retrieved from its map rect");
        return 0;
    }

    if (type == type_none) {
        /* if we have multiple occurrences of this item in the list, move forward beyond the last one */
        while (ip->mr->next_item && (ip->mr->next_item->data == ip->mr->item))
            ip->mr->next_item = g_list_next(ip->mr->next_item);

        /* remove the item from the map and set last retrieved item to NULL */
        ip->mr->mpriv->items = g_list_remove_all(ip->mr->mpriv->items, ip->mr->item);
        ip->mr->item = NULL;
    } else {
        ip->mr->item->type = type;
    }

    return 1;
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
    tm_rect_create_item, /* map_rect_create_item: Create a new item in the map */
    tm_get_attr,      /* map_get_attr */
    NULL,             /* map_set_attr */
};

/**
 * @brief Whether the contents of an event are valid.
 *
 * This identifies any malformed events in which mandatory members are not set.
 *
 * @return true if the event is valid, false if it is malformed
 */
static int traffic_event_is_valid(struct traffic_event * this_) {
    if (!this_->event_class || !this_->type) {
        dbg(lvl_debug, "event_class (%d) or type (%d) are unknown", this_->event_class, this_->type);
        return 0;
    }
    switch (this_->event_class) {
    case event_class_congestion:
        if ((this_->type < event_congestion_cleared) || (this_->type >= event_delay_clearance)) {
            dbg(lvl_debug, "illegal type (%d) for event_class_congestion", this_->type);
            return 0;
        }
        break;
    case event_class_delay:
        if ((this_->type < event_delay_clearance)
                || (this_->type >= event_restriction_access_restrictions_lifted)) {
            dbg(lvl_debug, "illegal type (%d) for event_class_delay", this_->type);
            return 0;
        }
        break;
    case event_class_restriction:
        if ((this_->type < event_restriction_access_restrictions_lifted)
                || (this_->type > event_restriction_speed_limit_lifted)) {
            dbg(lvl_debug, "illegal type (%d) for event_class_restriction", this_->type);
            return 0;
        }
        break;
    default:
        dbg(lvl_debug, "unknown event class %d", this_->event_class);
        return 0;
    }
    if (this_->si_count && !this_->si) {
        dbg(lvl_debug, "si_count=%d but no supplementary information", this_->si_count);
        return 0;
    }
    /* TODO check SI */
    return 1;
}

/**
 * @brief Determines the degree to which the attributes of a location and a map item match.
 *
 * The result of this method is used to match a location to a map item. Its result is a score—the higher
 * the score, the better the match.
 *
 * To calculate the score, all supplied attributes are examined and points are given for each attribute
 * which is defined for the location. An exact match adds 4 points, a partial match adds 2. Values of 1
 * and 3 are added where additional granularity is needed. The number of points attained is divided by
 * the maximum number of points attainable, and the result is returned as a percentage value.
 *
 * If no points can be attained (because no attributes which must match are supplied), the score is 100
 * for any item supplied.
 *
 * @param this_ The location
 * @param item The map item
 *
 * @return The score, as a percentage value
 */
static int traffic_location_match_attributes(struct traffic_location * this_, struct item *item) {
    int score = 0;
    int maxscore = 0;
    struct attr attr;

    /* road type */
    if ((this_->road_type != type_line_unspecified)) {
        maxscore += 400;
        if (item->type == this_->road_type)
            score += 400;
        else
            switch (this_->road_type) {
            /* motorway */
            case type_highway_land:
                if (item->type == type_highway_city)
                    score += 300;
                else if (item->type == type_street_n_lanes)
                    score += 200;
                break;
            case type_highway_city:
                if (item->type == type_highway_land)
                    score += 300;
                else if (item->type == type_street_n_lanes)
                    score += 200;
                break;
            /* trunk */
            case type_street_n_lanes:
                if ((item->type == type_highway_land) || (item->type == type_highway_city)
                        || (item->type == type_street_4_land)
                        || (item->type == type_street_4_city))
                    score += 200;
                break;
            /* primary */
            case type_street_4_land:
                if (item->type == type_street_4_city)
                    score += 300;
                else if ((item->type == type_street_n_lanes)
                         || (item->type == type_street_3_land))
                    score += 200;
                else if (item->type == type_street_3_city)
                    score += 100;
                break;
            case type_street_4_city:
                if (item->type == type_street_4_land)
                    score += 300;
                else if ((item->type == type_street_n_lanes)
                         || (item->type == type_street_3_city))
                    score += 200;
                else if (item->type == type_street_3_land)
                    score += 100;
                break;
            /* secondary */
            case type_street_3_land:
                if (item->type == type_street_3_city)
                    score += 300;
                else if ((item->type == type_street_4_land)
                         || (item->type == type_street_2_land))
                    score += 200;
                else if ((item->type == type_street_4_city)
                         || (item->type == type_street_2_city))
                    score += 100;
                break;
            case type_street_3_city:
                if (item->type == type_street_3_land)
                    score += 300;
                else if ((item->type == type_street_4_city)
                         || (item->type == type_street_2_city))
                    score += 200;
                else if ((item->type == type_street_4_land)
                         || (item->type == type_street_2_land))
                    score += 100;
                break;
            /* tertiary */
            case type_street_2_land:
                if (item->type == type_street_2_city)
                    score += 300;
                else if (item->type == type_street_3_land)
                    score += 200;
                else if (item->type == type_street_3_city)
                    score += 100;
                break;
            case type_street_2_city:
                if (item->type == type_street_2_land)
                    score += 300;
                else if (item->type == type_street_3_city)
                    score += 200;
                else if (item->type == type_street_3_land)
                    score += 100;
                break;
            default:
                break;
            }
    }

    /* road_ref */
    if (this_->road_ref) {
        maxscore += 400;
        item_attr_rewind(item);
        if (item_attr_get(item, attr_street_name_systematic, &attr))
            score += (400 * (MAX_MISMATCH - compare_name_systematic(this_->road_ref, attr.u.str))) / MAX_MISMATCH;
    }

    /* road_name */
    if (this_->road_name) {
        maxscore += 200;
        item_attr_rewind(item);
        if (item_attr_get(item, attr_street_name, &attr)) {
            // TODO crude comparison in need of refinement
            if (!strcmp(this_->road_name, attr.u.str))
                score += 200;
        }
    }

    // TODO direction
    // TODO destination

    // TODO ramps

    if (!maxscore)
        return 100;
    return (score * 100) / maxscore;
}

/**
 * @brief Determines the degree to which the attributes of a point and a map item match.
 *
 * The result of this method is used to match a location to a map item. Its result is a score—the higher
 * the score, the better the match.
 *
 * To calculate the score, all supplied attributes are examined and points are given for each attribute
 * which is defined for the location. An exact match adds 4 points, a partial match adds 2. Values of 1
 * and 3 are added where additional granularity is needed. The number of points attained is divided by
 * the maximum number of points attainable, and the result is returned as a percentage value.
 *
 * If no points can be attained (because no attributes which must match are supplied), the score is 0
 * for any item supplied.
 *
 * @param this_ The traffic point
 * @param item The map item
 *
 * @return The score, as a percentage value
 */
static int traffic_point_match_attributes(struct traffic_point * this_, struct item *item) {
    int score = 0;
    int maxscore = 0;
    struct attr attr;

    /* junction_ref */
    if (this_->junction_ref) {
        maxscore += 400;
        item_attr_rewind(item);
        if (item_attr_get(item, attr_ref, &attr))
            score += (400 * (MAX_MISMATCH - compare_name_systematic(this_->junction_ref, attr.u.str))) / MAX_MISMATCH;
    }

    /* junction_name */
    if (this_->junction_name) {
        item_attr_rewind(item);
        if (item_attr_get(item, attr_label, &attr)) {
            maxscore += 400;
            // TODO crude comparison in need of refinement
            if (!strcmp(this_->junction_name, attr.u.str))
                score += 400;
        }
    }

    // TODO tmc_table, point->tmc_id

    if (!maxscore)
        return 0;
    return (score * 100) / maxscore;
}

/**
 * @brief Determines the degree to which the attributes of a point match those of the segments connecting to it.
 *
 * The result of this method is used to match a location to a map item. Its result is a score—the higher the score, the
 * better the match.
 *
 * To calculate the score, this method iterates over all segments which begin or end at `p`. The `junction_name`
 * member of the location is compared against the name of the segment, and the highest score for any segment is
 * returned. Currently the name must match completely, resulting in a score of 100, while everything else is considered
 * a mismatch (with a score of 0). Future versions may introduce support for partial matches, with the score indicating
 * the quality of the match.
 *
 * Segments which are part of the route are treated in a different manner, as the direction in which the segment is
 * traversed (not the direction of the segment itself) is taken into account, which is needed to govern whether the
 * matched segment ends up being part of the route or not.
 *
 * In some cases, `this_` refers to a point which is actually a segment (such as a bridge or tunnel), which we want to
 * include in the route. In other cases, `this_` refers to an intersection with another road, and the junction name is
 * the name of the other road; these segments need to be excluded from the route.
 *
 * This is controlled by the `match_start` argument: if true, we are evaluating the start point of a route, else we are
 * evaluating its end point. To include the matched segment in the route, only the first point (whose `seg` member
 * points to the segment) will match for the start point, the opposite is true for the end point. To exclude the
 * matched segment, this logic is reversed.
 *
 * A heuristic is in place to distinguish whether or not we want the matched segment included.
 *
 * If no points can be attained (because no attributes which must match are supplied), the score is 0 for any point.
 *
 * @param this_ The traffic point
 * @param p The point shared by all segments to examine
 * @param start The first point of the path
 * @param match_start True to evaluate for the start point of a route, false for the end point
 *
 * @return The score, as a percentage value
 */
static int traffic_point_match_segment_attributes(struct traffic_point * this_, struct route_graph_point *p,
        struct route_graph_point * start, int match_start) {

    /*
     * Whether we want a match for the route segment starting at p (leading away from it) or the route segment ending
     * at p (leading towards it).
     */
    int want_start_match = match_start;

    /* Iterator for route graph points */
    struct route_graph_point *p_iter = start;

    /* The predecessor pf `p`in the route graph */
    struct route_graph_point *p_prev = NULL;

    /*
     * Whether this_ matches the route segment starting at p (leading away from it), the route segment ending at p
     * (leading towards it), or an off-route segment connected to p, respectively
     */
    int has_start_match = 0, has_end_match = 0, has_offroute_match = 0;

    /* The route segment being examined */
    struct route_graph_segment *s;

    /* Map rect for retrieving item data */
    struct map_rect *mr;

    /* The item being examined */
    struct item * item;

    /* The attribute being examined */
    struct attr attr;

    /* Name and systematic name for route segments starting and ending at p */
    char *start_name = NULL, *start_ref = NULL, *end_name = NULL, *end_ref = NULL;

    /* Whether or not the route follows the road (if both are true or both are false, the case is not clear) */
    int route_follows_road = 0, route_leaves_road = 0;

    if (!this_->junction_name) {
        /* nothing to compare, score is 0 */
        dbg(lvl_debug, "p=%p: no junction name, score 0", p);
        return 0;
    }

    /* find predecessor of p, if any */
    while (p_iter && (p_iter != p)) {
        if (!p_iter->seg) {
            p_prev = NULL;
            break;
        }
        p_prev = p_iter;
        if (p_iter == p_iter->seg->start)
            p_iter = p_iter->seg->end;
        else
            p_iter = p_iter->seg->start;
    }

    if (!p_prev && (p != start)) {
        /* not a point on the route */
        dbg(lvl_debug, "p=%p: not on the route, score 0", p);
        return 0;
    }
    /* check if we have a match for the start of a route segment */
    if (p->seg) {
        mr = map_rect_new(p->seg->data.item.map, NULL);
        if ((item = map_rect_get_item_byid(mr, p->seg->data.item.id_hi, p->seg->data.item.id_lo))) {
            if (item_attr_get(item, attr_street_name, &attr)) {
                start_name = g_strdup(attr.u.str);
                // TODO crude comparison in need of refinement
                if (!strcmp(this_->junction_name, attr.u.str))
                    has_start_match = 1;
            }
            item_attr_rewind(item);
            if (item_attr_get(item, attr_street_name_systematic, &attr))
                start_ref = g_strdup(attr.u.str);
        }
        map_rect_destroy(mr);
    }

    /* check if we have a match for the end of a route segment */
    if (p_prev && p_prev->seg) {
        mr = map_rect_new(p_prev->seg->data.item.map, NULL);
        if ((item = map_rect_get_item_byid(mr, p_prev->seg->data.item.id_hi, p_prev->seg->data.item.id_lo))) {
            if (item_attr_get(item, attr_street_name, &attr)) {
                end_name = g_strdup(attr.u.str);
                // TODO crude comparison in need of refinement
                if (!strcmp(this_->junction_name, attr.u.str))
                    has_end_match = 1;
            }
            item_attr_rewind(item);
            if (item_attr_get(item, attr_street_name_systematic, &attr))
                end_ref = g_strdup(attr.u.str);
        }
        map_rect_destroy(mr);
    }

    /*
     * If we have both a start match and an end match, the point is in the middle of a stretch of road which matches
     * the junction name. Regardless of whether we want that stretch included in the route or not, a middle point
     * cannot be an end point.
     */
    if (has_start_match && has_end_match) {
        dbg(lvl_debug, "p=%p: both start and end match, score 0", p);
        g_free(start_name);
        g_free(start_ref);
        g_free(end_name);
        g_free(end_ref);
        return 0;
    }

    if (start_name && end_name)
        // TODO crude comparison in need of refinement
        route_follows_road |= !strcmp(start_name, end_name);

    if (start_ref && end_ref)
        route_follows_road |= !compare_name_systematic(start_ref, end_ref);

    /* check if we have a match for an off-route segment */
    /* TODO consolidate these two loops, which differ only in their loop statement while the body is identical */
    for (s = p->start; s && !(has_offroute_match && route_leaves_road); s = s->start_next) {
        if ((p->seg == s) || (p_prev && (p_prev->seg == s)))
            /* segments is on the route, skip */
            continue;
        mr = map_rect_new(s->data.item.map, NULL);
        if ((item = map_rect_get_item_byid(mr, s->data.item.id_hi, s->data.item.id_lo))) {
            if (item_attr_get(item, attr_street_name, &attr)) {
                // TODO crude comparison in need of refinement
                if (!strcmp(this_->junction_name, attr.u.str))
                    has_offroute_match = 1;
                if (start_name)
                    route_leaves_road |= !strcmp(start_name, attr.u.str);
                if (end_name)
                    route_leaves_road |= !strcmp(end_name, attr.u.str);
            }
            item_attr_rewind(item);
            if (!route_leaves_road && item_attr_get(item, attr_street_name_systematic, &attr)) {
                if (start_ref)
                    route_leaves_road |= !compare_name_systematic(start_ref, attr.u.str);
                if (end_ref)
                    route_leaves_road |= !compare_name_systematic(end_ref, attr.u.str);
            }
        }
        map_rect_destroy(mr);
    }

    for (s = p->end; s && !(has_offroute_match && route_leaves_road); s = s->end_next) {
        if ((p->seg == s) || (p_prev && (p_prev->seg == s)))
            /* segments is on the route, skip */
            continue;
        mr = map_rect_new(s->data.item.map, NULL);
        if ((item = map_rect_get_item_byid(mr, s->data.item.id_hi, s->data.item.id_lo))) {
            if (item_attr_get(item, attr_street_name, &attr)) {
                // TODO crude comparison in need of refinement
                if (!strcmp(this_->junction_name, attr.u.str))
                    has_offroute_match = 1;
                if (start_name)
                    route_leaves_road |= !strcmp(start_name, attr.u.str);
                if (end_name)
                    route_leaves_road |= !strcmp(end_name, attr.u.str);
            }
            item_attr_rewind(item);
            if (!route_leaves_road && item_attr_get(item, attr_street_name_systematic, &attr)) {
                if (start_ref)
                    route_leaves_road |= !compare_name_systematic(start_ref, attr.u.str);
                if (end_ref)
                    route_leaves_road |= !compare_name_systematic(end_ref, attr.u.str);
            }
        }
        map_rect_destroy(mr);
    }

    dbg(lvl_debug,
        "p=%p: %s %s → %s %s\nhas_offroute_match=%d, has_start_match=%d, has_end_match=%d, route_follows_road=%d, route_leaves_road=%d",
        p, end_ref, end_name, start_ref, start_name,
        has_offroute_match, has_start_match, has_end_match, route_follows_road, route_leaves_road);

    g_free(start_name);
    g_free(start_ref);
    g_free(end_name);
    g_free(end_ref);

    if (route_leaves_road && !route_follows_road)
        want_start_match = !match_start;
    /* TODO decide how to handle ambiguous situations (both true or both false), currently we include the segment */

    if (has_offroute_match) {
        if (has_start_match || has_end_match) {
            /* we cannot have multiple matches in different categories */
            /* TODO maybe we can: e.g. one segment of the crossing road got added to the route, the other did not */
            dbg(lvl_debug, "p=%p: both off-route and start/end match, score 0", p);
            return 0;
        }
    } else {
        if ((want_start_match && !has_start_match) || (!want_start_match && !has_end_match)) {
            /* no match in requested category */
            dbg(lvl_debug, "p=%p: no match in requested category, score 0", p);
            return 0;
        }
    }

    dbg(lvl_debug, "p=%p: score 100 (full score)", p);
    return 100;
}

/**
 * @brief Returns the cost of the segment in the given direction.
 *
 * The cost is calculated based on the length of the segment and a penalty which depends on the score.
 * A segment with the maximum score of 100 is not penalized, i.e. its cost is equal to its length. A
 * segment with a zero score is penalized with a factor of `PENALTY_SEGMENT_MATCH`. For scores in between, a
 * penalty factor between 1 and `PENALTY_SEGMENT_MATCH` is applied.
 *
 * If the segment is impassable in the given direction, the cost is always `INT_MAX`.
 *
 * @param over The segment
 * @param data Data for the segments added to the map
 * @param dir The direction (positive numbers indicate positive direction)
 *
 * @return The cost of the segment
 */
static int traffic_route_get_seg_cost(struct route_graph_segment *over, struct seg_data * data, int dir) {
    if (over->data.flags & (dir >= 0 ? AF_ONEWAYREV : AF_ONEWAY))
        return INT_MAX;
    if (dir > 0 && (over->start->flags & RP_TURN_RESTRICTION))
        return INT_MAX;
    if (dir < 0 && (over->end->flags & RP_TURN_RESTRICTION))
        return INT_MAX;
    if ((over->data.item.type < route_item_first) || (over->data.item.type > route_item_last))
        return INT_MAX;
    /* at least a partial match is required for access flags */
    if (!(over->data.flags & data->flags & AF_ALL))
        return INT_MAX;

    return over->data.len * (100 - over->data.score) * (PENALTY_SEGMENT_MATCH - 1) / 100 + over->data.len;
}

/**
 * @brief Determines the “point triple” for a traffic location.
 *
 * Each traffic location is defined by up to three points:
 * \li a start and end point, and an optional auxiliary point in between
 * \li a single point, with one or two auxiliary points (one before, one after)
 * \li a start and end point, and a third point which is outside the location
 *
 * This method determines these three points, puts them in the order in which they are encountered and
 * returns a bit field indicating the end points. If a point in the array is NULL or refers to an
 * auxiliary point, its corresponding bit is not set. The following values are returned:
 * \li 2: Point location, the middle point is the actual point
 * \li 3: Point-to-point location from the first to the second point; the third point is an auxiliary
 * point outside the location
 * \li 5: Point-to-point location from the first to the last point; the second point (if not NULL) is an
 * auxiliary point located in between
 * \li 6: Point-to-point location from the second to the third point; the first point is an auxiliary
 * point outside the location
 *
 * @param this_ The location
 * @param coords Points to an array which will receive pointers to the coordinates. The array must be
 * able to store three pointers.
 *
 * @return A bit field indicating the end points for the location
 */
static int traffic_location_get_point_triple(struct traffic_location * this_, struct coord_geo ** coords) {
    /* Which members of coords are the end points */
    int ret = 0;

    /* Projected coordinates */
    struct coord c_from, c_to, c_not_via;

    if (this_->at) {
        coords[0] = this_->from ? &this_->from->coord : NULL;
        coords[1] = &this_->at->coord;
        coords[2] = this_->to ? &this_->to->coord : NULL;
        ret = 1 << 1;
    } else if (this_->via) {
        coords[0] = this_->from ? &this_->from->coord : NULL;
        coords[1] = &this_->via->coord;
        coords[2] = this_->to ? &this_->to->coord : NULL;
        ret = (1 << 2) | (1 << 0);
    } else if (this_->not_via) {
        /*
         * If not_via is set, we calculate a route either for not_via-from-to or for from-to-not_via,
         * then trim the ends. The order of points is determined by the distance between not_via and the
         * other two points.
         */
        if (!this_->from || !this_->to) {
            coords[0] = NULL;
            coords[1] = NULL;
            coords[2] = NULL;
            return ret;
        }
        transform_from_geo(projection_mg, &this_->from->coord, &c_from);
        transform_from_geo(projection_mg, &this_->to->coord, &c_to);
        transform_from_geo(projection_mg, &this_->not_via->coord, &c_not_via);
        if (transform_distance(projection_mg, &c_from, &c_not_via)
                < transform_distance(projection_mg, &c_to, &c_not_via)) {
            coords[0] = &this_->not_via->coord;
            coords[1] = &this_->from->coord;
            coords[2] = &this_->to->coord;
        } else {
            coords[0] = &this_->from->coord;
            coords[1] = &this_->to->coord;
            coords[2] = &this_->not_via->coord;
        }
    } else {
        coords[0] = this_->from ? &this_->from->coord : NULL;
        coords[1] = NULL;
        coords[2] = this_->to ? &this_->to->coord : NULL;
        ret = (1 << 2) | (1 << 0);
    }
    return ret;
}

/**
 * @brief Sets the rectangle enclosing all points of a location
 *
 * @param this_ The traffic location
 * @param coords The point triple, can be NULL
 */
static void traffic_location_set_enclosing_rect(struct traffic_location * this_, struct coord_geo ** coords) {
    struct coord_geo * sw;
    struct coord_geo * ne;
    struct coord_geo * int_coords[] = {NULL, NULL, NULL};
    int i;

    if (this_->priv->sw && this_->priv->ne)
        return;

    if (!coords) {
        coords = &int_coords[0];
        traffic_location_get_point_triple(this_, coords);
    }

    if (!this_->priv->sw) {
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
        this_->priv->sw = sw;
    }

    if (!this_->priv->ne) {
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
        this_->priv->ne = ne;
    }
}

/**
 * @brief Obtains a map selection for the traffic location.
 *
 * The map selection is a rectangle around the points of the traffic location, with some extra padding as
 * specified in `ROUTE_RECT_DIST_REL` and `ROUTE_RECT_DIST_ABS`, with a map order specified in `ROUTE_ORDER`.
 *
 * @param this_ The traffic location
 * @param projection The projection to be used by coordinates of the selection (should correspond to the
 * projection of the target map)
 *
 * @return A map selection
 */
static struct map_selection * traffic_location_get_rect(struct traffic_location * this_, enum projection projection) {
    /* Corners of the enclosing rectangle, in Mercator coordinates */
    struct coord c1, c2;
    transform_from_geo(projection, this_->priv->sw, &c1);
    transform_from_geo(projection, this_->priv->ne, &c2);
    return route_rect(ROUTE_ORDER(this_->road_type), &c1, &c2, ROUTE_RECT_DIST_REL(this_->fuzziness),
                      ROUTE_RECT_DIST_ABS(this_->fuzziness));
}

/**
 * @brief Opens a map rectangle around the end points of the traffic location.
 *
 * Prior to calling this function, the caller must ensure `rg->m` points to the map to be used, and the enclosing
 * rectangle for the traffic location has been set (e.g. by calling `traffic_location_set_enclosing_rect()`).
 *
 * @param this_ The traffic location
 * @param rg The route graph
 *
 * @return NULL on failure, the map selection on success
 */
static struct map_rect * traffic_location_open_map_rect(struct traffic_location * this_, struct route_graph * rg) {
    /* Corners of the enclosing rectangle, in Mercator coordinates */
    rg->sel = traffic_location_get_rect(this_, map_projection(rg->m));
    if (!rg->sel)
        return NULL;
    rg->mr = map_rect_new(rg->m, rg->sel);
    if (!rg->mr) {
        map_selection_destroy(rg->sel);
        rg->sel = NULL;
    }
    return rg->mr;
}

/**
 * @brief Populates a route graph.
 *
 * This adds all routable segments in the enclosing rectangle of the location (plus a safety margin) to
 * the route graph.
 *
 * @param rg The route graph
 * @param ms The mapset to read the ramps from
 */
static void traffic_location_populate_route_graph(struct traffic_location * this_, struct route_graph * rg,
        struct mapset * ms) {
    /* The item being processed */
    struct item *item;

    /* Mercator coordinates of current and previous point */
    struct coord c, l;

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

    /* Default flags assumed for the current item type */
    int *default_flags;

    /* Holds an attribute retrieved from the current item */
    struct attr attr;

    /* Start and end point of the current way or segment */
    struct route_graph_point *s_pnt, *e_pnt;

    traffic_location_set_enclosing_rect(this_, NULL);

    rg->h = mapset_open(ms);

    while ((rg->m = mapset_next(rg->h, 2))) {
        /* Skip traffic map (identified by the `attr_traffic` attribute) */
        if (map_get_attr(rg->m, attr_traffic, &attr, NULL))
            continue;
        if (!traffic_location_open_map_rect(this_, rg))
            continue;
        while ((item = map_rect_get_item(rg->mr))) {
            if (item->type == type_street_turn_restriction_no || item->type == type_street_turn_restriction_only)
                route_graph_add_turn_restriction(rg, item);
            else if ((item->type < route_item_first) || (item->type > route_item_last))
                continue;
            /* If road class is motorway, trunk or primary, ignore roads more than one level below */
            if ((this_->road_type == type_highway_land) || (this_->road_type == type_highway_city)) {
                if ((item->type != type_highway_land) && (item->type != type_highway_city) &&
                        (item->type != type_street_n_lanes) && (item->type != type_ramp))
                    continue;
            } else if (this_->road_type == type_street_n_lanes) {
                if ((item->type != type_highway_land) && (item->type != type_highway_city) &&
                        (item->type != type_street_n_lanes) && (item->type != type_ramp) &&
                        (item->type != type_street_4_land) && (item->type != type_street_4_city))
                    continue;
            } else if ((this_->road_type == type_street_4_land) || (this_->road_type == type_street_4_city)) {
                if ((item->type != type_highway_land) && (item->type != type_highway_city) &&
                        (item->type != type_street_n_lanes) && (item->type != type_ramp) &&
                        (item->type != type_street_4_land) && (item->type != type_street_4_city) &&
                        (item->type != type_street_3_land) && (item->type != type_street_3_city))
                    continue;
            }
            if (item_get_default_flags(item->type)) {

                item_coord_rewind(item);
                if (item_coord_get(item, &l, 1)) {
                    data.score = traffic_location_match_attributes(this_, item);
                    data.flags=0;
                    data.offset=1;
                    data.maxspeed=-1;
                    data.item=item;
                    len = 0;
                    segmented = 0;

                    if (!(default_flags = item_get_default_flags(item->type)))
                        default_flags = &item_default_flags_value;
                    if (item_attr_get(item, attr_flags, &attr)) {
                        data.flags = attr.u.num;
                        segmented = (data.flags & AF_SEGMENTED);
                    } else
                        data.flags = *default_flags;

                    item_attr_rewind(item);
                    if ((data.flags & AF_SPEED_LIMIT) && (item_attr_get(item, attr_maxspeed, &attr)))
                        data.maxspeed = attr.u.num;

                    /* clear flags we're not copying here */
                    data.flags &= ~(AF_DANGEROUS_GOODS | AF_SIZE_OR_WEIGHT_LIMIT);

                    s_pnt = route_graph_add_point(rg, &l);

                    if (!segmented) {
                        while (item_coord_get(item, &c, 1)) {
                            len += transform_distance(map_projection(item->map), &l, &c);
                            l = c;
                        }
                        e_pnt = route_graph_add_point(rg, &l);
                        dbg_assert(len >= 0);
                        data.len = len;
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
        map_selection_destroy(rg->sel);
        rg->sel = NULL;
        map_rect_destroy(rg->mr);
        rg->mr = NULL;
    }
    route_graph_build_done(rg, 0);
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
static struct route_graph * traffic_location_get_route_graph(struct traffic_location * this_,
        struct mapset * ms) {
    struct route_graph *rg;

    traffic_location_set_enclosing_rect(this_, NULL);

    rg = g_new0(struct route_graph, 1);

    rg->done_cb = NULL;
    rg->busy = 1;

    /* build the route graph */
    traffic_location_populate_route_graph(this_, rg, ms);

    return rg;
}

/**
 * @brief Whether two traffic points are equal.
 *
 * Comparison is done solely on coordinates and requires a precise match. This can result in two points
 * being reported as not equal, even though the locations using these points may translate to the same
 * segments later.
 *
 * @return true if `l` and `r` are equal, false if not
 */
static int traffic_point_equals(struct traffic_point * l, struct traffic_point * r) {
    if (l->coord.lat != r->coord.lat)
        return 0;
    if (l->coord.lng != r->coord.lng)
        return 0;
    return 1;
}

/**
 * @brief Whether two traffic locations are equal.
 *
 * Only directionality, the `ramps` member and reference points are considered for comparison; auxiliary
 * data (such as road names, road types and additional TMC information) is ignored.
 *
 * When in doubt, this function errs on the side of inequality, i.e. when equivalence cannot be reliably
 * determined, the locations will be reported as not equal, even though they may translate to the same
 * segments later.
 *
 * @return true if `l` and `r` are equal, false if not
 */
static int traffic_location_equals(struct traffic_location * l, struct traffic_location * r) {
    /* directionality and ramps must match for locations to be considered equal */
    if (l->directionality != r->directionality)
        return 0;
    if (l->ramps != r->ramps)
        return 0;

    /* locations must have the same points set to be considered equal */
    if (!l->from != !r->from)
        return 0;
    if (!l->to != !r->to)
        return 0;
    if (!l->at != !r->at)
        return 0;
    if (!l->via != !r->via)
        return 0;
    if (!l->not_via != !r->not_via)
        return 0;

    /* both locations have the same points set, compare them */
    if (l->from && !traffic_point_equals(l->from, r->from))
        return 0;
    if (l->to && !traffic_point_equals(l->to, r->to))
        return 0;
    if (l->at && !traffic_point_equals(l->at, r->at))
        return 0;
    if (l->via && !traffic_point_equals(l->via, r->via))
        return 0;
    if (l->not_via && !traffic_point_equals(l->not_via, r->not_via))
        return 0;

    /* No differences found, consider locations equal */
    return 1;
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
 * The cost of each node represents the cost to reach `to`. The cost is calculated in
 * `traffic_route_get_seg_cost()` for actual segments, and distance (with a penalty factor) for the
 * offroad connection from the last point in the graph to `to`.
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
 * @param data Data for the segments added to the map
 * @param c_start Start coordinates
 * @param c_dst Destination coordinates
 * @param start_existing Start point of an existing route (whose points will not be used)
 *
 * @return The point in the route graph at which the path begins, or `NULL` if no path was found.
 */
static struct route_graph_point * traffic_route_flood_graph(struct route_graph * rg, struct seg_data * data,
        struct coord * c_start, struct coord * c_dst, struct route_graph_point * start_existing) {
    struct route_graph_point * ret;

    int i;

    GList * existing = NULL;

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

    if (!c_start || !c_dst)
        return NULL;

    /* store points of existing route */
    if (start_existing) {
        p = start_existing;
        while (p) {
            /* Do not exclude the last point (seg==NULL) from the heap as that may result in the existing route not
             * being joined properly to the new one */
            if (p->seg)
                existing = g_list_prepend(existing, p);
            if (!p->seg)
                p = NULL;
            else if (p == p->seg->start)
                p = p->seg->end;
            else
                p = p->seg->start;
        }
    }

    /* prime the route graph */
    heap = fh_makekeyheap();

    start_value = PENALTY_OFFROAD * transform_distance(projection_mg, c_start, c_dst);
    ret = NULL;

    dbg(lvl_debug, "start flooding route graph, start_value=%d", start_value);

    for (i = 0; i < HASH_SIZE; i++) {
        p = rg->hash[i];
        while (p) {
            if (!g_list_find(existing, p)) {
                if (!(p->flags & RP_TURN_RESTRICTION)) {
                    p->value = PENALTY_OFFROAD * transform_distance(projection_mg, &p->c, c_dst);
                    p->el = fh_insertkey(heap, p->value, p);
                } else {
                    /* ignore points which are part of turn restrictions */
                    p->value = INT_MAX;
                    p->el = NULL;
                }
                p->seg = NULL;
            }
            p = p->hash_next;
        }
    }

    /* flood the route graph */
    for (;;) {
        p = fh_extractmin(heap); /* Starting Dijkstra by selecting the point with the minimum costs on the heap */
        if (!p) /* There are no more points with temporarily calculated costs, Dijkstra has finished */
            break;

        dbg(lvl_debug, "p=%p, value=%d", p, p->value);

        min = p->value;
        p->el = NULL; /* This point is permanently calculated now, we've taken it out of the heap */
        s = p->start;
        while (s) { /* Iterating all the segments leading away from our point to update the points at their ends */
            val = traffic_route_get_seg_cost(s, data, -1);

            dbg(lvl_debug, "  negative segment, val=%d", val);

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
                    new += PENALTY_OFFROAD * transform_distance(projection_mg, &s->end->c, c_start);
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
            val = traffic_route_get_seg_cost(s, data, 1);

            dbg(lvl_debug, "  positive segment, val=%d", val);

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
                    new += PENALTY_OFFROAD * transform_distance(projection_mg, &s->start->c, c_start);
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
    g_list_free(existing);
    return ret;
}

/**
 * @brief Extends the route beyond its end point.
 *
 * This function follows the road beginning at `end`, stopping at the next junction. It can be called
 * again on the result, again extending it to the next junction.
 *
 * To follow the road, each segment is compared to `last` and the segment whose attributes match it is
 * chosen, provided such a segment can be determined without ambiguity.
 *
 * When the function returns, all points added to the route will have their `seg` member set. To append
 * the new stretch to the route, set the `seg` member of its last point to the return value. After that,
 * the extended route can be walked in the usual manner.
 *
 * The value of each new point is the value of its predecessor on the route mins the length of the
 * segment which links the two points. Point values thus continue to decrease along the route, allowing
 * comparisons or difference calculations to be performed on the extended route. Note that this may
 * result in points having negative values.
 *
 * @param rg The flooded route graph
 * @param last The last segment in the current route graph (either the `start` or the `end` member of
 * this segment must be equal to the `end` argument)
 * @param end The last point of the current route graph (the `seg` member of this point must be NULL)
 *
 * @return The next segment in the route, or `NULL` if the route cannot be extended.
 */
static struct route_graph_segment * traffic_route_append(struct route_graph *rg,
        struct route_graph_segment * last, struct route_graph_point * end) {
    struct route_graph_segment * ret = NULL, * s = last, * s_cmp, * s_next;
    struct route_graph_point * p = end;
    int num_seg;
    int id_match;
    int is_ambiguous;

    if (!end) {
        dbg(lvl_error, "end point cannot be NULL");
        return NULL;
    }
    if (end->seg) {
        dbg(lvl_error, "end point cannot have a next segment");
        return NULL;
    }

    if ((end != last->end) && (end != last->start)) {
        dbg(lvl_error, "last segment must begin or end at end point");
        return NULL;
    }

    while (1) {
        num_seg = 0;
        id_match = 0;
        is_ambiguous = 0;
        s_next = NULL;
        for (s_cmp = p->start; s_cmp; s_cmp = s_cmp->start_next) {
            num_seg++;
            if ((s_cmp == s) || (s_cmp->data.flags & AF_ONEWAYREV) || (s_cmp->end->flags & RP_TURN_RESTRICTION))
                continue;
            if (item_is_equal_id(s_cmp->data.item, s->data.item)) {
                s_next = s_cmp;
                id_match = 1;
            } else if ((s_cmp->data.item.type == s->data.item.type) && !id_match && !is_ambiguous) {
                if (s_next) {
                    s_next = NULL;
                    is_ambiguous = 1;
                } else
                    s_next = s_cmp;
            }
        }
        for (s_cmp = p->end; s_cmp; s_cmp = s_cmp->end_next) {
            num_seg++;
            if ((s_cmp == s) || (s_cmp->data.flags & AF_ONEWAY) || (s_cmp->end->flags & RP_TURN_RESTRICTION))
                continue;
            if (item_is_equal_id(s_cmp->data.item, s->data.item)) {
                s_next = s_cmp;
                id_match = 1;
            } else if ((s_cmp->data.item.type == s->data.item.type) && !id_match && !is_ambiguous) {
                if (s_next) {
                    s_next = NULL;
                    is_ambiguous = 1;
                } else
                    s_next = s_cmp;
            }
        }

        /* cancel if we are past end and have hit a junction */
        if ((p != end) && (num_seg > 2))
            break;

        /* update links and move one step further */
        if (p != end)
            p->seg = s_next;
        else
            ret = s_next;
        if (s_next) {
            if (p == s_next->start) {
                s_next->end->value = p->value - s_next->data.len;
                p = s_next->end;
            } else {
                s_next->start->value = p->value - s_next->data.len;
                p = s_next->start;
            }
            s = s_next;
        } else
            break;
    }
    p->seg = NULL;
    dbg(lvl_debug, "return, last=%p, ret=%p", last, ret);
    return ret;
}

/**
 * @brief Extends the route beyond its start point.
 *
 * This function follows the road leading towards `start` backwards, stopping at the next junction. It
 * can be called again on the result, again extending it to the next junction.
 *
 * To follow the road, each segment is compared to `start->seg` and the segment whose attributes match
 * it is chosen, provided such a segment can be determined without ambiguity.
 *
 * When the function returns, all points added to the route will have their `seg` member set so the
 * extended route can be walked in the usual manner.
 *
 * @param rg The flooded route graph
 * @param start The current start of the route
 *
 * @return The start of the extended route, or `NULL` if the route cannot be extended (in which case
 * `start` continues to be the start of the route).
 */
static struct route_graph_point * traffic_route_prepend(struct route_graph * rg,
        struct route_graph_point * start) {
    struct route_graph_point * ret = start;
    struct route_graph_segment * s, * s_cmp, * s_prev = NULL;
    int num_seg;
    int id_match;
    int is_ambiguous;

    dbg(lvl_debug, "At %p (%d), start", start, start ? start->value : -1);
    if (!start)
        return NULL;

    s = start->seg;

    while (s) {
        num_seg = 0;
        id_match = 0;
        is_ambiguous = 0;
        for (s_cmp = ret->start; s_cmp; s_cmp = s_cmp->start_next) {
            num_seg++;
            if (s_cmp == s)
                continue;
            if (s_cmp->data.flags & AF_ONEWAY)
                continue;
            if (s_cmp->end->seg != s_cmp)
                continue;
            if (item_is_equal_id(s_cmp->data.item, s->data.item)) {
                s_prev = s_cmp;
                id_match = 1;
            } else if ((s_cmp->data.item.type == s->data.item.type) && !id_match && !is_ambiguous) {
                if (s_prev) {
                    s_prev = NULL;
                    is_ambiguous = 1;
                } else
                    s_prev = s_cmp;
            }
        }
        for (s_cmp = ret->end; s_cmp; s_cmp = s_cmp->end_next) {
            num_seg++;
            if (s_cmp == s)
                continue;
            if (s_cmp->data.flags & AF_ONEWAYREV)
                continue;
            if (s_cmp->start->seg != s_cmp)
                continue;
            if (item_is_equal_id(s_cmp->data.item, s->data.item)) {
                s_prev = s_cmp;
                id_match = 1;
            } else if ((s_cmp->data.item.type == s->data.item.type) && !id_match && !is_ambiguous) {
                if (s_prev) {
                    s_prev = NULL;
                    is_ambiguous = 1;
                } else
                    s_prev = s_cmp;
            }
        }

        /* cancel if we are past start and ret is a junction */
        if ((ret != start) && (num_seg > 2))
            break;

        /* move s and ret one step further and update links */
        s = s_prev;
        if (s) {
            if (ret == s->start) {
                ret = s->end;
                dbg(lvl_debug, "At %p (%d -> %d)", ret, ret->value, s->start->value + s->data.len);
                ret->value = s->start->value + s->data.len;
            } else {
                ret = s->start;
                dbg(lvl_debug, "At %p (%d -> %d)", ret, ret->value, s->end->value + s->data.len);
                ret->value = s->end->value + s->data.len;
            }
            ret->seg = s;
            s_prev = NULL;
        }
    }
    dbg(lvl_debug, "return, start=%p, ret=%p", start, ret);
    return ret;
}

/**
 * @brief Returns one of the traffic location’s points.
 *
 * @param this_ The traffic location
 * @param point The point of the traffic location to retrieve (0 = from, 1 = at, 2 = to, 16 = start, 17 = end)
 *
 * @return The matched points, or NULL if the requested point does not exist
 */
static struct traffic_point * traffic_location_get_point(struct traffic_location * this_, int point) {
    /* The point from the location to match */
    struct traffic_point * trpoint = NULL;

    switch(point) {
    case 0:
        trpoint = this_->from;
        break;
    case 1:
        trpoint = this_->at;
        break;
    case 2:
        trpoint = this_->to;
        break;
    case 16:
        trpoint = this_->from ? this_->from : this_->at;
        break;
    case 17:
        trpoint = this_->to ? this_->to : this_->at;
        break;
    default:
        break;
    }

    return trpoint;
}

/**
 * @brief Compares a given point to the traffic location and returns a score.
 *
 * This method obtains all points at coordinates `c` from the map_rect used to build the route graph, compares their
 * attributes to those supplied with the location, assigns a match score from 0 (no matching attributes) to 100 (all
 * supplied attributes match) and returns the highest score obtained. If no matching point is found, 0 is returned.
 *
 * @param this_ The traffic location
 * @param p The route graph point to examine for matches
 * @param point The point of the traffic location to use for matching (0 = from, 1 = at, 2 = to, 16 = start, 17 = end)
 * @param rg The route graph
 * @param start The first point of the path
 * @param match_start True to evaluate for the start point of a route, false for the end point
 * @param ms The mapset to read the items from
 *
 * @return A score from 0 (worst) to 100 (best).
 */
static int traffic_location_get_point_match(struct traffic_location * this_, struct route_graph_point * p, int point,
        struct route_graph * rg, struct route_graph_point * start, int match_start, struct mapset * ms) {
    int ret = 0;

    /* The point from the location to match */
    struct traffic_point * trpoint = NULL;

    /* The attribute matching score for the current item */
    int score;

    trpoint = traffic_location_get_point(this_, point);

    if (!trpoint)
        return 0;

    /* First examine route graph points and connected segments */
    score = traffic_point_match_segment_attributes(trpoint, p, start, match_start);
    if (ret < score)
        ret = score;
    return ret;
}

/**
 * @brief Returns points from the route graph which match a traffic location.
 *
 * This method obtains point items from the map_rect from which the route graph was built and compares
 * their attributes to those supplied with the location. Each point is assigned a match score, from 0
 * (no matching attributes) to 100 (all supplied attributes match), and a list of all points with a
 * nonzero score is returned.
 *
 * Points which have no corresponding map item (i.e. points which have no additional attributes) are not included in
 * the result and must be analyzed separately if needed.
 *
 * @param this_ The traffic location
 * @param point The point of the traffic location to use for matching (0 = from, 1 = at, 2 = to, 16 = start, 17 = end)
 * @param rg The route graph
 * @param start The first point of the path
 * @param match_start True to evaluate for the start point of a route, false for the end point
 * @param ms The mapset to read the items from
 *
 * @return The matched points as a `GList`. The `data` member of each item points to a `struct point_data` for the point.
 */
static GList * traffic_location_get_matching_points(struct traffic_location * this_, int point,
        struct route_graph * rg, struct route_graph_point * start, int match_start, struct mapset * ms) {
    GList * ret = NULL;

    /* The point from the location to match */
    struct traffic_point * trpoint = NULL;

    /* Map attribute (currently not evaluated) */
    struct attr attr;

    /* The item being processed */
    struct item *item;

    /* Mercator coordinates of current and previous point */
    struct coord c;

    /* The corresponding point in the route graph */
    struct route_graph_point * p;

    /* The attribute matching score for the current item */
    int score;

    /* Data for the current point */
    struct point_data * data;

    trpoint = traffic_location_get_point(this_, point);

    if (!trpoint)
        return NULL;

    traffic_location_set_enclosing_rect(this_, NULL);

    rg->h = mapset_open(ms);

    while ((rg->m = mapset_next(rg->h, 2))) {
        /* Skip traffic map (identified by the `attr_traffic` attribute) */
        if (map_get_attr(rg->m, attr_traffic, &attr, NULL))
            continue;
        if (!traffic_location_open_map_rect(this_, rg))
            continue;
        while ((item = map_rect_get_item(rg->mr))) {
            /* exclude non-point items */
            if ((item->type < type_town_label) || (item->type >= type_line))
                continue;

            /* exclude items from which we can't obtain a coordinate pair */
            if (!item_coord_get(item, &c, 1))
                continue;

            /* exclude items not in the route graph (points with turn restrictions are ignored) */
            p = route_graph_get_point(rg, &c);
            while (p && (p->flags & RP_TURN_RESTRICTION))
                p = route_graph_get_point_next(rg, &c, p);
            if (!p)
                continue;

            /* determine score */
            score = traffic_point_match_attributes(trpoint, item);

            /* exclude items with a zero score */
            if (!score)
                continue;

            dbg(lvl_debug, "adding item, score: %d", score);

            do {
                if (!(p->flags & RP_TURN_RESTRICTION)) {
                    data = g_new0(struct point_data, 1);
                    data->score = score;
                    data->p = p;

                    ret = g_list_append(ret, data);
                }
            } while ((p = route_graph_get_point_next(rg, &c, p)));
        }
        map_selection_destroy(rg->sel);
        rg->sel = NULL;
        map_rect_destroy(rg->mr);
        rg->mr = NULL;
    }
    route_graph_build_done(rg, 1);

    return ret;
}

/**
 * @brief Whether the contents of a location are valid.
 *
 * This identifies any malformed locations in which mandatory members are not set.
 *
 * @return true if the locations is valid, false if it is malformed
 */
static int traffic_location_is_valid(struct traffic_location * this_) {
    if (!this_->at && !(this_->from && this_->to))
        return 0;
    return 1;
}

/**
 * @brief Whether the current point is a candidate for low-res endpoint matching.
 *
 * @param this_ The point to examine
 * @param s_prev The route segment leading to `this_` (NULL for the start point)
 */
static int route_graph_point_is_endpoint_candidate(struct route_graph_point *this_,
        struct route_graph_segment *s_prev) {
    int ret;

    /* Whether we are at a junction of 3 or more segments */
    int is_junction;

    /* Segment used for comparison */
    struct route_graph_segment *s_cmp;

    /* Current segment */
    struct route_graph_segment *s = this_->seg;

    if (!s_prev || !s)
        /* the first and last points are always candidates */
        ret = 1;
    else
        /* detect tunnel portals */
        ret = ((s->data.flags & AF_UNDERGROUND) != (s_prev->data.flags & AF_UNDERGROUND));
    if (!ret) {
        /* detect junctions */
        is_junction = (s && s_prev) ? 0 : -1;
        for (s_cmp = this_->start; s_cmp; s_cmp = s_cmp->start_next) {
            if ((s_cmp != s) && (s_cmp != s_prev))
                is_junction += 1;
        }
        for (s_cmp = this_->end; s_cmp; s_cmp = s_cmp->end_next) {
            if ((s_cmp != s) && (s_cmp != s_prev))
                is_junction += 1;
        }
        ret = (is_junction > 0);
    }
    return ret;
}

/**
 * @brief Gets the speed for a traffic distortion item.
 *
 * @param item The road item to which the traffic distortion refers (not the traffic distortion item)
 * @param data Segment data
 * @param item_maxspeed Speed limit for the item, `INT_MAX` if none
 */
static int traffic_get_item_speed(struct item * item, struct seg_data * data, int item_maxspeed) {
    /* Speed calculated in various ways */
    int maxspeed, speed, penalized_speed, factor_speed;

    speed = data->speed;
    if ((data->speed != INT_MAX) || data->speed_penalty || (data->speed_factor != 100)) {
        if (item_maxspeed != INT_MAX) {
            maxspeed = item_maxspeed;
        } else {
            switch (item->type) {
            case type_highway_land:
            case type_street_n_lanes:
                maxspeed = 100;
                break;
            case type_highway_city:
            case type_street_4_land:
                maxspeed = 80;
                break;
            case type_street_3_land:
                maxspeed = 70;
                break;
            case type_street_2_land:
                maxspeed = 65;
                break;
            case type_street_1_land:
                maxspeed = 60;
                break;
            case type_street_4_city:
                maxspeed = 50;
                break;
            case type_ramp:
            case type_street_3_city:
            case type_street_unkn:
                maxspeed = 40;
                break;
            case type_street_2_city:
            case type_track_paved:
                maxspeed = 30;
                break;
            case type_track:
            case type_cycleway:
                maxspeed = 20;
                break;
            case type_roundabout:
            case type_street_1_city:
            case type_street_0:
            case type_living_street:
            case type_street_service:
            case type_street_parking_lane:
            case type_path:
            case type_track_ground:
            case type_track_gravelled:
            case type_track_unpaved:
            case type_track_grass:
            case type_bridleway:
                maxspeed = 10;
                break;
            case type_street_pedestrian:
            case type_footway:
            case type_steps:
                maxspeed = 5;
                break;
            default:
                maxspeed = 50;
            }
        }
        penalized_speed = maxspeed - data->speed_penalty;
        if (penalized_speed < 5)
            penalized_speed = 5;
        factor_speed = maxspeed * data->speed_factor / 100;
        if (speed > penalized_speed)
            speed = penalized_speed;
        if (speed > factor_speed)
            speed = factor_speed;
    }
    return speed;
}

/**
 * @brief Gets the delay for a traffic distortion item
 *
 * @param delay Total delay for all items associated with the same message and direction
 * @param item_len Length of the current item
 * @param len Combined length of all items associated with the same message and direction
 */
static int traffic_get_item_delay(int delay, int item_len, int len) {
    if (delay)
        return delay * item_len / len;
    else
        return delay;
}

/**
 * @brief Generates segments affected by a traffic message.
 *
 * This translates the approximate coordinates in the `from`, `at`, `to`, `via` and `not_via` members of
 * the location to one or more map segments, using both the raw coordinates and the auxiliary information
 * contained in the location. Each segment is stored in the map, if not already present, and a link is
 * stored with the message.
 *
 * @param this_ The traffic message
 * @param ms The mapset to use for matching
 * @param data Data for the segments added to the map
 * @param map The traffic map
 * @param route The route affected by the changes
 *
 * @return `true` if the locations were matched successfully, `false` if there was a failure.
 */
static int traffic_message_add_segments(struct traffic_message * this_, struct mapset * ms, struct seg_data * data,
                                        struct map *map, struct route * route) {
    int i;

    struct coord_geo * coords[] = {NULL, NULL, NULL};
    struct coord * pcoords[] = {NULL, NULL, NULL};

    /* How many point pairs coords contains (number of members minus one) */
    int point_pairs = -1;

    /* Which members of coords are the end points */
    int endpoints = 0;

    /* The direction (positive or negative) */
    int dir = 1;

    /* Start point for the route path */
    struct route_graph_point * p_start = NULL;

    /* Current and previous segment */
    struct route_graph_segment *s = NULL;
    struct route_graph_segment *s_prev;

    /* Iterator for the route path */
    struct route_graph_point *p_iter;

    /* route graph for simplified routing */
    struct route_graph *rg;

    /* Coordinate count for matched segment */
    int ccnt;

    /* Coordinates of matched segment and pointer into it, order as read from map */
    struct coord *c, ca[2048];

    /* Coordinates of matched segment, sorted */
    struct coord *cd, *cs;

    /* Speed for the current segment */
    int speed;

    /* Delay for the current segment */
    int delay;

    /* Number of new segments and existing segments */
    int count = 0, prev_count;

    /* Length of location */
    int len;

    /* The message's previous list of items */
    struct item ** prev_items;

    /* The next item in the message's list of items */
    struct item ** next_item;

    /* Flags for the next item to add */
    int flags;

    /* The last item added */
    struct item * item;

    /* Projected coordinates of start and end points of the actual location
     * (if at is set, both point to the same coordinates) */
    struct coord * c_from, * c_to;

    /* Matched points */
    GList * points;
    GList * points_iter;

    /* The corresponding point data */
    struct point_data * pd;

    /* The match score of the current point */
    int score;

    /* Current and minimum cost to reference point */
    int val, minval;

    /* Start of extended route */
    struct route_graph_point * start_new;

    /* Last segment of the route (before extension) */
    struct route_graph_segment * s_last = NULL;

    /* Aligned points */
    struct route_graph_point * p_from;
    struct route_graph_point * p_to;

    dbg(lvl_debug, "*****checkpoint ADD-1");
    if (!data) {
        dbg(lvl_error, "no data for segments, aborting");
        return 0;
    }

    if (this_->location->ramps != location_ramps_none)
        /* TODO Ramps, not supported yet */
        return 0;

    /* Main carriageway */

    dbg(lvl_debug, "*****checkpoint ADD-2");
    /* get point triple and enclosing rectangle */
    endpoints = traffic_location_get_point_triple(this_->location, &coords[0]);
    if (!endpoints) {
        dbg(lvl_error, "invalid location (mandatory points missing)");
        return 0;
    }
    traffic_location_set_enclosing_rect(this_->location, &coords[0]);
    for (i = 0; i < 3; i++)
        if (coords[i]) {
            pcoords[i] = g_new0(struct coord, 1);
            transform_from_geo(projection_mg, coords[i], pcoords[i]);
            point_pairs++;
        }

    if (this_->location->at && !(this_->location->from || this_->location->to))
        /* TODO Point location with no auxiliary points, not supported yet */
        return 0;

    dbg(lvl_debug, "*****checkpoint ADD-3");
    rg = traffic_location_get_route_graph(this_->location, ms);

    /* transform coordinates */
    c_from = (endpoints & 4) ? pcoords[0] : pcoords[1];
    c_to = (endpoints & 1) ? pcoords[2] : pcoords[1];

    /* determine segments */
    dbg(lvl_debug, "*****checkpoint ADD-4 (loop start)");
    while (1) { /* once for each direction (loop logic at the end) */
        dbg(lvl_debug, "*****checkpoint ADD-4.1");
        if (point_pairs == 1) {
            if (dir > 0)
                p_start = traffic_route_flood_graph(rg, data,
                                                    pcoords[0] ? pcoords[0] : pcoords[1],
                                                    pcoords[2] ? pcoords[2] : pcoords[1], NULL);
            else
                p_start = traffic_route_flood_graph(rg, data,
                                                    pcoords[2] ? pcoords[2] : pcoords[1],
                                                    pcoords[0] ? pcoords[0] : pcoords[1], NULL);
            dbg(lvl_debug, "*****checkpoint ADD-4.1.1");
        } else if (point_pairs == 2) {
            /*
             * If we have more than two points, create the route in two stages (from the first to the second point,
             * then from the second to the third point) and concatenate them. This could easily be extended to any
             * number of points, provided they are spaced sufficiently far apart to calculate a route between each pair
             * of subsequent points.
             * This will create a kind of “Frankenstein route” in which the cost of points does not decrease
             * continuously but has an upward leap as we pass the middle point. This is not an issue as long as we do
             * not do any further processing based on point cost (which we currently don’t).
             * If the route needs to be extended beyond the start point, this has to be done after the first stage,
             * as doing so relies on the route graph for that stage.
             */
            /* TODO handle cases in which the route goes through the "third" point
             * (this should not happen; if it does, we need to detect and fix it) */
            if (dir > 0)
                p_start = traffic_route_flood_graph(rg, data, pcoords[0], pcoords[1], NULL);
            else
                p_start = traffic_route_flood_graph(rg, data, pcoords[2], pcoords[1], NULL);
            if ((this_->location->fuzziness == location_fuzziness_low_res)
                    || this_->location->at || this_->location->not_via) {
                /* extend start to next junction */
                start_new = traffic_route_prepend(rg, p_start);
                if (start_new)
                    p_start = start_new;
            }
            if (dir > 0) {
                if (!p_start) {
                    /* fallback if calculating the first piece of the route failed */
                    p_start = traffic_route_flood_graph(rg, data, pcoords[1], pcoords[2], NULL);
                    start_new = traffic_route_prepend(rg, p_start);
                } else
                    traffic_route_flood_graph(rg, data, pcoords[1], pcoords[2], p_start);
            } else {
                if (!p_start) {
                    /* fallback if calculating the first piece of the route failed */
                    p_start = traffic_route_flood_graph(rg, data, pcoords[1], pcoords[0], NULL);
                    start_new = traffic_route_prepend(rg, p_start);
                } else
                    traffic_route_flood_graph(rg, data, pcoords[1], pcoords[0], p_start);
            }
            dbg(lvl_debug, "*****checkpoint ADD-4.1.2");
        }

        dbg(lvl_debug, "*****checkpoint ADD-4.2");
        /* tweak ends (find the point where the ramp touches the main road) */
        if ((this_->location->fuzziness == location_fuzziness_low_res)
                || this_->location->at || this_->location->not_via) {
            dbg(lvl_debug, "*****checkpoint ADD-4.2.1");
            /* tweak end point */
            if (this_->location->at)
                points = traffic_location_get_matching_points(this_->location, 1, rg, p_start, 0, ms);
            else if (dir > 0)
                points = traffic_location_get_matching_points(this_->location, 2, rg, p_start, 0, ms);
            else
                points = traffic_location_get_matching_points(this_->location, 0, rg, p_start, 0, ms);
            if (!p_start) {
                dbg(lvl_error, "end point not found on map");
                for (points_iter = points; points_iter; points_iter = g_list_next(points_iter))
                    g_free(points_iter->data);
                g_list_free(points);
                route_graph_free_points(rg);
                route_graph_free_segments(rg);
                g_free(rg);
                for (i = 0; i < 3; i++)
                    g_free(pcoords[i]);
                return 0;
            }
            s = p_start ? p_start->seg : NULL;
            p_iter = p_start;

            dbg(lvl_debug, "*****checkpoint ADD-4.2.2");
            /* extend end to next junction */
            for (s = p_start ? p_start->seg : NULL; s; s = p_iter->seg) {
                dbg(lvl_debug, "*****checkpoint ADD-4.2.2.1, s=%p, p_iter=%p (%d)", s, p_iter, p_iter ? p_iter->value : INT_MAX);
                s_last = s;
                if (s->start == p_iter)
                    p_iter = s->end;
                else
                    p_iter = s->start;
            }
            s = traffic_route_append(rg, s_last, p_iter);
            p_iter->seg = s;

            s = p_start ? p_start->seg : NULL;
            s_prev = NULL;
            p_iter = p_start;
            minval = INT_MAX;
            p_to = NULL;

            dbg(lvl_debug, "*****checkpoint ADD-4.2.3");
            struct coord_geo wgs;
            while (p_iter) {
                transform_to_geo(projection_mg, &(p_iter->c), &wgs);
                dbg(lvl_debug, "*****checkpoint ADD-4.2.3, p_iter=%p (value=%d)\nhttps://www.openstreetmap.org?mlat=%f&mlon=%f/#map=13",
                    p_iter, p_iter->value, wgs.lat, wgs.lng);
                if (route_graph_point_is_endpoint_candidate(p_iter, s_prev)) {
                    score = traffic_location_get_point_match(this_->location, p_iter,
                            this_->location->at ? 1 : (dir > 0) ? 2 : 0,
                            rg, p_start, 0, ms);
                    pd = NULL;
                    for (points_iter = points; points_iter && (score < 100); points_iter = g_list_next(points_iter)) {
                        pd = (struct point_data *) points_iter->data;
                        if ((pd->p == p_iter) && (pd->score > score))
                            score = pd->score;
                    }
                    val = transform_distance(projection_mg, &p_iter->c, (dir > 0) ? c_to : c_from);
                    val += (val * (100 - score) * (PENALTY_POINT_MATCH) / 100);
                    if (val < minval) {
                        minval = val;
                        p_to = p_iter;
                        dbg(lvl_debug, "candidate end point found, point %p, value %d (score %d)", p_iter, val, score);
                    }
                }

                if (!s)
                    p_iter = NULL;
                else  {
                    p_iter = (s->start == p_iter) ? s->end : s->start;
                    s_prev = s;
                    s = p_iter->seg;
                }
            }

            dbg(lvl_debug, "*****checkpoint ADD-4.2.4");
            for (points_iter = points; points_iter; points_iter = g_list_next(points_iter))
                g_free(points_iter->data);
            g_list_free(points);

            dbg(lvl_debug, "*****checkpoint ADD-4.2.5");
            /* tweak start point */
            if (this_->location->at)
                points = traffic_location_get_matching_points(this_->location, 1, rg, p_start, 1, ms);
            else if (dir > 0)
                points = traffic_location_get_matching_points(this_->location, 0, rg, p_start, 1, ms);
            else
                points = traffic_location_get_matching_points(this_->location, 2, rg, p_start, 1, ms);
            s_prev = NULL;
            minval = INT_MAX;
            p_from = NULL;

            transform_to_geo(projection_mg, &(p_start->c), &wgs);
            dbg(lvl_debug, "*****checkpoint ADD-4.2.6, p_start=%p\nhttps://www.openstreetmap.org?mlat=%f&mlon=%f/#map=13",
                p_start, wgs.lat, wgs.lng);
            if (point_pairs == 1) {
                /* extend start to next junction (if we have more than two points, this has already been done) */
                start_new = traffic_route_prepend(rg, p_start);
                if (start_new)
                    p_start = start_new;
            }

            s = p_start ? p_start->seg : NULL;
            p_iter = p_start;
            dbg(lvl_debug, "*****checkpoint ADD-4.2.7");
            while (p_iter) {
                transform_to_geo(projection_mg, &(p_iter->c), &wgs);
                dbg(lvl_debug, "*****checkpoint ADD-4.2.7, p_iter=%p (value=%d)\nhttps://www.openstreetmap.org?mlat=%f&mlon=%f/#map=13",
                    p_iter, p_iter->value, wgs.lat, wgs.lng);
                if (route_graph_point_is_endpoint_candidate(p_iter, s_prev)) {
                    score = traffic_location_get_point_match(this_->location, p_iter,
                            this_->location->at ? 1 : (dir > 0) ? 0 : 2,
                            rg, p_start, 1, ms);
                    pd = NULL;
                    for (points_iter = points; points_iter && (score < 100); points_iter = g_list_next(points_iter)) {
                        pd = (struct point_data *) points_iter->data;
                        if ((pd->p == p_iter) && (pd->score > score))
                            score = pd->score;
                    }
                    val = transform_distance(projection_mg, &p_iter->c, (dir > 0) ? c_from : c_to);
                    /* TODO does attribute matching make sense for the start segment? */
                    val += (val * (100 - score) * (PENALTY_POINT_MATCH) / 100);
                    if (val < minval) {
                        minval = val;
                        p_from = p_iter;
                        dbg(lvl_debug, "candidate start point found, point %p, value %d (score %d)",
                            p_iter, val, score);
                    }
                }

                if (!s)
                    p_iter = NULL;
                else {
                    p_iter = (s->start == p_iter) ? s->end : s->start;
                    s_prev = s;
                    s = p_iter->seg;
                }
            }

            dbg(lvl_debug, "*****checkpoint ADD-4.2.8");
            for (points_iter = points; points_iter; points_iter = g_list_next(points_iter))
                g_free(points_iter->data);
            g_list_free(points);

            if (!p_from)
                p_from = p_start;

            dbg(lvl_debug, "*****checkpoint ADD-4.2.9");
            /* ensure we have at least one segment */
            if ((p_from == p_to) || !p_from->seg) {
                dbg(lvl_debug, "*****checkpoint ADD-4.2.9.1");
                p_iter = p_start;
                dbg(lvl_debug, "*****checkpoint ADD-4.2.9.2");
                while (p_iter->seg) {
                    dbg(lvl_debug, "*****checkpoint ADD-4.2.9.2.1, p_iter=%p, p_iter->seg=%p", p_iter, p_iter ? p_iter->seg : NULL);
                    if (p_iter == p_iter->seg->start) {
                        dbg(lvl_debug, "*****checkpoint ADD-4.2.9.2.2 (p_iter == p_iter->seg->start)");
                        /* compare to the last point: because p_to may be NULL here, we're comparing to
                         * p_from instead, which at this point is guaranteed to be non-NULL and either
                         * equal to p_to or without a successor, making it the designated end point. */
                        if (p_iter->seg->end == p_from)
                            break;
                        dbg(lvl_debug, "*****checkpoint ADD-4.2.9.2.3");
                        p_iter = p_iter->seg->end;
                        dbg(lvl_debug, "*****checkpoint ADD-4.2.9.2.4");
                    } else {
                        dbg(lvl_debug, "*****checkpoint ADD-4.2.9.2.2 (p_iter != p_iter->seg->start)");
                        if (p_iter->seg->start == p_from)
                            break;
                        dbg(lvl_debug, "*****checkpoint ADD-4.2.9.2.3");
                        p_iter = p_iter->seg->start;
                        dbg(lvl_debug, "*****checkpoint ADD-4.2.9.2.4");
                    }
                }
                if (p_from->seg) {
                    dbg(lvl_debug, "*****checkpoint ADD-4.2.9.3, p_from->seg is non-NULL");
                    /* decide between predecessor and successor of the point, based on proximity */
                    p_to = (p_from == p_from->seg->end) ? p_from->seg->start : p_from->seg->end;
                    if (transform_distance(projection_mg, &p_to->c, pcoords[1] ? pcoords[1] : pcoords[2])
                            > transform_distance(projection_mg, &p_iter->c, pcoords[1] ? pcoords[1] : pcoords[2])) {
                        p_to = p_from;
                        p_from = p_iter;
                    }
                } else {
                    dbg(lvl_debug, "*****checkpoint ADD-4.2.9.3, p_from->seg is NULL");
                    /* p_from has no successor, the segment goes from its predecessor to p_from */
                    p_to = p_from;
                    p_from = p_iter;
                }
            }

            dbg(lvl_debug, "*****checkpoint ADD-4.2.10");
            /* if we have identified a last point, drop everything after it from the path */
            if (p_to)
                p_to->seg = NULL;

            /* set first point to be the start point */
            if (p_from != p_start) {
                dbg(lvl_debug, "changing p_start from %p to %p", p_start, p_from);
            }
            p_start = p_from;
        }

        dbg(lvl_debug, "*****checkpoint ADD-4.3");
        /* calculate route */
        s = p_start ? p_start->seg : NULL;
        p_iter = p_start;

        if (!s)
            dbg(lvl_error, "no segments");

        /* count segments and calculate length */
        prev_count = count;
        count = 0;
        len = 0;
        dbg(lvl_debug, "*****checkpoint ADD-4.4");
        while (s) {
            dbg(lvl_debug, "*****checkpoint ADD-4.4.1 (#%d, p_iter=%p, s=%p, next %p)",
                count, p_iter, s, (s->start == p_iter) ? s->end : s->start);
            count++;
            len += s->data.len;
            if (s->start == p_iter)
                p_iter = s->end;
            else
                p_iter = s->start;
            s = p_iter->seg;
        }
        dbg(lvl_debug, "*****checkpoint ADD-4.5");

        /* add segments */

        s = p_start ? p_start->seg : NULL;
        p_iter = p_start;

        if (this_->priv->items) {
            prev_items = this_->priv->items;
            this_->priv->items = g_new0(struct item *, count + prev_count + 1);
            memcpy(this_->priv->items, prev_items, sizeof(struct item *) * prev_count);
            next_item = this_->priv->items + prev_count;
            g_free(prev_items);
        } else {
            this_->priv->items = g_new0(struct item *, count + 1);
            next_item = this_->priv->items;
        }

        dbg(lvl_debug, "*****checkpoint ADD-4.6 (loop start)");
        while (s) {
            ccnt = item_coord_get_within_range(&s->data.item, ca, 2047, &s->start->c, &s->end->c);
            c = ca;
            cs = g_new0(struct coord, ccnt);
            cd = cs;

            speed = traffic_get_item_speed(&(s->data.item), data,
                                           (s->data.flags & AF_SPEED_LIMIT) ? RSD_MAXSPEED(&s->data) : INT_MAX);

            delay = traffic_get_item_delay(data->delay, s->data.len, len);

            for (i = 0; i < ccnt; i++) {
                *cd++ = *c++;
            }

            if (s->start == p_iter) {
                /* forward direction */
                p_iter = s->end;
                flags = data->flags | (s->data.flags & AF_ONEWAYMASK)
                        | (data->dir == location_dir_one ? AF_ONEWAY : 0);
            } else {
                /* backward direction */
                p_iter = s->start;
                flags = data->flags | (s->data.flags & AF_ONEWAYMASK)
                        | (data->dir == location_dir_one ? AF_ONEWAYREV : 0);
            }


            item = tm_add_item(map, type_traffic_distortion, s->data.item.id_hi, s->data.item.id_lo, flags, data->attrs, cs, ccnt,
                               this_->id);

            tm_item_add_message_data(item, this_->id, speed, delay, data->attrs, route);

            g_free(cs);

            *next_item = tm_item_ref(item);
            next_item++;

            s = p_iter->seg;
        }

        dbg(lvl_debug, "*****checkpoint ADD-4.7");
        if ((this_->location->directionality == location_dir_one) || (dir < 0))
            break;

        dir = -1;
    }

    dbg(lvl_debug, "*****checkpoint ADD-5");
    route_graph_free_points(rg);
    route_graph_free_segments(rg);
    g_free(rg);

    for (i = 0; i < 3; i++)
        g_free(pcoords[i]);

    dbg(lvl_debug, "*****checkpoint ADD-6");
    return 1;
}

/**
 * @brief Restores segments associated with a traffic message from cached data.
 *
 * This reads textfile map data and compares it to data in the current map set. If one or more items
 * (identified by their ID) are no longer present in the map, or their coordinates no longer match, the
 * cached data is discarded and the location needs to be expanded into a set of segments as for a new
 * message. Otherwise the cached segments are added to the traffic map.
 *
 * While this operation does require extensive examination of map data, it is still less expensive than
 * expanding the location from scratch, as the most expensive operation in the latter is routing between the
 * points involved.
 *
 * @param this_ The traffic message
 * @param ms The mapset to use for matching
 * @param data Data for the segments to be added to the map, in textfile format
 * @param map The traffic map
 * @param route The route affected by the changes
 *
 * @return `true` if the locations were matched successfully, `false` if there was a failure.
 */
static int traffic_message_restore_segments(struct traffic_message * this_, struct mapset * ms,
        struct map *map, struct route * route) {
    /* Textfile data: pointers to current and next line, copy and length of current line */
    char * data_curr = this_->location->priv->txt_data, * data_next, * line = NULL;
    int len;

    /* Iterator */
    int i;

    /* Data for attribute parsing */
    int pos;
    char name[TEXTFILE_LINE_SIZE];
    char value[TEXTFILE_LINE_SIZE];

    /* Item data */
    enum item_type type;
    int id_hi;
    int id_lo;
    int flags;
    struct attr * attrs[TEXTFILE_LINE_SIZE / 4];
    int acnt;

    /* For map matching */
    struct mapset_handle *msh;
    struct map * m;
    struct attr attr;
    struct map_selection * msel;
    struct map_rect * mr;
    struct item * map_item;
    int * default_flags;
    int item_flags, segmented, maxspeed=INT_MAX;
    struct coord map_c;

    /*
     * Coordinate count for matched segment
     * (-1 if we are expecting item type and attributes in the next line rather than coordinates)
     */
    int ccnt = -1;

    /* Coordinates of matched segment and pointer into it, order as read from map */
    struct coord ca[2048];

    /* Newly added item */
    struct parsed_item * pitem;
    struct item * item;

    /* Location length */
    int loc_len = 0;

    /* List of parsed items */
    GList * items = NULL, * curr_item;

    /* Whether all items are matched by a map item */
    int is_matched;

    struct seg_data * seg_data;

    dbg(lvl_debug, "*****checkpoint RESTORE-1, txt_data:\n%s", this_->location->priv->txt_data);
    traffic_location_set_enclosing_rect(this_->location, NULL);

    while (1) {
        if (data_curr) {
            data_next = strchr(data_curr, 0x0a);
            len = data_next ? (data_next - data_curr) : strlen(data_curr);
            line = g_new0(char, len + 1);
            strncpy(line, data_curr, len);
            dbg(lvl_debug, "*****checkpoint RESTORE-2, line: %s", line);
        }
        if (line && (ccnt < 0)) {
            /* first line with item type and attributes */
            dbg(lvl_debug, "*****checkpoint RESTORE-3, item/attribute line");
            pos = 0;
            type = type_none;
            id_hi = -1;
            id_lo = -1;
            acnt = 0;
            while (attr_from_line(line, NULL, &pos, value, name)) {
                dbg(lvl_debug, "*****checkpoint RESTORE-4, parsing %s=%s", name, value);
                ccnt = 0;
                if (!strcmp(name, "type")) {
                    /* item type */
                    dbg(lvl_debug, "*****checkpoint RESTORE-4.1, parsing type: %s", value);
                    type = item_from_name(value);
                } else if (!strcmp(name, "id")) {
                    /* item ID */
                    dbg(lvl_debug, "*****checkpoint RESTORE-4.1, parsing ID: %s", value);
                    sscanf(value, "%i,%i", &id_hi, &id_lo);
                    dbg(lvl_debug, "*****checkpoint RESTORE-4.2, ID is 0x%x,0x%x", id_hi, id_lo);
                } else if (!strcmp(name, "flags")) {
                    dbg(lvl_debug, "*****checkpoint RESTORE-4.1, parsing flags: %s", value);
                    char *tail;
                    if (value[0] == '0' && value[1] == 'x')
                        flags = strtoul(value, &tail, 0);
                    else
                        flags = strtol(value, &tail, 0);
                    if (*tail) {
                        dbg(lvl_warning, "Incorrect value '%s' for attribute '%s': expected a number, assuming 0x%x. \n", value, name, flags);
                    }
                } else {
                    /* generic attribute */
                    dbg(lvl_debug, "*****checkpoint RESTORE-4.1, parsing attribute %s=%s", name, value);
                    attrs[acnt] = attr_new_from_text(name, value);
                    if (attrs[acnt])
                        acnt++;
                }
            }
            if (ccnt < 0) {
                /* empty line, ignore */
                dbg(lvl_debug, "*****checkpoint RESTORE-4, skipping empty line");
            }
            attrs[acnt] = NULL;
            g_free(line);
            line = NULL;
            if (data_next)
                data_curr = data_next + 1;
            else {
                data_curr = NULL;
                // TODO abort (last item is invalid as it has no coords)
            }
        } else if (line && coord_parse(line, projection_mg, &ca[ccnt])) {
            /* add coordinates until we hit a line without */
            ccnt++;
            g_free(line);
            line = NULL;
            if (data_next)
                data_curr = data_next + 1;
            else
                data_curr = NULL;
        } else {
            /* end of coordinates, find matching item */
            if (line) {
                g_free(line);
                line = NULL;
            }
            if (ccnt < 1) {
                /* not a complete item, possibly trailing empty line */
                dbg(lvl_debug, "*****checkpoint RESTORE-5, skipping incomplete item (possibly trailing empty line)");
                break;
            }

            pitem = g_new0(struct parsed_item, 1);
            pitem->id_hi = id_hi;
            pitem->id_lo = id_lo;
            pitem->type = type;
            pitem->flags = flags;
            pitem->coords = g_new0(struct coord, ccnt);
            for (i = 0; i < ccnt; i++)
                pitem->coords[i] = ca[i];
            pitem->coord_count = ccnt;
            pitem->attrs = attr_list_dup(attrs);
            for (i = 0; attrs[i]; i++) {
                g_free(attrs[i]);
                attrs[i] = NULL;
            }
            items = g_list_append(items, pitem);

            if (!data_curr)
                /* no more data to process, finish up */
                break;

            ccnt = -1;
        }
    } /* while 1 */

    /*
     * Walk through mapset and look for a routable item with a matching ID and geometry.
     * If no match is found, the map data has changed since we generated the cached segments and we
     * need to recreate the data. In this case, stop processing segments immediately and drop any
     * segments restored so far.
     */
    if (items) {
        dbg(lvl_debug, "*****checkpoint RESTORE-6, comparing items to map data");
        msh = mapset_open(ms);
        map_item = NULL;

        while (!map_item && (m = mapset_next(msh, 2))) {
            /* Skip traffic map (identified by the `attr_traffic` attribute) */
            if (map_get_attr(m, attr_traffic, &attr, NULL))
                continue;

            msel = traffic_location_get_rect(this_->location, map_projection(m));
            if (!msel)
                continue;
            mr = map_rect_new(m, msel);
            if (!mr) {
                map_selection_destroy(msel);
                msel = NULL;
                continue;
            }
            /*
             * Iterate through items in the map.
             */
            while ((map_item = map_rect_get_item(mr))) {
                /* If item is not routable, continue */
                if ((map_item->type < route_item_first) || (map_item->type > route_item_last))
                    continue;
                /* If road class is motorway, trunk or primary, ignore roads more than one level below */
                if ((this_->location->road_type == type_highway_land) || (this_->location->road_type == type_highway_city)) {
                    if ((map_item->type != type_highway_land) && (map_item->type != type_highway_city) &&
                            (map_item->type != type_street_n_lanes) && (map_item->type != type_ramp))
                        continue;
                } else if (this_->location->road_type == type_street_n_lanes) {
                    if ((map_item->type != type_highway_land) && (map_item->type != type_highway_city) &&
                            (map_item->type != type_street_n_lanes) && (map_item->type != type_ramp) &&
                            (map_item->type != type_street_4_land) && (map_item->type != type_street_4_city))
                        continue;
                } else if ((this_->location->road_type == type_street_4_land) || (this_->location->road_type == type_street_4_city)) {
                    if ((map_item->type != type_highway_land) && (map_item->type != type_highway_city) &&
                            (map_item->type != type_street_n_lanes) && (map_item->type != type_ramp) &&
                            (map_item->type != type_street_4_land) && (map_item->type != type_street_4_city) &&
                            (map_item->type != type_street_3_land) && (map_item->type != type_street_3_city))
                        continue;
                }
                /* Look for a matching item in the cache */
                for (curr_item = items; curr_item; curr_item = g_list_next(curr_item)) {
                    pitem = (struct parsed_item *) curr_item->data;

                    /* Skip already-matched items */
                    if (pitem->is_matched)
                        continue;
                    /* If IDs do not match, continue */
                    if ((map_item->id_hi != pitem->id_hi) || (map_item->id_lo != pitem->id_lo))
                        continue;
                    dbg(lvl_debug, "*****checkpoint RESTORE-6.0, comparing item 0x%x, 0x%x to map data",
                        pitem->id_hi, pitem->id_lo);
                    /* Get flags (access and other) for the item */
                    if (!(default_flags = item_get_default_flags(map_item->type)))
                        default_flags = &item_default_flags_value;
                    if (item_attr_get(map_item, attr_flags, &attr)) {
                        item_flags = attr.u.num;
                        segmented = (item_flags & AF_SEGMENTED);
                    } else {
                        item_flags = *default_flags;
                        segmented = 0;
                    }
                    /* Get maxspeed, if any */
                    item_attr_rewind(map_item);
                    if ((item_flags & AF_SPEED_LIMIT) && (item_attr_get(map_item, attr_maxspeed, &attr)))
                        maxspeed = attr.u.num;
                    else
                        maxspeed = INT_MAX;
                    /* Compare coordinates */
                    item_coord_rewind(map_item);
                    if (!segmented) {
                        for (i = 0; i < pitem->coord_count; i++) {
                            if (!item_coord_get(map_item, &map_c, 1)) {
                                /* map item has fewer coordinates than cached item */
                                dbg(lvl_debug, "*****checkpoint RESTORE-6.1, item 0x%x, 0x%x has fewer coordinates than cached item",
                                    pitem->id_hi, pitem->id_lo);
                                map_item = NULL;
                                break;
                            }
                            if ((map_c.x != pitem->coords[i].x) || (map_c.y != pitem->coords[i].y)) {
                                /* coordinate mismatch between map item and cached item */
                                dbg(lvl_debug, "*****checkpoint RESTORE-6.1, coordinate #%d for item 0x%x, 0x%x does not match",
                                    i, pitem->id_hi, pitem->id_lo);
                                map_item = NULL;
                                break;
                            }
                        }
                        if (map_item && item_coord_get(map_item, &map_c, 1)) {
                            /* map item has more coordinates than cached item */
                            dbg(lvl_debug, "*****checkpoint RESTORE-6.1, item 0x%x, 0x%x has more coordinates than cached item",
                                pitem->id_hi, pitem->id_lo);
                            map_item = NULL;
                            continue;
                        }
                    } else {
                        /* TODO implement comparison for segmented items */
                        dbg(lvl_debug, "*****checkpoint RESTORE-6.1, restoring segmented items is not supported yet");
                        map_item = NULL;
                    }
                    if (map_item) {
                        pitem->is_matched = 1;
                        for (i = 1; i < pitem->coord_count; i++)
                            pitem->length += transform_distance(map_projection(m), &(ca[i-1]), &(ca[i]));
                        loc_len += pitem->length;
                    }
                }
            }

            map_selection_destroy(msel);
            msel = NULL;
            map_rect_destroy(mr);
            mr = NULL;
        }
        mapset_close(msh);
        msh = NULL;
    } else {
        dbg(lvl_debug, "*****checkpoint RESTORE-6, no items to compare");
    }

    /* No items = no match; else examine each item */
    is_matched = !!items;
    for (curr_item = items; is_matched && curr_item; curr_item = g_list_next(curr_item)) {
        pitem = (struct parsed_item *) curr_item->data;
        if (!pitem->is_matched) {
            dbg(lvl_debug, "*****checkpoint RESTORE-6.2, item 0x%x, 0x%x is unmatched",
                pitem->id_hi, pitem->id_lo);
            is_matched = 0;
        }
    }

    if (is_matched) {
        dbg(lvl_debug, "*****checkpoint RESTORE-7, restoring items for message %s from cache", this_->id);
        seg_data = traffic_message_parse_events(this_);
        this_->priv->items = g_new0(struct item *, g_list_length(items) + 1);
        i = 0;
        for (curr_item = items; curr_item; curr_item = g_list_next(curr_item)) {
            pitem = (struct parsed_item *) curr_item->data;
            item = tm_add_item(map, pitem->type, pitem->id_hi, pitem->id_lo, pitem->flags, pitem->attrs,
                               pitem->coords, pitem->coord_count, this_->id);
            tm_item_add_message_data(item, this_->id,
                                     traffic_get_item_speed(item, seg_data, maxspeed),
                                     traffic_get_item_delay(seg_data->delay, pitem->length, loc_len),
                                     NULL, route);
            parsed_item_destroy(pitem);
            this_->priv->items[i] = item;
            i++;
        }
        g_list_free(items);
        items = NULL;
        g_free(seg_data);
    } else {
        dbg(lvl_debug, "*****checkpoint RESTORE-7, items for message %s need to be regenerated", this_->id);
    }

    /* clean up */
    for (curr_item = items; curr_item; curr_item = g_list_next(curr_item)) {
        pitem = (struct parsed_item *) curr_item->data;
        parsed_item_destroy(pitem);
    }
    if (this_->location->priv->txt_data) {
        g_free(this_->location->priv->txt_data);
        this_->location->priv->txt_data = NULL;
    }

    dbg(lvl_debug, "*****checkpoint RESTORE-8, done");
    return is_matched;
}

/**
 * @brief Prints a dump of a message to debug output.
 *
 * @param this_ The message to dump
 */
static void traffic_message_dump_to_stderr(struct traffic_message * this_) {
    int i, j;
    char * point_names[5] = {"From", "At", "Via", "Not via", "To"};
    struct traffic_point * points[5];
    char * timestamp = NULL;

    if (!this_) {
        dbg(lvl_debug, "(null)");
        return;
    }

    if (this_->location) {
        points[0] = this_->location->from;
        points[1] = this_->location->at;
        points[2] = this_->location->via;
        points[3] = this_->location->not_via;
        points[4] = this_->location->to;
    } else
        memset(&points, 0, sizeof(struct traffic_point *) * 5);

    dbg(lvl_debug, "id='%s', is_cancellation=%d, is_forecast=%d",
        this_->id, this_->is_cancellation, this_->is_forecast);
    if (this_->receive_time) {
        timestamp = time_to_iso8601(this_->receive_time);
        dbg(lvl_debug, "  First received: %s (%ld)", timestamp, this_->receive_time);
        g_free(timestamp);
    }
    if (this_->update_time) {
        timestamp = time_to_iso8601(this_->update_time);
        dbg(lvl_debug, "  Last updated:   %s (%ld)", timestamp, this_->update_time);
        g_free(timestamp);
    }
    if (this_->start_time) {
        timestamp = time_to_iso8601(this_->start_time);
        dbg(lvl_debug, "  Start time:     %s (%ld)", timestamp, this_->start_time);
        g_free(timestamp);
    }
    if (this_->end_time) {
        timestamp = time_to_iso8601(this_->end_time);
        dbg(lvl_debug, "  End time:       %s (%ld)", timestamp, this_->end_time);
        g_free(timestamp);
    }
    if (this_->expiration_time) {
        timestamp = time_to_iso8601(this_->expiration_time);
        dbg(lvl_debug, "  Expires:        %s (%ld)", timestamp, this_->expiration_time);
        g_free(timestamp);
    }

    /* dump replaced message IDs */
    dbg(lvl_debug, "  replaced_count=%d",
        this_->replaced_count);
    for (i = 0; i < this_->replaced_count; i++) {
        dbg(lvl_debug, "  Replaces: '%s'", this_->replaces[i]);
    }

    /* dump location */
    if (this_->location) {
        dbg(lvl_debug, "  Location: road_type='%s', road_ref='%s', road_name='%s'",
            item_to_name(this_->location->road_type), this_->location->road_ref,
            this_->location->road_name);
        dbg(lvl_debug, "    directionality=%d, destination='%s', direction='%s'",
            this_->location->directionality, this_->location->destination, this_->location->direction);
        dbg(lvl_debug, "    fuzziness=%s, ramps=%s, tmc_table='%s', tmc_direction=%+d",
            location_fuzziness_to_string(this_->location->fuzziness),
            location_ramps_to_string(this_->location->ramps), this_->location->tmc_table,
            this_->location->tmc_direction);
        for (i = 0; i < 5; i++) {
            if (points[i]) {
                dbg(lvl_debug, "    %s: lat=%.5f, lng=%.5f",
                    point_names[i], points[i]->coord.lat, points[i]->coord.lng);
                dbg(lvl_debug, "      junction_name='%s', junction_ref='%s', tmc_id='%s'",
                    points[i]->junction_name, points[i]->junction_ref, points[i]->tmc_id);
            } else {
                dbg(lvl_debug, "    %s: (null)",
                    point_names[i]);
            }
        }
    } else {
        dbg(lvl_debug, "  Location: null");
    }

    /* dump events */
    dbg(lvl_debug, "  event_count=%d",
        this_->event_count);
    for (i = 0; i < this_->event_count; i++) {
        dbg(lvl_debug, "  Event: event_class=%s, type=%s, length=%d m, speed=%d km/h",
            event_class_to_string(this_->events[i]->event_class),
            event_type_to_string(this_->events[i]->type),
            this_->events[i]->length, this_->events[i]->speed);
        /* TODO quantifier */

        /* dump supplementary information */
        dbg(lvl_debug, "    si_count=%d",
            this_->events[i]->si_count);
        for (j = 0; j < this_->events[i]->si_count; j++) {
            dbg(lvl_debug, "    Supplementary Information: si_class=%s, type=%s",
                si_class_to_string(this_->events[i]->si[j]->si_class),
                si_type_to_string(this_->events[i]->si[j]->type));
            /* TODO quantifier */
        }
    }
}

/**
 * @brief Whether the contents of a message are valid.
 *
 * This identifies any malformed messages in which mandatory members are not set.
 *
 * @return true if the message is valid, false if it is malformed
 */
static int traffic_message_is_valid(struct traffic_message * this_) {
    int i;
    int has_valid_events = 0;

    if (!this_->id || !this_->id[0]) {
        dbg(lvl_debug, "ID is NULL or empty");
        return 0;
    }
    if (!this_->receive_time || !this_->update_time) {
        dbg(lvl_debug, "%s: receive_time or update_time not supplied", this_->id);
        return 0;
    }
    if (!this_->is_cancellation) {
        if (!this_->expiration_time && !this_->end_time) {
            dbg(lvl_debug, "%s: not a cancellation, but neither expiration_time nor end_time supplied",
                this_->id);
            return 0;
        }
        if (!this_->location) {
            dbg(lvl_debug, "%s: not a cancellation, but no location supplied", this_->id);
            return 0;
        }
        if (!traffic_location_is_valid(this_->location)) {
            dbg(lvl_debug, "%s: not a cancellation, but location is invalid", this_->id);
            return 0;
        }
        if (!this_->event_count || !this_->events) {
            dbg(lvl_debug, "%s: not a cancellation, but no events supplied", this_->id);
            return 0;
        }
        for (i = 0; i < this_->event_count; i++)
            if (this_->events[i])
                has_valid_events |= traffic_event_is_valid(this_->events[i]);
        if (!has_valid_events) {
            dbg(lvl_debug, "%s: not a cancellation, but all events (%d in total) are invalid",
                this_->id, this_->event_count);
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Parses the events of a traffic message.
 *
 * @param message The message to parse
 *
 * @return A `struct seg_data`, or `NULL` if the message contains no usable information
 */
static struct seg_data * traffic_message_parse_events(struct traffic_message * this_) {
    struct seg_data * ret = NULL;

    int i, j;
    int has_flags = 0;
    int flags = 0;

    /* Default assumptions, used only if no explicit values are given */
    int speed = INT_MAX;
    int speed_penalty = 0;
    int speed_factor = 100;
    int delay = 0;

    for (i = 0; i < this_->event_count; i++) {
        if (this_->events[i]->speed != INT_MAX) {
            if (!ret)
                ret = seg_data_new();
            if (ret->speed > this_->events[i]->speed)
                ret->speed = this_->events[i]->speed;
        }
        if (this_->events[i]->event_class == event_class_congestion) {
            switch (this_->events[i]->type) {
            case event_congestion_heavy_traffic:
            case event_congestion_traffic_building_up:
            case event_congestion_traffic_heavier_than_normal:
            case event_congestion_traffic_much_heavier_than_normal:
                /* Heavy traffic: assume 10 km/h below the posted limit, unless explicitly specified */
                if ((this_->events[i]->speed == INT_MAX) && (speed_penalty < 10))
                    speed_penalty = 10;
                break;
            case event_congestion_slow_traffic:
            case event_congestion_traffic_congestion:
            case event_congestion_traffic_problem:
                /* Slow traffic or unspecified congestion: assume half the posted limit, unless explicitly specified */
                if ((this_->events[i]->speed == INT_MAX) && (speed_factor > 50))
                    speed_factor = 50;
                break;
            case event_congestion_queue:
                /* Queuing traffic: assume 20 km/h, unless explicitly specified */
                if ((this_->events[i]->speed == INT_MAX) && (speed > 20))
                    speed = 20;
                break;
            case event_congestion_stationary_traffic:
            case event_congestion_long_queue:
                /* Stationary traffic or long queues: assume 5 km/h, unless explicitly specified */
                if ((this_->events[i]->speed == INT_MAX) && (speed > 5))
                    speed = 5;
                break;
            default:
                break;
            }
        } else if (this_->events[i]->event_class == event_class_delay) {
            switch (this_->events[i]->type) {
            case event_delay_delay:
            case event_delay_long_delay:
                /* Delay or long delay: assume 30 minutes, unless explicitly specified */
                if (this_->events[i]->quantifier) {
                    if (!ret)
                        ret = seg_data_new();
                    if (ret->delay < this_->events[i]->quantifier->u.q_duration)
                        ret->delay = this_->events[i]->quantifier->u.q_duration;
                } else if (delay < 18000)
                    delay = 18000;
                break;
            case event_delay_very_long_delay:
                /* Very long delay: assume 1 hour, unless explicitly specified */
                if (this_->events[i]->quantifier) {
                    if (!ret)
                        ret = seg_data_new();
                    if (ret->delay < this_->events[i]->quantifier->u.q_duration)
                        ret->delay = this_->events[i]->quantifier->u.q_duration;
                } else if (delay < 36000)
                    delay = 36000;
                break;
            case event_delay_several_hours:
            case event_delay_uncertain_duration:
                /* Delay of several hours or uncertain duration: assume 3 hours */
                if (delay < 108000)
                    delay = 108000;
                break;
            default:
                break;
            }
        } else if (this_->events[i]->event_class == event_class_restriction) {
            switch (this_->events[i]->type) {
            case event_restriction_blocked:
            case event_restriction_blocked_ahead:
            case event_restriction_carriageway_blocked:
            case event_restriction_carriageway_closed:
            case event_restriction_closed:
            case event_restriction_closed_ahead:
                if (!ret)
                    ret = seg_data_new();
                ret->speed = 0;
                break;
            case event_restriction_intermittent_closures:
            case event_restriction_batch_service:
            case event_restriction_single_alternate_line_traffic:
                /* Assume 30% of the posted limit for all of these cases */
                if (speed_factor > 30)
                    speed_factor = 30;
                break;
            case event_restriction_lane_blocked:
            case event_restriction_lane_closed:
            case event_restriction_reduced_lanes:
                /* Assume speed is reduced proportionally to number of lanes, and never higher than 80 */
                speed = 80;
                /* TODO determine actual numbers of lanes */
                speed_factor = 67;
                break;
            case event_restriction_contraflow:
                /* Contraflow: assume 80, unless explicitly specified */
                speed = 80;
                break;
            /* restriction_speed_limit is not in the list: either it comes with a maxspeed attribute, which gets
             * evaluated regardless of the event it comes with, and if it doesn’t come with one, it carries no
             * useful information. */
            default:
                break;
            }
        }

        for (j = 0; j < this_->events[i]->si_count; j++) {
            switch (this_->events[i]->si[j]->type) {
            case si_vehicle_all:
                /* For all vehicles */
                flags |= AF_ALL;
                has_flags = 1;
                break;
            case si_vehicle_bus:
                /* For buses only */
                /* TODO what about other (e.g. chartered) buses? */
                flags |= AF_PUBLIC_BUS;
                has_flags = 1;
                break;
            case si_vehicle_car:
                /* For cars only */
                flags |= AF_CAR;
                has_flags = 1;
                break;
            case si_vehicle_car_with_caravan:
                /* For cars with caravans only */
                /* TODO no matching flag */
                has_flags = 1;
                break;
            case si_vehicle_car_with_trailer:
                /* For cars with trailers only */
                /* TODO no matching flag */
                has_flags = 1;
                break;
            case si_vehicle_hazmat:
                /* For hazardous loads only */
                flags |= AF_DANGEROUS_GOODS;
                has_flags = 1;
                break;
            case si_vehicle_hgv:
                /* For heavy trucks only */
                flags |= AF_TRANSPORT_TRUCK | AF_DELIVERY_TRUCK;
                has_flags = 1;
                break;
            case si_vehicle_motor:
                /* For all motor vehicles */
                flags |= AF_MOTORIZED_FAST | AF_MOPED;
                has_flags = 1;
                break;
            case si_vehicle_with_trailer:
                /* For vehicles with trailers only */
                /* TODO no matching flag */
                has_flags = 1;
                break;
            default:
                break;
            }
        }
    }

    /* if no vehicle type is specified in supplementary information, assume all */
    if (!has_flags) {
        if (this_->location->road_type == type_line_unspecified)
            flags = AF_ALL;
        else
            flags = AF_MOTORIZED_FAST | AF_MOPED;
    }

    if (!ret)
        ret = seg_data_new();

    /* use implicit values if no explicit ones are given */
    if ((speed != INT_MAX) || speed_penalty || (speed_factor != 100) || delay) {
        if (ret->speed == INT_MAX) {
            ret->speed = speed;
            ret->speed_penalty = speed_penalty;
            ret->speed_factor = speed_factor;
        }
        if (!ret->delay)
            ret->delay = delay;
    }

    ret->dir = this_->location->directionality;
    ret->flags = flags;

    return ret;
}

/**
 * @brief Removes message data from the items associated with a message.
 *
 * Removing message data also triggers an update of the affected items’ attributes.
 *
 * It is possible to skip items associated with a particular message from being removed by passing that
 * message as the `new` argument. This is used for message updates, as this function is called after the
 * items associated with both the old and the new message have already been updated. Skipping items
 * referenced by `new` ensures that message data is only stripped from items which are no longer being
 * referenced by the updated message.
 *
 * If the IDs of `old` and `new` differ, `new` is ignored.
 *
 * @param old The message whose data it so be removed from its associated items
 * @param new If non-NULL, items referenced by this message will be skipped, see description
 * @param route The route affected by the changes
 */
static void traffic_message_remove_item_data(struct traffic_message * old, struct traffic_message * new,
        struct route * route) {
    int i, j;
    int skip;
    struct item_priv * ip;
    GList * msglist;
    struct item_msg_priv * msgdata;

    if (new && strcmp(old->id, new->id))
        new = NULL;

    for (i = 0; old->priv->items && old->priv->items[i]; i++) {
        skip = 0;
        if (new)
            for (j = 0; new->priv->items && new->priv->items[j] && !skip; j++)
                skip |= (old->priv->items[i] == new->priv->items[j]);
        if (!skip) {
            ip = (struct item_priv *) old->priv->items[i]->priv_data;
            for (msglist = ip->message_data; msglist; ) {
                msgdata = (struct item_msg_priv *) msglist->data;
                msglist = g_list_next(msglist);
                if (!strcmp(msgdata->message_id, old->id)) {
                    ip->message_data = g_list_remove(ip->message_data, msgdata);
                    g_free(msgdata->message_id);
                    g_free(msgdata);
                }
            }
            tm_item_update_attrs(old->priv->items[i], route);
        }
    }
}

/**
 * @brief Ensures the traffic instance points to valid shared data.
 *
 * This method first examines all registered traffic instances to see if one of them has the `shared`
 * member set. If that is the case, the current instance copies the `shared` pointer of the other
 * instance. Otherwise a new `struct traffic_shared_priv` is created and its address stored in `shared`.
 *
 * Calling this method on a traffic instance with a non-NULL `shared` member has no effect.
 *
 * @param this_ The traffic instance
 */
static void traffic_set_shared(struct traffic *this_) {
    struct attr_iter *iter;
    struct attr attr;
    struct traffic * traffic;

    dbg(lvl_debug, "enter");

    if (!this_->shared) {
        iter = navit_attr_iter_new(NULL);
        while (navit_get_attr(this_->navit, attr_traffic, &attr, iter)) {
            traffic = (struct traffic *) attr.u.navit_object;
            if (traffic->shared)
                this_->shared = traffic->shared;
        }
        navit_attr_iter_destroy(iter);
    }

    if (!this_->shared) {
        this_->shared = g_new0(struct traffic_shared_priv, 1);
    }
}

/**
 * @brief Dumps all currently active traffic messages to an XML file.
 */
static void traffic_dump_messages_to_xml(struct traffic_shared_priv * shared) {
    /* add the configuration directory to the name of the file to use */
    char *traffic_filename = g_strjoin(NULL, navit_get_user_data_directory(TRUE),
                                       "/traffic.xml", NULL);
    GList * msgiter;
    struct traffic_message * message;
    char * strval;
    char * point_names[5] = {"from", "at", "via", "not_via", "to"};
    struct traffic_point * points[5];
    int i, j;
    struct item ** curr;

    if (traffic_filename) {
        FILE *f = fopen(traffic_filename,"w");
        if (f) {
            fprintf(f, "<navit_messages>\n");
            for (msgiter = shared->messages; msgiter; msgiter = g_list_next(msgiter)) {
                message = (struct traffic_message *) msgiter->data;
                points[0] = message->location->from;
                points[1] = message->location->at;
                points[2] = message->location->via;
                points[3] = message->location->not_via;
                points[4] = message->location->to;

                strval = time_to_iso8601(message->receive_time);
                fprintf(f, "  <message id=\"%s\" receive_time=\"%s\"", message->id, strval);
                g_free(strval);
                strval = time_to_iso8601(message->update_time);
                fprintf(f, " update_time=\"%s\"", strval);
                g_free(strval);
                if (message->start_time) {
                    strval = time_to_iso8601(message->start_time);
                    fprintf(f, " start_time=\"%s\"", strval);
                    g_free(strval);
                }
                if (message->end_time) {
                    strval = time_to_iso8601(message->end_time);
                    fprintf(f, " end_time=\"%s\"", strval);
                    g_free(strval);
                }
                if (message->expiration_time) {
                    strval = time_to_iso8601(message->expiration_time);
                    fprintf(f, " expiration_time=\"%s\"", strval);
                    g_free(strval);
                }
                if (message->is_forecast)
                    fprintf(f, " forecast=\"%d\"", message->is_forecast);
                fprintf(f, ">\n");

                fprintf(f, "    <location directionality=\"%s\"",
                        message->location->directionality == location_dir_one ? "ONE_DIRECTION" : "BOTH_DIRECTIONS");
                if (message->location->fuzziness)
                    fprintf(f, " fuzziness=\"%s\"", location_fuzziness_to_string(message->location->fuzziness));
                if (message->location->ramps)
                    fprintf(f, " ramps=\"%s\"", location_ramps_to_string(message->location->ramps));
                if (message->location->road_type != type_line_unspecified)
                    fprintf(f, " road_class=\"%s\"", item_to_name(message->location->road_type));
                if (message->location->road_ref)
                    fprintf(f, " road_ref=\"%s\"", message->location->road_ref);
                if (message->location->road_name)
                    fprintf(f, " road_name=\"%s\"", message->location->road_name);
                if (message->location->destination)
                    fprintf(f, " destination=\"%s\"", message->location->destination);
                if (message->location->direction)
                    fprintf(f, " direction=\"%s\"", message->location->direction);
                if ((message->location->directionality == location_dir_one)
                        && message->location->tmc_direction)
                    fprintf(f, " tmc_direction=\"%+d\"", message->location->tmc_direction);
                if (message->location->tmc_table)
                    fprintf(f, " tmc_table=\"%s\"", message->location->tmc_table);
                fprintf(f, ">\n");

                for (i = 0; i < 5; i++)
                    if (points[i]) {
                        fprintf(f, "      <%s", point_names[i]);
                        if (points[i]->junction_name)
                            fprintf(f, " junction_name=\"%s\"", points[i]->junction_name);
                        if (points[i]->junction_ref)
                            fprintf(f, " junction_ref=\"%s\"", points[i]->junction_ref);
                        if (points[i]->tmc_id)
                            fprintf(f, " tmc_id=\"%s\"", points[i]->tmc_id);
                        fprintf(f, ">");
                        fprintf(f, "%+f %+f", points[i]->coord.lat, points[i]->coord.lng);
                        fprintf(f, "</%s>\n", point_names[i]);
                    }

                if (message->priv->items) {
                    fprintf(f, "      <navit_items>\n");
                    for (curr = message->priv->items; *curr; curr++) {
                        tm_item_dump_to_file(*curr, f);
                    }
                    fprintf(f, "      </navit_items>\n");
                } else if (message->location->priv->txt_data)
                    fprintf(f, "      <navit_items>%s</navit_items>\n", message->location->priv->txt_data);

                fprintf(f, "    </location>\n");

                fprintf(f, "    <events>\n");
                for (i = 0; i < message->event_count; i++) {
                    fprintf(f, "      <event class=\"%s\" type=\"%s\"",
                            event_class_to_string(message->events[i]->event_class),
                            event_type_to_string(message->events[i]->type));
                    if (message->events[i]->length >= 0)
                        fprintf(f, " length=\"%d\"", message->events[i]->length);
                    if (message->events[i]->speed != INT_MAX)
                        fprintf(f, " speed=\"%d\"", message->events[i]->speed);
                    /* TODO message->events[i]->quantifier */
                    fprintf(f, ">\n");

                    for (j = 0; j < message->events[i]->si_count; j++) {
                        fprintf(f, "        <supplementary_info class=\"%s\" type=\"%s\"",
                                si_class_to_string(message->events[i]->si[j]->si_class),
                                si_type_to_string(message->events[i]->si[j]->type));
                        /* TODO message->events[i]->si[j]->quantifier */
                        fprintf(f, "/>\n");
                    }

                    fprintf(f, "      </event>\n");
                }
                fprintf(f, "    </events>\n");
                fprintf(f, "  </message>\n");
            }
            fprintf(f, "</navit_messages>\n");
            fclose(f);
        } else {
            dbg(lvl_error,"could not open file for traffic messages");

        } /* else - if (f) */
        g_free(traffic_filename);			/* free the file name */
    } /* if (traffic_filename) */
}

/**
 * @brief Processes new traffic messages.
 *
 * This is the internal backend for `traffic_process_messages()`. It is also used internally.
 *
 * The behavior of this function can be controlled via flags.
 *
 * `PROCESS_MESSAGES_PURGE_EXPIRED` causes expired messages to be purged from the message store after
 * new messages have been processed. It is intended to be used with timer-triggered calls.
 *
 * `PROCESS_MESSAGES_NO_DUMP_STORE` prevents saving of the message store to disk, intended to be used
 * when reading stored message data on startup.
 *
 * Traffic messages are always read from `this->shared->message_queue`. It can be empty, which makes sense e.g. when
 * the `PROCESS_MESSAGES_PURGE_EXPIRED` flag is used, to just purge expired messages.
 *
 * @param this_ The traffic instance
 * @param flags Flags, see description
 *
 * @return A combination of flags, `MESSAGE_UPDATE_MESSAGES` indicating that new messages were processed
 * and `MESSAGE_UPDATE_SEGMENTS` that segments were changed
 */
/* TODO what if the update for a still-valid message expires in the past? */
static int traffic_process_messages_int(struct traffic * this_, int flags) {
    /* Start and current time */
    struct timeval start, now;

    /* Current message */
    struct traffic_message * message;

    /* Return value */
    int ret = 0;

    /* Number of messages processed so far */
    int i = 0;

    /* Iterator over messages */
    GList * msg_iter;

    /* Stored message being compared */
    struct traffic_message * stored_msg;

    /* Messages to remove */
    GList * msgs_to_remove = NULL;

    /* Pointer into messages[i]->replaces */
    char ** replaces;

    /* Attributes for traffic distortions generated from the current traffic message */
    struct seg_data * data;

    /* Message replaced by the current one whose segments can be reused */
    struct traffic_message * swap_candidate;

    /* Temporary store for swapping locations and items */
    struct traffic_location * swap_location;
    struct item ** swap_items;

    /* Holds queried attributes */
    struct attr attr;

    /* Map selections for the location and the route, and iterator */
    struct map_selection * loc_ms, * rt_ms, * ms_iter;

    /* Time elapsed since start */
    double msec = 0;

    if (this_->shared->message_queue)
        dbg(lvl_debug, "*****enter, %d messages in queue", g_list_length(this_->shared->message_queue));

    gettimeofday(&start, NULL);
    for (; this_->shared->message_queue && (msec < TIME_SLICE);
            this_->shared->message_queue = g_list_remove(this_->shared->message_queue, message)) {
        message = (struct traffic_message *) this_->shared->message_queue->data;
        i++;
        if (message->expiration_time < time(NULL)) {
            dbg(lvl_debug, "message is no longer valid, ignoring");
            traffic_message_destroy(message);
        } else {
            dbg(lvl_debug, "*****checkpoint PROCESS-1, id='%s'", message->id);
            ret |= MESSAGE_UPDATE_MESSAGES;

            for (msg_iter = this_->shared->messages; msg_iter; msg_iter = g_list_next(msg_iter)) {
                stored_msg = (struct traffic_message *) msg_iter->data;
                if (!strcmp(stored_msg->id, message->id))
                    msgs_to_remove = g_list_append(msgs_to_remove, stored_msg);
                else
                    for (replaces = ((struct traffic_message *) this_->shared->message_queue->data)->replaces; replaces; replaces++)
                        if (!strcmp(stored_msg->id, *replaces) && !g_list_find(msgs_to_remove, message))
                            msgs_to_remove = g_list_append(msgs_to_remove, stored_msg);
            }

            if (!message->is_cancellation) {
                dbg(lvl_debug, "*****checkpoint PROCESS-2");
                /* if the message is not just a cancellation, store it and match it to the map */
                data = traffic_message_parse_events(message);
                swap_candidate = NULL;

                dbg(lvl_debug, "*****checkpoint PROCESS-3");
                /* check if any of the replaced messages has the same location and segment data */
                for (msg_iter = msgs_to_remove; msg_iter && !swap_candidate; msg_iter = g_list_next(msg_iter)) {
                    stored_msg = (struct traffic_message *) msg_iter->data;
                    if (seg_data_equals(data, traffic_message_parse_events(stored_msg))
                            && traffic_location_equals(message->location, stored_msg->location))
                        swap_candidate = stored_msg;
                }

                if (swap_candidate) {
                    dbg(lvl_debug, "*****checkpoint PROCESS-4, swap candidate found");
                    /* reuse location and segments if we are replacing a matching message */
                    swap_location = message->location;
                    swap_items = message->priv->items;
                    message->location = swap_candidate->location;
                    message->priv->items = swap_candidate->priv->items;
                    swap_candidate->location = swap_location;
                    swap_candidate->priv->items = swap_items;
                } else {
                    dbg(lvl_debug, "*****checkpoint PROCESS-4, need to find matching segments");
                    if (route_get_attr(this_->shared->rt, attr_route_status, &attr, NULL)
                            && route_get_pos(this_->shared->rt)
                            && ((attr.u.num & route_status_destination_set))) {
                        traffic_location_set_enclosing_rect(message->location, NULL);
                        loc_ms = traffic_location_get_rect(message->location, traffic_map_meth.pro);
                        rt_ms = route_get_selection(this_->shared->rt);
                        for (ms_iter = rt_ms; ms_iter; ms_iter = ms_iter->next)
                            if (coord_rect_overlap(&(loc_ms->u.c_rect), &(ms_iter->u.c_rect))) {
                                /*
                                 * If we have cached segments, restore them.
                                 */
                                if (message->location->priv->txt_data) {
                                    dbg(lvl_debug, "location has txt_data, trying to restore segments");
                                    traffic_message_restore_segments(message, this_->shared->ms,
                                                                     this_->shared->map, this_->shared->rt);
                                } else {
                                    dbg(lvl_debug, "location has no txt_data, nothing to restore");
                                }
                                /*
                                 * We need to find matching segments from scratch.
                                 * This needs to happen immediately if we have a route and the location is within its map
                                 * selection, as the message might have an effect on the route. Otherwise this operation
                                 * is deferred until a rectangle overlapping with the location is queried.
                                 */
                                if (!message->priv->items) {
                                    /* TODO do this in an idle loop, not here */
                                    traffic_message_add_segments(message, this_->shared->ms, data,
                                                                 this_->shared->map, this_->shared->rt);
                                    break;
                                    map_selection_destroy(loc_ms);
                                    map_selection_destroy(rt_ms);
                                }
                            }
                    }
                    ret |= MESSAGE_UPDATE_SEGMENTS;
                }

                g_free(data);

                /* store message */
                this_->shared->messages = g_list_append(this_->shared->messages, message);
                dbg(lvl_debug, "*****checkpoint PROCESS-5");
            }

            /* delete replaced messages */
            if (msgs_to_remove) {
                dbg(lvl_debug, "*****checkpoint PROCESS (messages to remove, start)");
                for (msg_iter = msgs_to_remove; msg_iter; msg_iter = g_list_next(msg_iter)) {
                    stored_msg = (struct traffic_message *) msg_iter->data;
                    if (stored_msg->priv->items)
                        ret |= MESSAGE_UPDATE_SEGMENTS;
                    this_->shared->messages = g_list_remove_all(this_->shared->messages, stored_msg);
                    traffic_message_remove_item_data(stored_msg, message, this_->shared->rt);
                    traffic_message_destroy(stored_msg);
                }

                g_list_free(msgs_to_remove);
                msgs_to_remove = NULL;
                dbg(lvl_debug, "*****checkpoint PROCESS (messages to remove, end)");
            }

            traffic_message_dump_to_stderr(message);

            if (message->is_cancellation)
                traffic_message_destroy(message);

            dbg(lvl_debug, "*****checkpoint PROCESS-6");
        }
        gettimeofday(&now, NULL);
        msec = (now.tv_usec - start.tv_usec) / ((double)1000) + (now.tv_sec - start.tv_sec) * 1000;
    }

    if (i)
        dbg(lvl_debug, "processed %d message(s), %d still in queue", i, g_list_length(this_->shared->message_queue));

    if (this_->shared->message_queue) {
        /* if we're in the middle of the queue, trigger a redraw (if needed) and exit */
        if ((ret & MESSAGE_UPDATE_SEGMENTS) && (navit_get_ready(this_->navit) == 3))
            navit_draw_async(this_->navit, 1);
        return ret;
    } else {
        /* last pass, remove our idle event and callback */
        if (this_->idle_ev)
            event_remove_idle(this_->idle_ev);
        if (this_->idle_cb)
            callback_destroy(this_->idle_cb);
        this_->idle_ev = NULL;
        this_->idle_cb = NULL;
    }

    if (flags & PROCESS_MESSAGES_PURGE_EXPIRED) {
        /* find and remove expired messages */
        for (msg_iter = this_->shared->messages; msg_iter; msg_iter = g_list_next(msg_iter)) {
            stored_msg = (struct traffic_message *) msg_iter->data;
            if (stored_msg->expiration_time < time(NULL))
                msgs_to_remove = g_list_append(msgs_to_remove, stored_msg);
        }

        if (msgs_to_remove) {
            for (msg_iter = msgs_to_remove; msg_iter; msg_iter = g_list_next(msg_iter)) {
                stored_msg = (struct traffic_message *) msg_iter->data;
                if (stored_msg->priv->items)
                    ret |= MESSAGE_UPDATE_SEGMENTS;
                this_->shared->messages = g_list_remove_all(this_->shared->messages, stored_msg);
                traffic_message_remove_item_data(stored_msg, NULL, this_->shared->rt);
                traffic_message_destroy(stored_msg);
            }

            dbg(lvl_debug, "%d message(s) expired", g_list_length(msgs_to_remove));

            g_list_free(msgs_to_remove);
        }
    }

    if (ret && !(flags & PROCESS_MESSAGES_NO_DUMP_STORE)) {
#ifdef TRAFFIC_DEBUG
        /* dump map if messages have been added, deleted or expired */
        tm_dump_to_textfile(this_->map);
#endif

        /* dump message store if new messages have been received */
        traffic_dump_messages_to_xml(this_->shared);
    }

    /* TODO see comment on route_recalculate_partial about thread-safety */
    route_recalculate_partial(this_->shared->rt);

    /* trigger redraw if segments have changed */
    if ((ret & MESSAGE_UPDATE_SEGMENTS) && (navit_get_ready(this_->navit) == 3))
        navit_draw_async(this_->navit, 1);

    return ret;
}

/**
 * @brief The loop function for the traffic module.
 *
 * This function polls backends for new messages and processes them by inserting, removing or modifying
 * traffic distortions and triggering route recalculations as needed.
 */
static void traffic_loop(struct traffic * this_) {
    struct traffic_message ** messages;
    struct traffic_message ** cur_msg;

    messages = this_->meth.get_messages(this_->priv);
    for (cur_msg = messages; cur_msg && *cur_msg; cur_msg++)
        this_->shared->message_queue = g_list_append(this_->shared->message_queue, *cur_msg);
    g_free(messages);

    /* make sure traffic_process_messages_int runs at least once to ensure purging of expired messages */
    if (this_->shared->message_queue) {
        if (this_->idle_ev)
            event_remove_idle(this_->idle_ev);
        if (this_->idle_cb)
            callback_destroy(this_->idle_cb);
        this_->idle_cb = callback_new_2(callback_cast(traffic_process_messages_int),
                                        this_, PROCESS_MESSAGES_PURGE_EXPIRED);
        this_->idle_ev = event_add_idle(50, this_->idle_cb);
    } else
        traffic_process_messages_int(this_, PROCESS_MESSAGES_PURGE_EXPIRED);
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
static struct traffic * traffic_new(struct attr *parent, struct attr **attrs) {
    struct traffic *this_;
    struct traffic_priv *(*traffic_new)(struct navit *nav, struct traffic_methods *meth,
                                        struct attr **attrs, struct callback_list *cbl);
    struct attr *attr;

    attr = attr_search(attrs, attr_type);
    if (!attr) {
        dbg(lvl_error, "type missing");
        return NULL;
    }
    dbg(lvl_debug, "type='%s'", attr->u.str);
    traffic_new = plugin_get_category_traffic(attr->u.str);
    dbg(lvl_debug, "new=%p", traffic_new);
    if (!traffic_new) {
        dbg(lvl_error, "wrong type '%s'", attr->u.str);
        return NULL;
    }
    this_ = (struct traffic *) navit_object_new(attrs, &traffic_func, sizeof(struct traffic));
    if (parent->type == attr_navit)
        this_->navit = parent->u.navit;
    else {
        dbg(lvl_error, "wrong parent type '%s', only navit is permitted", attr_to_name(parent->type));
        navit_object_destroy((struct navit_object *) this_);
        return NULL;
    }

    this_->priv = traffic_new(parent->u.navit, &this_->meth, this_->attrs, NULL);
    dbg(lvl_debug, "get_messages=%p", this_->meth.get_messages);
    dbg(lvl_debug, "priv=%p", this_->priv);
    if (!this_->priv) {
        dbg(lvl_error, "plugin initialization failed");
        navit_object_destroy((struct navit_object *) this_);
        return NULL;
    }
    dbg(lvl_debug,"return %p", this_);

    // TODO do this once and cycle through all plugins
    this_->callback = callback_new_1(callback_cast(traffic_loop), this_);
    this_->timeout = event_add_timeout(1000, 1, this_->callback); // TODO make interval configurable

    if (!this_->shared)
        traffic_set_shared(this_);

    return this_;
}

/**
 * @brief Creates a new XML element structure.
 *
 * Note that the structure of `names` and `values` may differ between XML libraries. Behavior is indicated by the
 * `XML_ATTR_DISTANCE` constant.
 *
 * If `XML_ATTR_DISTANCE == 1`, `names` and `values` are two separate arrays, and `values[n]` is the value that
 * corresponds to `names[n]`.
 *
 * If `XML_ATTR_DISTANCE == 2`, attribute names and values are kept in a single array in which names and values
 * alternate, names first. In this case, `names` points to the array while `values` points to its second element, i.e.
 * the first value. In this case, `value` is invalid for an empty array, and dereferencing it may segfault.
 *
 * @param tag_name The tag name
 * @param names Attribute names
 * @param values Attribute values
 */
static struct xml_element * traffic_xml_element_new(const char *tag_name, const char **names,
        const char **values) {
    struct xml_element * ret = g_new0(struct xml_element, 1);
    const char ** in;
    char ** out;

    ret->tag_name = g_strdup(tag_name);
    if (names) {
        ret->names = g_new0(char *, g_strv_length((gchar **) names) / XML_ATTR_DISTANCE + 1);
        in = names;
        out = ret->names;
        while (*in) {
            *out++ = g_strdup(*in);
            in += XML_ATTR_DISTANCE;
        }
    }
    /* extra check for mixed name-value array */
    if (names && *names && values) {
#if XML_ATTR_DISTANCE == 1
        ret->values = g_new0(char *, g_strv_length((gchar **) values) + 1);
#else
        ret->values = g_new0(char *, g_strv_length((gchar **) values) / XML_ATTR_DISTANCE + 2);
#endif
        in = values;
        out = ret->values;
        while (*in) {
            *out++ = g_strdup(*in++);
#if XML_ATTR_DISTANCE > 1
            if (*in)
                in++;
#endif
        }
    }
    return ret;
}

/**
 * @brief Frees up an XML element structure.
 *
 * This will free up the memory used by the struct and all its members.
 */
static void traffic_xml_element_destroy(struct xml_element * this_) {
    void ** iter;

    g_free(this_->tag_name);
    if (this_->names) {
        for (iter = (void **) this_->names; *iter; iter++)
            g_free(*iter);
        g_free(this_->names);
    }
    if (this_->values) {
        for (iter = (void **) this_->values; *iter; iter++)
            g_free(*iter);
        g_free(this_->values);
    }
    g_free(this_->text);
    g_free(this_);
}

/**
 * @brief Retrieves the value of an XML attribute.
 *
 * @param name The name of the attribute to retrieve
 * @param names All attribute names
 * @param values Attribute values (indices correspond to `names`)
 *
 * @return If `names` contains `name`, the corresponding value is returned, else NULL
 */
static char * traffic_xml_get_attr(const char * attr, char ** names, char ** values) {
    int i;
    for (i = 0; names[i] && values[i]; i++) {
        if (!g_ascii_strcasecmp(attr, names[i]))
            return values[i];
    }
    return NULL;
}

/**
 * @brief Whether the tag stack represents a hierarchy of elements which is recognized.
 *
 * @param state The XML parser state
 *
 * @return True if the stack is valid, false if invalid. An empty stack is considered invalid.
 */
static int traffic_xml_is_tagstack_valid(struct xml_state * state) {
    int ret = 0;
    GList * tagiter;
    struct xml_element * el, * el_parent;

    for (tagiter = g_list_last(state->tagstack); tagiter; tagiter = g_list_previous(tagiter)) {
        el = (struct xml_element *) tagiter->data;
        el_parent = tagiter->next ? tagiter->next->data : NULL;

        if (!g_ascii_strcasecmp(el->tag_name, "navit_messages")
                || !g_ascii_strcasecmp(el->tag_name, "feed"))
            ret = !tagiter->next;
        else if (!g_ascii_strcasecmp((char *) el->tag_name, "message"))
            ret = (!el_parent
                   || !g_ascii_strcasecmp(el_parent->tag_name, "navit_messages")
                   || !g_ascii_strcasecmp(el_parent->tag_name, "feed"));
        else if (!g_ascii_strcasecmp(el->tag_name, "events")
                 || !g_ascii_strcasecmp(el->tag_name, "location")
                 || !g_ascii_strcasecmp(el->tag_name, "merge"))
            ret = (el_parent && !g_ascii_strcasecmp(el_parent->tag_name, "message"));
        else if (!g_ascii_strcasecmp(el->tag_name, "event"))
            ret = (el_parent && !g_ascii_strcasecmp(el_parent->tag_name, "events"));
        else if (!g_ascii_strcasecmp(el->tag_name, "from")
                 || !g_ascii_strcasecmp(el->tag_name, "to")
                 || !g_ascii_strcasecmp(el->tag_name, "at")
                 || !g_ascii_strcasecmp(el->tag_name, "via")
                 || !g_ascii_strcasecmp(el->tag_name, "not_via")
                 || !g_ascii_strcasecmp(el->tag_name, "navit_items"))
            ret = (el_parent && !g_ascii_strcasecmp(el_parent->tag_name, "location"));
        else if (!g_ascii_strcasecmp(el->tag_name, "supplementary_info"))
            ret = (el_parent && !g_ascii_strcasecmp(el_parent->tag_name, "event"));
        else if (!g_ascii_strcasecmp(el->tag_name, "replaces"))
            ret = (el_parent && !g_ascii_strcasecmp(el_parent->tag_name, "merge"));
        else
            ret = 0;

        if (!ret)
            break;
    }

    return ret;
}

/**
 * @brief Callback function which gets called when an opening tag is encountered.
 *
 * @param tag_name The tag name
 * @param names Attribute names
 * @param values Attribute values (indices correspond to `names`)
 * @param data Points to a `struct xml_state` holding parser state
 */
static void traffic_xml_start(xml_context *dummy, const char *tag_name, const char **names,
                              const char **values, void *data, GError **error) {
    struct xml_state * state = (struct xml_state *) data;
    struct xml_element * el;

    el = traffic_xml_element_new(tag_name, names, values);
    state->tagstack = g_list_prepend(state->tagstack, el);
    state->is_opened = 1;
    state->is_valid = traffic_xml_is_tagstack_valid(state);
    if (!state->is_valid)
        return;

    dbg(lvl_debug, "OPEN: %s", tag_name);

    if (!g_ascii_strcasecmp((char *) tag_name, "supplementary_info")) {
        state->si = g_list_append(state->si, traffic_suppl_info_new(
                                      si_class_new(traffic_xml_get_attr("class", el->names, el->values)),
                                      si_type_new(traffic_xml_get_attr("type", el->names, el->values)),
                                      /* TODO quantifier */
                                      NULL));
    } else if (!g_ascii_strcasecmp((char *) tag_name, "replaces")) {
        /* TODO */
    }

    /*
     * No handling necessary for:
     *
     * navit_messages: No attributes, everything handled in children's callbacks
     * feed: No attributes, everything handled in children's callbacks
     * message: Everything handled in end callback
     * events: No attributes, everything handled in children's callbacks
     * location: Everything handled in end callback
     * event: Everything handled in end callback
     * merge: No attributes, everything handled in children's callbacks
     * from, to, at, via, not_via: Everything handled in end callback
     * navit_items: Everything handled in end callback
     */
}

/**
 * @brief Callback function which gets called when a closing tag is encountered.
 *
 * @param tag_name The tag name
 * @param data Points to a `struct xml_state` holding parser state
 */
static void traffic_xml_end(xml_context *dummy, const char *tag_name, void *data, GError **error) {
    struct xml_state * state = (struct xml_state *) data;
    struct xml_element * el = state->tagstack ? (struct xml_element *) state->tagstack->data : NULL;
    struct traffic_message * message;
    struct traffic_point ** point = NULL;

    /* Iterator and child element count */
    int i, count;

    /* Child elements */
    void ** children = NULL;

    /* Iterator for children in GList */
    GList * iter;

    /* Some elements we need to check for null */
    char * tmc_direction;
    char * length;
    char * speed;

    /* New traffic event */
    struct traffic_event * event = NULL;

    float lat, lon;

    if (state->is_valid) {
        dbg(lvl_debug, "  END:  %s", tag_name);

        if (!g_ascii_strcasecmp((char *) tag_name, "message")) {
            count = g_list_length(state->events);
            if (count) {
                children = (void **) g_new0(struct traffic_event *, count);
                iter = state->events;
                for (i = 0; iter && (i < count); i++) {
                    children[i] = iter->data;
                    iter = g_list_next(iter);
                }
            }
            message = traffic_message_new(traffic_xml_get_attr("id", el->names, el->values),
                                          time_new(traffic_xml_get_attr("receive_time", el->names, el->values)),
                                          time_new(traffic_xml_get_attr("update_time", el->names, el->values)),
                                          time_new(traffic_xml_get_attr("expiration_time", el->names, el->values)),
                                          time_new(traffic_xml_get_attr("start_time", el->names, el->values)),
                                          time_new(traffic_xml_get_attr("end_time", el->names, el->values)),
                                          boolean_new(traffic_xml_get_attr("cancellation", el->names, el->values), 0),
                                          boolean_new(traffic_xml_get_attr("forecast", el->names, el->values), 0),
                                          /* TODO replaces */
                                          0, NULL,
                                          state->location,
                                          count,
                                          (struct traffic_event **) children);
            if (!traffic_message_is_valid(message)) {
                dbg(lvl_error, "%s: malformed message detected, skipping", message->id);
                traffic_message_destroy(message);
            } else
                state->messages = g_list_append(state->messages, message);
            g_free(children);
            state->location = NULL;
            g_list_free(state->events);
            state->events = NULL;
            /* TODO replaces */
        } else if (!g_ascii_strcasecmp((char *) tag_name, "location")) {
            tmc_direction = traffic_xml_get_attr("tmc_direction", el->names, el->values);
            state->location = traffic_location_new(state->at, state->from,
                                                   state->to, state->via, state->not_via,
                                                   traffic_xml_get_attr("destination", el->names, el->values),
                                                   traffic_xml_get_attr("direction", el->names, el->values),
                                                   location_dir_new(traffic_xml_get_attr("directionality", el->names, el->values)),
                                                   location_fuzziness_new(traffic_xml_get_attr("fuzziness", el->names, el->values)),
                                                   location_ramps_new(traffic_xml_get_attr("ramps", el->names, el->values)),
                                                   item_type_from_road_type(traffic_xml_get_attr("road_class", el->names, el->values),
                                                           /* TODO revisit default for road_is_urban */
                                                           boolean_new(traffic_xml_get_attr("road_is_urban", el->names, el->values), 0)),
                                                   traffic_xml_get_attr("road_name", el->names, el->values),
                                                   traffic_xml_get_attr("road_ref", el->names, el->values),
                                                   traffic_xml_get_attr("tmc_table", el->names, el->values),
                                                   tmc_direction ? atoi(tmc_direction) : 0);
            state->from = NULL;
            state->to = NULL;
            state->at = NULL;
            state->via = NULL;
            state->not_via = NULL;
            state->location->priv->txt_data = state->location_txt_data;
            state->location_txt_data = NULL;
        } else if (!g_ascii_strcasecmp((char *) tag_name, "event")) {
            count = g_list_length(state->si);
            if (count) {
                children = (void **) g_new0(struct traffic_suppl_info *, count);
                iter = state->si;
                for (i = 0; iter && (i < count); i++) {
                    children[i] = iter->data;
                    iter = g_list_next(iter);
                }
            }
            length = traffic_xml_get_attr("length", el->names, el->values);
            speed = traffic_xml_get_attr("speed", el->names, el->values);
            event = traffic_event_new(event_class_new(traffic_xml_get_attr("class", el->names, el->values)),
                                      event_type_new(traffic_xml_get_attr("type", el->names, el->values)),
                                      length ? atoi(length) : -1,
                                      speed ? atoi(speed) : INT_MAX,
                                      /* TODO quantifier */
                                      NULL,
                                      count,
                                      (struct traffic_suppl_info **) children);
            g_free(children);
            g_list_free(state->si);
            state->si = NULL;
            /* TODO preserve unknown (and thus invalid) events if they have maxspeed set */
            if (!traffic_event_is_valid(event)) {
                dbg(lvl_debug, "invalid or unknown event %s/%s detected, skipping",
                    traffic_xml_get_attr("class", el->names, el->values),
                    traffic_xml_get_attr("type", el->names, el->values));
                traffic_event_destroy(event);
            } else
                state->events = g_list_append(state->events, event);
        } else if (!g_ascii_strcasecmp((char *) tag_name, "from")) {
            point = &state->from;
        } else if (!g_ascii_strcasecmp((char *) tag_name, "to")) {
            point = &state->to;
        } else if (!g_ascii_strcasecmp((char *) tag_name, "at")) {
            point = &state->at;
        } else if (!g_ascii_strcasecmp((char *) tag_name, "via")) {
            point = &state->via;
        } else if (!g_ascii_strcasecmp((char *) tag_name, "not_via")) {
            point = &state->not_via;
        } else if (!g_ascii_strcasecmp((char *) tag_name, "navit_items")) {
            state->location_txt_data = g_strdup(el->text);
        }

        /*
         * No handling necessary for:
         *
         * navit_messages: No attributes, everything handled in children's callbacks
         * feed: No attributes, everything handled in children's callbacks
         * events: No attributes, everything handled in children's callbacks
         * merge: No attributes, everything handled in children's callbacks
         * replaces: Leaf node, handled in start callback
         * supplementary_info: Leaf node, handled in start callback
         */

        if (point) {
            /* we have a location point (from, at, to, via or not_via) to process */
            if (sscanf(el->text, "%f %f", &lat, &lon) == 2) {
                *point = traffic_point_new(lon, lat,
                                           traffic_xml_get_attr("junction_name", el->names, el->values),
                                           traffic_xml_get_attr("junction_ref", el->names, el->values),
                                           traffic_xml_get_attr("tmc_id", el->names, el->values));
            } else {
                dbg(lvl_error, "%s has no valid lat/lon pair, skipping", tag_name);
            }
        }
    }

    if (el && !g_ascii_strcasecmp(tag_name, el->tag_name)) {
        traffic_xml_element_destroy(el);
        state->tagstack = g_list_remove(state->tagstack, state->tagstack->data);
    }
    if (!state->is_valid)
        state->is_valid = traffic_xml_is_tagstack_valid(state);
    state->is_opened = 0;
}

/**
 * @brief Callback function which gets called when character data is encountered.
 *
 * @param text The character data (note that the data is not NULL-terminated!)
 * @param len The number of characters in `text`
 * @param data Points to a `struct xml_state` holding parser state
 */
static void traffic_xml_text(xml_context *dummy, const char *text, gsize len, void *data, GError **error) {
    struct xml_state * state = (struct xml_state *) data;
    char * text_sz = g_strndup(text, len);
    struct xml_element * el = state->tagstack ? (struct xml_element *) state->tagstack->data : NULL;

    dbg(lvl_debug, " TEXT: '%s'", text_sz);
    if (state->is_valid && state->is_opened) {
        /* this will work only for leaf nodes, which is not an issue at the moment as the only nodes
         * with actual text data are leaf nodes */
        el->text = g_strndup(text, len);
    }
    g_free(text_sz);
}

enum event_class event_class_new(char * string) {
    if (string) {
        if (!g_ascii_strcasecmp(string, "CONGESTION"))
            return event_class_congestion;
        if (!g_ascii_strcasecmp(string, "DELAY"))
            return event_class_delay;
        if (!g_ascii_strcasecmp(string, "RESTRICTION"))
            return event_class_restriction;
    }
    return event_class_invalid;
}

const char * event_class_to_string(enum event_class this_) {
    switch (this_) {
    case event_class_congestion:
        return "CONGESTION";
    case event_class_delay:
        return "DELAY";
    case event_class_restriction:
        return "RESTRICTION";
    default:
        return "INVALID";
    }
}

enum event_type event_type_new(char * string) {
    if (string) {
        if (!g_ascii_strcasecmp(string, "CONGESTION_CLEARED"))
            return event_congestion_cleared;
        if (!g_ascii_strcasecmp(string, "CONGESTION_FORECAST_WITHDRAWN"))
            return event_congestion_forecast_withdrawn;
        if (!g_ascii_strcasecmp(string, "CONGESTION_HEAVY_TRAFFIC"))
            return event_congestion_heavy_traffic;
        if (!g_ascii_strcasecmp(string, "CONGESTION_LONG_QUEUE"))
            return event_congestion_long_queue;
        if (!g_ascii_strcasecmp(string, "CONGESTION_NONE"))
            return event_congestion_none;
        if (!g_ascii_strcasecmp(string, "CONGESTION_NORMAL_TRAFFIC"))
            return event_congestion_normal_traffic;
        if (!g_ascii_strcasecmp(string, "CONGESTION_QUEUE"))
            return event_congestion_queue;
        if (!g_ascii_strcasecmp(string, "CONGESTION_QUEKE_LIKELY"))
            return event_congestion_queue_likely;
        if (!g_ascii_strcasecmp(string, "CONGESTION_SLOW_TRAFFIC"))
            return event_congestion_slow_traffic;
        if (!g_ascii_strcasecmp(string, "CONGESTION_STATIONARY_TRAFFIC"))
            return event_congestion_stationary_traffic;
        if (!g_ascii_strcasecmp(string, "CONGESTION_STATIONARY_TRAFFIC_LIKELY"))
            return event_congestion_stationary_traffic_likely;
        if (!g_ascii_strcasecmp(string, "CONGESTION_TRAFFIC_BUILDING_UP"))
            return event_congestion_traffic_building_up;
        if (!g_ascii_strcasecmp(string, "CONGESTION_TRAFFIC_CONGESTION"))
            return event_congestion_traffic_congestion;
        if (!g_ascii_strcasecmp(string, "CONGESTION_TRAFFIC_EASING"))
            return event_congestion_traffic_easing;
        if (!g_ascii_strcasecmp(string, "CONGESTION_TRAFFIC_FLOWING_FREELY"))
            return event_congestion_traffic_flowing_freely;
        if (!g_ascii_strcasecmp(string, "CONGESTION_TRAFFIC_HEAVIER_THAN_NORMAL"))
            return event_congestion_traffic_heavier_than_normal;
        if (!g_ascii_strcasecmp(string, "CONGESTION_TRAFFIC_LIGHTER_THAN_NORMAL"))
            return event_congestion_traffic_lighter_than_normal;
        if (!g_ascii_strcasecmp(string, "CONGESTION_TRAFFIC_MUCH_HEAVIER_THAN_NORMAL"))
            return event_congestion_traffic_much_heavier_than_normal;
        if (!g_ascii_strcasecmp(string, "CONGESTION_TRAFFIC_PROBLEM"))
            return event_congestion_traffic_problem;
        if (!g_ascii_strcasecmp(string, "DELAY_CLEARANCE"))
            return event_delay_clearance;
        if (!g_ascii_strcasecmp(string, "DELAY_DELAY"))
            return event_delay_delay;
        if (!g_ascii_strcasecmp(string, "DELAY_DELAY_POSSIBLE"))
            return event_delay_delay_possible;
        if (!g_ascii_strcasecmp(string, "DELAY_FORECAST_WITHDRAWN"))
            return event_delay_forecast_withdrawn;
        if (!g_ascii_strcasecmp(string, "DELAY_LONG_DELAY"))
            return event_delay_long_delay;
        if (!g_ascii_strcasecmp(string, "DELAY_SEVERAL_HOURS"))
            return event_delay_several_hours;
        if (!g_ascii_strcasecmp(string, "DELAY_UNCERTAIN_DURATION"))
            return event_delay_uncertain_duration;
        if (!g_ascii_strcasecmp(string, "DELAY_VERY_LONG_DELAY"))
            return event_delay_very_long_delay;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_ACCESS_RESTRICTIONS_LIFTED"))
            return event_restriction_access_restrictions_lifted;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_ALL_CARRIAGEWAYS_CLEARED"))
            return event_restriction_all_carriageways_cleared;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_ALL_CARRIAGEWAYS_REOPENED"))
            return event_restriction_all_carriageways_reopened;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_BATCH_SERVICE"))
            return event_restriction_batch_service;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_BLOCKED"))
            return event_restriction_blocked;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_BLOCKED_AHEAD"))
            return event_restriction_blocked_ahead;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_CARRIAGEWAY_BLOCKED"))
            return event_restriction_carriageway_blocked;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_CARRIAGEWAY_CLOSED"))
            return event_restriction_carriageway_closed;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_CONTRAFLOW"))
            return event_restriction_contraflow;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_CLOSED"))
            return event_restriction_closed;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_CLOSED_AHEAD"))
            return event_restriction_closed_ahead;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_ENTRY_BLOCKED"))
            return event_restriction_entry_blocked;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_ENTRY_REOPENED"))
            return event_restriction_entry_reopened;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_INTERMITTENT_CLOSURES"))
            return event_restriction_intermittent_closures;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_LANE_BLOCKED"))
            return event_restriction_lane_blocked;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_LANE_CLOSED"))
            return event_restriction_lane_closed;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_OPEN"))
            return event_restriction_open;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_RAMP_BLOCKED"))
            return event_restriction_ramp_blocked;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_RAMP_CLOSED"))
            return event_restriction_ramp_closed;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_RAMP_REOPENED"))
            return event_restriction_ramp_reopened;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_REDUCED_LANES"))
            return event_restriction_reduced_lanes;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_REOPENED"))
            return event_restriction_reopened;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_ROAD_CLEARED"))
            return event_restriction_road_cleared;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_SINGLE_ALTERNATE_LINE_TRAFFIC"))
            return event_restriction_single_alternate_line_traffic;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_SPEED_LIMIT"))
            return event_restriction_speed_limit;
        if (!g_ascii_strcasecmp(string, "RESTRICTION_SPEED_LIMIT_LIFTED"))
            return event_restriction_speed_limit_lifted;
    }
    return event_invalid;
}

const char * event_type_to_string(enum event_type this_) {
    switch (this_) {
    case event_congestion_cleared:
        return "CONGESTION_CLEARED";
    case event_congestion_forecast_withdrawn:
        return "CONGESTION_FORECAST_WITHDRAWN";
    case event_congestion_heavy_traffic:
        return "CONGESTION_HEAVY_TRAFFIC";
    case event_congestion_long_queue:
        return "CONGESTION_LONG_QUEUE";
    case event_congestion_none:
        return "CONGESTION_NONE";
    case event_congestion_normal_traffic:
        return "CONGESTION_NORMAL_TRAFFIC";
    case event_congestion_queue:
        return "CONGESTION_QUEUE";
    case event_congestion_queue_likely:
        return "CONGESTION_QUEUE_LIKELY";
    case event_congestion_slow_traffic:
        return "CONGESTION_SLOW_TRAFFIC";
    case event_congestion_stationary_traffic:
        return "CONGESTION_STATIONARY_TRAFFIC";
    case event_congestion_stationary_traffic_likely:
        return "CONGESTION_STATIONARY_TRAFFIC_LIKELY";
    case event_congestion_traffic_building_up:
        return "CONGESTION_TRAFFIC_BUILDING_UP";
    case event_congestion_traffic_congestion:
        return "CONGESTION_TRAFFIC_CONGESTION";
    case event_congestion_traffic_easing:
        return "CONGESTION_TRAFFIC_EASING";
    case event_congestion_traffic_flowing_freely:
        return "CONGESTION_TRAFFIC_FLOWING_FREELY";
    case event_congestion_traffic_heavier_than_normal:
        return "CONGESTION_TRAFFIC_HEAVIER_THAN_NORMAL";
    case event_congestion_traffic_lighter_than_normal:
        return "CONGESTION_TRAFFIC_LIGHTER_THAN_NORMAL";
    case event_congestion_traffic_much_heavier_than_normal:
        return "CONGESTION_TRAFFIC_MUCH_HEAVIER_THAN_NORMAL";
    case event_congestion_traffic_problem:
        return "CONGESTION_TRAFFIC_PROBLEM";
    case event_delay_clearance:
        return "DELAY_CLEARANCE";
    case event_delay_delay:
        return "DELAY_DELAY";
    case event_delay_delay_possible:
        return "DELAY_DELAY_POSSIBLE";
    case event_delay_forecast_withdrawn:
        return "DELAY_FORECAST_WITHDRAWN";
    case event_delay_long_delay:
        return "DELAY_LONG_DELAY";
    case event_delay_several_hours:
        return "DELAY_SEVERAL_HOURS";
    case event_delay_uncertain_duration:
        return "DELAY_UNCERTAIN_DURATION";
    case event_delay_very_long_delay:
        return "DELAY_VERY_LONG_DELAY";
    case event_restriction_access_restrictions_lifted:
        return "RESTRICTION_ACCESS_RESTRICTIONS_LIFTED";
    case event_restriction_all_carriageways_cleared:
        return "RESTRICTION_ALL_CARRIAGEWAYS_CLEARED";
    case event_restriction_all_carriageways_reopened:
        return "RESTRICTION_ALL_CARRIAGEWAYS_REOPENED";
    case event_restriction_batch_service:
        return "RESTRICTION_BATCH_SERVICE";
    case event_restriction_blocked:
        return "RESTRICTION_BLOCKED";
    case event_restriction_blocked_ahead:
        return "RESTRICTION_BLOCKED_AHEAD";
    case event_restriction_carriageway_blocked:
        return "RESTRICTION_CARRIAGEWAY_BLOCKED";
    case event_restriction_carriageway_closed:
        return "RESTRICTION_CARRIAGEWAY_CLOSED";
    case event_restriction_closed:
        return "RESTRICTION_CLOSED";
    case event_restriction_closed_ahead:
        return "RESTRICTION_CLOSED_AHEAD";
    case event_restriction_contraflow:
        return "RESTRICTION_CONTRAFLOW";
    case event_restriction_entry_blocked:
        return "RESTRICTION_ENTRY_BLOCKED";
    case event_restriction_entry_reopened:
        return "RESTRICTION_ENTRY_REOPENED";
    case event_restriction_exit_blocked:
        return "RESTRICTION_EXIT_BLOCKED";
    case event_restriction_exit_reopened:
        return "RESTRICTION_EXIT_REOPENED";
    case event_restriction_intermittent_closures:
        return "RESTRICTION_INTERMITTENT_CLOSURES";
    case event_restriction_lane_blocked:
        return "RESTRICTION_LANE_BLOCKED";
    case event_restriction_lane_closed:
        return "RESTRICTION_LANE_CLOSED";
    case event_restriction_open:
        return "RESTRICTION_OPEN";
    case event_restriction_ramp_blocked:
        return "RESTRICTION_RAMP_BLOCKED";
    case event_restriction_ramp_closed:
        return "RESTRICTION_RAMP_CLOSED";
    case event_restriction_ramp_reopened:
        return "RESTRICTION_RAMP_REOPENED";
    case event_restriction_reduced_lanes:
        return "RESTRICTION_REDUCED_LANES";
    case event_restriction_reopened:
        return "RESTRICTION_REOPENED";
    case event_restriction_road_cleared:
        return "RESTRICTION_ROAD_CLEARED";
    case event_restriction_single_alternate_line_traffic:
        return "RESTRICTION_SINGLE_ALTERNATE_LINE_TRAFFIC";
    case event_restriction_speed_limit:
        return "RESTRICTION_SPEED_LIMIT";
    case event_restriction_speed_limit_lifted:
        return "RESTRICTION_SPEED_LIMIT_LIFTED";
    default:
        return "INVALID";
    }
}

enum item_type item_type_from_road_type(char * string, int is_urban) {
    enum item_type ret = type_line_unspecified;

    if (string) {
        if (!g_ascii_strcasecmp(string, "MOTORWAY"))
            return is_urban ? type_highway_city : type_highway_land;
        if (!g_ascii_strcasecmp(string, "TRUNK"))
            return type_street_n_lanes;
        if (!g_ascii_strcasecmp(string, "PRIMARY"))
            return is_urban ? type_street_4_city : type_street_4_land;
        if (!g_ascii_strcasecmp(string, "SECONDARY"))
            return is_urban ? type_street_3_city : type_street_3_land;
        if (!g_ascii_strcasecmp(string, "TERTIARY"))
            return is_urban ? type_street_2_city : type_street_2_land;

        ret = item_from_name(string);
    }
    if ((ret < route_item_first) || (ret > route_item_last))
        return type_line_unspecified;
    return ret;
}

enum location_dir location_dir_new(char * string) {
    if (string && !g_ascii_strcasecmp(string, "ONE_DIRECTION"))
        return location_dir_one;
    return location_dir_both;
}

enum location_fuzziness location_fuzziness_new(char * string) {
    if (string) {
        if (!g_ascii_strcasecmp(string, "LOW_RES"))
            return location_fuzziness_low_res;
        if (!g_ascii_strcasecmp(string, "END_UNKNOWN"))
            return location_fuzziness_end_unknown;
        if (!g_ascii_strcasecmp(string, "START_UNKNOWN"))
            return location_fuzziness_start_unknown;
        if (!g_ascii_strcasecmp(string, "EXTENT_UNKNOWN"))
            return location_fuzziness_extent_unknown;
    }
    return location_fuzziness_none;
}

const char * location_fuzziness_to_string(enum location_fuzziness this_) {
    switch (this_) {
    case location_fuzziness_low_res:
        return "LOW_RES";
    case location_fuzziness_end_unknown:
        return "END_UNKNOWN";
    case location_fuzziness_start_unknown:
        return "START_UNKNOWN";
    case location_fuzziness_extent_unknown:
        return "EXTENT_UNKNOWN";
    default:
        return NULL;
    }
}

enum location_ramps location_ramps_new(char * string) {
    if (string) {
        if (!g_ascii_strcasecmp(string, "ALL_RAMPS"))
            return location_ramps_all;
        if (!g_ascii_strcasecmp(string, "ENTRY_RAMP"))
            return location_ramps_entry;
        if (!g_ascii_strcasecmp(string, "EXIT_RAMP"))
            return location_ramps_exit;
    }
    return location_ramps_none;
}

const char * location_ramps_to_string(enum location_ramps this_) {
    switch (this_) {
    case location_ramps_none:
        return "NONE";
    case location_ramps_all:
        return "ALL_RAMPS";
    case location_ramps_entry:
        return "ENTRY_RAMP";
    case location_ramps_exit:
        return "EXIT_RAMP";
    default:
        return NULL;
    }
}

enum si_class si_class_new(char * string) {
    if (string) {
        if (!g_ascii_strcasecmp(string, "PLACE"))
            return si_class_place;
        if (!g_ascii_strcasecmp(string, "TENDENCY"))
            return si_class_tendency;
        if (!g_ascii_strcasecmp(string, "VEHICLE"))
            return si_class_vehicle;
    }
    return si_class_invalid;
}

const char * si_class_to_string(enum si_class this_) {
    switch (this_) {
    case si_class_place:
        return "PLACE";
    case si_class_tendency:
        return "TENDENCY";
    case si_class_vehicle:
        return "VEHICLE";
    default:
        return "INVALID";
    }
}

enum si_type si_type_new(char * string) {
    if (string) {
        if (!g_ascii_strcasecmp(string, "S_PLACE_BRIDGE"))
            return si_place_bridge;
        if (!g_ascii_strcasecmp(string, "S_PLACE_RAMP"))
            return si_place_ramp;
        if (!g_ascii_strcasecmp(string, "S_PLACE_ROADWORKS"))
            return si_place_roadworks;
        if (!g_ascii_strcasecmp(string, "S_PLACE_TUNNEL"))
            return si_place_tunnel;
        if (!g_ascii_strcasecmp(string, "S_TENDENCY_QUEUE_DECREASING"))
            return si_tendency_queue_decreasing;
        if (!g_ascii_strcasecmp(string, "S_TENDENCY_QUEUE_INCREASING"))
            return si_tendency_queue_increasing;
        if (!g_ascii_strcasecmp(string, "S_VEHICLE_ALL"))
            return si_vehicle_all;
        if (!g_ascii_strcasecmp(string, "S_VEHICLE_BUS"))
            return si_vehicle_bus;
        if (!g_ascii_strcasecmp(string, "S_VEHICLE_CAR"))
            return si_vehicle_car;
        if (!g_ascii_strcasecmp(string, "S_VEHICLE_CAR_WITH_CARAVAN"))
            return si_vehicle_car_with_caravan;
        if (!g_ascii_strcasecmp(string, "S_VEHICLE_CAR_WITH_TRAILER"))
            return si_vehicle_car_with_trailer;
        if (!g_ascii_strcasecmp(string, "S_VEHICLE_HAZMAT"))
            return si_vehicle_hazmat;
        if (!g_ascii_strcasecmp(string, "S_VEHICLE_HGV"))
            return si_vehicle_hgv;
        if (!g_ascii_strcasecmp(string, "S_VEHICLE_MOTOR"))
            return si_vehicle_motor;
        if (!g_ascii_strcasecmp(string, "S_VEHICLE_WITH_TRAILER"))
            return si_vehicle_with_trailer;
    }
    return si_invalid;
}

const char * si_type_to_string(enum si_type this_) {
    switch (this_) {
    case si_place_bridge:
        return "S_PLACE_BRIDGE";
    case si_place_ramp:
        return "S_PLACE_RAMP";
    case si_place_roadworks:
        return "S_PLACE_ROADWORKS";
    case si_place_tunnel:
        return "S_PLACE_TUNNEL";
    case si_tendency_queue_decreasing:
        return "S_TENDENCY_QUEUE_DECREASING";
    case si_tendency_queue_increasing:
        return "S_TENDENCY_QUEUE_INCREASING";
    case si_vehicle_all:
        return "S_VEHICLE_ALL";
    case si_vehicle_bus:
        return "S_VEHICLE_BUS";
    case si_vehicle_car:
        return "S_VEHICLE_CAR";
    case si_vehicle_car_with_caravan:
        return "S_VEHICLE_CAR_WITH_CARAVAN";
    case si_vehicle_car_with_trailer:
        return "S_VEHICLE_CAR_WITH_TRAILER";
    case si_vehicle_hazmat:
        return "S_VEHICLE_HAZMAT";
    case si_vehicle_hgv:
        return "S_VEHICLE_HGV";
    case si_vehicle_motor:
        return "S_VEHICLE_MOTOR";
    case si_vehicle_with_trailer:
        return "S_VEHICLE_WITH_TRAILER";
    default:
        return "INVALID";
    }
}

struct traffic_point * traffic_point_new(float lon, float lat, char * junction_name, char * junction_ref,
        char * tmc_id) {
    struct traffic_point * ret;

    ret = g_new0(struct traffic_point, 1);
    ret->coord.lat = lat;
    ret->coord.lng = lon;
    ret->junction_name = junction_name ? g_strdup(junction_name) : NULL;
    ret->junction_ref = junction_ref ? g_strdup(junction_ref) : NULL;
    ret->tmc_id = tmc_id ? g_strdup(tmc_id) : NULL;
    return ret;
}

struct traffic_point * traffic_point_new_short(float lon, float lat) {
    return traffic_point_new(lon, lat, NULL, NULL, NULL);
}

void traffic_point_destroy(struct traffic_point * this_) {
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
        struct traffic_point * to, struct traffic_point * via, struct traffic_point * not_via,
        char * destination, char * direction, enum location_dir directionality,
        enum location_fuzziness fuzziness, enum location_ramps ramps, enum item_type road_type,
        char * road_name, char * road_ref, char * tmc_table, int tmc_direction) {
    struct traffic_location * ret;

    ret = g_new0(struct traffic_location, 1);
    ret->at = at;
    ret->from = from;
    ret->to = to;
    ret->via = via;
    ret->not_via = not_via;
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
    ret->priv = g_new0(struct traffic_location_priv, 1);
    ret->priv->sw = NULL;
    ret->priv->ne = NULL;
    return ret;
}

struct traffic_location * traffic_location_new_short(struct traffic_point * at, struct traffic_point * from,
        struct traffic_point * to, struct traffic_point * via, struct traffic_point * not_via,
        enum location_dir directionality, enum location_fuzziness fuzziness) {
    return traffic_location_new(at, from, to, via, not_via, NULL, NULL, directionality, fuzziness,
                                location_ramps_none, type_line_unspecified, NULL, NULL, NULL, 0);
}

void traffic_location_destroy(struct traffic_location * this_) {
    if (this_->at)
        traffic_point_destroy(this_->at);
    if (this_->from)
        traffic_point_destroy(this_->from);
    if (this_->to)
        traffic_point_destroy(this_->to);
    if (this_->via)
        traffic_point_destroy(this_->via);
    if (this_->not_via)
        traffic_point_destroy(this_->not_via);
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
    if (this_->priv->txt_data)
        g_free(this_->priv->txt_data);
    if (this_->priv->sw)
        g_free(this_->priv->sw);
    if (this_->priv->ne)
        g_free(this_->priv->ne);
    g_free(this_->priv);
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
    if (this_->si && this_->si_count) {
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
    if (event_count && events) {
        ret->event_count = event_count;
        ret->events = g_memdup(events, sizeof(struct traffic_event *) * event_count);
    }
    ret->priv = g_new0(struct traffic_message_priv, 1);
    ret->priv->items = NULL;
    return ret;
}

struct traffic_message * traffic_message_new_short(char * id, time_t receive_time, time_t update_time,
        time_t expiration_time, int is_forecast, struct traffic_location * location,
        int event_count, struct traffic_event ** events) {
    return traffic_message_new(id, receive_time, update_time, expiration_time, 0, 0, 0,
                               is_forecast, 0, NULL, location, event_count, events);
}

struct traffic_message * traffic_message_new_single_event(char * id, time_t receive_time, time_t update_time,
        time_t expiration_time, int is_forecast, struct traffic_location * location,
        enum event_class event_class, enum event_type type) {
    struct traffic_event * event;
    struct traffic_event ** events;

    event = traffic_event_new_short(event_class, type);
    events = g_new0(struct traffic_event *, 1);
    events[0] = event;
    return traffic_message_new_short(id, receive_time, update_time, expiration_time, is_forecast,
                                     location, 1, events);
    g_free(events);
}

struct traffic_message * traffic_message_new_cancellation(char * id, time_t receive_time, time_t update_time,
        time_t expiration_time, struct traffic_location * location) {
    return traffic_message_new(id, receive_time, update_time, expiration_time, 0, 0, 1,
                               0, 0, NULL, location, 0, NULL);
}

void traffic_message_destroy(struct traffic_message * this_) {
    int i;
    struct item ** items;

    g_free(this_->id);
    if (this_->replaces) {
        for (i = 0; i < this_->replaced_count; i++)
            g_free(this_->replaces[i]);
        g_free(this_->replaces);
    }
    if (this_->location)
        traffic_location_destroy(this_->location);
    if (this_->events && this_->event_count) {
        for (i = 0; i < this_->event_count; i++)
            traffic_event_destroy(this_->events[i]);
        g_free(this_->events);
    }
    if (this_->priv->items) {
        for (items = this_->priv->items; *items; items++)
            *items = tm_item_unref(*items);
        g_free(this_->priv->items);
    }
    g_free(this_->priv);
    g_free(this_);
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

struct item ** traffic_message_get_items(struct traffic_message * this_) {
    struct item ** ret;
    struct item ** in;
    int i;
    if (!this_->priv->items) {
        ret = g_new0(struct item *, 1);
        return ret;
    }
    in = this_->priv->items;
    for (i = 1; *in; i++)
        in++;
    ret = g_new0(struct item *, i);
    memcpy(ret, this_->priv->items, sizeof(struct item *) * i);
    return ret;
}

/**
 * @brief Registers a new traffic map plugin
 *
 * @param meth Receives the map methods
 * @param attrs The attributes for the map
 * @param cbl
 *
 * @return A pointer to a `map_priv` structure for the map
 */
static struct map_priv * traffic_map_new(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl) {
    struct map_priv *ret;
    struct attr *traffic_attr;

    traffic_attr = attr_search(attrs, attr_traffic);
    if (!traffic_attr) {
        dbg(lvl_error, "attr_traffic not found!");
        return NULL;
    }

    ret = g_new0(struct map_priv, 1);
    *meth = traffic_map_meth;
    ret->shared = traffic_attr->u.traffic->shared;

    return ret;
}

void traffic_init(void) {
    dbg(lvl_debug, "enter");
    plugin_register_category_map("traffic", traffic_map_new);
}

struct map * traffic_get_map(struct traffic *this_) {
    char * filename;
    struct traffic_message ** messages;
    struct traffic_message ** cur_msg;

    if (!this_->shared->map) {
        /* no map yet, create a new one */
        struct attr *attrs[5];
        struct attr a_type,data,a_description,a_traffic;
        a_type.type = attr_type;
        a_type.u.str = "traffic";
        data.type = attr_data;
        data.u.str = "";
        a_description.type = attr_description;
        a_description.u.str = "Traffic";
        a_traffic.type = attr_traffic;
        a_traffic.u.traffic = this_;

        attrs[0] = &a_type;
        attrs[1] = &data;
        attrs[2] = &a_description;
        attrs[3] = &a_traffic;
        attrs[4] = NULL;

        this_->shared->map = map_new(NULL, attrs);

        /* populate map with previously stored messages */
        filename = g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/traffic.xml", NULL);
        messages = traffic_get_messages_from_xml_file(this_, filename);
        g_free(filename);

        if (messages) {
            for (cur_msg = messages; *cur_msg; cur_msg++)
                this_->shared->message_queue = g_list_append(this_->shared->message_queue, *cur_msg);
            g_free(messages);
            if (this_->shared->message_queue) {
                if (!this_->idle_cb)
                    this_->idle_cb = callback_new_2(callback_cast(traffic_process_messages_int),
                                                    this_, PROCESS_MESSAGES_NO_DUMP_STORE);
                if (!this_->idle_ev)
                    this_->idle_ev = event_add_idle(50, this_->idle_cb);
            }
        }
    }

    return this_->shared->map;
}

/**
 * @brief Reads previously stored traffic messages from parsed XML data.
 *
 * @param state The XML parser state after parsing the XML data
 *
 * @return A `NULL`-terminated pointer array. Each element points to one `struct traffic_message`.
 * `NULL` is returned (rather than an empty pointer array) if there are no messages to report.
 */
static struct traffic_message ** traffic_get_messages_from_parsed_xml(struct xml_state * state) {
    struct traffic_message ** ret = NULL;
    int i, count;
    GList * msg_iter;

    count = g_list_length(state->messages);
    if (count)
        ret = g_new0(struct traffic_message *, count + 1);
    msg_iter = state->messages;
    for (i = 0; i < count; i++) {
        ret[i] = (struct traffic_message *) msg_iter->data;
        msg_iter = g_list_next(msg_iter);
    }
    g_list_free(state->messages);
    return ret;
}

struct traffic_message ** traffic_get_messages_from_xml_file(struct traffic * this_, char * filename) {
    struct traffic_message ** ret = NULL;
    struct xml_state state;
    int read_success = 0;

    if (filename) {
        memset(&state, 0, sizeof(struct xml_state));
        read_success = xml_parse_file(filename, &state, traffic_xml_start, traffic_xml_end, traffic_xml_text);
        if (read_success) {
            ret = traffic_get_messages_from_parsed_xml(&state);
        } else {
            dbg(lvl_error,"could not retrieve stored traffic messages");
        }
    } /* if (traffic_filename) */
    return ret;
}

struct traffic_message ** traffic_get_messages_from_xml_string(struct traffic * this_, char * xml) {
    struct traffic_message ** ret = NULL;
    struct xml_state state;
    int read_success = 0;

    if (xml) {
        memset(&state, 0, sizeof(struct xml_state));
        read_success = xml_parse_text(xml, &state, traffic_xml_start, traffic_xml_end, traffic_xml_text);
        if (read_success) {
            ret = traffic_get_messages_from_parsed_xml(&state);
        } else {
            dbg(lvl_error,"no data supplied");
        }
    } /* if (xml) */
    return ret;
}

struct traffic_message ** traffic_get_stored_messages(struct traffic *this_) {
    struct traffic_message ** ret = g_new0(struct traffic_message *, g_list_length(this_->shared->messages) + 1);
    struct traffic_message ** out = ret;
    GList * in = this_->shared->messages;

    /* Iterator over active messages */
    GList * msgiter;

    /* Current message */
    struct traffic_message * message;

    /* Attributes for traffic distortions generated from the current traffic message */
    struct seg_data * data;

    /* Ensure all locations are fully resolved */
    for (msgiter = this_->shared->messages; msgiter; msgiter = g_list_next(msgiter)) {
        message = (struct traffic_message *) msgiter->data;
        if (message->priv->items == NULL) {
            data = traffic_message_parse_events(message);
            traffic_message_add_segments(message, this_->shared->ms, data, this_->shared->map, this_->shared->rt);
            g_free(data);
        }
    }
    while (in) {
        *out = (struct traffic_message *) in->data;
        in = g_list_next(in);
        out++;
    }

    return ret;
}

void traffic_process_messages(struct traffic * this_, struct traffic_message ** messages) {
    struct traffic_message ** cur_msg;

    for (cur_msg = messages; cur_msg && *cur_msg; cur_msg++)
        this_->shared->message_queue = g_list_append(this_->shared->message_queue, *cur_msg);
    if (this_->shared->message_queue) {
        if (this_->idle_ev)
            event_remove_idle(this_->idle_ev);
        if (this_->idle_cb)
            callback_destroy(this_->idle_cb);
        this_->idle_cb = callback_new_2(callback_cast(traffic_process_messages_int), this_, 0);
        this_->idle_ev = event_add_idle(50, this_->idle_cb);
    }
}

void traffic_set_mapset(struct traffic *this_, struct mapset *ms) {
    this_->shared->ms = ms;
}

void traffic_set_route(struct traffic *this_, struct route *rt) {
    this_->shared->rt = rt;
}

void traffic_destroy(struct traffic *this_) {
    if (this_->meth.destroy)
        this_->meth.destroy(this_->priv);
    attr_list_free(this_->attrs);
    g_free(this_);
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
    (object_func_destroy)traffic_destroy,
    (object_func_dup)NULL,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};
