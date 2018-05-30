/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2009 Navit Team
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

#define _USE_MATH_DEFINES 1
#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <glib.h>
#include <math.h>
#include <time.h>
#include "debug.h"
#include "navit.h"
#include "callback.h"
#include "gui.h"
#include "item.h"
#include "xmlconfig.h"
#include "projection.h"
#include "map.h"
#include "mapset.h"
#include "main.h"
#include "coord.h"
#include "point.h"
#include "transform.h"
#include "param.h"
#include "menu.h"
#include "graphics.h"
#include "popup.h"
#include "data_window.h"
#include "route.h"
#include "navigation.h"
#include "speech.h"
#include "track.h"
#include "vehicle.h"
#include "layout.h"
#include "log.h"
#include "attr.h"
#include "event.h"
#include "file.h"
#include "profile.h"
#include "command.h"
#include "navit_nls.h"
#include "map.h"
#include "util.h"
#include "messages.h"
#include "vehicleprofile.h"
#include "sunriset.h"
#include "bookmarks.h"
#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#include "util.h"
#endif
#ifdef HAVE_API_WIN32_CE
#include "libc.h"
#endif

/* define string for bookmark handling */
#define TEXTFILE_COMMENT_NAVI_STOPPED "# navigation stopped\n"

/**
 * @defgroup navit The navit core instance
 * @brief navit is the object containing most global data structures.
 *
 * Among others:
 * - a set of maps
 * - one or more vehicles
 * - a graphics object for rendering the map
 * - a gui object for displaying the user interface
 * - a route object
 * - a navigation object
 * @{
 */

//! The vehicle used for navigation.
struct navit_vehicle {
    int follow;
    /*! Limit of the follow counter. See navit_add_vehicle */
    int follow_curr;
    /*! Deprecated : follow counter itself. When it reaches 'update' counts, map is recentered*/
    struct coord coord;
    int dir;
    int speed;
    struct coord last; /*< Position of the last update of this vehicle */
    struct vehicle *vehicle;
    struct attr callback;
    int animate_cursor;
};

struct navit {
    NAVIT_OBJECT
    struct attr self;
    GList *mapsets;
    GList *layouts;
    struct gui *gui;
    struct layout *layout_current;
    struct graphics *gra;
    struct action *action;
    struct transformation *trans, *trans_cursor;
    struct compass *compass;
    struct route *route;
    struct navigation *navigation;
    struct speech *speech;
    struct tracking *tracking;
    int ready;
    struct window *win;
    struct displaylist *displaylist;
    int tracking_flag;
    int orientation;
    int recentdest_count;
    int osd_configuration;
    GList *vehicles;
    GList *windows_items;
    struct navit_vehicle *vehicle;
    struct callback_list *attr_cbl;
    struct callback *nav_speech_cb, *roadbook_callback, *popup_callback, *route_cb, *progress_cb;
    struct datawindow *roadbook_window;
    struct map *former_destination;
    struct point pressed, last, current;
    int button_pressed,moved,popped,zoomed;
    int center_timeout;
    int autozoom_secs;
    int autozoom_min;
    int autozoom_max;
    int autozoom_active;
    int autozoom_paused;
    struct event_timeout *button_timeout, *motion_timeout;
    struct callback *motion_timeout_callback;
    int ignore_button;
    int ignore_graphics_events;
    struct log *textfile_debug_log;
    struct pcoord destination;
    int destination_valid;
    int blocked;	/**< Whether draw operations are currently blocked. This can be a combination of the
					     following flags:
					     1: draw operations are blocked
					     2: draw operations are pending, requiring a redraw once draw operations are unblocked */
    int w,h;
    int drag_bitmap;
    int use_mousewheel;
    struct messagelist *messages;
    struct callback *resize_callback,*button_callback,*motion_callback,*predraw_callback;
    struct vehicleprofile *vehicleprofile;
    GList *vehicleprofiles;
    int pitch;
    int follow_cursor;
    int prevTs;
    int graphics_flags;
    int zoom_min, zoom_max;
    int radius;
    struct bookmarks *bookmarks;
    int flags;
    /* 1=No graphics ok */
    /* 2=No gui ok */
    int border;
    int imperial;
    int waypoints_flag;
    struct coord_geo center;
    int auto_switch; /*auto switching between day/night layout enabled ?*/
};

struct gui *main_loop_gui;

struct attr_iter {
    void *iter;
    union {
        GList *list;
        struct mapset_handle *mapset_handle;
    } u;
};

static void navit_vehicle_update_position(struct navit *this_, struct navit_vehicle *nv);
static void navit_vehicle_draw(struct navit *this_, struct navit_vehicle *nv, struct point *pnt);
static int navit_add_vehicle(struct navit *this_, struct vehicle *v);
static int navit_set_attr_do(struct navit *this_, struct attr *attr, int init);
static int navit_get_cursor_pnt(struct navit *this_, struct point *p, int keep_orientation, int *dir);
static void navit_set_cursors(struct navit *this_);
static void navit_cmd_zoom_to_route(struct navit *this);
static void navit_cmd_set_center_cursor(struct navit *this_);
static void navit_cmd_announcer_toggle(struct navit *this_);
static void navit_set_vehicle(struct navit *this_, struct navit_vehicle *nv);
static int navit_set_vehicleprofile(struct navit *this_, struct vehicleprofile *vp);
static void navit_cmd_switch_layout_day_night(struct navit *this_, char *function, struct attr **in, struct attr ***out,
        int valid);
struct object_func navit_func;

struct navit *global_navit;

void navit_add_mapset(struct navit *this_, struct mapset *ms) {
    this_->mapsets = g_list_append(this_->mapsets, ms);
}

struct mapset *
navit_get_mapset(struct navit *this_) {
    if(this_->mapsets) {
        return this_->mapsets->data;
    } else {
        dbg(lvl_error,"No mapsets enabled! Is it on purpose? Navit can't draw a map. Please check your navit.xml");
    }
    return NULL;
}

struct tracking *
navit_get_tracking(struct navit *this_) {
    return this_->tracking;
}

/**
 * @brief	Get the user data directory.
 * @param[in]	 create	- create the directory if it does not exist
 *
 * @return	char * to the data directory string.
 *
 * returns the directory used to store user data files (center.txt,
 * destination.txt, bookmark.txt, ...)
 *
 */
char* navit_get_user_data_directory(int create) {
    char *dir;
    dir = getenv("NAVIT_USER_DATADIR");
    if (create && !file_exists(dir)) {
        dbg(lvl_debug,"creating dir %s", dir);
        if (file_mkdir(dir,0)) {
            dbg(lvl_error,"failed creating dir %s", dir);
            return NULL;
        }
    }
    return dir;
}


void navit_draw_async(struct navit *this_, int async) {

    if (this_->blocked) {
        this_->blocked |= 2;
        return;
    }
    transform_setup_source_rect(this_->trans);
    graphics_draw(this_->gra, this_->displaylist, this_->mapsets->data, this_->trans, this_->layout_current, async, NULL,
                  this_->graphics_flags|1);
}

void navit_draw(struct navit *this_) {
    if (this_->ready == 3)
        navit_draw_async(this_, 0);
}

int navit_get_ready(struct navit *this_) {
    return this_->ready;
}



void navit_draw_displaylist(struct navit *this_) {
    if (this_->ready == 3)
        graphics_displaylist_draw(this_->gra, this_->displaylist, this_->trans, this_->layout_current, this_->graphics_flags|1);
}

static void navit_map_progress(struct navit *this_) {
    struct map *map;
    struct mapset *ms;
    struct mapset_handle *msh;
    struct attr attr;
    struct point p;
    if (this_->ready != 3)
        return;
    p.x=10;
    p.y=32;

    ms=this_->mapsets->data;
    msh=mapset_open(ms);
    while (msh && (map=mapset_next(msh, 0))) {
        if (map_get_attr(map, attr_progress, &attr, NULL)) {
            char *str=g_strdup_printf("%s           ",attr.u.str);
            graphics_draw_mode(this_->gra, draw_mode_begin);
            graphics_draw_text_std(this_->gra, 16, str, &p);
            g_free(str);
            p.y+=32;
            graphics_draw_mode(this_->gra, draw_mode_end);
        }
    }
    mapset_close(msh);
}

static void navit_redraw_route(struct navit *this_, struct route *route, struct attr *attr) {
    int updated;
    if (attr->type != attr_route_status)
        return;
    updated=attr->u.num;
    if (this_->ready != 3)
        return;
    if (updated != route_status_path_done_new)
        return;
    if (this_->vehicle) {
        if (this_->vehicle->follow_curr == 1)
            return;
        if (this_->vehicle->follow_curr <= this_->vehicle->follow)
            this_->vehicle->follow_curr=this_->vehicle->follow;
    }
    navit_draw(this_);
}

void navit_handle_resize(struct navit *this_, int w, int h) {
    struct map_selection sel;
    int callback=(this_->ready == 1);
    this_->ready |= 2;
    memset(&sel, 0, sizeof(sel));
    this_->w=w;
    this_->h=h;
    sel.u.p_rect.rl.x=w;
    sel.u.p_rect.rl.y=h;
    transform_set_screen_selection(this_->trans, &sel);
    graphics_init(this_->gra);
    graphics_set_rect(this_->gra, &sel.u.p_rect);
    if (callback)
        callback_list_call_attr_1(this_->attr_cbl, attr_graphics_ready, this_);
    if (this_->ready == 3)
        navit_draw_async(this_, 1);
}

static void navit_resize(void *data, int w, int h) {
    struct navit *this=data;
    if (!this->ignore_graphics_events)
        navit_handle_resize(this, w, h);
}

int navit_get_width(struct navit *this_) {
    return this_->w;
}


int navit_get_height(struct navit *this_) {
    return this_->h;
}

static void navit_popup(void *data) {
    struct navit *this_=data;
    popup(this_, 1, &this_->pressed);
    this_->button_timeout=NULL;
    this_->popped=1;
}


/**
 * @brief Sets a flag indicating that the current button event should be ignored by subsequent handlers.
 *
 * Calling this function will set the {@code ignore_button} member to {@code true} and return its previous state.
 * The default handler, {@link navit_handle_button(navit *, int, int, point *, callback *)} calls this function
 * just before the actual event handling core and aborts if the result is {@code true}. In order to prevent
 * multiple handlers from firing on a single event, custom button click handlers should implement the same logic
 * for events they wish to handle.
 *
 * If a handler wishes to pass down an event to other handlers, it must abort without calling this function.
 *
 * @param this_ The navit instance
 * @return {@code true} if the caller should ignore the button event, {@code false} if it should handle it
 */
int navit_ignore_button(struct navit *this_) {
    if (this_->ignore_button)
        return 1;
    this_->ignore_button=1;
    return 0;
}

void navit_ignore_graphics_events(struct navit *this_, int ignore) {
    this_->ignore_graphics_events=ignore;
}

static int navit_restrict_to_range(int value, int min, int max) {
    if (value>max) {
        value = max;
    }
    if (value<min) {
        value = min;
    }
    return value;
}

static void navit_restrict_map_center_to_world_boundingbox(struct transformation *tr, struct coord *new_center) {
    new_center->x = navit_restrict_to_range(new_center->x, WORLD_BOUNDINGBOX_MIN_X, WORLD_BOUNDINGBOX_MAX_X);
    new_center->y = navit_restrict_to_range(new_center->y, WORLD_BOUNDINGBOX_MIN_Y, WORLD_BOUNDINGBOX_MAX_Y);
}

/**
 * @brief Change map center position by translating from "old" to "new".
 */
static void update_transformation(struct transformation *tr, struct point *old, struct point *new) {
    /* Code for rotation was removed in rev. 5252; see Trac #1078. */
    struct coord coord_old,coord_new;
    struct coord center_new,*center_old;
    if (!transform_reverse(tr, old, &coord_old))
        return;
    if (!transform_reverse(tr, new, &coord_new))
        return;
    center_old=transform_get_center(tr);
    center_new.x=center_old->x+coord_old.x-coord_new.x;
    center_new.y=center_old->y+coord_old.y-coord_new.y;
    navit_restrict_map_center_to_world_boundingbox(tr, &center_new);
    dbg(lvl_debug,"change center from 0x%x,0x%x to 0x%x,0x%x", center_old->x, center_old->y, center_new.x, center_new.y);
    transform_set_center(tr, &center_new);
}

void navit_set_timeout(struct navit *this_) {
    struct attr follow;
    follow.type=attr_follow;
    follow.u.num=this_->center_timeout;
    navit_set_attr(this_, &follow);
}

int navit_handle_button(struct navit *this_, int pressed, int button, struct point *p,
                        struct callback *popup_callback) {
    int border=16;

    dbg(lvl_debug,"button %d %s (ignore: %d)",button,pressed?"pressed":"released",this_->ignore_button);
    callback_list_call_attr_4(this_->attr_cbl, attr_button, this_, GINT_TO_POINTER(pressed), GINT_TO_POINTER(button), p);
    if (this_->ignore_button) {
        this_->ignore_button=0;
        return 0;
    }
    if (pressed) {
        this_->pressed=*p;
        this_->last=*p;
        this_->zoomed=0;
        if (button == 1) {
            this_->button_pressed=1;
            this_->moved=0;
            this_->popped=0;
            if (popup_callback)
                this_->button_timeout=event_add_timeout(500, 0, popup_callback);
        }
        if (button == 2)
            navit_set_center_screen(this_, p, 1);
        if (button == 3)
            popup(this_, button, p);
        if (button == 4 && this_->use_mousewheel) {
            this_->zoomed = 1;
            navit_zoom_in(this_, 2, p);
        }
        if (button == 5 && this_->use_mousewheel) {
            this_->zoomed = 1;
            navit_zoom_out(this_, 2, p);
        }
    } else {

        this_->button_pressed=0;
        if (this_->button_timeout) {
            event_remove_timeout(this_->button_timeout);
            this_->button_timeout=NULL;
            if (! this_->moved && ! transform_within_border(this_->trans, p, border)) {
                navit_set_center_screen(this_, p, !this_->zoomed);
            }
        }
        if (this_->motion_timeout) {
            event_remove_timeout(this_->motion_timeout);
            this_->motion_timeout=NULL;
        }
        if (this_->moved) {
            dbg(lvl_debug, "mouse drag (%d, %d)->(%d, %d)", this_->pressed.x, this_->pressed.y, p->x, p->y);
            update_transformation(this_->trans, &this_->pressed, p);
            graphics_draw_drag(this_->gra, NULL);
            transform_copy(this_->trans, this_->trans_cursor);
            graphics_overlay_disable(this_->gra, 0);
            if (!this_->zoomed)
                navit_set_timeout(this_);
            navit_draw(this_);
        } else
            return 1;
    }
    return 0;
}

