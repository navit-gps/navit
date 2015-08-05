/* vim: set tabstop=4 expandtab: */
#include <glib.h>
#include <navit/main.h>
#include <navit/debug.h>
#include <navit/navit.h>
#include <navit/callback.h>
#include <navit/event.h>

#include "time.h"
#include <libspotify/api.h>
#include "audio.h"
#include "queue.h"
#ifdef USE_ECHONEST
#include "jansson.h"
#endif

extern const uint8_t spotify_apikey[];
extern const size_t spotify_apikey_size;
extern const char *echonest_api_key;
const bool autostart = 0;

/// Handle to the playlist currently being played
static sp_playlist *g_jukeboxlist;
/// Handle to the current track 
static sp_track *g_currenttrack;
/// Index to the next track
static int g_track_index;
/// The global session handle
static sp_session *g_sess;
int g_logged_in;
static audio_fifo_t g_audiofifo;

int next_timeout = 0;
long previous_volume = -1;

struct attr initial_layout, main_layout;

struct spotify
{
    struct navit *navit;
    struct callback *callback;
    struct event_idle *idle;
    struct attr **attrs;
    char *login;
    char *password;
    char *playlist;
    sp_playlistcontainer *pc;
    gboolean playing;
} *spotify;

/**
 * Called on various events to start playback if it hasn't been started already.
 *
 * The function simply starts playing the first track of the playlist.
 */
