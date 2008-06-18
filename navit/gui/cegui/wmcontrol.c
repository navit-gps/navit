/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
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


// This is loosely based upon wmctrl 1.07.

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/Xmu/WinUtil.h>
#include <glib.h>


#include "wmcontrol.h"

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */

#define MAX_PROPERTY_VALUE_LEN 4096
#define SELECT_WINDOW_MAGIC ":SELECT:"
#define ACTIVE_WINDOW_MAGIC ":ACTIVE:"

#define p_verbose(...) if (options.verbose) { \
    fprintf(stderr, __VA_ARGS__); \
}

/* declarations of static functions *//*{{{*/
static gboolean wm_supports (Display *disp, const gchar *prop);
static Window *get_client_list (Display *disp, unsigned long *size);
static int client_msg(Display *disp, Window win, char *msg, 
        unsigned long data0, unsigned long data1, 
        unsigned long data2, unsigned long data3,
        unsigned long data4);
static int list_windows (Display *disp);
static int list_desktops (Display *disp);
static int showing_desktop (Display *disp);
static int change_viewport (Display *disp);
static int change_geometry (Display *disp);
static int change_number_of_desktops (Display *disp);
static int switch_desktop (Display *disp);
static int wm_info (Display *disp);
static gchar *get_output_str (gchar *str, gboolean is_utf8);
static int action_window (Display *disp, Window win, char mode);
static int action_window_pid (Display *disp, char mode);
static int action_window_str (Display *disp, char mode);
static int activate_window (Display *disp, Window win, 
        gboolean switch_desktop);
static int close_window (Display *disp, Window win);
static int longest_str (gchar **strv);
static int window_to_desktop (Display *disp, Window win, int desktop);
static void window_set_title (Display *disp, Window win, char *str, char mode);
static gchar *get_window_title (Display *disp, Window win);
static gchar *get_window_class (Display *disp, Window win);
static gchar *get_property (Display *disp, Window win, 
        Atom xa_prop_type, gchar *prop_name, unsigned long *size);
static void init_charset(void);
static int window_move_resize (Display *disp, Window win, char *arg);
static int window_state (Display *disp, Window win, char *arg);
static Window Select_Window(Display *dpy);
static Window get_active_window(Display *dpy);

/*}}}*/
   
static struct {
    int verbose;
    int force_utf8;
    int show_class;
    int show_pid;
    int show_geometry;
    int match_by_id;
	int match_by_cls;
    int full_window_title_match;
    int wa_desktop_titles_invalid_utf8;
    char *param_window;
    char *param;
} options;

static gboolean envir_utf8;

int window_switch(const char *windowName) { /* {{{ */
    int opt;
    int ret = EXIT_SUCCESS;
    int missing_option = 1;
    Display *disp;

    memset(&options, 0, sizeof(options)); /* just for sure */
    
    /* necessary to make g_get_charset() and g_locale_*() work */
    setlocale(LC_ALL, "");
    
    options.param_window=malloc(strlen(windowName));
    strncpy(options.param_window,windowName,strlen(windowName));

    init_charset();
    
    if (! (disp = XOpenDisplay(NULL))) {
        fputs("Cannot open display.\n", stderr);
        return EXIT_FAILURE;
    }
    if (! options.param_window) {
        fputs("No window was specified.\n", stderr);
        return EXIT_FAILURE;
    }
        ret = action_window_str(disp, 'a');
    XCloseDisplay(disp);
    return ret;
}
/* }}} */

static void init_charset (void) {/*{{{*/
    const gchar *charset; /* unused */
    gchar *lang = getenv("LANG") ? g_ascii_strup(getenv("LANG"), -1) : NULL; 
    gchar *lc_ctype = getenv("LC_CTYPE") ? g_ascii_strup(getenv("LC_CTYPE"), -1) : NULL;
    
    /* this glib function doesn't work on my system ... */
    envir_utf8 = g_get_charset(&charset);

    /* ... therefore we will examine the environment variables */
    if (lc_ctype && (strstr(lc_ctype, "UTF8") || strstr(lc_ctype, "UTF-8"))) {
        envir_utf8 = TRUE;
    }
    else if (lang && (strstr(lang, "UTF8") || strstr(lang, "UTF-8"))) {
        envir_utf8 = TRUE;
    }

    g_free(lang);
    g_free(lc_ctype);
    
    if (options.force_utf8) {
        envir_utf8 = TRUE;
    }
    p_verbose("envir_utf8: %d\n", envir_utf8);
}/*}}}*/

static int client_msg(Display *disp, Window win, char *msg, /* {{{ */
        unsigned long data0, unsigned long data1, 
        unsigned long data2, unsigned long data3,
        unsigned long data4) {
    XEvent event;
    long mask = SubstructureRedirectMask | SubstructureNotifyMask;

    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = XInternAtom(disp, msg, False);
    event.xclient.window = win;
    event.xclient.format = 32;
    event.xclient.data.l[0] = data0;
    event.xclient.data.l[1] = data1;
    event.xclient.data.l[2] = data2;
    event.xclient.data.l[3] = data3;
    event.xclient.data.l[4] = data4;
    
    if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event)) {
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "Cannot send %s event.\n", msg);
        return EXIT_FAILURE;
    }
}/*}}}*/

