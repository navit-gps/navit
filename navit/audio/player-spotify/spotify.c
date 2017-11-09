/* vim: set tabstop=8 expandtab: */
#include <glib.h>
#include "item.h"
#include <navit/main.h>
#include <navit/debug.h>
#include <libspotify/api.h>
#include <navit/callback.h>
#include <navit/event.h>
#include <navit/audio.h>
#include "spotify.h"

#define AUDIO_PLAYBACK_NEXT AUDIO_PLAYBACK_NEXT_TRACK
#define AUDIO_PLAYBACK_PREVIOUS AUDIO_PLAYBACK_PREVIOUS_TRACK


struct audio_priv {
	/* this is the data structure for the audio plugin
	 * you might not need every element of it
	 */
	struct navit *navit;
	struct callback *callback;
	struct event_idle *idle;
	struct callback_list *cbl;
	struct event_timeout *timeout;
	struct attr **attrs;
	GList *current_playlist;
	GList *playlists;
	GList *actions;
	gboolean random_track;
	gboolean random_playlist;
	gboolean repeat;
	gboolean single;
	gboolean shuffle;
	char current_track[64];
	int volume;
	int width;
	gboolean muted;
	int playing;
	char *login;
	char *password;
	char *playlist;
	sp_playlistcontainer *pc;
	char *audio_playback_pcm;

} *spotify;


const bool autostart = 0;

int current_playlist_index;
/// Handle to the playlist currently being played
static sp_playlist *g_jukeboxlist;
/// Handle to the current track
static sp_track *g_currenttrack;
/// Index to the next track
static int g_track_index;
static audio_fifo_t g_audiofifo;
static sp_session *g_sess;
int g_logged_in;
int next_timeout = 0;

extern const uint8_t spotify_apikey[];
extern const size_t spotify_apikey_size;


char *spotify_get_playlist_name(int playlist_index);
char *get_playlist_name(GList * list);
void spotify_play(void);
void spotify_pause(void);
void spotify_play_track(int track);
GList *get_entry_by_index(GList * list, int index);
void spotify_toggle_repeat(struct audio_actions *action);
void spotify_toggle_shuffle(struct audio_actions *action);
struct audio_actions *get_specific_action(GList * actions,
					  int specific_action);

/**
 * Get function for attributes
 *
 * @param priv Pointer to the audio instance data
 * @param type The attribute type to look for
 * @param attr Pointer to a {@code struct attr} to store the attribute
 * @return True for success, false for failure
 */
int
spotify_get_attr(struct audio_priv *priv, enum attr_type type,
		 struct attr *attr)
{
	int ret = 1;
	dbg(lvl_debug, "priv: %p, type: %i (%s), attr: %p\n", priv, type,
	    attr_to_name(type), attr);
	if (priv != spotify) {
		dbg(lvl_debug, "failed\n");
		return -1;
	}
	switch (type) {
	case attr_playing:{
			attr->u.num = spotify->playing;
			dbg(lvl_debug, "Status: %ld\n", attr->u.num);
			break;
		}
	case attr_name:{
			attr->u.str = g_strdup("spotify");
			dbg(lvl_debug, "%s\n", attr->u.str);
			break;
		}
	case attr_shuffle:{
			int toggle = 0;
			if (spotify->random_track)
				toggle++;
			if (spotify->random_playlist)
				toggle += 2;
			attr->u.num = toggle;
			dbg(lvl_debug, "%ld\n", attr->u.num);
			break;
		}
	case attr_repeat:{
			int toggle = 0;
			if (spotify->single)
				toggle++;
			if (spotify->repeat)
				toggle += 2;
			attr->u.num = toggle;
			dbg(lvl_debug, "%ld\n", attr->u.num);
			break;
		}
	default:{
			dbg(lvl_error,
			    "Don't know what to do with ATTR type %s\n",
			    attr_to_name(attr->type));
			ret = 0;
			break;
		}
	}
	attr->type = type;
	return ret;
}

/**
 * Set function for attributes
 *
 * @param priv An audio instance data
 * @param attr The attribute to set
 * @return False on success, true on failure
 */
