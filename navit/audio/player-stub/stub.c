/* vim: set tabstop=8 expandtab: */
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
#include "graphics.h"
#include "color.h"
#include "stub.h"

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define PLAYLIST 1
#define TRACK 2

/// Index to the next track
static int g_track_index;

static audio_fifo_t g_audiofifo;
char str[25]; // this string is just for visualisation of the functions
char track[64];

struct stub
{
	/* this is the data structure for the audio plugin
	 * you might not need every element of it
	 */ 
    struct navit *navit;
    struct callback *callback;
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
    gboolean playing;
} *stub;

struct audio_priv 
{
    struct stub *stub;
};

GList* sort_playlists(GList* list);

char * stub_get_playlist_name (int playlist_index);
char* get_playlist_name(GList* list);
void stub_play(void);
void stub_play_track(int track);
GList* get_entry_by_index(GList* list, int index);

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

int
reindex_playlists(GList *list)
{
    GList *current = list;
    int i = 0;
    get_playlist_data(list)->index = i;
    dbg(lvl_error, "playlist:%s \n\n", get_playlist_name(list));
    
    while (NULL != (current = current->next))
	{
        dbg(lvl_debug,  "playlist:%s \n", get_playlist_name(current));
        get_playlist_data(current)->index = ++i;
    }
    dbg(lvl_debug,  "%i Playlists indexed\n",i);
	return i;
}

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

GList*
insert_right(GList* list, struct audio_playlist* playlist)
{
	dbg(lvl_debug, "Appending playlist %s\n", playlist->name);
	return g_list_append(list, (gpointer) playlist);
}

void
print_all(GList* list)
{
	return;
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

GList*
random_entry(GList* list){
	int n = 9;
	int i = random();
	i %= n;
	dbg(lvl_debug,  "\tTest a random number: \t%i\n", i );
	return get_entry_by_index(g_list_first(list), i);
}

GList*
next_playlist(GList* list)
{
	if(stub->random_playlist){
		return random_entry(list);
	}
	return (list->next!=NULL)?g_list_next(list):g_list_first(list);
} 

GList*
prev_playlist(GList* list)
{
	if(stub->random_playlist){
		return random_entry(list);
	}
	return (list->prev!=NULL)?g_list_previous(list):g_list_last(list);
} 

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

GList* 
delete_and_free(GList *list)
{
    return g_list_delete_link(g_list_first(list),list);
}

GList*
delete_playlist(GList* list)
{	
    // remove the playlist from your api or lib here
    // and finally it removes the playlist from the list
    return g_list_delete_link(g_list_first(list),list);
}

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

void
swap_playlists(GList* a, GList* b)
{
    char* temp = NULL;
    temp = a->data;
    a->data = b->data;
    b->data = temp;
}

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

void 
save_playlist(GList* list){
	/*
	 * save the current playlist to a file
	 */
 } 

void
load_playlist(GList * list){
	if(list){
		// load your playlist into the player here
		save_playlist(list);
	}
}
GList* 
load_next_playlist(GList* current)
{
    GList* next = next_playlist(current);
	load_playlist(next);
    stub->playlist_index = g_list_index(stub->playlists, next->data);
    stub_play();
    return next;
}

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

GList*
get_entry_by_index(GList* list, int index)
{
	GList* entry = g_list_nth(list, index);
	return entry;
}

GList* 
load_prev_playlist(GList* current)
{
	GList* previous = prev_playlist(current);
    load_playlist(previous);
    stub->playlist_index = g_list_index(stub->playlists, previous->data);
    stub_play();
    return previous;
}

GList*
change_artist(GList* current, int next)
{
	/* this function iterates the playlists and checks the name
	 * it expects an "artist - album" named playlist
	 * if loads the first playlist with a differing "artist" substring
	 * the minus in the string is important 
	 */ 
    char ca[32] = {0,};
    char na[32] = {0,};
    int i,j;
    for(i=0; i<32; i++)
	{
		if(get_playlist_data(current)->name[i] == '-') // <= note the minus separator
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
            if(get_playlist_data(current)->name[j] == '-') // <= note the minus separator
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

GList*
next_artist(GList* current)
{
	return change_artist(current, 1);
}

GList*
prev_artist(GList* current)
{
	return change_artist(current, 0);
}

char *
stub_get_track_name (int track_index)
{
	/* this function should return the 
	 * track "track_index" of the current loaded playlist
	 */ 
	 sprintf(str, "track %i",track_index);
	 return str;
}

char *
stub_get_current_playlist_name ()
{
	/* return the name of the currently loaded playlist */

	return "currently loaded playlist";
}

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

void
stub_set_current_track (int track_index)
{
	stub_play_track(track_index + 1);
    g_track_index = track_index;
}

int
stub_get_current_playlist_items_count ()
{
	/* get and return the number of items in the currently 
	 * loaded playlist
	 */ 
	return 7;
}

int
stub_get_playlists_count ()
{
	/* get and return the number of items in the currently 
	 * loaded playlist
	 */ 
	return 9;
}

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


/*  Function to provide all possible audio actions for the player
 *  takes nothing
 *  returns a list of actions with their icons for GUI and OSD
 */ 
GList*
stub_get_actions(void){
	/*
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



static void
stub_stub_idle (struct stub *stub)
{
	/*
	 * periodic called function
	 * here you might get the current track and other information from
	 * your player api or lib
	 */ 

}

void stub_play(void)
{
	stub->playing = true;
	// start your playback
}

void stub_play_track(int track)
{
	stub->playing = true;
	// choose and play track track here
}

void
stub_next_playlist(void)
{
	dbg(lvl_debug,  "\n");
	stub->current_playlist = load_next_playlist(stub->current_playlist);
	
}

void
stub_prev_playlist(void)
{
	dbg(lvl_debug,  "\n");
	stub->current_playlist = load_prev_playlist(stub->current_playlist);
}

int stub_get_volume(void)
{
	return stub->volume;
}

void stub_set_volume(int vol)
{
	// set volume to vol here
	stub->volume = vol;
}



void stub_toggle_repeat(struct audio_actions *action){
	int toggle = 0;
	if(stub->single) toggle++;
	if(stub->repeat) toggle += 2;
	switch(toggle){
		case 0:// no repeat
		case 1:{
			
			if(action != NULL){
				action->icon = g_strdup("media_repeat_playlist");
			}
			//dbg(lvl_error, "\nrepeat playlist\n");
			break;
		}
		case 2:{// repeat playlist
			
			if(action != NULL){
				action->icon = g_strdup("media_repeat_track");
			}
			//dbg(lvl_error, "\nrepeat track\n");
			break;
		}
		case 3:{// repeat track
			
			if(action != NULL){
				action->icon = g_strdup("media_repeat_off");
			}
			//dbg(lvl_error, "\nrepeat off\n");
			break;
		}
	}
}

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
			//dbg(lvl_error,  "Toggle Shuffle Playlists: %i\n", toggle);
			break;
		}
		case 1:{
			
			stub->random_track = TRUE;
			stub->random_playlist = TRUE;
			if(action != NULL){
				action->icon = g_strdup("media_shuffle_tracks_playlists");
			}
			//dbg(lvl_error,  "Toggle Shuffle Tracks & Playlists: %i\n", toggle);
			break;
		}
		case 3:{
			
			stub->random_track = FALSE;
			stub->random_playlist = TRUE;
			if(action != NULL){
				action->icon = g_strdup("media_shuffle_tracks");
			}
			//dbg(lvl_error,  "Toggle Shuffle Tracks: %i\n", toggle);
			break;
		}
		case 2:{
			
			stub->random_track = FALSE;
			stub->random_playlist = FALSE;
			if(action != NULL){
				action->icon = g_strdup("media_shuffle_off");
			}
			//dbg(lvl_error,  "Toggle Shuffle OFF: %i\n", toggle);
			break;
		}
	}
}

