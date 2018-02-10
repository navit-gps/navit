#include "debug.h"
#include "glib.h"
#include "alsa.h"
#include "item.h"
#include "navit.h"
#include "attr.h"
#include "audio.h"
#include "command.h"

#ifdef OSSCONTROL
#define MIXER_DEV "/dev/dsp"

#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <stdio.h>
#else
#include <alsa/asoundlib.h>
#endif


struct audio_priv {
	struct navit *nav;
};

char *card = "";
char *selem_name = "";

void
enumerate_devices()
{
	char **hints;
	int err = snd_device_name_hint(-1, "pcm", (void ***) &hints);
	if (err != 0) {
		dbg(lvl_error, "Can't enumerate audio devices\n");
	} else {
		char **n = hints;
		while (*n != NULL) {
			char *name = snd_device_name_get_hint(*n, "NAME");

			if (name != NULL && 0 != strcmp("null", name)) {
				dbg(lvl_info,
				    "Found audio playback device %s\n",
				    name);
				free(name);
			}
			n++;
		}
		snd_device_name_free_hint((void **) hints);
	}
}

void
audio_toggle_mute()
{
	long min, max;
	int value;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;

	snd_mixer_open(&handle, 0);
	if (!handle) {
		dbg(lvl_error, "snd_mixer_open failed for card %s\n",
		    card);
		return;
	}
	snd_mixer_elem_t *elem;
	for (elem = snd_mixer_first_elem(handle);
	     elem != NULL; elem = snd_mixer_elem_next(elem)) {
		dbg(lvl_error, "Found item %s : %d\n",
		    snd_mixer_selem_get_name(elem),
		    snd_mixer_selem_get_index(elem)
		    );
	}
	snd_mixer_attach(handle, card);
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	elem = snd_mixer_find_selem(handle, sid);
	if (elem) {
		snd_mixer_selem_get_playback_switch(elem,
						    SND_MIXER_SCHN_UNKNOWN,
						    &value);
		if (snd_mixer_selem_has_playback_switch(elem)) {
			snd_mixer_selem_set_playback_switch_all(elem,
								value ? 0 :
								1);
		}
	} else {
		dbg(lvl_error,
		    "snd_mixer_selem_get_playback_switch failed for card %s\n",
		    card);
		enumerate_devices();
	}

	snd_mixer_close(handle);
}

/*
  Drawbacks. Sets volume on both channels but gets volume on one. Can be easily adapted.
 */
