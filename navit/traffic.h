/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2017 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

/** @file
 *
 * @brief Contains exported code for traffic.c, the traffic module
 *
 * This file contains types and function prototypes exported from the traffic module, which enables
 * Navit to route around traffic problems.
 *
 * The traffic module consists of two parts:
 *
 * The traffic core interacts with the Navit core and converts traffic messages into traffic
 * distortions (future versions may add support for other traffic information).
 *
 * The traffic backends obtain traffic information from a source of their choice (e.g. from a TMC
 * receiver or a network service), translate them into Navit data structures and report them to the
 * traffic plugin.
 *
 * Traffic messages and related structures are considered immutable once created (when information
 * changes, the old message is replaced with a new one). For this reason, there are very few data
 * manipulation methods. Those that exist are intended for the creation of new messages rather than
 * for extensive manipulation.
 *
 * As a rule, responsibility for freeing up any `traffic_*` instances normally lies with the
 * traffic plugin, which frees messages as they expire or are replaced. Since this also frees all child
 * data structures, traffic backends will seldom need to call any of the destructors. The only case in
 * which this would be necessary is if a backend has instantiated an object which is not going to be
 * used (i.e. attached to a parent object or, in the case of `traffic_message`, reported to the
 * traffic plugin: these need to be freed up manually by calling the destructor of the topmost object in
 * the hierarchy.
 *
 * Any other references passed in functions (including pointer arrays and `quantifier` instances)
 * must be freed up by the caller. This is safe to do as soon as the function returns.
 */

#ifndef NAVIT_TRAFFIC_H
#define NAVIT_TRAFFIC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Classes for events.
 */
/* If additional event classes are introduced, traffic_event_is_valid() must be adapted to recognize them. */
enum event_class {
	event_class_invalid = 0, /*!< Invalid event which should be ignored */
	event_class_congestion,  /*!< Traffic congestion, typically indicating the approximate speed */
	event_class_delay,       /*!< Delays, typically indicating the amount of extra waiting time */
	event_class_restriction, /*!< Temporary traffic restrictions, such as road or lane closures or size,
	                          *   weight or access restrictions */
};

/**
 * @brief Event types.
 */
/* If additional events are introduced, remember to do the following:
 * - If the events belong to an existing class, insert them right after the last existing event for that class.
 * - If the events belong to a new class, insert them at the end of the list.
 * - Always keep events of the same class together.
 * - After adding events (of any class) at the end of the list, adapt traffic_event_is_valid() to recognize them. */
enum event_type {
	event_invalid = 0,                                 /*!< Invalid event which should be ignored */
	event_congestion_cleared,                          /*!< Traffic congestion cleared */
	event_congestion_forecast_withdrawn,               /*!< Traffic congestion forecast withdrawn */
	event_congestion_heavy_traffic,                    /*!< Heavy traffic with average speeds of `speed` */
	event_congestion_long_queue,                       /*!< Long queues with average speeds of `speed` */
	event_congestion_none,                             /*!< No problems to report */
	event_congestion_normal_traffic,                   /*!< Traffic has returned to normal */
	event_congestion_queue,                            /*!< Queuing traffic with average speeds of `speed` */
	event_congestion_queue_likely,                     /*!< Danger of queuing traffic with average speeds
	                                                    *   of `speed` */
	event_congestion_slow_traffic,                     /*!< Slow traffic with average speeds of `speed` */
	event_congestion_stationary_traffic,               /*!< Stationary traffic (frequent standstills) */
	event_congestion_stationary_traffic_likely,        /*!< Danger of stationary traffic */
	event_congestion_traffic_building_up,              /*!< Traffic building up with average speeds of
	                                                    *   `speed` */
	event_congestion_traffic_congestion,               /*!< Traffic congestion with average speeds of
	                                                    *   `speed` */
	event_congestion_traffic_easing,                   /*!< Traffic easing */
	event_congestion_traffic_flowing_freely,           /*!< Traffic flowing freely with average speeds
	                                                    *   of `speed` */
	event_congestion_traffic_heavier_than_normal,      /*!< Traffic heavier than normal with average
	                                                    *   speeds of `speed` */
	event_congestion_traffic_lighter_than_normal,      /*!< Traffic lighter than normal with average
	                                                    *   speeds of `speed` */
	event_congestion_traffic_much_heavier_than_normal, /*!< Traffic very much heavier than normal with
	                                                    *   average speeds of `speed` (increased density
	                                                    *   but no significant decrease in speed) */
	event_congestion_traffic_problem,                  /*!< Traffic problem */
	event_delay_clearance,                             /*!< Delays cleared */
	event_delay_delay,                                 /*!< Delays up to `q_timespan` */
	event_delay_delay_possible,                        /*!< Delays up to `q_timespan` possible */
	event_delay_forecast_withdrawn,                    /*!< Delay forecast withdrawn */
	event_delay_long_delay,                            /*!< Long delays up to `q_timespan` */
	event_delay_several_hours,                         /*!< Delays of several hours */
	event_delay_uncertain_duration,                    /*!< Delays of uncertain duration */
	event_delay_very_long_delay,                       /*!< Very long delays up to `q_timespan` */
	event_restriction_access_restrictions_lifted,      /*!< Traffic restrictions lifted: reopened for all
                                                        *   traffic, other restrictions (overtaking etc.)
                                                        *   remain in place */
	event_restriction_all_carriageways_cleared,        /*!< All carriageways cleared */
	event_restriction_all_carriageways_reopened,       /*!< All carriageways reopened */
	event_restriction_batch_service,                   /*!< Batch service (to limit the amount of traffic
	                                                    *   passing through a section, unlike single
	                                                    *   alternate line traffic) */
	event_restriction_blocked,                         /*!< Blocked (refers to the entire road; separate
	                                                    *   codes exist for blockages of individual lanes
	                                                    *   or carriageways) */
	event_restriction_blocked_ahead,                   /*!< Blocked ahead (at a point beyond the
	                                                    *   indicated location) */
	event_restriction_carriageway_blocked,             /*!< Carriageway blocked (main carriageway, unless
	                                                    *   otherwise indicated in supplementary information) */
	event_restriction_carriageway_closed,              /*!< Carriageway closed (main carriageway, unless
	                                                    *   otherwise indicated in supplementary information) */
	event_restriction_contraflow,                      /*!< Contraflow */
	event_restriction_closed,                          /*!< Closed until `q_time` (refers to the entire
	                                                    *   road; separate codes exist for closures of
	                                                    *   individual lanes or carriageways) */
	event_restriction_closed_ahead,                    /*!< Closed ahead (at a point beyond the indicated
	                                                    *   location) */
	event_restriction_entry_blocked,                   /*!< `q_int` th entry slip road blocked */
	event_restriction_entry_reopened,                  /*!< Entry reopened */
	event_restriction_exit_blocked,                    /*!< `q_int` th exit slip road blocked */
	event_restriction_exit_reopened,                   /*!< Exit reopened */
	event_restriction_intermittent_closures,           /*!< Intermittent short term closures */
	event_restriction_lane_blocked,                    /*!< `q_int` lanes blocked */
	event_restriction_lane_closed,                     /*!< `q_int` lanes closed */
	event_restriction_open,                            /*!< Open */
	event_restriction_ramp_blocked,                    /*!< Ramps blocked */
	event_restriction_ramp_closed,                     /*!< Ramps closed */
	event_restriction_ramp_reopened,                   /*!< Ramps reopened */
	event_restriction_reduced_lanes,                   /*!< Carriageway reduced from `q_ints[1]` lanes to `q_int[0]`
                                                        *   lanes (quantifiers are currently not implemented for this
                                                        *   event type) */
	event_restriction_reopened,                        /*!< Reopened */
	event_restriction_road_cleared,                    /*!< Road cleared */
	event_restriction_single_alternate_line_traffic,   /*!< Single alternate line traffic (because the
	                                                    *   affected stretch of road can only be used in
	                                                    *   one direction at a time, different from batch
	                                                    *   service) */
	event_restriction_speed_limit,                     /*!< Speed limit `speed` in force */
	event_restriction_speed_limit_lifted,              /*!< Speed limit lifted */
};

/**
 * @brief The directionality of a location.
 */
enum location_dir {
	location_dir_one = 1,  /*!< Indicates a unidirectional location. */
	location_dir_both = 2, /*!< Indicates a bidirectional location. */
};

/**
 * @brief The fuzziness of a location.
 */
enum location_fuzziness {
	location_fuzziness_none = 0,       /*!< No fuzziness information is given. */
	location_fuzziness_low_res,        /*!< Locations are constrained to a predefined table; the actual
	                                    *   extent of the condition may be shorter than indicated. */
	location_fuzziness_end_unknown,    /*!< The end is unknown, e.g. a traffic jam reported by a driver
	                                    *   who has just entered it. */
	location_fuzziness_start_unknown,  /*!< The start is unknown, e.g. a traffic jam reported by a driver
	                                        who has just passed the obstruction which caused it. */
	location_fuzziness_extent_unknown, /*!< Start and end are unknown, e.g. a traffic jam reported by a
	                                        driver who is in the middle of it. */
};

/**
 * @brief Whether a location refers to the main carriageway or the ramps.
 */
enum location_ramps {
	location_ramps_none = 0, /*!< The location refers to the carriageways of the main road. */
	location_ramps_all,      /*!< The location refers to the entry and exit ramps, not the main carriageway. */
	location_ramps_entry,    /*!< The location refers to the entry ramps only, not the main carriageway. */
	location_ramps_exit,     /*!< The location refers to the exit ramps only, not the main carriageway. */
};

/**
 * @brief Classes for supplementary information items.
 */
enum si_class {
	si_class_invalid = 0, /*!< Invalid supplementary information item which should be ignored */
	si_class_place,       /*!< Qualifiers specifying the place(s) to which the event refers */
	si_class_tendency,    /*!< Traffic density development */
	si_class_vehicle,     /*!< Specifies categories of vehicles to which the event applies */
};

/**
 * @brief Supplementary information types.
 */
enum si_type {
	si_invalid = 0,               /*!< Invalid supplementary information item which should be ignored */
	si_place_bridge,              /*!< On bridges */
	si_place_ramp,                /*!< On ramps (entry/exit) */
	si_place_roadworks,           /*!< In the roadworks area */
	si_place_tunnel,              /*!< In tunnels */
	si_tendency_queue_decreasing, /*!< Traffic queue length decreasing (average rate in optional `q_speed`) */
	si_tendency_queue_increasing, /*!< Traffic queue length increasing (average rate in optional `q_speed`) */
	si_vehicle_all,               /*!< For all vehicles */
	si_vehicle_bus,               /*!< For buses only (TODO currently supported for public buses only) */
	si_vehicle_car,               /*!< For cars only */
	si_vehicle_car_with_caravan,  /*!< For cars with caravans only (TODO currently not supported) */
	si_vehicle_car_with_trailer,  /*!< For cars with trailers only (TODO currently not supported) */
	si_vehicle_hazmat,            /*!< For hazardous loads only */
	si_vehicle_hgv,               /*!< For heavy trucks only */
	si_vehicle_motor,             /*!< For all motor vehicles */
	si_vehicle_with_trailer,      /*!< For vehicles with trailers only (TODO currently not supported) */
};

struct traffic;
struct traffic_priv;
struct traffic_location_priv;
struct traffic_message_priv;

/**
 * @brief Holds all functions a traffic plugin has to implement to be usable
 *
 * This structure holds pointers to a traffic plugin's functions which navit's core will call
 * to communicate with the plugin.
 */
struct traffic_methods {
	struct traffic_message **(* get_messages)(struct traffic_priv * this_); /**< Retrieves new messages from the traffic plugin */
};

/**
 * @brief A point on the road.
 *
 * This can either be a point location or an endpoint of a linear location. It specifies a coordinate
 * pair and can optionally be supplemented with a junction name and/or number where applicable.
 */
struct traffic_point {
	struct coord_geo coord; /*!< The coordinates of this point, as supplied by the source. These may
	                         *   deviate somewhat from the coordinates on the map. */
	char * junction_name;   /*!< The name of the motorway junction this point refers to. */
	char * junction_ref;    /*!< The reference number of the motorway junction this point refers to. */
	char * tmc_id;          /*!< The TMC identifier of the point, if the location was obtained via TMC.
	                         *   This can be an LCID (12345) or a combination of an LCID and an offset
	                         *   (12345+2, i.e. an offset of 2 points in positive direction from 12345).
	                         *   The offset is typically used with the secondary location in TMC. */
};

/**
 * @brief Location data for a traffic message.
 *
 * Locations can be either point or linear locations.
 *
 * Linear locations are indicated by a pair of points and refer to a stretch of road. The entire
 * location must be part of the same road, i.e. either the road name or the road reference must be the
 * same for all affected segments, or all segments must be of the same type and be of a higher order
 * than any road connecting to them directly.
 *
 * Point locations are indicated by a single point, as well as one or two auxiliary points to indicate
 * direction. Auxiliary points can be omitted if `tmc_table`, `tmc_direction` and
 * `at->tmc_id` are supplied. However, this will only work if the map has accurate TMC data for
 * the location, thus it is recommended to supply an auxiliary point nonetheless.
 *
 * The order of points is as a driver would encounter them, i.e. first `from`, then `at`,
 * finally `to`.
 */
struct traffic_location {
	struct traffic_point * at;         /*!< The point for a point location, NULL for linear locations. */
	struct traffic_point * from;       /*!< The start of a linear location, or a point before `at`. */
	struct traffic_point * to;         /*!< The end of a linear location, or a point after `at`. */
	struct traffic_point * via;        /*!< A point between `from` and `to`. Required on ring roads
	                                    *   unless `not_via` is used; cannot be used together with `at`. */
	struct traffic_point * not_via;    /*!< A point NOT between `from` and `to`. Required on ring roads
	                                    *   unless `via` is used; cannot be used together with `at`. */
	char * destination;                /*!< A destination, preferably the one given on road signs,
	                                    *   indicating that the message applies only to traffic going in
	                                    *   that direction. Do not use for bidirectional locations. */
	char * direction;                  /*!< A compass direction indicating the direction of travel which
	                                    *   this location refers to. Do not use where ambiguous. */
	enum location_dir directionality;  /*!< Indicates whether the message refers to one or both directions
	                                    *   of travel. */
	enum location_fuzziness fuzziness; /*!< Indicates how precisely the end points are known. */
	enum location_ramps ramps;         /*!< Any value other than `location_ramps_none` implies
	                                    *   that only the specified ramps are affected while the main
	                                    *   road is not. In that case, the `road*` fields refer to
	                                    *   the main road served by the ramp, not the ramp itself. This
	                                    *   is mainly intended for compatibility with TMC, where
	                                    *   junctions with all their ramps are represented by a single
	                                    *   point. Other sources should use coordinate pairs instead. */
	enum item_type road_type;          /*!< The importance of the road within the road network, must be a
	                                    *   road item type. Use `line_unspecified` if not known or
	                                    *   not consistent. */
	char * road_name;                  /*!< A road name, if consistent throughout the location. */
	char * road_ref;                   /*!< A road number, if consistent throughout the location. */
	char * tmc_table;                  /*!< For messages received via TMC, the country identifier (CID)
	                                    *   and location table number (LTN or TABCD) for the location
	                                    *   table to be used for location lookup. The CID is the decimal
	                                    *   number stored in the COUNTRIES and LOCATIONDATASETS tables,
	                                    *   not the hexadecimal code from the PI (known as CCD in TMC). */
	int tmc_direction;                 /*!< For messages received via TMC, the direction of the road to
	                                    *   which this message applies (positive or negative). Ignored
	                                    *   for bidirectional messages. */
	struct traffic_location_priv * priv; /*!< Internal data, not exposed via the API */
};

/**
 * @brief A quantifier, which can be used with events and supplementary information.
 */
/*
 * For now, these are various integer types, but other types may be added in the future.
 */
struct quantifier {
	union {
		int q_duration;      /*!< A duration in 1/10 of a second. */
		int q_int;           /*!< An integer. */
		int q_speed;         /*!< A speed in km/h. */
		unsigned int q_time; /*!< A timestamp in epoch time (seconds elapsed since Jan 1, 1970, 00:00 UTC). */
	} u;
};

/**
 * @brief Extra information supplied with a traffic event.
 */
struct traffic_suppl_info {
	enum si_class si_class;       /*!< The supplementary information class (generic category). */
	enum si_type type;            /*!< The supplementary information type, which can be mapped to a
	                               *   string to be displayed to the user. */
	struct quantifier * quantifier; /*!< Additional quantifier for supplementary information types
	                               *   allowing this. Data type and meaning depends on the event type. */
};

/**
 * @brief A traffic event.
 *
 * An event refers to a condition, its cause or its effect.
 */
struct traffic_event {
	enum event_class event_class;    /*!< The event class (generic category). */
	enum event_type type;            /*!< The event type, which can be mapped to a string to be displayed
                                      *   to the user. */
	int length;                      /*!< The length of the affected route in meters. */
	int speed;                       /*!< The speed in km/h at which vehicles can expect to pass through the
	                                  *   affected stretch of road (either a temporary speed limit or
	                                  *   average speed in practice, whichever is less), `INT_MAX` if
	                                  *   not set or unknown. */
	struct quantifier * quantifier;  /*!< Additional quantifier for events allowing this. Data type and
	                                  *   meaning depends on the event type. */
	int si_count;                    /*!< Number of supplementary information items in `si_count`. */
	struct traffic_suppl_info ** si; /*!< Points to an array of pointers to supplementary information items. */
};

/**
 * @brief A traffic message.
 *
 * A message is the atomic element of traffic information, referring to a particular condition at a
 * given location.
 *
 * If no updates are received for a message, it should be discarded after both `expiration_time`
 * and `end_time` (if specified) have elapsed.
 */
struct traffic_message {
	char * id;                  /*!< An identifier, which remains stable over the entire lifecycle of the
	                             *   message. The colon (:) is a reserved character to separate different
	                             *   levels of source identifiers from each other and from the local
	                             *   message identifier. */
	time_t receive_time;        /*!< When the message was first received by the source, should be kept
	                             *   stable across all updates. */
	time_t update_time;         /*!< When the last update to this message was received by the source. */
	time_t expiration_time;     /*!< How long the message should be considered valid.*/
	time_t start_time;          /*!< When the condition is expected to begin (optional, 0 if not set). */
	time_t end_time;            /*!< How long the condition is expected to last (optional, 0 if not set). */
	int is_cancellation;        /*!< If true, this message is a cancellation message, indicating that
	                             *   existing messages with the same ID should be deleted or no longer
	                             *   considered current. All other attributes of a cancellation message
	                             *   should be ignored. */
	int is_forecast;            /*!< If false, the message describes a current situation. If true, it
	                             *   describes an expected situation in the future. */
	int replaced_count;         /*!< The number of entries in `replaces`. */
	char ** replaces;           /*!< Points to an array of identifiers of messages which the current
	                             *   message replaces. */
	struct traffic_location * location; /*!< The location to which this message refers. */
	int event_count;            /*!< The number of events in `events`. */
	struct traffic_event ** events; /*!< Points to an array of pointers to the events for this message. */
	struct traffic_message_priv * priv; /*!< Internal data, not exposed via the API */
};

struct map;
struct mapset;
struct traffic;

/**
 * @brief Creates an event class from its string representation.
 *
 * @param string The string representation (case is ignored)
 *
 * @return The corresponding `enum event_class`, or `event_class_invalid` if `string` does not match a
 * known identifier
 */
enum event_class event_class_new(char * string);

/**
 * @brief Translates an event class to its string representation.
 *
 * @return The string representation of the event class
 */
const char * event_class_to_string(enum event_class this_);

/**
 * @brief Creates an event type from its string representation.
 *
 * @param string The string representation (case is ignored)
 *
 * @return The corresponding `enum event_type`, or `event_invalid` if `string` does not match a known
 * identifier
 */
enum event_type event_type_new(char * string);

/**
 * @brief Translates an event type to its string representation.
 *
 * @return The string representation of the event type
 */
const char * event_type_to_string(enum event_type this_);

/**
 * @brief Creates an item type from a road type.
 *
 * This is guaranteed to return either a routable type (i.e. `route_item_first <= type <= route_item_last`)
 * or `type_line_unspecified`. The latter is also returned if `string` refers to a Navit item type which
 * is not routable.
 *
 * @param string A TraFF road type or the string representation of a Navit item type
 * @param is_urban Whether the road is in a built-up area (ignored if `string` is a Navit item type)
 *
 * @return The corresponding `enum item_type`, or `type_line_unspecified` if `string` does not match a
 * known and routable identifier
 */
enum item_type item_type_from_road_type(char * string, int is_urban);

/**
 * @brief Creates a location directionality from its string representation.
 *
 * @param string The string representation (case is ignored)
 *
 * @return The corresponding `enum location_dir`, or `location_dir_both` if `string` does
 * not match a known identifier
 */
enum location_dir location_dir_new(char * string);

/**
 * @brief Creates a location fuzziness from its string representation.
 *
 * @param string The string representation (case is ignored)
 *
 * @return The corresponding `enum location_fuzziness`, or `location_fuzziness_none` if `string` does
 * not match a known identifier
 */
enum location_fuzziness location_fuzziness_new(char * string);

/**
 * @brief Translates location fuzziness to its string representation.
 *
 * @return The string representation of the location fuzziness, or NULL for `location_fuzziness_none`
 */
const char * location_fuzziness_to_string(enum location_fuzziness this_);

/**
 * @brief Creates an `enum location_ramps` from its string representation.
 *
 * @param string The string representation (case is ignored)
 *
 * @return The corresponding `enum location_ramps`, or `location_ramps_none` if `string` does
 * not match a known identifier
 */
enum location_ramps location_ramps_new(char * string);

/**
 * @brief Translates an `enum location_ramps` to its string representation.
 *
 * @return The string representation
 */
const char * location_ramps_to_string(enum location_ramps this_);

/**
 * @brief Creates a supplementary information class from its string representation.
 *
 * @param string The string representation (case is ignored)
 *
 * @return The corresponding `enum si_class`, or `si_class_invalid` if `string` does not match a
 * known identifier
 */
enum si_class si_class_new(char * string);

/**
 * @brief Translates a supplementary information class to its string representation.
 *
 * @return The string representation of the supplementary information class
 */
const char * si_class_to_string(enum si_class this_);

/**
 * @brief Creates a supplementary information type from its string representation.
 *
 * @param string The string representation (case is ignored)
 *
 * @return The corresponding `enum si_type`, or `si_invalid` if `string` does not match a known
 * identifier
 */
enum si_type si_type_new(char * string);

/**
 * @brief Translates a supplementary information type to its string representation.
 *
 * @return The string representation of the supplementary information type
 */
const char * si_type_to_string(enum si_type this_);

/**
 * @brief Creates a new `traffic_point`.
 *
 * It is the responsibility of the caller to destroy all references passed to this function. This can be
 * done immediately after the function returns.
 *
 * @param lon The longitude, as reported by the source, in GPS coordinates
 * @param lat The latitude, as reported by the source, in GPS coordinates
 * @param junction_name The name of the motorway junction this point refers to, NULL if not applicable
 * @param junction_ref The reference number of the motorway junction this point refers to, NULL if not applicable
 * @param tmc_id The TMC identifier of the point, if the location was obtained via TMC, or NULL if not applicable
 */
struct traffic_point * traffic_point_new(float lon, float lat, char * junction_name, char * junction_ref,
		char * tmc_id);

/**
 * @brief Creates a new `traffic_point`.
 *
 * This is the short version of the constructor, which sets only mandatory members. Other members can be
 * set after the instance is created.
 *
 * @param lon The longitude, as reported by the source, in GPS coordinates
 * @param lat The latitude, as reported by the source, in GPS coordinates
 */
struct traffic_point * traffic_point_new_short(float lon, float lat);

/**
 * @brief Destroys a `traffic_point`.
 *
 * This will release the memory used by the `traffic_point` and all related data.
 *
 * A `traffic_point` is usually destroyed together with its parent `traffic_location`, thus
 * it is usually not necessary to call this destructor directly.
 *
 * @param this_ The point
 */
void traffic_point_destroy(struct traffic_point * this_);

/**
 * @brief Creates a new `traffic_location`.
 *
 * The `traffic_point` instances are destroyed when the `traffic_location` is destroyed, and
 * therefore cannot be shared between multiple `traffic_location` instances.
 *
 * It is the responsibility of the caller to destroy all other references passed to this function. This
 * can be done immediately after the function returns.
 *
 * If `at` is non-NULL, the location is a point location, and `from` and `to` are
 * interpreted as auxiliary locations.
 *
 * Of `from` and `to`, one is mandatory for a unidirectional point location; both are
 * mandatory for a linear location.
 *
 * `ramps` is mainly intended for compatibility with TMC, where junctions with all their ramps are
 * represented by a single point. Other sources should use coordinate pairs instead.
 *
 * @param at The coordinates for a point location, NULL for a linear location
 * @param from The start of a linear location, or a point before `at`
 * @param to The end of a linear location, or a point after `at`
 * @param via A point between `from` and `to`, needed only on ring roads
 * @param not_via A point not between `from` and `to`, needed only on ring roads
 * @param destination A destination, preferably the one given on road signs, indicating that the message
 * applies only to traffic going in that direction; can be NULL, do not use for bidirectional locations
 * @param direction A compass direction indicating the direction of travel which this location refers to;
 * can be NULL, do not use where ambiguous
 * @param directionality Whether the location is unidirectional or bidirectional
 * @param fuzziness A precision indicator for `from` and `to`
 * @param ramps Whether the main carriageway or the ramps are affected
 * @param road_type The importance of the road within the road network, must be a road item type,
 * `type_line_unspecified` if not known or not consistent
 * @param road_name A road name, if consistent throughout the location; NULL if not known or inconsistent
 * @param road_ref A road number, if consistent throughout the location; NULL if not known or inconsistent
 * @param tmc_table For messages received via TMC, the CID and LTN; NULL otherwise
 * @param tmc_direction For messages received via TMC, the direction of the road; ignored for
 * bidirectional or non-TMC messages
 */
// TODO split CID/LTN?
struct traffic_location * traffic_location_new(struct traffic_point * at, struct traffic_point * from,
		struct traffic_point * to, struct traffic_point * via, struct traffic_point * not_via,
		char * destination, char * direction, enum location_dir directionality,
		enum location_fuzziness fuzziness, enum location_ramps ramps, enum item_type road_type,
		char * road_name, char * road_ref, char * tmc_table, int tmc_direction);

/**
 * @brief Creates a new `traffic_location`.
 *
 * This is the short version of the constructor, which sets only mandatory members. Other members can be
 * set after the instance is created.
 *
 * The `traffic_point` instances are destroyed when the `traffic_location` is destroyed, and
 * therefore cannot be shared between multiple `traffic_location` instances.
 *
 * If `at` is non-NULL, the location is a point location, and `from` and `to` are
 * interpreted as auxiliary locations.
 *
 * Of `from` and `to`, one is mandatory for a unidirectional point location; both are
 * mandatory for a linear location.
 *
 * @param at The coordinates for a point location, NULL for a linear location
 * @param from The start of a linear location, or a point before `at`
 * @param to The end of a linear location, or a point after `at`
 * @param via A point between `from` and `to`, needed only on ring roads
 * @param not_via A point not between `from` and `to`, needed only on ring roads
 * @param directionality Whether the location is unidirectional or bidirectional
 * @param fuzziness A precision indicator for `from` and `to`
 */
struct traffic_location * traffic_location_new_short(struct traffic_point * at, struct traffic_point * from,
		struct traffic_point * to, struct traffic_point * via, struct traffic_point * not_via,
		enum location_dir directionality, enum location_fuzziness fuzziness);

/**
 * @brief Destroys a `traffic_location`.
 *
 * This will release the memory used by the `traffic_location` and all related data.
 *
 * A `traffic_location` is usually destroyed together with its parent `traffic_message`, thus
 * it is usually not necessary to call this destructor directly.
 *
 * @param this_ The location
 */
void traffic_location_destroy(struct traffic_location * this_);

/**
 * @brief Creates a new `traffic_suppl_info`.
 *
 * It is the responsibility of the caller to destroy all references passed to this function. This can be
 * done immediately after the function returns.
 *
 * @param si_class The supplementary information class (generic category)
 * @param type The supplementary information type, which can be mapped to a string to be displayed to
 * the user
 * @param quantifier Additional quantifier for supplementary information types allowing this, or NULL
 */
struct traffic_suppl_info * traffic_suppl_info_new(enum si_class si_class, enum si_type type,
		struct quantifier * quantifier);

/**
 * @brief Destroys a `traffic_suppl_info`.
 *
 * This will release the memory used by the `traffic_suppl_info` and all related data.
 *
 * A `traffic_suppl_info` is usually destroyed together with its parent `traffic_event`, thus
 * it is usually not necessary to call this destructor directly.
 *
 * @param this_ The supplementary information item
 */
void traffic_suppl_info_destroy(struct traffic_suppl_info * this_);

/**
 * @brief Creates a new `traffic_event`.
 *
 * The `traffic_suppl_info` instances are destroyed when the `traffic_event` is destroyed, and
 * therefore cannot be shared between multiple `traffic_event` instances.
 *
 * It is the responsibility of the caller to destroy all other references passed to this function
 * (including the `si` buffer but not the `traffic_suppl_info` instances). This can be done
 * immediately after the function returns.
 *
 * @param event_class The event class (generic category)
 * @param type The event type, which can be mapped to a string to be displayed to the user
 * @param length The length of the affected route in meters, -1 if not known
 * @param speed The speed in km/h at which vehicles can expect to pass through the affected stretch of
 * road (either a temporary speed limit or average speed in practice, whichever is less); INT_MAX if unknown
 * @param quantifier Additional quantifier for supplementary information types allowing this, or NULL
 * @param si_count Number of supplementary information items in `si_count`
 * @param si Points to an array of pointers to supplementary information items
 */
struct traffic_event * traffic_event_new(enum event_class event_class, enum event_type type,
		int length, int speed, struct quantifier * quantifier, int si_count, struct traffic_suppl_info ** si);

/**
 * @brief Creates a new `traffic_event`.
 *
 * This is the short version of the constructor, which sets only mandatory members. Other members can be
 * set after the instance is created.
 *
 * @param event_class The event class (generic category)
 * @param type The event type, which can be mapped to a string to be displayed to the user
 */
