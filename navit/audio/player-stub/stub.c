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
 
 /*
  * this file is a working example for an entry point for developing an
  * own audio plugin.
  * 
  * you need to enable it inside the navit.xml file with the following:
  * <audio type="player-stub" /> anywhere between the <navit></navit>
  * tags. You also might want need an entry point to the gui_internal_media 
  * This could look that way:
  * <img src='music-blue' onclick='media_show_playlist()'><text>Music</text></img>
  * put this inside the <gui type="internal" enabled="yes" ...></gui> block
  */ 
#include <glib.h>
#include "item.h"
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <glib.h>
#include <math.h>
#include <time.h>
#include <termios.h>
#include <errno.h>
#include <dirent.h>
#include <sys/ioctl.h>

#include <attr.h>
#include <navit/map.h>
#include <navit/navit.h>
#include <navit/file.h>
#include <navit/plugin.h>
#include <navit/command.h>
#include <navit/config_.h>
#include <navit/main.h>
#include <navit/item.h>
#include <navit/debug.h>
#include <navit/callback.h>
#include <navit/event.h>
#include <navit/audio.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define PLAYLIST 1
#define TRACK 2

/// Index to the next track
static int g_track_index;

char str[25]; // this string is just for visualisation of the functions
char track[64];

struct audio_priv
{
	/* this is the data structure for the audio plugin
	 * you might not need every element of it
	 */ 
    struct navit *navit;
    struct callback *callback;
    struct callback *idle;
    struct callback_list *cbl;
    struct event_timeout *timeout;
    struct attr **attrs;
    GList* current_playlist;
    GList* playlists;
    GList* actions;
    gchar *musicdir;
    int num_playlists;
    int playlist_index;
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
} *stub;


GList* sort_playlists(GList* list);

char * stub_get_playlist_name (int playlist_index);
char* get_playlist_name(GList* list);
void stub_play(void);
void stub_pause(void);
void stub_play_track(int track);
GList* get_entry_by_index(GList* list, int index);
void stub_toggle_repeat(struct audio_actions *action);
void stub_toggle_shuffle(struct audio_actions *action);
struct audio_actions* get_specific_action(GList* actions, int specific_action);

/**
 * Get function for attributes
 *
 * @param priv Pointer to the audio instance data
 * @param type The attribute type to look for
 * @param attr Pointer to a {@code struct attr} to store the attribute
 * @return True for success, false for failure
 */