static void navit_button(void *data, int pressed, int button, struct point *p) {
    struct navit *this=data;
    dbg(lvl_debug,"enter %d %d ignore %d",pressed,button,this->ignore_graphics_events);
    if (!this->ignore_graphics_events) {
        if (! this->popup_callback)
            this->popup_callback=callback_new_1(callback_cast(navit_popup), this);
        navit_handle_button(this, pressed, button, p, this->popup_callback);
    }
}


static void navit_motion_timeout(struct navit *this_) {
    int dx, dy;

    if (this_->drag_bitmap) {
        struct point point;
        point.x=(this_->current.x-this_->pressed.x);
        point.y=(this_->current.y-this_->pressed.y);
        if (graphics_draw_drag(this_->gra, &point)) {
            graphics_overlay_disable(this_->gra, 1);
            graphics_draw_mode(this_->gra, draw_mode_end);
            this_->moved=1;
            this_->motion_timeout=NULL;
            return;
        }
    }
    dx=(this_->current.x-this_->last.x);
    dy=(this_->current.y-this_->last.y);
    if (dx || dy) {
        struct transformation *tr;
        this_->last=this_->current;
        graphics_overlay_disable(this_->gra, 1);
        tr=transform_dup(this_->trans);
        update_transformation(tr, &this_->pressed, &this_->current);
        graphics_draw_cancel(this_->gra, this_->displaylist);
        graphics_displaylist_draw(this_->gra, this_->displaylist, tr, this_->layout_current, this_->graphics_flags|512);
        transform_destroy(tr);
        this_->moved=1;
    }
    this_->motion_timeout=NULL;
    return;
}

void navit_handle_motion(struct navit *this_, struct point *p) {
    int dx, dy;

    if (this_->button_pressed && !this_->popped) {
        dx=(p->x-this_->pressed.x);
        dy=(p->y-this_->pressed.y);
        if (dx < -8 || dx > 8 || dy < -8 || dy > 8) {
            this_->moved=1;
            if (this_->button_timeout) {
                event_remove_timeout(this_->button_timeout);
                this_->button_timeout=NULL;
            }
            this_->current=*p;
            if (! this_->motion_timeout_callback)
                this_->motion_timeout_callback=callback_new_1(callback_cast(navit_motion_timeout), this_);
            if (! this_->motion_timeout)
                this_->motion_timeout=event_add_timeout(this_->drag_bitmap?10:100, 0, this_->motion_timeout_callback);
        }
    }
}

static void navit_motion(void *data, struct point *p) {
    struct navit *this=data;
    if (!this->ignore_graphics_events)
        navit_handle_motion(this, p);
}

static void navit_predraw(struct navit *this_) {
    GList *l;
    struct navit_vehicle *nv;
    transform_copy(this_->trans, this_->trans_cursor);
    l=this_->vehicles;
    while (l) {
        nv=l->data;
        navit_vehicle_draw(this_, nv, NULL);
        l=g_list_next(l);
    }
}

static void navit_scale(struct navit *this_, long scale, struct point *p, int draw) {
    struct coord c1, c2, *center;
    if (scale < this_->zoom_min)
        scale=this_->zoom_min;
    if (scale > this_->zoom_max)
        scale=this_->zoom_max;
    if (p)
        transform_reverse(this_->trans, p, &c1);
    transform_set_scale(this_->trans, scale);
    if (p) {
        transform_reverse(this_->trans, p, &c2);
        center = transform_center(this_->trans);
        center->x += c1.x - c2.x;
        center->y += c1.y - c2.y;
    }
    if (draw)
        navit_draw(this_);
}

/**
 * @brief Automatically adjusts zoom level
 *
 * This function automatically adjusts the current
 * zoom level according to the current speed.
 *
 * @param this_ The navit struct
 * @param center The "immovable" point - i.e. the vehicles position if we're centering on the vehicle
 * @param speed The vehicles speed in meters per second
 * @param dir The direction into which the vehicle moves
 */
static void navit_autozoom(struct navit *this_, struct coord *center, int speed, int draw) {
    struct point pc;
    int distance,w,h;
    double new_scale;
    long scale;

    if (! this_->autozoom_active) {
        return;
    }

    if(this_->autozoom_paused) {
        this_->autozoom_paused--;
        return;
    }

    distance = speed * this_->autozoom_secs;

    transform_get_size(this_->trans, &w, &h);
    transform(this_->trans, transform_get_projection(this_->trans), center, &pc, 1, 0, 0, NULL);
    scale = transform_get_scale(this_->trans);

    /* We make sure that the point we want to see is within a certain range
     * around the vehicle. The radius of this circle is the size of the
     * screen. This doesn't necessarily mean the point is visible because of
     * perspective etc. Quite rough, but should be enough. */

    if (w > h) {
        new_scale = (double)distance / h * 16;
    } else {
        new_scale = (double)distance / w * 16;
    }

    if (abs(new_scale - scale) < 2) {
        return; // Smoothing
    }
    if (new_scale > this_->autozoom_max)
        new_scale=this_->autozoom_max;
    if (new_scale < this_->autozoom_min)
        new_scale=this_->autozoom_min;
    if (new_scale != scale)
        navit_scale(this_, (long)new_scale, &pc, 0);
}

/**
 * Change the current zoom level, zooming closer to the ground
 *
 * @param navit The navit instance
 * @param factor The zoom factor, usually 2
 * @param p The invariant point (if set to NULL, default to center)
 * @returns nothing
 */
void navit_zoom_in(struct navit *this_, int factor, struct point *p) {
    long scale=transform_get_scale(this_->trans)/factor;
    if(this_->autozoom_active) {
        this_->autozoom_paused = 10;
    }
    if (scale < 1)
        scale=1;
    navit_scale(this_, scale, p, 1);
}

/**
 * Change the current zoom level
 *
 * @param navit The navit instance
 * @param factor The zoom factor, usually 2
 * @param p The invariant point (if set to NULL, default to center)
 * @returns nothing
 */
void navit_zoom_out(struct navit *this_, int factor, struct point *p) {
    long scale=transform_get_scale(this_->trans)*factor;
    if(this_->autozoom_active) {
        this_->autozoom_paused = 10;
    }
    navit_scale(this_, scale, p, 1);
}

void navit_zoom_in_cursor(struct navit *this_, int factor) {
    struct point p;
    if (this_->vehicle && this_->vehicle->follow_curr <= 1 && navit_get_cursor_pnt(this_, &p, 0, NULL)) {
        navit_zoom_in(this_, factor, &p);
        this_->vehicle->follow_curr=this_->vehicle->follow;
    } else
        navit_zoom_in(this_, factor, NULL);
}

void navit_zoom_out_cursor(struct navit *this_, int factor) {
    struct point p;
    if (this_->vehicle && this_->vehicle->follow_curr <= 1 && navit_get_cursor_pnt(this_, &p, 0, NULL)) {
        navit_zoom_out(this_, 2, &p);
        this_->vehicle->follow_curr=this_->vehicle->follow;
    } else
        navit_zoom_out(this_, 2, NULL);
}

static int navit_cmd_zoom_in(struct navit *this_) {

    navit_zoom_in_cursor(this_, 2);
    return 0;
}

static int navit_cmd_zoom_out(struct navit *this_) {
    navit_zoom_out_cursor(this_, 2);
    return 0;
}


static void navit_cmd_say(struct navit *this, char *function, struct attr **in, struct attr ***out, int *valid) {
    if (in && in[0] && ATTR_IS_STRING(in[0]->type) && in[0]->u.str)
        navit_say(this, in[0]->u.str);
}

static GHashTable *cmd_int_var_hash = NULL;
static GHashTable *cmd_attr_var_hash = NULL;

/**
 * Store key value pair for the  command system (for int typed values)
 *
 * @param navit The navit instance
 * @param function unused (needed to match command function signature)
 * @param in input attributes in[0] is the key string, in[1] is the integer value to store
 * @param out output attributes, unused
 * @param valid unused
 * @returns nothing
 */
static void navit_cmd_set_int_var(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                  int *valid) {
    char*key;
    struct attr*val;
    if(!cmd_int_var_hash) {
        cmd_int_var_hash = g_hash_table_new(g_str_hash, g_str_equal);
    }

    if ( (in && in[0] && ATTR_IS_STRING(in[0]->type) && in[0]->u.str) &&
            (in && in[1] && ATTR_IS_NUMERIC(in[1]->type))) {
        val = g_new(struct attr,1);
        attr_dup_content(in[1],val);
        key = g_strdup(in[0]->u.str);
        g_hash_table_insert(cmd_int_var_hash, key, val);
    }
}


/**
 * Store key value pair for the  command system (for attr typed values, can be used as opaque handles)
 *
 * @param navit The navit instance
 * @param function unused (needed to match command function signature)
 * @param in input attributes in[0] is the key string, in[1] is the attr* value to store
 * @param out output attributes, unused
 * @param valid unused
 * @returns nothing
 */
static void navit_cmd_set_attr_var(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                   int *valid) {
    char*key;
    struct attr*val;
    if(!cmd_attr_var_hash) {
        cmd_attr_var_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)attr_free);
    }

    if ( (in && in[0] && ATTR_IS_STRING(in[0]->type) && in[0]->u.str) &&
            (in && in[1] )) {
        val = attr_dup(in[1]);
        key = g_strdup(in[0]->u.str);
        g_hash_table_insert(cmd_attr_var_hash, key, val);
    } else {
        dbg(lvl_warning, "Wrong parameters for set_attr_var() command function");
    }
}



/**
 * command to toggle the active state of a named layer of the current layout
 *
 * @param navit The navit instance
 * @param function unused (needed to match command function signature)
 * @param in input attribute in[0] is the name of the layer
 * @param out output unused
 * @param valid unused
 * @returns nothing
 */
static void navit_cmd_toggle_layer(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                   int *valid) {
    if (in && in[0] && ATTR_IS_STRING(in[0]->type) && in[0]->u.str) {
        if(this->layout_current && this->layout_current->layers) {
            GList* layers = this->layout_current->layers;
            while (layers) {
                struct layer*l=layers->data;
                if(l && !strcmp(l->name,in[0]->u.str) ) {
                    l->active ^= 1;
                    navit_draw(this);
                    return;
                }
                layers=g_list_next(layers);
            }
        }
    }
}

/**
 * adds an item with the current coordinate of the vehicle to a named map
 *
 * @param navit The navit instance
 * @param function unused (needed to match command function signature)
 * @param in input attribute in[0] is the name of the map
 * @param out output attribute, 0 on error or the id of the created item on success
 * @param valid unused
 * @returns nothing
 */
static void navit_cmd_map_add_curr_pos(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                       int *valid) {
    struct attr **list = g_new0(struct attr *,2);
    struct attr*val = g_new0(struct attr,1);
    struct mapset* ms;
    struct map_selection sel;
    const int selection_range = 10;
    enum item_type item_type;
    struct item *it;
    struct map* curr_map = NULL;
    struct coord curr_coord;
    struct map_rect *mr;

    //return invalid item on error
    val->type   = attr_none;
    val->u.item  = NULL;
    list[0]     = val;
    list[1]     = NULL;
    *out = list;
    if (
        in && in[0] && ATTR_IS_STRING(in[0]->type) && in[0]->u.str && //map name
        in[1] && ATTR_IS_STRING(in[1]->type) && in[1]->u.str    //item type
    ) {

        if(!(ms=navit_get_mapset(this))) {
            dbg(lvl_error, "Command function map_add_curr_pos(): there is no active mapset");
            return;
        }

        if((item_type = item_from_name(in[1]->u.str))==type_none) {
            dbg(lvl_error, "Command function map_add_curr_pos(): unknown item type");
            return;
        }

        curr_map = mapset_get_map_by_name(ms, in[0]->u.str);

        //no map with the given name found
        if( ! curr_map) {
            dbg(lvl_error, "Command function map_add_curr_pos(): map not found");
            return;
        }

        if(this->vehicle && this->vehicle->vehicle ) {
            struct attr pos_attr;
            if(vehicle_get_attr(this->vehicle->vehicle,attr_position_coord_geo,&pos_attr,NULL)) {
                transform_from_geo(projection_mg, pos_attr.u.coord_geo, &curr_coord);
            } else {
                dbg(lvl_error, "Command function map_add_curr_pos(): vehicle position is not accessible");
                return;
            }
        } else {
            dbg(lvl_error, "Command function map_add_curr_pos(): no vehicle");
            return;
        }

        sel.next=NULL;
        sel.order=18;
        sel.range.min=type_none;
        sel.range.max=type_tec_common;
        sel.u.c_rect.lu.x=curr_coord.x-selection_range;
        sel.u.c_rect.lu.y=curr_coord.y+selection_range;
        sel.u.c_rect.rl.x=curr_coord.x+selection_range;
        sel.u.c_rect.rl.y=curr_coord.y-selection_range;

        mr = map_rect_new(curr_map, &sel);
        if(mr) {

            it = map_rect_create_item( mr, item_type);
            if (it) {
                struct attr attr;
                attr.type=attr_type_item_begin;
                attr.u.item=it;
                attr_dup_content(&attr,val);
                item_coord_set(it,&curr_coord, 1, change_mode_modify);
            }
        }
        map_rect_destroy(mr);
    }
}

/**
 * sets an attribute (name value pair) of a map item specified by map name and item id
 *
 * @param navit The navit instance
 * @param function unused (needed to match command function signature)
 * @param in input attribute in[0] - name of the map  ; in[1] - item  ; in[2] - attr name ; in[3] - attr value
 * @param out output attribute, 0 on error, 1 on success
 * @param valid unused
 * @returns nothing
 */
static void navit_cmd_map_item_set_attr(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                        int *valid) {
    if (
        in && in[0] && ATTR_IS_STRING(in[0]->type) && in[0]->u.str  &&//map name
        in[1] && ATTR_IS_ITEM(in[1]->type)   && in[2]->u.item &&//item
        in[2] && ATTR_IS_STRING(in[2]->type) && in[2]->u.str && //attr_type str
        in[3] && ATTR_IS_STRING(in[3]->type) && in[3]->u.str    //attr_value str
    ) {
        struct attr attr_to_set;
        struct map* curr_map = NULL;
        struct mapset *ms;
        struct item *it;
        struct map_rect *mr;

        if(ATTR_IS_STRING(attr_from_name(in[2]->u.str))) {
            attr_to_set.u.str = in[3]->u.str;
            attr_to_set.type = attr_from_name(in[2]->u.str);
        } else if(ATTR_IS_INT(attr_from_name(in[2]->u.str))) {
            attr_to_set.u.num = atoi(in[3]->u.str);
            attr_to_set.type = attr_from_name(in[2]->u.str);
        } else if(ATTR_IS_DOUBLE(attr_from_name(in[2]->u.str))) {
            double* val = g_new0(double,1);
            *val = atof(in[3]->u.str);
            attr_to_set.u.numd = val;
            attr_to_set.type = attr_from_name(in[2]->u.str);
        }

        ms = navit_get_mapset(this);

        curr_map = mapset_get_map_by_name(ms, in[0]->u.str);

        if( ! curr_map) {
            return;
        }

        mr=map_rect_new(curr_map,NULL);
        it=in[1]->u.item;
        it=map_rect_get_item_byid(mr,it->id_hi,it->id_lo);

        if(it) {
            item_attr_set(it, &attr_to_set, change_mode_modify);
        }
        map_rect_destroy(mr);
    } else {
        dbg(lvl_debug,"Error in command function item_set_attr()");
        dbg(lvl_debug,"Command function item_set_attr(): map cond:       %d",(in[0] && ATTR_IS_STRING(in[0]->type)
                && in[0]->u.str)?1:0);
        dbg(lvl_debug,"Command function item_set_attr(): item cond:      %d",(in[1] && ATTR_IS_ITEM(in[1]->type))?1:0);
        dbg(lvl_debug,"Command function item_set_attr(): attr type cond: %d",(in[2] && ATTR_IS_STRING(in[2]->type)
                && in[2]->u.str)?1:0);
        dbg(lvl_debug,"Command function item_set_attr(): attr val cond:  %d",(in[3] && ATTR_IS_STRING(in[3]->type)
                && in[3]->u.str)?1:0);
    }
}

/**
 * Get attr variable given a key string for the command system (for opaque usage)
 *
 * @param navit The navit instance
 * @param function unused (needed to match command function signature)
 * @param in input attribute in[0] is the key string
 * @param out output attribute, the attr for the given key string if exists or NULL
 * @param valid unused
 * @returns nothing
 */
static void navit_cmd_get_attr_var(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                   int *valid) {
    struct attr **list = g_new0(struct attr *,2);
    list[1] = NULL;
    *out = list;
    if(!cmd_attr_var_hash) {
        struct attr*val = g_new0(struct attr,1);
        val->type   = attr_type_item_begin;
        val->u.item = NULL;
        list[0]     = val;
        return;
    }
    if (in && in[0] && ATTR_IS_STRING(in[0]->type) && in[0]->u.str) {
        struct attr*ret = g_hash_table_lookup(cmd_attr_var_hash, in[0]->u.str);
        if(ret) {
            list[0] = attr_dup(ret);
        } else {
            struct attr*val = g_new0(struct attr,1);
            val->type   = attr_type_int_begin;
            val->u.item = NULL;
            list[0]   = val;
        }
    }
}


/**
 * Get value given a key string for the command system
 *
 * @param navit The navit instance
 * @param function unused (needed to match command function signature)
 * @param in input attribute in[0] is the key string
 * @param out output attribute, the value for the given key string if exists or 0
 * @param valid unused
 * @returns nothing
 */
static void navit_cmd_get_int_var(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                  int *valid) {
    struct attr **list = g_new0(struct attr *,2);
    list[1] = NULL;
    *out = list;
    if(!cmd_int_var_hash) {
        struct attr*val = g_new0(struct attr,1);
        val->type   = attr_type_int_begin;
        val->u.num  = 0;
        list[0]     = val;
        return;
    }
    if (in && in[0] && ATTR_IS_STRING(in[0]->type) && in[0]->u.str) {
        struct attr*ret = g_hash_table_lookup(cmd_int_var_hash, in[0]->u.str);
        if(ret) {
            list[0] = attr_dup(ret);
        } else {
            struct attr*val = g_new0(struct attr,1);
            val->type   = attr_type_int_begin;
            val->u.num  = 0;
            list[0]   = val;
        }
    }
}

GList *cmd_int_var_stack = NULL;

/**
 * Push an integer to the stack for the command system
 *
 * @param navit The navit instance
 * @param function unused (needed to match command function signature)
 * @param in input attribute in[0] is the integer attibute to push
 * @param out output attributes, unused
 * @param valid unused
 * @returns nothing
 */
static void navit_cmd_push_int(struct navit *this, char *function, struct attr **in, struct attr ***out, int *valid) {
    if (in && in[0] && ATTR_IS_NUMERIC(in[0]->type)) {
        struct attr*val = g_new(struct attr,1);
        attr_dup_content(in[0],val);
        cmd_int_var_stack = g_list_prepend(cmd_int_var_stack, val);
    }
}

/**
 * Pop an integer from the command system's integer stack
 *
 * @param navit The navit instance
 * @param function unused (needed to match command function signature)
 * @param in input attributes unused
 * @param out output attribute, the value popped if stack isn't empty or 0
 * @param valid unused
 * @returns nothing
 */
static void navit_cmd_pop_int(struct navit *this, char *function, struct attr **in, struct attr ***out, int *valid) {
    struct attr **list = g_new0(struct attr *,2);
    if(!cmd_int_var_stack) {
        struct attr*val = g_new0(struct attr,1);
        val->type = attr_type_int_begin;
        val->u.num  = 0;
        list[0]   = val;
    } else {
        list[0] = cmd_int_var_stack->data;
        cmd_int_var_stack = g_list_remove_link(cmd_int_var_stack,cmd_int_var_stack);
    }
    list[1] = NULL;
    *out = list;
}

/**
 * Get current size of command system's integer stack
 *
 * @param navit The navit instance
 * @param function unused (needed to match command function signature)
 * @param in input attributes unused
 * @param out output attribute, the size of stack
 * @param valid unused
 * @returns nothing
 */
static void navit_cmd_int_stack_size(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                     int *valid) {
    struct attr **list;
    struct attr *attr  = g_new0(struct attr,1);
    attr->type  = attr_type_int_begin;
    if(!cmd_int_var_stack) {
        attr->u.num = 0;
    } else {
        attr->u.num = g_list_length(cmd_int_var_stack);
    }
    list = g_new0(struct attr *,2);
    list[0] = attr;
    list[1] = NULL;
    *out = list;
    cmd_int_var_stack = g_list_remove_link(cmd_int_var_stack,cmd_int_var_stack);
}

static struct attr ** navit_get_coord(struct navit *this, struct attr **in, struct pcoord *pc) {
    if (!in)
        return NULL;
    if (!in[0])
        return NULL;
    pc->pro = transform_get_projection(this->trans);
    if (ATTR_IS_STRING(in[0]->type)) {
        struct coord c;
        coord_parse(in[0]->u.str, pc->pro, &c);
        pc->x=c.x;
        pc->y=c.y;
        in++;
    } else if (ATTR_IS_COORD(in[0]->type)) {
        pc->x=in[0]->u.coord->x;
        pc->y=in[0]->u.coord->y;
        in++;
    } else if (ATTR_IS_PCOORD(in[0]->type)) {
        *pc=*in[0]->u.pcoord;
        in++;
    } else if (in[1] && in[2] && ATTR_IS_INT(in[0]->type) && ATTR_IS_INT(in[1]->type) && ATTR_IS_INT(in[2]->type)) {
        pc->pro=in[0]->u.num;
        pc->x=in[1]->u.num;
        pc->y=in[2]->u.num;
        in+=3;
    } else if (in[1] && ATTR_IS_INT(in[0]->type) && ATTR_IS_INT(in[1]->type)) {
        pc->x=in[0]->u.num;
        pc->y=in[1]->u.num;
        in+=2;
    } else
        return NULL;
    return in;
}

static void navit_cmd_set_destination(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                      int *valid) {
    struct pcoord pc;
    char *description=NULL;
    in=navit_get_coord(this, in, &pc);
    if (!in)
        return;
    if (in[0] && ATTR_IS_STRING(in[0]->type))
        description=in[0]->u.str;
    navit_set_destination(this, &pc, description, 1);
}


static void navit_cmd_route_remove_next_waypoint(struct navit *this, char *function, struct attr **in,
        struct attr ***out,
        int *valid) {
    navit_remove_waypoint(this);
}


static void navit_cmd_route_remove_last_waypoint(struct navit *this, char *function, struct attr **in,
        struct attr ***out,
        int *valid) {
    navit_remove_nth_waypoint(this, navit_get_destination_count(this)-1);
}


static void navit_cmd_set_center(struct navit *this, char *function, struct attr **in, struct attr ***out, int *valid) {
    struct pcoord pc;
    int set_timeout=0;
    in=navit_get_coord(this, in, &pc);
    if (!in)
        return;
    if(in[0] && ATTR_IS_INT(in[0]->type))
        set_timeout=in[0]->u.num!=0;
    navit_set_center(this, &pc, set_timeout);
}


static void navit_cmd_set_position(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                   int *valid) {
    struct pcoord pc;
    in=navit_get_coord(this, in, &pc);
    if (!in)
        return;
    navit_set_position(this, &pc);
}


static void navit_cmd_fmt_coordinates(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                      int *valid) {
    struct attr attr;
    attr.type=attr_type_string_begin;
    attr.u.str="Fix me";
    if (out) {
        *out=attr_generic_add_attr(*out, &attr);
    }
}

/**
 * Join several string attributes into one
 *
 * @param navit The navit instance
 * @param function unused (needed to match command function signature)
 * @param in input attributes in[0] - separator, in[1..] - attributes to join
 * @param out output attribute joined attribute as string
 * @param valid unused
 * @returns nothing
 */
static void navit_cmd_strjoin(struct navit *this, char *function, struct attr **in, struct attr ***out, int *valid) {
    struct attr attr;
    gchar *ret, *sep;
    int i;
    attr.type=attr_type_string_begin;
    attr.u.str=NULL;
    if(in[0] && in[1]) {
        sep=attr_to_text(in[0],NULL,1);
        ret=attr_to_text(in[1],NULL,1);
        for(i=2; in[i]; i++) {
            gchar *in_i=attr_to_text(in[i],NULL,1);
            gchar *r=g_strjoin(sep,ret,in_i,NULL);
            g_free(in_i);
            g_free(ret);
            ret=r;
        }
        g_free(sep);
        attr.u.str=ret;
        if(out) {
            *out=attr_generic_add_attr(*out, &attr);
        }
        g_free(ret);
    }
}

/**
 * Call external program
 *
 * @param navit The navit instance
 * @param function unused (needed to match command function signature)
 * @param in input attributes in[0] - name of executable, in[1..] - parameters
 * @param out output attribute unused
 * @param valid unused
 * @returns nothing
 */
static void navit_cmd_spawn(struct navit *this, char *function, struct attr **in, struct attr ***out, int *valid) {
    int i,j, nparms, nvalid;
    char ** argv=NULL;
    struct spawn_process_info *pi;

    nparms=0;
    nvalid=0;
    if(in) {
        while(in[nparms]) {
            if (in[nparms]->type!=attr_none)
                nvalid++;
            nparms++;
        }
    }

    if(nvalid>0) {
        argv=g_new(char*,nvalid+1);
        for(i=0,j=0; in[i]; i++) {
            if(in[i]->type!=attr_none ) {
                argv[j++]=attr_to_text(in[i],NULL,1);
            } else {
                dbg(lvl_debug,"Parameter #%i is attr_none - skipping",i);
            }
        }
        argv[j]=NULL;
        pi=spawn_process(argv);

        // spawn_process() testing suite - uncomment following code to test.
        //sleep(3);
        // example of non-blocking wait
        //int st=spawn_process_check_status(pi,0);dbg(lvl_debug,"status %i",st);
        // example of blocking wait
        //st=spawn_process_check_status(pi,1);dbg(lvl_debug,"status %i",st);
        // example of wait after process is finished and status is
        // already tested
        //st=spawn_process_check_status(pi,1);dbg(lvl_debug,"status %i",st);
        // example of wait after process is finished and status is
        // already tested - unblocked
        //st=spawn_process_check_status(pi,0);dbg(lvl_debug,"status %i",st);

        // End testing suite
        spawn_process_info_free(pi);
        for(i=0; argv[i]; i++)
            g_free(argv[i]);
        g_free(argv);
    }
}


static struct command_table commands[] = {
    {"zoom_in",command_cast(navit_cmd_zoom_in)},
    {"zoom_out",command_cast(navit_cmd_zoom_out)},
    {"zoom_to_route",command_cast(navit_cmd_zoom_to_route)},
    {"say",command_cast(navit_cmd_say)},
    {"set_center",command_cast(navit_cmd_set_center)},
    {"set_center_cursor",command_cast(navit_cmd_set_center_cursor)},
    {"set_destination",command_cast(navit_cmd_set_destination)},
    {"set_position",command_cast(navit_cmd_set_position)},
    {"route_remove_next_waypoint",command_cast(navit_cmd_route_remove_next_waypoint)},
    {"route_remove_last_waypoint",command_cast(navit_cmd_route_remove_last_waypoint)},
    {"set_position",command_cast(navit_cmd_set_position)},
    {"announcer_toggle",command_cast(navit_cmd_announcer_toggle)},
    {"fmt_coordinates",command_cast(navit_cmd_fmt_coordinates)},
    {"set_int_var",command_cast(navit_cmd_set_int_var)},
    {"get_int_var",command_cast(navit_cmd_get_int_var)},
    {"push_int",command_cast(navit_cmd_push_int)},
    {"pop_int",command_cast(navit_cmd_pop_int)},
    {"int_stack_size",command_cast(navit_cmd_int_stack_size)},
    {"toggle_layer",command_cast(navit_cmd_toggle_layer)},
    {"strjoin",command_cast(navit_cmd_strjoin)},
    {"spawn",command_cast(navit_cmd_spawn)},
    {"map_add_curr_pos",command_cast(navit_cmd_map_add_curr_pos)},
    {"map_item_set_attr",command_cast(navit_cmd_map_item_set_attr)},
    {"set_attr_var",command_cast(navit_cmd_set_attr_var)},
    {"get_attr_var",command_cast(navit_cmd_get_attr_var)},
    {"switch_layout_day_night",command_cast(navit_cmd_switch_layout_day_night)},
};

void navit_command_add_table(struct navit*this_, struct command_table *commands, int count) {
    command_add_table(this_->attr_cbl, commands, count, this_);
}

struct navit *
navit_new(struct attr *parent, struct attr **attrs) {
    struct navit *this_=g_new0(struct navit, 1);
    struct pcoord center;
    struct coord co;
    struct coord_geo g;
    enum projection pro=projection_mg;
    int zoom = 256;
    g.lat=53.13;
    g.lng=11.70;

    this_->func=&navit_func;
    navit_object_ref((struct navit_object *)this_);
    this_->attrs=attr_list_dup(attrs);
    this_->self.type=attr_navit;
    this_->self.u.navit=this_;
    this_->attr_cbl=callback_list_new();

    this_->orientation=-1;
    this_->tracking_flag=1;
    this_->recentdest_count=10;
    this_->osd_configuration=-1;

    this_->center_timeout = 10;
    this_->use_mousewheel = 1;
    this_->autozoom_secs = 10;
    this_->autozoom_min = 7;
    this_->autozoom_active = 0;
    this_->autozoom_paused = 0;
    this_->zoom_min = 1;
    this_->zoom_max = 2097152;
    this_->autozoom_max = this_->zoom_max;
    this_->follow_cursor = 1;
    this_->radius = 30;
    this_->border = 16;
    this_->auto_switch = TRUE;

    transform_from_geo(pro, &g, &co);
    center.x=co.x;
    center.y=co.y;
    center.pro = pro;
    this_->trans = transform_new(&center, zoom, (this_->orientation != -1) ? this_->orientation : 0);
    this_->trans_cursor = transform_new(&center, zoom, (this_->orientation != -1) ? this_->orientation : 0);

    this_->bookmarks=bookmarks_new(&this_->self, NULL, this_->trans);

    this_->prevTs=0;

    for (; *attrs; attrs++) {
        navit_set_attr_do(this_, *attrs, 1);
    }
    this_->displaylist=graphics_displaylist_new();
    command_add_table(this_->attr_cbl, commands, sizeof(commands)/sizeof(struct command_table), this_);

    this_->messages = messagelist_new(attrs);

    dbg(lvl_debug,"return %p",this_);

    return this_;
}

