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

#ifndef NAVIT_APRS_SDR_HW_H
#define NAVIT_APRS_SDR_HW_H

#include "config.h"

#ifdef HAVE_RTLSDR
#include <rtl-sdr.h>
#endif

struct aprs_sdr_hw;

enum aprs_sdr_device_type {
    APRS_SDR_UNKNOWN = 0,
    APRS_SDR_BLOG_V3,  /* RTL-SDR Blog V3 (R820T2) */
    APRS_SDR_V4_R828D, /* V4 R828D */
    APRS_SDR_NOOELEC,  /* Nooelec dongles */
    APRS_SDR_GENERIC   /* Generic RTL2832U */
};

struct aprs_sdr_hw_config {
    double frequency_mhz; /* Center frequency in MHz */
    int sample_rate;      /* Sample rate (48000 recommended) */
    int gain;             /* Tuner gain (0-49, or -1 for auto) */
    int ppm_correction;   /* Frequency correction in PPM */
    enum aprs_sdr_device_type device_type;
    int device_index; /* Device index if multiple dongles */
};

struct aprs_sdr_hw *aprs_sdr_hw_new(const struct aprs_sdr_hw_config *config);
void aprs_sdr_hw_destroy(struct aprs_sdr_hw *hw);

int aprs_sdr_hw_start(struct aprs_sdr_hw *hw);
int aprs_sdr_hw_stop(struct aprs_sdr_hw *hw);
int aprs_sdr_hw_is_running(struct aprs_sdr_hw *hw);

int aprs_sdr_hw_detect_devices(int *count);
int aprs_sdr_hw_get_device_info(int index, char *name, size_t name_len, enum aprs_sdr_device_type *type);

typedef void (*aprs_sdr_hw_callback)(const unsigned char *samples, int length, void *user_data);
int aprs_sdr_hw_set_callback(struct aprs_sdr_hw *hw, aprs_sdr_hw_callback cb, void *user_data);

int aprs_sdr_hw_set_frequency(struct aprs_sdr_hw *hw, double frequency_mhz);
int aprs_sdr_hw_set_gain(struct aprs_sdr_hw *hw, int gain);
int aprs_sdr_hw_set_ppm(struct aprs_sdr_hw *hw, int ppm);

#endif /* NAVIT_APRS_SDR_HW_H */
