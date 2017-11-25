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
        traffic_report_messages(0, NULL);
}

/**
 * @brief Inserts a dummy traffic report.
 *
 * This will report one single message, indicating queuing traffic on the A9 Munich–Nuremberg between
 * Neufahrn and Allershausen. This mimics a TMC message in that coordinates are approximate, TMC
 * identifiers are supplied for the location and extra data fields which can be inferred from the TMC
 * location table are filled. The message purports to just have been received for the first time, and
 * expire in 24 hours.
 */
void traffic_dummy_report(void) {
	struct traffic_message ** messages = g_new0(struct traffic_message *, 1);
	struct traffic_point * from = traffic_point_new(11.6208, 48.3164, "Neufahrn", "68", "12732-4");
	struct traffic_point * to = traffic_point_new(11.5893, 48.429, "Allershausen", "67", "12732");
	struct trafic_location * location = traffic_location_new(NULL, from, to, "Nürnberg", NULL,
			location_dir_one, location_fuzziness_low_res, location_ramps_none, type_highway_land,
			NULL, "A9", "58:1", -1);

	messages[0] = traffic_message_new_single_event("dummy:A9-68-67", time(NULL), time(NULL),
			time(NULL) + 86400, 0, 0, location, event_class_congestion, event_congestion_queue);
	traffic_report_messages(1, messages);
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
