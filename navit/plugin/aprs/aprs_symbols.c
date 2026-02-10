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
 *
 * APRS Symbol Lookup - Integration with hessu/aprs-symbols
 *
 * Maps APRS symbol_table and symbol_code to icon filenames from
 * the hessu/aprs-symbols repository (https://github.com/hessu/aprs-symbols)
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "debug.h"
#include "aprs_symbols.h"

#ifndef IMAGE_DIR
#define IMAGE_DIR "share/navit/icons"
#endif

/* Symbol lookup entry */
struct symbol_entry {
    char table;
    char code;
    char *filename;
    char *description;
};

/* Built-in symbol lookup table based on hessu/aprs-symbols structure
 * Primary symbol table (/) - most common symbols */
static const struct symbol_entry primary_symbols[] = {
    {'/', '!', "digipeater.png", "Digipeater"},
    {'/', '"', "mailbox.png", "Mailbox"},
    {'/', '#', "phone.png", "Phone"},
    {'/', '$', "dx_cluster.png", "DX Cluster"},
    {'/', '%', "hf_gateway.png", "HF Gateway"},
    {'/', '&', "small_circle.png", "Small Circle"},
    {'/', '\'', "mob_station.png", "Mobile Station"},
    {'/', '(', "repeater.png", "Repeater"},
    {'/', ')', "repeater.png", "Repeater"},
    {'/', '*', "aircraft.png", "Aircraft"},
    {'/', '+', "snowmobile.png", "Snowmobile"},
    {'/', ',', "red_x.png", "Red X"},
    {'/', '-', "house.png", "House"},
    {'/', '.', "circle.png", "Circle"},
    {'/', '/', "file_server.png", "File Server"},
    {'/', '0', "fire.png", "Fire"},
    {'/', '1', "campground.png", "Campground"},
    {'/', '2', "motorcycle.png", "Motorcycle"},
    {'/', '3', "rail_engine.png", "Rail Engine"},
    {'/', '4', "car.png", "Car"},
    {'/', '5', "file_server.png", "File Server"},
    {'/', '6', "cloudy.png", "Cloudy"},
    {'/', '7', "hail.png", "Hail"},
    {'/', '8', "rain.png", "Rain"},
    {'/', '9', "snow.png", "Snow"},
    {'/', ':', "tornado.png", "Tornado"},
    {'/', ';', "hurricane.png", "Hurricane"},
    {'/', '<', "tropical_storm.png", "Tropical Storm"},
    {'/', '=', "hurricane.png", "Hurricane"},
    {'/', '>', "vehicle.png", "Vehicle"},
    {'/', '?', "unknown.png", "Unknown"},
    {'/', '@', "digipeater.png", "Digipeater"},
    {'/', 'A', "aid_station.png", "Aid Station"},
    {'/', 'B', "bike.png", "Bike"},
    {'/', 'C', "canoe.png", "Canoe"},
    {'/', 'D', "fire_dept.png", "Fire Department"},
    {'/', 'E', "horse.png", "Horse"},
    {'/', 'F', "fire_truck.png", "Fire Truck"},
    {'/', 'G', "glider.png", "Glider"},
    {'/', 'H', "hospital.png", "Hospital"},
    {'/', 'I', "iota.png", "IOTA"},
    {'/', 'J', "jeep.png", "Jeep"},
    {'/', 'K', "school.png", "School"},
    {'/', 'L', "user.png", "User"},
    {'/', 'M', "macap.png", "MacAP"},
    {'/', 'N', "nws_station.png", "NWS Station"},
    {'/', 'O', "balloon.png", "Balloon"},
    {'/', 'P', "police.png", "Police"},
    {'/', 'Q', "truck.png", "Truck"},
    {'/', 'R', "repeater.png", "Repeater"},
    {'/', 'S', "boat.png", "Boat"},
    {'/', 'T', "truck_stop.png", "Truck Stop"},
    {'/', 'U', "truck.png", "Truck"},
    {'/', 'V', "van.png", "Van"},
    {'/', 'W', "water_station.png", "Water Station"},
    {'/', 'X', "x.png", "X"},
    {'/', 'Y', "yacht.png", "Yacht"},
    {'/', 'Z', "truck.png", "Truck"},
    {'/', '[', "picnic_table.png", "Picnic Table"},
    {'/', '\\', "car.png", "Car"},
    {'/', ']', "wall.png", "Wall"},
    {'/', '^', "aircraft.png", "Aircraft"},
    {'/', '_', "weather_station.png", "Weather Station"},
    {'/', '`', "house.png", "House"},
    {'/', 'a', "ambulance.png", "Ambulance"},
    {'/', 'b', "bike.png", "Bike"},
    {'/', 'c', "car.png", "Car"},
    {'/', 'd', "fire_dept.png", "Fire Department"},
    {'/', 'e', "eye.png", "Eye"},
    {'/', 'f', "farm.png", "Farm"},
    {'/', 'g', "grid_square.png", "Grid Square"},
    {'/', 'h', "hotel.png", "Hotel"},
    {'/', 'i', "tcpip.png", "TCP/IP"},
    {'/', 'j', "triangle.png", "Triangle"},
    {'/', 'k', "school.png", "School"},
    {'/', 'l', "lighthouse.png", "Lighthouse"},
    {'/', 'm', "mara.png", "MARA"},
    {'/', 'n', "nav_beacon.png", "Navigation Beacon"},
    {'/', 'o', "balloon.png", "Balloon"},
    {'/', 'p', "parking.png", "Parking"},
    {'/', 'q', "truck.png", "Truck"},
    {'/', 'r', "repeater.png", "Repeater"},
    {'/', 's', "satellite.png", "Satellite"},
    {'/', 't', "sstv.png", "SSTV"},
    {'/', 'u', "bus.png", "Bus"},
    {'/', 'v', "van.png", "Van"},
    {'/', 'w', "water_station.png", "Water Station"},
    {'/', 'x', "x.png", "X"},
    {'/', 'y', "yacht.png", "Yacht"},
    {'/', 'z', "z.png", "Z"},
    {'/', '{', "shelter.png", "Shelter"},
    {'/', '|', "truck.png", "Truck"},
    {'/', '}', "truck.png", "Truck"},
    {'/', '~', "grid_square.png", "Grid Square"},
};

/* Alternate symbol table (\) - alternate symbols */
static const struct symbol_entry alternate_symbols[] = {
    {'\\', '!', "digipeater.png", "Digipeater"},
    {'\\', '"', "mailbox.png", "Mailbox"},
    {'\\', '#', "phone.png", "Phone"},
    {'\\', '$', "dx_cluster.png", "DX Cluster"},
    {'\\', '%', "hf_gateway.png", "HF Gateway"},
    {'\\', '&', "small_circle.png", "Small Circle"},
    {'\\', '\'', "mob_station.png", "Mobile Station"},
    {'\\', '(', "repeater.png", "Repeater"},
    {'\\', ')', "repeater.png", "Repeater"},
    {'\\', '*', "aircraft.png", "Aircraft"},
    {'\\', '+', "snowmobile.png", "Snowmobile"},
    {'\\', ',', "red_x.png", "Red X"},
    {'\\', '-', "house.png", "House"},
    {'\\', '.', "circle.png", "Circle"},
    {'\\', '/', "file_server.png", "File Server"},
    {'\\', '>', "vehicle.png", "Vehicle"},
    {'\\', '?', "unknown.png", "Unknown"},
    {'\\', '@', "digipeater.png", "Digipeater"},
    {'\\', 'A', "aid_station.png", "Aid Station"},
    {'\\', 'B', "bike.png", "Bike"},
    {'\\', 'C', "canoe.png", "Canoe"},
    {'\\', 'D', "fire_dept.png", "Fire Department"},
    {'\\', 'E', "horse.png", "Horse"},
    {'\\', 'F', "fire_truck.png", "Fire Truck"},
    {'\\', 'G', "glider.png", "Glider"},
    {'\\', 'H', "hospital.png", "Hospital"},
    {'\\', 'I', "iota.png", "IOTA"},
    {'\\', 'J', "jeep.png", "Jeep"},
    {'\\', 'K', "school.png", "School"},
    {'\\', 'L', "user.png", "User"},
    {'\\', 'M', "macap.png", "MacAP"},
    {'\\', 'N', "nws_station.png", "NWS Station"},
    {'\\', 'O', "balloon.png", "Balloon"},
    {'\\', 'P', "police.png", "Police"},
    {'\\', 'Q', "truck.png", "Truck"},
    {'\\', 'R', "repeater.png", "Repeater"},
    {'\\', 'S', "boat.png", "Boat"},
    {'\\', 'T', "truck_stop.png", "Truck Stop"},
    {'\\', 'U', "truck.png", "Truck"},
    {'\\', 'V', "van.png", "Van"},
    {'\\', 'W', "water_station.png", "Water Station"},
    {'\\', 'X', "x.png", "X"},
    {'\\', 'Y', "yacht.png", "Yacht"},
    {'\\', 'Z', "truck.png", "Truck"},
    {'\\', '[', "picnic_table.png", "Picnic Table"},
    {'\\', '\\', "car.png", "Car"},
    {'\\', ']', "wall.png", "Wall"},
    {'\\', '^', "aircraft.png", "Aircraft"},
    {'\\', '_', "weather_station.png", "Weather Station"},
    {'\\', '`', "house.png", "House"},
    {'\\', 'a', "ambulance.png", "Ambulance"},
    {'\\', 'b', "bike.png", "Bike"},
    {'\\', 'c', "car.png", "Car"},
    {'\\', 'd', "fire_dept.png", "Fire Department"},
    {'\\', 'e', "eye.png", "Eye"},
    {'\\', 'f', "farm.png", "Farm"},
    {'\\', 'g', "grid_square.png", "Grid Square"},
    {'\\', 'h', "hotel.png", "Hotel"},
    {'\\', 'i', "tcpip.png", "TCP/IP"},
    {'\\', 'j', "triangle.png", "Triangle"},
    {'\\', 'k', "school.png", "School"},
    {'\\', 'l', "lighthouse.png", "Lighthouse"},
    {'\\', 'm', "mara.png", "MARA"},
    {'\\', 'n', "nav_beacon.png", "Navigation Beacon"},
    {'\\', 'o', "balloon.png", "Balloon"},
    {'\\', 'p', "parking.png", "Parking"},
    {'\\', 'q', "truck.png", "Truck"},
    {'\\', 'r', "repeater.png", "Repeater"},
    {'\\', 's', "satellite.png", "Satellite"},
    {'\\', 't', "sstv.png", "SSTV"},
    {'\\', 'u', "bus.png", "Bus"},
    {'\\', 'v', "van.png", "Van"},
    {'\\', 'w', "water_station.png", "Water Station"},
    {'\\', 'x', "x.png", "X"},
    {'\\', 'y', "yacht.png", "Yacht"},
    {'\\', 'z', "z.png", "Z"},
    {'\\', '{', "shelter.png", "Shelter"},
    {'\\', '|', "truck.png", "Truck"},
    {'\\', '}', "truck.png", "Truck"},
    {'\\', '~', "grid_square.png", "Grid Square"},
};

static char *icon_base_path = NULL;
static GHashTable *symbol_lookup = NULL;

static const struct symbol_entry *find_symbol_entry(char table, char code) {
    const struct symbol_entry *entries = NULL;
    size_t count = 0;

    if (table == '/') {
        entries = primary_symbols;
        count = sizeof(primary_symbols) / sizeof(primary_symbols[0]);
    } else if (table == '\\') {
        entries = alternate_symbols;
        count = sizeof(alternate_symbols) / sizeof(alternate_symbols[0]);
    } else {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        if (entries[i].table == table && entries[i].code == code) {
            return &entries[i];
        }
    }

    return NULL;
}

char *aprs_symbol_get_icon(char symbol_table, char symbol_code) {
    if (!icon_base_path) {
        dbg(lvl_warning, "APRS symbols not initialized");
        return NULL;
    }

    const struct symbol_entry *entry = find_symbol_entry(symbol_table, symbol_code);
    if (!entry) {
        dbg(lvl_debug, "Symbol not found: table='%c' code='%c'", symbol_table, symbol_code);
        return NULL;
    }

    /* Return relative path - Navit's graphics_icon_path() will resolve it */
    char *icon_path = g_build_filename(icon_base_path, entry->filename, NULL);
    return icon_path;
}

char *aprs_symbol_get_description(char symbol_table, char symbol_code) {
    const struct symbol_entry *entry = find_symbol_entry(symbol_table, symbol_code);
    if (!entry) {
        return NULL;
    }

    return g_strdup(entry->description);
}

int aprs_symbol_init(const char *symbol_index_path, const char *icon_base) {
    (void)symbol_index_path; /* TODO: Parse CSV if provided */

    if (icon_base) {
        icon_base_path = g_strdup(icon_base);
    } else {
        /* Use Navit's standard share directory for plugin symbols
         * Symbols are installed to share/navit/aprs-symbols/48x48/primary
         * We'll use a relative path that works with graphics_icon_path() */
        icon_base_path = g_strdup("aprs-symbols/48x48/primary");
    }

    dbg(lvl_info, "APRS symbols initialized: base path=%s", icon_base_path);
    return 1;
}

void aprs_symbol_cleanup(void) {
    if (icon_base_path) {
        g_free(icon_base_path);
        icon_base_path = NULL;
    }

    if (symbol_lookup) {
        g_hash_table_destroy(symbol_lookup);
        symbol_lookup = NULL;
    }
}

