/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2010 Navit Team
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

//##############################################################################################################
//#
//# File: gui_internal.c
//# Description: New "internal" GUI for use with any graphics library
//# Comment: Trying to make a touchscreen friendly GUI
//# Authors: Martin Schaller (04/2008), Stefan Klumpp (04/2008)
//#
//##############################################################################################################


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include <time.h>
#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#endif
#ifndef _MSC_VER
#include <sys/time.h>
#endif /* _MSC_VER */
#include "item.h"
#include "xmlconfig.h"
#include "file.h"
#include "navit.h"
#include "navit_nls.h"
#include "gui.h"
#include "coord.h"
#include "point.h"
#include "plugin.h"
#include "graphics.h"
#include "transform.h"
#include "color.h"
#include "map.h"
#include "callback.h"
#include "vehicle.h"
#include "vehicleprofile.h"
#include "window.h"
#include "config_.h"
#include "keys.h"
#include "mapset.h"
#include "route.h"
#include "navit/search.h"
#include "track.h"
#include "country.h"
#include "config.h"
#include "event.h"
#include "navit_nls.h"
#include "navigation.h"
#include "gui_internal.h"
#include "command.h"
#include "util.h"
#include "bookmarks.h"
#include "linguistics.h"
#include "debug.h"
#include "fib.h"
#include "types.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_html.h"
#include "gui_internal_bookmark.h"
#include "gui_internal_menu.h"
#include "gui_internal_search.h"
#include "gui_internal_gesture.h"
#include "gui_internal_poi.h"
#include "gui_internal_command.h"
#include "gui_internal_keyboard.h"


/**
 * Indexes into the config_profiles array.
 */
const int LARGE_PROFILE=0;
const int MEDIUM_PROFILE=1;
const int SMALL_PROFILE=2;

/**
 * The default config profiles.
 *
 * [0] =>  LARGE_PROFILE (screens 640 in one dimension)
 * [1] =>  MEDIUM PROFILE (screens larger than 320 in one dimension
 * [2] => Small profile (default)
 */
static struct gui_config_settings config_profiles[]= {
    {545,32,48,96,10}
    , {300,32,48,64,3}
    ,{200,16,32,48,2}
};

static void gui_internal_cmd_view_in_browser(struct gui_priv *this, struct widget *wm, void *data);
static void gui_internal_prepare_search_results_map(struct gui_priv *this, struct widget *table, struct coord_rect *r);

static int gui_internal_is_active_vehicle(struct gui_priv *this, struct vehicle *vehicle);

/**
 * @brief Displays an image scaled to a specific size
 *
 * Searches for scaleable and pre-scaled image
 *
 * @param this Our gui context
 * @param name image name
 * @param w desired width of image
 * @param h desired height of image
 *
 * @return image_struct Ptr to scaled image struct or NULL if not scaled or found
 */
static struct graphics_image *image_new_scaled(struct gui_priv *this, const char *name, int w, int h) {
    struct graphics_image *ret=NULL;
    char *full_path=NULL;
    full_path=graphics_icon_path(name);
    ret=graphics_image_new_scaled(this->gra, full_path, w, h);
    dbg(lvl_debug,"Trying to load image '%s' (w=%d, h=%d): %s", name, w, h, ret ? "OK" : "NOT FOUND");
    g_free(full_path);
    if (!ret) {
        dbg(lvl_error,"Failed to load image for '%s' (w=%d, h=%d)", name, w, h);
        full_path=graphics_icon_path("unknown");
        ret=graphics_image_new_scaled(this->gra, full_path, w, h);
        g_free(full_path);
    }
    return ret;
}

#if 0
static struct graphics_image *image_new_o(struct gui_priv *this, char *name) {
    return image_new_scaled(this, name, -1, -1);
}
#endif

/**
 * @brief Displays an image scaled to xs (extra small) size
 *
 * This image size can be too small to click it on some devices.
 *
 * @param this Our gui context
 * @param name image name
 *
 * @return image_struct Ptr to scaled image struct or NULL if not scaled or found
 */
struct graphics_image *
image_new_xs(struct gui_priv *this, const char *name) {
    return image_new_scaled(this, name, this->icon_xs, this->icon_xs);
}

/**
 * @brief Displays an image scaled to s (small) size
 *
 * @param this Our gui context
 * @param name image name
 *
 * @return image_struct Ptr to scaled image struct or NULL if not scaled or found
 */
struct graphics_image *
image_new_s(struct gui_priv *this, const char *name) {
    return image_new_scaled(this, name, this->icon_s, this->icon_s);
}

/**
 * @brief Displays an image scaled to l (large) size
 * @param this Our gui context
 * @param name image name
 *
 * @return image_struct Ptr to scaled image struct or NULL if not scaled or found
 */
struct graphics_image *
image_new_l(struct gui_priv *this, const char *name) {
    return image_new_scaled(this, name, this->icon_l, this->icon_l);
}



static int gui_internal_button_attr_update(struct gui_priv *this, struct widget *w) {
    struct widget *wi;
    int is_on=0;
    struct attr curr;
    GList *l;

    if (w->get_attr(w->instance, w->on.type, &curr, NULL))
        is_on=curr.u.data == w->on.u.data;
    else
        is_on=w->deflt;
    if (is_on != w->is_on) {
        if (w->redraw)
            this->redraw=1;
        w->is_on=is_on;
        l=g_list_first(w->children);
        if (l) {
            wi=l->data;
            if (wi->img)
                graphics_image_free(this->gra, wi->img);
            wi->img=image_new_xs(this, is_on ? "gui_active" : "gui_inactive");
        }
        if (w->is_on && w->off.type == attr_none)
            w->state &= ~STATE_SENSITIVE;
        else
            w->state |= STATE_SENSITIVE;
        return 1;
    }
    return 0;
}

static void gui_internal_button_attr_callback(struct gui_priv *this, struct widget *w) {
    if (gui_internal_button_attr_update(this, w))
        gui_internal_widget_render(this, w);
}
static void gui_internal_button_attr_pressed(struct gui_priv *this, struct widget *w, void *data) {
    if (w->is_on)
        w->set_attr(w->instance, &w->off);
    else
        w->set_attr(w->instance, &w->on);
    gui_internal_button_attr_update(this, w);

}

struct widget *
gui_internal_button_navit_attr_new(struct gui_priv *this, const char *text, enum flags flags, struct attr *on,
                                   struct attr *off) {
    struct graphics_image *image=NULL;
    struct widget *ret;
    if (!on && !off)
        return NULL;
    image=image_new_xs(this, "gui_inactive");
    ret=gui_internal_button_new_with_callback(this, text, image, flags, gui_internal_button_attr_pressed, NULL);
    if (on)
        ret->on=*on;
    if (off)
        ret->off=*off;
    ret->get_attr=(int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))navit_get_attr;
    ret->set_attr=(int (*)(void *, struct attr *))navit_set_attr;
    ret->remove_cb=(void (*)(void *, struct callback *))navit_remove_callback;
    ret->instance=this->nav;
    ret->cb=callback_new_attr_2(callback_cast(gui_internal_button_attr_callback), on?on->type:off->type, this, ret);
    navit_add_callback(this->nav, ret->cb);
    gui_internal_button_attr_update(this, ret);
    return ret;
}

struct widget *
gui_internal_button_map_attr_new(struct gui_priv *this, const char *text, enum flags flags, struct map *map,
                                 struct attr *on, struct attr *off, int deflt) {
    struct graphics_image *image=NULL;
    struct widget *ret;
    image=image_new_xs(this, "gui_inactive");
    if (!on && !off)
        return NULL;
    ret=gui_internal_button_new_with_callback(this, text, image, flags, gui_internal_button_attr_pressed, NULL);
    if (on)
        ret->on=*on;
    if (off)
        ret->off=*off;
    ret->deflt=deflt;
    ret->get_attr=(int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))map_get_attr;
    ret->set_attr=(int (*)(void *, struct attr *))map_set_attr;
    ret->remove_cb=(void (*)(void *, struct callback *))map_remove_callback;
    ret->instance=map;
    ret->redraw=1;
    ret->cb=callback_new_attr_2(callback_cast(gui_internal_button_attr_callback), on?on->type:off->type, this, ret);
    map_add_callback(map, ret->cb);
    gui_internal_button_attr_update(this, ret);
    return ret;
}






/*
 * @brief Calculate movement vector and timing of the gesture.
 * @param in this gui context
 * @param in msec time in milliseconds to find gesture within
 * @param out p0 pointer to the point object, where gesture starting point coordinates should be placed. Can be NULL.
 * @param out dx pointer to variable to store horizontal movement of the gesture.
 * @param out dy pointer to variable to store vertical movement of the gesture.
 * @return amount of time the actual movement took.
 */
/* FIXME where is the implementation? */