int
audio_volume(audio_volume_action action, long *outvol)
{
	int ret = 0;
#ifdef OSSCONTROL
	int fd, devs;

	if ((fd = open(MIXER_DEV, O_WRONLY)) > 0) {
		if (action == AUDIO_VOLUME_SET) {
			if (*outvol < 0 || *outvol > 100)
				return -2;
			*outvol = (*outvol << 8) | *outvol;
			ioctl(fd, SOUND_MIXER_WRITE_VOLUME, outvol);
		} else if (action == AUDIO_VOLUME_GET) {
			ioctl(fd, SOUND_MIXER_READ_VOLUME, outvol);
			*outvol = *outvol & 0xff;
		}
		close(fd);
		return 0;
	}
	return -1;;
#else
	snd_mixer_t *handle;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;

	static int mix_index = 0;

	long pmin, pmax;
	long get_vol, set_vol;
	float f_multi;

	snd_mixer_selem_id_alloca(&sid);

	//sets simple-mixer index and name
	snd_mixer_selem_id_set_index(sid, mix_index);
	snd_mixer_selem_id_set_name(sid, selem_name);

	if ((snd_mixer_open(&handle, 0)) < 0) {
		dbg(lvl_error, "snd_mixer_open(&handle, 0) failed\n");
		return -1;
	}
	if ((snd_mixer_attach(handle, card)) < 0) {
		dbg(lvl_error, "snd_mixer_attach(handle, card) failed\n");
		snd_mixer_close(handle);
		return -2;
	}
	if ((snd_mixer_selem_register(handle, NULL, NULL)) < 0) {
		dbg(lvl_error,
		    "snd_mixer_selem_register(handle, NULL, NULL) failed\n");
		snd_mixer_close(handle);
		return -3;
	}
	ret = snd_mixer_load(handle);
	if (ret < 0) {
		dbg(lvl_error, "snd_mixer_load(handle  failed\n");
		snd_mixer_close(handle);
		return -4;
	}
	//snd_mixer_elem_t *elem;
	for (elem = snd_mixer_first_elem(handle);
	     elem != NULL; elem = snd_mixer_elem_next(elem)) {
		dbg(lvl_error, "Found item %s : %d\n",
		    snd_mixer_selem_get_name(elem),
		    snd_mixer_selem_get_index(elem)
		    );
	}
	elem = snd_mixer_find_selem(handle, sid);
	if (!elem) {
		snd_mixer_close(handle);
		return -5;
	}

	long minv, maxv;

	snd_mixer_selem_get_playback_volume_range(elem, &minv, &maxv);
	dbg(lvl_error, "Volume range <%li,%li>\n", minv, maxv);

	if (action == AUDIO_VOLUME_GET) {
		if (snd_mixer_selem_get_playback_volume(elem, 0, outvol) <
		    0) {
			snd_mixer_close(handle);
			dbg(lvl_error, "Failed to get volume\n");
			return -6;
		}

		dbg(lvl_error, "Get volume %li with status %i\n", *outvol,
		    ret);
		/* make the value bound to 100 */
		*outvol -= minv;
		maxv -= minv;
		minv = 0;
		*outvol = 100 * (*outvol) / maxv;	// make the value bound from 0 to 100
	} else if (action == AUDIO_VOLUME_SET) {
		// if(*outvol < 0 || *outvol > VOLUME_BOUND) // out of bounds
		if (*outvol < 0 || *outvol > 100)	// out of bounds
			return -7;
		*outvol = (*outvol * (maxv - minv) / (100 - 1)) + minv;

		dbg(lvl_error, "Setting volume to %li\n", outvol);

		if (snd_mixer_selem_set_playback_volume(elem, 0, *outvol) <
		    0) {
			snd_mixer_close(handle);
			return -8;
		}
		if (snd_mixer_selem_set_playback_volume(elem, 1, *outvol) <
		    0) {
			dbg(lvl_error, "About to crash?\n");
			snd_mixer_close(handle);

			return -9;
		}
		dbg(lvl_error, "Set volume %li with status %i\n", *outvol,
		    ret);
	}

	snd_mixer_close(handle);
	return 0;
#endif
}

int
alsa_volume(struct audio_priv *this, const int direction)
{
	dbg(lvl_error, "In the alsa_volume method\n");
	long vol = -1;
	int ret;
	if (direction) {
		dbg(lvl_error, "Changing volume direction %i\n",
		    direction);
		ret = audio_volume(AUDIO_VOLUME_GET, &vol);
		dbg(lvl_error, "Got get ret = %d\n", ret);
		vol += 10 * direction;
		audio_volume(AUDIO_VOLUME_SET, &vol);
		dbg(lvl_error, "Got set ret = %d\n", ret);
	} else {
		dbg(lvl_error, "Toggling volume mute\n");
		audio_toggle_mute();
	}
	return 0;
}

static struct audio_methods output_alsa_meth = {
	alsa_volume,
	NULL,			//playback 
	NULL,			//action_do, 
	NULL,			//tracks, 
	NULL,			//playlists, 
	NULL,			//actions, 
	NULL,			//current_track, 
	NULL,			//current_playlist, 

};

static struct audio_priv *
output_alsa_new(struct audio_methods *meth, struct callback_list *cbl,
		struct attr **attrs, struct attr *parent)
{
	struct audio_priv *this;
	struct attr *attr;
	if (!parent || parent->type != attr_navit)
		return NULL;
	this = g_new(struct audio_priv, 1);
	this->nav = parent->u.navit;
	*meth = output_alsa_meth;

	dbg(lvl_error, "Real alsa init\n");

	if ((attr = attr_search(attrs, NULL, attr_audio_device))) {
		card = g_strdup(attr->u.str);
		dbg(lvl_info, "Will use alsa device %s\n", card);
	}

	if ((attr = attr_search(attrs, NULL, attr_audio_device_mixer))) {
		selem_name = g_strdup(attr->u.str);
		dbg(lvl_info, "Will use alsa mixer %s\n", selem_name);
	}


	dbg(lvl_debug, "Real alsa init\n");
	enumerate_devices();
	return this;
}

void
plugin_init(void)
{
	dbg(lvl_debug, "output-alsa plugin init\n");
	plugin_register_category_audio("output-alsa", output_alsa_new);
}
