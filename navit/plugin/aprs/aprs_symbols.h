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

#ifndef NAVIT_APRS_SYMBOLS_H
#define NAVIT_APRS_SYMBOLS_H

/**
 * Get icon filename for APRS symbol
 *
 * @param symbol_table APRS symbol table character (e.g., '/', '\', ']')
 * @param symbol_code APRS symbol code character (e.g., '>', '!', '&')
 * @return Icon filename (e.g., "aprs-symbols/48x48/primary/vehicle.png")
 *         Returns NULL if symbol not found or invalid
 *         Caller must free the returned string with g_free()
 */
char *aprs_symbol_get_icon(char symbol_table, char symbol_code);

/**
 * Get description for APRS symbol
 *
 * @param symbol_table APRS symbol table character
 * @param symbol_code APRS symbol code character
 * @return Human-readable description (e.g., "Vehicle")
 *         Returns NULL if symbol not found
 *         Caller must free the returned string with g_free()
 */
char *aprs_symbol_get_description(char symbol_table, char symbol_code);

/**
 * Initialize symbol lookup system
 *
 * Loads symbol index from CSV file or uses built-in lookup table
 *
 * @param symbol_index_path Path to symbol-index CSV file (optional, NULL for built-in)
 * @param icon_base_path Base path to icon directory (e.g., "/usr/share/navit/aprs-symbols/48x48")
 * @return 1 on success, 0 on failure
 */
int aprs_symbol_init(const char *symbol_index_path, const char *icon_base_path);

/**
 * Cleanup symbol lookup system
 */
void aprs_symbol_cleanup(void);

#endif