static void gui_internal_motion_cb(struct gui_priv *this) {
    this->motion_timeout_event=NULL;
    gui_internal_gesture_ring_add(this, &(this->current));

    /* Check for scrollable table below the highligted item if there's a movement with the button pressed */
    if (this->pressed && this->highlighted) {
        struct widget *wt=NULL;
        struct widget *wr=NULL;
        int dx,dy;

        /* Guard against accidental scrolling when user is likely going to swipe */
        gui_internal_gesture_get_vector(this, 1000, NULL, &dx, &dy);
        if(abs(dx)>abs(dy) || abs(dy)<this->icon_s)
            return;

        if(this->highlighted)
            for(wr=this->highlighted; wr && wr->type!=widget_table_row; wr=wr->parent);
        if(wr)
            wt=wr->parent;

        if(wt && wt->type==widget_table && (wt->state & STATE_SCROLLABLE)) {
            struct table_data *td=wt->data;
            GList *top=NULL;
            GList *btm=NULL;
            GList *ttop, *tbtm;



            if(!wr || !wr->h)
                return;

            if(this->current.y < wr->p.y  && wr!=td->top_row->data ) {
                int n=(wr->p.y-this->current.y)/wr->h+1;

                btm=td->bottom_row;
                top=td->top_row;

                while(n-->0 && (tbtm=gui_internal_widget_table_next_row(btm))!=NULL
                        && (ttop=gui_internal_widget_table_next_row(top))!=NULL) {
                    top=ttop;
                    btm=tbtm;
                    if(top->data==wr)
                        break;
                }
                this->pressed=2;
            } else if (this->current.y > wr->p.y + wr->h ) {
                int y=wt->p.y+wt->h-wr->h;
                int n;

                if(td->scroll_buttons.button_box && td->scroll_buttons.button_box->p.y!=0)
                    y=td->scroll_buttons.button_box->p.y - td->scroll_buttons.button_box->h;

                if(y>this->current.y)
                    y=this->current.y;

                n=(y - wr->p.y )/wr->h;

                btm=td->bottom_row;
                top=td->top_row;

                while(n-->0 && (ttop=gui_internal_widget_table_prev_row(top))!=NULL
                        && (tbtm=gui_internal_widget_table_prev_row(btm))!=NULL) {
                    btm=tbtm;
                    top=ttop;
                    if(btm->data==wr)
                        break;
                }
                this->pressed=2;
            }
            if( top && btm && (td->top_row!=top || td->bottom_row!=btm) ) {
                gui_internal_table_hide_rows(wt->data);
                td->top_row=top;
                td->bottom_row=btm;
                graphics_draw_mode(this->gra, draw_mode_begin);
                gui_internal_widget_render(this,wt);
                graphics_draw_mode(this->gra, draw_mode_end);
            }

            return;
        }
    }

    /* Else, just move highlight after pointer if there's nothing to scroll */
    gui_internal_highlight(this);
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gui_internal_call_highlighted(struct gui_priv *this) {
    if (! this->highlighted || ! this->highlighted->func)
        return;
    this->highlighted->reason=gui_internal_reason_click;
    this->highlighted->func(this, this->highlighted, this->highlighted->data);
}

void gui_internal_say(struct gui_priv *this, struct widget *w, int questionmark) {
    char *text=w->speech;
    if (! this->speech)
        return;
    if (!text)
        text=w->text;
    if (!text)
        text=w->name;
    if (text) {
        text=g_strdup_printf("%s%c", text, questionmark ? '?':'\0');
        navit_say(this->nav, text);
        g_free(text);
    }
}





void gui_internal_back(struct gui_priv *this, struct widget *w, void *data) {
    gui_internal_prune_menu_count(this, 1, 1);
}

void gui_internal_cmd_return(struct gui_priv *this, struct widget *wm, void *data) {
    gui_internal_prune_menu(this, wm->data);
}



void gui_internal_cmd_main_menu(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w=this->root.children->data;
    if (w && w->menu_data && w->menu_data->href && !strcmp(w->menu_data->href,"#Main Menu"))
        gui_internal_prune_menu(this, w);
    else
        gui_internal_html_main_menu(this);
}


struct widget *
gui_internal_time_help(struct gui_priv *this) {
    struct widget *w,*wc,*wcn;
    char timestr[64];
    struct tm *tm;
    time_t timep;

    w=gui_internal_box_new(this, gravity_right_center|orientation_horizontal|flags_fill);
    w->bl=this->spacing;
    w->spx=this->spacing;
    w->spx=10;
    w->bl=10;
    w->br=10;
    w->bt=6;
    w->bb=6;
    if (this->flags & 64) {
        wc=gui_internal_box_new(this, gravity_right_top|orientation_vertical|flags_fill);
        wc->bl=10;
        wc->br=20;
        wc->bt=6;
        wc->bb=6;
        timep=time(NULL);
        tm=localtime(&timep);
        strftime(timestr, 64, "%H:%M %d.%m.%Y", tm);
        wcn=gui_internal_label_new(this, timestr);
        gui_internal_widget_append(wc, wcn);
        gui_internal_widget_append(w, wc);
    }
    if (this->flags & 128) {
        wcn=gui_internal_button_new_with_callback(this, _("Help"), image_new_l(this, "gui_help"),
                gravity_center|orientation_vertical|flags_fill, NULL, NULL);
        gui_internal_widget_append(w, wcn);
    }
    return w;
}


/**
 * Applies the configuration values to this based on the settings
 * specified in the configuration file (this->config) and
 * the most appropriate default profile based on screen resolution.
 *
 * This function should be run after this->root is setup and could
 * be rerun after the window is resized.
 *
 * @author Steve Singer <ssinger_pg@sympatico.ca> (09/2008)
 */
void gui_internal_apply_config(struct gui_priv *this) {
    struct gui_config_settings *  current_config=0;

    dbg(lvl_debug,"w=%d h=%d", this->root.w, this->root.h);
    /*
     * Select default values from profile based on the screen.
     */
    if((this->root.w > 320 || this->root.h > 320) && this->root.w > 240 && this->root.h > 240) {
        if((this->root.w > 640 || this->root.h > 640) && this->root.w > 480 && this->root.h > 480 ) {
            current_config = &config_profiles[LARGE_PROFILE];
        } else {
            current_config = &config_profiles[MEDIUM_PROFILE];
        }
    } else {
        current_config = &config_profiles[SMALL_PROFILE];
    }

    /*
     * Apply override values from config file
     */
    if(this->config.font_size == -1 ) {
        this->font_size = current_config->font_size;
    } else {
        this->font_size = this->config.font_size;
    }

    if(this->config.icon_xs == -1 ) {
        this->icon_xs = current_config->icon_xs;
    } else {
        this->icon_xs = this->config.icon_xs;
    }

    if(this->config.icon_s == -1 ) {
        this->icon_s = current_config->icon_s;
    } else {
        this->icon_s = this->config.icon_s;
    }
    if(this->config.icon_l == -1 ) {
        this->icon_l = current_config->icon_l;
    } else {
        this->icon_l = this->config.icon_l;
    }
    if(this->config.spacing == -1 ) {
        this->spacing = current_config->spacing;
    } else {
        this->spacing = this->config.spacing;
        dbg(lvl_info, "Overriding default spacing %d with value %d provided in config file", current_config->spacing,
            this->config.spacing);
    }
    if (!this->fonts[0]) {
        int i,sizes[]= {100,66,50};
        for (i = 0 ; i < 3 ; i++) {
            if (this->font_name)
                this->fonts[i]=graphics_named_font_new(this->gra,this->font_name,this->font_size*sizes[i]/100,1);
            else
                this->fonts[i]=graphics_font_new(this->gra,this->font_size*sizes[i]/100,1);
        }
    }

}





static void gui_internal_cmd_set_destination(struct gui_priv *this, struct widget *wm, void *data) {
    char *name=data;
    dbg(lvl_info,"c=%d:0x%x,0x%x", wm->c.pro, wm->c.x, wm->c.y);
    navit_set_destination(this->nav, &wm->c, name, 1);
    if (this->flags & 512) {
        struct attr follow;
        follow.type=attr_follow;
        follow.u.num=180;
        navit_set_attr(this->nav, &this->osd_configuration);
        navit_set_attr(this->nav, &follow);
        navit_zoom_to_route(this->nav, 0);
    }
    gui_internal_prune_menu(this, NULL);
}

static void gui_internal_cmd_insert_destination_do(struct gui_priv *this, struct widget *wm, void *data) {
    char *name=data;
    int dstcount=navit_get_destination_count(this->nav)+1;
    int pos,i;
    struct pcoord *dst=g_alloca(dstcount*sizeof(struct pcoord));
    dstcount=navit_get_destinations(this->nav,dst,dstcount);

    pos=dstcount-wm->datai;
    if(pos<0)
        pos=0;

    for(i=dstcount; i>pos; i--)
        dst[i]=dst[i-1];

    dst[pos]=wm->c;
    navit_add_destination_description(this->nav,&wm->c,(char*)data);
    navit_set_destinations(this->nav,dst,dstcount+1,name,1);
    gui_internal_prune_menu(this, NULL);
}

/*
 * @brief Displays a waypoint list to the user.
 *
 * This display a waypoint list to the user. When the user chooses an item from the list, the callback
 * function passed as {@code cmd} will be called.
 *
 * Widget passed as wm parameter of the called cmd function will have item set to user chosen waypoint item. Its data will be set
 *  to zero-based chosen waypoint number, counting from the route end. Coordinates to wm->c will be copied from wm_->c if wm_ is not null. Otherwise,
 *  waypoint coordinates will be copied to wm->c.
 *
 * @param this gui context
 * @param title Menu title
 * @param hint Text to display above the waypoint list describing the action to be performed, can be NULL
 * @param wm_ The called widget pointer. Can be NULL.
 * @param cmd Callback function which will be called on item selection
 * @param data data argument to be passed to the callback function
 */
void gui_internal_select_waypoint(struct gui_priv *this, const char *title, const char *hint, struct widget *wm_,
                                  void(*cmd)(struct gui_priv *priv, struct widget *widget, void *data),void *data) {
    struct widget *wb,*w,*wtable,*row,*wc;
    struct map *map;
    struct map_rect *mr;
    struct item *item;
    char *text;
    int i;
    int dstcount=navit_get_destination_count(this->nav)+1;

    map=route_get_map(navit_get_route(this->nav));
    if(!map)
        return;
    mr = map_rect_new(map, NULL);
    if(!mr)
        return;

    wb=gui_internal_menu(this, title);
    w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    if(hint)
        gui_internal_widget_append(w, gui_internal_label_new(this, hint));
    wtable = gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);
    gui_internal_widget_append(w,wtable);

    i=0;
    while((item = map_rect_get_item(mr))!=NULL) {
        struct attr attr;
        if(item->type!=type_waypoint && item->type!=type_route_end)
            continue;
        if (item_attr_get(item, attr_label, &attr)) {
            text=g_strdup_printf(_("Waypoint %s"), map_convert_string_tmp(item->map, attr.u.str));
        } else
            continue;
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,	wc=gui_internal_button_new_with_callback(this, text,
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           cmd, data));
        wc->item=*item;
        if(wm_)
            wc->c=wm_->c;
        else {
            struct coord c;
            item_coord_get(item,&c,1);
            wc->c.x=c.x;
            wc->c.y=c.y;
            wc->c.pro=map_projection(item->map);
        }
        i++;
        wc->datai=dstcount-i;
        g_free(text);
    }
    map_rect_destroy(mr);
    gui_internal_menu_render(this);
}

static void gui_internal_cmd_insert_destination(struct gui_priv *this, struct widget *wm, void *data) {
    gui_internal_select_waypoint(this, data, _("Select waypoint to insert the new one before"), wm,
                                 gui_internal_cmd_insert_destination_do, data);
}



static void gui_internal_cmd_set_position(struct gui_priv *this, struct widget *wm, void *data) {
    struct attr v;
    if(data) {
        v.type=attr_vehicle;
        v.u.vehicle=NULL;
        navit_set_attr(this->nav, &v);
    }
    navit_set_position(this->nav, &wm->c);
    gui_internal_prune_menu(this, NULL);
}






/**
 * @brief Generic notification function for Editable widgets to call Another widget notification function when Enter is pressed in editable field.
 * The Editable widget should have data member pointing to the Another widget.
 */
void gui_internal_call_linked_on_finish(struct gui_priv *this, struct widget *wm, void *data) {
    if (wm->reason==gui_internal_reason_keypress_finish && data) {
        struct widget *w=data;
        if(w->func)
            w->func(this, w, w->data);
    }
}

struct widget * gui_internal_keyboard(struct gui_priv *this, int mode);


struct widget * gui_internal_keyboard_show_native(struct gui_priv *this, struct widget *w, int mode, char *lang);


static void gui_internal_cmd_delete_bookmark(struct gui_priv *this, struct widget *wm, void *data) {
    struct attr mattr;
    GList *l;
    navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL);
    bookmarks_delete_bookmark(mattr.u.bookmarks,wm->text);
    l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
    gui_internal_prune_menu(this, l->data);
}




/**
 * @brief Remove the case in a string
 *
 * @warning Result should be g_free()d after use.
 *
 * @param s The input utf-8 string
 * @return An equivalent string prepared for case insensitive search
 */
char *removecase(char *s) {
    char *r;
    r=linguistics_casefold(s);
    return r;
}

/**
 * @brief Apply the command "View on Map", centers the map on the selected point and highlight this point using
 * type_found_item style
 *
 * @param this The GUI context
 * @param wm The widget that points to this function as a callback
 * @param data Private data provided during callback (unused)
 */
static void gui_internal_cmd_view_on_map(struct gui_priv *this, struct widget *wm, void *data) {

    struct widget *w;
    struct widget *wr;
    struct widget *wi;
    char *label;

    if (wm->item.type != type_none) {
        enum item_type type;
        if (wm->item.type < type_line)
            type=type_selected_point;
        else if (wm->item.type < type_area)
            type=type_selected_point;
        else
            type=type_selected_area;
        graphics_clear_selection(this->gra, NULL);
        graphics_add_selection(this->gra, &wm->item, type, NULL);
    } else {
        if (wm->item.priv_data)
            label = wm->item.priv_data;	/* Use the label of the point to view on map */
        else
            label = g_strdup("");
        w = gui_internal_widget_table_new(this, 0, 0);	/* Create a basic table */
        gui_internal_widget_append(w,wr=gui_internal_widget_table_row_new(this,0));	/* In this table, add one row */
        gui_internal_widget_append(wr,wi=gui_internal_box_new_with_label(this,0,
                                         label));	/* That row contains a widget of type widget_box */
        wi->name = label;	/* Use the label of the point to view on map */
        wi->c.x=wm->c.x;	/* Use the coordinates of the point to place it on the map */
        wi->c.y=wm->c.y;
        gui_internal_prepare_search_results_map(this, w, NULL);
        g_free(label);
        wi->name = NULL;
        gui_internal_widget_destroy(this, w);
    }
    navit_set_center(this->nav, &wm->c, 1);
    gui_internal_prune_menu(this, NULL);
}


static void gui_internal_cmd_view_attribute_details(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w,*wb;
    struct map_rect *mr;
    struct item *item;
    struct attr attr;
    char *text,*url;
    int i;

    text=g_strdup_printf("Attribute %s",wm->name);
    wb=gui_internal_menu(this, text);
    g_free(text);
    w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    mr=map_rect_new(wm->item.map, NULL);
    item = map_rect_get_item_byid(mr, wm->item.id_hi, wm->item.id_lo);
    for (i = 0 ; i < wm->datai ; i++) {
        item_attr_get(item, attr_any, &attr);
    }
    if (item_attr_get(item, attr_any, &attr)) {
        url=NULL;
        switch (attr.type) {
        case attr_osm_nodeid:
            url=g_strdup_printf("http://www.openstreetmap.org/browse/node/"LONGLONG_FMT"\n",*attr.u.num64);
            break;
        case attr_osm_wayid:
            url=g_strdup_printf("http://www.openstreetmap.org/browse/way/"LONGLONG_FMT"\n",*attr.u.num64);
            break;
        case attr_osm_relationid:
            url=g_strdup_printf("http://www.openstreetmap.org/browse/relation/"LONGLONG_FMT"\n",*attr.u.num64);
            break;
        default:
            break;
        }
        if (url) {
            gui_internal_widget_append(w,
                                       wb=gui_internal_button_new_with_callback(this, _("View in Browser"),
                                               image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                               gui_internal_cmd_view_in_browser, NULL));
            wb->name=url;
        }
    }
    map_rect_destroy(mr);
    gui_internal_menu_render(this);
}

static void gui_internal_cmd_view_attributes(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w,*wb;
    struct map_rect *mr;
    struct item *item;
    struct attr attr;
    char *text;
    int count=0;

    dbg(lvl_info,"item=%p 0x%x 0x%x", wm->item.map,wm->item.id_hi, wm->item.id_lo);
    wb=gui_internal_menu(this, "Attributes");
    w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    mr=map_rect_new(wm->item.map, NULL);
    item = map_rect_get_item_byid(mr, wm->item.id_hi, wm->item.id_lo);
    dbg(lvl_info,"item=%p", item);
    if (item) {
        text=g_strdup_printf("%s:%s", _("Item type"), item_to_name(item->type));
        gui_internal_widget_append(w,
                                   wb=gui_internal_button_new(this, text,
                                           NULL, gravity_left_center|orientation_horizontal|flags_fill));
        wb->name=g_strdup(text);
        wb->item=wm->item;
        g_free(text);
        while(item_attr_get(item, attr_any, &attr)) {
            char *attrtxt;
            text=g_strdup_printf("%s:%s", attr_to_name(attr.type), attrtxt=attr_to_text(&attr, wm->item.map, 1));
            g_free(attrtxt);
            gui_internal_widget_append(w,
                                       wb=gui_internal_button_new_with_callback(this, text,
                                               NULL, gravity_left_center|orientation_horizontal|flags_fill,
                                               gui_internal_cmd_view_attribute_details, NULL));
            wb->name=g_strdup(text);
            wb->item=wm->item;
            wb->datai=count++;
            g_free(text);
        }
        text=g_strdup_printf("%s:0x%x,0x%x", "ID", item->id_hi, item->id_lo);
        gui_internal_widget_append(w,
                                   wb=gui_internal_button_new(this, text,
                                           NULL, gravity_left_center|orientation_horizontal|flags_fill));
        wb->name=text;
        wb->item=wm->item;
    }
    map_rect_destroy(mr);
    gui_internal_menu_render(this);
}