int
spotify_set_attr(struct audio_priv *priv, struct attr *attr)
{
	dbg(lvl_debug, "priv: %p, type: %i (%s), attr: %p\n", priv,
	    attr->type, attr_to_name(attr->type), attr);
	if (priv != spotify) {
		dbg(lvl_debug, "failed\n");
		return -1;
	}
	if (attr) {
		switch (attr->type) {
		case attr_name:{
				dbg(lvl_debug, "%s\n", attr->u.str);
				break;
			}
		case attr_playing:{
				dbg(lvl_debug, "attr->u.num: %ld\n",
				    attr->u.num);

				if (attr->u.num == 0) {
					dbg(lvl_debug,
					    "spotify_pause();%ld\n",
					    attr->u.num);
					spotify_pause();
				} else {
					dbg(lvl_debug,
					    "spotify_play();%ld\n",
					    attr->u.num);
					spotify_play();
				}

				break;
			}
		case attr_shuffle:{

				spotify_toggle_shuffle(get_specific_action
						       (spotify->actions,
							AUDIO_MODE_TOGGLE_SHUFFLE));
				break;
			}
		case attr_repeat:{

				spotify_toggle_repeat(get_specific_action
						      (spotify->actions,
						       AUDIO_MODE_TOGGLE_REPEAT));
				break;
			}
		default:{
				dbg(lvl_error,
				    "Don't know what to do with ATTR type %s",
				    attr_to_name(attr->type));
				return 0;
				break;
			}
		}

		return 1;
	}
	return 0;
}

/**
 * @brief this function creates all possible actions for the player
 * 
 * @return the list of actions
 * 
 * Function to provide all possible audio actions for the player. These 
 * actions are acessible inside the media gui as a toolbar, so they 
 * might provide an icon. Their order will be applied to the toolbar too.
 * It is possible to change the icon depending on the actions state
 */
GList *
spotify_get_actions(void)
{
	/**
	 * this function creates all actions your player is able to perform
	 * except volume and choosing of a specific track
	 * just remove the actions you do not need 
	 * all listed actions will be put into a player toolbar with its icon
	 * so you should define one for each action
	 */
	GList *actions = NULL;
	struct audio_actions *aa;
	if (spotify->actions) {
		return spotify->actions;
	} else {
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_PREVIOUS_ARTIST;
		aa->icon = g_strdup("media_prev_artist");	//todo: make beautiful icon
		actions = g_list_append(actions, aa);

		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_PREVIOUS_PLAYLIST;
		aa->icon = g_strdup("media_prev");	//todo: make beautiful icon
		actions = g_list_append(actions, aa);

		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_PREVIOUS_TRACK;
		aa->icon = g_strdup("media_minus");	//todo: make beautiful icon
		actions = g_list_append(actions, aa);

		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_TOGGLE;
		if (spotify->playing) {
			aa->icon = g_strdup("media_pause");
		} else {
			aa->icon = g_strdup("media_play");
		}
		actions = g_list_append(actions, aa);

		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_NEXT_TRACK;
		aa->icon = g_strdup("media_plus");	//todo: make beautiful icon
		actions = g_list_append(actions, aa);

		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_NEXT_PLAYLIST;
		aa->icon = g_strdup("media_next");	//todo: make beautiful icon
		actions = g_list_append(actions, aa);

		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_NEXT_ARTIST;
		aa->icon = g_strdup("media_next_artist");	//todo: make beautiful icon
		actions = g_list_append(actions, aa);

		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_MODE_TOGGLE_REPEAT;
		aa->icon = g_strdup("media_repeat_off");	//todo: make beautiful icon
		actions = g_list_append(actions, aa);

		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_MODE_TOGGLE_SHUFFLE;
		aa->icon = g_strdup("media_shuffle_off");	//todo: make beautiful icon
		actions = g_list_append(actions, aa);

		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_MISC_DELETE_PLAYLIST;
		aa->icon = g_strdup("media_trash");	//todo: make beautiful icon
		actions = g_list_append(actions, aa);

		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_MISC_RELOAD_PLAYLISTS;
		aa->icon = g_strdup("media_close");	//todo: make beautiful icon
		actions = g_list_append(actions, aa);
	}
	return actions;
}



