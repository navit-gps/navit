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

#ifndef NAVIT_APRS_SDR_ANDROID_H
#define NAVIT_APRS_SDR_ANDROID_H

#include "config.h"

#ifdef ANDROID
#include <jni.h>
#include <android/log.h>

/**
 * @file aprs_sdr_android.h
 * @brief Android USB Host API integration for RTL-SDR devices
 *
 * This module provides Android-specific USB access for RTL-SDR dongles
 * using the Android USB Host API. It handles:
 * - USB device discovery and enumeration
 * - USB permission requests
 * - Bulk transfer operations for I/Q samples
 * - Integration with the standard RTL-SDR interface
 */

struct aprs_sdr_android;

/**
 * Initialize Android USB support
 * @param env JNI environment
 * @param context Android application context (jobject)
 * @return USB manager instance or NULL on failure
 */
struct aprs_sdr_android *aprs_sdr_android_new(JNIEnv *env, jobject context);

/**
 * Destroy Android USB manager
 */
void aprs_sdr_android_destroy(struct aprs_sdr_android *android_usb);

/**
 * Request USB device permissions
 * @param android_usb USB manager instance
 * @param vendor_id USB vendor ID (0x0bda for RTL-SDR)
 * @param product_id USB product ID
 * @return 1 if permission granted, 0 otherwise
 */
int aprs_sdr_android_request_permission(struct aprs_sdr_android *android_usb,
                                       int vendor_id, int product_id);

/**
 * Open USB device
 * @param android_usb USB manager instance
 * @param vendor_id USB vendor ID
 * @param product_id USB product ID
 * @return File descriptor or -1 on error
 */
int aprs_sdr_android_open_device(struct aprs_sdr_android *android_usb,
                                 int vendor_id, int product_id);

/**
 * Close USB device
 */
void aprs_sdr_android_close_device(struct aprs_sdr_android *android_usb);

/**
 * Read bulk data from USB device
 * @param android_usb USB manager instance
 * @param buffer Buffer to store data
 * @param length Buffer size
 * @return Number of bytes read, or -1 on error
 */
int aprs_sdr_android_bulk_read(struct aprs_sdr_android *android_usb,
                               unsigned char *buffer, int length);

/**
 * Write control request to USB device
 * @param android_usb USB manager instance
 * @param request_type Request type
 * @param request Request code
 * @param value Value field
 * @param index Index field
 * @param data Data buffer (for OUT requests)
 * @param length Data length
 * @return 0 on success, -1 on error
 */
int aprs_sdr_android_control_transfer(struct aprs_sdr_android *android_usb,
                                     int request_type, int request,
                                     int value, int index,
                                     unsigned char *data, int length);

/**
 * Check if USB device is connected
 */
int aprs_sdr_android_is_device_connected(struct aprs_sdr_android *android_usb);

#endif /* ANDROID */

#endif /* NAVIT_APRS_SDR_ANDROID_H */

