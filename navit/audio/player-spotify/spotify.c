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

struct spotify
{
    struct callback *callback;
    struct event_idle *idle;
    struct attr **attrs;
    char *login;
    char *password;
    char *playlist;
    sp_playlistcontainer *pc;
    gboolean playing;
} *spotify;

struct audio_priv {
    struct spotify *spotify;
};

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


static void
try_jukebox_start (void)
{
    dbg (lvl_error, "Starting the jukebox\n");
    sp_track *t;
    spotify->playing = 0;

    if (!g_jukeboxlist)
      {
      dbg (lvl_error, "jukebox: No playlist. Waiting\n");
      return;
      }

    if (!sp_playlist_num_tracks (g_jukeboxlist))
      {
      dbg (lvl_error, "jukebox: No tracks in playlist. Waiting\n");
      return;
      }

    if (sp_playlist_num_tracks (g_jukeboxlist) < g_track_index)
      {
      dbg (lvl_error, "jukebox: No more tracks in playlist. Waiting\n");
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

    dbg (lvl_error, "jukebox: Now playing \"%s\"... g_sess=%p\n", sp_track_name (t), g_sess);
        
    sp_session_player_load (g_sess, t);
    dbg(lvl_error, "loaded\n");
    spotify->playing = 1;
    dbg(lvl_error, "playing\n");
    if (g_sess) {
        sp_session_player_play (g_sess, 1);
        dbg(lvl_error, "sp_session_player_play called\n");
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
tracks_added (sp_playlist * pl, sp_track * const *tracks, int num_tracks, int position, void *userdata)
{
    dbg (lvl_error, "jukebox: %d tracks were added to %s\n", num_tracks, sp_playlist_name (pl));

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
    dbg (lvl_error, "List name: %s\n", sp_playlist_name (pl));

    if (!strcasecmp (sp_playlist_name (pl), spotify->playlist))
      {
      g_jukeboxlist = pl;
      current_playlist_index = position;
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
    dbg (lvl_error, "jukebox: Rootlist synchronized (%d playlists)\n", sp_playlistcontainer_num_playlists (pc));
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
on_login (sp_session * session, sp_error error)
{
    if (error != SP_ERROR_OK)
     {
      dbg (lvl_error, "Error: unable to log in: %s\n", sp_error_message (error));
      return;
     } else {
      dbg (lvl_error, "Logged in!\n");
     }
     
    g_logged_in = 1;
        spotify->pc = sp_session_playlistcontainer (session);
        sp_playlistcontainer_add_callbacks (spotify->pc, &pc_callbacks, NULL);
}

static int
on_music_delivered (sp_session * session, const sp_audioformat * format, const void *frames, int num_frames)
{
    audio_fifo_t *af = &g_audiofifo;
    audio_fifo_data_t *afd;
    size_t s;
    if (num_frames == 0)
        return 0; // Audio discontinuity, do nothing
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
    dbg(lvl_debug,"Delivery done\n");
    return num_frames;   
}

static void
on_end_of_track (sp_session * session)
{
    dbg (lvl_error, "end of track\n");
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
    .application_key_size = 0,  // set in main()
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
toggle_playback ()
{
    if (spotify->playing)
      {
      dbg (lvl_error, "pausing playback\n");
      sp_session_player_play (g_sess, 0);
      }
    else
      {
      dbg (lvl_error, "resuming playback\n");
      sp_session_player_play (g_sess, 1);
      try_jukebox_start ();
      }
    spotify->playing = !spotify->playing;
}

GList *
playlists(struct audio_priv *this)
{
        GList * playlists=NULL;
        struct audio_playlist *pl;
        int i;
        dbg(lvl_error,"Spotify's playlists method\n");
        sp_playlistcontainer *pc = sp_session_playlistcontainer(g_sess);
        dbg(lvl_error,"get_playlists: Looking at %d playlists\n", sp_playlistcontainer_num_playlists(pc)); 
        for (i = 0; i < sp_playlistcontainer_num_playlists(pc); ++i) {
                sp_playlist *spl = sp_playlistcontainer_playlist(pc, i);
                // fixme : should we enable this callback?
                // sp_playlist_add_callbacks(pl, &pl_callbacks, NULL);
                pl = g_new0(struct audio_playlist, 1);
                pl->name=g_strdup(sp_playlist_name(spl));
                pl->index=i;
                pl->status=0;
                playlists=g_list_append(playlists, pl);
        }
        return playlists;
}

GList *
tracks(struct audio_priv *this, int playlist_index)
{
        GList * tracks=NULL;
        struct audio_track *t;
        sp_playlist *spl;
        int i;
        if ( playlist_index < 0 ) {
                playlist_index = current_playlist_index;
        }
        dbg(lvl_error,"Spotify's tracks method\n");
        sp_playlistcontainer *pc = sp_session_playlistcontainer(g_sess);
        spl = sp_playlistcontainer_playlist(pc, playlist_index);
        for (i = 0; i < sp_playlist_num_tracks(spl); i++) {
                t = g_new0(struct audio_track, 1);
                t->name=g_strdup(sp_track_name (sp_playlist_track(spl, i)));
                t->index=i;
                t->status=0;
                tracks=g_list_append(tracks, t);
        }
	g_jukeboxlist=spl;
	dbg(lvl_error,"Active playlist updated\n");
        return tracks;
}

static int
playback(struct audio_priv *this, const int action)
{
        dbg(lvl_error,"in spotify's playback control\n");
        switch(action){
        case 0:
                toggle_playback();
                break;
        case -1:
                ++g_track_index;
                try_jukebox_start ();
                break;
        case -2:
                if (g_track_index > 0)
                        --g_track_index;
                try_jukebox_start ();
                break;
        default:
                if ( action > 0 ) {
                       g_track_index = action;
                       try_jukebox_start ();
                } else {
                        dbg(lvl_error,"Don't know what to do with action '%i'. That's a bug\n", action);
                }
        }
        return 0;
}

static struct audio_methods player_spotify_meth = {
        NULL,
        playback,
        tracks,
        playlists,
};

static struct audio_priv *
player_spotify_new(struct audio_methods *meth, struct attr **attrs, struct attr *parent) 
{
    struct audio_priv *this;
    struct attr *attr;
    sp_error error;
    sp_session *session;
    attr=attr_search(attrs, NULL, attr_spotify_password);
    dbg(lvl_error,"Initializing spotify\n");

    spotify = g_new0 (struct spotify, 1);
    if ((attr = attr_search (attrs, NULL, attr_spotify_login)))
      {
          spotify->login = g_strdup(attr->u.str);
          dbg (lvl_error, "found spotify_login %s\n", spotify->login);
      }
    if ((attr = attr_search (attrs, NULL, attr_spotify_password)))
      {
          spotify->password = g_strdup(attr->u.str);
          dbg (lvl_error, "found spotify_password %s\n", spotify->password);
      }
    if ((attr = attr_search (attrs, NULL, attr_spotify_playlist)))
      {
          spotify->playlist = g_strdup(attr->u.str);
          dbg (lvl_error, "found spotify_playlist %s\n", spotify->playlist);
      }
    spconfig.application_key_size = spotify_apikey_size;
    error = sp_session_create (&spconfig, &session);
    if (error != SP_ERROR_OK)
      {
      dbg (lvl_error, "Can't create spotify session :(\n");
      return NULL;
      }
    dbg (lvl_error, "Session created successfully :)\n");
    g_sess = session;
    g_logged_in = 0;
    sp_session_login (session, spotify->login, spotify->password, 0, NULL);
    audio_init (&g_audiofifo);
    // FIXME : we should maybe use a timer instead of the idle loop
    spotify->callback = callback_new_1 (callback_cast (spotify_spotify_idle), spotify);
    event_add_idle (125, spotify->callback);
    dbg (lvl_error, "Callback created successfully\n");
    this=g_new(struct audio_priv,1);

    *meth=player_spotify_meth;
    return this;
}


void
plugin_init(void)
{
        plugin_register_audio_type("player-spotify", player_spotify_new);
}