int stub_get_attr(struct audio_priv* priv, enum attr_type type, struct attr *attr){
	int ret = 1;
	dbg(lvl_debug, "priv: %p, type: %i (%s), attr: %p\n", priv, type,attr_to_name(type), attr);
	if(priv != stub) {
		dbg(lvl_debug, "failed\n");
		return -1;
	}
	switch(type){
		case attr_playing:{
			attr->u.num = stub->playing;
			dbg(lvl_debug, "Status: %ld\n", attr->u.num);
			break;
		}
		case attr_name:{
			attr->u.str = g_strdup("STUB");
			dbg(lvl_debug, "%s\n", attr->u.str);
			break;
		}
		case attr_shuffle:{			
			int toggle = 0;	
			if(stub->random_track) toggle++;
			if(stub->random_playlist) toggle += 2;
			attr->u.num = toggle;
			dbg(lvl_debug, "%ld\n", attr->u.num);
			break;
		}
		case attr_repeat:{
			int toggle = 0;	
			if(stub->single) toggle++;
			if(stub->repeat) toggle += 2;
			attr->u.num = toggle;
			dbg(lvl_debug, "%ld\n", attr->u.num);
			break;
		}
		default:{
			dbg(lvl_error, "Don't know what to do with ATTR type %s\n", attr_to_name(attr->type));
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
int stub_set_attr(struct audio_priv *priv, struct attr *attr){
	dbg(lvl_debug, "priv: %p, type: %i (%s), attr: %p\n", priv, attr->type,attr_to_name(attr->type), attr);
	if(priv != stub){
		dbg(lvl_debug, "failed\n");
		return -1;
	}
	if(attr){
		switch(attr->type){
			case attr_name:{
				dbg(lvl_debug, "%s\n", attr->u.str);
				break;
			}
			case attr_playing:{
				dbg(lvl_debug, "attr->u.num: %ld\n", attr->u.num);

				if(attr->u.num == 0){
					dbg(lvl_debug, "stub_pause();%ld\n", attr->u.num);
					stub_pause();
				}else{
					dbg(lvl_debug, "stub_play();%ld\n", attr->u.num);
					stub_play();
				}

				break;
			}
			case attr_shuffle:{

				stub_toggle_shuffle(get_specific_action(stub->actions, AUDIO_MODE_TOGGLE_SHUFFLE));
				break;
			}
			case attr_repeat:{
				
				stub_toggle_repeat(get_specific_action(stub->actions, AUDIO_MODE_TOGGLE_REPEAT));
				break;
			}
			default:{
				dbg(lvl_error, "Don't know what to do with ATTR type %s", attr_to_name(attr->type));
				return 0;
				break;
			}
		}

		return 1;
	}
	return 0;
}
	
/**
* @brief this function reads a data of a certain playlist
*
* @param the playlist object
* @return the playlist data structure or NULL on error
*/
struct audio_playlist*
get_playlist_data(GList* list)
{
	if(list != NULL)
	{
		if(list->data!=NULL)
		{
			return (struct audio_playlist*) list->data;
		}
	}
	dbg(lvl_error, "WAH! Data is corrupted\n");
	return NULL;
}

/**
* @brief this function reindexes a list of playlists
*
* @param the playlist object to start indexing
* @return the number of reindexed playlists.
*
* this function reindexes a list of playlists to reorder them.  
* It's intended to be used after a sorting algorithm on the head of a list of playlists.  
*/
int
reindex_playlists(GList *list)
{
    GList *current = list;
    int i = 0;
    get_playlist_data(list)->index = i;
    dbg(lvl_debug, "playlist:%s \n\n", get_playlist_name(list));
    
    while (NULL != (current = current->next))
	{
        dbg(lvl_debug,  "playlist:%s \n", get_playlist_name(current));
        get_playlist_data(current)->index = ++i;
    }
    dbg(lvl_debug,  "%i Playlists indexed\n",i);
	return i;
}

/**
* @brief this function creates a new playlist data structure
* 
* @param the playlist name
* @param the desired playlist index
* @return  the playlist data object.
*/
struct audio_playlist* new_audio_playlist(char *name, int index)
{
	struct audio_playlist *pl;
	pl = g_new0(struct audio_playlist, 1);
	pl->name=g_strdup(name);
	pl->index = index;
	pl->status=0;
	pl->icon = "playlist";
	dbg(lvl_debug,  "Created a playlist with name %s and index %i\n", name, index);
	return pl;
}

/**
* @brief this function appends a playlist data structure to the list of playlists
* 
* @param list the playlist to apped the data
* @param playlist the playlist data to append
* @return  the playlist.
*/
GList*
insert_right(GList* list, struct audio_playlist* playlist)
{
	dbg(lvl_debug, "Appending playlist %s\n", playlist->name);
	return g_list_append(list, (gpointer) playlist);
}

/**
 * @brief Print all nodes of a list of playlists
 * @param list the list of playlists
 * 
 */
void
print_all(GList* list)
{
	int i = 0;
	if(list==NULL) return;
	GList* current = list;
	while (NULL !=  current->next)
	{
		if(get_playlist_data(current)!=NULL)
		{
			printf("List element %i, %s. Index %i\n", i++, get_playlist_name(current), get_playlist_data(current)->index );
		}else{
			dbg(lvl_error, "%i: This appears to be an empty list. That's probably a Bug!\n",i);
		}
		current = current->next;
	}
}	

/**
* @brief choose a playlist randomly
*
* @param list the entire list of playlists
*
* @return the randomly chosen list element 
*/
GList*
random_entry(GList* list){
	int n = 9;
	int i = random();
	i %= n;
	dbg(lvl_debug,  "\tTest a random number: \t%i\n", i );
	return get_entry_by_index(g_list_first(list), i);
}

/**
* @brief switch to the next playlist
*
* @param list the entire list of playlists
*
* @return the next list element 
*/
GList*
next_playlist(GList* list)
{
	if(stub->random_playlist){
		return random_entry(list);
	}
	return (list->next!=NULL)?g_list_next(list):g_list_first(list);
} 

/**
* @brief switch to the previous playlist
*
* @param list the entire list of playlists
*
* @return the previous list element 
*/
GList*
prev_playlist(GList* list)
{
	if(stub->random_playlist){
		return random_entry(list);
	}
	return (list->prev!=NULL)?g_list_previous(list):g_list_last(list);
} 
/**
* @brief Get the name of the playlist
*
* @param list the playlist object
*
* @return the name of the playlist
*/
char*
get_playlist_name(GList* list)
{
	if(list != NULL)
	{
		struct audio_playlist *pl = list->data;
		if(pl != NULL)
		{
			if(pl->name != NULL)
			{
				return pl->name;
			} 
		}
	}
	return NULL;
}

/**
* @brief Delete a playlist from the list of playlists
*
* @param list the playlist object to be deleted
*
* @return the list of playlists without the deleted element
*/
GList* 
delete_and_free(GList *list)
{
    return g_list_delete_link(g_list_first(list),list);
}

/**
* @brief Delete a playlist from the list of playlists and from mpc's database
*
* @param list the playlist object to be deleted
*
* @return the list of playlists without the deleted element
*/
GList*
delete_playlist(GList* list)
{	
    // remove the playlist from your api or lib here
    // and finally it removes the playlist from the list
    return g_list_delete_link(g_list_first(list),list);
}

/**
* @brief set the index of a playlist
*
* @param entry the playlist element
* @param entry_index the desired index 
*/
void set_playlist_index(GList* entry, int entry_index)
{
	struct audio_playlist *pl;
	if(entry)
	{
		pl = (struct audio_playlist*) entry->data;
		if(pl)
		{
			pl->index = entry_index;
		}
	}
}


/**
* @brief swap two playlists
*
* @param a playlist a to swap
* @param b playlist b to swap
* 
* this function is used to bubblesort the playlists
*/
void
swap_playlists(GList* a, GList* b)
{
    char* temp = NULL;
    temp = a->data;
    a->data = b->data;
    b->data = temp;
}

/**
* @brief this function sorts a list of playlists alphabetically
*
* @param list the entire list of playlists
*
* @return the sorted list of playlists
*/
GList* 
sort_playlists(GList* list)
{
	// bubblesort for playlists
	if(list == NULL) return NULL;
    GList* i;
    GList* j;
    int index = 0;
    for(i=list; i->next!=NULL; i=i->next)
	{
        for(j=list; j->next!=NULL; j=j->next)
		{
            if(strcmp(get_playlist_data(j)->name, get_playlist_data(j->next)->name) > 0)
			{
                swap_playlists(j, j->next);
            }
        }
    }
    for(i=g_list_first(list); i->next!=NULL; i=i->next)
    {
		set_playlist_index(i, index++);
	}
	print_all(g_list_first(list));
    return list;
}

/**
 * @brief Save a playlist
 * @param list the list element to be saved
 * 
 * Save a playlist in a proer way to be able to load it on next startup
 */
void 
save_playlist(GList* list){
	/*
	 * save the current playlist to a file
	 */
 } 
 
/**
* @brief this function loads a playlist to mpd/mpd
*
* @param list the playlist to be loaded
*/
void
load_playlist(GList * list){
	if(list){
		// load your playlist into the player here
		save_playlist(list);
	}
}

/**
* @brief this function choses the next playlist from the list of playlists
*
* @param current the currently loaded playlist element
*
* @return the playlist element that was loaded
*/
GList* 
load_next_playlist(GList* current)
{
    GList* next = next_playlist(current);
	load_playlist(next);
    stub->playlist_index = g_list_index(stub->playlists, next->data);
    stub_play();
    return next;
}

/**
* @brief seeks and returns the playlist with the given data
*
* @param head the list of playlist to search on
* @param data the data, which contains the playlist name to search
*
* @return the searched element or NULL if it wasnt found
*/
GList* 
get_entry(GList* head, char *data)
{
	/* this function searches the list for an entry with name data
	 * 
	 */ 
    GList* current = head;
    while(strcmp(get_playlist_data(current)->name, data) != 0)
	{
        current = current->next;
        if(current == head)
		{
            return NULL; //nothing found!
        }
    }
    return current; //found!
}

/**
* @brief this function returns the nth element of the list of playlists
*
* @param list the list of playlists
* @param index the index to get
*
* @return the nth playlist
*/
GList*
get_entry_by_index(GList* list, int index)
{
	GList* entry = g_list_nth(list, index);
	return entry;
}

/**
* @brief this function choses the previous playlist from the list of playlists
*
* @param current the currently loaded playlist element
*
* @return the playlist element that was loaded
*/
GList* 
load_prev_playlist(GList* current)
{
	GList* previous = prev_playlist(current);
    load_playlist(previous);
    stub->playlist_index = g_list_index(stub->playlists, previous->data);
    stub_play();
    return previous;
}

/**
* @brief this function gets the next playlist where the artist differs
*
* @param current playlist entry
* @param next specifies if we go forwards or backwards
*
* @return the playlist item which differs in its artist name
* 
* For proper us of this function the playlists must be named in a special way:
* "Artist - Album" The algorithm iterates over the list of playlists forwards 
* or backwards (depending on next is set or not) and stops and return 
* the first item which differs in the part until the dash char
*/
GList*
change_artist(GList* current, int next)
{
	/** this function iterates the playlists and checks the name
	 * it expects an "artist - album" named playlist
	 * if loads the first playlist with a differing "artist" substring
	 * the minus in the string is important 
	 */ 
    char ca[32] = {0,};
    char na[32] = {0,};
    int i,j;
    for(i=0; i<32; i++)
	{
		if(get_playlist_data(current)->name[i] == '-') /// <= note the minus separator
		{
            ca[i] = 0x00;
            break;
        }
        else
        {
            ca[i] = get_playlist_data(current)->name[i];
        }
    }
    do {
        if(next)
		{
			current = next_playlist(current);
		}else{
			current = prev_playlist(current);
		}
        for(j=0; j<32; j++)
		{
            if(get_playlist_data(current)->name[j] == '-') /// <= note the minus separator
			{
                na[j] = 0x00;
                break;
            }
            else
            {
                na[j] = get_playlist_data(current)->name[j];
            }
        }
        dbg(lvl_debug,  "%s ca:%s na:%s strcmp: %i\n", get_playlist_name(current), ca, na, strncmp(ca, na, min(i,j)));
    } while (strncmp(ca, na, min(i,j))==0);
    load_playlist(current);
    stub_play();
    return current;
}

/**
 * @brief wrapper for change next artist
 * @param current the currently chosen playlist
 * @returns the next chosen playlist
 * 
 * 
 */
GList*
next_artist(GList* current)
{
	return change_artist(current, 1);
}

/**
 * @brief wrapper for change previous artist
 * @param current the currently chosen playlist
 * @returns the previous chosen playlist
 * 
 * 
 */
GList*
prev_artist(GList* current)
{
	return change_artist(current, 0);
}

/**
* @brief this function reads the track name to print it on the gui
*
* @param track_index the index of the track
*
* @return the track name
*/
char *
stub_get_track_name (int track_index)
{
	/** this function should return the 
	 * track "track_index" of the current loaded playlist
	 */ 
	 sprintf(str, "track %i",track_index);
	 return str;
}

/**
* @brief this function gets and returns the current playing playlist
*
* @return the playlist name
* 
* this function is used for OSD and GUI
*/
char *
stub_get_current_playlist_name ()
{
	/* return the name of the currently loaded playlist */

	return "currently loaded playlist";
}

/**
* @brief this function reads the playlist name to print it on the gui
*
* @param playlist_index the index of the playlist
*
* @return the playlist name
*/
char *
stub_get_playlist_name (int playlist_index)
{

	/* this function should return the 
	 * playlist name  "playlist_index" 
	 * of the playlist list
	 */
	 sprintf(str, "playlist %i",playlist_index);
	 return str;
}

/**
* @brief this function starts playback for the track from the current playlist
*
* @param track_index the number of the track
*/
void
stub_set_current_track (int track_index)
{
	stub_play_track(track_index + 1);
    g_track_index = track_index;
}

/**
* @brief this funtion reads the number of tracks for the current playlist
*
* @return the number of tracks
*/
int
stub_get_current_playlist_items_count ()
{
	/* get and return the number of items in the currently 
	 * loaded playlist
	 */ 
	return 7;
}

/**
* @brief this function counts all playlists and returns the number
*
* @return the number of playlists
*/
int
stub_get_playlists_count ()
{
	/* get and return the number of items in the currently 
	 * loaded playlist
	 */ 
	return 9;
}

/**
* @brief this function starts playback for the given playlist
*
* @param playlist_index the given playlist to play
*/
void
stub_set_active_playlist (int playlist_index)
{
    stub->playlist_index = playlist_index;
    stub->current_playlist = get_entry_by_index(stub->playlists, playlist_index);
    if(stub->current_playlist == NULL){
		stub->playlist_index = -1;
		return;
	}
    load_playlist(stub->current_playlist);
	stub_play();
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
GList*
stub_get_actions(void){
	/**
	 * this function creates all actions your player is able to perform
	 * except volume and choosing of a specific track
	 * just remove the actions you do not need 
	 * all listed actions will be put into a player toolbar with its icon
	 * so you should define one for each action
	 */ 
	GList* actions = NULL;
	struct audio_actions *aa;
	if(stub->actions){
		return stub->actions;
	}else{
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_PREVIOUS_ARTIST;
		aa->icon = g_strdup("media_prev_artist");//todo: make beautiful icon
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_PREVIOUS_PLAYLIST;
		aa->icon = g_strdup("media_prev");//todo: make beautiful icon
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_PREVIOUS_TRACK;
		aa->icon = g_strdup("media_minus");//todo: make beautiful icon
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_TOGGLE;
		if(stub->playing){
			aa->icon = g_strdup("media_pause");
		}else{
			aa->icon = g_strdup("media_play");
		}
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_NEXT_TRACK;
		aa->icon = g_strdup("media_plus");//todo: make beautiful icon
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_NEXT_PLAYLIST;
		aa->icon = g_strdup("media_next");//todo: make beautiful icon
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_NEXT_ARTIST;
		aa->icon = g_strdup("media_next_artist");//todo: make beautiful icon
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_MODE_TOGGLE_REPEAT;
		aa->icon = g_strdup("media_repeat_off");//todo: make beautiful icon
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_MODE_TOGGLE_SHUFFLE;
		aa->icon = g_strdup("media_shuffle_off");//todo: make beautiful icon
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_MISC_DELETE_PLAYLIST;
		aa->icon = g_strdup("media_trash");//todo: make beautiful icon
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_MISC_RELOAD_PLAYLISTS;
		aa->icon = g_strdup("media_close");//todo: make beautiful icon
		actions = g_list_append(actions, aa);
	}
	return actions;
}



/**
* @brief the loop function for the audio player
*
* @param the audio player object
*
* this function is the loop core for this plugin. Here is the playlist 
* change and the currend trackname processes for continous playback and 
* nice osd view. If the last song of a playlist ends, the mpd process 
* pauses playback automatically. For continous playback we have to 
* change to the next playlist. 
* 
* We also read some statuses and save them to the audio player object
*/
static void
stub_stub_idle (struct audio_priv *stub)
{
	/**
	 * periodic called function
	 * here you might get the current track and other information from
	 * your player api or lib
	 */ 
	dbg(lvl_debug, "In Stubs Idle Loop\n"); 
	 
	/* List all attached attrs.
	struct attr** attrs = stub->attrs;
	if (attrs) {
		for (;*attrs; attrs++){
			struct attr* a = *attrs;
			dbg(lvl_debug, "Attr (%s) at %p\n", attr_to_name(a->type), *attrs);
		}
	}
	//*/
}

/**
* @brief pause :)
*/
void stub_pause(void)
{
	stub->playing = 0;
	struct attr* playing = attr_search(stub->attrs,NULL, attr_playing);
	if(playing)
		playing->u.num = stub->playing;
	else
		dbg(lvl_debug, "No such Attr in %p\n", stub->attrs);
	dbg(lvl_debug,"\n");
	callback_list_call_attr_0(stub->cbl, attr_playing);
	// pause your playback
}
/**
* @brief play :)
*/
void stub_play(void)
{
	stub->playing = 1;
	struct attr* playing = attr_search(stub->attrs,NULL, attr_playing);
	if(playing)
		playing->u.num = stub->playing;
	else
		dbg(lvl_debug, "No such Attr in %p\n", stub->attrs);
	dbg(lvl_debug,  "\n");
	callback_list_call_attr_0(stub->cbl, attr_playing);
	// start your playback
}

/**
* @brief play a specific track
*
* @param track the number of the track to play
*/
void stub_play_track(int track)
{
	stub->playing = 1;
	struct attr* playing = attr_search(stub->attrs,NULL, attr_playing);
	if(playing)
		playing->u.num = stub->playing;
	else
		dbg(lvl_debug, "No such Attr in %p\n", stub->attrs);
	dbg(lvl_debug,  "\n");
	callback_list_call_attr_0(stub->cbl, attr_playing);
	// choose and play track track here
}

/**
* @brief load the next playlist
*/
void
stub_next_playlist(void)
{
	dbg(lvl_debug,  "\n");
	stub->current_playlist = load_next_playlist(stub->current_playlist);
}

/**
* @brief load the previous playlist
*/
void
stub_prev_playlist(void)
{
	dbg(lvl_debug,  "\n");
	stub->current_playlist = load_prev_playlist(stub->current_playlist);
}

/**
* @brief getter for the volume value of the audio player object
*
* @return the volume
*/
int stub_get_volume(void)
{
	return stub->volume;
}

/**
* @brief sets the volume value
*
* @param vol the volume value to set
*/
void stub_set_volume(int vol)
{
	// set volume to vol here
	stub->volume = vol;
}

/**
* @brief this function toggles the repeat mode
*
* @param action the action that owns the toggle
*/
void stub_toggle_repeat(struct audio_actions *action){
	int toggle = 0;
	if(stub->single) toggle++;
	if(stub->repeat) toggle += 2;
	switch(toggle){
		case 0:// no repeat
		case 1:{
			stub->single = 0;
			stub->repeat = 1;
			if(action != NULL){
				action->icon = g_strdup("media_repeat_playlist");
			}
			dbg(lvl_debug, "\nrepeat playlist\n");
			break;
		}
		case 2:{// repeat playlist
			stub->single = 1;
			stub->repeat = 1;
			if(action != NULL){
				action->icon = g_strdup("media_repeat_track");
			}
			dbg(lvl_debug, "\nrepeat track\n");
			break;
		}
		case 3:{// repeat track
			stub->single = 0;
			stub->repeat = 0;
			if(action != NULL){
				action->icon = g_strdup("media_repeat_off");
			}
			dbg(lvl_debug, "\nrepeat off\n");
			break;
		}
	}
	callback_list_call_attr_0(stub->cbl, attr_repeat);
}
/**
* @brief this function toggles the shuffle mode
*
* @param action the action that owns the toggle
*/
void stub_toggle_shuffle(struct audio_actions *action){
	
	int toggle = 0;
	if(stub->random_track) toggle++;
	if(stub->random_playlist) toggle += 2;
	dbg(lvl_debug,  "Toggle Shuffle: %i\n", toggle);
	switch(toggle){
		case 0:{ 
			
			stub->random_track = TRUE;
			stub->random_playlist = FALSE;
			if(action != NULL){
				action->icon = g_strdup("media_shuffle_playlists");
			}
			dbg(lvl_debug,  "Toggle Shuffle Playlists: %i\n", toggle);
			break;
		}
		case 1:{
			
			stub->random_track = TRUE;
			stub->random_playlist = TRUE;
			if(action != NULL){
				action->icon = g_strdup("media_shuffle_tracks_playlists");
			}
			dbg(lvl_debug,  "Toggle Shuffle Tracks & Playlists: %i\n", toggle);
			break;
		}
		case 3:{
			
			stub->random_track = FALSE;
			stub->random_playlist = TRUE;
			if(action != NULL){
				action->icon = g_strdup("media_shuffle_tracks");
			}
			dbg(lvl_debug,  "Toggle Shuffle Tracks: %i\n", toggle);
			break;
		}
		case 2:{
			
			stub->random_track = FALSE;
			stub->random_playlist = FALSE;
			if(action != NULL){
				action->icon = g_strdup("media_shuffle_off");
			}
			dbg(lvl_debug,  "Toggle Shuffle OFF: %i\n", toggle);
			break;
		}
	}
	callback_list_call_attr_0(stub->cbl, attr_shuffle);
}

/**
* @brief command to toggle playback
*
* @param action the action that owns the toggle
*
* the action must be passed because the playback action keeps the icon. This icon changes dependent on the playback state
*/
void
stub_toggle_playback (struct audio_actions *action)
{

	struct attr* playing = attr_search(stub->attrs,NULL, attr_playing);
	if(playing){
		stub_get_attr(stub, attr_playing, playing);
		playing->u.num = stub->playing;
		if(stub_get_attr(stub, attr_playing, playing) && playing->u.num)
			stub->playing = 1;
	}
	else
		dbg (lvl_debug, "No such Attr in: %p\n", stub->attrs);
	if (stub->playing)
	{
		dbg (lvl_debug, "pausing playback\n");
		// pause your playback here
		if(action != NULL){
			action->icon = g_strdup("media_play");
		}
	}
	else
	{
		dbg (lvl_debug, "resuming playback\n");
		// resume your playback here
		if(action != NULL){
			action->icon = g_strdup("media_pause");
		}
	}
	stub->playing = !stub->playing;
	if(playing){
		playing->u.num = stub->playing;
		stub_set_attr(stub, playing);
	}
	callback_list_call_attr_0(stub->cbl, attr_playing);
}

/**
* @brief this function returns a list of playlists
*
* @param this the audio player object
*
* @return the list of playlists
* 
* if no playlists are found, this function iniyializes the playists for this player
*/
GList *
playlists(struct audio_priv *this)
{
        dbg(lvl_debug, "stub's playlists method\n");
        GList * playlists=NULL;
        playlists = stub->playlists;
        if(playlists==NULL)
		{
			dbg(lvl_error,"No Playlists found. Try again.\n");
			/*
			 * add function to create a playlists-list from the data of your api or lib
			 */
			 
			/* this block is potentially rubbish you should provide a
			 * function which cares about your playlists
			 */ 
			int i; 
			for(i=0;i<9;i++){
				struct audio_playlist *apl = g_new0(struct audio_playlist, 1);
				apl->name=g_strdup(stub_get_playlist_name(i));
				apl->index = i;
				apl->icon = "media_hierarchy";
				playlists=g_list_append(playlists, apl);
				stub->playlists = playlists;
			}
			return stub->playlists;
        }
        return playlists;
}

/**
* @brief this function returns a list of tracks for a specific playlist 
*
* @param this the audio player object
* @param playlist_index the index of the playlist to get the tracks from
*
* @return the list of tracks
*/
GList *
tracks(struct audio_priv *this, int playlist_index)
{
        GList * tracks=NULL;
        struct audio_track *t;
        int i, track_count;
        if ( playlist_index < 0 ) {
                playlist_index = stub->playlist_index;
        }
        
        if(playlist_index != stub->playlist_index)
		{
			stub_set_active_playlist(playlist_index);
		}
        track_count = stub_get_current_playlist_items_count();
        dbg(lvl_debug, "playlist_index: %i\n", playlist_index);
        for (i = 0; i < track_count; i++) {
                t = g_new0(struct audio_track, 1);
                t->name=g_strdup(stub_get_track_name(i));
                t->index=i;
                t->status=0;
                t->icon = "media_audio";
                tracks=g_list_append(tracks, t);
        }
		dbg(lvl_debug, "Active playlist updated\n");
        return tracks;
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
GList*
actions(struct audio_priv *this){
	dbg(lvl_debug, "In stub's actions\n");
	GList *act = NULL;
	act = stub->actions;
	if(act){
		return act;
	}else{
		stub->actions = stub_get_actions();
	}
	return stub->actions;
}

/**
* @brief this function iterates over all possible actions for this player and searches for an action
*
* @param actions the list of actions 
* @param action the action we want to find
*
* @return the audio action object wh searched or NULL if its not present
*/
struct audio_actions*
get_specific_action(GList* actions, int specific_action)
{
	GList* result = g_list_first(actions);
	while(result != NULL && result->next != NULL){
		struct audio_actions *aa = result->data;
		if(aa->action == specific_action)
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
	dbg(lvl_debug, "In stub's action control\n");
	/** methosd where the defined actions are mentioned
	 * remove the case blocks for actions you do not need
	 */ 
	switch(action)
	{
		case AUDIO_PLAYBACK_PAUSE:{
			stub_pause();
			break;
		}
		case AUDIO_PLAYBACK_PLAY:{
			stub_play();
			break;
		}
		case AUDIO_PLAYBACK_TOGGLE:{
			stub_toggle_playback(get_specific_action(stub->actions, AUDIO_PLAYBACK_TOGGLE));
			break;
		}
		case AUDIO_PLAYBACK_NEXT_TRACK:{
			++g_track_index;
			// call next track here
			 
			break;
		}
		case AUDIO_PLAYBACK_PREVIOUS_TRACK:{
			if (g_track_index > 0)
			{
				--g_track_index;
				// call previous track here
			}
			break;
		}
		case AUDIO_PLAYBACK_NEXT_PLAYLIST:{
			//stub_next_playlist();
			break;
		}
		case AUDIO_PLAYBACK_PREVIOUS_PLAYLIST:{
			//stub_prev_playlist();
			break;
		}		
		case AUDIO_PLAYBACK_NEXT_ARTIST:{
			//stub_next_artist();
			break;
		}
		case AUDIO_PLAYBACK_PREVIOUS_ARTIST:{
			//stub_prev_artist();
			break;
		}	
		case AUDIO_MISC_DELETE_PLAYLIST:{
			//stub_delete_playlist();
			break;
		}
		case AUDIO_MODE_TOGGLE_REPEAT:{
			/* if your player has different repeat modes
			 * you can have different icons for each mode
			 */ 
			stub_toggle_repeat(get_specific_action(stub->actions, AUDIO_MODE_TOGGLE_REPEAT));
			break;
		}
		case AUDIO_MODE_TOGGLE_SHUFFLE:{
			/* if your player has different shuffle modes
			 * you can have different icons for each mode
			 */
			stub_toggle_shuffle(get_specific_action(stub->actions, AUDIO_MODE_TOGGLE_SHUFFLE));
			break;
		}
		case AUDIO_MISC_RELOAD_PLAYLISTS:{
			/* maybe you'll never need this
			 */ 
			//reload_playlists(stub);
			break;
		}
		default:{
			dbg(lvl_error,"Don't know what to do with action '%i'. That's a bug\n", action);
			break;
		}
	}
	return action;
}

/**
* @brief this function provides the playback control for the audio player
*
* @param this the audio player object
* @param action the index of the song to play
*
* @return the playback status
* 
* this function is accessed, when one clicks on a specific song in gui internal
*/
static int
playback(struct audio_priv *this, const int action)
{
	dbg(lvl_debug, "In stub's playback control %i\n", action);
	/* here are the players playback functions
	 * it's called when a track is clicked to play that track
	 * 
	 */ 
	if ( action > -1 ) {
		g_track_index = action;
		stub_play();
		// call play_track_function here
	} else
		dbg(lvl_error,"Don't know what to do with play track '%i'. That's a bug\n", action);
	if(stub->playing) return 1;
	return 0;
}

/**
* @brief this function provides the volume control for the audio player
*
* @param this the audio player object
* @param action the kind of action to perform
*
* @return the resulting volume
* 
* possible values are:
* 	AUDIO_MUTE: save the current volume and set the mpd volume to 0
* 	AUDIO_LOWER_VOLUME: decrease the volume by 1
* 	AUDIO_RAISE_VOLUME: increase the volume by 1
*   any positive integer between 1 and 100: Set the value as volume
*/
static int 
volume(struct audio_priv *this, const int action)
{
	switch(action)
	{
		case AUDIO_MUTE:
			if(stub->muted){
				stub_set_volume(stub->volume);
			}else{
				//mute volume here
			}
			stub->muted = !stub->muted;
			break;
		case AUDIO_LOWER_VOLUME:
			// reduce volume here
			if(stub->volume >= 0){
				 stub->volume--;
			}else{
				stub->volume = 0;
			}
			break;
		case AUDIO_RAISE_VOLUME:
			//raise volume here
			if(stub->volume <= 100){
				stub->volume++; 
			}else{ 
				stub->volume = 100;
			}
			break;
		default:
			if(action >0){
				// set volume to i here
				stub->volume = action;
			} else {
				dbg(lvl_error,"Don't know what to do with action '%i'. That's a bug\n", action);
			}
			break;
	}

	return stub->volume;
}

/**
* @brief this function returns the currently playing trsck
*
* @param this the audio player object
*
* @return the track name of the current track
*/
char*
current_track(struct audio_priv *this)
{
	return "current track";
	//return stub_get_current_track_name();
}

/**
* @brief this function returns the currently loaded playlist
*
* @param this the audio player object
*
* @return the playlist name
*/
char*
current_playlist(struct audio_priv *this)
{
	return "current playlist";
	//return stub_get_current_playlist_name();
}

/**
* @brief the audio methods
*
* these methods are acessible from anywhere in the program. 
* actions you do not need, should be NULL.
*/
static struct audio_methods player_stub_meth = {
		volume,
        playback,
        action_do,
        tracks,
        playlists,
        actions,
        current_track,
        current_playlist,
        stub_get_attr,
        stub_set_attr,
};

/**
* @brief Inizialisation of the audio plugin instance data
*
* @param meth The methods to be overidden by the plugins methods
* @param cbl A list of callbacks which are called from the plugin
* @param attrs A list of attributes which are generated from navit.xml
* @param parent reference to the parant attribute
* 
* @return The audio plugin instance data
* 
*/
static struct audio_priv *
player_stub_new(struct audio_methods *meth, struct callback_list * cbl, struct attr **attrs, struct attr *parent) 
{
    struct attr *attr, *playing, *shuffle, *repeat;
    
    dbg(lvl_debug,"Initializing stub\n");
	srandom(time(NULL));
	stub = g_new0 (struct audio_priv, 1);
	/* example for reading an attribute from navit.xml
	 * here is the path to the music directory given
	 */
    if ((attr = attr_search (attrs, NULL, attr_music_dir)))
	{
		stub->musicdir = g_strdup(attr->u.str);
		dbg (lvl_info, "found music directory: %s\n", stub->musicdir);
	}
   
	/**
	 * place init code for your player implementation here
	 */
	stub->idle = callback_new_1 (callback_cast (stub_stub_idle), stub);
    stub->timeout = event_add_timeout(1000, 1,  stub->idle);
    stub->callback = callback_new_1 (callback_cast (stub_stub_idle), stub);
    stub->timeout = event_add_timeout(1000, 1,  stub->callback);

    stub->playing = false;
	stub->attrs=attrs;
    //*
    playing = attr_search(stub->attrs, NULL, attr_playing);

    if(!playing){
		playing = g_new0( struct attr, 1);
		playing->type = attr_playing;
		stub->attrs=attr_generic_add_attr(stub->attrs, playing);
		dbg (lvl_debug,"*\n");
	}	
	repeat = attr_search(stub->attrs, NULL, attr_repeat);

    if(!repeat){
		repeat = g_new0( struct attr, 1);
		repeat->type = attr_repeat;
		stub->attrs=attr_generic_add_attr(stub->attrs, repeat);
		dbg (lvl_debug,"*\n");
	}	
	shuffle = attr_search(stub->attrs, NULL, attr_shuffle);

    if(!shuffle){
		shuffle = g_new0( struct attr, 1);
		shuffle->type = attr_shuffle;
		stub->attrs=attr_generic_add_attr(stub->attrs, shuffle);
		dbg (lvl_debug,"*\n");
	}	
    //*/ 

    dbg (lvl_debug,  "Callback created successfully\n");

	stub->cbl = cbl;
    *meth=player_stub_meth;

    return stub;
}


/**
* @brief plugin entry point
*/
void
plugin_init(void)
{ 
	dbg (lvl_debug,  "player-stub\n");
    plugin_register_category_audio("player-stub", player_stub_new);
}