/**
* @brief this function returns the list of possible actions
*
* @param this the audio player object
*
* @return the list of actions
* 
* if there are no actions present, the command inits the action list
*/
GList *
actions(struct audio_priv * this)
{
	dbg(lvl_debug, "In spotify's actions\n");
	GList *act = NULL;
	act = spotify->actions;
	if (act) {
		return act;
	} else {
		spotify->actions = spotify_get_actions();
	}
	return spotify->actions;
}

/**
* @brief this function iterates over all possible actions for this player and searches for an action
*
* @param actions the list of actions 
* @param action the action we want to find
*
* @return the audio action object wh searched or NULL if its not present
*/
struct audio_actions *
get_specific_action(GList * actions, int specific_action)
{
	GList *result = g_list_first(actions);
	while (result != NULL && result->next != NULL) {
		struct audio_actions *aa = result->data;
		if (aa->action == specific_action)
			return aa;
		result = g_list_next(result);
	}
	return NULL;
}

/**
* @brief this function provides the action control for the audio player
*
* @param this the audio player object
* @param action the action to be performed on the player
*
* @return returns the action
* 
* possible actions:
* AUDIO_PLAYBACK_PLAY
* AUDIO_PLAYBACK_PAUSE
* AUDIO_PLAYBACK_TOGGLE
* AUDIO_PLAYBACK_NEXT_TRACK
* AUDIO_PLAYBACK_PREVIOUS_TRACK
* AUDIO_PLAYBACK_NEXT_PLAYLIST
* AUDIO_PLAYBACK_PREVIOUS_PLAYLIST
* AUDIO_PLAYBACK_NEXT_ARTIST: switches to the next playlist that differs before the Artist - Track delimiter " - "
* AUDIO_PLAYBACK_PREVIOUS_ARTIST: switches to the next playlist that differs before the Artist - Track delimiter " - " but backwards
* AUDIO_MISC_DELETE_PLAYLIST
* AUDIO_MODE_TOGGLE_REPEAT: switches through the repeat modes
* AUDIO_MODE_TOGGLE_SHUFFLE: switches through the shuffle modes
* AUDIO_MISC_RELOAD_PLAYLISTS: reload all playlists (delete and reload)
*/
static int
action_do(struct audio_priv *this, const int action)
{
	dbg(lvl_debug, "In spotify's action control\n");
	/** methosd where the defined actions are mentioned
	 * remove the case blocks for actions you do not need
	 */
	switch (action) {
	case AUDIO_PLAYBACK_PAUSE:{
			spotify_pause();
			break;
		}
	case AUDIO_PLAYBACK_PLAY:{
			spotify_play();
			break;
		}
	case AUDIO_PLAYBACK_TOGGLE:{
			spotify_toggle_playback(get_specific_action
						(spotify->actions,
						 AUDIO_PLAYBACK_TOGGLE));
			break;
		}
	case AUDIO_PLAYBACK_NEXT_TRACK:{
			++g_track_index;
			try_jukebox_start();
			break;
		}
	case AUDIO_PLAYBACK_PREVIOUS_TRACK:{
			if (g_track_index > 0) {
				--g_track_index;
				try_jukebox_start();
			}
			break;
		}
	case AUDIO_PLAYBACK_NEXT_PLAYLIST:{
			//spotify_next_playlist();
			break;
		}
	case AUDIO_PLAYBACK_PREVIOUS_PLAYLIST:{
			//spotify_prev_playlist();
			break;
		}
	case AUDIO_PLAYBACK_NEXT_ARTIST:{
			//spotify_next_artist();
			break;
		}
	case AUDIO_PLAYBACK_PREVIOUS_ARTIST:{
			//spotify_prev_artist();
			break;
		}
	case AUDIO_MISC_DELETE_PLAYLIST:{
			//spotify_delete_playlist();
			break;
		}
	case AUDIO_MODE_TOGGLE_REPEAT:{
			/* if your player has different repeat modes
			 * you can have different icons for each mode
			 */
			spotify_toggle_repeat(get_specific_action
					      (spotify->actions,
					       AUDIO_MODE_TOGGLE_REPEAT));
			break;
		}
	case AUDIO_MODE_TOGGLE_SHUFFLE:{
			/* if your player has different shuffle modes
			 * you can have different icons for each mode
			 */
			spotify_toggle_shuffle(get_specific_action
					       (spotify->actions,
						AUDIO_MODE_TOGGLE_SHUFFLE));
			break;
		}
	case AUDIO_MISC_RELOAD_PLAYLISTS:{
			/* maybe you'll never need this
			 */
			//reload_playlists(spotify);
			break;
		}
	default:{
			dbg(lvl_error,
			    "Don't know what to do with action '%i'. That's a bug\n",
			    action);
			break;
		}
	}
	return action;
}

