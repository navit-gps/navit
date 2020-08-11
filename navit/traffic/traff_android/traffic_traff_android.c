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

/**
 * @file traffic_traff_android.c
 *
 * @brief The TraFF plugin for Android
 *
 * This plugin receives TraFF feeds via Android broadcasts and content providers.
 */

#include <string.h>
#include <time.h>

#ifdef _POSIX_C_SOURCE
#include <sys/types.h>
#endif
#include "glib_slice.h"
#include "config.h"
#include "item.h"
#include "attr.h"
#include "coord.h"
#include "map.h"
#include "route_protected.h"
#include "route.h"
#include "transform.h"
#include "xmlconfig.h"
#include "android.h"
#include "traffic.h"
#include "plugin.h"
#include "callback.h"
#include "vehicle.h"
#include "debug.h"
#include "navit.h"
#include "util.h"

/**
 * @brief Minimum area around the current position for which to retrieve traffic updates.
 *
 * 100000 is equivalent to around 50 km on each side of the current position. The actual subscription area
 * can be larger, allowing for a subscription area to be kept over multiple position updates.
 *
 * The actual subscription area around the current location is stored in
 * {@link struct traffic_priv::position_rect} and updated in
 * {@link traffic_traff_android_position_callback(struct traffic_priv *, struct navit *, struct vehicle *)}.
 */
#define POSITION_RECT_SIZE 100000

/**
 * @brief Stores information about the plugin instance.
 */
struct traffic_priv {
    struct navit * nav;         /**< The navit instance */
    struct callback * cbid;     /**< The callback function for TraFF feeds **/
    int position_valid;         /**< Whether Navit currently has a valid position */
    struct coord_rect * position_rect; /**< Rectangle around last known vehicle position (in `projection_mg`) */
    struct map_selection * route_map_sel; /**< Map selection for the current route */
    jclass NavitTraffClass;     /**< The `NavitTraff` class */
    jobject NavitTraff;         /**< An instance of `NavitTraff` */
};

void traffic_traff_android_destroy(struct traffic_priv * this_);
struct traffic_message ** traffic_traff_android_get_messages(struct traffic_priv * this_);

/**
 * @brief Destructor.
 */
void traffic_traff_android_destroy(struct traffic_priv * this_) {
    jmethodID cid;

    cid = (*jnienv)->GetMethodID(jnienv, this_->NavitTraffClass, "close", "()V");
    if (cid == NULL) {
        dbg(lvl_error,"no method found");
        return; /* exception thrown */
    }
    (*jnienv)->CallVoidMethod(jnienv, this_->NavitTraff, cid);

    if (this_->position_rect)
        g_free(this_->position_rect);
    this_->position_rect = NULL;
    if (this_->route_map_sel)
        route_free_selection(this_->route_map_sel);
    this_->route_map_sel = NULL;
}

/**
 * @brief Returns an empty traffic report.
 *
 * @return Always `NULL`
 */
struct traffic_message ** traffic_traff_android_get_messages(struct traffic_priv * this_) {
    return NULL;
}

/**
 * @brief The methods implemented by this plugin
 */
static struct traffic_methods traffic_traff_android_meth = {
    traffic_traff_android_get_messages,
    traffic_traff_android_destroy,
};


/**
 * @brief Called when a new TraFF feed is received.
 *
 * @param this_ Private data for the module instance
 * @param feed Feed data in string form
 */
static void traffic_traff_android_on_feed_received(struct traffic_priv * this_, char * feed) {
    struct attr * attr;
    struct attr_iter * a_iter;
    struct traffic * traffic = NULL;
    struct traffic_message ** messages;

    dbg(lvl_debug, "enter");
    attr = g_new0(struct attr, 1);
    a_iter = navit_attr_iter_new(NULL);
    if (navit_get_attr(this_->nav, attr_traffic, attr, a_iter))
        traffic = (struct traffic *) attr->u.navit_object;
    navit_attr_iter_destroy(a_iter);
    g_free(attr);

    if (!traffic) {
        dbg(lvl_error, "failed to obtain traffic instance");
        return;
    }

    dbg(lvl_debug, "processing traffic feed:\n%s", feed);
    messages = traffic_get_messages_from_xml_string(traffic, feed);
    if (messages) {
        dbg(lvl_debug, "got messages from feed, processing");
        traffic_process_messages(traffic, messages);
        g_free(messages);
    }
}


/**
 * @brief Sets the route map selection
 *
 * @param this_ The instance which will handle the selection update
 */
static void traffic_traff_android_set_selection(struct traffic_priv * this_) {
    struct route * route;
    struct coord_geo lu, rl;
    gchar *filter_list;
    jstring j_filter_list;
    gchar *min_road_class;
    jmethodID cid;

    if (this_->route_map_sel)
        route_free_selection(this_->route_map_sel);
    this_->route_map_sel = NULL;
    if (navit_get_destination_count(this_->nav) && (route = (navit_get_route(this_->nav))))
        this_->route_map_sel = route_get_selection(route);

    /* start building the filter list */
    filter_list = g_strconcat_printf(NULL, "<filter_list>\n");
    if (this_->position_rect) {
        transform_to_geo(projection_mg, &this_->position_rect->lu, &lu);
        transform_to_geo(projection_mg, &this_->position_rect->rl, &rl);
        filter_list = g_strconcat_printf(filter_list, "    <filter bbox=\"%.5f %.5f %.5f %.5f\"/>\n",
                rl.lat, lu.lng, lu.lat, rl.lng);
    }
    for (struct map_selection * sel = this_->route_map_sel; sel; sel = sel->next) {
        transform_to_geo(projection_mg, &sel->u.c_rect.lu, &lu);
        transform_to_geo(projection_mg, &sel->u.c_rect.rl, &rl);
        min_road_class = order_to_min_road_class(sel->order);
        if (!min_road_class)
            filter_list = g_strconcat_printf(filter_list, "    <filter bbox=\"%.5f %.5f %.5f %.5f\"/>\n",
                    rl.lat, lu.lng, lu.lat, rl.lng);
        else
            filter_list = g_strconcat_printf(filter_list, "    <filter min_road_class=\"%s\" bbox=\"%.5f %.5f %.5f %.5f\"/>\n",
                    min_road_class, rl.lat, lu.lng, lu.lat, rl.lng);
    }
    /* the trailing \0 is required for NewStringUTF */
    filter_list = g_strconcat_printf(filter_list, "</filter_list>\0");
    j_filter_list = (*jnienv)->NewStringUTF(jnienv, filter_list);
    cid = (*jnienv)->GetMethodID(jnienv, this_->NavitTraffClass, "onFilterUpdate", "(Ljava/lang/String;)V");
    if (cid)
        (*jnienv)->CallVoidMethod(jnienv, this_->NavitTraff, cid, j_filter_list);
    g_free(filter_list);
}


/**
 * @brief Callback for destination changes
 *
 * @param this_ The instance which will handle the destination update
 */
static void traffic_traff_android_destination_callback(struct traffic_priv * this_) {
    traffic_traff_android_set_selection(this_);
}


/**
 * @brief Callback for navigation status changes
 *
 * This callback is necessary to force an update of existing subscriptions when Navit acquires a new
 * position (after not having had valid position information), as the map selection will change when
 * the current position becomes known for the first time.
 *
 * @param this_ The instance which will handle the navigation status update
 * @param status The status of the navigation engine (the value of the {@code nav_status} attribute)
 */
static void traffic_traff_android_status_callback(struct traffic_priv * this_, int status) {
    int new_position_valid = (status != 1);
    if (new_position_valid && !this_->position_valid) {
        this_->position_valid = new_position_valid;
        traffic_traff_android_set_selection(this_);
    } else if (new_position_valid != this_->position_valid)
        this_->position_valid = new_position_valid;
}


/**
 * @brief Callback for position changes
 *
 * This updates {@link struct traffic_priv::position_rect} if the vehicle has moved far enough from its
 * center to be within {@link POSITION_RECT_SIZE} of one of its boundaries. The new rectangle is created
 * with twice that amount of padding, allowing the vehicle to move for at least that distance before the
 * subscription needs to be updated again.
 *
 * @param this_ The instance which will handle the position update
 * @param navit The Navit instance
 * @param vehicle The vehicle which delivered the position update and from which the position can be queried
 */
static void traffic_traff_android_position_callback(struct traffic_priv * this_, struct navit *navit, struct vehicle *vehicle) {
    struct attr attr;
    struct coord c;
    struct coord_rect cr;
    jmethodID cid;
    if (!vehicle_get_attr(vehicle, attr_position_coord_geo, &attr, NULL))
        return;
    transform_from_geo(projection_mg, attr.u.coord_geo, &c);
    cr.lu = c;
    cr.rl = c;
    cr.lu.x -= POSITION_RECT_SIZE;
    cr.rl.x += POSITION_RECT_SIZE;
    cr.lu.y += POSITION_RECT_SIZE;
    cr.rl.y -= POSITION_RECT_SIZE;
    if (!this_->position_rect)
        this_->position_rect = g_new0(struct coord_rect, 1);
    if (!coord_rect_contains(this_->position_rect, &cr.lu) || !coord_rect_contains(this_->position_rect, &cr.rl)) {
        cr.lu.x -= POSITION_RECT_SIZE;
        cr.rl.x += POSITION_RECT_SIZE;
        cr.lu.y += POSITION_RECT_SIZE;
        cr.rl.y -= POSITION_RECT_SIZE;
        *(this_->position_rect) = cr;
        traffic_traff_android_set_selection(this_);
    }
}


/**
 * @brief Initializes a traff_android plugin
 *
 * @return True on success, false on failure
 */
static int traffic_traff_android_init(struct traffic_priv * this_) {
    jmethodID cid;
    struct route * route;
    struct attr attr;
    struct navigation * navigation;

    if (!android_find_class_global("org/navitproject/navit/NavitTraff", &this_->NavitTraffClass))
        return 0;
    cid = (*jnienv)->GetMethodID(jnienv, this_->NavitTraffClass, "<init>", "(Landroid/content/Context;J)V");
    if (cid == NULL) {
        dbg(lvl_error,"no method found");
        return 0; /* exception thrown */
    }
    this_->NavitTraff=(*jnienv)->NewObject(jnienv, this_->NavitTraffClass, cid, android_activity,
                                           (jlong) this_->cbid);
    dbg(lvl_debug,"result=%p", this_->NavitTraff);
    if (!this_->NavitTraff)
        return 0;
    if (this_->NavitTraff)
        this_->NavitTraff = (*jnienv)->NewGlobalRef(jnienv, this_->NavitTraff);

    /* register callbacks for position and destination changes */
    navit_add_callback(this_->nav, callback_new_attr_1(callback_cast(traffic_traff_android_position_callback),
            attr_position_coord_geo, this_));
    navit_add_callback(this_->nav, callback_new_attr_1(callback_cast(traffic_traff_android_destination_callback),
            attr_destination, this_));
    if ((navigation = navit_get_navigation(this_->nav)))
        navigation_register_callback(navigation, attr_nav_status,
                callback_new_attr_1(callback_cast(traffic_traff_android_status_callback), attr_nav_status, this_));

    return 1;
}


/**
 * @brief Registers a new traff_android traffic plugin
 *
 * @param nav The navit instance
 * @param meth Receives the traffic methods
 * @param attrs The attributes for the map
 * @param cbl
 *
 * @return A pointer to a `traffic_priv` structure for the plugin instance
 */
static struct traffic_priv * traffic_traff_android_new(struct navit *nav, struct traffic_methods *meth,
        struct attr **attrs, struct callback_list *cbl) {
    struct traffic_priv *ret;

    dbg(lvl_debug, "enter");

    ret = g_new0(struct traffic_priv, 1);
    ret->nav = nav;
    ret->cbid = callback_new_1(callback_cast(traffic_traff_android_on_feed_received), ret);
    ret->position_valid = 0;
    ret->position_rect = NULL;
    ret->route_map_sel = NULL;
    /* TODO populate members, if any */
    *meth = traffic_traff_android_meth;

    traffic_traff_android_init(ret);

    return ret;
}

/**
 * @brief Initializes the traffic plugin.
 *
 * This function is called once on startup.
 */
void plugin_init(void) {
    dbg(lvl_debug, "enter");

    plugin_register_category_traffic("traff_android", traffic_traff_android_new);
}