static gchar *get_output_str (gchar *str, gboolean is_utf8) {/*{{{*/
    gchar *out;
   
    if (str == NULL) {
        return NULL;
    }
    
    if (envir_utf8) {
        if (is_utf8) {
            out = g_strdup(str);
        }
        else {
            if (! (out = g_locale_to_utf8(str, -1, NULL, NULL, NULL))) {
                p_verbose("Cannot convert string from locale charset to UTF-8.\n");
                out = g_strdup(str);
            }
        }
    }
    else {
        if (is_utf8) {
            if (! (out = g_locale_from_utf8(str, -1, NULL, NULL, NULL))) {
                p_verbose("Cannot convert string from UTF-8 to locale charset.\n");
                out = g_strdup(str);
            }
        }
        else {
            out = g_strdup(str);
        }
    }

    return out;
}/*}}}*/

static int wm_info (Display *disp) {/*{{{*/
    Window *sup_window = NULL;
    gchar *wm_name = NULL;
    gchar *wm_class = NULL;
    unsigned long *wm_pid = NULL;
    unsigned long *showing_desktop = NULL;
    gboolean name_is_utf8 = TRUE;
    gchar *name_out;
    gchar *class_out;
    
    if (! (sup_window = (Window *)get_property(disp, DefaultRootWindow(disp),
                    XA_WINDOW, "_NET_SUPPORTING_WM_CHECK", NULL))) {
        if (! (sup_window = (Window *)get_property(disp, DefaultRootWindow(disp),
                        XA_CARDINAL, "_WIN_SUPPORTING_WM_CHECK", NULL))) {
            fputs("Cannot get window manager info properties.\n"
                  "(_NET_SUPPORTING_WM_CHECK or _WIN_SUPPORTING_WM_CHECK)\n", stderr);
            return EXIT_FAILURE;
        }
    }

    /* WM_NAME */
    if (! (wm_name = get_property(disp, *sup_window,
            XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL))) {
        name_is_utf8 = FALSE;
        if (! (wm_name = get_property(disp, *sup_window,
                XA_STRING, "_NET_WM_NAME", NULL))) {
            p_verbose("Cannot get name of the window manager (_NET_WM_NAME).\n");
        }
    }
    name_out = get_output_str(wm_name, name_is_utf8);
  
    /* WM_CLASS */
    if (! (wm_class = get_property(disp, *sup_window,
            XInternAtom(disp, "UTF8_STRING", False), "WM_CLASS", NULL))) {
        name_is_utf8 = FALSE;
        if (! (wm_class = get_property(disp, *sup_window,
                XA_STRING, "WM_CLASS", NULL))) {
            p_verbose("Cannot get class of the window manager (WM_CLASS).\n");
        }
    }
    class_out = get_output_str(wm_class, name_is_utf8);
  

    /* WM_PID */
    if (! (wm_pid = (unsigned long *)get_property(disp, *sup_window,
                    XA_CARDINAL, "_NET_WM_PID", NULL))) {
        p_verbose("Cannot get pid of the window manager (_NET_WM_PID).\n");
    }
    
    /* _NET_SHOWING_DESKTOP */
    if (! (showing_desktop = (unsigned long *)get_property(disp, DefaultRootWindow(disp),
                    XA_CARDINAL, "_NET_SHOWING_DESKTOP", NULL))) {
        p_verbose("Cannot get the _NET_SHOWING_DESKTOP property.\n");
    }
    
    /* print out the info */
    printf("Name: %s\n", name_out ? name_out : "N/A");
    printf("Class: %s\n", class_out ? class_out : "N/A");
    
    if (wm_pid) {
        printf("PID: %lu\n", *wm_pid);
    }
    else {
        printf("PID: N/A\n");
    }
    
    if (showing_desktop) {
        printf("Window manager's \"showing the desktop\" mode: %s\n",
                *showing_desktop == 1 ? "ON" : "OFF");
    }
    else {
        printf("Window manager's \"showing the desktop\" mode: N/A\n");
    }
    
    g_free(name_out);
    g_free(sup_window);
    g_free(wm_name);
	g_free(wm_class);
    g_free(wm_pid);
    g_free(showing_desktop);
    
    return EXIT_SUCCESS;
}/*}}}*/

static int showing_desktop (Display *disp) {/*{{{*/
    unsigned long state;
    
    if (strcmp(options.param, "on") == 0) {
        state = 1;
    }
    else if (strcmp(options.param, "off") == 0) {
        state = 0;
    }
    else {
        fputs("The argument to the -k option must be either \"on\" or \"off\"\n", stderr);
        return EXIT_FAILURE;
    }

    return client_msg(disp, DefaultRootWindow(disp), "_NET_SHOWING_DESKTOP", 
        state, 0, 0, 0, 0);
}/*}}}*/

static int change_viewport (Display *disp) {/*{{{*/
    unsigned long x, y;
    const char *argerr = "The -o option expects two integers separated with a comma.\n";
   
    if (sscanf(options.param, "%lu,%lu", &x, &y) == 2) {
        return client_msg(disp, DefaultRootWindow(disp), "_NET_DESKTOP_VIEWPORT", 
            x, y, 0, 0, 0);
    }
    else {
        fputs(argerr, stderr);
        return EXIT_FAILURE;
    }
}/*}}}*/

static int change_geometry (Display *disp) {/*{{{*/
    unsigned long x, y;
    const char *argerr = "The -g option expects two integers separated with a comma.\n";
   
    if (sscanf(options.param, "%lu,%lu", &x, &y) == 2) {
        return client_msg(disp, DefaultRootWindow(disp), "_NET_DESKTOP_GEOMETRY", 
            x, y, 0, 0, 0);
    }
    else {
        fputs(argerr, stderr);
        return EXIT_FAILURE;
    }
}/*}}}*/

