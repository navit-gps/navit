/* app.c
 * For use with GTKeyboard
 * written by David Allen, s2mdalle@titan.vcu.edu
 * http://opop.nols.com/
 *
 * #define DEBUGGING at compile time for interesting info most people don't
 * want to see.
 */
/* GTKeyboard - A Graphical Keyboard For X
 * Copyright (C) 1999, 2000 David Allen  
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/extensions/shape.h>
#include <X11/Xmu/WinUtil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

/* The regular non-fun glib calls for distro copies of gtkeyboard. */
#  define g_new0_(t,c)      g_new0(t,c)
#  define g_free_(mem)      g_free(mem)
#  define g_new_(t,c)       g_new(t,c)
#  define g_strdup_(x)      g_strdup(x)
#  define g_malloc_(x)      g_malloc(x)
#  define MEM(x)            ;	/* Don't do anything */


/* modmap.h
 * Written by David Allen <s2mdalle@titan.vcu.edu>
 * 
 * Released under the terms of the GNU General Public License
 */




#define slot_number_to_mask(x)                          (1<<x)

typedef struct {
	KeyCode codes[4];
} ModmapRow;

typedef struct {
	int max_keypermod;	/* Alias for the entry in XModifierMap */
	ModmapRow modifiers[8];	/* Exactly 8 entries */
} ModmapTable;

static int file_exists(const char *filename);
static void send_redirect_a_keysym(KeySym input);
static unsigned long find_modifier_mask(KeyCode code);
static ModmapTable *ModmapTable_new(void);
static void ModmapTable_destroy(ModmapTable * table);
static int ModmapTable_insert(ModmapTable * table, KeyCode code, int slot);
static int mask_name_to_slot_number(char *maskname);
GtkWidget *build_keyboard(GtkWidget * input, char *filename);


/* templates.h
 * 
 * Written by David Allen <s2mdalle@titan.vcu.edu>
 * http://opop.nols.com/
 * Released under the terms of the GNU General Public License
 */

#define MAXIMUM_ROWS           6

/* Abstract data types */

typedef struct {
	KeySym lower_case;	/* What happens when pressed by itself */
	KeySym upper_case;	/* What happens when pressed with Shift/CL */
	KeySym alt_gr;		/* Alt GR+ button -- mostly unused */
	KeyCode code;
	char *aux_string;	/* Cheating string holder for things that aren't
				 * right with XKeysymToString() -- most notably
				 * the F keys and the keypad keys 
				 * This should NEVER be used in conjunction
				 * with foreign windows - only text insertion in
				 * our editing buffer
				 */
} KEY;

typedef struct {
	char *tab;
	char *backspace;
	char *caps_lock;
	char *space;
	char *alt;
	char *alt_gr;
	char *control;
	char *shift;
} KeyboardTranslation;

typedef struct {
	int row_values[MAXIMUM_ROWS];
	int keycount;
	KeySym *syms;
	KeyCode *codes;
	KeyboardTranslation *trans;
	char *name;
	ModmapTable *modmap;
} KEYBOARD;

/* Macros */
#define NO_KEYBOARD            ((KEYBOARD *)NULL)
#define NO_KEY                 ((KEY *)NULL)

/* Function prototypes */
static KEY *gtkeyboard_key_copy(KEY * key);
static KEY *gtkeyboard_keyboard_get_key(KEYBOARD * keyb, int row, int keyno);
static KEYBOARD *gtkeyboard_destroy_keyboard(KEYBOARD * input);
static KEYBOARD *read_keyboard_template(char *filename);
static KEY *gtkeyboard_destroy_key(KEY * input);
static KEY *gtkeyboard_new_key(const KeySym lower,
			const KeySym upper,
			const KeySym altgr, const char *alt);
gint track_focus(gpointer data);



/* rc_file.h - A simple configuration file reader/writer header file
 * by Patrick Gallot <patrick.gallot@cimlinc.com>
 *
 * This file is part of GTKeyboard and as such is licensed under the terms
 * of the GNU General Public License.
 */


/* Different types that resource file varaibles can have */
#define RC_NONE                 0x0000
#define RC_STR	                0x0002	/* String */
#define RC_PARSE_FUNC           0x0006	/* This one means that an external
					 * parsing function should be used
					 * and the resource file code 
					 * shouldn't bother with the values
					 */
typedef struct config_var {
	/* These go together to specify a line in a file that could say something
	 * like "set foo_variable 10" or "foobar = 20" or even 
	 * "toolbar menubar off"
	 */

	gchar *Prefix;		/* This could be something like "set" or "toolbar" */
	gchar *Name;		/* Element name */
	gint Type;		/* Type of the value */
	gpointer Val;		/* Pointer to the value to store */
	/* Function pointer for custom handling -- it should have parameters
	 * of the "prefix" token, (usually "set"), the variable name, the
	 * varaible value, and the pointer we were supposed to store it in.
	 * The pointer is usually going to be the same as Val above 
	 */
	int (*func) (char *prefix, char *varname, char *val, gpointer ptr);
} RCVARS;

static int read_ConfigFile(char *filename, RCVARS * vars, int complain);

#define CONDFREE(x)                      if(x){  g_free_(x);  x = NULL; }

#define FLUSH_EVERYTHING                 fflush(Q); fflush(stdout);\
                                         fflush(stderr);

/* Toggling Defines */
#define ON                               1
#define OFF                              0
/* Whatever the status is, FLIP IT */
#define ISOFF(x)                         ( x==OFF )

/* In case we ever achieve portability to winblows, we can change this
 * to a backslash.  :)
 */
#define _DIR                             "/"

#define FLUSH                            fflush(Q)
#define NONE                             NULL

#define LEFT_MOUSE_BUTTON                1
#define RIGHT_MOUSE_BUTTON               3
#define MIDDLE_MOUSE_BUTTON              2

#define Q                                stderr
#define ABOUT_INFO                       "about.data"
#define RC_FILENAME                      ".gtkeyboardrc"
#define PROVIDED_RCFILE            "example_configurations/defaults.gtkeyboard"
#define APPNAME                          "GTKeyboard"
#define CR                               "\n"



/* adt.h
 * For GTKeyboard
 * written by David Allen s2mdalle@titan.vcu.edu
 * http://opop.nols.com/
 *
 * This file is released under the terms of the GNU General Public License.
 * Please see COPYING for more details.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>

/* Maximum number of custom colors.  10 is a good number, above that is
 * really not needed per se.
 */
#define MAX_CUSTOM_COLORS         10

/**************************************************************************/

/* ADTs */
/* Program configuration settings */
typedef struct {
	/* Changed to bits in 0.97.3 to save a little space. */
	/* Different program options - initialized in app.c changed in 
	 * file_manip.c and messed with elsewhere.
	 *
	 * See app.c and the initialization code for comments on what they 
	 * all mean.
	 */
	int REDIRECT_POLICY_IMPLICIT;
	int STATUS_LOGGING;
	int CAPS_LOCK;
	int IGNORE_LAYOUT_FILE;
	/* int ASK_REMAP_ON_EXIT;            -- Deprecated for now... */
	int SHIFT;
	int ALT_GR;
	int NUMLOCK;
	int VERBOSE;
	int ALT;
	int CONTROL;
	int USE_KEYBOARD;


	char *home;		/* Home dir of user running the app.       */
	char *tempslot;
	char *extrafiles;	/* Extra files, docs, default rcfile, lic. */
	char *redirect_window_name;	/* Window name of redirection location.    */
	char *keyboard_file;
	char *cache_file;
	Window redirect_window;	/* Xlib window structure for redirection.  */
	Window other;		/* Temporary window structure for Xlib     */

	KEYBOARD *keyboard;
	KEYBOARD *old_keyboard;

	/* These memory areas won't be used unless PROD isn't defined.
	 * When PROD isn't defined, we're not debugging, and they essentially
	 * keep track of the number of memory calls that I make.  See also
	 * mem_header.h for a full definition of how they're used.
	 */
#if defined(GTKEYBOARD_MEMORY_DEBUGGING) || defined(DEBUGGING)
	long MEM;		/* Number of bytes allocated          */
	int gnew;		/* Number of gnew() calls (alloc)     */
	int gnew0;		/* Number of gnew0() calls (alloc)    */
	int gmalloc;		/* Number of g_malloc() calls (alloc) */
	int gstrdup;		/* Number of g_strdup() calls (alloc) */
	int gfree;		/* Number of g_free() calls (free)    */
#endif
} GTKeyboardOptions;

typedef struct {
	GtkWidget *keyboard;
	gint show_keyboard;
} GTKeyboardKeyboardElements;

typedef struct {
	int keyboard;
} GTKeyboardElements;

/****************************************************************************/

