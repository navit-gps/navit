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
 * This plugin receives TraFF feeds via Android broadcasts.
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
#include "xmlconfig.h"
#include "android.h"
#include "traffic.h"
#include "plugin.h"
#include "callback.h"
#include "debug.h"
#include "navit.h"

/**
 * @brief Stores information about the plugin instance.
 */
struct traffic_priv {
    struct navit * nav;         /**< The navit instance */
    struct callback * cbid;     /**< The callback function for TraFF feeds **/
    jclass NavitTraffClass;     /**< The `NavitTraff` class */
    jobject NavitTraff;         /**< An instance of `NavitTraff` */
};

struct traffic_message ** traffic_traff_android_get_messages(struct traffic_priv * this_);

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
 * @brief Initializes a traff_android plugin
 *
 * @return True on success, false on failure
 */
static int traffic_traff_android_init(struct traffic_priv * this_) {
    jmethodID cid;

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
