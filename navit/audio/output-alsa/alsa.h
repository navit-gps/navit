#ifndef NAVIT_AUDIO_OUTPUT_ALSA_H
#define NAVIT_AUDIO_OUTPUT_ALSA_H

#include <pthread.h>
#include <stdint.h>
#include "queue.h"

void enumerate_devices();

typedef enum {
    AUDIO_VOLUME_SET,
    AUDIO_VOLUME_GET,
} audio_volume_action;

typedef struct audio_fifo_data {
        TAILQ_ENTRY(audio_fifo_data) link;
        int channels;
        int rate;
        int nsamples;
        int16_t samples[0];
} audio_fifo_data_t;

typedef struct audio_fifo {
        TAILQ_HEAD(, audio_fifo_data) q;
        int qlen;
        pthread_mutex_t mutex;
        pthread_cond_t cond;
} audio_fifo_t;

#endif