static int navit_set_gui(struct navit *this_, struct gui *gui) {
    if (this_->gui)
        return 0;
    this_->gui=gui;
    if (gui_has_main_loop(this_->gui)) {
        if (! main_loop_gui) {
            main_loop_gui=this_->gui;
        } else {
            dbg(lvl_error,"gui with main loop already active, ignoring this instance");
            return 0;
        }
    }
    return 1;
}

void navit_add_message(struct navit *this_, const char *message) {
    message_new(this_->messages, message);
}

struct message
*navit_get_messages(struct navit *this_) {
    return message_get(this_->messages);
}

static int navit_set_graphics(struct navit *this_, struct graphics *gra) {
    if (this_->gra)
        return 0;
    this_->gra=gra;
    this_->resize_callback=callback_new_attr_1(callback_cast(navit_resize), attr_resize, this_);
    graphics_add_callback(gra, this_->resize_callback);
    this_->button_callback=callback_new_attr_1(callback_cast(navit_button), attr_button, this_);
    graphics_add_callback(gra, this_->button_callback);
    this_->motion_callback=callback_new_attr_1(callback_cast(navit_motion), attr_motion, this_);
    graphics_add_callback(gra, this_->motion_callback);
    this_->predraw_callback=callback_new_attr_1(callback_cast(navit_predraw), attr_predraw, this_);
    graphics_add_callback(gra, this_->predraw_callback);
    return 1;
}

struct graphics *
navit_get_graphics(struct navit *this_) {
    return this_->gra;
}

struct vehicleprofile *
navit_get_vehicleprofile(struct navit *this_) {
    return this_->vehicleprofile;
}

GList *navit_get_vehicleprofiles(struct navit *this_) {
    return this_->vehicleprofiles;
}

static void navit_projection_set(struct navit *this_, enum projection pro, int draw) {
    struct coord_geo g;
    struct coord *c;

    c=transform_center(this_->trans);
    transform_to_geo(transform_get_projection(this_->trans), c, &g);
    transform_set_projection(this_->trans, pro);
    transform_from_geo(pro, &g, c);
    if (draw)
        navit_draw(this_);
}

static void navit_mark_navigation_stopped(char *former_destination_file) {
    FILE *f;
    f=fopen(former_destination_file, "a");
    if (f) {
        fprintf(f,"%s", TEXTFILE_COMMENT_NAVI_STOPPED);
        fclose(f);
    } else {
        dbg(lvl_error, "Error setting mark in destination file %s: %s", former_destination_file, strerror(errno));
    }
}


/**
 * Start or add a given set of coordinates for route computing
 *
 * @param navit The navit instance
 * @param c The coordinate to start routing to
 * @param description A label which allows the user to later identify this destination in the former destinations selection
 * @param async Set to 1 to do route calculation asynchronously
 * @return nothing
 */
void navit_set_destination(struct navit *this_, struct pcoord *c, const char *description, int async) {
    char *destination_file;
    destination_file = bookmarks_get_destination_file(TRUE);
    if (c) {
        this_->destination=*c;
        this_->destination_valid=1;

        dbg(lvl_debug, "c=(%i,%i)", c->x,c->y);
        bookmarks_append_destinations(this_->former_destination, destination_file, c, 1, type_former_destination, description,
                                      this_->recentdest_count);
    } else {
        this_->destination_valid=0;
        bookmarks_append_destinations(this_->former_destination, destination_file, NULL, 0, type_former_destination, NULL,
                                      this_->recentdest_count);
        navit_mark_navigation_stopped(destination_file);
    }
    g_free(destination_file);

    callback_list_call_attr_0(this_->attr_cbl, attr_destination);

    if (this_->route) {
        struct attr attr;
        int dstcount;
        struct pcoord *pc;

        navit_get_attr(this_, attr_waypoints_flag, &attr, NULL);
        if (this_->waypoints_flag==0 || route_get_destination_count(this_->route)==0) {
            route_set_destination(this_->route, c, async);
        } else {
            route_append_destination(this_->route, c, async);
        }

        dstcount=route_get_destination_count(this_->route);
        if(dstcount>0) {
            destination_file = bookmarks_get_destination_file(TRUE);
            pc=g_new(struct pcoord,dstcount);
            route_get_destinations(this_->route,pc,dstcount);
            bookmarks_append_destinations(this_->former_destination, destination_file, pc, dstcount, type_former_itinerary,
                                          description, this_->recentdest_count);
            g_free(pc);
            g_free(destination_file);
        }

        if (this_->ready == 3 && !(this_->flags & 4))
            navit_draw(this_);
    }
}

/**
 * Add destination description to the recent dest file. Doesn't start routing.
 *
 * @param navit The navit instance
 * @param c The coordinate to start routing to
 * @param description A label which allows the user to later identify this destination in the former destinations selection
 * @returns nothing
 */
void navit_add_destination_description(struct navit *this_, struct pcoord *c, const char *description) {
    char *destination_file;
    if (c) {
        destination_file = bookmarks_get_destination_file(TRUE);
        bookmarks_append_destinations(this_->former_destination, destination_file, c, 1, type_former_destination, description,
                                      this_->recentdest_count);
        g_free(destination_file);
    }
}


/**
 * Start the route computing to a given set of coordinates including waypoints
 *
 * @param this_ The navit instance
 * @param c The coordinate to start routing to
 * @param description A label which allows the user to later identify this destination in the former destinations selection
 * @param async If routing should be done asynchronously
 * @returns nothing
 */
void navit_set_destinations(struct navit *this_, struct pcoord *c, int count, const char *description, int async) {
    char *destination_file;
    if (c && count) {
        this_->destination=c[count-1];
        this_->destination_valid=1;

        destination_file = bookmarks_get_destination_file(TRUE);
        bookmarks_append_destinations(this_->former_destination, destination_file, c, count, type_former_itinerary, description,
                                      this_->recentdest_count);
        g_free(destination_file);
    } else
        this_->destination_valid=0;
    callback_list_call_attr_0(this_->attr_cbl, attr_destination);
    if (this_->route) {
        route_set_destinations(this_->route, c, count, async);

        if (this_->ready == 3)
            navit_draw(this_);
    }
}

int navit_get_destinations(struct navit *this_, struct pcoord *pc, int count) {
    if(!this_->route)
        return 0;
    return route_get_destinations(this_->route, pc, count);

}

int navit_get_destination_count(struct navit *this_) {
    if(!this_->route)
        return 0;
    return route_get_destination_count(this_->route);
}

char* navit_get_destination_description(struct navit *this_, int n) {
    if(!this_->route)
        return NULL;
    return route_get_destination_description(this_->route, n);
}

void navit_remove_nth_waypoint(struct navit *this_, int n) {
    if(!this_->route)
        return;
    if (route_get_destination_count(this_->route)>1) {
        route_remove_nth_waypoint(this_->route, n);
    } else {
        navit_set_destination(this_, NULL, NULL, 0);
    }
}

void navit_remove_waypoint(struct navit *this_) {
    if(!this_->route)
        return;
    if (route_get_destination_count(this_->route)>1) {
        route_remove_waypoint(this_->route);
    } else {
        navit_set_destination(this_, NULL, NULL, 0);
    }
}

/**
 * @brief Checks if a route is calculated
 *
 * This function checks if a route is calculated.
 *
 * @param this_ The navit struct whose route should be checked.
 * @return True if the route is set, false otherwise.
 */
int navit_check_route(struct navit *this_) {
    if (this_->route) {
        return route_get_path_set(this_->route);
    }

    return 0;
}

static int navit_former_destinations_active(struct navit *this_) {
    char *destination_file_name = bookmarks_get_destination_file(FALSE);
    FILE *destination_file;
    int active=0;
    char lastline[100];
    destination_file=fopen(destination_file_name,"r");
    if (destination_file) {
        while(fgets(lastline, sizeof(lastline), destination_file));
        fclose(destination_file);
        if ((lastline != NULL) && (strcmp(lastline, TEXTFILE_COMMENT_NAVI_STOPPED))) {
            active=1;
        }
    }
    g_free(destination_file_name);
    return active;
}


struct map* read_former_destinations_from_file() {
    struct attr type, data, no_warn, flags, *attrs[5];
    char *destination_file = bookmarks_get_destination_file(FALSE);
    struct map *m;

    type.type=attr_type;
    type.u.str="textfile";

    data.type=attr_data;
    data.u.str=destination_file;

    no_warn.type=attr_no_warning_if_map_file_missing;
    no_warn.u.num=1;

    flags.type=attr_flags;
    flags.u.num=1;

    attrs[0]=&type;
    attrs[1]=&data;
    attrs[2]=&flags;
    attrs[3]=&no_warn;
    attrs[4]=NULL;

    m=map_new(NULL, attrs);
    g_free(destination_file);
    return m;
}

static void navit_add_former_destinations_from_file(struct navit *this_) {
    struct item *item;
    int i,valid=0,count=0,maxcount=1;
    struct coord *c=g_new(struct coord, maxcount);
    struct pcoord *pc;
    struct map_rect *mr;

    this_->former_destination=read_former_destinations_from_file();
    if (!this_->route || !navit_former_destinations_active(this_) || !this_->vehicle)
        return;
    mr=map_rect_new(this_->former_destination, NULL);
    while ((item=map_rect_get_item(mr))) {
        if (item->type == type_former_itinerary || item->type == type_former_itinerary_part) {
            count=item_coord_get(item, c, maxcount);
            while(count==maxcount) {
                maxcount*=2;
                c=g_realloc(c, sizeof(struct coord)*maxcount);
                count+=item_coord_get(item, &c[count], maxcount-count);
            }
            if(count)
                valid=1;
        }
    }
    map_rect_destroy(mr);
    if (valid && count > 0) {
        pc=g_new(struct pcoord, count);
        for (i = 0 ; i < count ; i++) {
            pc[i].pro=map_projection(this_->former_destination);
            pc[i].x=c[i].x;
            pc[i].y=c[i].y;
        }
        if (count == 1)
            route_set_destination(this_->route, &pc[0], 1);
        else
            route_set_destinations(this_->route, pc, count, 1);
        this_->destination=pc[count-1];
        this_->destination_valid=1;
        g_free(pc);
    }
    g_free(c);
}


void navit_textfile_debug_log(struct navit *this_, const char *fmt, ...) {
    va_list ap;
    char *str1,*str2;
    va_start(ap, fmt);
    if (this_->textfile_debug_log && this_->vehicle) {
        str1=g_strdup_vprintf(fmt, ap);
        str2=g_strdup_printf("0x%x 0x%x%s%s\n", this_->vehicle->coord.x, this_->vehicle->coord.y, strlen(str1) ? " " : "",
                             str1);
        log_write(this_->textfile_debug_log, str2, strlen(str2), 0);
        g_free(str2);
        g_free(str1);
    }
    va_end(ap);
}

void navit_textfile_debug_log_at(struct navit *this_, struct pcoord *pc, const char *fmt, ...) {
    va_list ap;
    char *str1,*str2;
    va_start(ap, fmt);
    if (this_->textfile_debug_log && this_->vehicle) {
        str1=g_strdup_vprintf(fmt, ap);
        str2=g_strdup_printf("0x%x 0x%x%s%s\n", pc->x, pc->y, strlen(str1) ? " " : "", str1);
        log_write(this_->textfile_debug_log, str2, strlen(str2), 0);
        g_free(str2);
        g_free(str1);
    }
    va_end(ap);
}

void navit_say(struct navit *this_, const char *text) {
    struct attr attr;
    if(this_->speech) {
        if (!speech_get_attr(this_->speech, attr_active, &attr, NULL))
            attr.u.num = 1;
        dbg(lvl_debug, "this_.speech->active %ld", attr.u.num);
        if(attr.u.num)
            speech_say(this_->speech, text);
    }
}

/**
 * @brief Toggles the navigation announcer for navit
 * @param this_ The navit object
 */
static void navit_cmd_announcer_toggle(struct navit *this_) {
    struct attr attr, speechattr;

    // search for the speech attribute
    if(!navit_get_attr(this_, attr_speech, &speechattr, NULL))
        return;
    // find out if the corresponding attribute attr_active has been set
    if(speech_get_attr(speechattr.u.speech, attr_active, &attr, NULL)) {
        // flip it then...
        attr.u.num = !attr.u.num;
    } else {
        // otherwise disable it because voice is enabled by default
        attr.type = attr_active;
        attr.u.num = 0;
    }

    // apply the new state
    if(!speech_set_attr(speechattr.u.speech, &attr))
        return;

    // announce that the speech attribute has changed
    callback_list_call_attr_1(this_->attr_cbl, attr_speech, this_);
}

void navit_speak(struct navit *this_) {
    struct navigation *nav=this_->navigation;
    struct map *map=NULL;
    struct map_rect *mr=NULL;
    struct item *item;
    struct attr attr;

    if (!speech_get_attr(this_->speech, attr_active, &attr, NULL))
        attr.u.num = 1;
    dbg(lvl_debug, "this_.speech->active %ld", attr.u.num);
    if(!attr.u.num)
        return;

    if (nav)
        map=navigation_get_map(nav);
    if (map)
        mr=map_rect_new(map, NULL);
    if (mr) {
        while ((item=map_rect_get_item(mr)) && (item->type == type_nav_position || item->type == type_nav_none));
        if (item && item_attr_get(item, attr_navigation_speech, &attr)) {
            if (*attr.u.str != '\0') {
                speech_say(this_->speech, attr.u.str);
                navit_add_message(this_, attr.u.str);
            }
            navit_textfile_debug_log(this_, "type=announcement label=\"%s\"", attr.u.str);
        }
        map_rect_destroy(mr);
    }
}

