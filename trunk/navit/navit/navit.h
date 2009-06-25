/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef NAVIT_NAVIT_H
#define NAVIT_NAVIT_H

#ifdef __cplusplus
extern "C" {
#endif
extern struct gui *main_loop_gui;
/* prototypes */
enum attr_type;
struct attr;
struct attr_iter;
struct callback;
struct displaylist;
struct graphics;
struct gui;
struct mapset;
struct message;
struct navigation;
struct navit;
struct pcoord;
struct point;
struct route;
struct tracking;
struct transformation;
struct vehicleprofile;
void navit_add_mapset(struct navit *this_, struct mapset *ms);
struct mapset *navit_get_mapset(struct navit *this_);
struct tracking *navit_get_tracking(struct navit *this_);
void navit_draw(struct navit *this_);
void navit_draw_displaylist(struct navit *this_);
void navit_handle_resize(struct navit *this_, int w, int h);
int navit_get_width(struct navit *this_);
int navit_get_height(struct navit *this_);
int navit_ignore_button(struct navit *this_);
void navit_ignore_graphics_events(struct navit *this_, int ignore);
int navit_handle_button(struct navit *this_, int pressed, int button, struct point *p, struct callback *popup_callback);
void navit_handle_motion(struct navit *this_, struct point *p);
void navit_zoom_in(struct navit *this_, int factor, struct point *p);
void navit_zoom_out(struct navit *this_, int factor, struct point *p);
struct navit *navit_new(struct attr *parent, struct attr **attrs);
void navit_add_message(struct navit *this_, char *message);
struct message *navit_get_messages(struct navit *this_);
struct graphics *navit_get_graphics(struct navit *this_);
struct vehicleprofile *navit_get_vehicleprofile(struct navit *this_);
void navit_set_destination(struct navit *this_, struct pcoord *c, const char *description, int async);
int navit_check_route(struct navit *this_);
void navit_add_bookmark(struct navit *this_, struct pcoord *c, const char *description);
void navit_textfile_debug_log(struct navit *this_, const char *fmt, ...);
int navit_speech_estimate(struct navit *this_, char *str);
void navit_say(struct navit *this_, char *text);
void navit_announcer_toggle(struct navit *this_);
void navit_speak(struct navit *this_);
void navit_window_roadbook_destroy(struct navit *this_);
void navit_window_roadbook_new(struct navit *this_);
void navit_init(struct navit *this_);
void navit_zoom_to_route(struct navit *this_, int orientation);
void navit_set_center(struct navit *this_, struct pcoord *center);
void navit_set_center_screen(struct navit *this_, struct point *p);
int navit_set_attr(struct navit *this_, struct attr *attr);
int navit_get_attr(struct navit *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int navit_add_attr(struct navit *this_, struct attr *attr);
int navit_remove_attr(struct navit *this_, struct attr *attr);
struct attr_iter *navit_attr_iter_new(void);
void navit_attr_iter_destroy(struct attr_iter *iter);
void navit_add_callback(struct navit *this_, struct callback *cb);
void navit_remove_callback(struct navit *this_, struct callback *cb);
void navit_set_position(struct navit *this_, struct pcoord *c);
struct gui *navit_get_gui(struct navit *this_);
struct transformation *navit_get_trans(struct navit *this_);
struct route *navit_get_route(struct navit *this_);
struct navigation *navit_get_navigation(struct navit *this_);
struct displaylist *navit_get_displaylist(struct navit *this_);
int navit_block(struct navit *this_, int block);
void navit_destroy(struct navit *this_);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif

