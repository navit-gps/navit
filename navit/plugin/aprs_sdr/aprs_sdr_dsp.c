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
 * 2. Baseband -> DC block, decimate to audio rate
 * 3. FM discriminator (phase difference) -> slow DC estimate -> centered FM for PLL
 * 4. Bit timing PLL (zero-crossings) + per-symbol average -> raw mark/space bit
 * 5. Bits -> NRZI decode
 * 6. Decoded bits -> AX.25 frame extraction
 *
 * Goertzel helpers remain for reference/testing but are not used on the main path.
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

    /* FM discriminator -> bit decision (audio-domain) */
    double bit_audio_sum;
    int bit_audio_count;

    /* FM output DC tracker: atan2 discriminator is offset positive; subtract estimate
     * so mark/space straddle 0 for PLL zero-crossings and threshold-at-0 decisions. */
    double fm_dc;
    double fm_dc_alpha;

    /* Bit timing PLL */
    double pll_phase;
    double pll_phase_inc;
    double pll_error;
    double pll_alpha;
    double pll_prev_sample;
    int pll_locked;
    int pll_transition_count;

    /* NRZI decoder state */
    int last_bit;

    /* AX.25 frame extraction */
    unsigned char *frame_buffer;
    int frame_buffer_size;
    int frame_buffer_pos;
    int in_frame;
    int bit_stuff_count;
    int flag_pos;
    int in_frame_bit_pos;
    unsigned char in_frame_current_byte;

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
    long long diag_goertzel_block;
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

static G_GNUC_UNUSED void goertzel_process_sample(struct aprs_sdr_dsp *dsp, double sample) {
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
        fprintf(stderr, "goertzel_input[blk7,sample%d]=%.6f\n", dsp->goertzel_sample_count - 1, sample);
    }
}

static G_GNUC_UNUSED int goertzel_get_bit(struct aprs_sdr_dsp *dsp) {
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
        fprintf(stderr, "goertzel[%d]: mark_power=%.6f space_power=%.6f raw_bit=%d\n", goertzel_dump, mark_power,
                space_power, raw_bit);
        goertzel_dump++;
    }
    if (dsp->diag_goertzel_blocks >= 5 && dsp->diag_goertzel_blocks <= 10) {
        fprintf(stderr, "goertzel_blk[%lld]: mark=%.6f space=%.6f raw=%d\n", (long long)dsp->diag_goertzel_blocks,
                mark_power, space_power, raw_bit);
    }
    return raw_bit;
}

static int nrzi_decode(int bit, int *last_bit) {
    /* NRZI: bit is 1 if no transition, 0 if transition */
    int decoded = (bit == *last_bit) ? 1 : 0;
    *last_bit = bit;
    return decoded;
}

/* Look for the AX.25/HDLC flag pattern (0x7E, LSB-first).
 *
 * Important:
 * - This runs on the NRZI-decoded *data* bits.
 * - Do not keep state as static locals; this DSP is used in tests and can be
 *   instantiated multiple times.
 * - When a flag is found we enter in-frame mode, but we still expect to see
 *   additional 0x7E bytes (preamble flush) before real payload starts. */
