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

#ifndef NAVIT_APRS_NMEA_H
#define NAVIT_APRS_NMEA_H

#include "aprs_db.h"
#include "config.h"
#include "coord.h"

struct aprs_nmea_config {
    char *device;  /* Serial port device path */
    int baud_rate; /* Baud rate (4800, 9600, 19200, 38400, etc.) */
    char parity;   /* 'N' = none, 'E' = even, 'O' = odd */
    int data_bits; /* Usually 8 */
    int stop_bits; /* Usually 1 */
};

struct aprs_nmea;

struct aprs_nmea *aprs_nmea_new(const struct aprs_nmea_config *config);
void aprs_nmea_destroy(struct aprs_nmea *nmea);

int aprs_nmea_start(struct aprs_nmea *nmea);
int aprs_nmea_stop(struct aprs_nmea *nmea);
int aprs_nmea_is_running(const struct aprs_nmea *nmea);

void aprs_nmea_set_callback(struct aprs_nmea *nmea, void (*callback)(void *data, struct aprs_station *station),
                            void *data);

/* Parse NMEA sentence and create station */
int aprs_nmea_parse_sentence(const char *sentence, struct aprs_station *station);

#endif /* NAVIT_APRS_NMEA_H */
