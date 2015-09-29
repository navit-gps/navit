#ifndef NAVIT_AUDIO_H
#define NAVIT_AUDIO_H
#include "glib_slice.h"
#ifdef __cplusplus
extern "C" {
#endif

// const int AUDIO_PLAYBACK_TOGGLE=0;
// const int AUDIO_PLAYBACK_NEXT=1;
// const int AUDIO_PLAYBACK_PREVIOUS=-1;

struct audio_priv;

struct audio_playlist {
	char * name;
	int index;
	int status;
};

struct audio_track {
	char * name;
	int index;
	int status;
};


struct audio_methods {
	int (*volume)(struct audio_priv *this_, const int direction);
	int (*playback)(struct audio_priv  *this_, const int action);
	GList * (*tracks)(struct audio_priv *this_);
	GList * (*playlists)(struct audio_priv *this_);
};

struct audio {
        char * name;
        struct audio_methods meth;
        struct audio_priv *priv;
};

struct audio * audio_new(struct attr *parent, struct attr **attrs);
int change_volume(struct audio *this_, const int direction);
int audio_play_track(struct navit *this, int track_index);
GList * audio_get_playlists(struct navit *this);
GList * audio_get_tracks(struct navit *this, const int playlist_index);

#ifdef __cplusplus
}
#endif
#endif
