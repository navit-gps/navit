/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2016 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */


#include <glib.h>
#include <navit/main.h>
#include <navit/debug.h>
#include <navit/point.h>
#include <navit/navit.h>
#include <navit/callback.h>
#include <navit/color.h>
#include <navit/event.h>

#include "time.h"
#include "gui_internal.h"
#include "gui_internal_menu.h"
#include "coord.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_media.h"
#include "gui_internal_command.h"
#include "audio.h"

int currently_displayed_playlist = -1;

static void
tracks_free(gpointer data)
{
	if (data != NULL) {
		struct audio_track *track = data;
		g_free(track->name);
		g_free(track);
	}
}

/**
 * @brief   play a track from the playlist
 * @param[in]   this    - pointer to the gui_priv
 *              wm      - pointer the the parent widget
 *              date    - pointer to arbitrary data
 */
void
media_play_track(struct gui_priv *this, struct widget *wm, void *data)
{
	dbg(lvl_info, "Got a request to play a specific track : %i\n",
	    wm->c.x);
	audio_play_track(this->nav, wm->c.x);
	gui_internal_prune_menu(this, NULL);
	//gui_internal_media_show_playlist (this, NULL, NULL);
}

/**
 * @brief   play a playlist from the list of playlist playlist
 * @param[in]   this    - pointer to the gui_priv
 *              wm      - pointer the the parent widget
 *              date    - pointer to arbitrary data
 */
void
media_play_playlist(struct gui_priv *this, struct widget *wm, void *data)
{
	dbg(lvl_info, "Got a request to play a specific playlist : %i\n",
	    wm->c.x);
	currently_displayed_playlist = wm->c.x;
	gui_internal_media_show_playlist(this, NULL, NULL);
}

/**
 * @brief   Perform a media action
 * @param[in]   this    - pointer to the gui_priv
 *              wm      - pointer the the parent widget
 *              date    - pointer to arbitrary data
 *
 * Perform an action on the audio player which are defined in the player implementation
 *
 */
void
media_action_do(struct gui_priv *this, struct widget *wm, void *data)
{
	dbg(lvl_info, "Got a request to perform an action : %i\n",
	    (int) data);
	int action = (int) data;
	audio_do_action(this->nav, action);
	switch (action) {
	case AUDIO_PLAYBACK_TOGGLE:
	case AUDIO_PLAYBACK_NEXT_TRACK:
	case AUDIO_PLAYBACK_PREVIOUS_TRACK:
	case AUDIO_MISC_RELOAD_PLAYLISTS:{
			gui_internal_prune_menu(this, NULL);
			break;
		}
	case AUDIO_PLAYBACK_NEXT_PLAYLIST:
	case AUDIO_PLAYBACK_PREVIOUS_PLAYLIST:
	case AUDIO_PLAYBACK_NEXT_ARTIST:
	case AUDIO_PLAYBACK_PREVIOUS_ARTIST:
	case AUDIO_MODE_TOGGLE_SHUFFLE:
	case AUDIO_MODE_TOGGLE_REPEAT:{
			gui_internal_media_show_playlist(this, NULL, NULL);
			break;
		}
	case AUDIO_MISC_DELETE_PLAYLIST:{
			gui_internal_media_show_rootlist(this, NULL, NULL);
			break;
		}
	default:{
			dbg(lvl_error,
			    "Don't know what to do with action '%i'. That's a bug\n",
			    action);
			gui_internal_media_show_playlist(this, NULL, NULL);
		}
	}
}

/**
 * @brief   Build a playlist 'toolbar'
 *
 * @param[in]   this    - pointer to the gui_priv
 *
 * @return  the resulting widget
 *
 * Builds a widget containing a button to browse the root playlist,
 * and another botton to display and toggle the state of the offline availability
 *
 */
static struct widget *
gui_internal_media_playlist_toolbar(struct gui_priv *this)
{
	struct widget *wl, *wb;
	int nitems, nrows;
	GList *actions = audio_get_actions(this->nav);
	wl = gui_internal_box_new(this,
				  gravity_left_center |
				  orientation_horizontal_vertical |
				  flags_fill);
	wl->background = this->background;
	wl->w = this->root.w;
	wl->cols = this->root.w / this->icon_s;
	nitems = 2;
	nrows = nitems / wl->cols + (nitems % wl->cols > 0);
	wl->h = this->icon_l * nrows;
	wb = gui_internal_button_new_with_callback(this, "Playlists",
						   image_new_s(this,
							       "media_category"),
						   gravity_left_center |
						   orientation_horizontal,
						   gui_internal_media_show_rootlist,
						   NULL);
	gui_internal_widget_append(wl, wb);
	while (actions) {
		struct audio_actions *aa = actions->data;
		actions = g_list_next(actions);
		if (aa->icon && aa->action) {
			gui_internal_widget_append(wl, wb =
						   gui_internal_button_new_with_callback
						   (this, NULL,
						    image_new_s(this,
								aa->icon),
						    gravity_left_center |
						    orientation_horizontal,
						    media_action_do,
						    aa->action));
		}
	}
	return wl;
}