static void gui_internal_cmd_view_in_browser(struct gui_priv *this, struct widget *wm, void *data) {
    struct map_rect *mr;
    struct item *item;
    struct attr attr;
    char *cmd=NULL;

    if (!wm->name) {
        dbg(lvl_info,"item=%p 0x%x 0x%x", wm->item.map,wm->item.id_hi, wm->item.id_lo);
        mr=map_rect_new(wm->item.map, NULL);
        item = map_rect_get_item_byid(mr, wm->item.id_hi, wm->item.id_lo);
        dbg(lvl_info,"item=%p", item);
        if (item) {
            while(item_attr_get(item, attr_url_local, &attr)) {
                if (! cmd)
                    cmd=g_strdup_printf("navit-browser.sh '%s' &",map_convert_string_tmp(item->map,attr.u.str));
            }
        }
        map_rect_destroy(mr);
    } else {
        cmd=g_strdup_printf("navit-browser.sh '%s' &",wm->name);
    }
    if (cmd) {
#ifdef HAVE_SYSTEM
        system(cmd);
#else
        dbg(lvl_error,"Error: External commands were disabled during compilation, cannot call '%s'.",cmd);
#endif
        g_free(cmd);
    }
}

/**
 * @brief Create a map rect highlighting one of multiple points provided in argument @data and displayed using
 *        the style type_found_item (name for each point will also be displayed aside)
 *
 * @param this The GUI context
 * @param table A table widget or any of its descendants. The table contain results to place on the map.
 *              Providing NULL here will remove all previous results from the map.
 * @param[out] r The minimum rect focused to contain all results placed  on the map (or unchanged if r==NULL)
 */
static void gui_internal_prepare_search_results_map(struct gui_priv *this, struct widget *table, struct coord_rect *r) {
    struct widget *w;
    GList *l;	/* Cursor in the list of widgets */
    GList* list = NULL;	/* List we will create to store the points to add to the result map */
    struct attr a;
    GList* p;

    this->results_map_population=0;

    /* Find the table to populate the map */
    for(w=table; w && w->type!=widget_table; w=w->parent);

    if(!w) {
        dbg(lvl_warning,"Can't find the results table - only map clean up is done.");
    } else {
        /* Create a GList containing all search results */
        for(l=w->children; l; l=g_list_next(l)) {
            struct widget *wr=l->data;
            if(wr->type==widget_table_row) {
                struct widget *wi=wr->children->data;
                if(wi->name==NULL)
                    continue;
                struct lcoord *result = g_new0(struct lcoord, 1);
                result->c.x=wi->c.x;
                result->c.y=wi->c.y;
                result->label=g_strdup(wi->name);
                list = g_list_prepend(list, result);
            }
        }
    }
    this->results_map_population=navit_populate_search_results_map(this->nav, list, r);
    /* Parse the GList starting at list and free all payloads before freeing the list itself */
    if (list) {
        for(p=list; p; p=g_list_next(p)) {
            if (((struct lcoord *)(p->data))->label)
                g_free(((struct lcoord *)(p->data))->label);
        }
    }
    g_list_free(list);
    if(!this->results_map_population)
        return;
    a.type=attr_orientation;
    a.u.num=0;
    navit_set_attr(this->nav,&a);	/* Set orientation to North */
    if (r) {
        navit_zoom_to_rect(this->nav,r);
        gui_internal_prune_menu(this, NULL);
    }
}

/**
 * @brief Apply the command "Show results on the map", highlighting one of multiple points using
 *        type_found_item style (with their respective name placed aside)
 *
 * @param this The GUI context
 * @param wm The widget that called us
 * @param data Private data provided during callback (should be a pointer to the table widget containing results,
 *             or NULL to remove all previous results from the map).
 */
static void gui_internal_cmd_results_to_map(struct gui_priv *this, struct widget *wm, void *data) {
    struct coord_rect r;

    gui_internal_prepare_search_results_map(this, (struct widget *)data, &r);
}

/*
 * @brief Removes all existing search results from a map.
 *
 * @param this The GUI context
 * @param wm The widget that called us
 * @param data Private data (unused).
 */
static void gui_internal_cmd_results_map_clean(struct gui_priv *this, struct widget *wm, void *data) {
    gui_internal_cmd_results_to_map(this,wm,NULL);
    gui_internal_prune_menu(this, NULL);
    navit_draw(this->nav);
}

static void gui_internal_cmd_delete_waypoint(struct gui_priv *this, struct widget *wm, void *data) {
    int dstcount=navit_get_destination_count(this->nav);
    int i;
    struct map_rect *mr;
    struct item *item;
    struct pcoord *dst=g_alloca(dstcount*sizeof(struct pcoord));
    dstcount=navit_get_destinations(this->nav,dst,dstcount);
    mr=map_rect_new(wm->item.map, NULL);
    i=0;
    while((item=map_rect_get_item(mr))!=NULL) {
        struct coord c;
        if(item->type!=type_waypoint && item->type!=type_route_end)
            continue;
        if(item_is_equal_id(*item,wm->item))
            continue;
        item_coord_get_pro(item,&c,1,projection_mg);
        dst[i].x=c.x;
        dst[i].y=c.y;
        dst[i].pro=projection_mg;
        i++;
    }
    map_rect_destroy(mr);
    navit_set_destinations(this->nav,dst,i,NULL,1);
    gui_internal_prune_menu(this, NULL);
}


/**
 * @brief Displays the commands available for a location.
 *
 * This displays the available commands for the given location in a dialog from which the user can
 * choose an action. The location can be supplied either in projected coordinates via the {@code pc_in}
 * argument or in WGS84 coordinates (i.e. latitude and longitude in degrees) via the {@code g_in}
 * argument. One of these must be supplied, the other should be {@code NULL}.
 *
 * @param this The GUI context
 * @param pc_in Projected coordinates of the position
 * @param g_in WGS84 coordinates of the position
 * @param wm The widget that points to this function as a callback
 * @param name The display name for the position
 * @param flags Flags specifying the operations available from the GUI
 */
/* meaning of the bits in "flags":
 * 1: "Streets"
 * 2: "House numbers"
 * 4: "View in Browser", "View Attributes"
 * 8: "Set as dest."
 * 16: "Set as pos."
 * 32: "Add as bookm."
 * 64: "POIs"
 * 128: "View on Map"
 * 256: POIs around this point, "Drop search results from the map"
 * 512: "Cut/Copy... bookmark"
 * 1024: "Jump to attributes of top item within this->radius pixels of this point (implies flags|=256)"
 * 2048: "Show search results on the map"
 * TODO define constants for these values
 */
void gui_internal_cmd_position_do(struct gui_priv *this, struct pcoord *pc_in, struct coord_geo *g_in,
                                  struct widget *wm, const char *name, int flags) {
    struct widget *wb,*w,*wtable,*row,*wc,*wbc,*wclosest=NULL;
    struct coord_geo g;
    struct pcoord pc;
    struct coord c;

    if (pc_in) {
        pc=*pc_in;
        c.x=pc.x;
        c.y=pc.y;
        dbg(lvl_info,"x=0x%x y=0x%x", c.x, c.y);
        transform_to_geo(pc.pro, &c, &g);
    } else if (g_in) {
        struct attr attr;
        if (!navit_get_attr(this->nav, attr_projection, &attr, NULL))
            return;
        g=*g_in;
        pc.pro=attr.u.projection;
        transform_from_geo(pc.pro, &g, &c);
        pc.x=c.x;
        pc.y=c.y;
    } else
        return;

    wb=gui_internal_menu(this, name);
    w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    char coord_str[32];
    pcoord_format_short(&pc, coord_str, sizeof(coord_str), " ");
    gui_internal_widget_append(w, gui_internal_label_new(this, coord_str));
    wtable = gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);
    gui_internal_widget_append(w,wtable);

    if ((flags & 1) && wm) {
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wc=gui_internal_button_new_with_callback(this, _("Streets"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_search_street_in_town, wm));
        wc->item=wm->item;
        wc->selection_id=wm->selection_id;
    }
    if ((flags & 2) && wm) {
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wc=gui_internal_button_new_with_callback(this, _("House numbers"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_search_house_number_in_street, wm));
        wc->item=wm->item;
        wc->selection_id=wm->selection_id;
    }
    if ((flags & 4) && wm) {
        struct map_rect *mr;
        struct item *item;
        struct attr attr;
        mr=map_rect_new(wm->item.map, NULL);
        item = map_rect_get_item_byid(mr, wm->item.id_hi, wm->item.id_lo);
        if (item) {
            if (item_attr_get(item, attr_description, &attr))
                gui_internal_widget_append(w, gui_internal_label_new(this, map_convert_string_tmp(item->map,attr.u.str)));
            if (item_attr_get(item, attr_url_local, &attr)) {
                gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                                      gravity_left|orientation_horizontal|flags_fill));
                gui_internal_widget_append(row,
                                           wb=gui_internal_button_new_with_callback(this, _("View in Browser"),
                                                   image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                                   gui_internal_cmd_view_in_browser, NULL));
                wb->item=wm->item;
            }
            gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                                  gravity_left|orientation_horizontal|flags_fill));
            gui_internal_widget_append(row,
                                       wb=gui_internal_button_new_with_callback(this, _("View Attributes"),
                                               image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                               gui_internal_cmd_view_attributes, NULL));
            wb->item=wm->item;
        }
        map_rect_destroy(mr);
    }
    if (flags & 8) {
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, _("Set as destination"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_set_destination, g_strdup(name)));
        wbc->data_free=g_free_func;
        wbc->c=pc;
        if(navit_get_destination_count(this->nav)>=1) {
            gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                                  gravity_left|orientation_horizontal|flags_fill));
            gui_internal_widget_append(row,
                                       wbc=gui_internal_button_new_with_callback(this, _("Visit before..."),
                                               image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                               gui_internal_cmd_insert_destination, g_strdup(name)));
            wbc->data_free=g_free_func;
            wbc->c=pc;
        }
    }
    if (flags & 16) {
        const char *text;
        struct attr vehicle, source;
        int deactivate=0;
        if (navit_get_attr(this->nav, attr_vehicle, &vehicle, NULL) && vehicle.u.vehicle &&
                !(vehicle_get_attr(vehicle.u.vehicle, attr_source, &source, NULL) && source.u.str && !strcmp("demo://",source.u.str)))
            deactivate=1;

        text=deactivate? _("Set as position (and deactivate vehicle)") : _("Set as position");

        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, text,
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_set_position, (void*)(long)deactivate));
        wbc->c=pc;
    }
    if (flags & 32) {
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, _("Add as bookmark"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_add_bookmark2, g_strdup(name)));
        wbc->data_free=g_free_func;
        wbc->c=pc;
    }
#ifndef _MSC_VER
//POIs are not operational under MSVC yet
    if (flags & 64) {
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, _("POIs"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_pois, NULL));
        wbc->c=pc;
    }
#endif /* _MSC_VER */
#if 0
    gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                          gravity_left|orientation_horizontal|flags_fill));
    gui_internal_widget_append(row,
                               gui_internal_button_new(this, "Add to tour",
                                       image_new_o(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill));
#endif
    if (flags & 128) {
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, _("View on map"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_view_on_map, NULL));
        wbc->c=pc;
        if ((flags & 4) && wm) {
            wbc->item=wm->item;
        } else {
            wbc->item.type=type_none;
            wbc->item.priv_data = g_strdup(name); /* Will be freed up by gui_internal_cmd_view_on_map() */
        }
    }
    if(flags & 256 && this->results_map_population) {
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, _("Remove search results from the map"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_results_map_clean, NULL));
        wbc->data=wm;
    }
    if(flags & 2048) {
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, _("Show results on the map"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_results_to_map, NULL));
        wbc->data=wm;
    }
    if ((flags & 256) || (flags & 1024)) {
        struct displaylist_handle *dlh;
        struct displaylist *display;
        struct attr attr;
        struct point p;
        struct transformation *trans;

        char *text;
        struct map_selection *sel;
        GList *l, *ll;

        c.x=pc.x;
        c.y=pc.y;

        trans=navit_get_trans(this->nav);
        transform(trans,pc.pro,&c,&p,1,0,0,0);
        display=navit_get_displaylist(this->nav);
        dlh=graphics_displaylist_open(display);
        sel=displaylist_get_selection(display);
        l=displaylist_get_clicked_list(display, &p, this->radius);
        for(ll=l; ll; ll=g_list_next(ll)) {
            struct displayitem *di;
            struct item *item;
            struct map_rect *mr;
            struct item *itemo;

            di=(struct displayitem*)ll->data;
            item=graphics_displayitem_get_item(di);

            mr=map_rect_new(item->map, sel);
            itemo=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
            if(!itemo) {
                map_rect_destroy(mr);
                continue;
            }
            if (item_attr_get(itemo, attr_label, &attr)) {
                text=g_strdup(map_convert_string_tmp(itemo->map, attr.u.str));
            } else
                text=g_strdup(item_to_name(item->type));
            gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                                  gravity_left|orientation_horizontal|flags_fill));
            gui_internal_widget_append(row,	wc=gui_internal_cmd_pois_item(this, NULL, itemo, NULL, NULL, -1, text));
            wc->c=pc;
            g_free(wc->name);
            wc->name=g_strdup(text);
            wc->item=*itemo;
            g_free(text);
            map_rect_destroy(mr);
            if(!wclosest)
                wclosest=wc;

        }
        g_list_free(l);
        map_selection_destroy(sel);
        graphics_displaylist_close(dlh);
    }
    if (flags & 512) {
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, _("Cut Bookmark"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_cut_bookmark, NULL));
        wbc->text=g_strdup(wm->text);
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, _("Copy Bookmark"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_copy_bookmark, NULL));
        wbc->text=g_strdup(wm->text);
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, _("Rename Bookmark"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_rename_bookmark, NULL));
        wbc->text=g_strdup(wm->text);
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, _("Paste Bookmark"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_paste_bookmark, NULL));
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, _("Delete Bookmark"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_delete_bookmark, NULL));
        wbc->text=g_strdup(wm->text);
    }

    if (wm && (wm->item.type==type_waypoint || wm->item.type==type_route_end)) {
        gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,
                                              gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   wbc=gui_internal_button_new_with_callback(this, _("Delete waypoint"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_delete_waypoint, NULL));
        wbc->item=wm->item;
    }

    gui_internal_menu_render(this);

    if((flags & 1024) && wclosest)
        gui_internal_cmd_view_attributes(this,wclosest,wclosest->data);
}