static void dsp_log_ax25_in_frame_debug(struct aprs_sdr_dsp *dsp, int bit) {
    if (dsp->frame_buffer_pos == 0 && dsp->in_frame_bit_pos < 8) {
        fprintf(stderr, "ASSEMBLE decoded=%lld bit=%d bit_pos=%d\n", (long long)dsp->diag_decoded_bits, bit,
                dsp->in_frame_bit_pos);
    }
    if (dsp->frame_buffer_pos == 1 && dsp->in_frame_bit_pos < 8) {
        fprintf(stderr, "ASSEMBLE2 decoded=%lld bit=%d bit_pos=%d\n", (long long)dsp->diag_decoded_bits, bit,
                dsp->in_frame_bit_pos);
    }
    if (dsp->frame_buffer_pos == 0 && dsp->in_frame_bit_pos < 8) {
        fprintf(stderr, "ASSEMBLE_POST_FLAG decoded=%lld bit=%d bit_pos=%d\n", (long long)dsp->diag_decoded_bits, bit,
                dsp->in_frame_bit_pos);
    }
    if (dsp->frame_buffer_pos == 0 && dsp->in_frame_bit_pos == 0 && dsp->in_frame_current_byte == 0) {
        fprintf(stderr, "ASSEMBLE_POST_FLAG_RESET decoded=%lld\n", (long long)dsp->diag_decoded_bits);
    }
    if (dsp->frame_buffer_pos == 0 && dsp->in_frame_bit_pos < 8) {
        fprintf(stderr, "ASSEMBLE_POST_FLAG_LAST decoded=%lld last_bit=%d\n", (long long)dsp->diag_decoded_bits,
                dsp->last_bit);
    }
    fprintf(stderr, "in_frame_bit: decoded=%lld bit=%d bit_pos=%d stuff=%d buf_pos=%d\n",
            (long long)dsp->diag_decoded_bits, bit, dsp->in_frame_bit_pos, dsp->bit_stuff_count, dsp->frame_buffer_pos);
}

/* Returns 1 if this bit is a stuffed zero to discard (caller should return). */
static int ax25_bit_stuffing_discards_bit(struct aprs_sdr_dsp *dsp, int bit) {
    if (bit == 1) {
        dsp->bit_stuff_count++;
        return 0;
    }
    if (dsp->bit_stuff_count == 5) {
        fprintf(stderr, "DISCARD stuffed zero at decoded=%lld bit_pos=%d\n", (long long)dsp->diag_decoded_bits,
                dsp->in_frame_bit_pos);
        dsp->bit_stuff_count = 0;
        return 1;
    }
    dsp->bit_stuff_count = 0;
    return 0;
}

static void ax25_deliver_closing_frame(struct aprs_sdr_dsp *dsp) {
    dsp->diag_flags_found++;
    if (dsp->frame_callback && dsp->frame_buffer_pos > 2) {
        fprintf(stderr, "frame_callback: length=%d first4=%02X %02X %02X %02X\n", dsp->frame_buffer_pos - 2,
                dsp->frame_buffer[0], dsp->frame_buffer[1], dsp->frame_buffer[2], dsp->frame_buffer[3]);
        dsp->frame_callback(dsp->frame_buffer, dsp->frame_buffer_pos - 2, dsp->frame_callback_user_data);
    }
    dsp->frames_decoded++;
    dsp->in_frame = 0;
    dsp->frame_buffer_pos = 0;
    dsp->bit_stuff_count = 0;
    dsp->in_frame_bit_pos = 0;
    dsp->in_frame_current_byte = 0;
}

static void ax25_reset_for_preamble_flag(struct aprs_sdr_dsp *dsp) {
    dsp->frame_buffer_pos = 0;
    dsp->bit_stuff_count = 0;
    dsp->in_frame_bit_pos = 0;
    dsp->in_frame_current_byte = 0;
}

static void ax25_handle_completed_byte(struct aprs_sdr_dsp *dsp, unsigned char completed_byte) {
    fprintf(stderr, "frame_byte[%d]=0x%02X (bit_pos=8)\n", dsp->frame_buffer_pos, completed_byte);

    if (completed_byte != 0x7E) {
        if (dsp->frame_buffer_pos < dsp->frame_buffer_size) {
            dsp->frame_buffer[dsp->frame_buffer_pos++] = completed_byte;
        }
        return;
    }
    if (dsp->frame_buffer_pos >= 15) {
        ax25_deliver_closing_frame(dsp);
    } else {
        ax25_reset_for_preamble_flag(dsp);
    }
}