/**
* @brief this function returns the currently playing trsck
*
* @param this the audio player object
*
* @return the track name of the current track
*/
char *
current_track(struct audio_priv *this)
{
	if (g_currenttrack) {
		return sp_track_name(g_currenttrack);
	} else {
		return "";
	}
}

/**
* @brief this function returns the currently loaded playlist
*
* @param this the audio player object
*
* @return the playlist name
*/
char *
current_playlist(struct audio_priv *this)
{
	if (g_jukeboxlist) {
		return sp_playlist_name(g_jukeboxlist);
	} else {
		return "";
	}
}



static void
try_jukebox_start(void)
{
	dbg(lvl_info, "Starting the jukebox\n");
	sp_track *t;
	spotify->playing = 0;

	if (!g_jukeboxlist) {
		dbg(lvl_info, "jukebox: No playlist. Waiting\n");
		return;
	}

	if (!sp_playlist_num_tracks(g_jukeboxlist)) {
		dbg(lvl_info, "jukebox: No tracks in playlist. Waiting\n");
		return;
	}

	if (sp_playlist_num_tracks(g_jukeboxlist) < g_track_index) {
		dbg(lvl_info,
		    "jukebox: No more tracks in playlist. Waiting\n");
		return;
	}

	t = sp_playlist_track(g_jukeboxlist, g_track_index);

	if (g_currenttrack && t != g_currenttrack) {
		/* Someone changed the current track */
		audio_fifo_flush(&g_audiofifo);
		sp_session_player_unload(g_sess);
		g_currenttrack = NULL;
	}

	if (!t)
		return;

	if (sp_track_error(t) != SP_ERROR_OK)
		return;

	if (g_currenttrack == t)
		return;

	g_currenttrack = t;

	dbg(lvl_info, "jukebox: Now playing \"%s\"... g_sess=%p\n",
	    sp_track_name(t), g_sess);

	// Ensure that the playlist is "offline" if we play a track
	sp_playlist_set_offline_mode(g_sess, g_jukeboxlist, 1);
	dbg(lvl_info, "Forced offline mode for the playlist\n");

	sp_session_player_load(g_sess, t);
	spotify->playing = 1;
	if (g_sess) {
		sp_session_player_play(g_sess, 1);
		dbg(lvl_info, "sp_session_player_play called\n");
	} else {
		dbg(lvl_error, "g_sess is null\n");
	}
}

/**
 * Callback from libspotify, saying that a track has been added to a playlist.
 *
 * @param  pl          The playlist handle
 * @param  tracks      An array of track handles
 * @param  num_tracks  The number of tracks in the \c tracks array
 * @param  position    Where the tracks were inserted
 * @param  userdata    The opaque pointer
 */
static void
tracks_added(sp_playlist * pl, sp_track * const *tracks, int num_tracks,
	     int position, void *userdata)
{
	dbg(lvl_info, "jukebox: %d tracks were added to %s\n", num_tracks,
	    sp_playlist_name(pl));

	if (!strcasecmp(sp_playlist_name(pl), spotify->playlist)) {
		g_jukeboxlist = pl;
		if (autostart)
			try_jukebox_start();
	}
}

/**
 * The callbacks we are interested in for individual playlists.
 */
static sp_playlist_callbacks pl_callbacks = {
	.tracks_added = &tracks_added,
//        .tracks_removed = &tracks_removed,
//        .tracks_moved = &tracks_moved,
//        .playlist_renamed = &playlist_renamed,
};