/* wm->data:	0 Nothing special
		1 Map Point
		2 Item
		3 Town
		4 County
		5 Street
		6 House number
		7 Bookmark
		8 Former destination
		9 Item from the POI list
*/

void gui_internal_cmd_position(struct gui_priv *this, struct widget *wm, void *data) {
    int flags;

    if(!data)
        data=wm->data;

    switch ((long) data) {
    case 0:
        flags=8|16|32|64|128|256;
        break;
    case 1:
        flags=8|16|32|64|256;
        break;
    case 2:
        flags=4|8|16|32|64|128;
        break;
    case 3:
        flags=1|4|8|16|32|64|128|2048;
        flags &= this->flags_town;
        break;
    case 4:
        gui_internal_search_town_in_country(this, wm);
        return;
    case 5:
        flags=2|8|16|32|64|128|2048;
        flags &= this->flags_street;
        break;
    case 6:
        flags=8|16|32|64|128|2048;
        flags &= this->flags_house_number;
        break;
    case 7:
        flags=8|16|64|128|512;
        break;
    case 8:
        flags=8|16|32|64|128;
        break;
    case 9:
        flags=4|8|16|32|64|128|2048;
        break;
    default:
        return;
    }
    switch (flags) {
    case 2:
        gui_internal_search_house_number_in_street(this, wm, NULL);
        return;
    case 8:
        gui_internal_cmd_set_destination(this, wm, NULL);
        return;
    }
    gui_internal_cmd_position_do(this, &wm->c, NULL, wm, wm->name ? wm->name : wm->text, flags);
}





/**
  * The "Bookmarks" section of the OSD
  *
  */
void gui_internal_cmd_bookmarks(struct gui_priv *this, struct widget *wm, void *data) {
    struct attr attr,mattr;
    struct item *item;
    char *label_full,*prefix=0;
    int plen=0,hassub,found=0;
    struct widget *wb,*w,*wbm;
    struct coord c;
    struct widget *tbl, *row;

    if (data)
        prefix=g_strdup(data);
    else {
        if (wm && wm->prefix)
            prefix=g_strdup(wm->prefix);
    }
    if ( prefix )
        plen=strlen(prefix);

    gui_internal_prune_menu_count(this, 1, 0);
    wb=gui_internal_menu(this, _("Bookmarks"));
    wb->background=this->background;
    w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    //w->spy=this->spacing*3;
    gui_internal_widget_append(wb, w);

    if(navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL) ) {
        if (!plen) {
            bookmarks_move_root(mattr.u.bookmarks);
        } else {
            if (!strcmp(prefix,"..")) {
                bookmarks_move_up(mattr.u.bookmarks);
                g_free(prefix);
                prefix=g_strdup(bookmarks_item_cwd(mattr.u.bookmarks));
                if (prefix) {
                    plen=strlen(prefix);
                } else {
                    plen=0;
                }
            } else {
                bookmarks_move_down(mattr.u.bookmarks,prefix);
            }

            // "Back" button, when inside a bookmark folder

            if (plen) {
                wbm=gui_internal_button_new_with_callback(this, "..",
                        image_new_xs(this, "gui_inactive"), gravity_left_center|orientation_horizontal|flags_fill,
                        gui_internal_cmd_bookmarks, NULL);
                wbm->prefix=g_strdup("..");
                gui_internal_widget_append(w, wbm);

                // load bookmark folder as Waypoints, if any
                if (bookmarks_get_bookmark_count(mattr.u.bookmarks) > 0) {
                    wbm=gui_internal_button_new_with_callback(this, _("Bookmarks as waypoints"),
                            image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                            gui_internal_cmd_load_bookmarks_as_waypoints, NULL);
                    wbm->prefix=g_strdup(prefix);
                    gui_internal_widget_append(w, wbm);
                }

                // save Waypoints in bookmark folder, if route exists
                if (navit_get_destination_count(this->nav) > 0) {
                    if (bookmarks_get_bookmark_count(mattr.u.bookmarks)==0) {
                        wbm=gui_internal_button_new_with_callback(this, _("Save waypoints"),
                                image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                gui_internal_cmd_replace_bookmarks_from_waypoints, NULL);
                    } else {
                        wbm=gui_internal_button_new_with_callback(this, _("Replace with waypoints"),
                                image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                gui_internal_cmd_replace_bookmarks_from_waypoints, NULL);
                    }
                    wbm->prefix=g_strdup(prefix);
                    gui_internal_widget_append(w, wbm);
                }

                // delete empty folder
                if (bookmarks_get_bookmark_count(mattr.u.bookmarks)==0) {
                    gui_internal_widget_append(w,
                                               wbm=gui_internal_button_new_with_callback(this, _("Delete Folder"),
                                                       image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                                       gui_internal_cmd_delete_bookmark_folder, NULL));
                    wbm->prefix=g_strdup(prefix);
                }

            }
        }

        // Adds the Bookmark folders
        wbm=gui_internal_button_new_with_callback(this, _("Add Bookmark folder"),
                image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                gui_internal_cmd_add_bookmark_folder2, NULL);
        gui_internal_widget_append(w, wbm);

        // Pastes the Bookmark
        wbm=gui_internal_button_new_with_callback(this, _("Paste bookmark"),
                image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                gui_internal_cmd_paste_bookmark, NULL);
        gui_internal_widget_append(w, wbm);

        bookmarks_item_rewind(mattr.u.bookmarks);

        tbl=gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);
        gui_internal_widget_append(w,tbl);

        while ((item=bookmarks_get_item(mattr.u.bookmarks))) {
            if (!item_attr_get(item, attr_label, &attr)) continue;
            label_full=map_convert_string_tmp(item->map,attr.u.str);
            dbg(lvl_info,"full_labled: %s",label_full);

            // hassub == 1 if the item type is a sub-folder
            if (item->type == type_bookmark_folder) {
                hassub=1;
            } else {
                hassub=0;
            }

            row=gui_internal_widget_table_row_new(this,gravity_left| flags_fill| orientation_horizontal);
            gui_internal_widget_append(tbl, row);
            wbm=gui_internal_button_new_with_callback(this, label_full,
                    image_new_xs(this, hassub ? "gui_inactive" : "gui_active" ), gravity_left_center|orientation_horizontal|flags_fill,
                    hassub ? gui_internal_cmd_bookmarks : gui_internal_cmd_position, NULL);

            gui_internal_widget_append(row,wbm);
            if (item_coord_get(item, &c, 1)) {
                wbm->c.x=c.x;
                wbm->c.y=c.y;
                wbm->c.pro=bookmarks_get_projection(mattr.u.bookmarks);
                wbm->name=g_strdup_printf(_("Bookmark %s"),label_full);
                wbm->text=g_strdup(label_full);
                if (!hassub) {
                    wbm->data=(void*)7;//Mark us as a bookmark
                }
                wbm->prefix=g_strdup(label_full);
            } else {
                gui_internal_widget_destroy(this, row);
            }
        }
    }

    g_free(prefix);

    if (found)
        gui_internal_check_exit(this);
    else
        gui_internal_menu_render(this);
}




static void gui_internal_keynav_highlight_next(struct gui_priv *this, int dx, int dy, int rotary);

static int gui_internal_keynav_find_next(struct widget *wi, struct widget *current_highlight, struct widget **result);

static int gui_internal_keynav_find_prev(struct widget *wi, struct widget *current_highlight, struct widget **result);

static struct widget* gui_internal_keynav_find_next_sensitive_child(struct widget *wi);

void gui_internal_keypress_do(struct gui_priv *this, char *key) {
    struct widget *wi,*menu,*search_list;
    int len=0;
    char *text=NULL;

    menu=g_list_last(this->root.children)->data;
    wi=gui_internal_find_widget(menu, NULL, STATE_EDIT);
    if (wi) {
        /* select first item of the searchlist */
        if (*key == NAVIT_KEY_RETURN) {
            search_list=gui_internal_menu_data(this)->search_list;
            if(search_list) {
                GList *l=gui_internal_widget_table_top_row(this, search_list);
                if (l && l->data) {
                    struct widget *w=l->data;
                    this->current.x=w->p.x+w->w/2;
                    this->current.y=w->p.y+w->h/2;
                    gui_internal_highlight(this);
                }
            } else {
                wi->reason=gui_internal_reason_keypress_finish;
                wi->func(this, wi, wi->data);
            }
            return;
        } else if (*key == NAVIT_KEY_BACKSPACE) {
            dbg(lvl_debug,"backspace");
            if (wi->text && wi->text[0]) {
                len=g_utf8_prev_char(wi->text+strlen(wi->text))-wi->text;
                wi->text[len]='\0';
                text=g_strdup(wi->text);
            }
        } else {
            if (wi->state & STATE_CLEAR) {
                dbg(lvl_info,"wi->state=0x%x", wi->state);
                g_free(wi->text);
                wi->text=NULL;
                wi->state &= ~STATE_CLEAR;
                dbg(lvl_info,"wi->state=0x%x", wi->state);
            }
            text=g_strdup_printf("%s%s", wi->text ? wi->text : "", key);

            gui_internal_keyboard_to_lower_case(this);
        }
        g_free(wi->text);
        wi->text=text;

        if(!wi->text || !*wi->text)
            gui_internal_keyboard_to_upper_case(this);

        if (wi->func) {
            wi->reason=gui_internal_reason_keypress;
            wi->func(this, wi, wi->data);
        }
        gui_internal_widget_render(this, wi);
    }
}




char *gui_internal_cmd_match_expand(char *pattern, struct attr **in) {
    char p,*ret=g_strdup(pattern),*r=ret,*a;
    int len;
    while ((p=*pattern++)) {
        switch (p) {
        case '*':
            *r='\0';
            a=attr_to_text(*in++,NULL,0);
            len=strlen(ret)+strlen(a)+strlen(pattern)+1;
            r=g_malloc(len);
            strcpy(r, ret);
            strcat(r, a);
            g_free(ret);
            g_free(a);
            ret=r;
            r=ret+strlen(ret);
            break;
        case '\\':
            p=*pattern++;
        default:
            *r++=p;
        }
    }
    *r++='\0';
    return ret;
}

static int gui_internal_match(const char *pattern, const char *string) {
    char p,s;
    while ((p=*pattern++)) {
        switch (p) {
        case '*':
            while ((s=*string)) {
                if (gui_internal_match(pattern,string))
                    return 1;
                string++;
            }
            break;
        case '\\':
            p=*pattern++;
        default:
            if (*string++ != p)
                return 0;
        }
    }
    return 1;
}

int gui_internal_set(char *remove, char *add) {
    char *gui_file=g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/gui_internal.txt", NULL);
    char *gui_file_new=g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/gui_internal_new.txt", NULL);
    FILE *fo=fopen(gui_file_new,"w");
    FILE *fi=fopen(gui_file,"r");
    char *line=NULL;
    int ret;
    size_t size=0;
    if (fi != NULL) {
        while (getline(&line,&size,fi) > 0) {
            int len=strlen(line);
            if (len > 0 && line[len-1] == '\n')
                line[len-1]='\0';
            dbg(lvl_debug,"line=%s",line);
            if (!gui_internal_match(remove, line))
                fprintf(fo,"%s\n",line);
        }
        if (line)
            free(line);
        fclose(fi);
    }
    if (add)
        fprintf(fo,"%s;\n",add);
    fclose(fo);
    unlink(gui_file);
    ret=(rename(gui_file_new, gui_file)==0);
    g_free(gui_file_new);
    g_free(gui_file);

    return ret;
}



static void gui_internal_window_closed(struct gui_priv *this) {
    gui_internal_cmd2_quit(this, NULL, NULL, NULL, NULL);
}


