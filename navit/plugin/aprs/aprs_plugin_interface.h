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

#ifndef NAVIT_APRS_PLUGIN_INTERFACE_H
#define NAVIT_APRS_PLUGIN_INTERFACE_H

/**
 * @file aprs_plugin_interface.h
 * @brief Interface for inter-plugin communication between APRS and SDR plugins
 *
 * This header defines the interface that allows the aprs_sdr plugin to
 * deliver decoded AX.25 frames to the aprs plugin.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Callback function type for delivering decoded APRS packets
 * @param data Pointer to raw AX.25 frame data
 * @param length Length of the frame in bytes
 * @param user_data User-provided context (typically NULL)
 */
typedef void (*aprs_packet_callback_t)(const unsigned char *data, int length, void *user_data);

/**
 * Register a packet callback with the APRS plugin
 * 
 * This function is exported by the APRS plugin and can be called by
 * other plugins (like aprs_sdr) to register a callback that will be
 * invoked whenever a decoded APRS packet is available.
 * 
 * @param callback Function to call when a packet is decoded
 * @param user_data User context to pass to callback
 * @return 1 on success, 0 on failure (APRS plugin not available)
 */
int aprs_register_packet_source(aprs_packet_callback_t callback, void *user_data);

/**
 * Unregister a packet callback
 * 
 * @param callback The callback function to unregister
 * @return 1 on success, 0 on failure
 */
int aprs_unregister_packet_source(aprs_packet_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* NAVIT_APRS_PLUGIN_INTERFACE_H */