typedef struct {
	Window xwindow;		/* So we can play with xlib. (our window) */
	GtkWidget *KEYBOARD;	/* Container containing all keyb elements */


	GTKeyboardKeyboardElements keyboard_elements;

	/* If a field has a comment of "Cleaned" next to it, then it is
	 * either deallocated or destroyed in 
	 * callbacks.c:gtkeyboard_mem_cleanup() via macro calls.
	 */
	char *custom_colors[MAX_CUSTOM_COLORS];	/* Cleaned */
	gchar *fontname;	/* Cleaned */
	gchar *kfontname;	/* Cleaned */
	gchar *colorname;	/* Cleaned */
	gdouble saved_colors[4];

	GtkShadowType SHADOW_TYPE;
	GtkPositionType PULLOFFS_SIDE;

	GtkUIManager *uimanager;
	GtkWidget *popup_menu;	/* Cleaned */
	GdkCursor *cursor;
	GtkStyle *style;

	/* Style referring to the main output text area */
	GtkStyle *textstyle;	/* Cleaned */

	/* The default GTK style */
	GtkStyle *deflt;	/* Cleaned */

	GdkFont *font;		/* Cleaned */

	/* The keyboard widget's style */
	GtkStyle *kstyle;	/* Cleaned */

	/* Keyboard font */
	GdkFont *kfont;		/* Cleaned */

	GtkTooltips *tooltips;
} GTKeyboardGUI;

/**************************************************************************/

typedef struct {
	int width;
	int height;
	int x;
	int y;
} window_dimensions;


/* file_manip.h
 * 
 * For use with GTKeyboard
 * written by David Allen s2mdalle@titan.vcu.edu
 * This file is released under the terms of the GNU General Public License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>


#define	MAX_LINE_LENGTH	              128
#define	TOKEN_SIZE	              50
#define DEFAULT_RCFILE  "# Default .gtkeyboardrc file written by GTKeyboard\nkeyboard main_keyboard on\nkeyboard number_pad on\nkeyboard cursor_keys on\nkeyboard f_keys on\n\nset handlebars off\n\nset word_wrap off\nset info_popups on\nset bump_amount 15\nhide buttonbar\nset random_strlen 10\nset eyecandy off\nset redirect_policy implicit\n"




#define STAT_AND_RETURN_IF_BAD(fname, someint, statstruct) \
                                   if(!fname)return(0);\
                                   someint = stat(fname, &statstruct);\
                                   if(someint<0) return(0);

/* List of strings - GTKeyboard is really looking for these when it
 * reads the rcfiles.  These are the options it recognizes.  To the user,
 * they aren't case sensitive, but here, they must ALL be specified in all caps
 */
#define OPT_KEYBOARD_FILE            "KEYBOARD_FILE"


/* Prototypes - creeping evil */
static void parse_user_resource_file(char *filename);
static void FILE_readline(FILE * fp, char *buffer, const int maxlen);
static int setup_default_rcfile(char *file);

/* EOF */

static const char *active_window_name = "CurrentWindow";

static Window root;
static XEvent xev;

/* fake_timestamp is static in irxevent.c */
static Window find_window(Window top, char *name);
static void gtkeyboard_XEvent_common_setup(XKeyEvent * xev);

/* Various keyboards are included only for app.c and build_keyboard.c
 * in master.h
 */

/* Just make them all different - gets passed to build_an_entire_keyboard */
/* Not used yet.. */

/* Local defines for what keys appear as */

/*
 * Check the notes at the bottom of the page for an ASCII drawing of how the
 * keyboard should look
 *
 * Each button is defined as a character string of two characters.  The first
 * is what you get when SHIFT || CAPS_LOCK > 0 and the second is the normal
 * key function.  This function loops through grabbing two characters at a
 * time and then calling makekeybutton to set things up and map the
 * callback to the key and so on.
 *
 * SPECIAL CASES WITHIN stuff[]:  (Yes it's braindamaged)
 * LL is the left cursor key
 * RR is the right cursor key
 * || is caps lock key
 * \\ (or escaped, it's \\\\) is SHIFT key
 * CC is control
 * AA is alt
 *
*/

/* PROTOTYPES - Most functions in build_keyboard.c are static and don't need
 * prototypes here.
 */
int init_keyboard_stuff(char *input);
static void keysym_callback(GtkWidget * emitter, gpointer data);
static void capslock_toggle(GtkWidget * w, gpointer data);
static void alt_toggle(GtkWidget * w, gpointer data);
static void alt_gr_toggle(GtkWidget * w, gpointer data);
static void control_toggle(GtkWidget * w, gpointer data);
static void shift_on(GtkWidget * w, gpointer data);

typedef struct {
	GtkStyle *style;
	int mask;
} GTKeyboardStyle;

typedef struct {
	GtkWidget *dialog;
	GdkColor *color;
} color_info;

typedef struct {
	long code;
	char *name;
} binding;


/* Main program options - do not declare as static - all global */
static GTKeyboardOptions options;
static GTKeyboardGUI GUI;
static GTKeyboardElements ELEMENTS;


#define NO_HANDLER                          666

/* Static function prototypes */
static void setup_default_opts(void);
static void setup_default_opts_other(void);

#define INSTALL_DOC_DIR "/usr/local/share/gtkeyboard"

int init_keyboard_stuff(char *input)
{
	char *filename;
	char *homeptr;

	/* This sets the values in the structure GUI to default values.
	 * I'll add things for changing the default in that function later.
	 */
	setup_default_opts();	/* Handles the stuff in options */
	setup_default_opts_other();

	/* Allocate the necessary memory to copy location of support files
	 * into */
	options.extrafiles = g_new0_(char, (strlen(INSTALL_DOC_DIR) + 3));
	sprintf(options.extrafiles, "%s%s", INSTALL_DOC_DIR, _DIR);

	options.keyboard = NO_KEYBOARD;
	options.old_keyboard = NO_KEYBOARD;

	/* End if */
	homeptr = getenv("HOME");

	if (homeptr) {
		options.home = g_strdup_(getenv("HOME"));
	} /* End if */
	else
		options.home = NULL;

	filename =
	    g_new0_(char, strlen(options.home) + 5 + strlen(RC_FILENAME));
	sprintf(filename, "%s%s%s", (options.home ? options.home : "/tmp"),
		_DIR, RC_FILENAME);

	/* This parses ALL user preferences from ~/.gtkeyboardrc into various
	 * structures - the important parts of this are in rc_file.h and
	 * file_manip.c
	 */

	parse_user_resource_file(filename);

	CONDFREE(filename);

	return (1);
}				/* End init_keyboard_stuff */

/* This gets called once at setup to initialize all the GUI but 
 * non-keyboard related opts
 * Shouldn't return anything or allocate any memory, it just sets
 * things to "reasonable" defaults.
 */
static void setup_default_opts_other(void)
{
	/* Here's where you get to set all of the defaults for GUI structure
	 * members.
	 */
	GUI.SHADOW_TYPE = GTK_SHADOW_ETCHED_OUT;
	GUI.PULLOFFS_SIDE = GTK_POS_LEFT;
	GUI.cursor = (GdkCursor *) NULL;
	GUI.fontname = (gchar *) NULL;
	GUI.kfontname = (gchar *) NULL;
	GUI.colorname = (gchar *) NULL;
	GUI.textstyle = (GtkStyle *) NULL;
	GUI.style = (GtkStyle *) NULL;
	GUI.font = (GdkFont *) NULL;
	GUI.kfont = (GdkFont *) NULL;
	GUI.deflt = (GtkStyle *) NULL;
	GUI.xwindow = (Window) NULL;
	GUI.popup_menu = (GtkWidget *) NULL;
	GUI.uimanager = (GtkUIManager *) NULL;

	GUI.keyboard_elements.show_keyboard = ON;

	GUI.keyboard_elements.keyboard = (GtkWidget *) NULL;

	return;
}				/* End setup_default_opts_other */

/* This gets called once at startup, sets all the initial values for
 * options.whatever stuff
 */
static void setup_default_opts(void)
{
	/* All screen ELEMENTS on by default */

	ELEMENTS.keyboard = 1;

	/* Set up some default values for program parameters
	 * Can be changed by parse_rcfile later.  Hopefully these are reasonable.
	 */
	options.other = (Window) NULL;
	options.redirect_window = (Window) NULL;
	options.home = (char *) NULL;
	options.redirect_window_name = (char *) NULL;
	options.tempslot = (char *) NULL;
	options.keyboard_file = (char *) NULL;


	/* options.ASK_REMAP_ON_EXIT     = OFF; *//* Currently deprecated      */
	options.STATUS_LOGGING = OFF;	/* Save the status window    */
	options.CAPS_LOCK = OFF;	/* Caps lock starts at OFF   */
	options.SHIFT = OFF;	/* Shift starts at OFF       */
	options.ALT_GR = OFF;	/* Alt-GR key status         */
	options.CONTROL = OFF;	/* Control button at OFF     */
	options.NUMLOCK = OFF;	/* Numlock starts at OFF     */
	options.ALT = OFF;	/* Alt key starts at OFF     */
	options.VERBOSE = OFF;	/* Spew messages             */
	options.REDIRECT_POLICY_IMPLICIT = ON;	/* Default to implicit redir */
	options.IGNORE_LAYOUT_FILE = OFF;	/* Dont ignore  layout file  */

}				/* End setup_default_opts */



