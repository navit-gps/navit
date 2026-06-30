/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Internal declarations shared between Driver Break database translation units.
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_DB_PRIV_H
#define NAVIT_PLUGIN_DRIVER_BREAK_DB_PRIV_H

#include "driver_break_db.h"
#include <sqlite3.h>

struct driver_break_db {
    sqlite3 *db;
};

struct config_key_map {
    const char *key;
    size_t offset;
    int min_value;
    int max_value;
    int allow_zero;
};

#define MAX_HOURS_PER_DAY 12
#define MAX_BREAK_DURATION_MIN 120
#define MAX_VEHICLE_TYPE 3
#define SQLITE_COLUMN_INDEX_WAS_MANDATORY 5
#define MAX_DISTANCE_METERS 10000
#define MAX_POI_SEARCH_RADIUS_KM 1000
#define MAX_POI_WATER_CABIN_RADIUS_KM 100
#define MAX_HOURS_PER_DAY_24 24
#define MAX_CONFIG_VALUE 1000000
#define DECIMAL_BASE 10

int driver_break_db_set_config_value(const struct config_key_map *map, struct driver_break_config *config, int value,
                                     int *loaded_count);
int driver_break_db_load_config_value(const char *key, int value, struct driver_break_config *config,
                                      int *loaded_count);
void driver_break_db_clean_corrupted_config(struct driver_break_db *db);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_DB_PRIV_H */
