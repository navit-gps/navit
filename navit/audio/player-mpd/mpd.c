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
#include "mpd.h"

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define PLAYLIST 1
#define TRACK 2
#define NO_TRACK ' '

/// Index to the next track
static int g_track_index;

static audio_fifo_t g_audiofifo;

// placeholder for the current track used to communicate with the osd item
char track[64];


struct mpd
{
    struct navit *navit;
    struct callback *callback;
    struct event_timeout *timeout;
    struct attr **attrs;
    GList* current_playlist; 	// the currently playing album/playlist
    GList* playlists;		// the head of the playlist list
    GList* actions;		// all possible actions
    gchar *musicdir;		// path where the music is stored
    int num_playlists;		// count value for the number of playlists
    int playlist_index;		// the index of the currently loaded list
    gboolean random_track;	// setting for randomized playback, true means play randomized
    gboolean random_playlist;	// setting for randomized playback of playlists, true means choose a playlist randomized
    gboolean repeat;		// setting for repeated playback. 
    gboolean single;		// setting for single playback the playback stops if the track ended. 
    gboolean shuffle;		// setting for shuffle playback
    char current_track[64];
    int volume;			// stores the volume for mpc - needs to be restored if the audio output was muted
    int width;
    gboolean muted;		// muting status true is volume = 0
    gboolean playing;		// playing status true is playing
} *mpd;

struct audio_priv 
{
    struct mpd *mpd;
};

GList* sort_playlists(GList* list);

char * mpd_get_playlist_name (int playlist_index);
char* get_playlist_name(GList* list);
void mpd_play(void);
void mpd_play_track(int track);
GList* get_entry_by_index(GList* list, int index);
//void save_playlist(GList* list);
int mpd_get_playlists_count (void);
void reload_playlists(struct mpd* this);

// playlist functions

/**
* @brief this function checks if the audio player is playing
*
* @return the status of the player. True is playing.
*
* this function checks if the player is playing. 
* If the player 'says' not playing it asks the mpc process if its playing.
*/
gboolean mpd_get_playing_status(void){
	if(mpd->playing)
		return mpd->playing;
	// check if mpd is really paused
	FILE *fp;
	fp = popen("mpc ", "r");
	char text[32] = {0,};
    if (fp == NULL) {
        dbg(lvl_error, "Failed to run command\n" );
        return false;
    }
    while (fgets(text, sizeof(text)-1, fp) != NULL) {
		if(strstr(text, "[playing]")){
			mpd->playing = true;
			pclose(fp);
			return true;
		}else if (strstr(text, "[paused]")){
			mpd->playing = false;
			pclose(fp);
			return false;
		}
	}	
	pclose(fp);
	return false;
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
	dbg(lvl_error, "No playlists or data is corrupted\n");
	reload_playlists(mpd);
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
/**
* @brief choose a playlist randomly
*
* @param list the entire list of playlists
*
* @return the randomly chosen list element 
*/
GList*
random_entry(GList* list){
	int n = mpd_get_playlists_count();
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
	if(mpd->random_playlist){
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
	if(mpd->random_playlist){
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
	char command[100] = {0,};
    strcat(command, "mpc rm \"");
    strcat(command, get_playlist_name(list));
    strcat(command, "\"");
    system(&command[0]);
    return g_list_delete_link(g_list_first(list),list);
}

/**
* @brief Initialize the mpd database and create the list of playlists
*
* @return the created list of playlists alphabetically sorted
*/
GList*
load_playlists(void)
{
    DIR           *d;
    struct dirent *dir;
    GList* list = NULL;
    dbg(lvl_debug, "MusicDir: %s\n", mpd->musicdir);
    d = opendir(mpd->musicdir);
    system("mpc clear");
    if (d)
	{
        while ((dir = readdir(d)) != NULL)
		{
           
            if(dir->d_name[0] != '.')
			{
                dbg(lvl_debug,  "%s\n", dir->d_name);
                char command[100] = {0,};
                strcat(command, "mpc ls \"");
                strcat(command, dir->d_name);
                strcat(command, "\" | mpc add");
                system(&command[0]);
                dbg(lvl_debug,  command);
                dbg(lvl_debug,  "\n");
                char command2[100] = {0,};
                strcat(command2, "mpc save \"");
                strcat(command2, dir->d_name);
                strcat(command2, "\"");
                system(command2);
                //system("mpc playlist");
                system("mpc clear");
                dbg(lvl_debug,  command2);
                dbg(lvl_debug,  "\n");
                list = insert_right(list, new_audio_playlist(dir->d_name, -1));
            }
		}
        closedir(d);
        
    }
    return sort_playlists(list);
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
	dbg(lvl_debug,  "\n");
	dbg(lvl_debug,  "start\n");
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
    dbg(lvl_debug,  "end\n");
    return list;
}

/**
* @brief this function loads a playlist to mpd/mpd
*
* @param list the playlist to be loaded
*/
void
load_playlist(GList * list){
	if(list){
		char command[100] = {0,};
		system("mpc clear");
		strcat(command, "mpc load \"");
		strcat(command, get_playlist_name(list));
		strcat(command, "\"");
		system(&command[0]);
		//save_playlist(list);
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
    mpd->playlist_index = g_list_index(mpd->playlists, next->data);
    mpd_play();
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
	if(head != NULL)
	{
		dbg(lvl_debug,  "Search Entry: %s\n",data);
		GList* current = head;
		struct audio *currend_data = NULL;
		int cmp = -1;
		while(cmp != 0){
			currend_data = get_playlist_data(current);
			if(currend_data){
				if(currend_data->name){
					dbg(lvl_debug,  "Got Entry: %s\n",currend_data->name);
					cmp = strcmp(currend_data->name, data);
					if(current == NULL)
					{
						return NULL; //nothing found!
					}
					if(cmp != 0){
						current = current->next;
					}
				}
			}
		}
		return current; //found!
	}
	return NULL;
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
	return g_list_nth(list, index);
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
    mpd->playlist_index = g_list_index(mpd->playlists, previous->data);
    mpd_play();
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
    char ca[32] = {0,};
    char na[32] = {0,};
    int i,j;
    for(i=0; i<32; i++)
	{
		if(get_playlist_data(current)->name[i] == '-')
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
            if(get_playlist_data(current)->name[j] == '-')
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
    
    system("mpc clear");
    load_playlist(current);
    mpd_play();
    return current;
}

void
mpd_next_artist(void)
{
	mpd->current_playlist = change_artist(mpd->current_playlist,1);
	printf("next Artist");
}

void
mpd_prev_artist(void)
{
	mpd->current_playlist = change_artist(mpd->current_playlist,0);
	printf("next Artist");
}

/**
* @brief test if a directory on the file system exists
*
* @param dir_name path of the directory
*
* @return true if the directory exists, false otherwise. surprise.
*/
gboolean
directory_exists(char* dir_name)
{
	gboolean exists = false;
	dbg(lvl_debug, "%s\n", dir_name);
	DIR* dir = opendir(dir_name);
	if (dir)
	{
		exists = true;
		closedir(dir);
	}
	return exists;
}

/**
* @brief this function checks if mpd already knows playlists and gets it or creates a new list of playlists
*
* @return the list of playlists
* 
* mpd stores the saved playlists on the file system and can be asked for
* them. If the audio player gets a list of playlists from mpd, it will 
* create a list of playlists from them. Otherwise it creates a new list 
* of playlist from the music that is stored inside the music directory
*/
GList*
check_playlists(void)
{
    FILE *fp;
    char _playlist[64];
    GList* list = NULL;
    gchar md[100] = {0,};
    int i = 0;
   
    fp = popen("mpc lsplaylists", "r");
    if (fp == NULL) {
        dbg(lvl_error, "Failed to run command\n" );
        return(NULL);
    }

    while (fgets(_playlist, sizeof(_playlist)-1, fp) != NULL) {
        strtok(_playlist, "\n");
        dbg(lvl_debug, "Check Playlist: %s\n", _playlist);
        i = 0;
        
        while(i<100 && mpd->musicdir[i] != 0){
			md[i] = mpd->musicdir[i];
			i++;
		}
		for(;i<100;i++){
			md[i] = 0;
		}
        dbg(lvl_debug,"Check Dir: %s/%s\n", mpd->musicdir, _playlist);
        if(directory_exists(strcat(strcat(md,"/"),_playlist)))
        {
			dbg(lvl_debug,"%s exists\n",md);
			list = insert_right(list, new_audio_playlist(_playlist, -1));	
			dbg(lvl_debug, "list = insert_right(list, new_audio_playlist(_playlist, -1));\n\n");
		} 
		else 
		{
			pclose(fp);
			dbg(lvl_debug,"%s doesnt exist\n",md);
			dbg(lvl_debug, "reload_playlists(mpd);\n\n");
			reload_playlists(mpd);
			return mpd->playlists;
		}
    }
    pclose(fp);
    return list;
}

/**
* @brief this function deletes all playlists
*
* @param this the audio player object
*/
void 
delete_all_playlists(struct mpd* this)
{
    GList *head = g_list_first(this->playlists);
    GList *current = g_list_first(this->current_playlist);
    if(head == NULL || current == NULL)
		{ // 
		dbg(lvl_error,"no playlists found\n");
		FILE *fp;
		char text[64];
		fp = popen("mpc lsplaylists", "r");
		if (fp == NULL) {
			dbg(lvl_error, "Failed to run command 'mpc lsplaylists'\n" );;
			return;
		}
		while (fgets(text, sizeof(text)-1, fp) != NULL) {
			strtok(text, "\n");
			char command[100] = {0,};
			strcat(command, "mpc rm \"");
			strcat(command, text);
			strcat(command, "\"");
			system(&command[0]);
			dbg(lvl_debug,command);
			dbg(lvl_debug,"\n");
		}
		pclose(fp);
		dbg(lvl_debug,"return;\n");
		return;
	}
    while (current->next != NULL)
	{
        dbg(lvl_debug,  "Delete: %s\n", get_playlist_name(current));
        current = delete_playlist(current);
        print_all(current);
    }
    char command[100] = {0,};
    strcat(command, "mpc rm \"");
    strcat(command, get_playlist_name(current));
    strcat(command, "\"");
    system(&command[0]);
    dbg(lvl_debug,command);
    system("mpc update --wait");
    dbg(lvl_debug,"%s\n",get_playlist_name(current));
    this->playlists = current;
    this->current_playlist = current;
}

/**
* @brief this function deletes and reloads all playlists
*
* @param this the audio player object 
*
* This function is used to clean all playlist data and reload the playlist
* this might be useful if the music is stored on a usb media and the usb 
* storage device is changed to change the music
*/
void 
reload_playlists(struct mpd* this)
{
	dbg(lvl_debug, "\nreload_playlists\n\n");
	delete_all_playlists(this);                      
	system("mpc stop");
	system("mpc update");
	this->playlists = load_playlists();
	sort_playlists(this->playlists);
	this->current_playlist = this->playlists;
	system("mpc update");
}

/**
* @brief this function reads the currently by mpd loaded playlist and 
* sets the playlist pointer on the corresponding object in the list of 
* playlists
*
* @param this the audio player object
*
* @return the pointer to the read playlist
* 
* When the embedded device is shut down mpd keeps the currently loaded playlist and track position. On restart 
*/
GList*
get_last_playlist(struct mpd* this)
{
    FILE *fp;
    GList* playlist = NULL;
    char text[64];
    char *playlist_name;
    fp = popen("mpc -f %file%","r");
    if (fp == NULL) {
        dbg(lvl_error, "Failed to open file\n" );  
        return NULL;
    }
    if(fgets(text, sizeof(text)-1, fp) != NULL) {
		playlist_name = strtok(text, "/");
		playlist = get_entry(this->playlists, playlist_name);
        if(playlist == NULL){
			playlist = this->playlists;
		}else{
			mpd->current_playlist = playlist;
		}
    }
    fclose(fp);
    return playlist;
}


/**
* @brief this function reads the track name to print it on the gui
*
* @param track_index the index of the track
*
* @return the track name
*/
char *
mpd_get_track_name (int track_index)
{
	FILE *fp;
    int l = 0;
    
    fp = popen("mpc playlist -f %title%", "r");
    if (fp == NULL) {
        dbg(lvl_error, "Failed to run command 'mpc playlist'\n" );
        return "Failed to run command 'mpc playlist'\n";
    }
    while (fgets(track, sizeof(track)-1, fp) != NULL) {
        strtok(track, "\n");
        if(l == track_index)
		{
			fclose(fp);
			return track;
		}
		if(l>64)
		{
			fclose(fp);
			return "NULL (track-name)";
		}
		l++;
	}
	fclose(fp);
    return "NULL (track name)";
}

/**
* @brief this function gets and returns the current playing playlist
*
* @return the playlist name
* 
* this function is used for OSD and GUI
*/
char *
mpd_get_current_playlist_name ()
{
	
    char *data = get_playlist_name(mpd->current_playlist);
	if(data == NULL)
	{
		return " ";
	}
	return data;
}

/**
* @brief this function reads the playlist name to print it on the gui
*
* @param playlist_index the index of the playlist
*
* @return the playlist name
*/
char *
mpd_get_playlist_name (int playlist_index)
{

	dbg( lvl_debug,  "%s\n", get_playlist_name(mpd->current_playlist));
	GList *p = get_entry_by_index(mpd->playlists, playlist_index);
	if(p != NULL)
	{
		char *data = get_playlist_name(p);
		if(data != NULL)
		{
			dbg(lvl_debug,  "%s\n", data);
			return data;
		}
	}
	dbg( lvl_debug,  "%s\n", get_playlist_name(mpd->current_playlist));
	return "NULL (playlist name index)";//text;
}

/**
* @brief this function starts playback for the track from the current playlist
*
* @param track_index the number of the track
*/
void
mpd_set_current_track (int track_index)
{
	mpd_play_track(track_index + 1);
    g_track_index = track_index;
}

/**
* @brief this funtion reads the number of tracks for the current playlist
*
* @return the number of tracks
*/
int
mpd_get_current_playlist_items_count ()
{
	FILE *fp;
    int l = 0;
    char text[64];
    char command[100]= {0,};
    strcat(command, "mpc playlist");
    dbg(lvl_debug,  "Run command '%s'\n", command );
    fp = popen(command, "r");
    if (fp == NULL) 
    {
        dbg(lvl_error, "Failed to run command '%s'\n", command );
		pclose(fp);
        return 0;
    }
    while (fgets(text, sizeof(text)-1, fp) != NULL) 
    {
		l++;
	}
	pclose(fp);
	dbg (lvl_debug,  "%i\n",l);
	return l;
}

/**
* @brief this function counts all playlists and returns the number
*
* @return the number of playlists
*/
int
mpd_get_playlists_count(void)
{
	FILE *fp;
    int l = 0;
    char text[64];
    fp = popen("mpc lsplaylists", "r");
    if (fp == NULL) {
        dbg(lvl_error, "Failed to run command 'mpc lsplaylists'\n" );
        return 0;
    }
    while (fgets(text, sizeof(text)-1, fp) != NULL) {
		l++;
	}
	pclose(fp);
	return l;
}

/**
* @brief this function starts playback for the given playlist
*
* @param playlist_index the given playlist to play
*/
void
mpd_set_active_playlist (int playlist_index)
{
    mpd->playlist_index = playlist_index;
    mpd->current_playlist = get_entry_by_index(mpd->playlists, playlist_index);
    if(mpd->current_playlist == NULL){
		mpd->playlist_index = -1;
		return;
	}
	system("mpc clear");
    load_playlist(mpd->current_playlist);
	mpd_play();
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
mpd_get_actions(void){
	GList* actions = NULL;
	struct audio_actions *aa;
	if(mpd->actions){
		return mpd->actions;
	}else{
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_PREVIOUS_ARTIST;
		aa->icon = g_strdup("media_prev_artist");
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_PREVIOUS_PLAYLIST;
		aa->icon = g_strdup("media_prev");
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_PREVIOUS_TRACK;
		aa->icon = g_strdup("media_minus");
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_TOGGLE;
		if(mpd->playing){
			aa->icon = g_strdup("media_pause");
		}else{
			aa->icon = g_strdup("media_play");
		}
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_NEXT_TRACK;
		aa->icon = g_strdup("media_plus");
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_NEXT_PLAYLIST;
		aa->icon = g_strdup("media_next");
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_PLAYBACK_NEXT_ARTIST;
		aa->icon = g_strdup("media_next_artist");
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_MODE_TOGGLE_REPEAT;
		if(mpd->single && mpd->repeat){
			aa->icon = g_strdup("media_repeat_track");
		}else if(!mpd->single && mpd->repeat){	
			aa->icon = g_strdup("media_repeat_playlist");
		}else{
			aa->icon = g_strdup("media_repeat_off");
		}
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_MODE_TOGGLE_SHUFFLE;
		if(mpd->random_track && mpd->random_playlist){
			aa->icon = g_strdup("media_shuffle_tracks_playlists");
		}else if(!mpd->random_track && mpd->random_playlist){
			aa->icon = g_strdup("media_shuffle_playlists");
		}else if(mpd->random_track && !mpd->random_playlist){
			aa->icon = g_strdup("media_shuffle_tracks");
		}else{
			aa->icon = g_strdup("media_shuffle_off");
		}
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_MISC_DELETE_PLAYLIST;
		aa->icon = g_strdup("media_trash");
		actions = g_list_append(actions, aa);
		
		aa = g_new0(struct audio_actions, 1);
		aa->action = AUDIO_MISC_RELOAD_PLAYLISTS;
		aa->icon = g_strdup("media_close");
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
mpd_mpd_idle (struct mpd *mpd)
{
	dbg(lvl_debug, "%s\n", mpd_get_current_playlist_name());
    if(mpd->current_playlist == NULL)
	{
		dbg(lvl_error,"Playlist=nULL\n");
		return;
	}
    FILE *fp;
    int l = 0, j = 0;
    char text[64] = {0,};
    fp = popen("mpc -f %title%", "r");
    if (fp == NULL) 
    {
        dbg(lvl_error, "Failed to run command 'mpc'\n" );
        return;
    }
    while (fgets(text, sizeof(text)-1, fp) != NULL) {
		strtok(text, "\n");
		if(l==0)
		{
			if(mpd->playing){
				if(strstr(text, "volume:") != NULL && mpd->current_track[0] != NO_TRACK)
				{
					// playlist ist zu ende.. die naechste bitte ;)
					// playlist has finished, the next please.
					system("mpc clear");
					mpd->current_playlist = load_next_playlist(mpd->current_playlist);
					for(j=0; j<64;j++)
					{
						mpd->current_track[j] = NO_TRACK;
					}
					mpd_play();
					fclose(fp);
					return;
				}else{
					strcpy(mpd->current_track, text);
					dbg(lvl_debug, "%s\n",mpd->current_track);
				}
			}
		}else{
			// get mpc output and save as attribute
			
			if(strstr(text, "volume:") != NULL)
			{
				
				if(!mpd->muted){
					int i0 = text[7];
					int i1 = text[8];
					int i2 = text[9];
					int i = (i0=='1')?100:0;
					i+=(i1-'0')*10;
					i+=(i2-'0');
					mpd->volume = i;
				}
				if(strstr(text, "repeat: off") != NULL)
				{
					mpd->repeat = false;
				}
				else if(strstr(text, "repeat: on") != NULL)
				{
					mpd->repeat = true;
				}
				if(strstr(text, "random: off") != NULL)
				{
					mpd->random_track = false;
				}
				else if(strstr(text, "random: on") != NULL)
				{
					mpd->random_track = true;
				}
				if(strstr(text, "single: off") != NULL)
				{
					mpd->single = false;
				}
				else if(strstr(text, "single: on") != NULL)
				{
					mpd->single = true;
				}
			}
		}
		l++;
	}
	fclose(fp);

}

/**
* @brief play :)
*/
void mpd_play(void)
{
	mpd->playing = true;
	system("mpc play");
}

/**
* @brief play a specific track
*
* @param track the number of the track to play
*/
void mpd_play_track(int track)
{
	char command[15] = {0,};
	mpd->playing = true;
	sprintf(command, "mpc play %i", track);
	system(&command[0]);
}

/**
* @brief load the next playlist
*/
void
mpd_next_playlist(void)
{
	dbg(lvl_debug,  "\n");
	mpd->current_playlist = load_next_playlist(mpd->current_playlist);
	
}

/**
* @brief load the previous playlist
*/
void
mpd_prev_playlist(void)
{
	dbg(lvl_debug,  "\n");
	mpd->current_playlist = load_prev_playlist(mpd->current_playlist);
}

/**
* @brief getter for the volume value of the audio player object
*
* @return the volume
*/
int mpd_get_volume(void)
{
	return mpd->volume;
}

/**
* @brief sets the volume value
*
* @param vol the volume value to set
*/
void mpd_set_volume(int vol)
{
	char command[100] = {0,};
	sprintf(command, "mpc volume %i", vol);
	system(command);
	mpd->volume = vol;
}

/**
* @brief delete the current playlist
*/
void mpd_delete_playlist(void){
	dbg(lvl_debug, "Hi, I was told to delete this playlist: %s", mpd_get_current_playlist_name());
	delete_playlist(get_entry(mpd->playlists, mpd_get_current_playlist_name()));
}

/**
* @brief this function toggles the repeat mode
*
* @param action the action that owns the toggle
*/
void mpd_toggle_repeat(struct audio_actions *action){
	
	int toggle = 0;
	if(mpd->single) toggle++;
	if(mpd->repeat) toggle += 2;
	switch(toggle){
		case 0:// no repeat
		case 1:{
			system("mpc repeat on");
			system("mpc single off");
			if(action != NULL){
				action->icon = g_strdup("media_repeat_playlist");
			}
			dbg(lvl_debug, "\nrepeat playlist\n");
			break;
		}
		case 2:{// repeat playlist
			system("mpc single on");
			system("mpc repeat on");
			if(action != NULL){
				action->icon = g_strdup("media_repeat_track");
			}
			dbg(lvl_debug, "\nrepeat track\n");
			break;
		}
		case 3:{// repeat track
			system("mpc repeat off");
			system("mpc single off");
			if(action != NULL){
				action->icon = g_strdup("media_repeat_off");
			}
			dbg(lvl_debug, "\nrepeat off\n");
			break;
		}
	}
}

/**
* @brief this function toggles the shuffle mode
*
* @param action the action that owns the toggle
*
* starting from OFF state the command switches through
* 	shuffle track only (of the loaded playlist)
* 	shuffle playlist and track (a randomly chosen track from the entire music database)
* 	shuffle playlist only (play all tracks on the playlist in the given order but coose the next playlist randomly)
*/
void mpd_toggle_shuffle(struct audio_actions *action){

	int toggle = 0;
	if(mpd->random_track) toggle++;
	if(mpd->random_playlist) toggle += 2;
	dbg(lvl_debug,  "Toggle Shuffle: %i\n", toggle);
	switch(toggle){
		//0 -> 1 -> 3 -> 2 -> 0
		case 0:{ 
			system("mpc random on");
			mpd->random_track = TRUE;
			mpd->random_playlist = FALSE;
			if(action != NULL){
				action->icon = g_strdup("media_shuffle_playlists");
			}
			dbg(lvl_debug,  "Toggle Shuffle Playlists: %i\n", toggle);
			break;
		}
		case 1:{
			system("mpc random on");
			mpd->random_track = TRUE;
			mpd->random_playlist = TRUE;
			if(action != NULL){
				action->icon = g_strdup("media_shuffle_tracks_playlists");
			}
			dbg(lvl_debug,  "Toggle Shuffle Tracks & Playlists: %i\n", toggle);
			break;
		}
		case 3:{
			system("mpc random off");
			mpd->random_track = FALSE;
			mpd->random_playlist = TRUE;
			if(action != NULL){
				action->icon = g_strdup("media_shuffle_tracks");
			}
			dbg(lvl_debug,  "Toggle Shuffle Tracks: %i\n", toggle);
			break;
		}
		case 2:{
			system("mpc random off");
			mpd->random_track = FALSE;
			mpd->random_playlist = FALSE;
			if(action != NULL){
				action->icon = g_strdup("media_shuffle_off");
			}
			dbg(lvl_debug,  "Toggle Shuffle OFF: %i\n", toggle);
			break;
		}
	}
}

/**
* @brief command to toggle playback
*
* @param action the action that owns the toggle
*
* the action must be passed because the playback action keeps the icon. This icon changes dependent on the playback state
*/
void
mpd_toggle_playback (struct audio_actions *action)
{
	if (mpd->playing)
	{
		dbg (0, "pausing playback\n");
		system("mpc pause");
		if(action != NULL){
			action->icon = g_strdup("media_play");
		}
	}
	else
	{
		dbg (0, "resuming playback\n");
		system("mpc play");
		if(action != NULL){
			action->icon = g_strdup("media_pause");
		}
	}
	mpd->playing = !mpd->playing;
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
        dbg(lvl_debug, "Mpd's playlists method\n");
        GList * playlists=NULL;
        playlists = mpd->playlists;
        if(playlists==NULL)
		{
			dbg(lvl_error,"No Playlists found. Try again.\n");
			mpd->playlists = load_playlists();
			return mpd->playlists;
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
                playlist_index = mpd->playlist_index;
        }
        
        if(playlist_index != mpd->playlist_index)
		{
			mpd_set_active_playlist(playlist_index);
		}
        track_count = mpd_get_current_playlist_items_count();
        dbg(lvl_debug, "playlist_index: %i\n", playlist_index);
        for (i = 0; i < track_count; i++) {
                t = g_new0(struct audio_track, 1);
                t->name=g_strdup(mpd_get_track_name(i));
                t->index=i;
                t->status=0;
                t->icon = "music-green"; //we init with the green icon - maybe we will use thsi for any kind of rating in the future
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
	dbg(lvl_debug, "In mpd's actions\n");
	GList *act = NULL;
	act = mpd->actions;
	if(act){
		return act;
	}else{
		mpd->actions = mpd_get_actions();
	}
	return mpd->actions;
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
	if(result){
		while(result->next != NULL){
			struct audio_actions *aa = result->data;
			if(aa->action == specific_action)
				return aa;
			result = g_list_next(result);
		}
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
	dbg(lvl_debug, "In mpd's action control\n");
	switch(action)
	{
		case AUDIO_PLAYBACK_PLAY:{	
			dbg (lvl_debug, "resuming playback\n");
			system("mpc play");
			mpd->playing = 1;
			break;
		}
		case AUDIO_PLAYBACK_PAUSE:{
			dbg (lvl_debug, "pausing playback\n");
			system("mpc pause");
			mpd->playing = 0;
			break;
		}
		case AUDIO_PLAYBACK_TOGGLE:{
			mpd_toggle_playback(get_specific_action(mpd->actions, AUDIO_PLAYBACK_TOGGLE));
			break;
		}
		case AUDIO_PLAYBACK_NEXT_TRACK:{
			++g_track_index;
			system("mpc next");
			break;
		}
		case AUDIO_PLAYBACK_PREVIOUS_TRACK:{
			if (g_track_index > 0)
			{
				--g_track_index;
				system("mpc prev");
			}
			break;
		}
		case AUDIO_PLAYBACK_NEXT_PLAYLIST:{
			mpd_next_playlist();
			break;
		}
		case AUDIO_PLAYBACK_PREVIOUS_PLAYLIST:{
			mpd_prev_playlist();
			break;
		}		
		case AUDIO_PLAYBACK_NEXT_ARTIST:{
			mpd_next_artist();
			break;
		}
		case AUDIO_PLAYBACK_PREVIOUS_ARTIST:{
			mpd_prev_artist();
			break;
		}	
		case AUDIO_MISC_DELETE_PLAYLIST:{
			mpd_delete_playlist();
			break;
		}
		case AUDIO_MODE_TOGGLE_REPEAT:{
			mpd_toggle_repeat(get_specific_action(mpd->actions, AUDIO_MODE_TOGGLE_REPEAT));
			break;
		}
		case AUDIO_MODE_TOGGLE_SHUFFLE:{
			mpd_toggle_shuffle(get_specific_action(mpd->actions, AUDIO_MODE_TOGGLE_SHUFFLE));
			break;
		}
		case AUDIO_MISC_RELOAD_PLAYLISTS:{
			reload_playlists(mpd);
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
	dbg(lvl_debug, "In mpd's playback control\n");
	if ( action > -1 ) {
		g_track_index = action;
		mpd_play_track(action + 1);
	} else {
		dbg(lvl_error,"Don't know what to do with play track '%i'. That's a bug\n", action);
	}
	if(mpd->playing) return 1;
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
	char command[18] = {0,};
	switch(action)
	{
		case AUDIO_MUTE:
			if(mpd->muted){
				mpd_set_volume(mpd->volume);
			}else{
				sprintf(command, "mpc volume 0");
				system(&command[0]);
			}
			mpd->muted = !mpd->muted;
			break;
		case AUDIO_LOWER_VOLUME:
			system("mpc volume -1");
			if(mpd->volume >= 0){
				 mpd->volume--;
			}else{
				mpd->volume = 0;
			}
			break;
		case AUDIO_RAISE_VOLUME:
			system("mpc volume +1");
			if(mpd->volume <= 100){
				mpd->volume++; 
			}else{ 
				mpd->volume = 100;
			}
			break;
		default:
			if(action >0 && action <= 100){
				sprintf(command, "mpc volume %i", action);
				system(&command[0]);
				mpd->volume = action;
			} else {
				dbg(lvl_error,"Don't know what to do with action '%i'. That's a bug\n", action);
			}
			break;
	}

	return mpd->volume;
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
	if(mpd->current_track[0] != NO_TRACK){
		return mpd->current_track;
	}else{
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
char*
current_playlist(struct audio_priv *this)
{
	return mpd_get_current_playlist_name();
}

/**
* @brief the audio methods
*
* these methods are acessible from anywhere in the program
*/
static struct audio_methods player_mpd_meth = {
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
player_mpd_new(struct audio_methods *meth, struct attr **attrs, struct attr *parent) 
{
    struct audio_priv *this;
    struct attr *attr;
    attr=attr_search(attrs, NULL, attr_music_dir);
    dbg(lvl_error,"Initializing mpd\n");
	srandom(time(NULL));
	mpd = g_new0 (struct mpd, 1);
    if ((attr = attr_search (attrs, NULL, attr_music_dir)))
      {
          mpd->musicdir = g_strdup(attr->u.str);
          dbg (lvl_error, "found music directory: %s\n", mpd->musicdir);
      }
   
	audio_init (&g_audiofifo);
	GList* pl = check_playlists();
	if(pl == NULL){
		reload_playlists(mpd);
	}else{
		mpd->playlists = sort_playlists(pl);
		pl = NULL;
		pl = get_last_playlist(mpd);
		if(pl == NULL){
			load_playlist(mpd->playlists);
		}
	}
	mpd_play();
    mpd->callback = callback_new_1 (callback_cast (mpd_mpd_idle), mpd);
    mpd->timeout = event_add_timeout(1000, 1,  mpd->callback);
    dbg (lvl_error,  "Callback created successfully\n");

    this=g_new(struct audio_priv,1);

    *meth=player_mpd_meth;
    return this;
}

/**
* @brief plugin entry point
*/
void
plugin_init(void)
{ 
	dbg (lvl_debug,  "player-mpd\n");
    plugin_register_audio_type("player-mpd", player_mpd_new);
}