static void navit_window_roadbook_update(struct navit *this_) {
    struct navigation *nav=this_->navigation;
    struct map *map=NULL;
    struct map_rect *mr=NULL;
    struct item *item;
    struct attr attr;
    struct param_list param[5];
    int secs;

    /* Respect the Imperial attribute as we enlighten the user. */
    int imperial = FALSE;  /* default to using metric measures. */
    if (navit_get_attr(this_, attr_imperial, &attr, NULL))
        imperial=attr.u.num;

    dbg(lvl_debug,"enter");
    datawindow_mode(this_->roadbook_window, 1);
    if (nav)
        map=navigation_get_map(nav);
    if (map)
        mr=map_rect_new(map, NULL);
    dbg(lvl_debug,"nav=%p map=%p mr=%p", nav, map, mr);
    if (mr) {
        dbg(lvl_debug,"while loop");
        while ((item=map_rect_get_item(mr))) {
            dbg(lvl_debug,"item=%p", item);
            attr.u.str=NULL;
            if (item->type != type_nav_position) {
                item_attr_get(item, attr_navigation_long, &attr);
                if (attr.u.str == NULL) {
                    continue;
                }
                dbg(lvl_info, "Command='%s'", attr.u.str);
                param[0].value=g_strdup(attr.u.str);
            } else
                param[0].value=_("Position");
            param[0].name=_("Command");

            /* Distance to the next maneuver. */
            item_attr_get(item, attr_length, &attr);
            dbg(lvl_info, "Length=%ld in meters", attr.u.num);
            param[1].name=_("Length");

            if ( attr.u.num >= 2000 ) {
                param[1].value=g_strdup_printf("%5.1f %s",
                                               imperial == TRUE ? (float)attr.u.num * METERS_TO_MILES : (float)attr.u.num / 1000,
                                               imperial == TRUE ? _("mi") : _("km")
                                              );
            } else {
                param[1].value=g_strdup_printf("%7.0f %s",
                                               imperial == TRUE ? (attr.u.num * FEET_PER_METER) : attr.u.num,
                                               imperial == TRUE ? _("feet") : _("m")
                                              );
            }

            /* Time to next maneuver. */
            item_attr_get(item, attr_time, &attr);
            dbg(lvl_info, "Time=%ld", attr.u.num);
            secs=attr.u.num/10;
            param[2].name=_("Time");
            if ( secs >= 3600 ) {
                param[2].value=g_strdup_printf("%d:%02d:%02d",secs / 60, ( secs / 60 ) % 60, secs % 60);
            } else {
                param[2].value=g_strdup_printf("%d:%02d",secs / 60, secs % 60);
            }

            /* Distance from next maneuver to destination. */
            item_attr_get(item, attr_destination_length, &attr);
            dbg(lvl_info, "Destlength=%ld in meters.", attr.u.num);
            param[3].name=_("Destination Length");
            if ( attr.u.num >= 2000 ) {
                param[3].value=g_strdup_printf("%5.1f %s",
                                               imperial == TRUE ? (float)attr.u.num * METERS_TO_MILES : (float)attr.u.num / 1000,
                                               imperial == TRUE ? _("mi") : _("km")
                                              );
            } else {
                param[3].value=g_strdup_printf("%7.0f %s",
                                               imperial == TRUE ? (attr.u.num * FEET_PER_METER) : attr.u.num,
                                               imperial == TRUE ? _("feet") : _("m")
                                              );
            }

            /* Time from next maneuver to destination. */
            item_attr_get(item, attr_destination_time, &attr);
            dbg(lvl_info, "Desttime=%ld", attr.u.num);
            secs=attr.u.num/10;
            param[4].name=_("Destination Time");
            if ( secs >= 3600 ) {
                param[4].value=g_strdup_printf("%d:%02d:%02d",secs / 3600, (secs / 60 ) % 60, secs % 60);
            } else {
                param[4].value=g_strdup_printf("%d:%02d",secs / 60, secs % 60);
            }
            datawindow_add(this_->roadbook_window, param, 5);
        }
        map_rect_destroy(mr);
    }
    datawindow_mode(this_->roadbook_window, 0);
}

void navit_window_roadbook_destroy(struct navit *this_) {
    dbg(lvl_debug, "enter");
    navigation_unregister_callback(this_->navigation, attr_navigation_long, this_->roadbook_callback);
    callback_destroy(this_->roadbook_callback);
    this_->roadbook_window=NULL;
    this_->roadbook_callback=NULL;
}
void navit_window_roadbook_new(struct navit *this_) {
    if (!this_->gui || this_->roadbook_callback || this_->roadbook_window) {
        return;
    }

    this_->roadbook_callback=callback_new_1(callback_cast(navit_window_roadbook_update), this_);
    navigation_register_callback(this_->navigation, attr_navigation_long, this_->roadbook_callback);
    this_->roadbook_window=gui_datawindow_new(this_->gui, _("Roadbook"), NULL,
                           callback_new_1(callback_cast(navit_window_roadbook_destroy), this_));
    navit_window_roadbook_update(this_);
}

void navit_init(struct navit *this_) {
    struct mapset *ms;
    struct map *map;
    int callback;
    char *center_file;

    dbg(lvl_info,"enter gui %p graphics %p",this_->gui,this_->gra);

    if (!this_->gui && !(this_->flags & 2)) {
        dbg(lvl_error,"FATAL: No GUI available.");
        exit(1);
    }
    if (!this_->gra && !(this_->flags & 1)) {
        dbg(lvl_error,"FATAL: No graphics subsystem available.");
        exit(1);
    }
    dbg(lvl_info,"Connecting gui to graphics");
    if (this_->gui && this_->gra && gui_set_graphics(this_->gui, this_->gra)) {
        struct attr attr_type_gui, attr_type_graphics;
        gui_get_attr(this_->gui, attr_type, &attr_type_gui, NULL);
        graphics_get_attr(this_->gra, attr_type, &attr_type_graphics, NULL);
        dbg(lvl_error,"FATAL: Failed to connect graphics '%s' to gui '%s'", attr_type_graphics.u.str, attr_type_gui.u.str);
        dbg(lvl_error,"Please see http://wiki.navit-project.org/index.php/Failed_to_connect_graphics_to_gui "
            "for explanations and solutions\n");
        exit(1);
    }
    if (this_->speech && this_->navigation) {
        struct attr speech;
        speech.type=attr_speech;
        speech.u.speech=this_->speech;
        navigation_set_attr(this_->navigation, &speech);
    }
    dbg(lvl_info,"Initializing graphics");
    dbg(lvl_info,"Setting Vehicle");
    navit_set_vehicle(this_, this_->vehicle);
    dbg(lvl_info,"Adding dynamic maps to mapset %p",this_->mapsets);
    if (this_->mapsets) {
        struct mapset_handle *msh;
        ms=this_->mapsets->data;
        this_->progress_cb=callback_new_attr_1(callback_cast(navit_map_progress), attr_progress, this_);
        msh=mapset_open(ms);
        while (msh && (map=mapset_next(msh, 0))) {
            //pass new callback instance for each map in the mapset to make map callback list destruction work correctly
            struct callback *pcb = callback_new_attr_1(callback_cast(navit_map_progress), attr_progress, this_);
            map_add_callback(map, pcb);
        }
        mapset_close(msh);

        if (this_->route) {
            if ((map=route_get_map(this_->route))) {
                struct attr map_a;
                map_a.type=attr_map;
                map_a.u.map=map;
                mapset_add_attr(ms, &map_a);
            }
            if ((map=route_get_graph_map(this_->route))) {
                struct attr map_a,active;
                map_a.type=attr_map;
                map_a.u.map=map;
                active.type=attr_active;
                active.u.num=0;
                mapset_add_attr(ms, &map_a);
                map_set_attr(map, &active);
            }
            route_set_mapset(this_->route, ms);
            route_set_projection(this_->route, transform_get_projection(this_->trans));
        }
        if (this_->tracking) {
            tracking_set_mapset(this_->tracking, ms);
            if (this_->route)
                tracking_set_route(this_->tracking, this_->route);
        }
        if (this_->navigation) {
            if ((map=navigation_get_map(this_->navigation))) {
                struct attr map_a,active;
                map_a.type=attr_map;
                map_a.u.map=map;
                active.type=attr_active;
                active.u.num=0;
                mapset_add_attr(ms, &map_a);
                map_set_attr(map, &active);
            }
        }
        if (this_->tracking) {
            if ((map=tracking_get_map(this_->tracking))) {
                struct attr map_a,active;
                map_a.type=attr_map;
                map_a.u.map=map;
                active.type=attr_active;
                active.u.num=0;
                mapset_add_attr(ms, &map_a);
                map_set_attr(map, &active);
            }
        }
        navit_add_former_destinations_from_file(this_);
    } else {
        dbg(lvl_error, "FATAL: No mapset available. Please add a (valid) mapset to your configuration.");
        exit(1);
    }
    if (this_->route) {
        struct attr callback;
        this_->route_cb=callback_new_attr_1(callback_cast(navit_redraw_route), attr_route_status, this_);
        callback.type=attr_callback;
        callback.u.callback=this_->route_cb;
        route_add_attr(this_->route, &callback);
    }
    if (this_->navigation) {
        if (this_->speech) {
            this_->nav_speech_cb=callback_new_1(callback_cast(navit_speak), this_);
            navigation_register_callback(this_->navigation, attr_navigation_speech, this_->nav_speech_cb);
        }
        if (this_->route)
            navigation_set_route(this_->navigation, this_->route);
    }
    dbg(lvl_info,"Setting Center");
    center_file = bookmarks_get_center_file(FALSE);
    bookmarks_set_center_from_file(this_->bookmarks, center_file);
    g_free(center_file);
    global_navit=this_;

    messagelist_init(this_->messages);

    navit_set_cursors(this_);

    callback_list_call_attr_1(this_->attr_cbl, attr_navit, this_);
    callback=(this_->ready == 2);
    this_->ready|=1;
    dbg(lvl_info,"ready=%d",this_->ready);
    if (this_->ready == 3)
        navit_draw_async(this_, 1);
    if (callback)
        callback_list_call_attr_1(this_->attr_cbl, attr_graphics_ready, this_);
}

void navit_zoom_to_rect(struct navit *this_, struct coord_rect *r) {
    struct coord c;
    int w,h,scale=16;

    c.x=(r->rl.x+r->lu.x)/2;
    c.y=(r->rl.y+r->lu.y)/2;
    transform_set_center(this_->trans, &c);
    transform_get_size(this_->trans, &w, &h);
    dbg(lvl_debug,"center 0x%x,0x%x w %d h %d",c.x,c.y,w,h);
    dbg(lvl_debug,"%x,%x-%x,%x", r->lu.x,r->lu.y,r->rl.x,r->rl.y);
    while (scale < 1<<20) {
        struct point p1,p2;
        transform_set_scale(this_->trans, scale);
        transform_setup_source_rect(this_->trans);
        transform(this_->trans, transform_get_projection(this_->trans), &r->lu, &p1, 1, 0, 0, NULL);
        transform(this_->trans, transform_get_projection(this_->trans), &r->rl, &p2, 1, 0, 0, NULL);
        dbg(lvl_debug,"%d,%d-%d,%d",p1.x,p1.y,p2.x,p2.y);
        if (p1.x < 0 || p2.x < 0 || p1.x > w || p2.x > w ||
                p1.y < 0 || p2.y < 0 || p1.y > h || p2.y > h)
            scale*=2;
        else
            break;

    }
    dbg(lvl_debug,"scale=%d (0x%x) of %d (0x%x)",scale,scale,1<<20,1<<20);
    if (this_->ready == 3)
        navit_draw_async(this_,0);
}

void navit_zoom_to_route(struct navit *this_, int orientation) {
    struct map *map;
    struct map_rect *mr=NULL;
    struct item *item;
    struct coord c;
    struct coord_rect r;
    int count=0;
    if (! this_->route)
        return;
    dbg(lvl_debug,"enter");
    map=route_get_map(this_->route);
    dbg(lvl_debug,"map=%p",map);
    if (map)
        mr=map_rect_new(map, NULL);
    dbg(lvl_debug,"mr=%p",mr);
    if (mr) {
        while ((item=map_rect_get_item(mr))) {
            dbg(lvl_debug,"item=%s", item_to_name(item->type));
            while (item_coord_get(item, &c, 1)) {
                dbg(lvl_debug,"coord");
                if (!count)
                    r.lu=r.rl=c;
                else
                    coord_rect_extend(&r, &c);
                count++;
            }
        }
        map_rect_destroy(mr);
    }
    if (! count)
        return;
    if (orientation != -1)
        transform_set_yaw(this_->trans, orientation);
    navit_zoom_to_rect(this_, &r);
}

static void navit_cmd_zoom_to_route(struct navit *this) {
    navit_zoom_to_route(this, 0);
}


/**
 * Change the current zoom level
 *
 * @param navit The navit instance
 * @param center The point where to center the map, including its projection
 * @returns nothing
 */
void navit_set_center(struct navit *this_, struct pcoord *center, int set_timeout) {
    struct coord *c=transform_center(this_->trans);
    struct coord c1,c2;
    enum projection pro = transform_get_projection(this_->trans);
    if (pro != center->pro) {
        c1.x = center->x;
        c1.y = center->y;
        transform_from_to(&c1, center->pro, &c2, pro);
    } else {
        c2.x = center->x;
        c2.y = center->y;
    }
    *c=c2;
    if (set_timeout)
        navit_set_timeout(this_);
    if (this_->ready == 3)
        navit_draw(this_);
}

static void navit_set_center_coord_screen(struct navit *this_, struct coord *c, struct point *p, int set_timeout) {
    int width, height;
    struct point po;
    transform_set_center(this_->trans, c);
    transform_get_size(this_->trans, &width, &height);
    po.x=width/2;
    po.y=height/2;
    update_transformation(this_->trans, &po, p);
    if (set_timeout)
        navit_set_timeout(this_);
}

/**
 * Links all vehicles to a cursor depending on the current profile.
 *
 * @param this_ A navit instance
 * @author Ralph Sennhauser (10/2009)
 */
static void navit_set_cursors(struct navit *this_) {
    struct attr name;
    struct navit_vehicle *nv;
    struct cursor *c;
    GList *v;

    v=g_list_first(this_->vehicles); // GList of navit_vehicles
    while (v) {
        nv=v->data;
        if (vehicle_get_attr(nv->vehicle, attr_cursorname, &name, NULL)) {
            if (!strcmp(name.u.str,"none"))
                c=NULL;
            else
                c=layout_get_cursor(this_->layout_current, name.u.str);
        } else
            c=layout_get_cursor(this_->layout_current, "default");
        vehicle_set_cursor(nv->vehicle, c, 0);
        v=g_list_next(v);
    }
    return;
}


/**
 * @brief Calculates the position of the cursor on the screen.
 *
 * This method considers padding if supported by the graphics plugin. In that case, the inner rectangle
 * (i.e. screen size minus padding) will be used to center the cursor and to determine cursor offset (as
 * specified in `this_->radius`).
 *
 * @param this_ The navit object
 * @param p Receives the screen coordinates for the cursor
 * @param keep_orientation Whether to maintain the current map orientation. If false, the map will be
 * rotated so that the bearing of the vehicle is up.
 * @param dir Receives the new map orientation as requested by `screen_orientation` (can be `NULL`)
 *
 * @return Always 1
 */
