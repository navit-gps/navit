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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "aprs_sdr_dsp.h"
#include "config.h"
#include "debug.h"
#include <glib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define BELL202_MARK_FREQ 1200.0
#define BELL202_SPACE_FREQ 2200.0
#define BELL202_BAUD_RATE 1200.0

struct aprs_sdr_dsp {
    struct aprs_sdr_dsp_config config;
    aprs_sdr_dsp_frame_callback frame_callback;
    void *frame_callback_user_data;

    /* Goertzel filters for mark/space detection (operating on audio samples) */
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

    /* RF -> audio downconversion / FM discriminator state */
    double mixer_phase;
    double mixer_phase_inc;
    double prev_i;
    double prev_q;
    double dc_i;
    double dc_q;
    double dc_alpha;
    int decimation_factor;
    int decimation_index;

    /* Diagnostics */
    long long diag_rf_samples;
    long long diag_audio_samples;
    long long diag_goertzel_blocks;
    long long diag_raw_bits;
    long long diag_decoded_bits;
    long long diag_flags_found;
};

static void goertzel_init(struct aprs_sdr_dsp *dsp) {
    double n = dsp->goertzel_block_size;
    double mark_k = (n * dsp->config.mark_freq) / dsp->config.audio_sample_rate;
    double space_k = (n * dsp->config.space_freq) / dsp->config.audio_sample_rate;

    dsp->mark_coeff = 2.0 * cos(2.0 * M_PI * mark_k / n);
    dsp->space_coeff = 2.0 * cos(2.0 * M_PI * space_k / n);

    dsp->mark_q0 = dsp->mark_q1 = dsp->mark_q2 = 0.0;
    dsp->space_q0 = dsp->space_q1 = dsp->space_q2 = 0.0;
    dsp->goertzel_sample_count = 0;

    fprintf(stderr,
            "Goertzel: mark_coeff=%.6f space_coeff=%.6f block_size=%d audio_rate=%d mark_freq=%.0f space_freq=%.0f\n",
            dsp->mark_coeff, dsp->space_coeff, dsp->goertzel_block_size, dsp->config.audio_sample_rate,
            dsp->config.mark_freq, dsp->config.space_freq);
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

    /* For block 7 diagnostics, dump first few inputs. */
    if (dsp->diag_goertzel_blocks == 7 && dsp->goertzel_sample_count <= 3) {
        fprintf(stderr, "goertzel_input[blk7,sample%d]=%.6f\n",
                dsp->goertzel_sample_count - 1, sample);
    }
}

static int goertzel_get_bit(struct aprs_sdr_dsp *dsp) {
    /* Calculate power in each frequency bin */
    double mark_power = dsp->mark_q1 * dsp->mark_q1 + dsp->mark_q2 * dsp->mark_q2
                        - dsp->mark_coeff * dsp->mark_q1 * dsp->mark_q2;
    double space_power = dsp->space_q1 * dsp->space_q1 + dsp->space_q2 * dsp->space_q2
                         - dsp->space_coeff * dsp->space_q1 * dsp->space_q2;

    /* Reset Goertzel state */
    dsp->mark_q0 = dsp->mark_q1 = dsp->mark_q2 = 0.0;
    dsp->space_q0 = dsp->space_q1 = dsp->space_q2 = 0.0;
    dsp->goertzel_sample_count = 0;

    /* Decision: mark (1) if mark power > space power */
    static int goertzel_dump = 0;
    int raw_bit = (mark_power > space_power) ? 1 : 0;
    if (goertzel_dump < 16) {
        fprintf(stderr, "goertzel[%d]: mark_power=%.6f space_power=%.6f raw_bit=%d\n",
                goertzel_dump, mark_power, space_power, raw_bit);
        goertzel_dump++;
    }
    if (dsp->diag_goertzel_blocks >= 5 && dsp->diag_goertzel_blocks <= 10) {
        fprintf(stderr, "goertzel_blk[%lld]: mark=%.6f space=%.6f raw=%d\n",
                (long long)dsp->diag_goertzel_blocks, mark_power, space_power, raw_bit);
    }
    return raw_bit;
}

static int nrzi_decode(int bit, int *last_bit) {
    /* NRZI: bit is 1 if no transition, 0 if transition */
    int decoded = (bit == *last_bit) ? 1 : 0;
    *last_bit = bit;
    return decoded;
}

/* Look for flag sequence (0x7E). Returns after consuming this bit. */
static void process_ax25_flag_search(struct aprs_sdr_dsp *dsp, int bit) {
    static int flag_pattern[] = {0, 1, 1, 1, 1, 1, 1, 0};
    static int flag_pos = 0;

    if (bit == flag_pattern[flag_pos]) {
        flag_pos++;
        if (flag_pos == 8) {
            dsp->in_frame = 1;
            dsp->frame_buffer_pos = 0;
            dsp->bit_stuff_count = 0;
            dsp->diag_flags_found++;
            flag_pos = 0;
        }
    } else {
        flag_pos = (bit == flag_pattern[0]) ? 1 : 0;
    }
}

/* Handle bit stuffing and byte accumulation when in frame. */
static void process_ax25_in_frame(struct aprs_sdr_dsp *dsp, int bit) {
    static int bit_pos = 0;
    static unsigned char current_byte = 0;

    if (dsp->bit_stuff_count == 5) {
        if (bit != 0) {
            dsp->in_frame = 0;
            dsp->frame_buffer_pos = 0;
            dsp->bit_stuff_count = 0;
            return;
        }
        dsp->bit_stuff_count = 0;
        return;
    }
    if (bit == 1)
        dsp->bit_stuff_count++;
    else
        dsp->bit_stuff_count = 0;

    current_byte |= (bit << bit_pos);
    bit_pos++;
    if (bit_pos != 8)
        return;

    /* We have assembled a complete byte (LSB-first) */
    fprintf(stderr, "frame_byte[%d]=0x%02X (bit_pos=%d)\n", dsp->frame_buffer_pos, current_byte, bit_pos);
    bit_pos = 0;
    if (dsp->frame_buffer_pos >= dsp->frame_buffer_size) {
        dsp->in_frame = 0;
        dsp->frame_buffer_pos = 0;
        current_byte = 0;
        return;
    }
    dsp->frame_buffer[dsp->frame_buffer_pos++] = current_byte;
    if (current_byte == 0x7E) {
        if (dsp->frame_callback && dsp->frame_buffer_pos > 2)
            dsp->frame_callback(dsp->frame_buffer, dsp->frame_buffer_pos - 2, dsp->frame_callback_user_data);
        dsp->frames_decoded++;
        dsp->in_frame = 0;
        dsp->frame_buffer_pos = 0;
    }
    current_byte = 0;
}

static void process_ax25_bit(struct aprs_sdr_dsp *dsp, int bit) {
    static int bit_dump_count = 0;
    if (bit_dump_count < 32) {
        fprintf(stderr, "DSP bit[%d]=%d\n", bit_dump_count, bit);
        bit_dump_count++;
    }
    fprintf(stderr, "ax25_bit: pos=%lld bit=%d in_frame=%d\n",
            (long long)dsp->diag_decoded_bits, bit, dsp->in_frame);
    if (!dsp->in_frame) {
        process_ax25_flag_search(dsp, bit);
        return;
    }
    process_ax25_in_frame(dsp, bit);
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

    /* Derive decimation factor between RF and audio domains. Require integer factor for now. */
    if (dsp->config.rf_sample_rate <= 0 || dsp->config.audio_sample_rate <= 0
        || dsp->config.rf_sample_rate < dsp->config.audio_sample_rate) {
        dbg(lvl_error, "Invalid RF/audio sample rates (rf=%d, audio=%d) - using audio == RF and disabling decimation",
            dsp->config.rf_sample_rate, dsp->config.audio_sample_rate);
        dsp->config.audio_sample_rate = dsp->config.rf_sample_rate;
    }

    dsp->decimation_factor = dsp->config.rf_sample_rate / dsp->config.audio_sample_rate;
    if (dsp->decimation_factor <= 0) {
        dsp->decimation_factor = 1;
    }

    /* Calculate Goertzel block size (samples per bit) in audio domain */
    dsp->samples_per_bit = dsp->config.audio_sample_rate / dsp->config.baud_rate;
    dsp->goertzel_block_size = (int)(dsp->samples_per_bit + 0.5);

    /* Initialize Goertzel filters */
    goertzel_init(dsp);
    /* Offset Goertzel window by half a symbol (20 audio samples)
     * so decisions use the middle of each symbol, not the boundary. */
    dsp->goertzel_sample_count = 20;

    /* Allocate frame buffer (max AX.25 frame is ~330 bytes) */
    dsp->frame_buffer_size = 512;
    dsp->frame_buffer = g_malloc(dsp->frame_buffer_size);

    dsp->bit_phase = 0;
    dsp->bit_accumulator = 0.0;
    /* Assume idle NRZI line state is mark (1), so first transition (space) decodes as data 0. */
    dsp->last_bit = 1;
    dsp->in_frame = 0;
    dsp->frame_buffer_pos = 0;
    dsp->bit_stuff_count = 0;
    dsp->frames_decoded = 0;
    dsp->bit_errors = 0;

    /* RF -> audio conversion state */
    dsp->mixer_phase = 0.0;
    if (dsp->config.rf_sample_rate > 0) {
        /* Mix DOWN by IF offset to bring APRS channel from RF center
         * (APRS + if_offset) to baseband. */
        dsp->mixer_phase_inc = -2.0 * M_PI * dsp->config.if_offset_hz / (double)dsp->config.rf_sample_rate;
        fprintf(stderr,
                "mixer: phase_inc=%.10f if_offset=%.0f rf_rate=%d expected_phase_inc=%.10f\n",
                dsp->mixer_phase_inc, dsp->config.if_offset_hz, dsp->config.rf_sample_rate,
                -2.0 * M_PI * dsp->config.if_offset_hz / (double)dsp->config.rf_sample_rate);
    } else {
        dsp->mixer_phase_inc = 0.0;
    }
    dsp->prev_i = 0.0;
    dsp->prev_q = 0.0;
    dsp->dc_i = 0.0;
    dsp->dc_q = 0.0;
    dsp->dc_alpha = 0.001; /* Slow DC removal */
    dsp->decimation_index = 0;

    dsp->diag_rf_samples = 0;
    dsp->diag_audio_samples = 0;
    dsp->diag_goertzel_blocks = 0;
    dsp->diag_raw_bits = 0;
    dsp->diag_decoded_bits = 0;
    dsp->diag_flags_found = 0;

    fprintf(stderr,
            "DSP created: mark_freq=%.0f space_freq=%.0f if_offset=%.0f rf_rate=%d audio_rate=%d\n",
            dsp->config.mark_freq, dsp->config.space_freq, dsp->config.if_offset_hz, dsp->config.rf_sample_rate,
            dsp->config.audio_sample_rate);

    dbg(lvl_info,
        "Bell 202 DSP initialized: RF %d Hz, audio %d Hz, IF offset %.0f Hz, %.0f Hz mark, %.0f Hz space, %d "
        "samples/bit",
        dsp->config.rf_sample_rate, dsp->config.audio_sample_rate, dsp->config.if_offset_hz, dsp->config.mark_freq,
        dsp->config.space_freq, dsp->goertzel_block_size);

    return dsp;
}

void aprs_sdr_dsp_destroy(struct aprs_sdr_dsp *dsp) {
    if (!dsp)
        return;

    g_free(dsp->frame_buffer);
    g_free(dsp);
}

int aprs_sdr_dsp_process_samples(struct aprs_sdr_dsp *dsp, const unsigned char *samples, int length) {
    int frames_before = dsp->frames_decoded;
    int i;

    if (!dsp || !samples || length < 2)
        return 0;

    /* Process RF I/Q samples (interleaved, 8-bit unsigned) */
    for (i = 0; i < length - 1; i += 2) {
        /* Convert to signed and normalize */
        double i_sample = (double)((int)samples[i] - 128) / 128.0;
        double q_sample = (double)((int)samples[i + 1] - 128) / 128.0;

        if (dsp->diag_rf_samples == 0) {
            fprintf(stderr, "decimation_index initial value = %d\n", dsp->decimation_index);
        }
        dsp->diag_rf_samples++;

        /* Mix down from RF center (APRS + offset) to baseband APRS channel.
         * Correct complex multiply: (i + jq) * (cos + j sin). */
        double cos_phase = cos(dsp->mixer_phase);
        double sin_phase = sin(dsp->mixer_phase);
        if (dsp->diag_rf_samples < 4) {
            fprintf(stderr, "mixer_phase[%lld]=%.6f cos=%.4f sin=%.4f\n",
                    (long long)dsp->diag_rf_samples, dsp->mixer_phase, cos_phase, sin_phase);
        }
        double base_i = i_sample * cos_phase - q_sample * sin_phase;
        double base_q = i_sample * sin_phase + q_sample * cos_phase;
        dsp->mixer_phase += dsp->mixer_phase_inc;
        if (dsp->mixer_phase > M_PI) {
            dsp->mixer_phase -= 2.0 * M_PI;
        } else if (dsp->mixer_phase < -M_PI) {
            dsp->mixer_phase += 2.0 * M_PI;
        }

        /* Simple complex DC blocker */
        dsp->dc_i += dsp->dc_alpha * (base_i - dsp->dc_i);
        dsp->dc_q += dsp->dc_alpha * (base_q - dsp->dc_q);
        base_i -= dsp->dc_i;
        base_q -= dsp->dc_q;

        /* FM discriminator using phase difference between successive complex samples.
         * prev_i/prev_q must be spaced by one AUDIO sample (i.e. decimated),
         * not by one RF sample, so only update prev_* for kept (decimated) samples. */

        /* Decimate RF-rate audio down to configured audio_sample_rate */
        if (dsp->decimation_factor > 1) {
            if (dsp->decimation_index++ % dsp->decimation_factor != 0) {
                continue;
            }
        }

        /* Now at an audio-rate sample. Compute phase difference vs previous
         * kept sample (prev_i/prev_q) and then update prev_* to this one. */
        if (dsp->prev_i == 0.0 && dsp->prev_q == 0.0) {
            dsp->prev_i = base_i;
            dsp->prev_q = base_q;
            continue;
        }
        /* c * conj(prev) using POST-downmix samples */
        double re = base_i * dsp->prev_i + base_q * dsp->prev_q;
        double im = base_q * dsp->prev_i - base_i * dsp->prev_q;
        if (dsp->diag_audio_samples < 8) {
            fprintf(stderr, "disc_check: prev_i=%.4f prev_q=%.4f base_i=%.4f base_q=%.4f\n",
                    dsp->prev_i, dsp->prev_q, base_i, base_q);
        }
        dsp->prev_i = base_i;
        dsp->prev_q = base_q;

        /* Phase difference is proportional to instantaneous frequency deviation (audio sample) */
        double audio_sample = atan2(im, re);
        if (dsp->diag_audio_samples < 8
            || (dsp->diag_audio_samples >= 280 && dsp->diag_audio_samples <= 285)) {
            double measured_freq = audio_sample * dsp->config.audio_sample_rate / (2.0 * M_PI);
            fprintf(stderr,
                    "verify[%lld]: base_i=%.4f base_q=%.4f prev_i=%.4f prev_q=%.4f "
                    "audio=%.6f freq=%.1fHz\n",
                    (long long)dsp->diag_audio_samples, base_i, base_q,
                    dsp->prev_i, dsp->prev_q, audio_sample, measured_freq);
        }

        /* Feed each audio sample into Goertzel; one decision per goertzel_block_size samples. */
        dsp->diag_audio_samples++;
        goertzel_process_sample(dsp, audio_sample);

        if (dsp->goertzel_sample_count >= dsp->goertzel_block_size) {
            if (dsp->diag_goertzel_blocks < 4) {
                fprintf(stderr, "Goertzel block[%lld] starts at RF sample %lld\n",
                        (long long)dsp->diag_goertzel_blocks, dsp->diag_rf_samples);
            }
            int raw_bit = goertzel_get_bit(dsp);
            if (dsp->diag_raw_bits < 32) {
                fprintf(stderr, "raw_bit[%lld]=%d\n",
                        (long long)dsp->diag_raw_bits, raw_bit);
            }
            int decoded_bit = nrzi_decode(raw_bit, &dsp->last_bit);
            dsp->diag_goertzel_blocks++;
            dsp->diag_raw_bits++;
            dsp->diag_decoded_bits++;
            process_ax25_bit(dsp, decoded_bit);
        }
    }

    {
        int frames = dsp->frames_decoded - frames_before;
        dbg(lvl_info,
            "APRS SDR DSP stats: rf_samples=%lld audio_samples=%lld decim_factor=%d samples_per_bit=%.0f "
            "goertzel_block=%d goertzel_blocks=%lld raw_bits=%lld decoded_bits=%lld flags_found=%lld frames=%d",
            dsp->diag_rf_samples, dsp->diag_audio_samples, dsp->decimation_factor, dsp->samples_per_bit,
            dsp->goertzel_block_size, dsp->diag_goertzel_blocks, dsp->diag_raw_bits, dsp->diag_decoded_bits,
            dsp->diag_flags_found, dsp->frames_decoded);
        fprintf(stderr,
                "APRS SDR DSP stats: rf_samples=%lld audio_samples=%lld decim_factor=%d samples_per_bit=%.0f "
                "goertzel_block=%d goertzel_blocks=%lld raw_bits=%lld decoded_bits=%lld flags_found=%lld frames=%d\n",
                dsp->diag_rf_samples, dsp->diag_audio_samples, dsp->decimation_factor, dsp->samples_per_bit,
                dsp->goertzel_block_size, dsp->diag_goertzel_blocks, dsp->diag_raw_bits, dsp->diag_decoded_bits,
                dsp->diag_flags_found, dsp->frames_decoded);
        return frames;
    }
}

int aprs_sdr_dsp_set_frame_callback(struct aprs_sdr_dsp *dsp, aprs_sdr_dsp_frame_callback callback, void *user_data) {
    if (!dsp)
        return 0;
    dsp->frame_callback = callback;
    dsp->frame_callback_user_data = user_data;
    return 1;
}

