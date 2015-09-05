#ifndef NAVIT_AUDIO_H
#define NAVIT_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

// const int AUDIO_PLAYBACK_TOGGLE=0;
// const int AUDIO_PLAYBACK_NEXT=1;
// const int AUDIO_PLAYBACK_PREVIOUS=-1;

struct audio_priv;

struct audio_methods {
	int (*volume)(struct audio_priv *this_, const int direction);
	int (*playback)(struct audio_priv  *this_, const int action);
	int (*playlists)(struct audio_priv *this_, const int action);
};

struct audio {
        char * name;
        struct audio_methods meth;
        struct audio_priv *priv;
};

struct audio * audio_new(struct attr *parent, struct attr **attrs);
int change_volume(struct audio *this_, const int direction);
int playback_do(struct audio *this_, const int action);

#ifdef __cplusplus
}
#endif
#endif