static int navit_get_cursor_pnt(struct navit *this_, struct point *p, int keep_orientation, int *dir) {
    int width, height;
    struct navit_vehicle *nv=this_->vehicle;
    struct padding *padding = NULL;

    float offset=this_->radius;      // Cursor offset from the center of the screen (percent).
#if 0 /* Better improve track.c to get that issue resolved or make it configurable with being off the default, the jumping back to the center is a bit annoying */
    float min_offset = 0.;      // Percent offset at min_offset_speed.
    float max_offset = 30.;     // Percent offset at max_offset_speed.
    int min_offset_speed = 2;   // Speed in km/h
    int max_offset_speed = 50;  // Speed in km/h
    // Calculate cursor offset from the center of the screen, upon speed.
    if (nv->speed <= min_offset_speed) {
        offset = min_offset;
    } else if (nv->speed > max_offset_speed) {
        offset = max_offset;
    } else {
        offset = (max_offset - min_offset) / (max_offset_speed - min_offset_speed) * (nv->speed - min_offset_speed);
    }
#endif

    if (this_->gra) {
        padding = graphics_get_data(this_->gra, "padding");
    } else
        dbg(lvl_warning, "cannot get padding: this->gra is NULL");

    transform_get_size(this_->trans, &width, &height);
    dbg(lvl_debug, "width=%d height=%d", width, height);

    if (padding) {
        width -= (padding->left + padding->right);
        height -= (padding->top + padding->bottom);
        dbg(lvl_debug, "corrected for padding: width=%d height=%d", width, height);
    }

    if (this_->orientation == -1 || keep_orientation) {
        p->x=50*width/100;
        p->y=(50 + offset)*height/100;
        if (dir)
            *dir=keep_orientation?this_->orientation:nv->dir;
    } else {
        int mdir;
        if (this_->tracking && this_->tracking_flag) {
            mdir = tracking_get_angle(this_->tracking) - this_->orientation;
        } else {
            mdir=nv->dir-this_->orientation;
        }

        p->x=(50 - offset*sin(M_PI*mdir/180.))*width/100;
        p->y=(50 + offset*cos(M_PI*mdir/180.))*height/100;
        if (dir)
            *dir=this_->orientation;
    }

    if (padding) {
        p->x += padding->left;
        p->y += padding->top;
    }

    dbg(lvl_debug, "x=%d y=%d, offset=%f", p->x, p->y, offset);

    return 1;
}

/**
 * @brief Recalculates the map view so that the vehicle cursor is visible
 *
 * This function recalculates the parameters which control the visible map area, zoom and orientation. The
 * caller is responsible for redrawing the map after the function returns.
 *
 * If the vehicle supplies a {@code position_valid} attribute and it is {@code attr_position_valid_invalid},
 * the map position is not changed.
 *
 * @param this_ The navit object
 * @param autozoom Whether to set zoom based on current speed. If false, current zoom will be maintained.
 * @param keep_orientation Whether to maintain the current map orientation. If false, the map will be rotated
 * so that the bearing of the vehicle is up.
 */
void navit_set_center_cursor(struct navit *this_, int autozoom, int keep_orientation) {
    int dir;
    struct point pn;
    struct navit_vehicle *nv=this_->vehicle;
    struct attr attr;
    if (vehicle_get_attr(nv->vehicle, attr_position_valid, &attr, NULL) && (attr.u.num == attr_position_valid_invalid))
        return;
    navit_get_cursor_pnt(this_, &pn, keep_orientation, &dir);
    transform_set_yaw(this_->trans, dir);
    navit_set_center_coord_screen(this_, &nv->coord, &pn, 0);
    if (autozoom)
        navit_autozoom(this_, &nv->coord, nv->speed, 0);
}

/**
 * @brief Recenters the map so that the vehicle cursor is visible
 *
 * This function first calls {@code navit_set_center_cursor()} to recalculate the map display, then
 * triggers a redraw of the map.
 *
 *@param this_ The navit object
 */
static void navit_set_center_cursor_draw(struct navit *this_) {
    navit_set_center_cursor(this_,1,0);
    if (this_->ready == 3)
        navit_draw_async(this_, 1);
}

/**
 * @brief Recenters the map so that the vehicle cursor is visible
 *
 * This is the callback function for the {@code set_center_cursor()} command. It is just a wrapper around
 * {@code navit_set_center_cursor_draw()}.
 *
 *@param this_ The navit object
 */
static void navit_cmd_set_center_cursor(struct navit *this_) {
    navit_set_center_cursor_draw(this_);
}

void navit_set_center_screen(struct navit *this_, struct point *p, int set_timeout) {
    struct coord c;
    struct pcoord pc;
    transform_reverse(this_->trans, p, &c);
    pc.x = c.x;
    pc.y = c.y;
    pc.pro = transform_get_projection(this_->trans);
    navit_set_center(this_, &pc, set_timeout);
}

static int navit_set_attr_do(struct navit *this_, struct attr *attr, int init) {
    int dir=0, orient_old=0, attr_updated=0;
    struct coord co;
    long zoom;
    GList *l;
    struct navit_vehicle *nv;
    struct layout *lay;
    struct attr active;
    active.type=attr_active;
    active.u.num=0;

    dbg(lvl_debug, "enter, this_=%p, attr=%p (%s), init=%d", this_, attr, attr_to_name(attr->type), init);

    switch (attr->type) {
    case attr_autozoom:
        attr_updated=(this_->autozoom_secs != attr->u.num);
        this_->autozoom_secs = attr->u.num;
        break;
    case attr_autozoom_active:
        attr_updated=(this_->autozoom_active != attr->u.num);
        this_->autozoom_active = attr->u.num;
        break;
    case attr_center:
        transform_from_geo(transform_get_projection(this_->trans), attr->u.coord_geo, &co);
        dbg(lvl_debug,"0x%x,0x%x",co.x,co.y);
        transform_set_center(this_->trans, &co);
        break;
    case attr_drag_bitmap:
        attr_updated=(this_->drag_bitmap != !!attr->u.num);
        this_->drag_bitmap=!!attr->u.num;
        break;
    case attr_flags:
        attr_updated=(this_->flags != attr->u.num);
        this_->flags=attr->u.num;
        break;
    case attr_flags_graphics:
        attr_updated=(this_->graphics_flags != attr->u.num);
        this_->graphics_flags=attr->u.num;
        break;
    case attr_follow:
        if (!this_->vehicle)
            return 0;
        attr_updated=(this_->vehicle->follow_curr != attr->u.num);
        this_->vehicle->follow_curr = attr->u.num;
        break;
    case attr_layout:
        if(!attr->u.layout)
            return 0;
        if(this_->layout_current!=attr->u.layout) {
            this_->layout_current=attr->u.layout;
            graphics_font_destroy_all(this_->gra);
            navit_set_cursors(this_);
            if (this_->ready == 3)
                navit_draw(this_);
            attr_updated=1;
        }
        break;
    case attr_layout_name:
        if(!attr->u.str)
            return 0;
        l=this_->layouts;
        while (l) {
            lay=l->data;
            if (!strcmp(lay->name,attr->u.str)) {
                struct attr attr;
                attr.type=attr_layout;
                attr.u.layout=lay;
                return navit_set_attr_do(this_, &attr, init);
            }
            l=g_list_next(l);
        }
        return 0;
    case attr_map_border:
        if (this_->border != attr->u.num) {
            this_->border=attr->u.num;
            attr_updated=1;
        }
        break;
    case attr_orientation:
        orient_old=this_->orientation;
        this_->orientation=attr->u.num;
        if (!init) {
            if (this_->orientation != -1) {
                dir = this_->orientation;
            } else {
                if (this_->vehicle) {
                    dir = this_->vehicle->dir;
                }
            }
            transform_set_yaw(this_->trans, dir);
            if (orient_old != this_->orientation) {
#if 0
                if (this_->ready == 3)
                    navit_draw(this_);
#endif
                attr_updated=1;
            }
        }
        break;
    case attr_osd_configuration:
        dbg(lvl_debug,"setting osd_configuration to %ld (was %d)", attr->u.num, this_->osd_configuration);
        attr_updated=(this_->osd_configuration != attr->u.num);
        this_->osd_configuration=attr->u.num;
        break;
    case attr_pitch:
        attr_updated=(this_->pitch != attr->u.num);
        this_->pitch=attr->u.num;
        transform_set_pitch(this_->trans, round(this_->pitch*sqrt(240*320)/sqrt(
                this_->w*this_->h))); // Pitch corrected for window resolution
        if (!init && attr_updated && this_->ready == 3)
            navit_draw(this_);
        break;
    case attr_projection:
        if(this_->trans && transform_get_projection(this_->trans) != attr->u.projection) {
            navit_projection_set(this_, attr->u.projection, !init);
            attr_updated=1;
        }
        break;
    case attr_radius:
        attr_updated=(this_->radius != attr->u.num);
        this_->radius=attr->u.num;
        break;
    case attr_recent_dest:
        attr_updated=(this_->recentdest_count != attr->u.num);
        this_->recentdest_count=attr->u.num;
        break;
    case attr_speech:
        if(this_->speech && this_->speech != attr->u.speech) {
            attr_updated=1;
            this_->speech = attr->u.speech;
        }
        break;
    case attr_timeout:
        attr_updated=(this_->center_timeout != attr->u.num);
        this_->center_timeout = attr->u.num;
        break;
    case attr_tracking:
        attr_updated=(this_->tracking_flag != !!attr->u.num);
        this_->tracking_flag=!!attr->u.num;
        break;
    case attr_transformation:
        this_->trans=attr->u.transformation;
        break;
    case attr_use_mousewheel:
        attr_updated=(this_->use_mousewheel != !!attr->u.num);
        this_->use_mousewheel=!!attr->u.num;
        break;
    case attr_vehicle:
        if (!attr->u.vehicle) {
            if (this_->vehicle) {
                vehicle_set_attr(this_->vehicle->vehicle, &active);
                navit_set_vehicle(this_, NULL);
                attr_updated=1;
            }
            break;
        }
        l=this_->vehicles;
        while(l) {
            nv=l->data;
            if (nv->vehicle == attr->u.vehicle) {
                if (!this_->vehicle || this_->vehicle->vehicle != attr->u.vehicle) {
                    if (this_->vehicle)
                        vehicle_set_attr(this_->vehicle->vehicle, &active);
                    active.u.num=1;
                    vehicle_set_attr(nv->vehicle, &active);
                    attr_updated=1;
                }
                navit_set_vehicle(this_, nv);
            }
            l=g_list_next(l);
        }
        break;
    case attr_vehicleprofile:
        attr_updated=navit_set_vehicleprofile(this_, attr->u.vehicleprofile);
        break;
    case attr_zoom:
        zoom=transform_get_scale(this_->trans);
        attr_updated=(zoom != attr->u.num);
        transform_set_scale(this_->trans, attr->u.num);
        if (attr_updated && !init)
            navit_draw(this_);
        break;
    case attr_zoom_min:
        attr_updated=(attr->u.num != this_->zoom_min);
        this_->zoom_min=attr->u.num;
        break;
    case attr_zoom_max:
        attr_updated=(attr->u.num != this_->zoom_max);
        this_->zoom_max=attr->u.num;
        break;
    case attr_message:
        navit_add_message(this_, attr->u.str);
        break;
    case attr_follow_cursor:
        attr_updated=(this_->follow_cursor != !!attr->u.num);
        this_->follow_cursor=!!attr->u.num;
        break;
    case attr_imperial:
        attr_updated=(this_->imperial != attr->u.num);
        this_->imperial=attr->u.num;
        break;
    case attr_waypoints_flag:
        attr_updated=(this_->waypoints_flag != !!attr->u.num);
        this_->waypoints_flag=!!attr->u.num;
        break;
    default:
        dbg(lvl_debug, "calling generic setter method for attribute type %s", attr_to_name(attr->type))
        return navit_object_set_attr((struct navit_object *) this_, attr);
    }
    if (attr_updated && !init) {
        callback_list_call_attr_2(this_->attr_cbl, attr->type, this_, attr);
        if (attr->type == attr_osd_configuration)
            graphics_draw_mode(this_->gra, draw_mode_end);
    }
    return 1;
}

int navit_set_attr(struct navit *this_, struct attr *attr) {
    return navit_set_attr_do(this_, attr, 0);
}