#define BUILD_KEYBOARD_C

static char *string_special_cases(KeySym input);

static void rotate_keyboard_definitions(KEYBOARD * new_keyboard)
{
	if (!new_keyboard) {
		fprintf(stderr,
			"rotate_keyboard_definitions Error: Bad keyboard.\n");
		fflush(stderr);
		return;
	}

	/* End if */
	/* If there is an old definition laying around, destroy it to free
	 * up the memory.
	 */
	if (options.old_keyboard) {
		options.old_keyboard =
		    gtkeyboard_destroy_keyboard(options.old_keyboard);
#ifdef REMAPPING_DEBUGGING
		fprintf(Q, "Rotated old keyboard out...");
		fflush(Q);
#endif				/* REMAPPING_DEBUGGING */
	}

	/* End if */
	/* Copy the current keyboard definition to the 'old' keyboard */
	options.old_keyboard = options.keyboard;

	/* Make the argument to this function the 'current' keyboard */
	options.keyboard = new_keyboard;

#ifdef REMAPPING_DEBUGGING
	fprintf(Q, "Old keyboard is now %s, Current keyboard is now %s\n",
		(options.old_keyboard ? "valid" : "dead"),
		(options.keyboard ? "valid" : "dead"));
	fflush(Q);
#endif				/* REMAPPING_DEBUGGING */

	return;
}				/* End rotate_keyboard_definitions() */

/* This destroys the key attached as object data to a lot of the buttons
 * when they get destroyed
 */
static void destroy_key_widget_data(GtkObject *object, gpointer data)
{
	KEY *key = (KEY *) data;

	if (!key) {
		fprintf(stderr,
			"GTKeyboard error (destroy_key_widget_data) ");
		fprintf(stderr, "data == NULL!\n");
		fflush(stderr);
		return;
	}
	/* End if */
	key = gtkeyboard_destroy_key(key);
}				/* End destroy_key_widget_data() */

/* Call this for every widget that has a KEY structure as its object data
 * this makes sure that when the widget is destroyed, destroy_key_widget_data
 * gets called on the object data
 */
static void connect_destroy_signal(GtkWidget * widget, gpointer data)
{
#if 0
	g_signal_connect_data(G_OBJECT(widget),
				"destroy",
				G_CALLBACK(destroy_key_widget_data),
				(GtkCallbackMarshal)
				gtk_signal_default_marshaller, data,
				(GtkDestroyNotify) destroy_key_widget_data,
				FALSE, TRUE);
#endif
	g_signal_connect(G_OBJECT(widget), 
				"destroy", 
				G_CALLBACK(destroy_key_widget_data),
				data);
}				/* End connect_destroy_signal() */

static gint triple_callback(GtkWidget * emitter, GdkEvent * event,
			    gpointer data)
{
	KEY *k = (KEY *) data;
	KEY *key = NO_KEY;

	if (!k) {
		fprintf(stderr,
			"GTKeyboard internal error: %s: NULL \"KEY\" arg.\n",
			"(triple_callback)");
		fflush(stderr);
		return TRUE;
	}
	/* End if */
	if (event->type == GDK_BUTTON_PRESS) {
		key = gtkeyboard_key_copy(k);

		if (event->button.button == LEFT_MOUSE_BUTTON) {
			/* Regular keypress, deal with it as normal */
			keysym_callback((GtkWidget *) NULL,
					(gpointer) key);
			key = gtkeyboard_destroy_key(key);
			return TRUE;
		} /* End if */
		else if (event->button.button == MIDDLE_MOUSE_BUTTON) {
			KeySym lower, upper;

			/* Always generate the "Alt-GR" keysym */
			if (!key->alt_gr || key->alt_gr == NoSymbol) {
				key->alt_gr = key->lower_case;
			}	/* End if */
			key->lower_case = key->upper_case = key->alt_gr;

			/* Not sure whether this is upper case or lower case.  Try to
			 * find out by seeing if what XConvertCase returns is the same
			 * or different.
			 */
			XConvertCase(key->alt_gr, &lower, &upper);

			/* If upper is the same as alt_gr, then we need shift to be
			 * on.  Otherwise leave it however it is
			 */
			if (key->alt_gr == upper)
				options.SHIFT = ON;

			keysym_callback((GtkWidget *) NULL,
					(gpointer) key);
			/* Free the memory */
			key = gtkeyboard_destroy_key(key);
			return TRUE;
		} /* End else if */
		else if (event->button.button == RIGHT_MOUSE_BUTTON) {
			/* Always generate the "uppercase" Keysym */
			key->lower_case = key->alt_gr = key->upper_case;

			options.SHIFT = ON;

			keysym_callback((GtkWidget *) NULL,
					(gpointer) key);
			key = gtkeyboard_destroy_key(key);
			return TRUE;
		} /* End if */
		else {
			key = gtkeyboard_destroy_key(key);
			return FALSE;
		}		/* End else */
	}

	/* End if */
	/* Tell calling code that we have not handled this event; pass it on. */
	return FALSE;
}				/* End triple_callback() */

static void keysym_callback(GtkWidget * emitter, gpointer data)
{
	KEY *key = (KEY *) data;
	KeySym sym;
	char *keyname = (char *) NULL;
	char *altkeyname = (char *) NULL;
	KeySym lower = (KeySym) NULL, upper = (KeySym) NULL;

#ifdef PARANOID_DEBUGGING
	fprintf(Q, "keysym_callback():  Got (%s, %s, %s).\n",
		XKeysymToString(key->lower_case),
		XKeysymToString(key->upper_case),
		XKeysymToString(key->alt_gr));
	fflush(Q);
#endif				/* PARANOID_DEBUGGING */

	/* Determine which of the syms in the KEY * structure to use. */
	keyname = XKeysymToString(key->lower_case);
	altkeyname = XKeysymToString(key->alt_gr);

	if (options.ALT_GR) {
		sym = key->alt_gr;
		/* We have only three symbols, and we have to generate
		 * the fourth one for cyrillic letters.
		 */
		if (strstr(altkeyname, "Cyrillic_")) {
			XConvertCase(sym, &lower, &upper);

			if (options.SHIFT || options.CAPS_LOCK) {
				sym = upper;
			}	/* End if */
		}		/* End if */
	} /* End if */
	else if (strstr(keyname, "KP")) {
		if (options.NUMLOCK)
			sym = key->upper_case;
		else
			sym = key->lower_case;
	} /* End else if */
	else if (options.SHIFT)
		sym = key->upper_case;
	else if (options.CAPS_LOCK) {
		if (isalpha((char) key->upper_case))
			sym = key->upper_case;
		else
			sym = key->lower_case;
	} /* End else if */
	else
		sym = key->lower_case;

	if (options.redirect_window_name) {
		send_redirect_a_keysym(sym);

	}
	/* End if */
	return;
}				/* End keysym_callback() */

static int isspecial(KeySym input)
{
	char *ptr = XKeysymToString(input);

	if (input == NoSymbol || !ptr)
		return (1);
	if (strstr(ptr, "_L") || strstr(ptr, "_R"))
		return (1);
	if (strstr(ptr, "ontrol") || strstr(ptr, "Alt"))
		return (1);
	if (strstr(ptr, "ode"))
		return (1);
	if (strstr(ptr, "Tab") || strstr(ptr, "Lock")
	    || strstr(ptr, "pace"))
		return (1);

	return (0);
}				/* End isspecial */