static void gui_internal_cmd_map_download_do(struct gui_priv *this, struct widget *wm, void *data) {
    char *text=g_strdup_printf(_("Download %s"),wm->name);
    struct widget *w, *wb;
    struct map *map=data;
    double bllon,bllat,trlon,trlat;

    wb=gui_internal_menu(this, text);
    g_free(text);
    w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    w->spy=this->spacing*3;
    gui_internal_widget_append(wb, w);
    if (sscanf(wm->prefix,"%lf,%lf,%lf,%lf",&bllon,&bllat,&trlon,&trlat) == 4) {
        struct coord_geo g;
        struct map_selection sel;
        struct map_rect *mr;
        struct item *item;

        sel.next=NULL;
        sel.order=255;
        g.lng=bllon;
        g.lat=trlat;
        transform_from_geo(projection_mg, &g, &sel.u.c_rect.lu);
        g.lng=trlon;
        g.lat=bllat;
        transform_from_geo(projection_mg, &g, &sel.u.c_rect.rl);
        sel.range.min=type_none;
        sel.range.max=type_last;
        mr=map_rect_new(map, &sel);
        while ((item=map_rect_get_item(mr))) {
            dbg(lvl_info,"item");
        }
        map_rect_destroy(mr);
    }

    dbg(lvl_info,"bbox=%s",wm->prefix);
    gui_internal_menu_render(this);
}

void gui_internal_cmd_map_download(struct gui_priv *this, struct widget *wm, void *data) {
    struct attr on, off, download_enabled, download_disabled;
    struct widget *w,*wb,*wma;
    struct map *map=data;
    FILE *f;
    char *search,buffer[256];
    int found,sp_match=0;

    dbg(lvl_debug,"wm=%p prefix=%s",wm,wm->prefix);

    search=wm->prefix;
    if (search) {
        found=0;
        while(search[sp_match] == ' ')
            sp_match++;
        sp_match++;
    } else {
        found=1;
    }
    on.type=off.type=attr_active;
    on.u.num=1;
    off.u.num=0;
    wb=gui_internal_menu(this, wm->name?wm->name:_("Map Download"));
    w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    w->spy=this->spacing*3;
    gui_internal_widget_append(wb, w);
    if (!search) {
        wma=gui_internal_button_map_attr_new(this, _("Active"), gravity_left_center|orientation_horizontal|flags_fill, map, &on,
                                             &off, 1);
        gui_internal_widget_append(w, wma);
    }

    download_enabled.type=download_disabled.type=attr_update;
    download_enabled.u.num=1;
    download_disabled.u.num=0;
    wma=gui_internal_button_map_attr_new(this
                                         , _("Download Enabled")
                                         , gravity_left_center|orientation_horizontal|flags_fill
                                         , map
                                         , &download_enabled
                                         , &download_disabled
                                         , 0);
    gui_internal_widget_append(w, wma);


    f=fopen("maps/areas.tsv","r");
    while (f && fgets(buffer, sizeof(buffer), f)) {
        char *nl,*description,*description_size,*bbox,*size=NULL;
        int sp=0;
        if ((nl=strchr(buffer,'\n')))
            *nl='\0';
        if ((nl=strchr(buffer,'\r')))
            *nl='\0';
        while(buffer[sp] == ' ')
            sp++;
        if ((bbox=strchr(buffer,'\t')))
            *bbox++='\0';
        if (bbox && (size=strchr(bbox,'\t')))
            *size++='\0';
        if (search && !strcmp(buffer, search)) {
            wma=gui_internal_button_new_with_callback(this, _("Download completely"), NULL,
                    gravity_left_center|orientation_horizontal|flags_fill, gui_internal_cmd_map_download_do, map);
            wma->name=g_strdup(buffer+sp);
            wma->prefix=g_strdup(bbox);
            gui_internal_widget_append(w, wma);
            found=1;
        } else if (sp < sp_match)
            found=0;
        if (sp == sp_match && found && buffer[sp]) {
            description=g_strdup(buffer+sp);
            if (size)
                description_size=g_strdup_printf("%s (%s)",description,size);
            else
                description_size=g_strdup(description);
            wma=gui_internal_button_new_with_callback(this, description_size, NULL,
                    gravity_left_center|orientation_horizontal|flags_fill, gui_internal_cmd_map_download, map);
            g_free(description_size);
            wma->prefix=g_strdup(buffer);
            wma->name=description;
            gui_internal_widget_append(w, wma);
        }
    }

    gui_internal_menu_render(this);
}

static void gui_internal_cmd_set_active_vehicle(struct gui_priv *this, struct widget *wm, void *data) {
    struct attr vehicle = {attr_vehicle,{wm->data}};
    navit_set_attr(this->nav, &vehicle);
}

static void gui_internal_cmd_show_satellite_status(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w,*wb,*row;
    struct attr attr,sat_attr;
    struct vehicle *v=wm->data;
    char *str;
    int i;
    enum attr_type types[]= {attr_sat_prn, attr_sat_elevation, attr_sat_azimuth, attr_sat_snr};

    wb=gui_internal_menu(this, _("Show Satellite Status"));
    gui_internal_menu_data(this)->redisplay=gui_internal_cmd_show_satellite_status;
    gui_internal_menu_data(this)->redisplay_widget=wm;
    w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    w = gui_internal_widget_table_new(this,gravity_center | orientation_vertical | flags_expand | flags_fill, 0);
    row = gui_internal_widget_table_row_new(this,gravity_left_top);
    gui_internal_widget_append(row, gui_internal_label_new(this, " PRN "));
    gui_internal_widget_append(row, gui_internal_label_new(this, _(" Elevation ")));
    gui_internal_widget_append(row, gui_internal_label_new(this, _(" Azimuth ")));
    gui_internal_widget_append(row, gui_internal_label_new(this, " SNR "));
    gui_internal_widget_append(w,row);
    while (vehicle_get_attr(v, attr_position_sat_item, &attr, NULL)) {
        row = gui_internal_widget_table_row_new(this,gravity_left_top);
        for (i = 0 ; i < sizeof(types)/sizeof(enum attr_type) ; i++) {
            if (item_attr_get(attr.u.item, types[i], &sat_attr))
                str=g_strdup_printf("%ld", sat_attr.u.num);
            else
                str=g_strdup("");
            gui_internal_widget_append(row, gui_internal_label_new(this, str));
            g_free(str);
        }
        gui_internal_widget_append(w,row);
    }
    gui_internal_widget_append(wb, w);
    gui_internal_menu_render(this);
}

static void gui_internal_cmd_show_nmea_data(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w,*wb;
    struct attr attr;
    struct vehicle *v=wm->data;
    wb=gui_internal_menu(this, _("Show NMEA Data"));
    gui_internal_menu_data(this)->redisplay=gui_internal_cmd_show_nmea_data;
    gui_internal_menu_data(this)->redisplay_widget=wm;
    w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    if (vehicle_get_attr(v, attr_position_nmea, &attr, NULL))
        gui_internal_widget_append(w, gui_internal_text_new(this, attr.u.str, gravity_left_center|orientation_vertical));
    gui_internal_menu_render(this);
}

/**
 * A container to hold the selected vehicle and the desired profile in
 * one data item.
 */
struct vehicle_and_profilename {
    struct vehicle *vehicle;
    char *profilename;
};

/**
 * Figures out whether the given vehicle is the active vehicle.
 *
 * @return true if the vehicle is active, false otherwise.
 */
static int gui_internal_is_active_vehicle(struct gui_priv *this, struct vehicle
        *vehicle) {
    struct attr active_vehicle;

    if (!navit_get_attr(this->nav, attr_vehicle, &active_vehicle, NULL))
        active_vehicle.u.vehicle=NULL;

    return active_vehicle.u.vehicle == vehicle;
}

static void save_vehicle_xml(struct vehicle *v) {
    struct attr attr;
    struct attr_iter *iter=vehicle_attr_iter_new();
    int childs=0;
    printf("<vehicle");
    while (vehicle_get_attr(v, attr_any_xml, &attr, iter)) {
        if (ATTR_IS_OBJECT(attr.type))
            childs=1;
        else	{
            char *attrtxt;
            printf(" %s=\"%s\"",attr_to_name(attr.type),attrtxt=attr_to_text(&attr, NULL, 1));
            g_free(attrtxt);
        }
    }
    if (childs) {
        printf(">\n");
        printf("</vehicle>\n");
    } else
        printf(" />\n");
    vehicle_attr_iter_destroy(iter);
}


/**
 * Reacts to a button press that changes a vehicle's active profile.
 *
 * @see gui_internal_add_vehicle_profile
 */
static void gui_internal_cmd_set_active_profile(struct gui_priv *this, struct
        widget *wm, void *data) {
    struct vehicle_and_profilename *vapn = data;
    struct vehicle *v = vapn->vehicle;
    char *profilename = vapn->profilename;
    struct attr vehicle_name_attr;
    char *vehicle_name = NULL;
    struct attr profilename_attr;
    struct attr vehicle;

    // Get the vehicle name
    vehicle_get_attr(v, attr_name, &vehicle_name_attr, NULL);
    vehicle_name = vehicle_name_attr.u.str;

    dbg(lvl_debug, "Changing vehicle %s to profile %s", vehicle_name, profilename);

    // Change the profile name
    profilename_attr.type = attr_profilename;
    profilename_attr.u.str = profilename;
    if(!vehicle_set_attr(v, &profilename_attr)) {
        dbg(lvl_error, "Unable to set the vehicle's profile name");
    }

    navit_set_vehicleprofile_name(this->nav,profilename);

    save_vehicle_xml(v);

    // Notify Navit that the routing should be re-done if this is the
    // active vehicle.
    if (gui_internal_is_active_vehicle(this, v)) {
        vehicle.u.vehicle=v;
    } else {

        vehicle.u.vehicle=NULL;
    }

    vehicle.type=attr_vehicle;
    navit_set_attr(this->nav, &vehicle);


    gui_internal_prune_menu_count(this, 1, 0);
    gui_internal_menu_vehicle_settings(this, v, vehicle_name);
}

/**
 * Adds the vehicle profile to the GUI, allowing the user to pick a
 * profile for the currently selected vehicle.
 */
static void gui_internal_add_vehicle_profile(struct gui_priv *this, struct widget
        *parent, struct vehicle *v, struct vehicleprofile *profile) {
    // Just here to show up in the translation file, nice and close to
    // where the translations are actually used.
    struct attr profile_attr;
    struct attr *attr = NULL;
    char *name = NULL;
    char *active_profile = NULL;
    char *label = NULL;
    int active;
    struct vehicle_and_profilename *context = NULL;

#ifdef ONLY_FOR_TRANSLATION
    char *translations[] = {_n("car"), _n("bike"), _n("pedestrian")};
#endif

    // Figure out the profile name
    attr = attr_search(profile->attrs, NULL, attr_name);
    if (!attr) {
        dbg(lvl_error, "Adding vehicle profile failed. attr==NULL");
        return;
    }
    name = attr->u.str;

    // Determine whether the profile is the active one
    if (vehicle_get_attr(v, attr_profilename, &profile_attr, NULL))
        active_profile = profile_attr.u.str;
    active = active_profile != NULL && !strcmp(name, active_profile);

    dbg(lvl_debug, "Adding vehicle profile %s, active=%s/%i", name, active_profile, active);

    // Build a translatable label.
    if(active) {
        label = g_strdup_printf(_("Current profile: %s"), _(name));
    } else {
        label = g_strdup_printf(_("Change profile to: %s"), _(name));
    }

    // Create the context object (the vehicle and the desired profile)
    context = g_new0(struct vehicle_and_profilename, 1);
    context->vehicle = v;
    context->profilename = name;

    // Add the button
    gui_internal_widget_append(parent,
                               gui_internal_button_new_with_callback(
                                   this, label,
                                   image_new_xs(this, active ? "gui_active" : "gui_inactive"),
                                   gravity_left_center|orientation_horizontal|flags_fill,
                                   gui_internal_cmd_set_active_profile, context));

    free(label);
}

void gui_internal_menu_vehicle_settings(struct gui_priv *this, struct vehicle *v, char *name) {
    struct widget *w,*wb,*row;
    struct attr attr;
    struct vehicleprofile *profile = NULL;
    GList *profiles;

    wb=gui_internal_menu(this, name);
    w=gui_internal_widget_table_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill,1);
    gui_internal_widget_append(wb, w);

    // Add the "Set as active" button if this isn't the active
    // vehicle.
    if (!gui_internal_is_active_vehicle(this, v)) {
        gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,
                                          gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   gui_internal_button_new_with_callback(this, _("Set as active"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_set_active_vehicle, v));
    }

    if (vehicle_get_attr(v, attr_position_sat_item, &attr, NULL)) {
        gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,
                                          gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   gui_internal_button_new_with_callback(this, _("Show Satellite status"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_show_satellite_status, v));
    }
    if (vehicle_get_attr(v, attr_position_nmea, &attr, NULL)) {
        gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,
                                          gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row,
                                   gui_internal_button_new_with_callback(this, _("Show NMEA data"),
                                           image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
                                           gui_internal_cmd_show_nmea_data, v));
    }

    // Add all the possible vehicle profiles to the menu
    profiles = navit_get_vehicleprofiles(this->nav);
    while(profiles) {
        profile = (struct vehicleprofile *)profiles->data;
        gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,
                                          gravity_left|orientation_horizontal|flags_fill));
        gui_internal_add_vehicle_profile(this, row, v, profile);
        profiles = g_list_next(profiles);
    }

    callback_list_call_attr_2(this->cbl, attr_vehicle, w, v);
    gui_internal_menu_render(this);
}

