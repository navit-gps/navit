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
// defined in glib.h.
#ifndef __G_LIST_H__
struct _GList;
typedef struct _GList GList;
#endif

/* prototypes */
enum attr_type;
struct attr;
struct attr_iter;
struct callback;
struct coord_rect;
struct displaylist;
struct graphics;
struct gui;
struct layout;
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
struct command_table;
struct item;
void navit_add_mapset(struct navit *this_, struct mapset *ms);
struct mapset *navit_get_mapset(struct navit *this_);
struct map *navit_get_search_results_map(struct navit *this_);
int navit_populate_search_results_map(struct navit *navit, GList *search_results, struct coord_rect *r);
struct tracking *navit_get_tracking(struct navit *this_);
char *navit_get_user_data_directory(int create);
void navit_draw_async(struct navit *this_, int async);
void navit_draw(struct navit *this_);
int navit_get_ready(struct navit *this_);
void navit_draw_displaylist(struct navit *this_);
void navit_handle_resize(struct navit *this_, int w, int h);
int navit_get_width(struct navit *this_);
int navit_get_height(struct navit *this_);
int navit_ignore_button(struct navit *this_);
void navit_ignore_graphics_events(struct navit *this_, int ignore);
void navit_set_timeout(struct navit *this_);
int navit_handle_button(struct navit *this_, int pressed, int button, struct point *p, struct callback *popup_callback);
void navit_handle_motion(struct navit *this_, struct point *p);
void navit_zoom_in(struct navit *this_, int factor, struct point *p);
void navit_zoom_out(struct navit *this_, int factor, struct point *p);
void navit_zoom_in_cursor(struct navit *this_, int factor);
void navit_zoom_out_cursor(struct navit *this_, int factor);
struct navit *navit_new(struct attr *parent, struct attr **attrs);
void navit_add_message(struct navit *this_, const char *message);
struct message *navit_get_messages(struct navit *this_);
struct graphics *navit_get_graphics(struct navit *this_);
struct vehicleprofile *navit_get_vehicleprofile(struct navit *this_);
GList *navit_get_vehicleprofiles(struct navit *this_);
void navit_set_destination(struct navit *this_, struct pcoord *c, const char *description, int async);
void navit_set_destinations(struct navit *this_, struct pcoord *c, int count, const char *description, int async);
void navit_add_destination_description(struct navit *this_, struct pcoord *c, const char *description);
int navit_get_destinations(struct navit *this_, struct pcoord *pc, int count);
int navit_get_destination_count(struct navit *this_);
char* navit_get_destination_description(struct navit *this_, int n);
void navit_remove_nth_waypoint(struct navit *this_, int n);
void navit_remove_waypoint(struct navit *this_);
char* navit_get_coord_description(struct navit *this_, struct pcoord *c);
int navit_check_route(struct navit *this_);
struct map* read_former_destinations_from_file(void);
void navit_textfile_debug_log(struct navit *this_, const char *fmt, ...);
void navit_textfile_debug_log_at(struct navit *this_, struct pcoord *pc, const char *fmt, ...);
int navit_speech_estimate(struct navit *this_, char *str);
void navit_say(struct navit *this_, const char *text);
void navit_speak(struct navit *this_);
void navit_window_roadbook_destroy(struct navit *this_);
void navit_window_roadbook_new(struct navit *this_);
int navit_init(struct navit *this_);
void navit_zoom_to_rect(struct navit *this_, struct coord_rect *r);
void navit_zoom_to_route(struct navit *this_, int orientation);
void navit_set_center(struct navit *this_, struct pcoord *center, int set_timeout);
void navit_set_center_cursor(struct navit *this_, int autozoom, int keep_orientation);
void navit_set_center_screen(struct navit *this_, struct point *p, int set_timeout);
int navit_set_attr(struct navit *this_, struct attr *attr);
int navit_get_attr(struct navit *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
struct layout *navit_get_layout_by_name(struct navit *this_, const char *layout_name);
void navit_update_current_layout(struct navit *this_, struct layout *layout);
int navit_add_attr(struct navit *this_, struct attr *attr);
int navit_remove_attr(struct navit *this_, struct attr *attr);
struct attr_iter *navit_attr_iter_new(void * unused);
void navit_attr_iter_destroy(struct attr_iter *iter);
void navit_add_callback(struct navit *this_, struct callback *cb);
void navit_remove_callback(struct navit *this_, struct callback *cb);
void navit_set_position(struct navit *this_, struct pcoord *c);
struct gui *navit_get_gui(struct navit *this_);
struct transformation *navit_get_trans(struct navit *this_);
struct route *navit_get_route(struct navit *this_);
struct navigation *navit_get_navigation(struct navit *this_);
struct displaylist *navit_get_displaylist(struct navit *this_);
void navit_layout_switch(struct navit *n);
int navit_set_vehicle_by_name(struct navit *n, const char *name);
int navit_set_vehicleprofile_name(struct navit *this_, char *name);
int navit_set_layout_by_name(struct navit *n, const char *name);
void navit_disable_suspend(void);
int navit_block(struct navit *this_, int block);
int navit_get_blocked(struct navit *this_);
void navit_destroy(struct navit *this_);
void navit_command_add_table(struct navit*this_, struct command_table *commands, int count);
struct navit * navit_ref(struct navit *this_);
void navit_unref(struct navit *this_);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif

