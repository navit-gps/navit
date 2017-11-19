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
 * The traffic plugin interacts with the Navit core and converts traffic messages into traffic
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
 * As a rule, responsibility for freeing up any {@code traffic_*} instances normally lies with the
 * traffic plugin, which frees messages as they expire or are replaced. Since this also frees all child
 * data structures, traffic backends will seldom need to call any of the destructors. The only case in
 * which this would be necessary is if a backend has instantiated an object which is not going to be
 * used (i.e. attached to a parent object or, in the case of {@code traffic_message}, reported to the
 * traffic plugin: these need to be freed up manually by calling the destructor of the topmost object in
 * the hierarchy.
 *
 * Any other references passed in functions (including pointer arrays and {@code quantifier} instances)
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
enum event_class {
	event_class_congestion,  /*!< Traffic congestion, typically indicating the approximate speed */
	event_class_delay,       /*!< Delays, typically indicating the amount of extra waiting time */
	event_class_restriction, /*!< Temporary traffic restrictions, such as road or lane closures or size,
	                          *   weight or access restrictions */
};

/**
 * @brief Event types.
 */
enum event_type {
	event_congestion_cleared,                          /*!< Traffic congestion cleared */
	event_congestion_forecast_withdrawn,               /*!< Traffic congestion forecast withdrawn */
	event_congestion_heavy_traffic,                    /*!< Heavy traffic with average speeds of {@code speed} */
	event_congestion_long_queue,                       /*!< Long queues with average speeds of {@code speed} */
	event_congestion_none,                             /*!< No problems to report */
	event_congestion_normal_traffic,                   /*!< Traffic has returned to normal */
	event_congestion_queue,                            /*!< Queuing traffic with average speeds of {@code speed} */
	event_congestion_queue_likely,                     /*!< Danger of queuing traffic with average speeds
	                                                    *   of {@code speed} */
	event_congestion_slow_traffic,                     /*!< Slow traffic with average speeds of {@code speed} */
	event_congestion_stationary_traffic,               /*!< Stationary traffic (frequent standstills) */
	event_congestion_stationary_traffic_likely,        /*!< Danger of stationary traffic */
	event_congestion_traffic_building_up,              /*!< Traffic building up with average speeds of
	                                                    *   {@code speed} */
	event_congestion_traffic_congestion,               /*!< Traffic congestion with average speeds of
	                                                    *   {@code speed} */
	event_congestion_traffic_easing,                   /*!< Traffic easing */
	event_congestion_traffic_flowing_freely,           /*!< Traffic flowing freely with average speeds
	                                                    *   of {@code speed} */
	event_congestion_traffic_heavier_than_normal,      /*!< Traffic heavier than normal with average
	                                                    *   speeds of {@code speed} */
	event_congestion_traffic_lighter_than_normal,      /*!< Traffic lighter than normal with average
	                                                    *   speeds of {@code speed} */
	event_congestion_traffic_much_heavier_than_normal, /*!< Traffic very much heavier than normal with
	                                                    *   average speeds of {@code speed} (increased density
	                                                    *   but no significant decrease in speed) */
	event_congestion_traffic_problem,                  /*!< Traffic problem */
	event_delay_clearance,                             /*!< Delays cleared */
	event_delay_delay,                                 /*!< Delays up to {@code q_timespan} */
	event_delay_delay_possible,                        /*!< Delays up to {@code q_timespan} possible */
	event_delay_forecast_withdrawn,                    /*!< Delay forecast withdrawn */
	event_delay_long_delay,                            /*!< Long delays up to {@code q_timespan} */
	event_delay_several_hours,                         /*!< Delays of several hours */
	event_delay_uncertain_duration,                    /*!< Delays of uncertain duration */
	event_delay_very_long_delay,                       /*!< Very long delays up to {@code q_timespan} */
	event_restriction_access_restrictions_lifted,      /*!< Traffic restrictions lifted: reopened for all
                                                        *   traffic, other restrictions (overtaking etc.)
                                                        *   remain in place */
	event_restriction_all_carriagewqays_cleared,       /*!< All carriageways cleared */
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
	event_restriction_closed,                          /*!< Closed until {@code q_time} (refers to the entire
	                                                    *   road; separate codes exist for closures of
	                                                    *   individual lanes or carriageways) */
	event_restriction_closed_ahead,                    /*!< Closed ahead (at a point beyond the indicated
	                                                    *   location) */
	event_restriction_entry_blocked,                   /*!< {@code q_int} th entry slip road blocked */
	event_restriction_entry_reopened,                  /*!< Entry reopened */
	event_restriction_exit_blocked,                    /*!< {@code q_int} th exit slip road blocked */
	event_restriction_exit_reopened,                   /*!< Exit reopened */
	event_restriction_intermittent_closures,           /*!< Intermittent short term closures */
	event_restriction_open,                            /*!< Open */
	event_restriction_ramp_blocked,                    /*!< Ramps blocked */
	event_restriction_ramp_closed,                     /*!< Ramps closed */
	event_restriction_ramp_reopened,                   /*!< Ramps reopened */
	event_restriction_reopened,                        /*!< Reopened */
	event_restriction_road_cleared,                    /*!< Road cleared */
	event_restriction_single_alternate_line_traffic,   /*!< Single alternate line traffic (because the
	                                                    *   affected stretch of road can only be used in
	                                                    *   one direction at a time, different from batch
	                                                    *   service) */
	event_restriction_speed_limit,                     /*!< Speed limit {@code speed} in force */
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
	si_class_place,    /*!< Qualifiers specifying the place(s) to which the event refers */
	si_class_tendency, /*!< Traffic density development */
	si_class_vehicle,  /*!< Specifies categories of vehicles to which the event applies */
};

/**
 * @brief Supplementary information types.
 */
enum si_type {
	si_place_bridge,              /*!< On bridges */
	si_place_ramp,                /*!< On ramps (entry/exit) */
	si_place_roadworks,           /*!< In the roadworks area */
	si_place_tunnel,              /*!< In tunnels */
	si_tendency_queue_decreasing, /*!< Traffic queue length decreasing (average rate in optional {@code q_speed}) */
	si_tendency_queue_increasing, /*!< Traffic queue length increasing (average rate in optional {@code q_speed}) */
	si_vehicle_all,               /*!< For all vehicles */
	si_vehicle_bus,               /*!< For buses only */
	si_vehicle_car,               /*!< For cars only */
	si_vehicle_car_with_caravan,  /*!< For cars with caravans only */
	si_vehicle_car_with_trailer,  /*!< For cars with trailers only */
	si_vehicle_hazmat,            /*!< For hazardous loads only */
	si_vehicle_hgv,               /*!< For heavy trucks only */
	si_vehicle_motor,             /*!< For all motor vehicles */
	si_vehicle_with_trailer,      /*!< For vehicles with trailers only */
};

// TODO do we need priv members for structs?

/**
 * @brief A point on the road.
 *
 * This can either be a point location or an endpoint of a linear location. It specifies a coordinate
 * pair and can optionally be supplemented with a junction name and/or number where applicable.
 */
struct traffic_point {
	struct coord_geo coord; /*!< The coordinates of this point, as supplied by the source. These may
	                         *   deviate somewhat from the coordinates on the map. */
	struct pcoord * map_coord; /*!< The coordinates of this point on the map, in forward direction.
	                         *   This is filled by the traffic plugin. */
	struct pcoord * map_coord_backward; /*!< The coordinates of this point on the map, in backward
	                         *   direction. This is filled by the traffic plugin. Always NULL for
	                         *   unidirectional locations. */
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
 * direction. Auxiliary points can be omitted if {@code tmc_table}, {@code tmc_direction} and
 * {@code at->tmc_id} are supplied. However, this will only work if the map has accurate TMC data for
 * the location, thus it is recommended to supply an auxiliary point nonetheless.
 *
 * The order of points is as a driver would encounter them, i.e. first {@code from}, then {@code at},
 * finally {@code to}.
 */
struct traffic_location {
	struct traffic_point * at;         /*!< The point for a point location, NULL for linear locations. */
	struct traffic_point * from;       /*!< The start of a linear location, or a point before {@code at}. */
	struct traffic_point * to;         /*!< The end of a linear location, or a point after {@code at}. */
	char * destination;                /*!< A destination, preferably the one given on road signs,
	                                    *   indicating that the message applies only to traffic going in
	                                    *   that direction. Do not use for bidirectional locations. */
	char * direction;                  /*!< A compass direction indicating the direction of travel which
	                                    *   this location refers to. Do not use where ambiguous. */
	enum location_dir directionality;  /*!< Indicates whether the message refers to one or both directions
	                                    *   of travel. */
	enum location_fuzziness fuzziness; /*!< Indicates how precisely the end points are known. */
	enum location_ramps ramps;         /*!< Any value other than {@code location_ramps_none} implies
	                                    *   that only the specified ramps are affected while the main
	                                    *   road is not. In that case, the {@code road*} fields refer to
	                                    *   the main road served by the ramp, not the ramp itself. This
	                                    *   is mainly intended for compatibility with TMC, where
	                                    *   junctions with all their ramps are represented by a single
	                                    *   point. Other sources should use coordinate pairs instead. */
	enum item_type road_type;          /*!< The importance of the road within the road network, must be a
	                                    *   road item type. Use {@code line_unspecified} if not known or
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
	                                  *   average speed in practice, whichever is less). */
	struct quantifier * quantifier;  /*!< Additional quantifier for events allowing this. Data type and
	                                  *   meaning depends on the event type. */
	int si_count;                    /*!< Number of supplementary information items in {@code si_count}. */
	struct traffic_suppl_info ** si; /*!< Points to an array of pointers to supplementary information items. */
};

/**
 * @brief A traffic message.
 *
 * A message is the atomic element of traffic information, referring to a particular condition at a
 * given location.
 *
 * If no updates are received for a message, it should be discarded after both {@code expiration_time}
 * and {@code end_time} (if specified) have elapsed.
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
	int replaced_count;         /*!< The number of entries in {@code replaces}. */
	char ** replaces;           /*!< Points to an array of identifiers of messages which the current
	                             *   message replaces. */
	struct traffic_location * location; /*!< The location to which this message refers. */
	int event_count;            /*!< The number of events in {@code events}. */
	struct traffic_event ** events; /*!< Points to an array of pointers to the events for this message. */
};

/**
 * @brief Creates a new {@code traffic_point}.
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
 * @brief Creates a new {@code traffic_point}.
 *
 * This is the short version of the constructor, which sets only mandatory members. Other members can be
 * set after the instance is created.
 *
 * @param lon The longitude, as reported by the source, in GPS coordinates
 * @param lat The latitude, as reported by the source, in GPS coordinates
 */
struct traffic_point * traffic_point_new_short(float lon, float lat);

/**
 * @brief Destroys a {@code traffic_point}.
 *
 * This will release the memory used by the {@code traffic_point} and all related data.
 *
 * A {@code traffic_point} is usually destroyed together with its parent {@code traffic_location}, thus
 * it is usually not necessary to call this destructor directly.
 *
 * @param this_ The point
 */
void traffic_point_destroy(struct traffic_point * this_);

/**
 * @brief Creates a new {@code traffic_location}.
 *
 * The {@code traffic_point} instances are destroyed when the {@code traffic_location} is destroyed, and
 * therefore cannot be shared between multiple {@code traffic_location} instances.
 *
 * It is the responsibility of the caller to destroy all other references passed to this function. This
 * can be done immediately after the function returns.
 *
 * If {@code at} is non-NULL, the location is a point location, and {@code from} and {@code to} are
 * interpreted as auxiliary locations.
 *
 * Of {@code from} and {@code to}, one is mandatory for a unidirectional point location; both are
 * mandatory for a linear location.
 *
 * {@code ramps} is mainly intended for compatibility with TMC, where junctions with all their ramps are
 * represented by a single point. Other sources should use coordinate pairs instead.
 *
 * @param at The coordinates for a point location, NULL for a linear location
 * @param from The start of a linear location, or a point before {@code at}
 * @param to The end of a linear location, or a point after {@code at}
 * @param destination A destination, preferably the one given on road signs, indicating that the message
 * applies only to traffic going in that direction; can be NULL, do not use for bidirectional locations
 * @param direction A compass direction indicating the direction of travel which this location refers to;
 * can be NULL, do not use where ambiguous
 * @param directionality Whether the location is unidirectional or bidirectional
 * @param fuzziness A precision indicator for {@code from} and {@code to}
 * @param ramps Whether the main carriageway or the ramps are affected
 * @param road_type The importance of the road within the road network, must be a road item type,
 * {@code type_line_unspecified} if not known or not consistent
 * @param road_name A road name, if consistent throughout the location; NULL if not known or inconsistent
 * @param road_ref A road number, if consistent throughout the location; NULL if not known or inconsistent
 * @param tmc_table For messages received via TMC, the CID and LTN; NULL otherwise
 * @param tmc_direction For messages received via TMC, the direction of the road; ignored for
 * bidirectional or non-TMC messages
 */
// TODO split CID/LTN?
struct traffic_location * traffic_location_new(struct traffic_point * at, struct traffic_point * from,
		struct traffic_point * to, char * destination, char * direction, enum location_dir directionality,
		enum location_fuzziness fuzziness, enum location_ramps ramps, enum item_type road_type,
		char * road_name, char * road_ref, char * tmc_table, int tmc_direction);

/**
 * @brief Creates a new {@code traffic_location}.
 *
 * This is the short version of the constructor, which sets only mandatory members. Other members can be
 * set after the instance is created.
 *
 * The {@code traffic_point} instances are destroyed when the {@code traffic_location} is destroyed, and
 * therefore cannot be shared between multiple {@code traffic_location} instances.
 *
 * If {@code at} is non-NULL, the location is a point location, and {@code from} and {@code to} are
 * interpreted as auxiliary locations.
 *
 * Of {@code from} and {@code to}, one is mandatory for a unidirectional point location; both are
 * mandatory for a linear location.
 *
 * @param at The coordinates for a point location, NULL for a linear location
 * @param from The start of a linear location, or a point before {@code at}
 * @param to The end of a linear location, or a point after {@code at}
 * @param directionality Whether the location is unidirectional or bidirectional
 * @param fuzziness A precision indicator for {@code from} and {@code to}
 */
struct traffic_location * traffic_location_new_short(struct traffic_point * at, struct traffic_point * from,
		struct traffic_point * to, enum location_dir directionality, enum location_fuzziness fuzziness);

/**
 * @brief Destroys a {@code traffic_location}.
 *
 * This will release the memory used by the {@code traffic_location} and all related data.
 *
 * A {@code traffic_location} is usually destroyed together with its parent {@code traffic_message}, thus
 * it is usually not necessary to call this destructor directly.
 *
 * @param this_ The location
 */
void traffic_location_destroy(struct traffic_location * this_);

/**
 * @brief Creates a new {@code traffic_suppl_info}.
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
 * @brief Destroys a {@code traffic_suppl_info}.
 *
 * This will release the memory used by the {@code traffic_suppl_info} and all related data.
 *
 * A {@code traffic_suppl_info} is usually destroyed together with its parent {@code traffic_event}, thus
 * it is usually not necessary to call this destructor directly.
 *
 * @param this_ The supplementary information item
 */
void traffic_suppl_info_destroy(struct traffic_suppl_info * this_);

/**
 * @brief Creates a new {@code traffic_event}.
 *
 * The {@code traffic_suppl_info} instances are destroyed when the {@code traffic_event} is destroyed, and
 * therefore cannot be shared between multiple {@code traffic_event} instances.
 *
 * It is the responsibility of the caller to destroy all other references passed to this function
 * (including the {@code si} buffer but not the {@code traffic_suppl_info} instances). This can be done
 * immediately after the function returns.
 *
 * @param event_class The event class (generic category)
 * @param type The event type, which can be mapped to a string to be displayed to the user
 * @param length The length of the affected route in meters, -1 if not known
 * @param speed The speed in km/h at which vehicles can expect to pass through the affected stretch of
 * road (either a temporary speed limit or average speed in practice, whichever is less); INT_MAX if unknown
 * @param quantifier Additional quantifier for supplementary information types allowing this, or NULL
 * @param si_count Number of supplementary information items in {@code si_count}
 * @param si Points to an array of pointers to supplementary information items
 */
struct traffic_event * traffic_event_new(enum event_class event_class, enum event_type type,
		int length, int speed, struct quantifier * quantifier, int si_count, struct traffic_suppl_info ** si);

/**
 * @brief Creates a new {@code traffic_event}.
 *
 * This is the short version of the constructor, which sets only mandatory members. Other members can be
 * set after the instance is created.
 *
 * @param event_class The event class (generic category)
 * @param type The event type, which can be mapped to a string to be displayed to the user
 */
struct traffic_event * traffic_event_new_short(enum event_class event_class, enum event_type type);

/**
 * @brief Destroys a {@code traffic_event}.
 *
 * This will release the memory used by the {@code traffic_event} and all related data.
 *
 * A {@code traffic_event} is usually destroyed together with its parent {@code traffic_message}, thus
 * it is usually not necessary to call this destructor directly.
 *
 * @param this_ The event
 */
void traffic_event_destroy(struct traffic_event * this_);

/**
 * @brief Adds a supplementary information item to an event.
 *
 * The {@code traffic_suppl_info} instance is destroyed when the {@code traffic_event} is destroyed, and
 * therefore cannot be shared between multiple {@code traffic_event} instances.
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
 * @brief Creates a new {@code traffic_message}.
 *
 * The {@code traffic_event} and {@code traffic_location} instances are destroyed when the
 * {@code traffic_message} is destroyed, and therefore cannot be shared between multiple
 * {@code traffic_message} instances.
 *
 * It is the responsibility of the caller to destroy all other references passed to this function
 * (including the {@code events} buffer but not the {@code traffic_event} instances). This can be done
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
 * @param replaced_count The number of entries in {@code replaces}
 * @param replaces Points to an array of identifiers of messages which the current message replaces
 * @param location The location to which this message refers
 * @param event_count The number of events in {@code events}
 * @param events Points to an array of pointers to the events for this message
 */
struct traffic_message * traffic_message_new(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, time_t start_time, time_t end_time, int is_cancellation, int is_Forecast,
		int replaced_count, char ** replaces, struct traffic_location * location, int event_count,
		struct traffic_event ** events);

/**
 * @brief Creates a new {@code traffic_message}.
 *
 * This is the short version of the constructor, which sets only mandatory members. Other members can be
 * set after the instance is created.
 *
 * The {@code traffic_event} and {@code traffic_location} instances are destroyed when the
 * {@code traffic_message} is destroyed, and therefore cannot be shared between multiple
 * {@code traffic_message} instances.
 *
 * It is the responsibility of the caller to destroy all other references passed to this function
 * (including the {@code events} buffer but not the {@code traffic_event} instances). This can be done
 * immediately after the function returns.
 *
 * @param id The message identifier; existing messages with the same identifier will be replaced by the
 * new message
 * @param receive_time When the message was first received by the source, should be kept stable across
 * all updates
 * @param update_time When the last update to this message was received by the source
 * @param expiration_time How long the message should be considered valid
 * @param is_cancellation If true, create a cancellation message (existing messages with the same ID
 * should be deleted or no longer considered current, and all other attributes ignored)
 * @param is_forecast If false, the message describes a current situation; if true, it describes an
 * expected situation in the future
 * @param location The location to which this message refers
 * @param event_count The number of events in {@code events}
 * @param events Points to an array of pointers to the events for this message
 */
struct traffic_message * traffic_message_new_short(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, int is_cancellation, int is_forecast, struct traffic_location * location,
		int event_count, struct traffic_event ** events);

/**
 * @brief Creates a new single-event {@code traffic_message}.
 *
 * This is a convenience constructor, which sets only mandatory members. Other members can be
 * set after the instance is created.
 *
 * The {{@code traffic_location} instances are destroyed when the {@code traffic_message} is destroyed,
 * and therefore cannot be shared between multiple {@code traffic_message} instances.
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
 * @param is_cancellation If true, create a cancellation message (existing messages with the same ID
 * should be deleted or no longer considered current, and all other attributes ignored)
 * @param is_forecast If false, the message describes a current situation; if true, it describes an
 * expected situation in the future
 * @param location The location to which this message refers
 * @param event_class The event class (generic category)
 * @param type The event type, which can be mapped to a string to be displayed to the user
 */
struct traffic_message * traffic_message_new_single_event(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, int is_cancellation, int is_forecast, struct traffic_location * location,
		enum event_class event_class, enum event_type type);

/**
 * @brief Destroys a {@code traffic_message}.
 *
 * This will release the memory used by the {@code traffic_message} and all related data.
 *
 * A {@code traffic_message} is usually destroyed by the traffic plugin, thus it is usually not
 * necessary to call this destructor directly.
 *
 * @param this_ The message
 */
void traffic_message_destroy(struct traffic_message * this_);

/**
 * @brief Adds an event to a message.
 *
 * The {@code traffic_event} instance is destroyed when the {@code traffic_message} is destroyed, and
 * therefore cannot be shared between multiple {@code traffic_message} instances.
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

// TODO function to report traffic message

// TODO plugin functions

/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