void gui_internal_cmd_vehicle_settings(struct gui_priv *this, struct widget *wm, void *data) {
    gui_internal_menu_vehicle_settings(this, wm->data, wm->text);
}




//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gui_internal_motion(void *data, struct point *p) {

    struct gui_priv *this=data;
    if (!this->root.children) {
        navit_handle_motion(this->nav, p);
        return;
    }
    if (!this->pressed)
        return;
    this->current=*p;
    if(!this->motion_timeout_callback)
        this->motion_timeout_callback=callback_new_1(callback_cast(gui_internal_motion_cb), this);
    if(!this->motion_timeout_event)
        this->motion_timeout_event=event_add_timeout(30,0, this->motion_timeout_callback);
}

void gui_internal_evaluate(struct gui_priv *this, const char *command) {
    if (command)
        command_evaluate(&this->self, command);
}


void gui_internal_enter(struct gui_priv *this, int ignore) {
    struct graphics *gra=this->gra;
    if (ignore != -1)
        this->ignore_button=ignore;

    navit_block(this->nav, 1);
    graphics_overlay_disable(gra, 1);
    this->root.p.x=0;
    this->root.p.y=0;
    this->root.background=this->background;
}

void gui_internal_leave(struct gui_priv *this) {
    graphics_draw_mode(this->gra, draw_mode_end);
}

void gui_internal_set_click_coord(struct gui_priv *this, struct point *p) {
    struct coord c;
    struct coord_geo g;
    struct attr attr;
    struct transformation *trans;
    attr_free(this->click_coord_geo);
    this->click_coord_geo=NULL;
    if (p) {
        trans=navit_get_trans(this->nav);
        transform_reverse(trans, p, &c);
        dbg(lvl_debug,"x=0x%x y=0x%x", c.x, c.y);
        this->clickp.pro=transform_get_projection(trans);
        this->clickp.x=c.x;
        this->clickp.y=c.y;
        transform_to_geo(this->clickp.pro, &c, &g);
        attr.u.coord_geo=&g;
        attr.type=attr_click_coord_geo;
        this->click_coord_geo=attr_dup(&attr);
    }
}

static void gui_internal_set_position_coord(struct gui_priv *this) {
    struct transformation *trans;
    struct attr attr,attrp;
    struct coord c;

    attr_free(this->position_coord_geo);
    this->position_coord_geo=NULL;
    if (navit_get_attr(this->nav, attr_vehicle, &attr, NULL) && attr.u.vehicle
            && vehicle_get_attr(attr.u.vehicle, attr_position_coord_geo, &attrp, NULL)) {
        trans=navit_get_trans(this->nav);
        this->position_coord_geo=attr_dup(&attrp);
        this->vehiclep.pro=transform_get_projection(trans);
        transform_from_geo(this->vehiclep.pro, attrp.u.coord_geo, &c);
        this->vehiclep.x=c.x;
        this->vehiclep.y=c.y;
    }
}

void gui_internal_enter_setup(struct gui_priv *this) {
    if (!this->mouse_button_clicked_on_map)
        gui_internal_set_position_coord(this);
}

void gui_internal_cmd_menu(struct gui_priv *this, int ignore, char *href) {
    dbg(lvl_debug,"enter");
    gui_internal_enter(this, ignore);
    gui_internal_enter_setup(this);
    // draw menu
    if (href)
        gui_internal_html_load_href(this, href, 0);
    else
        gui_internal_html_main_menu(this);
}



static void gui_internal_cmd_log_do(struct gui_priv *this, struct widget *widget) {
    if (widget->text && strlen(widget->text)) {
        if (this->position_coord_geo)
            navit_textfile_debug_log_at(this->nav, &this->vehiclep, "type=log_entry label=\"%s\"",widget->text);
        else
            navit_textfile_debug_log(this->nav, "type=log_entry label=\"%s\"",widget->text);
    }
    g_free(widget->text);
    widget->text=NULL;
    gui_internal_prune_menu(this, NULL);
    gui_internal_check_exit(this);
}

void gui_internal_cmd_log_clicked(struct gui_priv *this, struct widget *widget, void *data) {
    gui_internal_cmd_log_do(this, widget->data);
}


void gui_internal_check_exit(struct gui_priv *this) {
    struct graphics *gra=this->gra;
    if (! this->root.children) {
        gui_internal_search_idle_end(this);
        gui_internal_search_list_destroy(this);
        graphics_overlay_disable(gra, 0);
        if (!navit_block(this->nav, 0)) {
            if (this->redraw)
                navit_draw(this->nav);
            else
                navit_draw_displaylist(this->nav);
        }
    }
}

static int gui_internal_get_attr(struct gui_priv *this, enum attr_type type, struct attr *attr) {
    switch (type) {
    case attr_active:
        attr->u.num=this->root.children != NULL;
        break;
    case attr_click_coord_geo:
        if (!this->click_coord_geo)
            return 0;
        *attr=*this->click_coord_geo;
        break;
    case attr_position_coord_geo:
        if (!this->position_coord_geo)
            return 0;
        *attr=*this->position_coord_geo;
        break;
    case attr_pitch:
        attr->u.num=this->pitch;
        break;
    case attr_button:
        attr->u.num=this->mouse_button_clicked_on_map;
        break;
    case attr_navit:
        attr->u.navit=this->nav;
        break;
    case attr_fullscreen:
        attr->u.num=(this->fullscreen > 0);
        break;
    default:
        return 0;
    }
    attr->type=type;
    return 1;
}

static int gui_internal_add_attr(struct gui_priv *this, struct attr *attr) {
    switch (attr->type) {
    case attr_xml_text:
        g_free(this->html_text);
        this->html_text=g_strdup(attr->u.str);
        return 1;
    default:
        return 0;
    }
}

static int gui_internal_set_attr(struct gui_priv *this, struct attr *attr) {
    switch (attr->type) {
    case attr_fullscreen:
        if ((this->fullscreen > 0) != (attr->u.num > 0)) {
            graphics_draw_mode(this->gra, draw_mode_end);
            this->win->fullscreen(this->win, attr->u.num > 0);
            graphics_draw_mode(this->gra, draw_mode_begin);
        }
        this->fullscreen=attr->u.num;
        return 1;
    case attr_menu_on_map_click:
        this->menu_on_map_click=attr->u.num;
        return 1;
    case attr_on_map_click:
        g_free(this->on_map_click);
        this->on_map_click=g_strdup(attr->u.str);
        return 1;
    default:
        dbg(lvl_error,"Unknown attribute: %s",attr_to_name(attr->type));
        return 1;
    }
}

static void gui_internal_dbus_signal(struct gui_priv *this, struct point *p) {
    struct displaylist_handle *dlh;
    struct displaylist *display;
    struct displayitem *di;
    struct attr cb,**attr_list=NULL;
    int valid=0;

    display=navit_get_displaylist(this->nav);
    dlh=graphics_displaylist_open(display);
    while ((di=graphics_displaylist_next(dlh))) {
        struct item *item=graphics_displayitem_get_item(di);
        if (item_is_point(*item) && graphics_displayitem_get_displayed(di) &&
                graphics_displayitem_within_dist(display, di, p, this->radius)) {
            struct map_rect *mr=map_rect_new(item->map, NULL);
            struct item *itemo=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
            struct attr attr;
            if (itemo && item_attr_get(itemo, attr_data, &attr))
                attr_list=attr_generic_add_attr(attr_list, &attr);
            map_rect_destroy(mr);
        }
    }
    graphics_displaylist_close(dlh);
    if (attr_list && navit_get_attr(this->nav, attr_callback_list, &cb, NULL))
        callback_list_call_attr_4(cb.u.callback_list, attr_command, "dbus_send_signal", attr_list, NULL, &valid);
    attr_list_free(attr_list);
}

/**
 * @brief Converts one geo coordinate in human readable form to double value.
 *
 * @author Martin Bruns (05/2012), mdankov
 */