/**
 * Callback from libspotify, telling us a playlist was added to the playlist container.
 *
 * We add our playlist callbacks to the newly added playlist.
 *
 * @param  pc            The playlist container handle
 * @param  pl            The playlist handle
 * @param  position      Index of the added playlist
 * @param  userdata      The opaque pointer
 */
static void
playlist_added(sp_playlistcontainer * pc, sp_playlist * pl, int position,
	       void *userdata)
{
	sp_playlist_add_callbacks(pl, &pl_callbacks, NULL);
	dbg(lvl_info, "List name: %s\n", sp_playlist_name(pl));

	if (!strcasecmp(sp_playlist_name(pl), spotify->playlist)) {
		g_jukeboxlist = pl;
		current_playlist_index = position;
		if (autostart)
			try_jukebox_start();
	}
}

/**
 * Callback from libspotify, telling us the rootlist is fully synchronized
 * We can resume playback
 *
 * @param  pc            The playlist container handle
 * @param  userdata      The opaque pointer
 */
static void
container_loaded(sp_playlistcontainer * pc, void *userdata)
{
	dbg(lvl_info, "jukebox: Rootlist synchronized (%d playlists)\n",
	    sp_playlistcontainer_num_playlists(pc));
	// FIXME : should we implement autostart?
}

/**
 * The playlist container callbacks
 */
static sp_playlistcontainer_callbacks pc_callbacks = {
	.playlist_added = &playlist_added,
//        .playlist_removed = &playlist_removed,
	.container_loaded = &container_loaded,
};

static void
on_login(sp_session * session, sp_error error)
{
	if (error != SP_ERROR_OK) {
		dbg(lvl_error, "Error: unable to log in: %s\n",
		    sp_error_message(error));
		return;
	} else {
		dbg(lvl_info, "Logged in!\n");
	}

	g_logged_in = 1;
	spotify->pc = sp_session_playlistcontainer(session);
	sp_playlistcontainer_add_callbacks(spotify->pc, &pc_callbacks,
					   NULL);
}

static int
on_music_delivered(sp_session * session, const sp_audioformat * format,
		   const void *frames, int num_frames)
{
	audio_fifo_t *af = &g_audiofifo;
	audio_fifo_data_t *afd;
	size_t s;
	if (num_frames == 0)
		return 0;	// Audio discontinuity, do nothing
	pthread_mutex_lock(&af->mutex);
	/* Buffer one second of audio */
	if (af->qlen > format->sample_rate) {
		pthread_mutex_unlock(&af->mutex);
		return 0;
	}
	s = num_frames * sizeof(int16_t) * format->channels;
	afd = malloc(sizeof(*afd) + s);
	memcpy(afd->samples, frames, s);
	afd->nsamples = num_frames;
	afd->rate = format->sample_rate;
	afd->channels = format->channels;
	TAILQ_INSERT_TAIL(&af->q, afd, link);
	af->qlen += num_frames;
	pthread_cond_signal(&af->cond);
	pthread_mutex_unlock(&af->mutex);
	dbg(lvl_debug, "Delivery done\n");
	return num_frames;
}

static void
on_end_of_track(sp_session * session)
{
	dbg(lvl_debug, "end of track\n");
	++g_track_index;
	try_jukebox_start();
}

static sp_session_callbacks session_callbacks = {
	.logged_in = &on_login,
//  .notify_main_thread = &on_main_thread_notified,
	.music_delivery = &on_music_delivered,
//  .log_message = &on_log,
	.end_of_track = &on_end_of_track,
//  .offline_status_updated = &offline_status_updated,
//  .play_token_lost = &play_token_lost,
};

static sp_session_config spconfig = {
	.api_version = SPOTIFY_API_VERSION,
	.cache_location = "/var/tmp/spotify",
	.settings_location = "/var/tmp/spotify",
	.application_key = spotify_apikey,
	.application_key_size = 0,	// set in main()
	.user_agent = "navit",
	.callbacks = &session_callbacks,
	NULL
};

static void
spotify_spotify_idle(struct audio_priv *spotify)
{
	sp_session_process_events(g_sess, &next_timeout);
}


