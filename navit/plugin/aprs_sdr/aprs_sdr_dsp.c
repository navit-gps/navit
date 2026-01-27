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

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include "debug.h"
#include "aprs_sdr_dsp.h"

/**
 * Bell 202 Demodulation Implementation
 * 
 * Based on patterns from Direwolf and standard APRS implementations.
 * 
 * Bell 202 uses 2FSK modulation:
 * - Mark (1): 1200 Hz
 * - Space (0): 2200 Hz
 * - Data rate: 1200 baud
 * - Encoding: NRZI (Non-Return-to-Zero Inverted)
 * 
 * Processing pipeline:
 * 1. I/Q samples -> Downconvert to baseband
 * 2. Baseband -> Filter and decimate
 * 3. Filtered -> Goertzel filters for mark/space detection
 * 4. Goertzel output -> Bit decision
 * 5. Bits -> NRZI decode
 * 6. Decoded bits -> AX.25 frame extraction
 */

#define BELL202_MARK_FREQ  1200.0
#define BELL202_SPACE_FREQ 2200.0
#define BELL202_BAUD_RATE  1200.0

struct aprs_sdr_dsp {
    struct aprs_sdr_dsp_config config;
    aprs_sdr_dsp_frame_callback frame_callback;
    void *frame_callback_user_data;
    
    /* Goertzel filters for mark/space detection */
    double mark_coeff;
    double space_coeff;
    int goertzel_block_size;
    
    /* Goertzel state */
    double mark_q0, mark_q1, mark_q2;
    double space_q0, space_q1, space_q2;
    int goertzel_sample_count;
    
    /* Bit synchronization */
    int bit_phase;
    double bit_accumulator;
    double samples_per_bit;
    
    /* NRZI decoder state */
    int last_bit;
    
    /* AX.25 frame extraction */
    unsigned char *frame_buffer;
    int frame_buffer_size;
    int frame_buffer_pos;
    int in_frame;
    int bit_stuff_count;
    
    /* Statistics */
    int frames_decoded;
    int bit_errors;
};

static void goertzel_init(struct aprs_sdr_dsp *dsp) {
    double n = dsp->goertzel_block_size;
    double mark_k = (n * dsp->config.mark_freq) / dsp->config.sample_rate;
    double space_k = (n * dsp->config.space_freq) / dsp->config.sample_rate;
    
    dsp->mark_coeff = 2.0 * cos(2.0 * M_PI * mark_k / n);
    dsp->space_coeff = 2.0 * cos(2.0 * M_PI * space_k / n);
    
    dsp->mark_q0 = dsp->mark_q1 = dsp->mark_q2 = 0.0;
    dsp->space_q0 = dsp->space_q1 = dsp->space_q2 = 0.0;
    dsp->goertzel_sample_count = 0;
}

static void goertzel_process_sample(struct aprs_sdr_dsp *dsp, double sample) {
    /* Update mark filter */
    dsp->mark_q0 = dsp->mark_coeff * dsp->mark_q1 - dsp->mark_q2 + sample;
    dsp->mark_q2 = dsp->mark_q1;
    dsp->mark_q1 = dsp->mark_q0;
    
    /* Update space filter */
    dsp->space_q0 = dsp->space_coeff * dsp->space_q1 - dsp->space_q2 + sample;
    dsp->space_q2 = dsp->space_q1;
    dsp->space_q1 = dsp->space_q0;
    
    dsp->goertzel_sample_count++;
}

static int goertzel_get_bit(struct aprs_sdr_dsp *dsp) {
    /* Calculate power in each frequency bin */
    double mark_power = dsp->mark_q1 * dsp->mark_q1 + dsp->mark_q2 * dsp->mark_q2 - 
                        dsp->mark_coeff * dsp->mark_q1 * dsp->mark_q2;
    double space_power = dsp->space_q1 * dsp->space_q1 + dsp->space_q2 * dsp->space_q2 - 
                         dsp->space_coeff * dsp->space_q1 * dsp->space_q2;
    
    /* Reset Goertzel state */
    dsp->mark_q0 = dsp->mark_q1 = dsp->mark_q2 = 0.0;
    dsp->space_q0 = dsp->space_q1 = dsp->space_q2 = 0.0;
    dsp->goertzel_sample_count = 0;
    
    /* Decision: mark (1) if mark power > space power */
    return (mark_power > space_power) ? 1 : 0;
}

static int nrzi_decode(int bit, int *last_bit) {
    /* NRZI: bit is 1 if no transition, 0 if transition */
    int decoded = (bit == *last_bit) ? 1 : 0;
    *last_bit = bit;
    return decoded;
}