static void process_ax25_flag_search(struct aprs_sdr_dsp *dsp, int bit) {
    static int flag_pattern[] = {0, 1, 1, 1, 1, 1, 1, 0};

    fprintf(stderr, "FLAG_SEARCH decoded=%lld bit=%d flag_pos=%d\n", (long long)dsp->diag_decoded_bits, bit,
            dsp->flag_pos);

    if (bit == flag_pattern[dsp->flag_pos]) {
        dsp->flag_pos++;
        if (dsp->flag_pos == 8) {
            fprintf(stderr, "FLAG_FOUND at decoded=%lld\n", (long long)dsp->diag_decoded_bits);
            fprintf(stderr, "FLAG_FOUND at decoded=%lld goertzel_blk=%lld\n", (long long)dsp->diag_decoded_bits,
                    (long long)dsp->diag_goertzel_block);
            fprintf(stderr, "FLAG_FOUND last_bit=%d\n", dsp->last_bit);
            dsp->in_frame = 1;
            dsp->frame_buffer_pos = 0;
            dsp->bit_stuff_count = 0;
            dsp->in_frame_bit_pos = 0;
            dsp->in_frame_current_byte = 0;
            dsp->diag_flags_found++;
            dsp->flag_pos = 0;
            return;
        }
    } else {
        fprintf(stderr, "FLAG_RESET at decoded=%lld bit=%d expected=%d\n", (long long)dsp->diag_decoded_bits, bit,
                flag_pattern[dsp->flag_pos]);
        dsp->flag_pos = (bit == flag_pattern[0]) ? 1 : 0;
    }
}

/* Handle bit stuffing and byte accumulation while in-frame.
 *
 * Critical boundary condition:
 * - Bit de-stuffing applies only to the payload bitstream between flags.
 * - If a stuffed 0 is discarded while assembling a would-be flag byte, the
 *   decoder slips by one bit and the next payload byte will be wrong (classic:
 *   0x45 instead of 0x82 in the integration test).
 *
 * The implementation relies on preamble flush handling (0x7E with small
 * frame_buffer_pos) to keep the byte boundary aligned before payload begins. */
static void process_ax25_in_frame(struct aprs_sdr_dsp *dsp, int bit) {
    dsp_log_ax25_in_frame_debug(dsp, bit);

    if (ax25_bit_stuffing_discards_bit(dsp, bit)) {
        return;
    }

    dsp->in_frame_current_byte |= (unsigned char)((bit & 1) << dsp->in_frame_bit_pos);
    dsp->in_frame_bit_pos++;

    if (dsp->in_frame_bit_pos < 8) {
        return;
    }

    {
        unsigned char completed_byte = dsp->in_frame_current_byte;
        dsp->in_frame_bit_pos = 0;
        dsp->in_frame_current_byte = 0;
        ax25_handle_completed_byte(dsp, completed_byte);
    }
}

static void process_ax25_bit(struct aprs_sdr_dsp *dsp, int bit) {
    static int bit_dump_count = 0;
    if (dsp->in_frame && dsp->frame_buffer_pos == 0) {
        fprintf(stderr, "AX25_BIT_INFRAME decoded=%lld bit=%d\n", (long long)dsp->diag_decoded_bits, bit);
    }
    if (bit_dump_count < 32) {
        fprintf(stderr, "DSP bit[%d]=%d\n", bit_dump_count, bit);
        bit_dump_count++;
    }
    fprintf(stderr, "ax25_bit: pos=%lld bit=%d in_frame=%d\n", (long long)dsp->diag_decoded_bits, bit, dsp->in_frame);
    if (!dsp->in_frame) {
        process_ax25_flag_search(dsp, bit);
        return;
    }
    process_ax25_in_frame(dsp, bit);
}

