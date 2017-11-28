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
#include "xmlconfig.h"
#include "traffic.h"
#include "debug.h"

/**
 * @brief Stores information about the plugin instance.
 */
struct traffic_priv {
	struct navit * nav;         /*!< The navit instance */
};

/**
 * @brief Returns a dummy traffic report.
 *
 * This will report one single message, indicating queuing traffic on the A9 Munich–Nuremberg between
 * Neufahrn and Allershausen. This mimics a TMC message in that coordinates are approximate, TMC
 * identifiers are supplied for the location and extra data fields which can be inferred from the TMC
 * location table are filled. The message purports to just have been received for the first time, and
 * expire in 24 hours.
 *
 * @return A `NULL`-terminated pointer array. Each element points to one `struct traffic_message`.
 */
struct traffic_message ** traffic_dummy_get_messages(void) {
	struct traffic_message ** messages = g_new0(struct traffic_message *, 2);
	struct traffic_point * from = traffic_point_new(11.6208, 48.3164, "Neufahrn", "68", "12732-4");
	struct traffic_point * to = traffic_point_new(11.5893, 48.429, "Allershausen", "67", "12732");
	struct trafic_location * location = traffic_location_new(NULL, from, to, "Nürnberg", NULL,
			location_dir_one, location_fuzziness_low_res, location_ramps_none, type_highway_land,
			NULL, "A9", "58:1", -1);

	messages[0] = traffic_message_new_single_event("dummy:A9-68-67", time(NULL), time(NULL),
			time(NULL) + 86400, 0, 0, location, event_class_congestion, event_congestion_queue);

	return messages;
}

/**
 * @brief The methods implemented by this plugin
 */
static struct traffic_methods traffic_dummy_meth = {
		traffic_dummy_get_messages,
};

/**
 * @brief Registers a new dummy traffic plugin
 *
 * @param meth Receives the traffic methods
 * @param attrs The attributes for the map
 * @param cbl
 * @param parent The parent of the plugin, must be the navit instance
 *
 * @return A pointer to a `traffic_priv` structure for the plugin instance
 */
static struct traffic_priv * traffic_dummy_new(struct traffic_methods *meth, struct attr **attrs,
		struct callback_list *cbl, struct attr *parent) {
	struct traffic_priv *ret;

	dbg(lvl_error, "enter\n");

	ret = g_new0(struct traffic_priv, 1);
	*meth = traffic_dummy_meth;

	return ret;
}

/**
 * @brief Initializes the traffic plugin.
 *
 * This function is called once on startup.
 */
void plugin_init(void) {
	dbg(lvl_error, "enter\n");

	plugin_register_category_traffic("dummy", traffic_dummy_new);
}