/**
* @brief this function toggles the repeat mode
*
* @param action the action that owns the toggle
*/
void
spotify_toggle_repeat(struct audio_actions *action)
{
	int toggle = 0;
	if (spotify->single)
		toggle++;
	if (spotify->repeat)
		toggle += 2;
	switch (toggle) {
	case 0:		// no repeat
	case 1:{
			spotify->single = 0;
			spotify->repeat = 1;
			if (action != NULL) {
				action->icon =
				    g_strdup("media_repeat_playlist");
			}
			dbg(lvl_debug, "\nrepeat playlist\n");
			break;
		}
	case 2:{		// repeat playlist
			spotify->single = 1;
			spotify->repeat = 1;
			if (action != NULL) {
				action->icon =
				    g_strdup("media_repeat_track");
			}
			dbg(lvl_debug, "\nrepeat track\n");
			break;
		}
	case 3:{		// repeat track
			spotify->single = 0;
			spotify->repeat = 0;
			if (action != NULL) {
				action->icon =
				    g_strdup("media_repeat_off");
			}
			dbg(lvl_debug, "\nrepeat off\n");
			break;
		}
	}
	callback_list_call_attr_0(spotify->cbl, attr_repeat);
}

/**
* @brief this function toggles the shuffle mode
*
* @param action the action that owns the toggle
*/
void
spotify_toggle_shuffle(struct audio_actions *action)
{

	int toggle = 0;
	if (spotify->random_track)
		toggle++;
	if (spotify->random_playlist)
		toggle += 2;
	dbg(lvl_debug, "Toggle Shuffle: %i\n", toggle);
	switch (toggle) {
	case 0:{

			spotify->random_track = TRUE;
			spotify->random_playlist = FALSE;
			if (action != NULL) {
				action->icon =
				    g_strdup("media_shuffle_playlists");
			}
			dbg(lvl_debug, "Toggle Shuffle Playlists: %i\n",
			    toggle);
			break;
		}
	case 1:{

			spotify->random_track = TRUE;
			spotify->random_playlist = TRUE;
			if (action != NULL) {
				action->icon =
				    g_strdup
				    ("media_shuffle_tracks_playlists");
			}
			dbg(lvl_debug,
			    "Toggle Shuffle Tracks & Playlists: %i\n",
			    toggle);
			break;
		}
	case 3:{

			spotify->random_track = FALSE;
			spotify->random_playlist = TRUE;
			if (action != NULL) {
				action->icon =
				    g_strdup("media_shuffle_tracks");
			}
			dbg(lvl_debug, "Toggle Shuffle Tracks: %i\n",
			    toggle);
			break;
		}
	case 2:{

			spotify->random_track = FALSE;
			spotify->random_playlist = FALSE;
			if (action != NULL) {
				action->icon =
				    g_strdup("media_shuffle_off");
			}
			dbg(lvl_debug, "Toggle Shuffle OFF: %i\n", toggle);
			break;
		}
	}
	callback_list_call_attr_0(spotify->cbl, attr_shuffle);
}

/**
* @brief command to toggle playback
*
* @param action the action that owns the toggle
*
* the action must be passed because the playback action keeps the icon. This icon changes dependent on the playback state
*/
void
spotify_toggle_playback(struct audio_actions *action)
{

	struct attr *playing =
	    attr_search(spotify->attrs, NULL, attr_playing);
	if (playing) {
		spotify_get_attr(spotify, attr_playing, playing);
		playing->u.num = spotify->playing;
		if (spotify_get_attr(spotify, attr_playing, playing)
		    && playing->u.num)
			spotify->playing = 1;
	} else
		dbg(lvl_debug, "No such Attr in: %p\n", spotify->attrs);
	if (spotify->playing) {
		dbg(lvl_debug, "pausing playback\n");
		// pause your playback here
		sp_session_player_play(g_sess, 0);
		if (action != NULL) {
			action->icon = g_strdup("media_play");
		}
	} else {
		dbg(lvl_debug, "resuming playback\n");
		// resume your playback here
		sp_session_player_play(g_sess, 1);
		try_jukebox_start();
		if (action != NULL) {
			action->icon = g_strdup("media_pause");
		}
	}
	spotify->playing = !spotify->playing;
	if (playing) {
		playing->u.num = spotify->playing;
		spotify_set_attr(spotify, playing);
	}
	callback_list_call_attr_0(spotify->cbl, attr_playing);
}

