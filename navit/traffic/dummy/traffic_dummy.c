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
 * @file traffic_dummy.c
 *
 * @brief A dummy traffic plugin.
 *
 * This is a dummy plugin to test the traffic framework.
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
#include "traffic.h"
#include "event.h"
#include "callback.h"
#include "debug.h"

/**
 * @brief Stores information about the plugin instance.
 */
struct traffic {
        struct navit * nav;         /*!< The navit instance */
        struct callback * callback; /*!< The callback function for the idle loop */
        struct event_idle * idle;   /*!< The idle event that triggers the idle function */
};

/**
 * @brief The idle function for the traffic plugin.
 *
 * This function polls backends for new messages and processes them by inserting, removing or modifying
 * traffic distortions and triggering route recalculations as needed.
 */
void traffic_idle(struct traffic * this_) {
        // TODO poll backends and process any new messages
        dbg(lvl_error, "THIS IS THE DUMMY TRAFFIC PLUGIN. Got nothing to do yet...\n");
}

/**
 * @brief Initializes the traffic plugin.
 *
 * This function is called once on startup.
 */
void plugin_init(void) {
        struct traffic * this_;

        dbg(lvl_error, "THIS IS THE DUMMY TRAFFIC PLUGIN. Startup successful, more to come soon...\n");

        this_ = g_new0(struct traffic, 1);

        // TODO register ourselves as a map plugin (once we have a map provider)

        // FIXME error:navit:event_add_idle:Can't find event system method add_idle. Event system is not set.
        this_->callback = callback_new_1(callback_cast(traffic_idle), this_);
        this_->idle = event_add_idle(200, this_->callback); // TODO store return value with plugin instance
}
