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

/**
 * @file traffic_null.c
 *
 * @brief A null traffic plugin.
 *
 * This plugin was mainly designed to test the traffic framework. It acts like a traffic plugin but will
 * never report any messages. This allows us to have the full traffic functionality without having an
 * actual source for traffic messages; useful for injecting messages via DBus.
 */

#include <string.h>
#include <time.h>

#ifdef _POSIX_C_SOURCE
#include <sys/types.h>
#endif
#include "glib_slice.h"
#include "config.h"
#include "coord.h"
#include "item.h"
#include "xmlconfig.h"
#include "traffic.h"
#include "plugin.h"
#include "debug.h"

/**
 * @brief Stores information about the plugin instance.
 */
struct traffic_priv {
    struct navit * nav;         /*!< The navit instance */
};

struct traffic_message ** traffic_null_get_messages(struct traffic_priv * this_);

/**
 * @brief Returns an empty traffic report.
 *
 * @return Always `NULL`
 */
struct traffic_message ** traffic_null_get_messages(struct traffic_priv * this_) {
    return NULL;
}

/**
 * @brief The methods implemented by this plugin
 */
static struct traffic_methods traffic_null_meth = {
    traffic_null_get_messages,
};

/**
 * @brief Registers a new null traffic plugin
 *
 * @param nav The navit instance
 * @param meth Receives the traffic methods
 * @param attrs The attributes for the map
 * @param cbl
 *
 * @return A pointer to a `traffic_priv` structure for the plugin instance
 */
static struct traffic_priv * traffic_null_new(struct navit *nav, struct traffic_methods *meth,
        struct attr **attrs, struct callback_list *cbl) {
    struct traffic_priv *ret;

    dbg(lvl_debug, "enter");

    ret = g_new0(struct traffic_priv, 1);
    *meth = traffic_null_meth;

    return ret;
}

/**
 * @brief Initializes the traffic plugin.
 *
 * This function is called once on startup.
 */
void plugin_init(void) {
    dbg(lvl_debug, "enter");

    plugin_register_category_traffic("null", traffic_null_new);
}