void
spotify_play()
{
	try_jukebox_start();
}

void
spotify_pause()
{
	try_jukebox_start();
}

GList *
playlists(struct audio_priv *this)
{
	GList *playlists = NULL;
	struct audio_playlist *pl;
	int i;
	dbg(lvl_error, "Listing playlists via Spotify\n");
	sp_playlistcontainer *pc = sp_session_playlistcontainer(g_sess);
	dbg(lvl_info, "get_playlists: Looking at %d playlists\n",
	    sp_playlistcontainer_num_playlists(pc));
	for (i = 0; i < sp_playlistcontainer_num_playlists(pc); ++i) {
		sp_playlist *spl = sp_playlistcontainer_playlist(pc, i);
		// fixme : should we enable this callback?
		// sp_playlist_add_callbacks(pl, &pl_callbacks, NULL);
		pl = g_new0(struct audio_playlist, 1);
		pl->name = g_strdup(sp_playlist_name(spl));
		pl->index = i;
		pl->status = 0;
		playlists = g_list_append(playlists, pl);

		switch (sp_playlist_get_offline_status(g_sess, spl)) {
		case SP_PLAYLIST_OFFLINE_STATUS_NO:
			pl->icon = "media_playlist_no_offline";
			break;

		case SP_PLAYLIST_OFFLINE_STATUS_YES:
			pl->icon = "media_playlist_offline";
			break;

		case SP_PLAYLIST_OFFLINE_STATUS_DOWNLOADING:
			pl->icon = "media_playlist_downloading";
			break;

		case SP_PLAYLIST_OFFLINE_STATUS_WAITING:
			pl->icon = "media_playlist_pending";
			break;

		default:
			pl->icon = "media_playlist_no_offline";
			break;
		}
	}
	return playlists;
}

GList *
tracks(struct audio_priv * this, int playlist_index)
{
	GList *tracks = NULL;
	struct audio_track *t;
	sp_playlist *spl;
	int i;
	if (playlist_index < 0) {
		playlist_index = current_playlist_index;
	}
	dbg(lvl_debug, "Spotify's tracks method\n");
	sp_playlistcontainer *pc = sp_session_playlistcontainer(g_sess);
	spl = sp_playlistcontainer_playlist(pc, playlist_index);
	for (i = 0; i < sp_playlist_num_tracks(spl); i++) {
		t = g_new0(struct audio_track, 1);
		sp_track *track = sp_playlist_track(spl, i);
		t->name = g_strdup(sp_track_name(track));
		t->index = i;
		t->status = 0;

		switch (sp_track_offline_get_status(track)) {
		case SP_TRACK_OFFLINE_DONE:
			t->icon = "media_track_offline_done";
			break;
		case SP_TRACK_OFFLINE_DOWNLOADING:
			t->icon = "media_track_downloading";
			break;
		case SP_TRACK_OFFLINE_NO:
			t->icon = "media_track_pending";
			break;
		default:
			t->icon = "media_track_offline";
		}
		t->icon =
		    g_strdup((i == g_track_index) ? "play" : t->icon);

		tracks = g_list_append(tracks, t);
	}
	g_jukeboxlist = spl;
	dbg(lvl_debug, "Active playlist updated\n");
	return tracks;
}

static int
playback(struct audio_priv *this, const int action)
{
	dbg(lvl_debug, "in spotify's playback control\n");
	switch (action) {
	case AUDIO_PLAYBACK_TOGGLE:
		spotify_toggle_playback(get_specific_action
					(spotify->actions,
					 AUDIO_PLAYBACK_TOGGLE));
		break;
	case AUDIO_PLAYBACK_NEXT:
		++g_track_index;
		try_jukebox_start();
		break;
	case AUDIO_PLAYBACK_PREVIOUS:
		if (g_track_index > 0)
			--g_track_index;
		try_jukebox_start();
		break;
	default:
		if (action < 0) {
			dbg(lvl_error,
			    "Don't know what to do with action '%i'. That's a bug\n",
			    action);
		} else {
			g_track_index = action;
			try_jukebox_start();
		}
	}
	return 0;
}