GtkWidget *build_keyboard(GtkWidget * input, char *filename)
{
	/* NEW BUILD_KEYBOARD() */
	GtkWidget *mainbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox = (GtkWidget *) NULL;
	GtkWidget *button = (GtkWidget *) NULL;
	char *name = (char *) NULL;
	char *altname = (char *) NULL;
	GtkWidget *align = (GtkWidget *) NULL;
	char label[512];
	char tooltip_label[1024];
	char *errbuf = NULL;
	char *utf8, *ptr;
	int rowno;
	int index;
	char letter = '\0';
	char cyrletter = '\0';
	KEY *key;
	KeySym s;
	KeySym altlower, altupper;

	/* Create the current keyboard in a new place. -- This takes care of
	 * destroying our old ones for us.
	 */
	rotate_keyboard_definitions(read_keyboard_template(filename));

	if (!options.keyboard) {
		fprintf(stderr, "Couldn't read keyboard:  Bummer.\n");
		fflush(stderr);

		errbuf = g_new0(char, strlen(filename) + 100);
		sprintf(errbuf,
			"Couldn't create keyboard from file:\n%s!\nCheck the file format!",
			filename);

		button = gtk_button_new_with_label(errbuf);

		CONDFREE(errbuf);

		return (button);
	} /* End if */
	else if (options.keyboard->keycount <= 0) {
		errbuf = g_new0(char, strlen(filename) + 100);
		sprintf(errbuf,
			"Couldn't create keyboard from file:\n%s!\nCheck the file format!",
			filename);

		button = gtk_button_new_with_label(errbuf);
		CONDFREE(errbuf);
		return (button);
	}
	/* End else if */
	for (rowno = 0; rowno < MAXIMUM_ROWS; rowno++) {
		hbox = gtk_hbox_new(FALSE, 0);
		align = gtk_alignment_new(0.5, 0.5, 0, 0);

		for (index = 0;
		     index < options.keyboard->row_values[rowno];
		     index++) {
			key =
			    gtkeyboard_keyboard_get_key(options.keyboard,
							rowno, index);

			letter = (int) key->upper_case;
			name = XKeysymToString(key->upper_case);
			altname = XKeysymToString(key->alt_gr);

			if (key->upper_case == XK_Mode_switch ||
			    key->lower_case == XK_Mode_switch) {
				sprintf(label, " Alt Gr");
			} /* End if */
			else if (strstr(altname, "Cyrillic_")) {
				/* We have only lower case letter, let us
				 * ask X to convert it to upper case.
				 */
				XConvertCase(key->alt_gr, &altlower,
					     &altupper);

				/* FIXME: Yes, this is totally wrong method to get
				 * the cyrillic letter. It just happen to to
				 * yield the right letter in koi8-r encoding.
				 */
				cyrletter = (char) altupper;
				if (!isalpha(letter)) {
					sprintf(label, " %c   \n %c  %c",
						(char) key->upper_case,
						(char) key->lower_case,
						cyrletter);
				} /* End if */
				else {
					sprintf(label, " %c   \n    %c",
						(char) key->upper_case,
						cyrletter);
				}	/* End else */
			} /* End else if */
			else if ((isalnum(letter) || ispunct(letter))
				 && (letter > 0)) {
				if (!isalpha(letter))
					sprintf(label, "  %c  \n  %c",
						(char) key->upper_case,
						(char) key->lower_case);
				else
					sprintf(label, "  %c  \n",
						(char) key->upper_case);
			} /* End if */
			else if (letter != 0) {
				if (!iscntrl(letter)
				    && !isspecial(key->upper_case)
				    && letter != ' ')
					sprintf(label, "  %c  \n  %c",
						(char) key->upper_case,
						(char) key->lower_case);
				else {
					ptr =
					    string_special_cases(key->
								 lower_case);
					strncpy(label, ptr, 512);
					g_free_(ptr);
				}	/* End else */
			} /* End else if */
			else {
				ptr =
				    string_special_cases(key->lower_case);
				strncpy(label, ptr, 512);
				g_free_(ptr);
			}	/* End else */

			s = key->lower_case;

#if 0
			utf8 =
			    g_locale_to_utf8(label, -1, NULL, NULL, NULL);
#else
			utf8=g_convert(label,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
#endif
			/* Make the correct key, and attach the correct signal
			 * function to it.  Toggle/normal button/function
			 */
			if (s == XK_Caps_Lock || s == XK_Control_L ||
			    s == XK_Control_R || s == XK_Alt_L ||
			    s == XK_Alt_R || s == XK_Mode_switch)
				button =
				    gtk_toggle_button_new_with_label(utf8);
			else
				button = gtk_button_new_with_label(utf8);

			g_free(utf8);
			GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
			if (key->code != 0)
				sprintf(tooltip_label,
					"KeyCode %d:\n%s\n%s\n%s",
					key->code,
					XKeysymToString(key->lower_case),
					XKeysymToString(key->upper_case),
					XKeysymToString(key->alt_gr));
			else
				sprintf(tooltip_label,
					"KeyCode unknown:\n%s\n%s\n%s",
					XKeysymToString(key->lower_case),
					XKeysymToString(key->upper_case),
					XKeysymToString(key->alt_gr));

			switch (key->lower_case) {
			case XK_Caps_Lock:
				g_signal_connect(G_OBJECT(button),
						   "clicked",
						   G_CALLBACK(capslock_toggle),
						   input);
				/* Key unused in signalling */
				key = gtkeyboard_destroy_key(key);
				break;
			case XK_Alt_L:
			case XK_Alt_R:
				g_signal_connect(G_OBJECT(button),
						   "clicked",
						   G_CALLBACK(alt_toggle), NULL);
				/* Key unused in signalling */
				key = gtkeyboard_destroy_key(key);
				break;
			case XK_Control_L:
			case XK_Control_R:
				g_signal_connect(G_OBJECT(button),
						   "clicked",
						   G_CALLBACK(control_toggle), NULL);
				/* Key unused in signalling */
				key = gtkeyboard_destroy_key(key);
				break;
			case XK_Shift_L:
			case XK_Shift_R:
				g_signal_connect(G_OBJECT(button),
						   "clicked",
						   G_CALLBACK(shift_on), NULL);
				/* Key unused in signalling */
				key = gtkeyboard_destroy_key(key);
				break;
			case XK_Mode_switch:
				g_signal_connect(G_OBJECT(button),
						   "clicked",
						   G_CALLBACK(alt_gr_toggle), NULL);
				/* Key unused in signalling */
				key = gtkeyboard_destroy_key(key);
				break;
			default:
				g_signal_connect(G_OBJECT(button),
						   "button_press_event",
						   G_CALLBACK(triple_callback), key);
				connect_destroy_signal(button, key);
				break;
			}	/* End switch */

			gtk_box_pack_start(GTK_BOX(hbox), button, FALSE,
					   FALSE, 0);
		}		/* End for */

		gtk_container_add(GTK_CONTAINER(align), hbox);
		gtk_box_pack_start(GTK_BOX(mainbox), align, FALSE, FALSE,
				   0);
	}			/* End for */

	if (filename) {
		if (options.keyboard_file) {
			/* We just built a complete keyboard with this file, so save its
			 * name for future use.
			 */
			/* This weird indirect freeing and copying of the string is
			 * due to the fact that the filename argument to this function
			 * may in fact be options.keyboard_file itself, so in that
			 * case it wouldn't be that bright to try to free it and 
			 * copy something that's pointing to the same location.  So
			 * instead we copy it to an intermediate spot, free the 
			 * original, and copy the new value back.
			 *
			 * When the value actually is options.keyboard_file, we do a 
			 * bit of redundant work, but oh well.
			 */
			char *tptr;
			tptr = g_strdup_(filename);
			g_free_(options.keyboard_file);
			options.keyboard_file = g_strdup_(tptr);
			g_free_(tptr);
#if 0
			fprintf(Q,
				"options.keyboard_file set to be \"%s\"\n",
				options.keyboard_file);
			fflush(Q);
#endif
		} /* End if */
		else {
			/* No need to free it - just copy */
			options.keyboard_file = g_strdup_(filename);
			fprintf(Q,
				"options.keyboard_file set to be \"%s\"\n",
				options.keyboard_file);
			fflush(Q);
		}		/* End else */
	}

	/* End if */
	/* gtk_widget_show_all(mainbox); */
	return (mainbox);
}				/* End build_keyboard() */

static char *string_special_cases(KeySym input)
{
	char label[1024];
	int len, x;
	char *ptr;

	if (input == XK_space) {
		sprintf(label,
			"                                               \n");
	} /* End if */
	else {
		/* Space out the output a bit depending on string target
		 * length so the buttons will look right.
		 */
		ptr = XKeysymToString(input);
		if (strlen(ptr) > 4)
			sprintf(label, " %s \n", ptr);
		else
			sprintf(label, "  %s  \n", ptr);
	}			/* End else */

	len = strlen(label);

	/* Special cases */
	if (strstr(label, "Control") || strstr(label, "control")) {
		strcpy(label, " Ctrl ");
	} /* End if */
	else if (input == XK_Mode_switch)
		strcpy(label, " Alt Gr");
	else {
		/* Remove the sillyness from XK_ names from the string. */
		for (x = 0; x < len; x++) {
			/* Make everything uppercase */
			label[x] = toupper(label[x]);

			/* Get rid of the _R or _L that may be there. */
			if (label[x] == '_' &&
			    (label[x + 1] == 'R' || label[x + 1] == 'L')) {
				label[x] = '\0';
			}	/* End if */
		}		/* End for */
	}			/* End else */

	ptr = g_strdup_(label);

	return (ptr);
}				/* End string_special_cases() */


#define MODE_WRITE

/* Do not allow both MODE_WRITE and MODE_APPEND to be defined.  If both are
 * defined, default to MODE_WRITE.  The End Of Earth As We Know It (TM) occurs
 * (or maybe just a compile error) MODE_WRITE is spiffier.
 */
#ifdef MODE_APPEND
#  ifdef MODE_WRITE
#    undef MODE_APPEND
#  endif			/* MODE_WRITE */
#endif				/* MODE_APPEND */

#ifndef __USE_GNU
#  define __USE_GNU
#endif				/* __USE_GNU */


static void alt_gr_toggle(GtkWidget * w, gpointer data)
{
	if (ISOFF(options.ALT_GR)) {
		options.ALT_GR = ON;
	} /* End if */
	else {
		options.ALT_GR = OFF;
	}			/* End else */
}				/* End alt_gr_toggle */

static void control_toggle(GtkWidget * w, gpointer data)
{
	if (ISOFF(options.CONTROL)) {
		options.CONTROL = ON;
	} /* End if */
	else {
		options.CONTROL = OFF;
	}			/* End else */
}				/* End control_toggle */

static void alt_toggle(GtkWidget * w, gpointer data)
{
	if (ISOFF(options.ALT)) {
		options.ALT = ON;
	} /* End if */
	else {
		options.ALT = OFF;
	}			/* End else */
}				/* End alt_toggle */

static void capslock_toggle(GtkWidget * w, gpointer data)
{
	if (options.redirect_window_name)
		send_redirect_a_keysym(XK_Caps_Lock);

	/* Whatever it currently is, swtich it */
	if (ISOFF(options.CAPS_LOCK)) {
		options.CAPS_LOCK = ON;
	} /* End if */
	else {
		options.CAPS_LOCK = OFF;
	}			/* End else */
}				/* End capslock_toggle */

static void shift_on(GtkWidget * w, gpointer data)
{
	/* Turn shift on */
	options.SHIFT = ON;

}				/* End shift_on */

/* This parses the user resource file via read_ConfigFile.  This function
 * doesn't actually do much other than define all of the structures for
 * parsing in one place and actually doing the function calls.  This is
 * where you need to tweak command options.
 */
static void parse_user_resource_file(char *filename)
{
	RCVARS rc_parse_values[] = {
		{"set", OPT_KEYBOARD_FILE, RC_STR, &options.keyboard_file,
		 NULL},
		{NULL, NULL, RC_NONE, NULL, NULL}
	};			/* End rc_parse_values */

	if (!file_exists(filename)) {
		fprintf(stderr,
			"Your resource file doesn't seem to exist.\n");
		fprintf(stderr, "I'll create one for you at \"%s\"\n",
			filename);
		fflush(stderr);
		setup_default_rcfile(filename);
	}
	/* End if */
	read_ConfigFile(filename, rc_parse_values, 1);
}				/* End parse_user_resource_file() */

/* Read one line from the specified file pointer and return it as
 * as a pointer to char and so on.
 */
static void FILE_readline(FILE * fp, char *buffer, const int maxlen)
{
	int index = 0;
	int x;
	char tmp;

	do {
		x = fread(&tmp, sizeof(char), 1, fp);
		if ((x == 0) || (tmp == '\n'))
			buffer[index] = '\0';	/* Terminate the string with a NULL */
		else
			buffer[index++] = tmp;	/*  Add the character */
		if (!(index < maxlen)) {
			fprintf(Q,
				"Error on FILE_readline: index >= maxlen\n");
			fflush(Q);
			return;
		}		/* End if */
	} while ((x != 0) && (tmp != '\n'));
	return;
}				/* End FILE_readline */

/* Copies a default rcfile from somewhere to the input filename */
static int setup_default_rcfile(char *file)
{
	FILE *fp;
	FILE *dflt;
	char buffer[1024];
	char buffer2[1024];

	if ((fp = fopen(file, "w")) == NULL) {
		fprintf(Q, "Couldn't open %s for writing - cannot create ",
			file);
		fprintf(Q, "default .gtkeyboardrc!\n");
		fflush(Q);
		return (0);
	} /* End if */
	else {
		/* Try to open the distribution-provided rcfile first */
		sprintf(buffer, "%s/%s", options.extrafiles,
			PROVIDED_RCFILE);

		if ((dflt = fopen(buffer, "r")) == NULL) {
			/* Ok, fine, so we can't open the default one we provided 
			 * with the distribution.  We'll just give the user a really
			 * short and crappy one.
			 */
			fprintf(Q, "Couldn't open %s: %s.\n", buffer,
				g_strerror(errno));
			fprintf(Q,
				"Fine then!  We'll try something else...\n");
			fflush(Q);

			fprintf(fp, "%s", DEFAULT_RCFILE);
			fclose(fp);
			fprintf(Q,
				"Success writing default .gtkeyboardrc\n");
			fprintf(Q,
				"You can edit it to make changes later by editing ");
			fprintf(Q, "%s\n", file);
			fflush(Q);
			return (1);
		} /* End if */
		else {
			while (!feof(dflt)) {
				FILE_readline(dflt, buffer2, 1024);
				/* Add a newline because FILE_readline kills them off */
				fprintf(fp, "%s\n", buffer2);
			}	/* End while */

			fflush(fp);
			fclose(dflt);
			fclose(fp);

			fprintf(Q,
				"Successfully wrote .gtkeyboardrc from ");
			fprintf(Q,
				"default.  (%s: Distribution provided)\n",
				PROVIDED_RCFILE);
		}		/* End else */
	}			/* End else */
	FLUSH_EVERYTHING;
	return (1);
}				/* End setup_default_rcfile */

static int file_exists(const char *filename)
{
	struct stat file_info;
	int x;

	STAT_AND_RETURN_IF_BAD(filename, x, file_info);

	return (1);
}				/* End file_exists */

/* seems that xfree86 computes the timestamps for events like this 
 * strange but it relies on the *1000-32bit-wrap-around     
 * if anybody knows exactly how to do it, please contact me 
 * returns: a timestamp for the 
 * constructed event 
 */
static Time fake_timestamp(void)
{
	int tint;
	struct timeval tv;
	struct timezone tz;	/* is not used since ages */
	gettimeofday(&tv, &tz);
	tint = (int) tv.tv_sec * 1000;
	tint = tint / 1000 * 1000;
	tint = tint + tv.tv_usec / 1000;
	return ((Time) tint);
}				/* End fake_timestamp() */

/* in: id of the root window name of the target window 
 * returns: id of the target window 
 * called by: main 
 */
static Window find_window(Window top, char *name)
{
	char *wname, *iname;
	XClassHint xch;
	Window *children, foo;
	int revert_to_return;
	unsigned int nc;

	if (!strcmp(active_window_name, name)) {
		XGetInputFocus(GDK_DISPLAY(), &foo, &revert_to_return);
		return (foo);
	}

	/* End if */
	/* First the base case */
	if (XFetchName(GDK_DISPLAY(), top, &wname)) {
		if (!strncmp(wname, name, strlen(name))) {
			XFree(wname);
			return (top);	/* found it! */
		}
		/* End if */
		XFree(wname);
	}
	/* End if */
	if (XGetIconName(GDK_DISPLAY(), top, &iname)) {
		if (!strncmp(iname, name, strlen(name))) {
			XFree(iname);
			return (top);	/* found it! */
		}		/* End if */
		XFree(iname);
	}
	/* End if */
	if (XGetClassHint(GDK_DISPLAY(), top, &xch)) {
		if (!strcmp(xch.res_class, name)) {
			XFree(xch.res_name);
			XFree(xch.res_class);
			return (top);	/* found it! */
		}		/* End if */
		XFree(xch.res_name);
		XFree(xch.res_class);
	}
	/* End if */
	if (!XQueryTree(GDK_DISPLAY(), top, &foo, &foo, &children, &nc) ||
	    children == NULL)
		return (0);	/* no more windows here */

	/* check all the sub windows */
	for (; nc > 0; nc--) {
		top = find_window(children[nc - 1], name);
		if (top)
			break;	/* we found it somewhere */
	}			/* End for */

	/* Free that mem!  Yeehaw!!! */
	if (children)
		XFree(children);

	return (top);
}				/* End find_window() */

/* This just assigns a bunch of things to certain elements of the pointer 
 * that are shared no matter what type of signal GTKeyboard is sending. 
 * Prevents code duplication in ugly places
 */
static void gtkeyboard_XEvent_common_setup(XKeyEvent * xev)
{
	xev->type = KeyPress;
	xev->display = GDK_DISPLAY();
	xev->root = root;
	xev->subwindow = None;
	xev->time = fake_timestamp();
	xev->same_screen = True;
	xev->state = 0;
	xev->x = xev->y = xev->x_root = xev->y_root = 1;
}				/* End gtkeyboard_XEvent_common_setup */

static int assign_keycode_from_keysym(KeySym foo, XKeyEvent * xev)
{
	unsigned long mask = 0;
	char *keyname = (char *) NULL;

	keyname = XKeysymToString(foo);
	xev->keycode = XKeysymToKeycode(GDK_DISPLAY(), foo);

	/* Check and assign masks. */
	if (options.SHIFT) {	/* Need ShiftMask? */
		mask = find_modifier_mask(XKeysymToKeycode(GDK_DISPLAY(),
							   XK_Shift_L));
		if (!mask) {
			/* WTF?  Shift_L isn't mapped?  OK, try Shift_R */
			mask =
			    find_modifier_mask(XKeysymToKeycode
					       (GDK_DISPLAY(),
						XK_Shift_R));
		}
		/* End if */
		fprintf(Q, "Shift mask: 0x%lx normal mask 0x%lx\n", mask,
			(unsigned long) ShiftMask);

		/* Even if mask is actually 0 because we couldn't find the shift
		 * key mapped, this just won't do anything at all.  
		 * find_modifier_mask issued a warning about not finding it, too
		 */
		xev->state |= mask;
		options.SHIFT = 0;
	}
	/* End if */
	if (options.CAPS_LOCK) {	/* Need LockMask? */
		mask = find_modifier_mask(XKeysymToKeycode(GDK_DISPLAY(),
							   XK_Caps_Lock));
		fprintf(Q, "Capslock mask: 0x%lx normal mask 0x%lx\n",
			mask, (unsigned long) LockMask);
		xev->state |= mask;	/* Normally LockMask */
	}
	/* End if */
	if (options.CONTROL) {	/* Need ControlMask? */
		mask = find_modifier_mask(XKeysymToKeycode(GDK_DISPLAY(),
							   XK_Control_L));
		if (!mask) {
			mask =
			    find_modifier_mask(XKeysymToKeycode
					       (GDK_DISPLAY(),
						XK_Control_R));
		}
		/* End if */
		fprintf(Q, "Control mask: 0x%lx normal mask 0x%lx\n",
			mask, (unsigned long) ControlMask);
		xev->state |= mask;
	}
	/* End if */
	if (options.ALT) {	/* Need Mod1Mask? */
		mask = find_modifier_mask(XKeysymToKeycode(GDK_DISPLAY(),
							   XK_Alt_L));
		if (!mask) {
			mask =
			    find_modifier_mask(XKeysymToKeycode
					       (GDK_DISPLAY(), XK_Alt_R));
		}
		/* End if */
		fprintf(Q, "Alt mask: 0x%lx normal mask 0x%lx\n",
			mask, (unsigned long) Mod1Mask);
		xev->state |= mask;
	}
	/* End if */
	if (options.NUMLOCK) {
		mask = find_modifier_mask(XKeysymToKeycode(GDK_DISPLAY(),
							   XK_Num_Lock));
		fprintf(Q, "Numlock: Mask 0x%lx\n", mask);
		xev->state |= mask;
	}
	/* End if */
	if (options.ALT_GR) {
		mask = find_modifier_mask(XKeysymToKeycode(GDK_DISPLAY(),
							   XK_Mode_switch));
		fprintf(Q, "Alt_GR: Mask 0x%lx\n", mask);
		xev->state |= mask;
	}
	/* End if */
	if (strstr(keyname, "Cyrillic_")) {	/* Cyrillic? */
		mask = find_modifier_mask(XKeysymToKeycode(GDK_DISPLAY(),
							   XK_ISO_Group_Shift));
		/* FIXME: How do we get this mask and what does it mean?
		 * Seems to be XKB_Group.  Default 0x2000
		 */

		xev->state |= mask;
	}


	/* End if */
	/* xev->state = xev->state | ButtonMotionMask; */
#if 0
	fprintf(Q, "Final mask on event: %ld 0x%lx\n",
		(unsigned long) xev->state, (unsigned long) xev->state);
#endif

	if (xev->keycode != 0)
		return 1;

	else
		return 0;
}				/* End assign_keycode_from_keysym() */

static void keysym_sendkey(KeySym somesym, Window w)
{
	gtkeyboard_XEvent_common_setup((XKeyEvent *) & xev);

	/* assign_keycode_from_keysym() will also add in the needed
	 * masks.  WARNING:  This may change options.SHIFT and other 
	 * bitflags in options according to whether or not they should
	 * change. 
	 */
	if (!assign_keycode_from_keysym(somesym, (XKeyEvent *) & xev)) {
		return;
	}
	/* End if */
	xev.xkey.window = w;

	/* This may produce a BadWindow error with Xlib.  Bummer.
	 * This happens most commonly when the window that was selected to
	 * redirect to doesn't exist on screen anymore.
	 */
	gdk_error_trap_push();	/* Catch errors, hopefully */

	XSendEvent(GDK_DISPLAY(), w, True, KeyPressMask, &xev);

	XSync(GDK_DISPLAY(), False);

	xev.type = KeyRelease;	/* Start the next Event */
	/* usleep(50000);  */
#if 0
	XFlush(GDK_DISPLAY());
#endif
	xev.xkey.time = fake_timestamp();
	XSendEvent(GDK_DISPLAY(), w, True, KeyReleaseMask, &xev);
	XSync(GDK_DISPLAY(), False);

#if 0
	gdk_flush();
#endif

	if (gdk_error_trap_pop()) {
	}
	/* End if */
	return;
}				/* End keysym_sendkey() */

/* Insert KeyCode code into slot slot in the table structure.
 * Returns != 0 on success, 0 on failure.  Failure means that 
 * the table is full, or that the code you're trying to insert is 0
 */
static int ModmapTable_insert(ModmapTable * table, KeyCode code, int slot)
{
	int x = 0;

	if ((code == (KeyCode) 0) || (slot < 0) || (slot > 8))
		/* This operation makes no sense.  Return failure. */
		return 0;

	for (x = 0; x < table->max_keypermod; x++) {
		/* Insert in the first available open slot 
		 * but not in taken slots.  That would be a bad idea to 
		 * silently overwrite some of the caller's data.  :)
		 */
		if (table->modifiers[slot].codes[x] == 0) {
			table->modifiers[slot].codes[x] = code;
			return 1;
		}		/* End if */
	}			/* End for */

	/* Fail - can't find empty slot */
	return (0);
}				/* End ModmapTable_insert() */

static ModmapTable *ModmapTable_new(void)
{
	XModifierKeymap *map = XGetModifierMapping(GDK_DISPLAY());
	ModmapTable *table;
	int mkpm = map->max_keypermod;
	int x = 0;
	int y = 0;

	XFreeModifiermap(map);
	table = g_new0_(ModmapTable, 1);
	table->max_keypermod = mkpm;

	for (x = 0; x < 8; x++) {
		for (y = 0; y < 4; y++) {
			table->modifiers[x].codes[y] = (KeyCode) 0;
		}		/* End for */
	}			/* End for */

	return table;
}				/* End ModmapTable_new() */

static void ModmapTable_destroy(ModmapTable * table)
{
	g_free_(table);
}				/* End ModmapTable_destroy() */

/* Translates a string mask name into a slot number for access to numerous
 * modmap data structures.
 */
static int mask_name_to_slot_number(char *maskname)
{
	char *masks[] = { "ShiftMask", "LockMask",
		"ControlMask", "Mod1Mask",
		"Mod2Mask", "Mod3Mask",
		"Mod4Mask", "Mod5Mask"
	};
	int maskcount = 8;
	int y = 0;

	for (y = 0; y < maskcount; y++) {
		if (g_ascii_strcasecmp(maskname, masks[y]) == 0)
			return y;
	}			/* End for */

	return (-1);
}				/* End mask_name_to_slot_number() */

static unsigned long find_modifier_mask(KeyCode code)
{
	XModifierKeymap *map = XGetModifierMapping(GDK_DISPLAY());
	int x = 0, y = 0;
	KeyCode c = (KeyCode) NULL;
	unsigned long mask = 0;

	if (code == (KeyCode) 0) {
		XFreeModifiermap(map);
		fprintf(Q,
			"Error finding modifier mask for 0 keycode:  Have you\n");
		fprintf(Q,
			"actually remapped your keyboard to this layout?\n");
		return 0;
	}
	/* End if */
	for (x = 0; x < 8; x++) {
		for (y = 0; y < map->max_keypermod; y++) {
			c = map->modifiermap[x * map->max_keypermod + y];
			if (c == code) {
				XFreeModifiermap(map);
				mask = slot_number_to_mask(x);
				fprintf(Q,
					"Found modifier %d in slot (%d,%d) mask %ld 0x%lx\n",
					code, x, y, mask, mask);
				return mask;
			}	/* End if */
		}		/* End for */
	}			/* End for */

	XFreeModifiermap(map);

	/* Return nothing.  This is bad, but better than doing the wrong thing. */
	fprintf(Q,
		"***** WARNING:  find_modifier_mask failed to locate code %d\n",
		code);
	fflush(Q);
	return 0;
}				/* End find_modifier_mask() */


/* Makes a NULL-terminated string that gets passed as input nothing but
 * upper case characters.
 * 
 * THIS FUNCTION MODIFIES ITS ARGUMENT
 */
static void str_toupper(char *string)
{
	int x = 0;

	while (string[x]) {	/* Stop on NULL */
		string[x] = toupper(string[x]);
		x++;
	}			/* End while */
}				/* End str_toupper() */

/* Passed - the filename, the set of variables to look for, (see the header
 * file for the definition of that structure) and an integer - if it's 0, 
 * then "syntax error" messages won't be printed.  Otherwise, we'll complain.
 */
static int read_ConfigFile(char *filename, RCVARS * vars, int complain)
{
	gint n = 0;
	gpointer N;
	FILE *rcFile;
	GHashTable *H;
	gchar Line[BUFSIZ], varPrefix[BUFSIZ], varName[BUFSIZ],
	    varVal[BUFSIZ];

	/* if the RC file doesn't exist, don't bother doing anything else */
	if ((rcFile = fopen(filename, "r")) == NULL) {
		if (complain) {
			fprintf(Q, "Couldn't open %s for reading: %s\n",
				filename, g_strerror(errno));
			fflush(Q);
		}		/* End if */
		return 1;
	}

	/* End if */
	/* Create a hash table of all the variable names and their arrayindex+1 */
	H = g_hash_table_new(g_str_hash, g_str_equal);

	n = 0;
	/* Hash in all of the variable names.  Later when we read them in,
	 * we'll hash in what we read to compare it against what was in the
	 * table passed to us.
	 */
	while (vars[n].Type != RC_NONE) {
		g_hash_table_insert(H, vars[n].Name,
				    GINT_TO_POINTER(n + 1));
		n++;
	}			/* End while */

	/* read each line of the RC file */
	while (fgets(Line, BUFSIZ, rcFile) != NULL) {
		/* Strip leading and trailing whitespace from the string */
		strcpy(Line, g_strstrip(Line));

		/* Skip comments and lines too short to have useful information
		 * in them.  This will include "blank" lines that got g_strstrip'd
		 * down to 0 or 1 bytes
		 */
		if ((strlen(Line) < 2) || Line[0] == '#')
			continue;

		/* Initialize values so in case of error they will be NULL, not
		 * the value of the last parse.
		 */
		varPrefix[0] = varName[0] = varVal[0] = '\0';

		/* grab each variable and its value (maybe). 
		 * If prefix is specified, it tries to read the
		 * given prefix.
		 */
		if (strstr(Line, "="))
			sscanf(Line, " %s = %s\n", varName, varVal);
		else
			sscanf(Line, " %s %s %s\n", varPrefix, varName,
			       varVal);

		/* Sometimes prefix *could* be null, but if name or value is
		 * null, there's really nothing we can do with that string.
		 */
		if (!varName[0] && !varVal[0]) {
			if (complain) {
				fprintf(stderr,
					"Error parsing line \"%s\": ",
					Line);
				fprintf(stderr,
					"I have no idea what that line means.\n");
				fflush(stderr);
			}
			/* End if */
			continue;
		}

		/* End if */
		/* We want the rc file to be case insensitive, but we're looking for
		 * all upper-case varaible names, so convert the string to all
		 * upper-case so it will hash in correctly.
		 */
		str_toupper(varName);

		/* use the hash table to find the correct array entry */
		if ((N = g_hash_table_lookup(H, varName)) == NULL) {
			continue;	/* but skip to the next line if can't find it. */
		}
		/* End if */
		n = GPOINTER_TO_INT(N) - 1;	/* convert back into an array index */

		/* We can't necessarily match the read prefix to the requested 
		 * prefix since the prefixes may be different and may require
		 * processing through the function pointer associated with the
		 * record
		 */

		/* 
		 * Did we see a prefix when we didn't want one?
		 */
		if (!vars[n].Prefix && varPrefix[0]) {
			fprintf(stderr,
				"Error:  Bad syntax.  I wasn't expecting to see ");
			fprintf(stderr, "a variable prefix on \"%s\".\n",
				(varName ? varName : Line));
			fprintf(stderr, "Ignoring line \"%s\"\n", Line);
			fflush(stderr);
			continue;
		}

		/* End if */
		/* Are we supposed to run this one through a function? */
		if (vars[n].Type == RC_PARSE_FUNC) {
			/* Use the outside function specified in the structure
			 * to parse these line elements since their grammar is 
			 * somewhat weird 
			 */
			if (!vars[n].
			    func(varPrefix, varName, varVal,
				 vars[n].Val)) {
				fprintf(stderr,
					"There was an error parsing \"%s\"\n",
					Line);
				fflush(stderr);
			}
			/* End if */
			continue;	/* Done with this line */
		}

		/* End if */
		/* We're not supposed to run this through a function --
		 * based on the variable's type, set the C variable to its saved
		 * value
		 */
		switch (vars[n].Type) {
		case (RC_STR):
			{
				char *tok;

				/* varVal is not trustworthy, find the string 
				 * within the quotes and use that instead. 
				 */
				if (strstr(Line, "\"")) {
					tok = strtok(Line, "\"");
					if (!tok) {
						/* This really shouldn't happen */
						if (complain) {
							fprintf(stderr,
								"Parse error within \"%s\"\n",
								Line);
							fflush(stderr);
						}	/* End if */
						break;
					}	/* End if */
					tok = strtok(NULL, "\"");
				} /* End if */
				else
					tok = &varVal[0];

				/* free the current contents of the variable */
				if (*(gchar **) (vars[n].Val))
					g_free_(*(gchar **) vars[n].Val);

				/* set the variable to its new value. */
				if (tok) {
					*(gchar **) (vars[n].Val) =
					    g_strdup_(tok);
				} /* End if */
				else
					*(gchar **) (vars[n].Val) =
					    (char *) NULL;
				break;
			}	/* End block */
		}		/* End switch */
	}			/* End while */

	/* clean up and exit */
	g_hash_table_destroy(H);
	fclose(rcFile);
	return 0;
}				/* End read_ConfigFile() */

static KEY *gtkeyboard_key_copy(KEY * key)
{
	KEY *newkey;

	if (!key)
		return (NO_KEY);

	newkey = gtkeyboard_new_key(key->lower_case,
				    key->upper_case,
				    key->alt_gr, key->aux_string);
	newkey->code = key->code;
	return (newkey);
}				/* End gtkeyboard_key_copy() */

/* Returns the "keyno"th key in row "row" of keyb 
 * This is allocated memory which must be g_free()'d later.
 * This will ALWAYS return allocated memory - it just might always be
 * filled with NoSymbol
 */
static KEY *gtkeyboard_keyboard_get_key(KEYBOARD * keyb, int row,
					int keyno)
{
	KEY *foobar;
	int index;
	int x, findex = 0;

	foobar = gtkeyboard_new_key(NoSymbol, NoSymbol, NoSymbol, NULL);

	if (row > MAXIMUM_ROWS) {
		fprintf(stderr,
			"gtkeyboard_keyboard_get_key:  Row out of range.\n");
		fflush(stderr);
		return (foobar);
	}
	/* End if */
	if (!keyb) {
		fprintf(stderr,
			"gtkeyboard_keyboard_get_key:  Null keyb.\n");
		fflush(stderr);
		return (foobar);
	}
	/* End if */
	x = findex = 0;

	while (x < row) {
		/* Add up the number of keys on all lines preceeding the one we're
		 * looking for
		 */
		findex += keyb->row_values[x];
		x++;
	}			/* End while */

	index = (findex * 3) + (keyno * 3);

	if (index > ((keyb->keycount * 3) - 3)) {
		fprintf(stderr, "gtkeyboard_keyboard_get_key():  ");
		fprintf(stderr,
			"Illegal index %d of a total keycount of %d (%d).\n",
			index, keyb->keycount, ((keyb->keycount * 3) - 3));
		fflush(stderr);
		return (foobar);
	}

	/* End if */
	/* Three consecutive KeySyms */
	foobar->lower_case = keyb->syms[index];
	foobar->upper_case = keyb->syms[(index + 1)];
	foobar->alt_gr = keyb->syms[(index + 2)];
	foobar->aux_string = (char *) NULL;	/* No auxilliary */

	if (keyb->codes)
		foobar->code = keyb->codes[(findex + keyno)];
	else
		foobar->code = 0;

	return (foobar);
}				/* End gtkeyboard_keyboard_get_key() */

static KEY *gtkeyboard_new_key(const KeySym lower, const KeySym upper,
			       const KeySym altgr, const char *alt)
{
	KEY *somekey = g_new0_(KEY, 1);
	somekey->lower_case = lower;
	somekey->upper_case = upper;
	somekey->alt_gr = altgr;
	somekey->code = 0;
	if (alt) {
		somekey->aux_string = g_strdup_(alt);
	} /* End if */
	else
		somekey->aux_string = NULL;

	return (somekey);
}				/* End gtkeyboard_new_key() */

static KEY *gtkeyboard_destroy_key(KEY * input)
{
	if (!input) {
		fprintf(Q,
			"Error:  gtkeyboard_destroy_key:  NULL argument.\n");
		fflush(Q);
		return (NO_KEY);
	}
	/* End if */
	if (input->aux_string) {
		g_free_(input->aux_string);
		input->aux_string = (char *) NULL;
	}
	/* End if */
	g_free_(input);
	input = NO_KEY;		/* Null pointer so it won't be reused */

	return (NO_KEY);
}				/* End gtkeyboard_destroy_key() */

static KEYBOARD *gtkeyboard_destroy_keyboard(KEYBOARD * input)
{
	if (!input) {
		fprintf(stderr,
			"gtkeyboard_destroy_keyboard: Cannot destroy NULL ptr.\n");
		fflush(stderr);
		return (NO_KEYBOARD);
	}
	/* End if */
	if (input->syms) {
		g_free_(input->syms);
		input->syms = NULL;
	}
	/* End if */
	if (input->name) {
		g_free_(input->name);
		input->name = NULL;
	}
	/* End if */
	if (input->codes) {
		g_free_(input->codes);
		input->codes = NULL;
	}
	/* End if */
	if (input->modmap) {
		ModmapTable_destroy(input->modmap);
		input->modmap = (ModmapTable *) NULL;
	}
	/* End if */
	if (input->trans) {
		CONDFREE(input->trans->space);
		CONDFREE(input->trans->tab);
		CONDFREE(input->trans->alt_gr);
		CONDFREE(input->trans->alt);
		CONDFREE(input->trans->control);
		CONDFREE(input->trans->shift);
		CONDFREE(input->trans->backspace);

		/* Free the parent pointer */
		CONDFREE(input->trans);
	}
	/* End if */
	g_free_(input);
	input = NO_KEYBOARD;

	return (NO_KEYBOARD);
}				/* End gtkeyboard_destroy_keyboard() */

static int get_valid_line(FILE * fp, char *mem, const int maxlen)
{
	mem[0] = '\0';

	while (!feof(fp) && mem[0] == '\0') {
		FILE_readline(fp, mem, maxlen);

		if (mem[0] != '\0') {
			strcpy(mem, g_strstrip(mem));

			if ((mem[0] && mem[0] == '#')
			    || (mem[0] && mem[0] == '!'))
				mem[0] = '\0';
		}		/* End if */
	}			/* End while */

	if (mem[0] == '\0')
		return (0);

	else
		return (1);
}				/* End get_valid_line() */

/* Parses the contents of a keyboard description file and returns
 * a corresponding KEYBOARD structure.
 */
static KEYBOARD *read_keyboard_template(char *filename)
{
	KEYBOARD *keyb = NO_KEYBOARD;
	FILE *fp;
	register int x = 0, y = 0;
	int line_size = 1024;
	int index = 0;
	char linedata[line_size];
	char **tokens;
	char **tofree;
	char *ptr;

	if (!filename || !file_exists(filename)) {
		fprintf(stderr, "Error loading keyboard file \"%s\":  ",
			(filename ? filename : "NULL"));
		fprintf(stderr, "File doesn't exist.");
		fflush(stderr);
		return (NO_KEYBOARD);
	}
	/* End if */
	fp = fopen(filename, "r");

	if (!fp) {
		return (NO_KEYBOARD);
	}
	/* End if */
	linedata[0] = '\0';

	if (!get_valid_line(fp, linedata, line_size)) {
		fclose(fp);
		return (NO_KEYBOARD);
	}
	/* End if */
	keyb = g_new0_(KEYBOARD, 1);
	keyb->modmap = ModmapTable_new();

	tofree = g_strsplit(linedata, " ", -1);
	tokens = tofree;

	keyb->keycount = 0;
	keyb->trans = g_new_(KeyboardTranslation, 1);

	/* Initialize it's various elements */
	keyb->trans->shift = keyb->trans->backspace = keyb->trans->space =
	    keyb->trans->caps_lock = keyb->trans->control =
	    keyb->trans->tab = keyb->trans->alt = keyb->trans->alt_gr =
	    (char *) NULL;

	for (x = 0; x < MAXIMUM_ROWS; x++) {
		if (*tokens)
			ptr = *tokens++;
		else
			ptr = NULL;

		if (ptr)
			keyb->row_values[x] = atoi(ptr);
		else {
			*tokens = NULL;
			keyb->row_values[x] = 0;
		}		/* End else */

		keyb->keycount += keyb->row_values[x];
	}			/* End for */

	g_strfreev(tofree);
	tofree = tokens = NULL;
	ptr = NULL;

	/* We now know how many keys we have to allocate, how many lines to read,
	 * and all that good stuff.
	 *
	 * Each key must have 3 syms, (lower case, upper case, and Alt Gr)
	 * So allocate 3*keyb->keycount items, and read keyb->keycount lines.
	 */
	keyb->syms = g_new0_(KeySym, (3 * keyb->keycount));
	keyb->codes = g_new0_(KeyCode, keyb->keycount);
	keyb->name = g_strdup_(filename);	/* Save the name of the keyboard */

	for (x = 0; x < keyb->keycount; x++) {
		keyb->codes[x] = 0;	/* Initialize that keycode since we're already
					 * paying the price of the loop and it needs
					 * to be done.
					 */

		if (!get_valid_line(fp, linedata, line_size)) {
			fprintf(stderr,
				"Error reading file %s: Bad line %d.\n",
				filename, (x + 1));
			fflush(stderr);
			fflush(stderr);
			keyb = gtkeyboard_destroy_keyboard(keyb);
			fclose(fp);
			return (NO_KEYBOARD);
		}
		/* End if */
		tokens = tofree = g_strsplit(linedata, " ", -1);

		for (y = 0; y < 3; y++) {
			if (*tokens)
				ptr = *tokens++;
			else
				ptr = NULL;

			index = (x * 3) + y;

			if (ptr) {
				/* Translate a string into a KeySym */
				keyb->syms[index] = XStringToKeysym(ptr);

				/* Error check that KeySym */
				if (!keyb->syms[index]
				    || keyb->syms[index] == NoSymbol) {
					keyb->syms[index] = NoSymbol;
					keyb =
					    gtkeyboard_destroy_keyboard
					    (keyb);
					return (NO_KEYBOARD);
				}	/* End if */
			} /* End if */
			else {
				/* This kinda sucks */
				keyb->syms[index] = NoSymbol;
			}	/* End else */
		}		/* End for */

		if (ptr) {
			ptr = *tokens++;
		}

		/* End if */
		/* Grab the KeyCode if it's there */
		keyb->codes[x] = atoi(ptr ? ptr : "0");

		if (ptr) {
			ptr = *tokens++;
		}
		/* End if */
		if (ptr && strcmp(ptr, "") != 0) {
#if 0
			fprintf(Q, "Reading proposed mask \"%s\"\n", ptr);
			fflush(Q);
#endif

			if (!ModmapTable_insert
			    (keyb->modmap, keyb->codes[x],
			     mask_name_to_slot_number(ptr))) {
				fprintf(Q,
					"*** Warning:  Failed to insert %d into %d\n",
					keyb->codes[x],
					mask_name_to_slot_number(ptr));
			}
			/* End if */
#if 0
			fprintf(Q, "Inserted code %d in slot %d\n",
				keyb->codes[x],
				mask_name_to_slot_number(ptr));
			fflush(Q);
#endif
		}
		/* End if */
		g_strfreev(tofree);
	}			/* End for */

	fclose(fp);

	return (keyb);
}				/* End read_keyboard_template() */

static void send_redirect_a_keysym(KeySym input)
{
	Window window;
	int revert_to;

	if (!options.other || options.other == (Window) NULL) {
		/* SEND_TO_BOTH_WINDOWS was probably set and there wasn't
		 * a redirect window to send to.  Let's save the time involved
		 * with doing all this string crap and just jump out here.
		 */
		return;
	}
	/* End if */
	if (options.other) {
		/* send to window user picked */
		keysym_sendkey(input, options.other);
	} /* End if */
	else {
		/* default to just send the event to whatever window has the input
		 * focus 
		 */
		XGetInputFocus(GDK_DISPLAY(), &window, &revert_to);
		keysym_sendkey(input, window);
	}			/* End else */
}				/* End send_redirect_a_keysym() */

gint track_focus(gpointer data)
{
	Window winFocus;
	Window wfcopy;
	int revert_to_return;
	char *winName;


	/* find out which window currently has focus */
	XGetInputFocus(GDK_DISPLAY(), &winFocus, &revert_to_return);
	wfcopy = winFocus;

	/* Return if the window is the same or if it's the program
	 * window or if we can't redirect to that window.
	 *
	 * If there was a previous window that was any good, stick to
	 * that one.
	 */
	if (winFocus == options.redirect_window ||
	    winFocus == GUI.xwindow ||
	    winFocus == None || winFocus == PointerRoot) {
		return TRUE;
	}



	/* End if */
	/* At this point, we know the window is "good" and that we want
	 * it's name.  We're going to use it as the redirect from now on.
	 */
	/* set up error trapping, in case we get a BadWindow error */
	gdk_error_trap_push();

	/* this could generate the error */
	XFetchName(GDK_DISPLAY(), winFocus, &winName);
	if (!winName)
		winName = "Unknown";

	gdk_flush();

	if (gdk_error_trap_pop()) {
		/* Oops...error.  Probably BadWindow */
		CONDFREE(options.redirect_window_name);

		options.redirect_window = None;	/* reset focus window */
		options.other = None;

		printf
		    ("There was an error finding a valid redirect window.\n");
		return TRUE;	/* better luck next time */
	}

	/* End if */
	/* since we made it this far, update the window_name */
	if (winName) {
		CONDFREE(options.redirect_window_name);

		/* Grab the window definition */
		options.redirect_window = wfcopy;
		options.other = wfcopy;

		options.redirect_window_name = g_strdup_(winName);
	}			/* End if */
	return TRUE;
}				/* End track_focus */