static void aprs_sdr_dsp_normalize_config(struct aprs_sdr_dsp *dsp) {
    if (dsp->config.mark_freq == 0.0) {
        dsp->config.mark_freq = BELL202_MARK_FREQ;
    }
    if (dsp->config.space_freq == 0.0) {
        dsp->config.space_freq = BELL202_SPACE_FREQ;
    }
    if (dsp->config.baud_rate == 0.0) {
        dsp->config.baud_rate = BELL202_BAUD_RATE;
    }

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

    dsp->samples_per_bit = dsp->config.audio_sample_rate / dsp->config.baud_rate;
    dsp->goertzel_block_size = (int)(dsp->samples_per_bit + 0.5);
}

static void aprs_sdr_dsp_init_demod_state(struct aprs_sdr_dsp *dsp) {
    dsp->bit_phase = 0;
    dsp->bit_accumulator = 0.0;
    dsp->bit_audio_sum = 0.0;
    dsp->bit_audio_count = 0;
    dsp->fm_dc = 0.0;
    dsp->fm_dc_alpha = 0.02;

    dsp->pll_phase = 0.0;
    dsp->pll_phase_inc = 1.0 / dsp->samples_per_bit;
    dsp->pll_error = 0.0;
    dsp->pll_alpha = dsp->config.pll_alpha;
    dsp->pll_prev_sample = 0.0;
    dsp->pll_locked = 0;
    dsp->pll_transition_count = 0;

    dsp->last_bit = 1;
    dsp->in_frame = 0;
    dsp->frame_buffer_pos = 0;
    dsp->bit_stuff_count = 0;
    dsp->flag_pos = 0;
    dsp->in_frame_bit_pos = 0;
    dsp->in_frame_current_byte = 0;
    dsp->frames_decoded = 0;
    dsp->bit_errors = 0;
}

static void aprs_sdr_dsp_init_rf_chain(struct aprs_sdr_dsp *dsp) {
    dsp->mixer_phase = 0.0;
    if (dsp->config.rf_sample_rate > 0) {
        dsp->mixer_phase_inc = -2.0 * M_PI * dsp->config.if_offset_hz / (double)dsp->config.rf_sample_rate;
        fprintf(stderr, "mixer: phase_inc=%.10f if_offset=%.0f rf_rate=%d expected_phase_inc=%.10f\n",
                dsp->mixer_phase_inc, dsp->config.if_offset_hz, dsp->config.rf_sample_rate,
                -2.0 * M_PI * dsp->config.if_offset_hz / (double)dsp->config.rf_sample_rate);
    } else {
        dsp->mixer_phase_inc = 0.0;
    }
    dsp->prev_i = 0.0;
    dsp->prev_q = 0.0;
    dsp->dc_i = 0.0;
    dsp->dc_q = 0.0;
    dsp->dc_alpha = 0.001;
    dsp->decimation_index = 0;
}

static void aprs_sdr_dsp_clear_diagnostics(struct aprs_sdr_dsp *dsp) {
    dsp->diag_rf_samples = 0;
    dsp->diag_audio_samples = 0;
    dsp->diag_goertzel_blocks = 0;
    dsp->diag_goertzel_block = 0;
    dsp->diag_raw_bits = 0;
    dsp->diag_decoded_bits = 0;
    dsp->diag_flags_found = 0;
}

struct aprs_sdr_dsp *aprs_sdr_dsp_new(const struct aprs_sdr_dsp_config *config) {
    struct aprs_sdr_dsp *dsp;

    if (!config) {
        dbg(lvl_error, "DSP config is NULL");
        return NULL;
    }

    dsp = g_new0(struct aprs_sdr_dsp, 1);
    dsp->config = *config;

    aprs_sdr_dsp_normalize_config(dsp);
    goertzel_init(dsp);
    dsp->goertzel_sample_count = 20;

    dsp->frame_buffer_size = 512;
    dsp->frame_buffer = g_malloc(dsp->frame_buffer_size);

    aprs_sdr_dsp_init_demod_state(dsp);
    aprs_sdr_dsp_init_rf_chain(dsp);
    aprs_sdr_dsp_clear_diagnostics(dsp);

