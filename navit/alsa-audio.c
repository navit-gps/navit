//* vim: set tabstop=4 expandtab: */
/*
 * Copyright (c) 2006-2009 Spotify Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *
 * ALSA audio output driver.
 *
 * This file is part of the libspotify examples suite.
 */

#include <alsa/asoundlib.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "debug.h"

#include "audio.h"


static snd_pcm_t *alsa_open(char *dev, int rate, int channels)
{
	snd_pcm_hw_params_t *hwp;
	snd_pcm_sw_params_t *swp;
	snd_pcm_t *h;
	int r;
	int dir;
	snd_pcm_uframes_t period_size_min;
	snd_pcm_uframes_t period_size_max;
	snd_pcm_uframes_t buffer_size_min;
	snd_pcm_uframes_t buffer_size_max;
	snd_pcm_uframes_t period_size;
	snd_pcm_uframes_t buffer_size;

	if ((r = snd_pcm_open(&h, dev, SND_PCM_STREAM_PLAYBACK, 0) < 0)) {
		dbg(lvl_error, "Something went wrong with %s\n", dev);
		return NULL;
	}

	hwp = alloca(snd_pcm_hw_params_sizeof());
	memset(hwp, 0, snd_pcm_hw_params_sizeof());
	snd_pcm_hw_params_any(h, hwp);

	snd_pcm_hw_params_set_access(h, hwp, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(h, hwp, SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_rate(h, hwp, rate, 0);
	snd_pcm_hw_params_set_channels(h, hwp, channels);

	/* Configurue period */

	dir = 0;
	snd_pcm_hw_params_get_period_size_min(hwp, &period_size_min, &dir);
	dir = 0;
	snd_pcm_hw_params_get_period_size_max(hwp, &period_size_max, &dir);

	period_size = 1024;

	dir = 0;
	r = snd_pcm_hw_params_set_period_size_near(h, hwp, &period_size, &dir);

	if (r < 0) {
		dbg(lvl_error, "audio: Unable to set period size %lu (%s)\n",
		        period_size, snd_strerror(r));
		snd_pcm_close(h);
		return NULL;
	}

	dir = 0;
	r = snd_pcm_hw_params_get_period_size(hwp, &period_size, &dir);

	if (r < 0) {
		dbg(lvl_error, "audio: Unable to get period size (%s)\n",
		        snd_strerror(r));
		snd_pcm_close(h);
		return NULL;
	}

	/* Configurue buffer size */

	snd_pcm_hw_params_get_buffer_size_min(hwp, &buffer_size_min);
	snd_pcm_hw_params_get_buffer_size_max(hwp, &buffer_size_max);
	buffer_size = period_size * 4;

	dir = 0;
	r = snd_pcm_hw_params_set_buffer_size_near(h, hwp, &buffer_size);

	if (r < 0) {
		dbg(lvl_error, "audio: Unable to set buffer size %lu (%s)\n",
		        buffer_size, snd_strerror(r));
		snd_pcm_close(h);
		return NULL;
	}

	r = snd_pcm_hw_params_get_buffer_size(hwp, &buffer_size);

	if (r < 0) {
		dbg(lvl_error, "audio: Unable to get buffer size (%s)\n",
		        snd_strerror(r));
		snd_pcm_close(h);
		return NULL;
	}

	/* write the hw params */
	r = snd_pcm_hw_params(h, hwp);

	if (r < 0) {
		dbg(lvl_error, "audio: Unable to configure hardware parameters (%s)\n",
		        snd_strerror(r));
		snd_pcm_close(h);
		return NULL;
	}

	/*
	 * Software parameters
	 */

	swp = alloca(snd_pcm_sw_params_sizeof());
	memset(hwp, 0, snd_pcm_sw_params_sizeof());
	snd_pcm_sw_params_current(h, swp);

	r = snd_pcm_sw_params_set_avail_min(h, swp, period_size);

	if (r < 0) {
		dbg(lvl_error, "audio: Unable to configure wakeup threshold (%s)\n",
		        snd_strerror(r));
		snd_pcm_close(h);
		return NULL;
	}

	snd_pcm_sw_params_set_start_threshold(h, swp, 0);

	if (r < 0) {
		dbg(lvl_error, "audio: Unable to configure start threshold (%s)\n",
		        snd_strerror(r));
		snd_pcm_close(h);
		return NULL;
	}

	r = snd_pcm_sw_params(h, swp);

	if (r < 0) {
		dbg(lvl_error, "audio: Cannot set soft parameters (%s)\n",
		snd_strerror(r));
		snd_pcm_close(h);
		return NULL;
	}

	r = snd_pcm_prepare(h);
	if (r < 0) {
		dbg(lvl_error, "audio: Cannot prepare audio for playback (%s)\n",
		snd_strerror(r));
		snd_pcm_close(h);
		return NULL;
	}

	return h;
}

static void* alsa_audio_start(void *aux)
{
	audio_fifo_t *af = aux;
	snd_pcm_t *h = NULL;
	int c;
	int cur_channels = 0;
	int cur_rate = 0;

	audio_fifo_data_t *afd;

	for (;;) {
		afd = audio_get(af);

		if (!h || cur_rate != afd->rate || cur_channels != afd->channels) {
			if (h) snd_pcm_close(h);

			cur_rate = afd->rate;
			cur_channels = afd->channels;

			h = alsa_open("front:CARD=Set,DEV=0", cur_rate, cur_channels);

			if (!h) {
				dbg(lvl_error, "Unable to open ALSA device (%d channels, %d Hz), can't continue\n", cur_channels, cur_rate);
                return;
			} else {
			    dbg(lvl_info, "ALSA device (%d channels, %d Hz), ok\n", cur_channels, cur_rate);
            }
		}

		c = snd_pcm_wait(h, 1000);

		if (c >= 0)
			c = snd_pcm_avail_update(h);

		if (c == -EPIPE)
			snd_pcm_prepare(h);

		snd_pcm_writei(h, afd->samples, afd->nsamples);
		free(afd);
	}
}

void audio_init(audio_fifo_t *af)
{
	pthread_t tid;

	TAILQ_INIT(&af->q);
	af->qlen = 0;

	pthread_mutex_init(&af->mutex, NULL);
	pthread_cond_init(&af->cond, NULL);

	pthread_create(&tid, NULL, alsa_audio_start, af);
}


void audio_toggle_mute()
{
    long min, max;
    int value;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Headphone";

    snd_mixer_open(&handle, 0);
    if(!handle) {
        dbg(lvl_error, "snd_mixer_open failed for card %s\n", card);
	return;
    }
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
    if(elem)
    {
        snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_UNKNOWN, &value);
        if (snd_mixer_selem_has_playback_switch(elem)) {
            snd_mixer_selem_set_playback_switch_all(elem, value ? 0 : 1);
        }
    } else {
        dbg(lvl_error, "snd_mixer_selem_get_playback_switch failed for card %s\n", card);
    }

    snd_mixer_close(handle);
}

/*
  Drawbacks. Sets volume on both channels but gets volume on one. Can be easily adapted.
 */
int audio_volume(audio_volume_action action, long* outvol)
{
    int ret = 0;
#ifdef OSSCONTROL
    int fd, devs;

    if ((fd = open(MIXER_DEV, O_WRONLY)) > 0)
    {
        if(action == AUDIO_VOLUME_SET) {
            if(*outvol < 0 || *outvol > 100)
                return -2;
            *outvol = (*outvol << 8) | *outvol;
            ioctl(fd, SOUND_MIXER_WRITE_VOLUME, outvol);
        }
        else if(action == AUDIO_VOLUME_GET) {
            ioctl(fd, SOUND_MIXER_READ_VOLUME, outvol);
            *outvol = *outvol & 0xff;
        }
        close(fd);
        return 0;
    }
    return -1;;
#else
    snd_mixer_t* handle;
    snd_mixer_elem_t* elem;
    snd_mixer_selem_id_t* sid;

    static const char* mix_name = "Headphone";
    static const char* card = "default";
    static int mix_index = 0;

    long pmin, pmax;
    long get_vol, set_vol;
    float f_multi;

    snd_mixer_selem_id_alloca(&sid);

    //sets simple-mixer index and name
    snd_mixer_selem_id_set_index(sid, mix_index);
    snd_mixer_selem_id_set_name(sid, mix_name);

        if ((snd_mixer_open(&handle, 0)) < 0)
        return -1;
    if ((snd_mixer_attach(handle, card)) < 0) {
        snd_mixer_close(handle);
        return -2;
    }
    if ((snd_mixer_selem_register(handle, NULL, NULL)) < 0) {
        snd_mixer_close(handle);
        return -3;
    }
    ret = snd_mixer_load(handle);
    if (ret < 0) {
        snd_mixer_close(handle);
        return -4;
    }
    elem = snd_mixer_find_selem(handle, sid);
    if (!elem) {
        snd_mixer_close(handle);
        return -5;
    }

    long minv, maxv;

    snd_mixer_selem_get_playback_volume_range (elem, &minv, &maxv);
    dbg(lvl_info, "Volume range <%i,%i>\n", minv, maxv);
    
    if(action == AUDIO_VOLUME_GET) {
        if(snd_mixer_selem_get_playback_volume(elem, 0, outvol) < 0) {
            snd_mixer_close(handle);
            printf("Failed to get volume\n");
            return -6;
        }

        dbg(lvl_info, "Get volume %i with status %i\n", *outvol, ret);
        /* make the value bound to 100 */
        *outvol -= minv;
        maxv -= minv;
        minv = 0;
        *outvol = 100 * (*outvol) / maxv; // make the value bound from 0 to 100
    }
    else if(action == AUDIO_VOLUME_SET) {
        // if(*outvol < 0 || *outvol > VOLUME_BOUND) // out of bounds
        if(*outvol < 0 || *outvol > 100) // out of bounds
            return -7;
        *outvol = (*outvol * (maxv - minv) / (100-1)) + minv;

        if(snd_mixer_selem_set_playback_volume(elem, 0, *outvol) < 0) {
            snd_mixer_close(handle);
            return -8;
        }
        if(snd_mixer_selem_set_playback_volume(elem, 1, *outvol) < 0) {
            snd_mixer_close(handle);
            return -9;
        }
        dbg(lvl_error, "Set volume %i with status %i\n", *outvol, ret);
    }

    snd_mixer_close(handle);
    return 0;
#endif
}