static int change_number_of_desktops (Display *disp) {/*{{{*/
    unsigned long n;

    if (sscanf(options.param, "%lu", &n) != 1) {
        fputs("The -n option expects an integer.\n", stderr);
        return EXIT_FAILURE;
    }
    
    return client_msg(disp, DefaultRootWindow(disp), "_NET_NUMBER_OF_DESKTOPS", 
        n, 0, 0, 0, 0);
}/*}}}*/

static int switch_desktop (Display *disp) {/*{{{*/
    int target = -1;
    
    target = atoi(options.param); 
    if (target == -1) {
        fputs("Invalid desktop ID.\n", stderr);
        return EXIT_FAILURE;
    }
    
    return client_msg(disp, DefaultRootWindow(disp), "_NET_CURRENT_DESKTOP", 
            (unsigned long)target, 0, 0, 0, 0);
}/*}}}*/

static void window_set_title (Display *disp, Window win, /* {{{ */
        char *title, char mode) {
    gchar *title_utf8;
    gchar *title_local;

    if (envir_utf8) {
        title_utf8 = g_strdup(title);
        title_local = NULL;
    }
    else {
        if (! (title_utf8 = g_locale_to_utf8(title, -1, NULL, NULL, NULL))) {
            title_utf8 = g_strdup(title);
        }
        title_local = g_strdup(title);
    }
    
    if (mode == 'T' || mode == 'N') {
        /* set name */
        if (title_local) {
            XChangeProperty(disp, win, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
                    title_local, strlen(title_local));
        }
        else {
            XDeleteProperty(disp, win, XA_WM_NAME);
        }
        XChangeProperty(disp, win, XInternAtom(disp, "_NET_WM_NAME", False), 
                XInternAtom(disp, "UTF8_STRING", False), 8, PropModeReplace,
                title_utf8, strlen(title_utf8));
    }

    if (mode == 'T' || mode == 'I') {
        /* set icon name */
        if (title_local) {
            XChangeProperty(disp, win, XA_WM_ICON_NAME, XA_STRING, 8, PropModeReplace,
                    title_local, strlen(title_local));
        }
        else {
            XDeleteProperty(disp, win, XA_WM_ICON_NAME);
        }
        XChangeProperty(disp, win, XInternAtom(disp, "_NET_WM_ICON_NAME", False), 
                XInternAtom(disp, "UTF8_STRING", False), 8, PropModeReplace,
                title_utf8, strlen(title_utf8));
    }
    
    g_free(title_utf8);
    g_free(title_local);
    
}/*}}}*/

static int window_to_desktop (Display *disp, Window win, int desktop) {/*{{{*/
    unsigned long *cur_desktop = NULL;
    Window root = DefaultRootWindow(disp);
   
    if (desktop == -1) {
        if (! (cur_desktop = (unsigned long *)get_property(disp, root,
                XA_CARDINAL, "_NET_CURRENT_DESKTOP", NULL))) {
            if (! (cur_desktop = (unsigned long *)get_property(disp, root,
                    XA_CARDINAL, "_WIN_WORKSPACE", NULL))) {
                fputs("Cannot get current desktop properties. "
                      "(_NET_CURRENT_DESKTOP or _WIN_WORKSPACE property)"
                      "\n", stderr);
                return EXIT_FAILURE;
            }
        }
        desktop = *cur_desktop;
    }
    g_free(cur_desktop);

    return client_msg(disp, win, "_NET_WM_DESKTOP", (unsigned long)desktop,
            0, 0, 0, 0);
}/*}}}*/