int navit_get_attr(struct navit *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
    struct message *msg;
    struct coord *c;
    int len,offset;
    int ret=1;

    switch (type) {
    case attr_message:
        msg = navit_get_messages(this_);

        if (!msg) {
            return 0;
        }

        len = 0;
        while (msg) {
            len += strlen(msg->text) + 1;
            msg = msg->next;
        }
        attr->u.str = g_malloc(len + 1);

        msg = navit_get_messages(this_);
        offset = 0;
        while (msg) {
            g_stpcpy((attr->u.str + offset), msg->text);
            offset += strlen(msg->text);
            attr->u.str[offset] = '\n';
            offset++;

            msg = msg->next;
        }

        attr->u.str[len] = '\0';
        break;
    case attr_imperial:
        attr->u.num=this_->imperial;
        break;
    case attr_bookmark_map:
        attr->u.map=bookmarks_get_map(this_->bookmarks);
        break;
    case attr_bookmarks:
        attr->u.bookmarks=this_->bookmarks;
        break;
    case attr_callback_list:
        attr->u.callback_list=this_->attr_cbl;
        break;
    case attr_center:
        c=transform_get_center(this_->trans);
        transform_to_geo(transform_get_projection(this_->trans), c, &this_->center);
        attr->u.coord_geo=&this_->center;
        break;
    case attr_destination:
        if (! this_->destination_valid)
            return 0;
        attr->u.pcoord=&this_->destination;
        break;
    case attr_displaylist:
        attr->u.displaylist=this_->displaylist;
        return (attr->u.displaylist != NULL);
    case attr_follow:
        if (!this_->vehicle)
            return 0;
        attr->u.num=this_->vehicle->follow_curr;
        break;
    case attr_former_destination_map:
        attr->u.map=this_->former_destination;
        break;
    case attr_graphics:
        attr->u.graphics=this_->gra;
        ret=(attr->u.graphics != NULL);
        break;
    case attr_gui:
        attr->u.gui=this_->gui;
        ret=(attr->u.gui != NULL);
        break;
    case attr_layer:
        ret=attr_generic_get_attr(this_->attrs, NULL, type, attr, iter?(struct attr_iter *)&iter->iter:NULL);
        break;
    case attr_layout:
        if (iter) {
            if (iter->u.list) {
                iter->u.list=g_list_next(iter->u.list);
            } else {
                iter->u.list=this_->layouts;
            }
            if (!iter->u.list)
                return 0;
            attr->u.layout=(struct layout *)iter->u.list->data;
        } else {
            attr->u.layout=this_->layout_current;
        }
        break;
    case attr_map:
        if (iter && this_->mapsets) {
            if (!iter->u.mapset_handle) {
                iter->u.mapset_handle=mapset_open((struct mapset *)this_->mapsets->data);
            }
            attr->u.map=mapset_next(iter->u.mapset_handle, 0);
            if(!attr->u.map) {
                mapset_close(iter->u.mapset_handle);
                return 0;
            }
        } else {
            return 0;
        }
        break;
    case attr_mapset:
        attr->u.mapset=this_->mapsets->data;
        ret=(attr->u.mapset != NULL);
        break;
    case attr_navigation:
        attr->u.navigation=this_->navigation;
        break;
    case attr_orientation:
        attr->u.num=this_->orientation;
        break;
    case attr_osd:
        ret=attr_generic_get_attr(this_->attrs, NULL, type, attr, iter?(struct attr_iter *)&iter->iter:NULL);
        break;
    case attr_osd_configuration:
        attr->u.num=this_->osd_configuration;
        break;
    case attr_pitch:
        attr->u.num=round(transform_get_pitch(this_->trans)*sqrt(this_->w*this_->h)/sqrt(
                              240*320)); // Pitch corrected for window resolution
        break;
    case attr_projection:
        if(this_->trans) {
            attr->u.num=transform_get_projection(this_->trans);
        } else {
            return 0;
        }
        break;
    case attr_route:
        attr->u.route=this_->route;
        break;
    case attr_speech:
        if(this_->speech) {
            attr->u.speech=this_->speech;
        } else {
            return  0;
        }
        break;
    case attr_timeout:
        attr->u.num=this_->center_timeout;
        break;
    case attr_tracking:
        attr->u.num=this_->tracking_flag;
        break;
    case attr_trackingo:
        attr->u.tracking=this_->tracking;
        break;
    case attr_transformation:
        attr->u.transformation=this_->trans;
        break;
    case attr_vehicle:
        if(iter) {
            if(iter->u.list) {
                iter->u.list=g_list_next(iter->u.list);
            } else {
                iter->u.list=this_->vehicles;
            }
            if(!iter->u.list)
                return 0;
            attr->u.vehicle=((struct navit_vehicle*)iter->u.list->data)->vehicle;
        } else {
            if(this_->vehicle) {
                attr->u.vehicle=this_->vehicle->vehicle;
            } else {
                return 0;
            }
        }
        break;
    case attr_vehicleprofile:
        if (iter) {
            if(iter->u.list) {
                iter->u.list=g_list_next(iter->u.list);
            } else {
                iter->u.list=this_->vehicleprofiles;
            }
            if(!iter->u.list)
                return 0;
            attr->u.vehicleprofile=iter->u.list->data;
        } else {
            attr->u.vehicleprofile=this_->vehicleprofile;
        }
        break;
    case attr_zoom:
        attr->u.num=transform_get_scale(this_->trans);
        break;
    case attr_autozoom_active:
        attr->u.num=this_->autozoom_active;
        break;
    case attr_follow_cursor:
        attr->u.num=this_->follow_cursor;
        break;
    case attr_waypoints_flag:
        attr->u.num=this_->waypoints_flag;
        break;
    default:
        dbg(lvl_debug, "calling generic getter method for attribute type %s", attr_to_name(type))
        return navit_object_get_attr((struct navit_object *) this_, type, attr, iter);
    }
    attr->type=type;
    return ret;
}

static int navit_add_log(struct navit *this_, struct log *log) {
    struct attr type_attr;
    if (!log_get_attr(log, attr_type, &type_attr, NULL))
        return 0;
    if (!strcmp(type_attr.u.str, "textfile_debug")) {
        char *header = "type=track_tracked\n";
        if (this_->textfile_debug_log)
            return 0;
        log_set_header(log, header, strlen(header));
        this_->textfile_debug_log=log;
        return 1;
    }
    return 0;
}

static int navit_add_layout(struct navit *this_, struct layout *layout) {
    struct attr active;
    this_->layouts = g_list_append(this_->layouts, layout);
    layout_get_attr(layout, attr_active, &active, NULL);
    if(active.u.num || !this_->layout_current) {
        this_->layout_current=layout;
        return 1;
    }
    return 0;
}

int navit_add_attr(struct navit *this_, struct attr *attr) {
    int ret=1;
    switch (attr->type) {
    case attr_callback:
        navit_add_callback(this_, attr->u.callback);
        break;
    case attr_log:
        ret=navit_add_log(this_, attr->u.log);
        break;
    case attr_gui:
        ret=navit_set_gui(this_, attr->u.gui);
        break;
    case attr_graphics:
        ret=navit_set_graphics(this_, attr->u.graphics);
        break;
    case attr_layout:
        navit_add_layout(this_, attr->u.layout);
        break;
    case attr_route:
        this_->route=attr->u.route;
        break;
    case attr_mapset:
        this_->mapsets = g_list_append(this_->mapsets, attr->u.mapset);
        break;
    case attr_navigation:
        this_->navigation=attr->u.navigation;
        break;
    case attr_osd:
        break;
    case attr_recent_dest:
        this_->recentdest_count = attr->u.num;
        break;
    case attr_speech:
        this_->speech=attr->u.speech;
        break;
    case attr_trackingo:
        this_->tracking=attr->u.tracking;
        break;
    case attr_vehicle:
        ret=navit_add_vehicle(this_, attr->u.vehicle);
        break;
    case attr_vehicleprofile:
        this_->vehicleprofiles=g_list_append(this_->vehicleprofiles, attr->u.vehicleprofile);
        break;
    case attr_autozoom_min:
        this_->autozoom_min = attr->u.num;
        break;
    case attr_autozoom_max:
        this_->autozoom_max = attr->u.num;
        break;
    case attr_layer:
    case attr_script:
        break;
    default:
        return 0;
    }
    if (ret)
        this_->attrs=attr_generic_add_attr(this_->attrs, attr);
    callback_list_call_attr_2(this_->attr_cbl, attr->type, this_, attr);
    return ret;
}

int navit_remove_attr(struct navit *this_, struct attr *attr) {
    int ret=1;
    switch (attr->type) {
    case attr_callback:
        navit_remove_callback(this_, attr->u.callback);
        break;
    case attr_vehicle:
    case attr_osd:
        this_->attrs=attr_generic_remove_attr(this_->attrs, attr);
        return 1;
    default:
        return 0;
    }
    return ret;
}

struct attr_iter *
navit_attr_iter_new(void) {
    return g_new0(struct attr_iter, 1);
}

void navit_attr_iter_destroy(struct attr_iter *iter) {
    g_free(iter);
}

void navit_add_callback(struct navit *this_, struct callback *cb) {
    callback_list_add(this_->attr_cbl, cb);
}

void navit_remove_callback(struct navit *this_, struct callback *cb) {
    callback_list_remove(this_->attr_cbl, cb);
}

static int coord_not_set(struct coord c) {
    return !(c.x || c.y);
}

/**
 * Toggle the cursor update : refresh the map each time the cursor has moved (instead of only when it reaches a border)
 *
 * @param this_ The navit instance
 * @param nv vehicle to draw
 * @param pnt Screen coordinates of the vehicle. If NULL, position stored in nv is used.
 * @returns nothing
 */

static void navit_vehicle_draw(struct navit *this_, struct navit_vehicle *nv, struct point *pnt) {
    struct point cursor_pnt;
    enum projection pro;

    if (this_->blocked||coord_not_set(nv->coord))
        return;
    if (pnt)
        cursor_pnt=*pnt;
    else {
        pro=transform_get_projection(this_->trans_cursor);
        if (!pro)
            return;
        transform(this_->trans_cursor, pro, &nv->coord, &cursor_pnt, 1, 0, 0, NULL);
    }
    vehicle_draw(nv->vehicle, this_->gra, &cursor_pnt, nv->dir-transform_get_yaw(this_->trans_cursor), nv->speed);
}

/**
 * @brief Called when the position of a vehicle changes.
 *
 * This function is called when the position of any configured vehicle changes and triggers all actions
 * that need to happen in response, such as:
 * <ul>
 * <li>Switching between day and night layout (based on the new position timestamp)</li>
 * <li>Updating position, bearing and speed of {@code nv} with the data of the active vehicle
 * (which may be different from the vehicle reporting the update)</li>
 * <li>Invoking callbacks for {@code navit}'s {@code attr_position} and {@code attr_position_coord_geo}
 * attributes</li>
 * <li>Triggering an update of the vehicle's position on the map and, if needed, an update of the
 * visible map area ad orientation</li>
 * <li>Logging a new track point, if enabled</li>
 * <li>Updating the position on the route</li>
 * <li>Stopping navigation if the destination has been reached</li>
 * </ul>
 *
 * @param this_ The navit object
 * @param nv The {@code navit_vehicle} which reported a new position
 */
static void navit_vehicle_update_position(struct navit *this_, struct navit_vehicle *nv) {
    struct attr attr_valid, attr_dir, attr_speed, attr_pos;
    struct pcoord cursor_pc;
    struct point cursor_pnt, *pnt=&cursor_pnt;
    struct tracking *tracking=NULL;
    struct pcoord *pc;
    enum projection pro=transform_get_projection(this_->trans_cursor);
    int count;
    int (*get_attr)(void *, enum attr_type, struct attr *, struct attr_iter *);
    void *attr_object;
    char *destination_file;
    char *description;

    profile(0,NULL);
    if (this_->ready == 3)
        navit_layout_switch(this_);
    if (this_->vehicle == nv && this_->tracking_flag)
        tracking=this_->tracking;
    if (tracking) {
        tracking_update(tracking, nv->vehicle, this_->vehicleprofile, pro);
        attr_object=tracking;
        get_attr=(int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))tracking_get_attr;
    } else {
        attr_object=nv->vehicle;
        get_attr=(int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))vehicle_get_attr;
    }
    if (get_attr(attr_object, attr_position_valid, &attr_valid, NULL))
        if (!attr_valid.u.num != attr_position_valid_invalid)
            return;
    if (! get_attr(attr_object, attr_position_direction, &attr_dir, NULL) ||
            ! get_attr(attr_object, attr_position_speed, &attr_speed, NULL) ||
            ! get_attr(attr_object, attr_position_coord_geo, &attr_pos, NULL)) {
        profile(0,"return 2\n");
        return;
    }
    nv->dir=*attr_dir.u.numd;
    nv->speed=*attr_speed.u.numd;
    transform_from_geo(pro, attr_pos.u.coord_geo, &nv->coord);
    if (nv != this_->vehicle) {
        if (this_->ready == 3)
            navit_vehicle_draw(this_, nv, NULL);
        profile(0,"return 3\n");
        return;
    }
    cursor_pc.x = nv->coord.x;
    cursor_pc.y = nv->coord.y;
    cursor_pc.pro = pro;
    if (this_->route) {
        if (tracking)
            route_set_position_from_tracking(this_->route, tracking, pro);
        else
            route_set_position(this_->route, &cursor_pc);
    }
    callback_list_call_attr_0(this_->attr_cbl, attr_position);
    navit_textfile_debug_log(this_, "type=trackpoint_tracked");
    if (this_->ready == 3) {
        if (this_->gui && nv->speed > 2)
            navit_disable_suspend();

        transform(this_->trans_cursor, pro, &nv->coord, &cursor_pnt, 1, 0, 0, NULL);
        if (this_->button_pressed != 1 && this_->follow_cursor && nv->follow_curr <= nv->follow &&
                (nv->follow_curr == 1 || !transform_within_border(this_->trans_cursor, &cursor_pnt, this_->border)))
            navit_set_center_cursor_draw(this_);
        else
            navit_vehicle_draw(this_, nv, pnt);

        if (nv->follow_curr > 1)
            nv->follow_curr--;
        else
            nv->follow_curr=nv->follow;
    }
    callback_list_call_attr_2(this_->attr_cbl, attr_position_coord_geo, this_, nv->vehicle);

    /* Finally, if we reached our destination, stop navigation. */
    if (this_->route) {
        switch(route_destination_reached(this_->route)) {
        case 1:
            description=route_get_destination_description(this_->route, 0);
            route_remove_waypoint(this_->route);
            count=route_get_destination_count(this_->route);
            pc=g_alloca(sizeof(*pc)*count);
            route_get_destinations(this_->route, pc, count);
            destination_file = bookmarks_get_destination_file(TRUE);
            bookmarks_append_destinations(this_->former_destination, destination_file, pc, count, type_former_itinerary_part,
                                          description, this_->recentdest_count);
            g_free(destination_file);
            g_free(description);
            break;
        case 2:
            destination_file = bookmarks_get_destination_file(TRUE);
            bookmarks_append_destinations(this_->former_destination, destination_file, NULL, 0, type_former_itinerary_part, NULL,
                                          this_->recentdest_count);
            navit_set_destination(this_, NULL, NULL, 0);
            g_free(destination_file);
            break;
        }
    }
    profile(0,"return 5\n");
}

/**
 * @brief Called when a status attribute of a vehicle changes.
 *
 * This function is called when the {@code position_fix_type}, {@code position_sats_used} or {@code position_hdop}
 * attribute of any configured vehicle changes.
 *
 * The function checks if {@code nv} refers to the active vehicle and if {@code type} is one of the above types.
 * If this is the case, it invokes the callback functions for {@code navit}'s respective attributes.
 *
 * Future actions that need to happen when one of these three attribute changes for any vehicle should be
 * implemented here.
 *
 * @param this_ The navit object
 * @param nv The {@code navit_vehicle} which reported a new status attribute
 * @param type The type of attribute with has changed
 */
static void navit_vehicle_update_status(struct navit *this_, struct navit_vehicle *nv, enum attr_type type) {
    if (this_->vehicle != nv)
        return;
    switch(type) {
    case attr_position_fix_type:
    case attr_position_sats_used:
    case attr_position_hdop:
        callback_list_call_attr_2(this_->attr_cbl, type, this_, nv->vehicle);
        break;
    default:
        return;
    }
}

/**
 * Set the position of the vehicle
 *
 * @param navit The navit instance
 * @param c The coordinate to set as position
 * @returns nothing
 */

void navit_set_position(struct navit *this_, struct pcoord *c) {
    if (this_->route) {
        route_set_position(this_->route, c);
        callback_list_call_attr_0(this_->attr_cbl, attr_position);
    }
    if (this_->ready == 3)
        navit_draw(this_);
}

static int navit_set_vehicleprofile(struct navit *this_, struct vehicleprofile *vp) {
    if (this_->vehicleprofile == vp)
        return 0;
    this_->vehicleprofile=vp;
    if (this_->route)
        route_set_profile(this_->route, this_->vehicleprofile);
    return 1;
}

int navit_set_vehicleprofile_name(struct navit *this_, char *name) {
    struct attr attr;
    GList *l;
    l=this_->vehicleprofiles;
    while (l) {
        if (vehicleprofile_get_attr(l->data, attr_name, &attr, NULL)) {
            if (!strcmp(attr.u.str, name)) {
                navit_set_vehicleprofile(this_, l->data);
                return 1;
            }
        }
        l=g_list_next(l);
    }
    return 0;
}