    fprintf(stderr, "DSP created: mark_freq=%.0f space_freq=%.0f if_offset=%.0f rf_rate=%d audio_rate=%d\n",
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

static void dsp_mix_to_baseband(struct aprs_sdr_dsp *dsp, double i_sample, double q_sample, double *base_i,
                                double *base_q) {
    double cos_phase = cos(dsp->mixer_phase);
    double sin_phase = sin(dsp->mixer_phase);
    if (dsp->diag_rf_samples < 4) {
        fprintf(stderr, "mixer_phase[%lld]=%.6f cos=%.4f sin=%.4f\n", (long long)dsp->diag_rf_samples,
                dsp->mixer_phase, cos_phase, sin_phase);
    }
    *base_i = i_sample * cos_phase - q_sample * sin_phase;
    *base_q = i_sample * sin_phase + q_sample * cos_phase;
    dsp->mixer_phase += dsp->mixer_phase_inc;
    if (dsp->mixer_phase > M_PI) {
        dsp->mixer_phase -= 2.0 * M_PI;
    } else if (dsp->mixer_phase < -M_PI) {
        dsp->mixer_phase += 2.0 * M_PI;
    }
}

static void dsp_dc_block_complex(struct aprs_sdr_dsp *dsp, double *base_i, double *base_q) {
    dsp->dc_i += dsp->dc_alpha * (*base_i - dsp->dc_i);
    dsp->dc_q += dsp->dc_alpha * (*base_q - dsp->dc_q);
    *base_i -= dsp->dc_i;
    *base_q -= dsp->dc_q;
}

static int dsp_should_skip_decimated_sample(struct aprs_sdr_dsp *dsp) {
    if (dsp->decimation_factor <= 1) {
        return 0;
    }
    return (dsp->decimation_index++ % dsp->decimation_factor != 0);
}

static double dsp_discriminator_audio(struct aprs_sdr_dsp *dsp, double base_i, double base_q) {
    if (dsp->prev_i == 0.0 && dsp->prev_q == 0.0) {
        dsp->prev_i = base_i;
        dsp->prev_q = base_q;
        return 0.0;
    }
    {
        double re = base_i * dsp->prev_i + base_q * dsp->prev_q;
        double im = base_q * dsp->prev_i - base_i * dsp->prev_q;
        if (dsp->diag_audio_samples < 8) {
            fprintf(stderr, "disc_check: prev_i=%.4f prev_q=%.4f base_i=%.4f base_q=%.4f\n", dsp->prev_i,
                    dsp->prev_q, base_i, base_q);
        }
        dsp->prev_i = base_i;
        dsp->prev_q = base_q;
        return atan2(im, re);
    }
}

static void dsp_log_audio_verify(struct aprs_sdr_dsp *dsp, double base_i, double base_q, double audio_sample) {
    if (dsp->diag_audio_samples < 8 || (dsp->diag_audio_samples >= 280 && dsp->diag_audio_samples <= 285)) {
        double measured_freq = audio_sample * dsp->config.audio_sample_rate / (2.0 * M_PI);
        fprintf(stderr,
                "verify[%lld]: base_i=%.4f base_q=%.4f prev_i=%.4f prev_q=%.4f "
                "audio=%.6f freq=%.1fHz\n",
                (long long)dsp->diag_audio_samples, base_i, base_q, dsp->prev_i, dsp->prev_q, audio_sample,
                measured_freq);
    }
}

static void dsp_pll_on_zero_crossing(struct aprs_sdr_dsp *dsp) {
    dsp->pll_transition_count++;
    if (dsp->pll_transition_count >= 8) {
        dsp->pll_locked = 1;
    }
    if (dsp->pll_locked) {
        dsp->pll_error = dsp->pll_phase - 0.5;
        dsp->pll_phase -= dsp->pll_alpha * dsp->pll_error;
        if (dsp->pll_phase < 0.0) {
            dsp->pll_phase += 1.0;
        }
        if (dsp->pll_phase >= 1.0) {
            dsp->pll_phase -= 1.0;
        }
    }
}

static void dsp_emit_symbol_bit(struct aprs_sdr_dsp *dsp) {
    if (dsp->bit_audio_count <= 0) {
        return;
    }
    {
        double avg = dsp->bit_audio_sum / (double)dsp->bit_audio_count;
        int raw_bit = (avg < 0.0) ? 1 : 0;
        int decoded_bit = nrzi_decode(raw_bit, &dsp->last_bit);

        dsp->bit_audio_sum = 0.0;
        dsp->bit_audio_count = 0;

        dsp->diag_goertzel_block++;
        dsp->diag_goertzel_blocks++;
        dsp->diag_raw_bits++;
        dsp->diag_decoded_bits++;
        process_ax25_bit(dsp, decoded_bit);
    }
}

static void dsp_feed_pll_and_decode(struct aprs_sdr_dsp *dsp, double audio_sample) {
    dsp->fm_dc += dsp->fm_dc_alpha * (audio_sample - dsp->fm_dc);
    {
        double pll_sample = audio_sample - dsp->fm_dc;

        dsp->bit_audio_sum += pll_sample;
        dsp->bit_audio_count++;

        if ((dsp->pll_prev_sample < 0.0 && pll_sample >= 0.0)
            || (dsp->pll_prev_sample >= 0.0 && pll_sample < 0.0)) {
            dsp_pll_on_zero_crossing(dsp);
        }
        dsp->pll_prev_sample = pll_sample;

        dsp->pll_phase += dsp->pll_phase_inc;

        if (dsp->pll_phase >= 1.0) {
            dsp->pll_phase -= 1.0;
            dsp_emit_symbol_bit(dsp);
        }
    }
}

static int dsp_log_and_return_frames(struct aprs_sdr_dsp *dsp, int frames_before) {
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

int aprs_sdr_dsp_process_samples(struct aprs_sdr_dsp *dsp, const unsigned char *samples, int length) {
    int frames_before = dsp->frames_decoded;
    int i;

    if (!dsp || !samples || length < 2) {
        return 0;
    }

    for (i = 0; i < length - 1; i += 2) {
        double i_sample = (double)((int)samples[i] - 128) / 128.0;
        double q_sample = (double)((int)samples[i + 1] - 128) / 128.0;
        double base_i;
        double base_q;
        double audio_sample;

        if (dsp->diag_rf_samples == 0) {
            fprintf(stderr, "decimation_index initial value = %d\n", dsp->decimation_index);
        }
        dsp->diag_rf_samples++;

        dsp_mix_to_baseband(dsp, i_sample, q_sample, &base_i, &base_q);
        dsp_dc_block_complex(dsp, &base_i, &base_q);

        if (dsp_should_skip_decimated_sample(dsp)) {
            continue;
        }

        audio_sample = dsp_discriminator_audio(dsp, base_i, base_q);
        dsp_log_audio_verify(dsp, base_i, base_q, audio_sample);
        dsp->diag_audio_samples++;
        dsp_feed_pll_and_decode(dsp, audio_sample);
    }

    return dsp_log_and_return_frames(dsp, frames_before);
}

int aprs_sdr_dsp_set_frame_callback(struct aprs_sdr_dsp *dsp, aprs_sdr_dsp_frame_callback callback, void *user_data) {
    if (!dsp)
        return 0;
    dsp->frame_callback = callback;
    dsp->frame_callback_user_data = user_data;
    return 1;
}

int aprs_sdr_dsp_pll_locked(const struct aprs_sdr_dsp *dsp) {
    return (dsp && dsp->pll_locked) ? 1 : 0;
}

int aprs_sdr_dsp_pll_transition_count(const struct aprs_sdr_dsp *dsp) {
    return dsp ? dsp->pll_transition_count : 0;
}

double aprs_sdr_dsp_pll_phase(const struct aprs_sdr_dsp *dsp) {
    return dsp ? dsp->pll_phase : 0.0;
}
