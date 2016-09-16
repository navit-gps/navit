#ifndef NAVIT_AUDIO_OUTPUT_ALSA_H
#define NAVIT_AUDIO_OUTPUT_ALSA_H

#include <pthread.h>
#include <stdint.h>
#include "audio/queue.h"

void enumerate_devices();

typedef enum {
    AUDIO_VOLUME_SET,
    AUDIO_VOLUME_GET,
} audio_volume_action;

#endif
