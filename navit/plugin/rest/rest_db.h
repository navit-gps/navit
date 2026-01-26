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

#ifndef NAVIT_PLUGIN_REST_DB_H
#define NAVIT_PLUGIN_REST_DB_H

#include "config.h"
#include "coord.h"
#include "rest.h"

struct rest_db;

/* Database operations */
struct rest_db *rest_db_new(const char *db_path);
void rest_db_destroy(struct rest_db *db);

/* Rest stop history */
int rest_db_add_history(struct rest_db *db, struct rest_stop_history *entry);
GList *rest_db_get_history(struct rest_db *db, time_t since);
int rest_db_clear_history(struct rest_db *db, time_t before);

/* Configuration persistence */
int rest_db_save_config(struct rest_db *db, struct rest_config *config);
int rest_db_load_config(struct rest_db *db, struct rest_config *config);

#endif /* NAVIT_PLUGIN_REST_DB_H */
