/**
 * @file driver_break_db.h
 * @brief Driver Break plugin database API
 *
 * Database operations for storing rest stop history and configuration.
 * For detailed API documentation, see https://doxygen.navit-project.org/
 *
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

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_DB_H
#define NAVIT_PLUGIN_DRIVER_BREAK_DB_H

#include "config.h"
#include "coord.h"
#include "driver_break.h"

struct driver_break_db;

/**
 * @brief Create a new database connection
 * @param db_path Path to SQLite database file (NULL for in-memory)
 * @return Database handle or NULL on failure
 */
struct driver_break_db *driver_break_db_new(const char *db_path);

/**
 * @brief Close database connection and free resources
 * @param db Database handle
 */
void driver_break_db_destroy(struct driver_break_db *db);

/**
 * @brief Add a rest stop entry to history
 * @param db Database handle
 * @param entry History entry to add
 * @return 1 on success, 0 on failure
 */
int driver_break_db_add_history(struct driver_break_db *db, struct driver_break_stop_history *entry);

/**
 * @brief Retrieve history entries since specified timestamp
 * @param db Database handle
 * @param since Unix timestamp (entries after this time)
 * @return GList of struct driver_break_stop_history* (caller must free with g_list_free_full)
 */
GList *driver_break_db_get_history(struct driver_break_db *db, time_t since);

/**
 * @brief Remove history entries older than specified timestamp
 * @param db Database handle
 * @param before Unix timestamp (entries before this time)
 * @return 1 on success, 0 on failure
 */
int driver_break_db_clear_history(struct driver_break_db *db, time_t before);

/**
 * @brief Save configuration to database
 * @param db Database handle
 * @param config Configuration structure to save
 * @return 1 on success, 0 on failure
 */
int driver_break_db_save_config(struct driver_break_db *db, struct driver_break_config *config);

/**
 * @brief Load configuration from database
 * @param db Database handle
 * @param config Configuration structure to populate
 * @return 1 on success, 0 if no saved config found
 */
int driver_break_db_load_config(struct driver_break_db *db, struct driver_break_config *config);

/**
 * @brief Free a history entry structure
 * @param entry History entry to free
 */
void driver_break_free_history_entry(struct driver_break_stop_history *entry);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_DB_H */