void
stub_toggle_playback (struct audio_actions *action)
{
	if (stub->playing)
	{
		dbg (0, "pausing playback\n");
		// pause your playback here
		if(action != NULL){
			action->icon = g_strdup("media_play");
		}
	}
	else
	{
		dbg (0, "resuming playback\n");
		// resume your playback here
		if(action != NULL){
			action->icon = g_strdup("media_pause");
		}
	}
	stub->playing = !stub->playing;
}

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
			 * rubbish you should provide a function which cares about your playlists
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


struct audio_actions*
get_specific_action(GList* actions, int specific_action)
{
	GList* result = g_list_first(actions);
	while(result->next != NULL){
		struct audio_actions *aa = result->data;
		if(aa->action == specific_action)
			return aa;
		result = g_list_next(result);
	}
	return NULL;
}

static int
action_do(struct audio_priv *this, const int action)
{
	dbg(lvl_debug, "In stub's action control\n");
	/* methosd where the defined actions are mentioned
	 * remove the case blocks for actions you do not need
	 */ 
	switch(action)
	{
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

static int
playback(struct audio_priv *this, const int action)
{
	dbg(lvl_debug, "In stub's playback control\n");
	/* here are the players playback functions
	 * it's called when a track is clicked to play that track
	 * 
	 */ 
	if ( action > -1 ) {
		g_track_index = action;
		// call play_track_function here
	} else {
		dbg(lvl_error,"Don't know what to do with play track '%i'. That's a bug\n", action);
	}
	if(stub->playing) return 1;
	return 0;
}

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

char*
current_track(struct audio_priv *this)
{
	return "current track";
	//return stub_get_current_track_name();
}

char*
current_playlist(struct audio_priv *this)
{
	return "current playlist";
	//return stub_get_current_playlist_name();
}

static struct audio_methods player_stub_meth = {
        volume,
        playback,
        action_do,
        tracks,
        playlists,
        actions,
        current_track,
        current_playlist,
};

static struct audio_priv *
player_stub_new(struct audio_methods *meth, struct attr **attrs, struct attr *parent) 
{
    struct audio_priv *this;
    struct attr *attr;
    attr=attr_search(attrs, NULL, attr_music_dir);
    dbg(lvl_debug,"Initializing stub\n");
	srandom(time(NULL));
	stub = g_new0 (struct stub, 1);
	/* example for reading an attribute from navit.xml
	 * here is the path to the music directory given
	 */
    if ((attr = attr_search (attrs, NULL, attr_music_dir)))
      {
          stub->musicdir = g_strdup(attr->u.str);
          dbg (lvl_info, "found music directory: %s\n", stub->musicdir);
      }
   
	audio_init (&g_audiofifo);
	
	/*
	 * place init code for your player implementation here
	 */
	  
    stub->callback = callback_new_1 (callback_cast (stub_stub_idle), stub);
    stub->timeout = event_add_timeout(1000, 1,  stub->callback);
    dbg (lvl_debug,  "Callback created successfully\n");

    this=g_new(struct audio_priv,1);

    *meth=player_stub_meth;
    return this;
}


void
plugin_init(void)
{ 
	dbg (lvl_debug,  "player-stub\n");
    plugin_register_audio_type("player-stub", player_stub_new);
}