static int activate_window (Display *disp, Window win, /* {{{ */
        gboolean switch_desktop) {
    unsigned long *desktop;


	printf("sounds good\n");
    /* desktop ID */
    if ((desktop = (unsigned long *)get_property(disp, win,
            XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL) {
        if ((desktop = (unsigned long *)get_property(disp, win,
                XA_CARDINAL, "_WIN_WORKSPACE", NULL)) == NULL) {
            p_verbose("Cannot find desktop ID of the window.\n");
        }
    }

    if (switch_desktop && desktop) {
        if (client_msg(disp, DefaultRootWindow(disp), 
                    "_NET_CURRENT_DESKTOP", 
                    *desktop, 0, 0, 0, 0) != EXIT_SUCCESS) {
            p_verbose("Cannot switch desktop.\n");
        }
        g_free(desktop);
    }

    client_msg(disp, win, "_NET_ACTIVE_WINDOW", 
            0, 0, 0, 0, 0);
    XMapRaised(disp, win);

    return EXIT_SUCCESS;
}/*}}}*/

static int close_window (Display *disp, Window win) {/*{{{*/
    return client_msg(disp, win, "_NET_CLOSE_WINDOW", 
            0, 0, 0, 0, 0);
}/*}}}*/

static int window_state (Display *disp, Window win, char *arg) {/*{{{*/
    unsigned long action;
    Atom prop1 = 0;
    Atom prop2 = 0;
    char *p1, *p2;
    const char *argerr = "The -b option expects a list of comma separated parameters: \"(remove|add|toggle),<PROP1>[,<PROP2>]\"\n";

    if (!arg || strlen(arg) == 0) {
        fputs(argerr, stderr);
        return EXIT_FAILURE;
    }

    if ((p1 = strchr(arg, ','))) {
        gchar *tmp_prop1, *tmp1;
        
        *p1 = '\0';

        /* action */
        if (strcmp(arg, "remove") == 0) {
            action = _NET_WM_STATE_REMOVE;
        }
        else if (strcmp(arg, "add") == 0) {
            action = _NET_WM_STATE_ADD;
        }
        else if (strcmp(arg, "toggle") == 0) {
            action = _NET_WM_STATE_TOGGLE;
        }
        else {
            fputs("Invalid action. Use either remove, add or toggle.\n", stderr);
            return EXIT_FAILURE;
        }
        p1++;

        /* the second property */
        if ((p2 = strchr(p1, ','))) {
            gchar *tmp_prop2, *tmp2;
            *p2 = '\0';
            p2++;
            if (strlen(p2) == 0) {
                fputs("Invalid zero length property.\n", stderr);
                return EXIT_FAILURE;
            }
            tmp_prop2 = g_strdup_printf("_NET_WM_STATE_%s", tmp2 = g_ascii_strup(p2, -1));
            p_verbose("State 2: %s\n", tmp_prop2); 
            prop2 = XInternAtom(disp, tmp_prop2, False);
            g_free(tmp2);
            g_free(tmp_prop2);
        }

        /* the first property */
        if (strlen(p1) == 0) {
            fputs("Invalid zero length property.\n", stderr);
            return EXIT_FAILURE;
        }
        tmp_prop1 = g_strdup_printf("_NET_WM_STATE_%s", tmp1 = g_ascii_strup(p1, -1));
        p_verbose("State 1: %s\n", tmp_prop1); 
        prop1 = XInternAtom(disp, tmp_prop1, False);
        g_free(tmp1);
        g_free(tmp_prop1);

        
        return client_msg(disp, win, "_NET_WM_STATE", 
            action, (unsigned long)prop1, (unsigned long)prop2, 0, 0);
    }
    else {
        fputs(argerr, stderr);
        return EXIT_FAILURE;
    }
}/*}}}*/

static gboolean wm_supports (Display *disp, const gchar *prop) {/*{{{*/
    Atom xa_prop = XInternAtom(disp, prop, False);
    Atom *list;
    unsigned long size;
    int i;

    if (! (list = (Atom *)get_property(disp, DefaultRootWindow(disp),
            XA_ATOM, "_NET_SUPPORTED", &size))) {
        p_verbose("Cannot get _NET_SUPPORTED property.\n");
        return FALSE;
    }

    for (i = 0; i < size / sizeof(Atom); i++) {
        if (list[i] == xa_prop) {
            g_free(list);
            return TRUE;
        }
    }
    
    g_free(list);
    return FALSE;
}/*}}}*/

static int window_move_resize (Display *disp, Window win, char *arg) {/*{{{*/
    signed long grav, x, y, w, h;
    unsigned long grflags;
    const char *argerr = "The -e option expects a list of comma separated integers: \"gravity,X,Y,width,height\"\n";
    
    if (!arg || strlen(arg) == 0) {
        fputs(argerr, stderr);
        return EXIT_FAILURE;
    }

    if (sscanf(arg, "%ld,%ld,%ld,%ld,%ld", &grav, &x, &y, &w, &h) != 5) {
        fputs(argerr, stderr);
        return EXIT_FAILURE;
    }

    if (grav < 0) {
        fputs("Value of gravity mustn't be negative. Use zero to use the default gravity of the window.\n", stderr);
        return EXIT_FAILURE;
    }
    
    grflags = grav;
    if (x != -1) grflags |= (1 << 8);
    if (y != -1) grflags |= (1 << 9);
    if (w != -1) grflags |= (1 << 10);
    if (h != -1) grflags |= (1 << 11);
   
    p_verbose("grflags: %lu\n", grflags);
   
    if (wm_supports(disp, "_NET_MOVERESIZE_WINDOW")){
        return client_msg(disp, win, "_NET_MOVERESIZE_WINDOW", 
            grflags, (unsigned long)x, (unsigned long)y, (unsigned long)w, (unsigned long)h);
    }
    else {
        p_verbose("WM doesn't support _NET_MOVERESIZE_WINDOW. Gravity will be ignored.\n");
        if ((w < 1 || h < 1) && (x >= 0 && y >= 0)) {
            XMoveWindow(disp, win, x, y);
        }
        else if ((x < 0 || y < 0) && (w >= 1 && h >= -1)) {
            XResizeWindow(disp, win, w, h);
        }
        else if (x >= 0 && y >= 0 && w >= 1 && h >= 1) {
            XMoveResizeWindow(disp, win, x, y, w, h);
        }
        return EXIT_SUCCESS;
    }
}/*}}}*/

static int action_window (Display *disp, Window win, char mode) {/*{{{*/
    p_verbose("Using window: 0x%.8lx\n", win);
    switch (mode) {
        case 'a':
            return activate_window(disp, win, TRUE);

        case 'c':
            return close_window(disp, win);

        case 'e':
            /* resize/move the window around the desktop => -r -e */
            return window_move_resize(disp, win, options.param);

        case 'b':
            /* change state of a window => -r -b */
            return window_state(disp, win, options.param);
        
        case 't':
            /* move the window to the specified desktop => -r -t */
            return window_to_desktop(disp, win, atoi(options.param));
        
        case 'R':
            /* move the window to the current desktop and activate it => -r */
            if (window_to_desktop(disp, win, -1) == EXIT_SUCCESS) {
                usleep(100000); /* 100 ms - make sure the WM has enough
                    time to move the window, before we activate it */
                return activate_window(disp, win, FALSE);
            }
            else {
                return EXIT_FAILURE;
            }

        case 'N': case 'I': case 'T':
            window_set_title(disp, win, options.param, mode);
            return EXIT_SUCCESS;

        default:
            fprintf(stderr, "Unknown action: '%c'\n", mode);
            return EXIT_FAILURE;
    }
}/*}}}*/

static int action_window_pid (Display *disp, char mode) {/*{{{*/
    unsigned long wid;

    if (sscanf(options.param_window, "0x%lx", &wid) != 1 &&
            sscanf(options.param_window, "0X%lx", &wid) != 1 &&
            sscanf(options.param_window, "%lu", &wid) != 1) {
        fputs("Cannot convert argument to number.\n", stderr);
        return EXIT_FAILURE;
    }

    return action_window(disp, (Window)wid, mode);
}/*}}}*/

static int action_window_str (Display *disp, char mode) {/*{{{*/
    Window activate = 0;
    Window *client_list;
    unsigned long client_list_size;
    int i;
    
    if (strcmp(SELECT_WINDOW_MAGIC, options.param_window) == 0) {
        activate = Select_Window(disp);
        if (activate) {
            return action_window(disp, activate, mode);
        }
        else {
            return EXIT_FAILURE;
        }
    }
    if (strcmp(ACTIVE_WINDOW_MAGIC, options.param_window) == 0) {
        activate = get_active_window(disp);
        if (activate)
        {
            return action_window(disp, activate, mode);
        }
        else
        {
            return EXIT_FAILURE;
        }
    }
    else {
        if ((client_list = get_client_list(disp, &client_list_size)) == NULL) {
            return EXIT_FAILURE; 
        }
        
        for (i = 0; i < client_list_size / sizeof(Window); i++) {
 			gchar *match_utf8;
 			if (options.show_class) {
 	            match_utf8 = get_window_class(disp, client_list[i]); /* UTF8 */
 			}
 			else {
 				match_utf8 = get_window_title(disp, client_list[i]); /* UTF8 */
 			}
            if (match_utf8) {
                gchar *match;
                gchar *match_cf;
                gchar *match_utf8_cf = NULL;
                if (envir_utf8) {
                    match = g_strdup(options.param_window);
                    match_cf = g_utf8_casefold(options.param_window, -1);
                }
                else {
                    if (! (match = g_locale_to_utf8(options.param_window, -1, NULL, NULL, NULL))) {
                        match = g_strdup(options.param_window);
                    }
                    match_cf = g_utf8_casefold(match, -1);
                }
                
                if (!match || !match_cf) {
                    continue;
                }

                match_utf8_cf = g_utf8_casefold(match_utf8, -1);

                if ((options.full_window_title_match && strcmp(match_utf8, match) == 0) ||
                        (!options.full_window_title_match && strstr(match_utf8_cf, match_cf))) {
                    activate = client_list[i];
                    g_free(match);
                    g_free(match_cf);
                    g_free(match_utf8);
                    g_free(match_utf8_cf);
                    break;
                }
                g_free(match);
                g_free(match_cf);
                g_free(match_utf8);
                g_free(match_utf8_cf);
            }
        }
        g_free(client_list);

        if (activate) {
		return action_window(disp, activate, mode);
        }
        else {
            return EXIT_FAILURE;
        }
    }
}/*}}}*/

static int list_desktops (Display *disp) {/*{{{*/
    unsigned long *num_desktops = NULL;
    unsigned long *cur_desktop = NULL;
    unsigned long desktop_list_size = 0;
    unsigned long *desktop_geometry = NULL;
    unsigned long desktop_geometry_size = 0;
    gchar **desktop_geometry_str = NULL;
    unsigned long *desktop_viewport = NULL;
    unsigned long desktop_viewport_size = 0;
    gchar **desktop_viewport_str = NULL;
    unsigned long *desktop_workarea = NULL;
    unsigned long desktop_workarea_size = 0;
    gchar **desktop_workarea_str = NULL;
    gchar *list = NULL;
    int i;
    int id;
    Window root = DefaultRootWindow(disp);
    int ret = EXIT_FAILURE;
    gchar **names = NULL;
    gboolean names_are_utf8 = TRUE;
    
    if (! (num_desktops = (unsigned long *)get_property(disp, root,
            XA_CARDINAL, "_NET_NUMBER_OF_DESKTOPS", NULL))) {
        if (! (num_desktops = (unsigned long *)get_property(disp, root,
                XA_CARDINAL, "_WIN_WORKSPACE_COUNT", NULL))) {
            fputs("Cannot get number of desktops properties. "
                  "(_NET_NUMBER_OF_DESKTOPS or _WIN_WORKSPACE_COUNT)"
                  "\n", stderr);
            goto cleanup;
        }
    }
    
    if (! (cur_desktop = (unsigned long *)get_property(disp, root,
            XA_CARDINAL, "_NET_CURRENT_DESKTOP", NULL))) {
        if (! (cur_desktop = (unsigned long *)get_property(disp, root,
                XA_CARDINAL, "_WIN_WORKSPACE", NULL))) {
            fputs("Cannot get current desktop properties. "
                  "(_NET_CURRENT_DESKTOP or _WIN_WORKSPACE property)"
                  "\n", stderr);
            goto cleanup;
        }
    }

    if (options.wa_desktop_titles_invalid_utf8 || 
            (list = get_property(disp, root, 
            XInternAtom(disp, "UTF8_STRING", False), 
            "_NET_DESKTOP_NAMES", &desktop_list_size)) == NULL) {
        names_are_utf8 = FALSE;
        if ((list = get_property(disp, root, 
            XA_STRING, 
            "_WIN_WORKSPACE_NAMES", &desktop_list_size)) == NULL) {
            p_verbose("Cannot get desktop names properties. "
                  "(_NET_DESKTOP_NAMES or _WIN_WORKSPACE_NAMES)"
                  "\n");
            /* ignore the error - list the desktops without names */
        }
    }
 
    /* common size of all desktops */
    if (! (desktop_geometry = (unsigned long *)get_property(disp, DefaultRootWindow(disp),
                    XA_CARDINAL, "_NET_DESKTOP_GEOMETRY", &desktop_geometry_size))) {
        p_verbose("Cannot get common size of all desktops (_NET_DESKTOP_GEOMETRY).\n");
    }
     
    /* desktop viewport */
    if (! (desktop_viewport = (unsigned long *)get_property(disp, DefaultRootWindow(disp),
                    XA_CARDINAL, "_NET_DESKTOP_VIEWPORT", &desktop_viewport_size))) {
        p_verbose("Cannot get common size of all desktops (_NET_DESKTOP_VIEWPORT).\n");
    }
     
    /* desktop workarea */
    if (! (desktop_workarea = (unsigned long *)get_property(disp, DefaultRootWindow(disp),
                    XA_CARDINAL, "_NET_WORKAREA", &desktop_workarea_size))) {
        if (! (desktop_workarea = (unsigned long *)get_property(disp, DefaultRootWindow(disp),
                        XA_CARDINAL, "_WIN_WORKAREA", &desktop_workarea_size))) {
            p_verbose("Cannot get _NET_WORKAREA property.\n");
        }
    }
     
    /* prepare the array of desktop names */
    names = g_malloc0(*num_desktops * sizeof(char *));
    if (list) {
        id = 0;
        names[id++] = list;
        for (i = 0; i < desktop_list_size; i++) {
            if (list[i] == '\0') {
                if (id >= *num_desktops) {
                    break;
                }
                names[id++] = list + i + 1;
            }
        }
    }

    /* prepare desktop geometry strings */
    desktop_geometry_str = g_malloc0((*num_desktops + 1) * sizeof(char *));
    if (desktop_geometry && desktop_geometry_size > 0) {
        if (desktop_geometry_size == 2 * sizeof(*desktop_geometry)) {
            /* only one value - use it for all desktops */
            p_verbose("WM provides _NET_DESKTOP_GEOMETRY value common for all desktops.\n");
            for (i = 0; i < *num_desktops; i++) {
                desktop_geometry_str[i] = g_strdup_printf("%lux%lu", 
                desktop_geometry[0], desktop_geometry[1]);
            }
        }
        else {
            /* seperate values for desktops of different size */
            p_verbose("WM provides separate _NET_DESKTOP_GEOMETRY value for each desktop.\n");
            for (i = 0; i < *num_desktops; i++) {
                if (i < desktop_geometry_size / sizeof(*desktop_geometry) / 2) {
                    desktop_geometry_str[i] = g_strdup_printf("%lux%lu", 
                        desktop_geometry[i*2], desktop_geometry[i*2+1]);
                }
                else {
                    desktop_geometry_str[i] = g_strdup("N/A");
                }
            }
        }
    }
    else {
        for (i = 0; i < *num_desktops; i++) {
            desktop_geometry_str[i] = g_strdup("N/A");
        }
    }
 
    /* prepare desktop viewport strings */
    desktop_viewport_str = g_malloc0((*num_desktops + 1) * sizeof(char *));
    if (desktop_viewport && desktop_viewport_size > 0) {
        if (desktop_viewport_size == 2 * sizeof(*desktop_viewport)) {
            /* only one value - use it for current desktop */
            p_verbose("WM provides _NET_DESKTOP_VIEWPORT value only for the current desktop.\n");
            for (i = 0; i < *num_desktops; i++) {
                if (i == *cur_desktop) {
                    desktop_viewport_str[i] = g_strdup_printf("%lu,%lu", 
                        desktop_viewport[0], desktop_viewport[1]);
                }
                else {
                    desktop_viewport_str[i] = g_strdup("N/A");
                }
            }
        }
        else {
            /* seperate values for each of desktops */
            for (i = 0; i < *num_desktops; i++) {
                if (i < desktop_viewport_size / sizeof(*desktop_viewport) / 2) {
                    desktop_viewport_str[i] = g_strdup_printf("%lu,%lu", 
                        desktop_viewport[i*2], desktop_viewport[i*2+1]);
                }
                else {
                    desktop_viewport_str[i] = g_strdup("N/A");
                }
            }
        }
    }
    else {
        for (i = 0; i < *num_desktops; i++) {
            desktop_viewport_str[i] = g_strdup("N/A");
        }
    }
 
    /* prepare desktop workarea strings */
    desktop_workarea_str = g_malloc0((*num_desktops + 1) * sizeof(char *));
    if (desktop_workarea && desktop_workarea_size > 0) {
        if (desktop_workarea_size == 4 * sizeof(*desktop_workarea)) {
            /* only one value - use it for current desktop */
            p_verbose("WM provides _NET_WORKAREA value only for the current desktop.\n");
            for (i = 0; i < *num_desktops; i++) {
                if (i == *cur_desktop) {
                    desktop_workarea_str[i] = g_strdup_printf("%lu,%lu %lux%lu", 
                        desktop_workarea[0], desktop_workarea[1],
                        desktop_workarea[2], desktop_workarea[3]);
                }
                else {
                    desktop_workarea_str[i] = g_strdup("N/A");
                }
            }
        }
        else {
            /* seperate values for each of desktops */
            for (i = 0; i < *num_desktops; i++) {
                if (i < desktop_workarea_size / sizeof(*desktop_workarea) / 4) {
                    desktop_workarea_str[i] = g_strdup_printf("%lu,%lu %lux%lu", 
                        desktop_workarea[i*4], desktop_workarea[i*4+1],
                        desktop_workarea[i*4+2], desktop_workarea[i*4+3]);
                }
                else {
                    desktop_workarea_str[i] = g_strdup("N/A");
                }
            }
        }
    }
    else {
        for (i = 0; i < *num_desktops; i++) {
            desktop_workarea_str[i] = g_strdup("N/A");
        }
    }
 
    /* print the list */
    for (i = 0; i < *num_desktops; i++) {
        gchar *out = get_output_str(names[i], names_are_utf8);
        printf("%-2d %c DG: %-*s  VP: %-*s  WA: %-*s  %s\n", i, i == *cur_desktop ? '*' : '-',
                longest_str(desktop_geometry_str), desktop_geometry_str[i], 
                longest_str(desktop_viewport_str), desktop_viewport_str[i], 
                longest_str(desktop_workarea_str), desktop_workarea_str[i], 
                out ? out : "N/A");
        g_free(out);
    }
    
    p_verbose("Total number of desktops: %lu\n", *num_desktops);
    p_verbose("Current desktop ID (counted from zero): %lu\n", *cur_desktop);
    
    ret = EXIT_SUCCESS;
    goto cleanup;
    
cleanup:
    g_free(names);
    g_free(num_desktops);
    g_free(cur_desktop);
    g_free(desktop_geometry);
    g_strfreev(desktop_geometry_str);
    g_free(desktop_viewport);
    g_strfreev(desktop_viewport_str);
    g_free(desktop_workarea);
    g_strfreev(desktop_workarea_str);
    g_free(list);
    
    return ret;
}/*}}}*/

static int longest_str (gchar **strv) {/*{{{*/
    int max = 0;
    int i = 0;
    
    while (strv && strv[i]) {
        if (strlen(strv[i]) > max) {
            max = strlen(strv[i]);
        }
        i++;
    }

    return max;
}/*}}}*/

static Window *get_client_list (Display *disp, unsigned long *size) {/*{{{*/
    Window *client_list;

    if ((client_list = (Window *)get_property(disp, DefaultRootWindow(disp), 
                    XA_WINDOW, "_NET_CLIENT_LIST", size)) == NULL) {
        if ((client_list = (Window *)get_property(disp, DefaultRootWindow(disp), 
                        XA_CARDINAL, "_WIN_CLIENT_LIST", size)) == NULL) {
            fputs("Cannot get client list properties. \n"
                  "(_NET_CLIENT_LIST or _WIN_CLIENT_LIST)"
                  "\n", stderr);
            return NULL;
        }
    }

    return client_list;
}/*}}}*/

static int list_windows (Display *disp) {/*{{{*/
    Window *client_list;
    unsigned long client_list_size;
    int i;
    int max_client_machine_len = 0;
    
    if ((client_list = get_client_list(disp, &client_list_size)) == NULL) {
        return EXIT_FAILURE; 
    }
    
    /* find the longest client_machine name */
    for (i = 0; i < client_list_size / sizeof(Window); i++) {
        gchar *client_machine;
        if ((client_machine = get_property(disp, client_list[i],
                XA_STRING, "WM_CLIENT_MACHINE", NULL))) {
            max_client_machine_len = strlen(client_machine);    
        }
        g_free(client_machine);
    }
    
    /* print the list */
    for (i = 0; i < client_list_size / sizeof(Window); i++) {
        gchar *title_utf8 = get_window_title(disp, client_list[i]); /* UTF8 */
        gchar *title_out = get_output_str(title_utf8, TRUE);
        gchar *client_machine;
        gchar *class_out = get_window_class(disp, client_list[i]); /* UTF8 */
        unsigned long *pid;
        unsigned long *desktop;
        int x, y, junkx, junky;
        unsigned int wwidth, wheight, bw, depth;
        Window junkroot;

        /* desktop ID */
        if ((desktop = (unsigned long *)get_property(disp, client_list[i],
                XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL) {
            desktop = (unsigned long *)get_property(disp, client_list[i],
                    XA_CARDINAL, "_WIN_WORKSPACE", NULL);
        }

        /* client machine */
        client_machine = get_property(disp, client_list[i],
                XA_STRING, "WM_CLIENT_MACHINE", NULL);
       
        /* pid */
        pid = (unsigned long *)get_property(disp, client_list[i],
                XA_CARDINAL, "_NET_WM_PID", NULL);

	    /* geometry */
        XGetGeometry (disp, client_list[i], &junkroot, &junkx, &junky,
                          &wwidth, &wheight, &bw, &depth);
        XTranslateCoordinates (disp, client_list[i], junkroot, junkx, junky,
                               &x, &y, &junkroot);
      
        /* special desktop ID -1 means "all desktops", so we 
           have to convert the desktop value to signed long */
        printf("0x%.8lx %2ld", client_list[i], 
                desktop ? (signed long)*desktop : 0);
        if (options.show_pid) {
           printf(" %-6lu", pid ? *pid : 0);
        }
        if (options.show_geometry) {
           printf(" %-4d %-4d %-4d %-4d", x, y, wwidth, wheight);
        }
		if (options.show_class) {
		   printf(" %-20s ", class_out ? class_out : "N/A");
		}

        printf(" %*s %s\n",
              max_client_machine_len,
              client_machine ? client_machine : "N/A",
              title_out ? title_out : "N/A"
		);
        g_free(title_utf8);
        g_free(title_out);
        g_free(desktop);
        g_free(client_machine);
        g_free(class_out);
        g_free(pid);
    }
    g_free(client_list);
   
    return EXIT_SUCCESS;
}/*}}}*/

static gchar *get_window_class (Display *disp, Window win) {/*{{{*/
    gchar *class_utf8;
    gchar *wm_class;
    unsigned long size;

    wm_class = get_property(disp, win, XA_STRING, "WM_CLASS", &size);
    if (wm_class) {
        gchar *p_0 = strchr(wm_class, '\0');
        if (wm_class + size - 1 > p_0) {
            *(p_0) = '.';
        }
        class_utf8 = g_locale_to_utf8(wm_class, -1, NULL, NULL, NULL);
    }
    else {
        class_utf8 = NULL;
    }

    g_free(wm_class);
    
    return class_utf8;
}/*}}}*/

static gchar *get_window_title (Display *disp, Window win) {/*{{{*/
    gchar *title_utf8;
    gchar *wm_name;
    gchar *net_wm_name;

    wm_name = get_property(disp, win, XA_STRING, "WM_NAME", NULL);
    net_wm_name = get_property(disp, win, 
            XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL);

    if (net_wm_name) {
        title_utf8 = g_strdup(net_wm_name);
    }
    else {
        if (wm_name) {
            title_utf8 = g_locale_to_utf8(wm_name, -1, NULL, NULL, NULL);
        }
        else {
            title_utf8 = NULL;
        }
    }

    g_free(wm_name);
    g_free(net_wm_name);
    
    return title_utf8;
}/*}}}*/

static gchar *get_property (Display *disp, Window win, /*{{{*/
        Atom xa_prop_type, gchar *prop_name, unsigned long *size) {
    Atom xa_prop_name;
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned long tmp_size;
    unsigned char *ret_prop;
    gchar *ret;
    
    xa_prop_name = XInternAtom(disp, prop_name, False);
    
    /* MAX_PROPERTY_VALUE_LEN / 4 explanation (XGetWindowProperty manpage):
     *
     * long_length = Specifies the length in 32-bit multiples of the
     *               data to be retrieved.
     */
    if (XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4, False,
            xa_prop_type, &xa_ret_type, &ret_format,     
            &ret_nitems, &ret_bytes_after, &ret_prop) != Success) {
        p_verbose("Cannot get %s property.\n", prop_name);
        return NULL;
    }
  
    if (xa_ret_type != xa_prop_type) {
        p_verbose("Invalid type of %s property.\n", prop_name);
        XFree(ret_prop);
        return NULL;
    }

    /* null terminate the result to make string handling easier */
    tmp_size = (ret_format / 8) * ret_nitems;
    ret = g_malloc(tmp_size + 1);
    memcpy(ret, ret_prop, tmp_size);
    ret[tmp_size] = '\0';

    if (size) {
        *size = tmp_size;
    }
    
    XFree(ret_prop);
    return ret;
}/*}}}*/

static Window Select_Window(Display *dpy) {/*{{{*/
    /*
     * Routine to let user select a window using the mouse
     * Taken from xfree86.
     */

    int status;
    Cursor cursor;
    XEvent event;
    Window target_win = None, root = DefaultRootWindow(dpy);
    int buttons = 0;
    int dummyi;
    unsigned int dummy;

    /* Make the target cursor */
    cursor = XCreateFontCursor(dpy, XC_crosshair);

    /* Grab the pointer using target cursor, letting it room all over */
    status = XGrabPointer(dpy, root, False,
            ButtonPressMask|ButtonReleaseMask, GrabModeSync,
            GrabModeAsync, root, cursor, CurrentTime);
    if (status != GrabSuccess) {
        fputs("ERROR: Cannot grab mouse.\n", stderr);
        return 0;
    }

    /* Let the user select a window... */
    while ((target_win == None) || (buttons != 0)) {
        /* allow one more event */
        XAllowEvents(dpy, SyncPointer, CurrentTime);
        XWindowEvent(dpy, root, ButtonPressMask|ButtonReleaseMask, &event);
        switch (event.type) {
            case ButtonPress:
                if (target_win == None) {
                    target_win = event.xbutton.subwindow; /* window selected */
                    if (target_win == None) target_win = root;
                }
                buttons++;
                break;
            case ButtonRelease:
                if (buttons > 0) /* there may have been some down before we started */
                    buttons--;
                break;
        }
    } 

    XUngrabPointer(dpy, CurrentTime);      /* Done with pointer */

    if (XGetGeometry (dpy, target_win, &root, &dummyi, &dummyi,
                &dummy, &dummy, &dummy, &dummy) && target_win != root) {
        target_win = XmuClientWindow (dpy, target_win);
    }
    
    return(target_win);
}/*}}}*/

static Window get_active_window(Display *disp) {/*{{{*/
    char *prop;
    unsigned long size;
    Window ret = (Window)0;
    
    prop = get_property(disp, DefaultRootWindow(disp), XA_WINDOW, 
                        "_NET_ACTIVE_WINDOW", &size);
    if (prop) {
        ret = *((Window*)prop);
        g_free(prop);
    }

    return(ret);
}/*}}}*/