struct traffic_event * traffic_event_new_short(enum event_class event_class, enum event_type type);

/**
 * @brief Destroys a `traffic_event`.
 *
 * This will release the memory used by the `traffic_event` and all related data.
 *
 * A `traffic_event` is usually destroyed together with its parent `traffic_message`, thus
 * it is usually not necessary to call this destructor directly.
 *
 * @param this_ The event
 */
void traffic_event_destroy(struct traffic_event * this_);

/**
 * @brief Adds a supplementary information item to an event.
 *
 * The `traffic_suppl_info` instance is destroyed when the `traffic_event` is destroyed, and
 * therefore cannot be shared between multiple `traffic_event` instances.
 *
 * @param this_ The event
 * @param si The supplementary information item
 */
void traffic_event_add_suppl_info(struct traffic_event * this_, struct traffic_suppl_info * si);

/**
 * @brief Retrieves a supplementary information item associated with an event.
 *
 * @param this_ The event
 * @param index The index of the supplementary information item, zero-based
 * @return The supplementary information item at the specified position, or NULL if out of bounds
 */
struct traffic_suppl_info * traffic_event_get_suppl_info(struct traffic_event * this_, int index);

/**
 * @brief Creates a new `traffic_message`.
 *
 * The `traffic_event` and `traffic_location` instances are destroyed when the
 * `traffic_message` is destroyed, and therefore cannot be shared between multiple
 * `traffic_message` instances.
 *
 * It is the responsibility of the caller to destroy all other references passed to this function
 * (including the `events` buffer but not the `traffic_event` instances). This can be done
 * immediately after the function returns.
 *
 * @param id The message identifier; existing messages with the same identifier will be replaced by the
 * new message
 * @param receive_time When the message was first received by the source, should be kept stable across
 * all updates
 * @param update_time When the last update to this message was received by the source
 * @param expiration_time How long the message should be considered valid
 * @param start_time When the condition is expected to begin (optional, 0 if not set)
 * @param end_time How long the condition is expected to last (optional, 0 if not set)
 * @param isCancellation If true, create a cancellation message (existing messages with the same ID
 * should be deleted or no longer considered current, and all other attributes ignored)
 * @param isForecast If false, the message describes a current situation; if true, it describes an
 * expected situation in the future
 * @param replaced_count The number of entries in `replaces`
 * @param replaces Points to an array of identifiers of messages which the current message replaces
 * @param location The location to which this message refers
 * @param event_count The number of events in `events`
 * @param events Points to an array of pointers to the events for this message
 */
struct traffic_message * traffic_message_new(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, time_t start_time, time_t end_time, int is_cancellation, int is_Forecast,
		int replaced_count, char ** replaces, struct traffic_location * location, int event_count,
		struct traffic_event ** events);

/**
 * @brief Creates a new `traffic_message`.
 *
 * This is the short version of the constructor, which sets only mandatory members. Other members can be
 * set after the instance is created.
 *
 * The `traffic_event` and `traffic_location` instances are destroyed when the
 * `traffic_message` is destroyed, and therefore cannot be shared between multiple
 * `traffic_message` instances.
 *
 * It is the responsibility of the caller to destroy all other references passed to this function
 * (including the `events` buffer but not the `traffic_event` instances). This can be done
 * immediately after the function returns.
 *
 * @param id The message identifier; existing messages with the same identifier will be replaced by the
 * new message
 * @param receive_time When the message was first received by the source, should be kept stable across
 * all updates
 * @param update_time When the last update to this message was received by the source
 * @param expiration_time How long the message should be considered valid
 * @param is_forecast If false, the message describes a current situation; if true, it describes an
 * expected situation in the future
 * @param location The location to which this message refers
 * @param event_count The number of events in `events`
 * @param events Points to an array of pointers to the events for this message
 */
struct traffic_message * traffic_message_new_short(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, int is_forecast, struct traffic_location * location,
		int event_count, struct traffic_event ** events);

/**
 * @brief Creates a new single-event `traffic_message`.
 *
 * This is a convenience constructor, which sets only mandatory members. Other members can be
 * set after the instance is created.
 *
 * The `traffic_location` instances are destroyed when the `traffic_message` is destroyed,
 * and therefore cannot be shared between multiple `traffic_message` instances.
 *
 * It is the responsibility of the caller to destroy all other references passed to this function. This
 * can be done immediately after the function returns.
 *
 * @param id The message identifier; existing messages with the same identifier will be replaced by the
 * new message
 * @param receive_time When the message was first received by the source, should be kept stable across
 * all updates
 * @param update_time When the last update to this message was received by the source
 * @param expiration_time How long the message should be considered valid
 * @param is_forecast If false, the message describes a current situation; if true, it describes an
 * expected situation in the future
 * @param location The location to which this message refers
 * @param event_class The event class (generic category)
 * @param type The event type, which can be mapped to a string to be displayed to the user
 */
struct traffic_message * traffic_message_new_single_event(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, int is_forecast, struct traffic_location * location,
		enum event_class event_class, enum event_type type);

