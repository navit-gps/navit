/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
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

#ifndef NAVIT_APRS_DECODE_H
#define NAVIT_APRS_DECODE_H

#include "aprs_db.h"

struct aprs_packet {
    char *source_callsign;
    char *destination_callsign;
    char **path;
    int path_count;
    char *information_field;
    int information_length;
};

struct aprs_position {
    struct coord_geo position;
    double altitude;
    int course;
    int speed;
    char symbol_table;
    char symbol_code;
    char *comment;
    int has_position;
    int has_altitude;
    int has_course_speed;
};

int aprs_decode_ax25(const unsigned char *data, int length, struct aprs_packet *packet);
int aprs_parse_position(const char *info_field, int length, struct aprs_position *pos);
int aprs_parse_timestamp(const char *info_field, int length, time_t *timestamp);

void aprs_packet_free(struct aprs_packet *packet);
void aprs_position_free(struct aprs_position *pos);

#endif

