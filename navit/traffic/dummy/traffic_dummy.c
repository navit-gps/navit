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
#include "plugin.h"
#include "debug.h"

/**
 * @brief Stores information about the plugin instance.
 */
struct traffic_priv {
    struct navit * nav;         /*!< The navit instance */
    int reports_requested;      /*!< How many reports have been requested */
};

struct traffic_message ** traffic_dummy_get_messages(struct traffic_priv * this_);

/**
 * @brief Returns a dummy traffic report.
 *
 * This method will report two messages when first called: The messages indicate queuing traffic on the
 * A9 Munich–Nuremberg between Neufahrn and Allershausen, and slow traffic on the A96 Lindau–Munich
 * between Gräfelfing and München-Laim.
 *
 * The 10th call will report an update message for the A9 (with a recent timestamp but otherwise the same
 * data) and a cancellation message for the A96.
 *
 * They mimic TMC messages in that coordinates are approximate, TMC identifiers are supplied for the
 * locations and extra data fields which can be inferred from the TMC location table are filled. The
 * timestamps indicate a message that has just been received for the first time, i.e. its “first
 * received” and “last updated” timestamps match and are recent. Expiration is after 20 seconds for
 * messages in the first feed and 10 seconds for messages in the first feed (far below the lowest
 * expiration timespan permitted in TMC).
 *
 * All other calls to this method will return `NULL`, indicating that there are no messages to report.
 *
 * @return A `NULL`-terminated pointer array. Each element points to one `struct traffic_message`.
 * `NULL` is returned (rather than an empty pointer array) if there are no messages to report.
 */
struct traffic_message ** traffic_dummy_get_messages(struct traffic_priv * this_) {
    struct traffic_message ** messages;
    struct traffic_point * from;
    struct traffic_point * to;
    struct traffic_point * at;
    struct traffic_point * via;
    struct traffic_location * location;

    this_->reports_requested++;

    switch (this_->reports_requested) {
    case 10:
        messages = g_new0(struct traffic_message *, 6);

        from = traffic_point_new(11.6208, 48.3164, "Neufahrn", "68", "12732-4");
        to = traffic_point_new(11.5893, 48.429, "Allershausen", "67", "12732");
        location = traffic_location_new(NULL, from, to, NULL, NULL, "Nürnberg", NULL, location_dir_one,
                                        location_fuzziness_low_res, location_ramps_none, type_highway_land, NULL, "A9", "58:1", -1);
        messages[0] = traffic_message_new_single_event("dummy:A9-68-67", time(NULL), time(NULL),
                      time(NULL) + 86400, 0, location, event_class_congestion, event_congestion_queue);

        from = traffic_point_new(11.4481, 48.1266, "Gräfelfing", "36b", "12961-2");
        to = traffic_point_new(11.5028, 48.1258, "München-Laim", "38", "12961");
        location = traffic_location_new(NULL, from, to, NULL, NULL, "München", NULL, location_dir_one,
                                        location_fuzziness_low_res, location_ramps_none, type_highway_land, NULL, "A96", "58:1", -1);
        messages[1] = traffic_message_new_single_event("dummy:A96-36b-38", time(NULL), time(NULL),
                      time(NULL) + 86400, 0, location, event_class_congestion, event_congestion_slow_traffic);

        from = traffic_point_new(11.6143, 48.15255, "Effnertunnel", NULL, "60922");
        to = traffic_point_new(11.53225, 48.13255, "Trappentreutunnel", NULL, "35333");
        via = traffic_point_new(11.5728, 48.178, "Petueltunnel", NULL, "29829");
        location = traffic_location_new(NULL, from, to, via, NULL, NULL, NULL, location_dir_one,
                                        location_fuzziness_low_res, location_ramps_none, type_line_unspecified, NULL, "B2R", "58:1", 1);
        messages[2] = traffic_message_new_single_event("dummy:B2R-N", time(NULL), time(NULL),
                      time(NULL) + 86400, 0, location, event_class_congestion, event_congestion_slow_traffic);

        from = traffic_point_new(11.6143, 48.15255, "Effnertunnel", NULL, "60922");
        to = traffic_point_new(11.53225, 48.13255, "Trappentreutunnel", NULL, "35333");
        via = traffic_point_new(11.55085, 48.11225, "Brudermühltunnel", NULL, "35329");
        location = traffic_location_new(NULL, from, to, via, NULL, NULL, NULL, location_dir_one,
                                        location_fuzziness_low_res, location_ramps_none, type_line_unspecified, NULL, "B2R", "58:1", -1);
        messages[3] = traffic_message_new_single_event("dummy:B2R-S", time(NULL), time(NULL),
                      time(NULL) + 86400, 0, location, event_class_congestion, event_congestion_slow_traffic);

        from = traffic_point_new(11.6208, 48.3164, "Neufahrn", "68", "12727+5");
        at = traffic_point_new(11.6405, 48.2435, "Garching-Süd", "71", "12727");
        location = traffic_location_new(at, from, NULL, NULL, NULL, NULL, NULL, location_dir_one,
                                        location_fuzziness_low_res, location_ramps_none, type_highway_land, NULL, "A9", "58:1", 1);
        messages[4] = traffic_message_new_single_event("dummy:A9-71-S", time(NULL), time(NULL),
                      time(NULL) + 86400, 0, location, event_class_congestion, event_congestion_slow_traffic);
        break;

    case 20:
        messages = g_new0(struct traffic_message *, 4);

        from = traffic_point_new(11.6208, 48.3164, "Neufahrn", "68", "12732-4");
        to = traffic_point_new(11.5893, 48.429, "Allershausen", "67", "12732");
        location = traffic_location_new(NULL, from, to, NULL, NULL, "Nürnberg", NULL, location_dir_one,
                                        location_fuzziness_low_res, location_ramps_none, type_highway_land, NULL, "A9", "58:1", -1);
        messages[0] = traffic_message_new_single_event("dummy:A9-68-67", time(NULL) - 10, time(NULL),
                      time(NULL) + 10, 0, location, event_class_congestion, event_congestion_queue);

        from = traffic_point_new(11.4481, 48.1266, "Gräfelfing", "36b", "12961-2");
        to = traffic_point_new(11.5028, 48.1258, "München-Laim", "38", "12961");
        location = traffic_location_new(NULL, from, to, NULL, NULL, "München", NULL, location_dir_one,
                                        location_fuzziness_low_res, location_ramps_none, type_highway_land, NULL, "A96", "58:1", -1);
        messages[1] = traffic_message_new_cancellation("dummy:A96-36b-38", time(NULL) - 10, time(NULL),
                      time(NULL) + 10, location);
        break;

    default:
        return NULL;
    }

    return messages;
}

/**
 * @brief The methods implemented by this plugin
 */
static struct traffic_methods traffic_dummy_meth = {
    traffic_dummy_get_messages,
    NULL,
};

/**
 * @brief Registers a new dummy traffic plugin
 *
 * @param nav The navit instance
 * @param meth Receives the traffic methods
 * @param attrs The attributes for the map
 * @param cbl
 *
 * @return A pointer to a `traffic_priv` structure for the plugin instance
 */
static struct traffic_priv * traffic_dummy_new(struct navit *nav, struct traffic_methods *meth,
        struct attr **attrs, struct callback_list *cbl) {
    struct traffic_priv *ret;

    dbg(lvl_debug, "enter");

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
    dbg(lvl_debug, "enter");

    plugin_register_category_traffic("dummy", traffic_dummy_new);
}
