#ifndef NAVIT_AUDIO_H
#define NAVIT_AUDIO_H
#include "glib_slice.h"
#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_PLAYBACK_TOGGLE -1
#define AUDIO_PLAYBACK_NEXT_TRACK -2
#define AUDIO_PLAYBACK_PREVIOUS_TRACK -3
#define AUDIO_PLAYBACK_NEXT_PLAYLIST -4
#define AUDIO_PLAYBACK_PREVIOUS_PLAYLIST -5
#define AUDIO_PLAYBACK_NEXT_ARTIST -6
#define AUDIO_PLAYBACK_PREVIOUS_ARTIST -7
#define AUDIO_MODE_TOGGLE_SHUFFLE -8
#define AUDIO_MODE_TOGGLE_REPEAT -9
#define AUDIO_MISC_DELETE_PLAYLIST -10
#define AUDIO_MISC_RELOAD_PLAYLISTS -11
#define AUDIO_PLAYBACK_PLAY -12
#define AUDIO_PLAYBACK_PAUSE -13

#define AUDIO_RAISE_VOLUME -1
#define AUDIO_LOWER_VOLUME -2
#define AUDIO_MUTE 0

#define STATUS_PLAYING 1
#define STATUS_RATING_POOR 2
#define STATUS_RATING_NORMAL 4
#define STATUS_RATING_GOOD 6

//#define AUDIO_MISC_DELETE_PLAYLIST -10
struct audio_priv;

struct audio_playlist {
	char * name;
	char * icon;
	int index;
	int status;
};

struct audio_track {
	char * name;
	char * icon;
	int index;
	int status;
};

/** actions which will be visible as a player toolbar in gui
 * 
 * */
struct audio_actions {
	char *icon;
	int action;
};


struct audio_methods {
	int (*volume)(struct audio_priv *this_, const int direction);
	int (*playback)(struct audio_priv  *this_, const int action);
	int (*action_do)(struct audio_priv  *this_, const int action);
	GList * (*tracks)(struct audio_priv *this_, int playlist_index);
	GList * (*playlists)(struct audio_priv *this_);
	GList * (*actions)(struct audio_priv *this_);
	char* (*current_track)(struct audio_priv *this_);
	char* (*current_playlist)(struct audio_priv *this_);
};

struct audio {
        char * name;
        struct audio_methods meth;
        struct audio_priv *priv;
};

struct audio * audio_new(struct attr *parent, struct attr **attrs);
//int change_volume(struct audio *this_, const int direction);
void audio_play_track(struct navit *this, int track_index);
GList * audio_get_playlists(struct navit *this);
GList * audio_get_tracks(struct navit *this, const int playlist_index);
void audio_set_volume(struct navit *this, int action);
void audio_do_action(struct navit *this, int action);
char *audio_get_current_playlist(struct navit *this);
char *audio_get_current_track(struct navit *this);

#ifdef __cplusplus
}
#endif
#endif