static void process_ax25_bit(struct aprs_sdr_dsp *dsp, int bit) {
    if (!dsp->in_frame) {
        /* Look for flag sequence (0x7E = 01111110) */
        static int flag_pattern[] = {0,1,1,1,1,1,1,0};
        static int flag_pos = 0;
        
        if (bit == flag_pattern[flag_pos]) {
            flag_pos++;
            if (flag_pos == 8) {
                dsp->in_frame = 1;
                dsp->frame_buffer_pos = 0;
                dsp->bit_stuff_count = 0;
                flag_pos = 0;
            }
        } else {
            flag_pos = (bit == flag_pattern[0]) ? 1 : 0;
        }
        return;
    }
    
    /* Handle bit stuffing: after 5 consecutive 1s, next bit must be 0 */
    if (dsp->bit_stuff_count == 5) {
        if (bit != 0) {
            /* Invalid: abort frame */
            dsp->in_frame = 0;
            dsp->frame_buffer_pos = 0;
            dsp->bit_stuff_count = 0;
            return;
        }
        dsp->bit_stuff_count = 0;
        return; /* Discard stuffed bit */
    }
    
    if (bit == 1) {
        dsp->bit_stuff_count++;
    } else {
        dsp->bit_stuff_count = 0;
    }
    
    /* Accumulate bits into bytes (LSB first) */
    static int bit_pos = 0;
    static unsigned char current_byte = 0;
    
    current_byte |= (bit << bit_pos);
    bit_pos++;
    
    if (bit_pos == 8) {
        if (dsp->frame_buffer_pos >= dsp->frame_buffer_size) {
            /* Frame too long, abort */
            dsp->in_frame = 0;
            dsp->frame_buffer_pos = 0;
            bit_pos = 0;
            current_byte = 0;
            return;
        }
        
        dsp->frame_buffer[dsp->frame_buffer_pos++] = current_byte;
        
        /* Check for end flag */
        if (current_byte == 0x7E) {
            /* Frame complete */
            if (dsp->frame_callback && dsp->frame_buffer_pos > 2) {
                /* Deliver frame (excluding flags) */
                dsp->frame_callback(dsp->frame_buffer, 
                                    dsp->frame_buffer_pos - 2, 
                                    dsp->frame_callback_user_data);
                dsp->frames_decoded++;
            }
            dsp->in_frame = 0;
            dsp->frame_buffer_pos = 0;
        }
        
        bit_pos = 0;
        current_byte = 0;
    }
}

struct aprs_sdr_dsp *aprs_sdr_dsp_new(const struct aprs_sdr_dsp_config *config) {
    struct aprs_sdr_dsp *dsp;
    
    if (!config) {
        dbg(lvl_error, "DSP config is NULL");
        return NULL;
    }
    
    dsp = g_new0(struct aprs_sdr_dsp, 1);
    dsp->config = *config;
    
    /* Default frequencies if not specified */
    if (dsp->config.mark_freq == 0.0) {
        dsp->config.mark_freq = BELL202_MARK_FREQ;
    }
    if (dsp->config.space_freq == 0.0) {
        dsp->config.space_freq = BELL202_SPACE_FREQ;
    }
    if (dsp->config.baud_rate == 0.0) {
        dsp->config.baud_rate = BELL202_BAUD_RATE;
    }
    
    /* Calculate Goertzel block size (samples per bit) */
    dsp->samples_per_bit = dsp->config.sample_rate / dsp->config.baud_rate;
    dsp->goertzel_block_size = (int)(dsp->samples_per_bit + 0.5);
    
    /* Initialize Goertzel filters */
    goertzel_init(dsp);
    
    /* Allocate frame buffer (max AX.25 frame is ~330 bytes) */
    dsp->frame_buffer_size = 512;
    dsp->frame_buffer = g_malloc(dsp->frame_buffer_size);
    
    dsp->bit_phase = 0;
    dsp->bit_accumulator = 0.0;
    dsp->last_bit = 0;
    dsp->in_frame = 0;
    dsp->frame_buffer_pos = 0;
    dsp->bit_stuff_count = 0;
    dsp->frames_decoded = 0;
    dsp->bit_errors = 0;
    
    dbg(lvl_info, "Bell 202 DSP initialized: %.0f Hz mark, %.0f Hz space, %d samples/bit",
        dsp->config.mark_freq, dsp->config.space_freq, dsp->goertzel_block_size);
    
    return dsp;
}

void aprs_sdr_dsp_destroy(struct aprs_sdr_dsp *dsp) {
    if (!dsp) return;
    
    g_free(dsp->frame_buffer);
    g_free(dsp);
}

int aprs_sdr_dsp_process_samples(struct aprs_sdr_dsp *dsp, 
                                  const unsigned char *samples, 
                                  int length) {
    int frames_before = dsp->frames_decoded;
    int i;
    
    if (!dsp || !samples || length < 2) return 0;
    
    /* Process I/Q samples (interleaved, 8-bit unsigned) */
    for (i = 0; i < length - 1; i += 2) {
        /* Convert to signed and compute magnitude/phase */
        /* Simplified: use I component for now (can be enhanced with proper downconversion) */
        double i_sample = (double)((int)samples[i] - 128) / 128.0;
        double q_sample = (double)((int)samples[i + 1] - 128) / 128.0;
        
        /* Downconvert to baseband (simplified - assumes center frequency is correct) */
        /* In a full implementation, this would include proper mixing and filtering */
        double magnitude = sqrt(i_sample * i_sample + q_sample * q_sample);
        
        /* Accumulate samples for bit timing */
        dsp->bit_accumulator += magnitude;
        dsp->bit_phase++;
        
        /* Process when we have enough samples for one bit */
        if (dsp->bit_phase >= dsp->goertzel_block_size) {
            double avg_sample = dsp->bit_accumulator / dsp->bit_phase;
            
            /* Process through Goertzel filters */
            goertzel_process_sample(dsp, avg_sample);
            
            /* Get bit decision when block is complete */
            if (dsp->goertzel_sample_count >= dsp->goertzel_block_size) {
                int raw_bit = goertzel_get_bit(dsp);
                int decoded_bit = nrzi_decode(raw_bit, &dsp->last_bit);
                process_ax25_bit(dsp, decoded_bit);
            }
            
            dsp->bit_accumulator = 0.0;
            dsp->bit_phase = 0;
        }
    }
    
    return dsp->frames_decoded - frames_before;
}

int aprs_sdr_dsp_set_frame_callback(struct aprs_sdr_dsp *dsp, 
                                     aprs_sdr_dsp_frame_callback callback,
                                     void *user_data) {
    if (!dsp) return 0;
    dsp->frame_callback = callback;
    dsp->frame_callback_user_data = user_data;
    return 1;
}

