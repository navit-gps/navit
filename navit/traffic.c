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

/** @file
 * @brief Contains code related to processing traffic messages into map items.
 *
 * Currently the only map items supported are traffic distortions. More may be added in the future.
 *
 * Traffic distortions are used by Navit to route around traffic problems.
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
#include "debug.h"

struct traffic_point * traffic_point_new(float lon, float lat, char * junction_name, char * junction_ref,
		char * tmc_id) {
	struct traffic_point * ret;

	ret = g_new0(struct traffic_point, 1);
	ret->coord.lat = lat;
	ret->coord.lng = lon;
	ret->map_coord = NULL;
	ret->map_coord_backward = NULL;
	ret->junction_name = junction_name ? g_strdup(junction_name) : NULL;
	ret->junction_ref = junction_ref ? g_strdup(junction_ref) : NULL;
	ret->tmc_id = tmc_id ? g_strdup(tmc_id) : NULL;
	return ret;
}

struct traffic_point * traffic_point_new_short(float lon, float lat) {
	return traffic_point_new(lon, lat, NULL, NULL, NULL);
}

void traffic_point_destroy(struct traffic_point * this_) {
	if (this_->map_coord)
		g_free(this_->map_coord);
	if (this_->map_coord_backward)
		g_free(this_->map_coord_backward);
	if (this_->junction_name)
		g_free(this_->junction_name);
	if (this_->junction_ref)
		g_free(this_->junction_ref);
	if (this_->tmc_id)
		g_free(this_->tmc_id);
	g_free(this_);
}

// TODO split CID/LTN?
struct traffic_location * traffic_location_new(struct traffic_point * at, struct traffic_point * from,
		struct traffic_point * to, char * destination, char * direction, enum location_dir directionality,
		enum location_fuzziness fuzziness, enum location_ramps ramps, enum item_type road_type,
		char * road_name, char * road_ref, char * tmc_table, int tmc_direction) {
	struct traffic_location * ret;

	ret = g_new0(struct traffic_location, 1);
	ret->at = at;
	ret->from = from;
	ret->to = to;
	ret->destination = destination ? g_strdup(destination) : NULL;
	ret->direction = direction ? g_strdup(direction) : NULL;
	ret->directionality = directionality;
	ret->fuzziness = fuzziness;
	ret->ramps = ramps;
	ret->road_type = road_type;
	ret->road_name = road_name ? g_strdup(road_name) : NULL;
	ret->road_ref = road_ref ? g_strdup(road_ref) : NULL;
	ret->tmc_table = tmc_table ? g_strdup(tmc_table) : NULL;
	ret->tmc_direction = tmc_direction;
	return ret;
}

struct traffic_location * traffic_location_new_short(struct traffic_point * at, struct traffic_point * from,
		struct traffic_point * to, enum location_dir directionality, enum location_fuzziness fuzziness) {
	return traffic_location_new(at, from, to, NULL, NULL, directionality, fuzziness, location_ramps_none,
			type_line_unspecified, NULL, NULL, NULL, 0);
}

void traffic_location_destroy(struct traffic_location * this_) {
	if (this_->at)
		traffic_point_destroy(this_->at);
	if (this_->from)
		traffic_point_destroy(this_->from);
	if (this_->to)
		traffic_point_destroy(this_->to);
	if (this_->destination)
		g_free(this_->destination);
	if (this_->direction)
		g_free(this_->direction);
	if (this_->road_name)
		g_free(this_->road_name);
	if (this_->road_ref)
		g_free(this_->road_ref);
	if (this_->tmc_table)
		g_free(this_->tmc_table);
	g_free(this_);
}

struct traffic_suppl_info * traffic_suppl_info_new(enum si_class si_class, enum si_type type,
		struct quantifier * quantifier) {
	struct traffic_suppl_info * ret;
	ret = g_new0(struct traffic_suppl_info, 1);
	ret->si_class = si_class;
	ret->type = type;
	ret->quantifier = quantifier ? g_memdup(quantifier, sizeof(struct quantifier)) : NULL;
	return ret;
}

void traffic_suppl_info_destroy(struct traffic_suppl_info * this_) {
	if (this_->quantifier)
		g_free(this_->quantifier);
	g_free(this_);
}

struct traffic_event * traffic_event_new(enum event_class event_class, enum event_type type,
		int length, int speed, struct quantifier * quantifier, int si_count, struct traffic_suppl_info ** si) {
	struct traffic_event * ret;

	ret = g_new0(struct traffic_event, 1);
	ret->event_class = event_class;
	ret->type = type;
	ret->length = length;
	ret->speed = speed;
	ret->quantifier = quantifier ? g_memdup(quantifier, sizeof(struct quantifier)) : NULL;
	if (si_count && si) {
		ret->si_count = si_count;
		ret->si = g_memdup(si, sizeof(struct traffic_suppl_info *) * si_count);
	} else {
		ret->si_count = 0;
		ret->si = NULL;
	}
	return ret;
}

struct traffic_event * traffic_event_new_short(enum event_class event_class, enum event_type type) {
	return traffic_event_new(event_class, type, -1, INT_MAX, NULL, 0, NULL);
}

void traffic_event_destroy(struct traffic_event * this_) {
	int i;

	if (this_->quantifier)
		g_free(this_->quantifier);
	if (this_->si) {
		for (i = 0; i < this_->si_count; i++)
			traffic_suppl_info_destroy(this_->si[i]);
		g_free(this_->si);
	}
	g_free(this_);
}

void traffic_event_add_suppl_info(struct traffic_event * this_, struct traffic_suppl_info * si) {
	struct traffic_suppl_info ** si_new;

	if (this_->si_count && this_->si) {
		si_new = g_new0(struct traffic_suppl_info *, this_->si_count + 1);
		memcpy(si_new, this_->si, sizeof(struct traffic_suppl_info *) * this_->si_count);
		si_new[this_->si_count] = si;
		g_free(this_->si);
		this_->si = si_new;
		this_->si_count++;
	} else {
		this_->si = g_new0(struct traffic_suppl_info *, 1);
		this_->si[0] = si;
		this_->si_count = 1;
	}
}

struct traffic_suppl_info * traffic_event_get_suppl_info(struct traffic_event * this_, int index) {
	if (this_->si && (index < this_->si_count))
		return this_->si[index];
	else
		return NULL;
}

struct traffic_message * traffic_message_new(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, time_t start_time, time_t end_time, int is_cancellation, int is_forecast,
		int replaced_count, char ** replaces, struct traffic_location * location, int event_count,
		struct traffic_event ** events) {
	struct traffic_message * ret;

	ret = g_new0(struct traffic_message, 1);
	ret->id = g_strdup(id);
	ret->receive_time = receive_time;
	ret->update_time = update_time;
	ret->expiration_time = expiration_time;
	ret->start_time = start_time;
	ret->end_time = end_time;
	ret->is_cancellation = is_cancellation;
	ret->is_forecast = is_forecast;
	if (replaced_count && replaces) {
		ret->replaced_count = replaced_count;
		ret->replaces = g_memdup(replaces, sizeof(char *) * replaced_count);
	} else {
		ret->replaced_count = 0;
		ret->replaces = NULL;
	}
	ret->location = location;
	ret->event_count = event_count;
	ret->events = g_memdup(events, sizeof(struct traffic_event *) * event_count);
	return ret;
}

struct traffic_message * traffic_message_new_short(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, int is_cancellation, int is_forecast, struct traffic_location * location,
		int event_count, struct traffic_event ** events) {
	return traffic_message_new(id, receive_time, update_time, expiration_time, 0, 0, is_cancellation,
			is_forecast, 0, NULL, location, event_count, events);
}

struct traffic_message * traffic_message_new_single_event(char * id, time_t receive_time, time_t update_time,
		time_t expiration_time, int is_cancellation, int is_forecast, struct traffic_location * location,
		enum event_class event_class, enum event_type type) {
	struct traffic_event * event;
	struct traffic_event ** events;

	event = traffic_event_new_short(event_class, type);
	events = g_new0(struct traffic_event *, 1);
	events[0] = event;
	return traffic_message_new_short(id, receive_time, update_time, expiration_time, is_cancellation,
			is_forecast, location, 1, events);
	g_free(events);
}

void traffic_message_destroy(struct traffic_message * this_) {
	int i;

	g_free(this_->id);
	if (this_->replaces) {
		for (i = 0; i < this_->replaced_count; i++)
			g_free(this_->replaces[i]);
		g_free(this_->replaces);
	}
	traffic_location_destroy(this_->location);
	for (i = 0; i < this_->event_count; i++)
		traffic_event_destroy(this_->events[i]);
	g_free(this_->events);
}

void traffic_message_add_event(struct traffic_message * this_, struct traffic_event * event) {
	struct traffic_event ** events_new;

	events_new = g_new0(struct traffic_event *, this_->event_count + 1);
	memcpy(events_new, this_->events, sizeof(struct traffic_event *) * this_->event_count);
	events_new[this_->event_count] = event;
	g_free(this_->events);
	this_->events = events_new;
	this_->event_count++;
}

struct traffic_event * traffic_message_get_event(struct traffic_message * this_, int index) {
	if (this_->events && (index < this_->event_count))
		return this_->events[index];
	else
		return NULL;
}

int traffic_report_messages(int message_count, struct traffic_message ** messages) {
	// TODO
    dbg(lvl_error, "MESSAGE RECEIVED, don't know how to process it yet...\n");
	return 0;
}

void traffic_init(void) {
        dbg(lvl_error, "THIS IS TRAFFIC CORE. Startup successful, more to come soon...\n");
}