/**
 * @brief Creates a new cancellation `traffic_message`.
 *
 * This is a convenience constructor, which creates a cancellation message, without the need to supply
 * members which are not required for cancellation messages. Upon receiving a cancellation message,
 * existing messages with the same ID should be deleted or no longer considered current, and all other
 * attributes ignored.
 *
 * The `traffic_location` instances are destroyed when the `traffic_message` is destroyed,
 * and therefore cannot be shared between multiple `traffic_message` instances.
 *
 * It is the responsibility of the caller to destroy all other references passed to this function. This
 * can be done immediately after the function returns.
 *
 * @param id The message identifier; existing messages with the same identifier will be replaced by the
 * new message
 * @param receive_time When the message was first received by the source, should be kept stable across
 * all updates
 * @param update_time When the last update to this message was received by the source
 * @param expiration_time How long the message should be considered valid
 * @param location The location to which this message refers
 */
struct traffic_message * traffic_message_new_cancellation(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, struct traffic_location * location);

/**
 * @brief Destroys a `traffic_message`.
 *
 * This will release the memory used by the `traffic_message` and all related data.
 *
 * A `traffic_message` is usually destroyed by the traffic plugin, thus it is usually not
 * necessary to call this destructor directly.
 *
 * @param this_ The message
 */
void traffic_message_destroy(struct traffic_message * this_);

/**
 * @brief Adds an event to a message.
 *
 * The `traffic_event` instance is destroyed when the `traffic_message` is destroyed, and
 * therefore cannot be shared between multiple `traffic_message` instances.
 *
 * @param this_ The message
 * @param event The event to add to this message
 */
void traffic_message_add_event(struct traffic_message * this_, struct traffic_event * event);

/**
 * @brief Retrieves an event associated with a message.
 *
 * @param this_ The message
 * @param index The index of the event, zero-based
 * @return The event at the specified position, or NULL if out of bounds
 */
struct traffic_event * traffic_message_get_event(struct traffic_message * this_, int index);

/**
 * @brief Returns the items associated with a message.
 *
 * Note that no map rectangle is required to obtain traffic items. This behavior is particular to traffic items, which
 * do not rely on a map rectangle. Items obtained from other maps may behave differently.
 *
 * @param this_ The message
 *
 * @return Items as a NULL-terminated array. The caller is responsible for freeing the array (not its elements) when it
 * is no longer needed. This method will always return a valid pointer—if no items are associated with the message, an
 * empty array (with just one single NULL element) will be returned. No particular order is guaranteed for the items.
 */
struct item ** traffic_message_get_items(struct traffic_message * this_);

/**
 * @brief Initializes the traffic plugin.
 *
 * This function is called once on startup.
 */
void traffic_init(void);

/**
 * @brief Reads previously stored traffic messages from an XML file.
 *
 * @param this_ The traffic instance
 * @param filename The full path to the XML file to parse
 *
 * @return A `NULL`-terminated pointer array. Each element points to one `struct traffic_message`.
 * `NULL` is returned (rather than an empty pointer array) if there are no messages to report.
 */
struct traffic_message ** traffic_get_messages_from_xml_file(struct traffic * this_, char * filename);

/**
 * @brief Reads traffic messages from an XML string.
 *
 * @param this_ The traffic instance
 * @param filename The XML document to parse, as a string
 *
 * @return A `NULL`-terminated pointer array. Each element points to one `struct traffic_message`.
 * `NULL` is returned (rather than an empty pointer array) if there are no messages to report.
 */
struct traffic_message ** traffic_get_messages_from_xml_string(struct traffic * this_, char * xml);

/**
 * @brief Returns the map for the traffic plugin.
 *
 * The map is created by the first traffic plugin loaded. If multiple traffic plugin instances are
 * active at the same time, they share the map created by the first instance.
 *
 * @param this_ The traffic plugin instance
 *
 * @return The traffic map
 */
struct map * traffic_get_map(struct traffic *this_);

/**
 * @brief Returns currently active traffic messages.
 *
 * If multiple plugin instances are active, this will give the same result for any plugin, as traffic messages are
 * shared between instances.
 *
 * @param this_ The traffic plugin instance
 *
 * @return A null-terminated array of traffic messages. The caller is responsible for freeing the array (not its
 * elements) when it is no longer needed. This method will always return a valid pointer—if the message store is empty,
 * an empty array (with just one single NULL element) will be returned.
 */
struct traffic_message ** traffic_get_stored_messages(struct traffic *this_);

/**
 * @brief Processes new traffic messages.
 *
 * Calling this method delivers new messages in a “push” manner (as opposed to the “pull” fashion of
 * calling a plugin method).
 *
 * Messages which are past their expiration timestamp are skipped, and the flags in the return value
 * are set only if at least one valid message is found.
 *
 * @param this_ The traffic instance
 * @param messages The new messages
 */
void traffic_process_messages(struct traffic * this_, struct traffic_message ** messages);

/**
 * @brief Sets the mapset for the traffic plugin.
 *
 * This sets the mapset from which the segments affected by a traffic report will be retrieved.
 *
 * @param this_ The traffic plugin instance
 * @param ms The mapset
 */
void traffic_set_mapset(struct traffic *this_, struct mapset *ms);

/**
 * @brief Sets the route for the traffic plugin.
 *
 * This sets the route which may get notified by the traffic plugin if traffic distortions change.
 */
void traffic_set_route(struct traffic *this_, struct route *rt);

/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
