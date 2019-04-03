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
 * @file traffic_traff_file.c
 *
 * @brief A traff_file traffic plugin.
 *
 * This is a traff_file plugin to read traff messages from a file.
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
#include <stdlib.h>

long xml_last_msg = 0;
FILE* traff_file_ptr = 0;

char *traffic_read_traff_file(char *filename);

char *traffic_read_traff_file(char *filename) {

    char *data = 0;

    if (traff_file_ptr == 0) {
        if (filename) {
            traff_file_ptr = fopen(filename, "rb");
        }
    }

    if (traff_file_ptr) {

        fseek(traff_file_ptr, 0, SEEK_END);
        long end = ftell(traff_file_ptr);
        rewind(traff_file_ptr);
        long start = xml_last_msg;

        fseek(traff_file_ptr, start, SEEK_SET);

        data = g_malloc(end - start + 1);
//           char data[end - start + 1];
        if ((sizeof(data) - 1) > 0) {
            while (fread(data, 1, (long) (end - start), traff_file_ptr))
                ;
            data[(end - start)] = '\0';

            xml_last_msg = end;
        }
    }
    return data;
}

    /**
     * @brief Stores information about the plugin instance.
     */
    struct traffic_priv {
        struct navit * nav; /*!< The navit instance */
        int reports_requested; /*!< How many reports have been requested */
    };

    struct traffic_message ** traffic_traff_file_get_messages(
            struct traffic_priv * this_);

    /**
     * @brief Returns a traff_file traffic report.
     *
     * This method will report two messages when first called: The messages indicate queuing traffic on the
     * A9 Munichâ€“Nuremberg between Neufahrn and Allershausen, and slow traffic on the A96 Lindauâ€“Munich
     * between GrÃ¤felfing and MÃ¼nchen-Laim.
     *
     * The 10th call will report an update message for the A9 (with a recent timestamp but otherwise the same
     * data) and a cancellation message for the A96.
     *
     * They mimic TMC messages in that coordinates are approximate, TMC identifiers are supplied for the
     * locations and extra data fields which can be inferred from the TMC location table are filled. The
     * timestamps indicate a message that has just been received for the first time, i.e. its â€œfirst
     * receivedâ€� and â€œlast updatedâ€� timestamps match and are recent. Expiration is after 20 seconds for
     * messages in the first feed and 10 seconds for messages in the first feed (far below the lowest
     * expiration timespan permitted in TMC).
     *
     * All other calls to this method will return `NULL`, indicating that there are no messages to report.
     *
     * @return A `NULL`-terminated pointer array. Each element points to one `struct traffic_message`.
     * `NULL` is returned (rather than an empty pointer array) if there are no messages to report.
     */
    struct traffic_message ** traffic_traff_file_get_messages(
            struct traffic_priv * this_) {
        struct traffic * traffic = NULL;
        struct traffic_message ** messages;

        dbg(lvl_debug, "enter");

        dbg(lvl_debug, "processing traffic from file: traff.xml");

        char *filename = g_strdup_printf("%s\\%s", getenv("NAVIT_USER_DATADIR"),
                "traff.xml");

        char * xml;

        xml = traffic_read_traff_file(filename);

        messages = traffic_get_messages_from_xml_string(traffic, xml);

        if (xml)
            g_free(xml);

        return messages;
    }

    /**
     * @brief The methods implemented by this plugin
     */
    static struct traffic_methods traffic_traff_file_meth = {
            traffic_traff_file_get_messages, };

    /**
     * @brief Registers a new traff_file traffic plugin
     *
     * @param nav The navit instance
     * @param meth Receives the traffic methods
     * @param attrs The attributes for the map
     * @param cbl
     *
     * @return A pointer to a `traffic_priv` structure for the plugin instance
     */
    static struct traffic_priv * traffic_traff_file_new(struct navit *nav,
            struct traffic_methods *meth, struct attr **attrs,
            struct callback_list *cbl) {
        struct traffic_priv *ret;

        dbg(lvl_debug, "enter");

        ret = g_new0(struct traffic_priv, 1);
        *meth = traffic_traff_file_meth;

        return ret;
    }

    /**
     * @brief Initializes the traffic plugin.
     *
     * This function is called once on startup.
     */
    void plugin_init(void) {
        dbg(lvl_debug, "enter");

        plugin_register_category_traffic("traff_file", traffic_traff_file_new);
    }