static struct audio_methods player_spotify_meth = {
	NULL,
	playback,
	action_do,
	tracks,
	playlists,
	actions,
	current_track,
	current_playlist,
	spotify_get_attr,
	spotify_set_attr,
};


static struct audio_priv *
player_spotify_new(struct audio_methods *meth, struct callback_list *cbl,
		   struct attr **attrs, struct attr *parent)
{
	struct audio_priv *this;
	struct attr *attr, *playing, *shuffle, *repeat;
	sp_error error;
	sp_session *session;
	attr = attr_search(attrs, NULL, attr_spotify_password);
	if (spotify_apikey_size == 0) {
		dbg(lvl_error,
		    "You need to set your spotify api key. Cannot initialize plugin\n");
		return NULL;
	}
	dbg(lvl_debug, "Initializing spotify\n");


	spotify = g_new0(struct audio_priv, 1);
	if ((attr = attr_search(attrs, NULL, attr_spotify_login))) {
		spotify->login = g_strdup(attr->u.str);
		dbg(lvl_info, "found spotify_login %s\n", spotify->login);
	}
	if ((attr = attr_search(attrs, NULL, attr_spotify_password))) {
		spotify->password = g_strdup(attr->u.str);
		dbg(lvl_info, "found spotify_password %s\n",
		    spotify->password);
	}
	if ((attr = attr_search(attrs, NULL, attr_spotify_playlist))) {
		spotify->playlist = g_strdup(attr->u.str);
		dbg(lvl_info, "found spotify_playlist %s\n",
		    spotify->playlist);
	}
	if ((attr = attr_search(attrs, NULL, attr_audio_playback_pcm))) {
		spotify->audio_playback_pcm = g_strdup(attr->u.str);
		dbg(lvl_info, "found audio playback pcm %s\n",
		    spotify->audio_playback_pcm);
	}
	spconfig.application_key_size = spotify_apikey_size;
	error = sp_session_create(&spconfig, &session);
	if (error != SP_ERROR_OK) {
		dbg(lvl_error, "Can't create spotify session :(\n");
		return NULL;
	}
	dbg(lvl_info, "Session created successfully :)\n");
	g_sess = session;
	g_logged_in = 0;
	sp_session_login(session, spotify->login, spotify->password, 0,
			 NULL);
	audio_init(&g_audiofifo, spotify->audio_playback_pcm);
	// FIXME : we should maybe use a timer instead of the idle loop

	spotify->callback =
	    callback_new_1(callback_cast(spotify_spotify_idle), spotify);
	spotify->timeout = event_add_timeout(1000, 1, spotify->callback);

	spotify->playing = FALSE;
	spotify->attrs = attrs;
	playing = attr_search(spotify->attrs, NULL, attr_playing);

	if (!playing) {
		playing = g_new0(struct attr, 1);
		playing->type = attr_playing;
		spotify->attrs =
		    attr_generic_add_attr(spotify->attrs, playing);
		dbg(lvl_debug, "*\n");
	}
	repeat = attr_search(spotify->attrs, NULL, attr_repeat);

	if (!repeat) {
		repeat = g_new0(struct attr, 1);
		repeat->type = attr_repeat;
		spotify->attrs =
		    attr_generic_add_attr(spotify->attrs, repeat);
		dbg(lvl_debug, "*\n");
	}
	shuffle = attr_search(spotify->attrs, NULL, attr_shuffle);

	if (!shuffle) {
		shuffle = g_new0(struct attr, 1);
		shuffle->type = attr_shuffle;
		spotify->attrs =
		    attr_generic_add_attr(spotify->attrs, shuffle);
		dbg(lvl_debug, "*\n");
	}


	spotify->callback =
	    callback_new_1(callback_cast(spotify_spotify_idle), spotify);
	event_add_idle(125, spotify->callback);
	dbg(lvl_info, "Callback created successfully\n");
	this = g_new(struct audio_priv, 1);

	*meth = player_spotify_meth;
	return this;
}


void
plugin_init(void)
{
	plugin_register_category_audio("player-spotify",
				       player_spotify_new);
}
