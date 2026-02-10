/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024 Navit Team
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

#ifndef NAVIT_APRS_DB_H
#define NAVIT_APRS_DB_H

#include "coord.h"
#include <sqlite3.h>

struct aprs_station {
    char *callsign;
    struct coord_geo position;
    double altitude;
    int course;
    int speed;
    time_t timestamp;
    char *comment;
    char symbol_table;
    char symbol_code;
    int path_count;
    char **path;
};

struct aprs_db;

struct aprs_db *aprs_db_new(const char *db_path);
void aprs_db_destroy(struct aprs_db *db);

int aprs_db_update_station(struct aprs_db *db, struct aprs_station *station);
int aprs_db_get_station(struct aprs_db *db, const char *callsign, struct aprs_station *station);
int aprs_db_delete_station(struct aprs_db *db, const char *callsign);
int aprs_db_delete_expired(struct aprs_db *db, time_t expire_seconds);
int aprs_db_get_stations_in_range(struct aprs_db *db, const struct coord_geo *center, double range_km,
                                   GList **stations);
int aprs_db_get_all_stations(struct aprs_db *db, GList **stations);

void aprs_station_free(struct aprs_station *station);
struct aprs_station *aprs_station_new(void);

#endif
