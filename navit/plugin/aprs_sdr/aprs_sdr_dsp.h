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

#ifndef NAVIT_APRS_SDR_DSP_H
#define NAVIT_APRS_SDR_DSP_H

#include "config.h"

/**
 * @file aprs_sdr_dsp.h
 * @brief Bell 202 demodulation for APRS (2FSK at 1200 bps)
 *
 * This module implements Bell 202 demodulation based on patterns from
 * Direwolf and other well-documented APRS implementations.
 *
 * Bell 202 characteristics:
 * - Modulation: 2FSK (Binary Frequency Shift Keying)
 * - Data rate: 1200 bps
 * - Mark frequency: 1200 Hz
 * - Space frequency: 2200 Hz
 * - Encoding: NRZI (Non-Return-to-Zero Inverted)
 * - Frame format: AX.25 UI-frames
 */

struct aprs_sdr_dsp;

struct aprs_sdr_dsp_config {
    int sample_rate;          /* Input sample rate (typically 48000 Hz) */
    double mark_freq;         /* Mark frequency (1200 Hz) */
    double space_freq;        /* Space frequency (2200 Hz) */
    double baud_rate;         /* Symbol rate (1200 bps) */
};

struct aprs_sdr_dsp *aprs_sdr_dsp_new(const struct aprs_sdr_dsp_config *config);
void aprs_sdr_dsp_destroy(struct aprs_sdr_dsp *dsp);

/**
 * Process I/Q samples and extract AX.25 frames
 * @param dsp DSP instance
 * @param samples I/Q sample buffer (interleaved, 8-bit unsigned)
 * @param length Number of samples (must be even, as I/Q pairs)
 * @return Number of frames extracted (0 if none)
 */
int aprs_sdr_dsp_process_samples(struct aprs_sdr_dsp *dsp, 
                                  const unsigned char *samples, 
                                  int length);

/**
 * Set callback for delivering decoded AX.25 frames
 * @param dsp DSP instance
 * @param callback Function to call when a frame is decoded
 * @param user_data User context
 */
typedef void (*aprs_sdr_dsp_frame_callback)(const unsigned char *frame, int frame_length, void *user_data);
int aprs_sdr_dsp_set_frame_callback(struct aprs_sdr_dsp *dsp, 
                                     aprs_sdr_dsp_frame_callback callback,
                                     void *user_data);

#endif /* NAVIT_APRS_SDR_DSP_H */