static int gui_internal_coordinate_parse(char *s, char plus, char minus, double *x) {
    int sign=0;
    char *degree, *minute, *second;
    double tmp;

    if(!s)
        return 0;

    if (strchr(s, minus)!=NULL)
        sign=-1;
    else if (strchr(s, plus)!=NULL)
        sign=1;

    if(!sign)
        return 0;


    /* Can't just use strtok here because ° is multibyte sequence in utf8 */
    degree=s;
    minute=strstr(s,"°");
    if(minute) {
        *minute=0;
        minute+=strlen("°");
    }

    sscanf(degree, "%lf", x);

    if(strchr(degree, plus) || strchr(degree, minus)) {
        dbg(lvl_debug,"degree %c/%c found",plus,minus);
    } else {/* DEGREES_MINUTES */
        if(!minute)
            return 0;
        minute = strtok(minute,"'");
        sscanf(minute, "%lf", &tmp);
        *x+=tmp/60;
        if(strchr(minute, plus) || strchr(minute, minus)) {
            dbg(lvl_debug,"minute %c/%c found",plus,minus);
        } else { /* DEGREES_MINUTES_SECONDS */
            second=strtok(NULL,"");
            if(!second)
                return 0;
            sscanf(second, "%lf", &tmp);
            *x+=tmp/3600;
        }
    }
    *x *= sign;
    return 1;
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Bruns (05/2012)
//##############################################################################################################
static void gui_internal_cmd_enter_coord_do(struct gui_priv *this, struct widget *widget) {
    char *lat, *lng;
    char *widgettext;
    double latitude, longitude;
    dbg(lvl_debug,"text entered:%s", widget->text);

    /* possible entry can be identical to coord_format output but only space between lat and lng is allowed */
    widgettext=g_ascii_strup(widget->text,-1);

    lat=strtok(widgettext," ");
    lng=strtok(NULL,"");

    if(!lat || !lng) {
        g_free(widgettext);
        return;
    }
    if( gui_internal_coordinate_parse(lat, 'N', 'S', &latitude)
            && gui_internal_coordinate_parse(lng, 'E', 'W', &longitude) ) {
        g_free(widgettext);
        widgettext=g_strdup_printf("%lf %lf", longitude, latitude);
        pcoord_parse(widgettext, projection_mg, &widget->c );
    } else if(!pcoord_parse(widget->text, projection_mg, &widget->c )) {
        g_free(widgettext);
        return;
    }
    g_free(widgettext);

    gui_internal_cmd_position(this, widget, (void*)8);
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Bruns (05/2012)
//##############################################################################################################
void gui_internal_cmd_enter_coord_clicked(struct gui_priv *this, struct widget *widget, void *data) {
    dbg(lvl_debug,"entered");
    gui_internal_cmd_enter_coord_do(this, widget->data);
}

/**
 * @brief Handles mouse clicks and scroll wheel movement
 *
 * @author Martin Schaller (04/2008), Stefan Klumpp (04/2008)
 */
static void gui_internal_button(void *data, int pressed, int button, struct point *p) {
    struct gui_priv *this=data;
    struct graphics *gra=this->gra;

    dbg(lvl_debug,"enter %d %d", pressed, button);
    // if still on the map (not in the menu, yet):
    dbg(lvl_debug,"children=%p ignore_button=%d",this->root.children,this->ignore_button);
    if (!this->root.children || this->ignore_button) {

        this->ignore_button=0;
        // check whether the position of the mouse changed during press/release OR if it is the scrollwheel
        if (!navit_handle_button(this->nav, pressed, button, p, NULL)) {
            dbg(lvl_debug,"navit has handled button");
            return;
        }
        dbg(lvl_debug,"menu_on_map_click=%d",this->menu_on_map_click);
        if (button != 1)
            return;
        if (this->on_map_click || this->menu_on_map_click) {
            this->mouse_button_clicked_on_map=1;
            gui_internal_set_click_coord(this, p);
            gui_internal_set_position_coord(this);
            if (this->on_map_click)
                command_evaluate(&this->self, this->on_map_click);
            else
                gui_internal_cmd_menu(this, 0, NULL);
            this->mouse_button_clicked_on_map=0;
        } else if (this->signal_on_map_click) {
            gui_internal_dbus_signal(this, p);
            return;
        }
        return;
    }


    /*
     * If already in the menu:
     */

    if (pressed) {
        this->pressed=1;
        this->current=*p;
        gui_internal_gesture_ring_clear(this);
        gui_internal_gesture_ring_add(this, p);
        gui_internal_highlight(this);
    } else {
        int dx,dy;
        gui_internal_gesture_ring_add(this, p);
        gui_internal_gesture_get_vector(this, 300, NULL, &dx, &dy);
        this->current.x=-1;
        this->current.y=-1;
        graphics_draw_mode(gra, draw_mode_begin);
        if(!gui_internal_gesture_do(this) && this->pressed!=2 && abs(dx)<this->icon_s && abs(dy)<this->icon_s)
            gui_internal_call_highlighted(this);
        this->pressed=0;
        if (!event_main_loop_has_quit()) {
            gui_internal_highlight(this);
            graphics_draw_mode(gra, draw_mode_end);
            gui_internal_check_exit(this);
        }
    }
}

static void gui_internal_setup(struct gui_priv *this) {
    struct color cbh= {0x9fff,0x9fff,0x9fff,0xffff};
    struct color cf= {0xbfff,0xbfff,0xbfff,0xffff};
    struct graphics *gra=this->gra;
    unsigned char *buffer;
    char *gui_file;
    int size;

    if (this->background)
        return;
    this->background=graphics_gc_new(gra);
    this->background2=graphics_gc_new(gra);
    this->highlight_background=graphics_gc_new(gra);
    graphics_gc_set_foreground(this->highlight_background, &cbh);
    this->foreground=graphics_gc_new(gra);
    graphics_gc_set_foreground(this->foreground, &cf);
    this->text_background=graphics_gc_new(gra);
    this->text_foreground=graphics_gc_new(gra);
    graphics_gc_set_foreground(this->background, &this->background_color);
    graphics_gc_set_foreground(this->background2, &this->background2_color);
    graphics_gc_set_foreground(this->text_background, &this->text_background_color);
    graphics_gc_set_foreground(this->text_foreground, &this->text_foreground_color);
    gui_file=g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/gui_internal.txt", NULL);
    if (file_get_contents(gui_file,&buffer,&size)) {
        char *command=g_malloc(size+1);
        strncpy(command,(const char *)buffer,size);
        command[size]=0;
        command_evaluate(&this->self, command);
        g_free(command);
        g_free(buffer);
    }
    g_free(gui_file);
}

/**
 * @brief Callback function invoked when display area is resized
 *
 * @param data A generic argument structure pointer, here we use it to store the the internal GUI context (this)
 * @param wnew The new width of the display area
 * @param hnew The new height of the display area
 *
 * @author Martin Schaller
 * @date 2008/04
 */
static void gui_internal_resize(void *data, int wnew, int hnew) {
    GList *l;
    struct widget *w;

    struct gui_priv *this=data;
    int changed=0;

    gui_internal_setup(this);

    changed=gui_internal_menu_needs_resizing(this, &(this->root), wnew, hnew);

    /*
     * If we're drawing behind system bars on Android, watching for actual size changes will not catch
     * fullscreen toggle events. As a workaround, always assume a size change if padding is supplied.
     */
    if (!changed && this->gra && graphics_get_data(this->gra, "padding"))
        changed = 1;
    navit_handle_resize(this->nav, wnew, hnew);
    if (this->root.children) {
        if (changed) {
            l = g_list_last(this->root.children);
            if (l) {
                w=l->data;
                if (!gui_internal_widget_reload_href(this,
                                                     w)) { /* If the foremost widget is a HTML menu, reload & redraw it from its href */
                    /* If not, resize the foremost widget */
                    dbg(lvl_debug, "Current GUI displayed is not a menu");
                    dbg(lvl_debug, "Will call resize with w=%d, h=%d", wnew, hnew)
                    gui_internal_menu_resize(this, wnew, hnew);
                    gui_internal_menu_render(this);
                } else {
                    dbg(lvl_debug,"Current GUI displayed is a menu");
                }
            }
        } else {
            gui_internal_menu_render(this);
        }
    }
}

static void gui_internal_keynav_point(struct widget *w, int dx, int dy, struct point *p) {
    p->x=w->p.x+w->w/2;
    p->y=w->p.y+w->h/2;
    if (dx < 0)
        p->x=w->p.x;
    if (dx > 0)
        p->x=w->p.x+w->w;
    if (dy < 0)
        p->y=w->p.y;
    if (dy > 0)
        p->y=w->p.y+w->h;
}

static struct widget* gui_internal_keynav_find_next_sensitive_child(struct widget *wi) {
    GList *l=wi->children;
    if (wi->state & STATE_OFFSCREEN)
        return NULL;
    if (wi->state & STATE_SENSITIVE)
        return wi;
    while (l) {
        struct widget* tmp = gui_internal_keynav_find_next_sensitive_child(l->data);
        if (tmp)
            return tmp;
        l=g_list_next(l);
    }
    return NULL;
}

static int gui_internal_keynav_find_next(struct widget *wi, struct widget *current_highlight, struct widget **result) {
    GList *l=wi->children;
    if (wi == current_highlight)
        return 1;
    while (l) {
        struct widget *child=l->data;
        l=g_list_next(l);
        if (gui_internal_keynav_find_next(child, current_highlight, result)) {
            while (l) {
                struct widget *new = gui_internal_keynav_find_next_sensitive_child(l->data);
                if (new) {
                    *result = new;
                    /* Found one! */
                    return 0;
                }
                l=g_list_next(l);
            }
            /* Try parent */
            return 1;
        }
    }
    return 0;
}

#define RESULT_FOUND 1
#define NO_RESULT_YET 0

static int gui_internal_keynav_find_prev(struct widget *wi, struct widget *current_highlight, struct widget **result) {
    if (wi == current_highlight && *result) {
        // Reached current widget; last widget found is the result.
        return RESULT_FOUND;
    }
    // If widget is off-screen, do not recurse into it.
    if (wi->state & STATE_OFFSCREEN)
        return NO_RESULT_YET;
    if (wi->state & STATE_SENSITIVE)
        *result= wi;
    GList *l=wi->children;
    while (l) {
        struct widget *child=l->data;
        if (gui_internal_keynav_find_prev(child, current_highlight, result) == RESULT_FOUND) {
            return RESULT_FOUND;
        }
        l=g_list_next(l);
    }
    // If no sensitive widget is found before "current_highlight", return the last sensitive widget when
    // recursion terminates.
    return NO_RESULT_YET;
}

static void gui_internal_keynav_find_closest(struct widget *wi, struct point *p, int dx, int dy, int *distance,
        struct widget **result) {
    GList *l=wi->children;
    // Skip hidden elements
    if (wi->p.x==0 && wi->p.y==0 && wi->w==0 && wi->h==0)
        return;
    if ((wi->state & STATE_SENSITIVE) ) {
        int dist1,dist2;
        struct point wp;
        gui_internal_keynav_point(wi, -dx, -dy, &wp);
        if (dx) {
            dist1=(wp.x-p->x)*dx;
            dist2=wp.y-p->y;
        } else if (dy) {
            dist1=(wp.y-p->y)*dy;
            dist2=wp.x-p->x;
        } else {
            dist2=wp.x-p->x;
            dist1=wp.y-p->y;
            if (dist1 < 0)
                dist1=-dist1;
        }
        dbg(lvl_debug,"checking %d,%d %d %d against %d,%d-%d,%d result %d,%d", p->x, p->y, dx, dy, wi->p.x, wi->p.y,
            wi->p.x+wi->w, wi->p.y+wi->h, dist1, dist2);
        if (dist1 >= 0) {
            if (dist2 < 0)
                dist1-=dist2;
            else
                dist1+=dist2;
            if (dist1 < *distance) {
                *result=wi;
                *distance=dist1;
            }
        }
    }
    while (l) {
        struct widget *child=l->data;
        gui_internal_keynav_find_closest(child, p, dx, dy, distance, result);
        l=g_list_next(l);
    }
}

/**
 * @brief Move keyboard focus to the next widget.
 *
 * Move keyboard focus to the appropriate next widget, depending on the direction of focus
 * movement.
 *
 * @param this GUI context
 * @param this dx horizontal movement (-1=left, +1=right), unless rotary==1
 * @param this dy vertical movement (+1=up, -1=down)
 * @param rotary (0/1) input from rotary encoder - dx indicates forwards/backwards movement
 *        through all widgets
 */
static void gui_internal_keynav_highlight_next(struct gui_priv *this, int dx, int dy, int rotary) {
    struct widget *result,*menu=g_list_last(this->root.children)->data;
    struct widget *current_highlight = NULL;
    struct point p;
    int distance;
    if (this->highlighted && this->highlighted_menu == menu) {
        gui_internal_keynav_point(this->highlighted, dx, dy, &p);
        current_highlight = this->highlighted;
    } else {
        p.x=0;
        p.y=0;
        distance=INT_MAX;
        result=NULL;
        gui_internal_keynav_find_closest(menu, &p, 0, 0, &distance, &result);
        if (result) {
            gui_internal_keynav_point(result, dx, dy, &p);
            dbg(lvl_debug,"result origin=%p p=%d,%d", result, p.x, p.y);
            current_highlight = result;
        }
    }
    result=NULL;
    distance=INT_MAX;
    if (rotary && dx > 0)
        gui_internal_keynav_find_next(menu, current_highlight, &result);
    else if (rotary && dx < 0)
        gui_internal_keynav_find_prev(menu, current_highlight, &result);
    else
        gui_internal_keynav_find_closest(menu, &p, dx, dy, &distance, &result);
    dbg(lvl_debug,"result=%p", result);
    if (! result) {
        if (dx < 0) {
            p.x=this->root.w;
            if (rotary) p.y = this->root.h;
        }
        if (dx > 0) {
            p.x=0;
            if (rotary) p.y = 0;
        }
        if (dy < 0)
            p.y=this->root.h;
        if (dy > 0)
            p.y=0;
        result=NULL;
        distance=INT_MAX;
        gui_internal_keynav_find_closest(menu, &p, dx, dy, &distance, &result);
        dbg(lvl_debug,"wraparound result=%p", result);
    }
    gui_internal_highlight_do(this, result);
    if (result)
        gui_internal_say(this, result, 1);
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gui_internal_keypress(void *data, char *key) {
    struct gui_priv *this=data;
    int w,h;
    struct point p;
    if (!this->root.children) {
        transform_get_size(navit_get_trans(this->nav), &w, &h);
        switch (*key) {
        case NAVIT_KEY_UP:
            p.x=w/2;
            p.y=0;
            navit_set_center_screen(this->nav, &p, 1);
            break;
        case NAVIT_KEY_DOWN:
            p.x=w/2;
            p.y=h;
            navit_set_center_screen(this->nav, &p, 1);
            break;
        case NAVIT_KEY_LEFT:
            p.x=0;
            p.y=h/2;
            navit_set_center_screen(this->nav, &p, 1);
            break;
        case NAVIT_KEY_RIGHT:
            p.x=w;
            p.y=h/2;
            navit_set_center_screen(this->nav, &p, 1);
            break;
        case NAVIT_KEY_ZOOM_IN:
            navit_zoom_in(this->nav, 2, NULL);
            break;
        case NAVIT_KEY_ZOOM_OUT:
            navit_zoom_out(this->nav, 2, NULL);
            break;
        case NAVIT_KEY_RETURN:
        case NAVIT_KEY_MENU:
            gui_internal_set_click_coord(this, NULL);
            gui_internal_cmd_menu(this, 0, NULL);
            break;
        }
        return;
    }
    graphics_draw_mode(this->gra, draw_mode_begin);
    switch (*key) {
    case NAVIT_KEY_PAGE_DOWN:
        gui_internal_keynav_highlight_next(this,1,0,1);
        break;
    case NAVIT_KEY_PAGE_UP:
        gui_internal_keynav_highlight_next(this,-1,0,1);
        break;
    case NAVIT_KEY_LEFT:
        gui_internal_keynav_highlight_next(this,-1,0,0);
        break;
    case NAVIT_KEY_RIGHT:
        gui_internal_keynav_highlight_next(this,1,0,0);
        break;
    case NAVIT_KEY_UP:
        gui_internal_keynav_highlight_next(this,0,-1,0);
        break;
    case NAVIT_KEY_DOWN:
        gui_internal_keynav_highlight_next(this,0,1,0);
        break;
    case NAVIT_KEY_BACK:
        if (g_list_length(this->root.children) > 1)
            gui_internal_back(this, NULL, NULL);
        else
            gui_internal_prune_menu(this, NULL);
        break;
    case NAVIT_KEY_RETURN:
        if (this->highlighted && this->highlighted_menu == g_list_last(this->root.children)->data)
            gui_internal_call_highlighted(this);
        else
            gui_internal_keypress_do(this, key);
        break;
    default:
        gui_internal_keypress_do(this, key);
    }
    if (!event_main_loop_has_quit()) {
        graphics_draw_mode(this->gra, draw_mode_end);
        gui_internal_check_exit(this);
    }
}


//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static int gui_internal_set_graphics(struct gui_priv *this, struct graphics *gra) {
    struct window *win;
    struct transformation *trans=navit_get_trans(this->nav);

    win=graphics_get_data(gra, "window");
    if (! win) {
        dbg(lvl_error, "failed to obtain window from graphics plugin, cannot set graphics");
        return 1;
    }
    navit_ignore_graphics_events(this->nav, 1);
    this->gra=gra;
    this->win=win;
    navit_ignore_graphics_events(this->nav, 1);
    transform_get_size(trans, &this->root.w, &this->root.h);
    this->resize_cb=callback_new_attr_1(callback_cast(gui_internal_resize), attr_resize, this);
    graphics_add_callback(gra, this->resize_cb);
    this->button_cb=callback_new_attr_1(callback_cast(gui_internal_button), attr_button, this);
    graphics_add_callback(gra, this->button_cb);
    this->motion_cb=callback_new_attr_1(callback_cast(gui_internal_motion), attr_motion, this);
    graphics_add_callback(gra, this->motion_cb);
    this->keypress_cb=callback_new_attr_1(callback_cast(gui_internal_keypress), attr_keypress, this);
    graphics_add_callback(gra, this->keypress_cb);
    this->window_closed_cb=callback_new_attr_1(callback_cast(gui_internal_window_closed), attr_window_closed, this);
    graphics_add_callback(gra, this->window_closed_cb);

    // set fullscreen if needed
    if (this->fullscreen)
        this->win->fullscreen(this->win, this->fullscreen != 0);
    /* Was resize callback already issued? */
    if (navit_get_ready(this->nav) & 2)
        gui_internal_setup(this);
    return 0;
}

static void gui_internal_disable_suspend(struct gui_priv *this) {
    if (this->win->disable_suspend)
        this->win->disable_suspend(this->win);
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct gui_methods gui_internal_methods = {
    NULL,
    NULL,
    gui_internal_set_graphics,
    NULL,
    NULL,
    NULL,
    gui_internal_disable_suspend,
    gui_internal_get_attr,
    gui_internal_add_attr,
    gui_internal_set_attr,
};


static void gui_internal_add_callback(struct gui_priv *priv, struct callback *cb) {
    callback_list_add(priv->cbl, cb);
}

static void gui_internal_remove_callback(struct gui_priv *priv, struct callback *cb) {
    callback_list_remove(priv->cbl, cb);
}


static struct gui_internal_methods gui_internal_methods_ext = {
    gui_internal_add_callback,
    gui_internal_remove_callback,
    gui_internal_menu_render,
    image_new_xs,
    image_new_l,
};


static enum flags gui_internal_get_flags(struct widget *widget) {
    return widget->flags;
}

static void gui_internal_set_flags(struct widget *widget, enum flags flags) {
    widget->flags=flags;
}

static int gui_internal_get_state(struct widget *widget) {
    return widget->state;
}

static void gui_internal_set_state(struct widget *widget, int state) {
    widget->state=state;
}

static void gui_internal_set_func(struct widget *widget, void (*func)(struct gui_priv *priv, struct widget *widget,
                                  void *data)) {
    widget->func=func;
}

static void gui_internal_set_data(struct widget *widget, void *data) {
    widget->data=data;
}

static void gui_internal_set_default_background(struct gui_priv *this, struct widget *widget) {
    widget->background=this->background;
}

static struct gui_internal_widget_methods gui_internal_widget_methods = {
    gui_internal_widget_append,
    gui_internal_button_new,
    gui_internal_button_new_with_callback,
    gui_internal_box_new,
    gui_internal_label_new,
    gui_internal_image_new,
    gui_internal_keyboard,
    gui_internal_menu,
    gui_internal_get_flags,
    gui_internal_set_flags,
    gui_internal_get_state,
    gui_internal_set_state,
    gui_internal_set_func,
    gui_internal_set_data,
    gui_internal_set_default_background,
};

/**
 * @brief finds the intersection point of 2 lines
 *
 * @param coord a1, a2, b1, b2 : coords of the start and
 * end of the first and the second line
 * @param coord res, will become the coords of the intersection if found
 * @return : TRUE if intersection found, otherwise FALSE
 */
int line_intersection(struct coord* a1, struct coord *a2, struct coord * b1, struct coord *b2, struct coord *res) {
    int n, a, b;
    int adx=a2->x-a1->x;
    int ady=a2->y-a1->y;
    int bdx=b2->x-b1->x;
    int bdy=b2->y-b1->y;
    n = bdy * adx - bdx * ady;
    a = bdx * (a1->y - b1->y) - bdy * (a1->x - b1->x);
    b = adx * (a1->y - b1->y) - ady * (a1->x - b1->x);
    if (n < 0) {
        n = -n;
        a = -a;
        b = -b;
    }
    if (a < 0 || b < 0)
        return FALSE;
    if (a > n || b > n)
        return FALSE;
    if (n == 0) {
        dbg(lvl_info,"a=%d b=%d n=%d", a, b, n);
        dbg(lvl_info,"a1=0x%x,0x%x ad %d,%d", a1->x, a1->y, adx, ady);
        dbg(lvl_info,"b1=0x%x,0x%x bd %d,%d", b1->x, b1->y, bdx, bdy);
        dbg(lvl_info,"No intersection found, lines assumed parallel ?");
        return FALSE;
    }
    res->x = a1->x + a * adx / n;
    res->y = a1->y + a * ady / n;
    return TRUE;
}

struct heightline *
item_get_heightline(struct item *item) {
    struct heightline *ret=NULL;
    struct street_data *sd;
    struct attr attr;
    int i,height;

    if (item_attr_get(item, attr_label, &attr)) {
        height=atoi(attr.u.str);
        sd=street_get_data(item);
        if (sd && sd->count > 1) {
            ret=g_malloc(sizeof(struct heightline)+sd->count*sizeof(struct coord));
            ret->bbox.lu=sd->c[0];
            ret->bbox.rl=sd->c[0];
            ret->count=sd->count;
            ret->height=height;
            for (i = 0 ; i < sd->count ; i++) {
                ret->c[i]=sd->c[i];
                coord_rect_extend(&ret->bbox, sd->c+i);
            }
        }
        street_data_free(sd);
    }
    return ret;
}

/**
 * @brief Called when the route is updated.
 */
void gui_internal_route_update(struct gui_priv * this, struct navit * navit, struct vehicle *v) {

    if(this->route_data.route_showing) {
        gui_internal_populate_route_table(this,navit);
        graphics_draw_mode(this->gra, draw_mode_begin);
        gui_internal_menu_render(this);
        graphics_draw_mode(this->gra, draw_mode_end);
    }


}


/**
 * @brief Called when the route screen is closed (deallocated).
 *
 * The main purpose of this function is to remove the widgets from
 * references route_data because those widgets are about to be freed.
 */
void gui_internal_route_screen_free(struct gui_priv * this_,struct widget * w) {
    if(this_) {
        this_->route_data.route_showing=0;
        this_->route_data.route_table=NULL;
        g_free(w);
    }

}

/**
 * @brief Populates the route table with route information
 *
 * @param this The gui context
 * @param navit The navit object
 */
void gui_internal_populate_route_table(struct gui_priv * this, struct navit * navit) {
    struct map * map=NULL;
    struct map_rect * mr=NULL;
    struct navigation * nav = NULL;
    struct item * item =NULL;
    struct attr attr,route;
    struct widget * label = NULL;
    struct widget * row = NULL;
    struct coord c;
    nav = navit_get_navigation(navit);
    if(!nav) {
        return;
    }
    map = navigation_get_map(nav);
    if(map)
        mr = map_rect_new(map,NULL);
    if(mr) {
        GList *toprow;
        struct item topitem= {0};
        toprow=gui_internal_widget_table_top_row(this, this->route_data.route_table);
        if(toprow && toprow->data)
            topitem=((struct widget*)toprow->data)->item;
        gui_internal_widget_table_clear(this,this->route_data.route_table);
        if (navit_get_attr(navit, attr_route, &route, NULL)) {
            struct attr destination_length, destination_time;
            char *length=NULL,*time=NULL,*length_time;
            if (route_get_attr(route.u.route, attr_destination_length, &destination_length, NULL))
                length=attr_to_text_ext(&destination_length, NULL, attr_format_with_units, attr_format_default, NULL);
            if (route_get_attr(route.u.route, attr_destination_time, &destination_time, NULL))
                time=attr_to_text_ext(&destination_time, NULL, attr_format_with_units, attr_format_default, NULL);
            row = gui_internal_widget_table_row_new(this,
                                                    gravity_left
                                                    | flags_fill
                                                    | orientation_horizontal);
            gui_internal_widget_append(this->route_data.route_table,row);
            length_time=g_strdup_printf("%s %s",length,time);
            label = gui_internal_label_new(this,length_time);
            g_free(length_time);
            g_free(length);
            g_free(time);
            gui_internal_widget_append(row,label);
        }
        while((item = map_rect_get_item(mr))) {
            if(item_attr_get(item,attr_navigation_long,&attr)) {
                row = gui_internal_widget_table_row_new(this,
                                                        gravity_left
                                                        | flags_fill
                                                        | orientation_horizontal);
                gui_internal_widget_append(this->route_data.route_table,row);

                label = gui_internal_label_new(this,map_convert_string_tmp(item->map,attr.u.str));
                gui_internal_widget_append(row,label);

                label->item=*item;
                row->item=*item;
                item_coord_get(item, &c, 1);
                label->c.x=c.x;
                label->c.y=c.y;
                label->c.pro=map_projection(map);
                label->func=gui_internal_cmd_position;
                label->state|=STATE_SENSITIVE;
                label->data=(void*)2;
                if(toprow && item->id_hi==topitem.id_hi && item->id_lo==topitem.id_lo && item->map==topitem.map)
                    gui_internal_widget_table_set_top_row(this, this->route_data.route_table, row);
            }

        }
        map_rect_destroy(mr);
    }
}

/*
 *  Command interface wrapper for commands which can be used both from gui html and to enter internal gui (for example, from osd or dbus).
 *  Set first command argument to integer 1, if this command was called by mouse click from oustside of gui (default). Set it to 0
 *  if command is called by some other means (dbus signal, for example). If first argument is non integer, it's passed on
 *  to actual handler.
 *
 */


//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct gui_priv * gui_internal_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs,
        struct gui *gui) {
    struct color color_white= {0xffff,0xffff,0xffff,0xffff};
    struct color color_black= {0x0,0x0,0x0,0xffff};
    struct color back2_color= {0x4141,0x4141,0x4141,0xffff};

    struct gui_priv *this;
    struct attr *attr;
    *meth=gui_internal_methods;
    this=g_new0(struct gui_priv, 1);
    this->nav=nav;

    this->self.type=attr_gui;
    this->self.u.gui=gui;

    if ((attr=attr_search(attrs, NULL, attr_menu_on_map_click)))
        this->menu_on_map_click=attr->u.num;
    else
        this->menu_on_map_click=1;

    if ((attr=attr_search(attrs, NULL, attr_on_map_click)))
        this->on_map_click=g_strdup(attr->u.str);

    if ((attr=attr_search(attrs, NULL, attr_signal_on_map_click)))
        this->signal_on_map_click=attr->u.num;
    gui_internal_command_init(this, attrs);

    if( (attr=attr_search(attrs,NULL,attr_font_size))) {
        this->config.font_size=attr->u.num;
    } else {
        this->config.font_size=-1;
    }
    if( (attr=attr_search(attrs,NULL,attr_icon_xs))) {
        this->config.icon_xs=attr->u.num;
    } else {
        this->config.icon_xs=-1;
    }
    if( (attr=attr_search(attrs,NULL,attr_icon_l))) {
        this->config.icon_l=attr->u.num;
    } else {
        this->config.icon_l=-1;
    }
    if( (attr=attr_search(attrs,NULL,attr_icon_s))) {
        this->config.icon_s=attr->u.num;
    } else {
        this->config.icon_s=-1;
    }
    if( (attr=attr_search(attrs,NULL,attr_spacing))) {
        this->config.spacing=attr->u.num;
    } else {
        this->config.spacing=-1;
    }
    if( (attr=attr_search(attrs,NULL,attr_gui_speech))) {
        this->speech=attr->u.num;
    }
    if( (attr=attr_search(attrs,NULL,attr_keyboard)))
        this->keyboard=attr->u.num;
    else
        this->keyboard=1;

    if( (attr=attr_search(attrs,NULL,attr_fullscreen)))
        this->fullscreen=attr->u.num;

    if( (attr=attr_search(attrs,NULL,attr_flags)))
        this->flags=attr->u.num;
    if( (attr=attr_search(attrs,NULL,attr_background_color)))
        this->background_color=*attr->u.color;
    else
        this->background_color=color_black;
    if( (attr=attr_search(attrs,NULL,attr_background_color2)))
        this->background2_color=*attr->u.color;
    else
        this->background2_color=back2_color;
    if( (attr=attr_search(attrs,NULL,attr_text_color)))
        this->text_foreground_color=*attr->u.color;
    else
        this->text_foreground_color=color_white;
    if( (attr=attr_search(attrs,NULL,attr_text_background)))
        this->text_background_color=*attr->u.color;
    else
        this->text_background_color=color_black;
    if( (attr=attr_search(attrs,NULL,attr_columns)))
        this->cols=attr->u.num;
    if( (attr=attr_search(attrs,NULL,attr_osd_configuration)))
        this->osd_configuration=*attr;

    if( (attr=attr_search(attrs,NULL,attr_pitch)))
        this->pitch=attr->u.num;
    else
        this->pitch=20;
    if( (attr=attr_search(attrs,NULL,attr_flags_town)))
        this->flags_town=attr->u.num;
    else
        this->flags_town=-1;
    if( (attr=attr_search(attrs,NULL,attr_flags_street)))
        this->flags_street=attr->u.num;
    else
        this->flags_street=-1;
    if( (attr=attr_search(attrs,NULL,attr_flags_house_number)))
        this->flags_house_number=attr->u.num;
    else
        this->flags_house_number=-1;
    if( (attr=attr_search(attrs,NULL,attr_radius)))
        this->radius=attr->u.num;
    else
        this->radius=10;
    if( (attr=attr_search(attrs,NULL,attr_font)))
        this->font_name=g_strdup(attr->u.str);

    if((attr=attr_search(attrs, NULL, attr_hide_impossible_next_keys)))
        this->hide_keys = attr->u.num;
    else
        this->hide_keys = 0;

    this->data.priv=this;
    this->data.gui=&gui_internal_methods_ext;
    this->data.widget=&gui_internal_widget_methods;
    this->cbl=callback_list_new();

    return this;
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void plugin_init(void) {
    plugin_register_category_gui("internal", gui_internal_new);
}