/**
 * @brief   Show the playlists in the root playlist
 * @param[in]   this    - pointer to the gui_priv
 *              wm      - pointer the the parent widget
 *              data    - pointer to arbitrary data
 *
 * @return  nothing
 *
 * Display a list of the playlists in the root playlist
 *
 */
void
gui_internal_media_show_rootlist(struct gui_priv *this, struct widget *wm,
				 void *data)
{
	struct widget *wb, *w, *wbm;
	struct widget *tbl, *row;
	dbg(lvl_error, "Showing rootlist\n");
	GList *playlists = audio_get_playlists(this->nav);

	gui_internal_prune_menu_count(this, 1, 0);
	wb = gui_internal_menu(this, "Media > Playlists");
	wb->background = this->background;
	w = gui_internal_box_new(this,
				 gravity_top_center | orientation_vertical
				 | flags_expand | flags_fill);
	gui_internal_widget_append(wb, w);


	tbl =
	    gui_internal_widget_table_new(this,
					  gravity_left_top | flags_fill |
					  flags_expand |
					  orientation_vertical, 1);
	gui_internal_widget_append(w, tbl);

	while (playlists) {
		struct audio_playlist *pl = playlists->data;
		dbg(lvl_error, "playlist : %s\n", pl->name);

		playlists = g_list_next(playlists);
		row =
		    gui_internal_widget_table_row_new(this,
						      gravity_left |
						      flags_fill |
						      orientation_horizontal);
		gui_internal_widget_append(tbl, row);
		wbm = gui_internal_button_new_with_callback(this,
							    pl->name,
							    image_new_s
							    (this,
							     (pl->
							      icon) ? (pl->
								       icon)
							     :
							     ("media_hierarchy")),
							    gravity_left_center
							    |
							    orientation_horizontal
							    | flags_fill,
							    media_play_playlist,
							    NULL);

		gui_internal_widget_append(row, wbm);
		wbm->c.x = pl->index;
	}

	gui_internal_menu_render(this);

}

/**
 * @brief   Show the tracks in the current playlist
 * @param[in]   this    - pointer to the gui_priv
 *              wm      - pointer the the parent widget
 *              data    - pointer to arbitrary data
 *
 * @return  nothing
 *
 * Display a list of the tracks in the current playlist
 *
 */
void
gui_internal_media_show_playlist(struct gui_priv *this, struct widget *wm,
				 void *data)
{
	struct widget *wb, *w, *wbm;
	struct widget *tbl, *row;
	int index = 0;
	gui_internal_prune_menu_count(this, 1, 0);
#ifndef USE_AUDIO_FRAMEWORK
	wb = gui_internal_menu(this,
			       g_strdup_printf("Media not available"));
	wb->background = this->background;
	w = gui_internal_box_new(this,
				 gravity_top_center | orientation_vertical
				 | flags_expand | flags_fill);
	gui_internal_widget_append(wb, w);
#else
	GList *tracks =
	    audio_get_tracks(this->nav, currently_displayed_playlist);
	struct audio_playlist *pl;
	GList *playlist = audio_get_playlists(this->nav);

	dbg(lvl_info, "\n\t%p\n\n", this);

	wb = gui_internal_menu(this,
			       g_strdup_printf("Media > %s",
					       audio_get_current_playlist
					       (this->nav)));
	wb->background = this->background;
	w = gui_internal_box_new(this,
				 gravity_top_center | orientation_vertical
				 | flags_expand | flags_fill);
	gui_internal_widget_append(wb, w);
	gui_internal_widget_append(w,
				   gui_internal_media_playlist_toolbar
				   (this));
	tbl =
	    gui_internal_widget_table_new(this,
					  gravity_left_top | flags_fill |
					  flags_expand |
					  orientation_vertical, 1);
	gui_internal_widget_append(w, tbl);
	while (tracks) {
		struct audio_track *track = tracks->data;
		tracks = g_list_next(tracks);
		row =
		    gui_internal_widget_table_row_new(this,
						      gravity_left |
						      flags_fill |
						      orientation_horizontal);
		gui_internal_widget_append(tbl, row);
		wbm = gui_internal_button_new_with_callback(this,
							    track->name,
							    image_new_s
							    (this,
							     track->icon),
							    gravity_left_center
							    |
							    orientation_horizontal
							    | flags_fill,
							    media_play_track,
							    NULL);
		wbm->c.x = track->index;
		gui_internal_widget_append(row, wbm);
	}
	g_list_free_full(tracks, tracks_free);
#endif
	gui_internal_menu_render(this);
}