static void
try_jukebox_start (void)
{
    dbg (lvl_info, "Starting the jukebox\n");
    sp_track *t;
    spotify->playing = 0;

    if (!g_jukeboxlist)
      {
	  dbg (lvl_info, "jukebox: No playlist. Waiting\n");
	  return;
      }

    if (!sp_playlist_num_tracks (g_jukeboxlist))
      {
	  dbg (lvl_info, "jukebox: No tracks in playlist. Waiting\n");
	  return;
      }

    if (sp_playlist_num_tracks (g_jukeboxlist) < g_track_index)
      {
	  dbg (lvl_info, "jukebox: No more tracks in playlist. Waiting\n");
	  return;
      }

    t = sp_playlist_track (g_jukeboxlist, g_track_index);

    if (g_currenttrack && t != g_currenttrack)
      {
	  /* Someone changed the current track */
	  audio_fifo_flush (&g_audiofifo);
	  sp_session_player_unload (g_sess);
	  g_currenttrack = NULL;
      }

    if (!t)
	return;

    if (sp_track_error (t) != SP_ERROR_OK)
	return;

    if (g_currenttrack == t)
	return;

    g_currenttrack = t;

    dbg (lvl_info, "jukebox: Now playing \"%s\"...\n", sp_track_name (t));

    sp_session_player_load (g_sess, t);
    spotify->playing = 1;
    sp_session_player_play (g_sess, 1);
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
tracks_added (sp_playlist * pl, sp_track * const *tracks, int num_tracks, int position, void *userdata)
{
    dbg (lvl_info, "jukebox: %d tracks were added to %s\n", num_tracks, sp_playlist_name (pl));

    if (!strcasecmp (sp_playlist_name (pl), spotify->playlist))
      {
	  g_jukeboxlist = pl;
	  if (autostart)
	      try_jukebox_start ();
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
playlist_added (sp_playlistcontainer * pc, sp_playlist * pl, int position, void *userdata)
{
    sp_playlist_add_callbacks (pl, &pl_callbacks, NULL);
    dbg (lvl_info, "List name: %s\n", sp_playlist_name (pl));

    if (!strcasecmp (sp_playlist_name (pl), spotify->playlist))
      {
	  g_jukeboxlist = pl;
	  if (autostart)
	      try_jukebox_start ();
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
container_loaded (sp_playlistcontainer * pc, void *userdata)
{
    dbg (lvl_info, "jukebox: Rootlist synchronized (%d playlists)\n", sp_playlistcontainer_num_playlists (pc));
    if (autostart)
	try_jukebox_start ();
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
on_login (sp_session * session, sp_error error)
{
    dbg (lvl_info, "spotify login\n");
    if (error != SP_ERROR_OK)
      {
	  dbg (lvl_error, "Error: unable to log in: %s\n", sp_error_message (error));
	  return;
      }

    g_logged_in = 1;
    spotify->pc = sp_session_playlistcontainer (session);
    int i;

    sp_playlistcontainer_add_callbacks (spotify->pc, &pc_callbacks, NULL);
    dbg (lvl_info, "Got %d playlists\n",
	 sp_playlistcontainer_num_playlists (spotify->pc)) for (i = 0; i < sp_playlistcontainer_num_playlists (spotify->pc); ++i)
      {
	  sp_playlist *pl = sp_playlistcontainer_playlist (spotify->pc, i);
	  sp_playlist_add_callbacks (pl, &pl_callbacks, NULL);

	  if (!strcasecmp (sp_playlist_name (pl), spotify->playlist))
	    {
		dbg (lvl_info, "Found the playlist %s\n", spotify->playlist);
		switch (sp_playlist_get_offline_status (session, pl))
		  {
		  case SP_PLAYLIST_OFFLINE_STATUS_NO:
		      dbg (lvl_info, "Playlist is not offline enabled.\n");
		      sp_playlist_set_offline_mode (session, pl, 1);
		      dbg (lvl_info, "  %d tracks to sync\n", sp_offline_tracks_to_sync (session));
		      break;

		  case SP_PLAYLIST_OFFLINE_STATUS_YES:
		      dbg (lvl_info, "Playlist is synchronized to local storage.\n");
		      break;

		  case SP_PLAYLIST_OFFLINE_STATUS_DOWNLOADING:
		      dbg (lvl_info, "This playlist is currently downloading. Only one playlist can be in this state any given time.\n");
		      break;

		  case SP_PLAYLIST_OFFLINE_STATUS_WAITING:
		      dbg (lvl_info, "Playlist is queued for download.\n");
		      break;

		  default:
		      dbg (lvl_error, "unknow state\n");
		      break;
		  }
		g_jukeboxlist = pl;
		// try_jukebox_start ();
	    }
      }
    if (!g_jukeboxlist)
      {
	  dbg (lvl_error, "jukebox: No such playlist. Waiting for one to pop up...\n");
      }
    // try_jukebox_start ();
}

static int
on_music_delivered (sp_session * session, const sp_audioformat * format, const void *frames, int num_frames)
{
    audio_fifo_t *af = &g_audiofifo;
    audio_fifo_data_t *afd;
    size_t s;

    if (num_frames == 0)
	return 0;		// Audio discontinuity, do nothing

    pthread_mutex_lock (&af->mutex);

    /* Buffer one second of audio */
    if (af->qlen > format->sample_rate)
      {
	  pthread_mutex_unlock (&af->mutex);

	  return 0;
      }

    s = num_frames * sizeof (int16_t) * format->channels;

    afd = malloc (sizeof (*afd) + s);
    memcpy (afd->samples, frames, s);

    afd->nsamples = num_frames;

    afd->rate = format->sample_rate;
    afd->channels = format->channels;

    TAILQ_INSERT_TAIL (&af->q, afd, link);
    af->qlen += num_frames;

    pthread_cond_signal (&af->cond);
    pthread_mutex_unlock (&af->mutex);

    return num_frames;
}

static void
on_end_of_track (sp_session * session)
{
    ++g_track_index;
    try_jukebox_start ();
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
spotify_spotify_idle (struct spotify *spotify)
{
    sp_session_process_events (g_sess, &next_timeout);
}

void
spotify_navit_init (struct navit *nav)
{
    dbg (lvl_error, "spotify_navit_init\n");
    sp_error error;
    sp_session *session;

    long vol = -1;
    audio_volume (AUDIO_VOLUME_GET, &vol);
    dbg (lvl_error, "Master volume is %i\n", vol);

    spconfig.application_key_size = spotify_apikey_size;
    if (spconfig.application_key_size == 0)
      {
	  dbg (lvl_error, "Can't create session, did you setup your spotify apikey ?\n");
	  return;
      }
    error = sp_session_create (&spconfig, &session);
    if (error != SP_ERROR_OK)
      {
	  dbg (lvl_error, "Can't create spotify session :(\n");
	  return;
      }
    dbg (lvl_info, "Session created successfully :)\n");
    g_sess = session;
    g_logged_in = 0;
    sp_session_login (session, spotify->login, spotify->password, 0, NULL);
    audio_init (&g_audiofifo);
    spotify->navit = nav;
    // FIXME : we should maybe use a timer instead of the idle loop
    spotify->callback = callback_new_1 (callback_cast (spotify_spotify_idle), spotify);
    event_add_idle (125, spotify->callback);
    dbg (lvl_error, "Callback created successfully\n");
    spotify->navit = nav;
}

void
spotify_navit (struct navit *nav, int add)
{
    struct attr callback;
    if (add)
      {
	  dbg (0, "adding callback\n");
	  callback.type = attr_callback;
	  callback.u.callback = callback_new_attr_0 (callback_cast (spotify_navit_init), attr_navit);
	  navit_add_attr (nav, &callback);
      }
}

void
spotify_set_attr (struct attr **attrs)
{
    spotify = g_new0 (struct spotify, 1);
    struct attr *attr;
    if ((attr = attr_search (attrs, NULL, attr_spotify_login)))
      {
	  dbg (lvl_info, "found spotify_login attr %s\n", attr->u.str);
	  spotify->login = attr->u.str;
      }
    if ((attr = attr_search (attrs, NULL, attr_spotify_password)))
      {
	  dbg (lvl_info, "found spotify_password attr %s\n", attr->u.str);
	  spotify->password = attr->u.str;
      }
    else
      {
	  dbg (lvl_error, "SPOTIFY PASSWORD NOT FOUND!\n");
      }
    if ((attr = attr_search (attrs, NULL, attr_spotify_playlist)))
      {
	  dbg (lvl_info, "found spotify_playlist attr %s\n", attr->u.str);
	  spotify->playlist = attr->u.str;
      }

    char **hints;
    /* Enumerate sound devices */
    int err = snd_device_name_hint (-1, "pcm", (void ***) &hints);
    if (err != 0)
	return;			//Error! Just return

    char **n = hints;
    while (*n != NULL)
      {

	  char *name = snd_device_name_get_hint (*n, "NAME");
	  dbg (lvl_error, "Found audio device %s\n", name);

	  if (name != NULL && 0 != strcmp ("null", name))
	    {
		//Copy name to another buffer and then free it
		free (name);
	    }
	  n++;
      }				//End of while
    snd_device_name_free_hint ((void **) hints);
}

char *
media_get_current_playlist_name ()
{
    return spotify->playlist;
}

char *
media_get_playlist_name (int playlist_index)
{
    return sp_playlist_name (sp_playlistcontainer_playlist (spotify->pc, playlist_index));
}

char *
media_get_track_name (int track_index)
{
    return sp_track_name (sp_playlist_track (g_jukeboxlist, track_index));
}


char *
media_get_playlist_status_icon (sp_playlist *playlist )
{
    char *icon;
    switch (sp_playlist_get_offline_status (g_sess, playlist))
      {
      case SP_PLAYLIST_OFFLINE_STATUS_NO:
	  icon = "playlist-no-offline";
	  break;

      case SP_PLAYLIST_OFFLINE_STATUS_YES:
	  icon = "playlist-offline";
	  break;

      case SP_PLAYLIST_OFFLINE_STATUS_DOWNLOADING:
	  icon = "playlist-downloading";
	  break;

      case SP_PLAYLIST_OFFLINE_STATUS_WAITING:
	  icon = "playlist-pending";
	  break;

      default:
	  icon = "music-red";
	  break;
      }
    return icon;
}

char *
media_get_playlist_status_icon_by_index (int playlist_index)
{
    return media_get_playlist_status_icon(sp_playlistcontainer_playlist (spotify->pc, playlist_index));
}

char *
media_get_current_playlist_status_icon ()
{
    return media_get_playlist_status_icon(g_jukeboxlist);
}

int
media_get_playlists_count ()
{
    return sp_playlistcontainer_num_playlists (spotify->pc);
}

int
media_get_current_playlist_items_count ()
{
    return media_get_playlist_items_count (g_jukeboxlist);
}

int
media_get_playlist_items_count ()
{
    return sp_playlist_num_tracks (g_jukeboxlist);
}

char *
media_get_track_status_icon (int track_index)
{
    char *icon;
    sp_track *t = sp_playlist_track (g_jukeboxlist, track_index);
    switch (sp_track_offline_get_status (t))
      {
      case SP_TRACK_OFFLINE_DONE:
	  icon = "music-green";
	  break;
      case SP_TRACK_OFFLINE_DOWNLOADING:
	  icon = "music-orange";
	  break;
      case SP_TRACK_OFFLINE_NO:
	  icon = "music-blue";
	  break;
      default:
	  icon = "music-red";
      }
    return g_strdup ((track_index == g_track_index) ? "play" : icon);
}

void
media_set_active_playlist (int playlist_index)
{
    sp_playlist *pl = sp_playlistcontainer_playlist (spotify->pc, playlist_index);
    dbg (0, "Got a request to play a specific playlist : #%i : %s\n", playlist_index, sp_playlist_name (pl));
    g_jukeboxlist = pl;
    spotify->playlist = sp_playlist_name (pl);
}

void
media_next_track ()
{
    ++g_track_index;
    dbg (0, "skipping to next track\n");
    try_jukebox_start ();
}

void
media_previous_track ()
{
    if (g_track_index > 0)
      {
	  --g_track_index;
      dbg (0, "rewinding to previous track\n");
      }
    try_jukebox_start ();
}

void
media_toggle_playback ()
{
    if (spotify->playing)
      {
	  dbg (0, "pausing playback\n");
	  sp_session_player_play (g_sess, 0);
      }
    else
      {
	  dbg (0, "resuming playback\n");
	  sp_session_player_play (g_sess, 1);
	  try_jukebox_start ();
      }
    spotify->playing = !spotify->playing;
}

void
media_volume_up ()
{
    long vol = -1;
    audio_volume (AUDIO_VOLUME_GET, &vol);
    vol += 10;
    audio_volume (AUDIO_VOLUME_SET, &vol);
}

void
media_volume_down ()
{
    long vol = -1;
    audio_volume (AUDIO_VOLUME_GET, &vol);
    vol -= 10;
    audio_volume (AUDIO_VOLUME_SET, &vol);
}

void
media_mute_toggle ()
{
    if (previous_volume == -1)
      {
	  // no previous volume know. We should mute
	  audio_volume (AUDIO_VOLUME_GET, &previous_volume);
	  long vol = 0;
	  audio_volume (AUDIO_VOLUME_SET, &vol);
      }
    else
      {
	  audio_volume (AUDIO_VOLUME_SET, &previous_volume);
	  previous_volume = -1;
      }
}

void
media_set_current_track (int track_index)
{
    g_track_index = track_index;
    try_jukebox_start ();
}

/**
 * @brief   Toggle the offline status of the current playlist
 * @param[in]  none 
 *
 * @return  nothing
 *
 * Toggle the offline status of the current playlist
 *
 */
void
media_toggle_current_playlist_offline()
{
    sp_playlist_set_offline_mode (g_sess, g_jukeboxlist,  sp_playlist_get_offline_status (g_sess, g_jukeboxlist) != 1);
}

#ifdef USE_ECHONEST
/**
 * @brief   Build a playlist (radio) from Echonest
 * @param[in]   track_index    - the index of the track in the current playlist to use as a baseline
 *
 * @return  nothing
 *
 * Attempts to build a playlist (radio) from Echonest, given a track to use as a reference.
 * If the playlist generation is successful, the playlist will be set as active
 *
 */
void
echonest_start_radio (int track_index)
{
    char url[256], uri[256];
    sp_playlist *echonest_pl;
    sp_link *l;
    sp_track *t = sp_playlist_track (g_jukeboxlist, track_index);
    if (t)
      {
	  l = sp_link_create_from_track (t, 0);
	  sp_link_as_string (l, uri, sizeof (uri));
	  dbg (0, "Starting radio for %i %s\n", track_index, uri);
	  strcpy (url,
		  g_strdup_printf
		  ("http://developer.echonest.com/api/v4/playlist/basic?api_key=%s&song_id=%s&format=json&results=20&type=song-radio&bucket=id:spotify&bucket=tracks",
		   echonest_api_key, uri));
	  sp_link_release (l);
	  dbg (0, "Url %s\n", url);

	  json_t *root;
	  json_error_t error;
	  int i;
	  char *item_js = fetch_url_to_string (url);
	  dbg (0, "%s\n", item_js);
	  root = json_loads (item_js, 0, &error);
	  free (item_js);
	  if (!root)
	    {
		/* FIXME : add feedback for the GUI */
		dbg (0, "Invalid json for url %s, giving up for track id %s\n", url, uri);
		json_decref (root);
		return;
	    }

	  /* Let's see if we have a playlist to handle this radio. If we do, delete it */
	  for (i = 0; i < sp_playlistcontainer_num_playlists (spotify->pc); i++)
	    {
		if (!strcasecmp (sp_playlist_name (sp_playlistcontainer_playlist (spotify->pc, i)), "_navit_echonest"))
		  {
		      dbg (0, "Deleting previous echonest playlist\n");
		      /* Fixme : error checking */
		      sp_playlistcontainer_remove_playlist (spotify->pc, i);
		  }
	    }

	  /* Fixme : error checking */
	  echonest_pl = sp_playlistcontainer_add_new_playlist (spotify->pc, "_navit_echonest");

	  json_t *response = json_object_get (root, "response");

	  json_t *songs = json_object_get (response, "songs");
	  for (i = 0; i < json_array_size (songs); i++)
	    {
		json_t *song = json_array_get (songs, i);
		json_t *artist_name = json_object_get (song, "artist_name");
		json_t *title = json_object_get (song, "title");

		json_t *tracks = json_object_get (song, "tracks");

		int j;
		sp_track *t;
		sp_link *l;
		// dbg (0, "Got song %s from artist %s\n", json_string_value (title), json_string_value (artist_name));
		for (j = 0; j < json_array_size (tracks); j++)
		  {
		      json_t *track = json_array_get (tracks, j);
		      json_t *foreign_id = json_object_get (track, "foreign_id");
		      l = sp_link_create_from_string (json_string_value (foreign_id));
		      if (!l)
			{
			    dbg (0, "Link creation for track %s failed\n", json_string_value (foreign_id));
			    return;
			}
		      t = sp_link_as_track (l);
		      if (!t)
			{
			    dbg (0, "Track creation for track %s failed\n", json_string_value (foreign_id));
			    return;
			}
		      sp_error res =
			  sp_playlist_add_tracks (echonest_pl, (const sp_track **) &t, 1, sp_playlist_num_tracks (echonest_pl),
						  g_sess);
		      if (res == SP_ERROR_OK)
			{
			    dbg (0, "Successfully added one instance of the track %s at on attempt %i. Moving to the next one \n",
				 json_string_value (foreign_id), j);
			    break;
			}
		      else
			{
			    dbg (0, "Failed to add one instance of the track %s on attempt %i. Trying the next one \n",
				 json_string_value (foreign_id), j);
			}
		  }
	    }
      }
    else
      {
	  /* FIXME : add an error message in the GUI */
	  dbg (0, "Cannot get a spotify URI for the track #%i\n", track_index);
      }
    g_jukeboxlist = echonest_pl;
    spotify->playlist = sp_playlist_name (echonest_pl);
}

#endif