static void navit_set_vehicle(struct navit *this_, struct navit_vehicle *nv) {
    struct attr attr;
    this_->vehicle=nv;
    if (nv && vehicle_get_attr(nv->vehicle, attr_profilename, &attr, NULL)) {
        if (navit_set_vehicleprofile_name(this_, attr.u.str))
            return;
    }
    if (!this_->vehicleprofile) { // When deactivating vehicle, keep the last profile if any
        if (!navit_set_vehicleprofile_name(this_,"car")) {
            /* We do not have a fallback "car" profile
            * so lets set any profile */
            GList *l;
            l=this_->vehicleprofiles;
            if (l) {
                this_->vehicleprofile=l->data;
                if (this_->route)
                    route_set_profile(this_->route, this_->vehicleprofile);
            }
        }
    } else {
        if (this_->route)
            route_set_profile(this_->route, this_->vehicleprofile);
    }
}

/**
 * @brief Registers a new vehicle.
 *
 * @param this_ The navit instance
 * @param v The vehicle to register
 * @return True for success
 */
static int navit_add_vehicle(struct navit *this_, struct vehicle *v) {
    struct navit_vehicle *nv=g_new0(struct navit_vehicle, 1);
    struct attr follow, active, animate;
    nv->vehicle=v;
    nv->follow=0;
    nv->last.x = 0;
    nv->last.y = 0;
    nv->animate_cursor=0;
    if ((vehicle_get_attr(v, attr_follow, &follow, NULL)))
        nv->follow=follow.u.num;
    nv->follow_curr=nv->follow;
    this_->vehicles=g_list_append(this_->vehicles, nv);
    if ((vehicle_get_attr(v, attr_active, &active, NULL)) && active.u.num)
        navit_set_vehicle(this_, nv);
    if ((vehicle_get_attr(v, attr_animate, &animate, NULL)))
        nv->animate_cursor=animate.u.num;
    nv->callback.type=attr_callback;
    nv->callback.u.callback=callback_new_attr_2(callback_cast(navit_vehicle_update_position), attr_position_coord_geo,
                            this_, nv);
    vehicle_add_attr(nv->vehicle, &nv->callback);
    nv->callback.u.callback=callback_new_attr_3(callback_cast(navit_vehicle_update_status), attr_position_fix_type, this_,
                            nv, attr_position_fix_type);
    vehicle_add_attr(nv->vehicle, &nv->callback);
    nv->callback.u.callback=callback_new_attr_3(callback_cast(navit_vehicle_update_status), attr_position_sats_used, this_,
                            nv, attr_position_sats_used);
    vehicle_add_attr(nv->vehicle, &nv->callback);
    nv->callback.u.callback=callback_new_attr_3(callback_cast(navit_vehicle_update_status), attr_position_hdop, this_, nv,
                            attr_position_hdop);
    vehicle_add_attr(nv->vehicle, &nv->callback);
    vehicle_set_attr(nv->vehicle, &this_->self);
    return 1;
}




struct gui *
navit_get_gui(struct navit *this_) {
    return this_->gui;
}

struct transformation *
navit_get_trans(struct navit *this_) {
    return this_->trans;
}

struct route *
navit_get_route(struct navit *this_) {
    return this_->route;
}

struct navigation *
navit_get_navigation(struct navit *this_) {
    return this_->navigation;
}

struct displaylist *
navit_get_displaylist(struct navit *this_) {
    return this_->displaylist;
}

/*todo : make it switch to nightlayout when we are in a tunnel */
void navit_layout_switch(struct navit *n) {

    int currTs=0;
    struct attr iso8601_attr,geo_attr,valid_attr,layout_attr;
    double trise,tset,trise_actual;
    struct layout *l;
    int year, month, day;
    int after_sunrise = FALSE;
    int after_sunset = FALSE;

    if (navit_get_attr(n,attr_layout,&layout_attr,NULL)!=1) {
        return; //No layout - nothing to switch
    }
    if (!n->vehicle)
        return;
    l=layout_attr.u.layout;

    if (l->dayname || l->nightname) {
        //Ok, we know that we have profile to switch

        //Check that we aren't calculating too fast
        if (vehicle_get_attr(n->vehicle->vehicle, attr_position_time_iso8601,&iso8601_attr,NULL)==1) {
            currTs=iso8601_to_secs(iso8601_attr.u.str);
            dbg(lvl_debug,"currTs: %u:%u",currTs%86400/3600,((currTs%86400)%3600)/60);
        }
        dbg(lvl_debug,"prevTs: %u:%u",n->prevTs%86400/3600,((n->prevTs%86400)%3600)/60);

        if (n->auto_switch == FALSE)
            return;

        if (currTs-(n->prevTs)<60) {
            //We've have to wait a little
            return;
        }

        if (sscanf(iso8601_attr.u.str,"%d-%02d-%02dT",&year,&month,&day) != 3)
            return;
        if (vehicle_get_attr(n->vehicle->vehicle, attr_position_valid, &valid_attr,NULL)
                && valid_attr.u.num==attr_position_valid_invalid) {
            return; //No valid fix yet
        }
        if (vehicle_get_attr(n->vehicle->vehicle, attr_position_coord_geo,&geo_attr,NULL)!=1) {
            //No position - no sun
            return;
        }
        //We calculate sunrise anyway, cause it is needed both for day and for night
        if (__sunriset__(year,month,day,geo_attr.u.coord_geo->lng,geo_attr.u.coord_geo->lat,-5,1,&trise,&tset)!=0) {
            dbg(lvl_debug,"near the pole sun never rises/sets, so we should never switch profiles");
            dbg(lvl_debug,"trise: %u:%u",HOURS(trise),MINUTES(trise));
            dbg(lvl_debug,"tset: %u:%u",HOURS(tset),MINUTES(tset));
            n->prevTs=currTs;
            return;
        }
        trise_actual=trise;
        dbg(lvl_debug,"trise: %u:%u",HOURS(trise),MINUTES(trise));
        dbg(lvl_debug,"tset: %u:%u",HOURS(tset),MINUTES(tset));
        dbg(lvl_debug,"dayname = %s, name =%s ",l->dayname, l->name);
        dbg(lvl_debug,"nightname = %s, name = %s ",l->nightname, l->name);
        if (HOURS(trise)*60+MINUTES(trise)<(currTs%86400)/60) {
            after_sunrise = TRUE;
        }

        if (((HOURS(tset)*60+MINUTES(tset)<(currTs%86400)/60)) ||
                ((HOURS(trise_actual)*60+MINUTES(trise_actual)>(currTs%86400)/60))) {
            after_sunset = TRUE;
        }
        if (after_sunrise && !after_sunset && l->dayname) {
            navit_set_layout_by_name(n,l->dayname);
            dbg(lvl_debug,"layout set to day");
        } else if (after_sunset && l->nightname) {
            navit_set_layout_by_name(n,l->nightname);
            dbg(lvl_debug,"layout set to night");
        }
        n->prevTs=currTs;
    }
}

/**
 * @brief this command is used to change the layout and enable/disable the automatic layout switcher
 *
 * @param this_ The navit instance
 * @param function unused
 * @param in input attributes in[0], a string, see usage below
 * @param out output attribute unused
 * @param valid unused
 *
 *
 * usage :
 * manual        : disable autoswitcher
 * auto          : enable autoswitcher
 * manual_toggle : disable autoswitcher and toggle between day / night layout
 * manual_day    : disable autoswitcher and set to day layout
 * manual_night  : disable autoswitcher and set to night layout
 *
 * todo : make it return the state of the autoswitcher and
 * the version of the active layout (day/night/undefined)
 */
static
void navit_cmd_switch_layout_day_night(struct navit *this_, char *function, struct attr **in, struct attr ***out,
                                       int valid) {

    if (!(in && in[0] && ATTR_IS_STRING(in[0]->type))) {
        return;
    }

    dbg(lvl_debug," called with mode =%s",in[0]->u.str);

    if (!this_->layout_current)
        return;

    if (!this_->vehicle)
        return;

    if (!strcmp(in[0]->u.str,"manual")) {
        this_->auto_switch = FALSE;
    } else if (!strcmp(in[0]->u.str,"auto")) {
        this_->auto_switch = TRUE;
        this_->prevTs = 0;
        navit_layout_switch(this_);
    } else if (!strcmp(in[0]->u.str,"manual_toggle")) {
        if (this_->layout_current->dayname) {
            navit_set_layout_by_name(this_,this_->layout_current->dayname);
            this_->auto_switch = FALSE;
            dbg(lvl_debug,"toggeled layout to = %s",this_->layout_current->name);
        } else if (this_->layout_current->nightname) {
            navit_set_layout_by_name(this_,this_->layout_current->nightname);
            this_->auto_switch = FALSE;
            dbg(lvl_debug,"toggeled layout to = %s",this_->layout_current->name);
        }
    } else if (!strcmp(in[0]->u.str,"manual_day") && this_->layout_current->dayname) {
        navit_set_layout_by_name(this_,this_->layout_current->dayname);
        this_->auto_switch = FALSE;
        dbg(lvl_debug,"switched layout to = %s",this_->layout_current->name);
    } else if (!strcmp(in[0]->u.str,"manual_night") && this_->layout_current->nightname) {
        navit_set_layout_by_name(this_,this_->layout_current->nightname);
        this_->auto_switch = FALSE;
        dbg(lvl_debug,"switched layout to = %s",this_->layout_current->name);
    }

    dbg(lvl_debug,"auto = %i",this_->auto_switch);
    return;
}

int navit_set_vehicle_by_name(struct navit *n,const char *name) {
    struct vehicle *v;
    struct attr_iter *iter;
    struct attr vehicle_attr, name_attr;

    iter=navit_attr_iter_new();

    while (navit_get_attr(n,attr_vehicle,&vehicle_attr,iter)) {
        v=vehicle_attr.u.vehicle;
        vehicle_get_attr(v,attr_name,&name_attr,NULL);
        if (name_attr.type==attr_name) {
            if (!strcmp(name,name_attr.u.str)) {
                navit_set_attr(n,&vehicle_attr);
                navit_attr_iter_destroy(iter);
                return 1;
            }
        }
    }
    navit_attr_iter_destroy(iter);
    return 0;
}

int navit_set_layout_by_name(struct navit *n,const char *name) {
    struct layout *l;
    struct attr_iter iter;
    struct attr layout_attr;

    iter.u.list=0x00;

    if (navit_get_attr(n,attr_layout,&layout_attr,&iter)!=1) {
        return 0; //No layouts - nothing to do
    }
    if (iter.u.list==NULL) {
        return 0;
    }

    iter.u.list=g_list_first(iter.u.list);

    while(iter.u.list) {
        l=(struct layout*)iter.u.list->data;
        if (!strcmp(name,l->name)) {
            layout_attr.u.layout=l;
            layout_attr.type=attr_layout;
            navit_set_attr(n,&layout_attr);
            iter.u.list=g_list_first(iter.u.list);
            return 1;
        }
        iter.u.list=g_list_next(iter.u.list);
    }

    iter.u.list=g_list_first(iter.u.list);
    return 0;
}

void navit_disable_suspend() {
    gui_disable_suspend(global_navit->gui);
    callback_list_call_attr_0(global_navit->attr_cbl,attr_unsuspend);
}

/**
 * @brief Blocks or unblocks redraw operations.
 *
 * The {@code block} parameter specifies the operation to carry out:
 *
 * {@code block > 0} cancels all draw operations in progress and blocks future operations. It sets flag 1 of the
 * {@code blocked} member. If draw operations in progress were canceled, flag 2 is also set.
 *
 * {@code block = 0} unblocks redraw operations, resetting {@code blocked} to 0. If flag 2 was previously set,
 * indicating that draw operations had been previously canceled, a redraw is triggered.
 *
 * {@code block < 0} unblocks redraw operations and forces a redraw. As above, {@code blocked} is reset to 0.
 *
 * @param this_ The navit instance
 * @param block The operation to perform, see description
 *
 * @return {@code true} if a redraw operation was triggered, {@code false} if not
 */
int navit_block(struct navit *this_, int block) {
    if (block > 0) {
        this_->blocked |= 1;
        if (graphics_draw_cancel(this_->gra, this_->displaylist))
            this_->blocked |= 2;
        return 0;
    }
    if ((this_->blocked & 2) || block < 0) {
        this_->blocked=0;
        navit_draw(this_);
        return 1;
    }
    this_->blocked=0;
    return 0;
}

/**
 * @brief Returns whether redraw operations are currently blocked.
 */
int navit_get_blocked(struct navit *this_) {
    return this_->blocked;
}

void navit_destroy(struct navit *this_) {
    dbg(lvl_debug,"enter %p",this_);
    graphics_draw_cancel(this_->gra, this_->displaylist);
    callback_list_call_attr_1(this_->attr_cbl, attr_destroy, this_);
    attr_list_free(this_->attrs);

    if(cmd_int_var_hash) {
        g_hash_table_destroy(cmd_int_var_hash);
        cmd_int_var_hash=NULL;
    }
    if(cmd_attr_var_hash) {
        g_hash_table_destroy(cmd_attr_var_hash);
        cmd_attr_var_hash=NULL;
    }
    if(cmd_int_var_stack) {
        g_list_foreach(cmd_int_var_stack, (GFunc)attr_free, NULL);
        g_list_free(cmd_int_var_stack);
        cmd_int_var_stack=NULL;
    }

    if (this_->bookmarks) {
        char *center_file = bookmarks_get_center_file(TRUE);
        bookmarks_write_center_to_file(this_->bookmarks, center_file);
        g_free(center_file);
        bookmarks_destroy(this_->bookmarks);
    }

    callback_destroy(this_->nav_speech_cb);
    callback_destroy(this_->roadbook_callback);
    callback_destroy(this_->popup_callback);
    callback_destroy(this_->motion_timeout_callback);
    callback_destroy(this_->progress_cb);

    if(this_->gra) {
        graphics_remove_callback(this_->gra, this_->resize_callback);
        graphics_remove_callback(this_->gra, this_->button_callback);
        graphics_remove_callback(this_->gra, this_->motion_callback);
        graphics_remove_callback(this_->gra, this_->predraw_callback);
    }

    callback_destroy(this_->resize_callback);
    callback_destroy(this_->motion_callback);
    callback_destroy(this_->predraw_callback);

    callback_destroy(this_->route_cb);
    if (this_->route)
        route_destroy(this_->route);

    map_destroy(this_->former_destination);

    graphics_displaylist_destroy(this_->displaylist);

    graphics_free(this_->gra);

    g_free(this_);
}

struct object_func navit_func = {
    attr_navit,
    (object_func_new)navit_new,
    (object_func_get_attr)navit_get_attr,
    (object_func_iter_new)navit_attr_iter_new,
    (object_func_iter_destroy)navit_attr_iter_destroy,
    (object_func_set_attr)navit_set_attr,
    (object_func_add_attr)navit_add_attr,
    (object_func_remove_attr)navit_remove_attr,
    (object_func_init)navit_init,
    (object_func_destroy)navit_destroy,
    (object_func_dup)NULL,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};

/** @} */
