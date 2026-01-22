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

#ifndef NAVIT_APRS_RTLSDR_H
#define NAVIT_APRS_RTLSDR_H

#include "config.h"

#ifdef HAVE_RTLSDR
#include <rtl-sdr.h>
#endif

struct aprs_rtlsdr;

enum aprs_rtlsdr_device_type {
    APRS_RTLSDR_UNKNOWN = 0,
    APRS_RTLSDR_BLOG_V3,      /* RTL-SDR Blog V3 (R820T2) */
    APRS_RTLSDR_V4_R828D,     /* V4 R828D */
    APRS_RTLSDR_NOOELEC,      /* Nooelec dongles */
    APRS_RTLSDR_GENERIC       /* Generic RTL2832U */
};

struct aprs_rtlsdr_config {
    double frequency_mhz;     /* Center frequency in MHz */
    int sample_rate;          /* Sample rate (48000 recommended) */
    int gain;                 /* Tuner gain (0-49, or -1 for auto) */
    int ppm_correction;       /* Frequency correction in PPM */
    int squelch;              /* Squelch level (0 = disabled) */
    enum aprs_rtlsdr_device_type device_type;
    int device_index;         /* Device index if multiple dongles */
};

struct aprs_rtlsdr *aprs_rtlsdr_new(const struct aprs_rtlsdr_config *config);
void aprs_rtlsdr_destroy(struct aprs_rtlsdr *rtl);

int aprs_rtlsdr_start(struct aprs_rtlsdr *rtl);
int aprs_rtlsdr_stop(struct aprs_rtlsdr *rtl);
int aprs_rtlsdr_is_running(struct aprs_rtlsdr *rtl);

int aprs_rtlsdr_detect_devices(int *count);
int aprs_rtlsdr_get_device_info(int index, char *name, size_t name_len, 
                                enum aprs_rtlsdr_device_type *type);

typedef void (*aprs_rtlsdr_callback)(const unsigned char *samples, int length, void *user_data);
int aprs_rtlsdr_set_callback(struct aprs_rtlsdr *rtl, aprs_rtlsdr_callback cb, void *user_data);

int aprs_rtlsdr_set_frequency(struct aprs_rtlsdr *rtl, double frequency_mhz);
int aprs_rtlsdr_set_gain(struct aprs_rtlsdr *rtl, int gain);
int aprs_rtlsdr_set_ppm(struct aprs_rtlsdr *rtl, int ppm);

#endif

