#include <glib.h>
#include <stdlib.h>
#include "color.h"
#include "coord.h"
#include "point.h"
#include "callback.h"
#include "graphics.h"
#include "debug.h"
#include "navit.h"
#include "navit_nls.h"
#include "bookmarks.h"
#include "gui_internal.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_menu.h"
#include "gui_internal_keyboard.h"
#include "gui_internal_vehicle_dimension.h"

static void gui_internal_cmd_change_vehicle_dimensions_weight_do(struct gui_priv *this, struct widget *widget) {
    GList *l;
    struct attr attr;
    struct vehicleprofile *active_profile;
    dbg(lvl_debug,"text='%s'", widget->text);
    if (widget->text && strlen(widget->text)) {
        active_profile = navit_get_vehicleprofile(this->nav);
        attr.type = attr_vehicle_weight;
        attr.u.num=atoi(widget->text);
        vehicleprofile_set_attr(active_profile, &attr);
        vehicleprofile_store_dimensions(active_profile);
    }
    g_free(widget->text);
    widget->text=NULL;
    l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
    gui_internal_prune_menu(this, l->data);
}

static void gui_internal_cmd_change_vehicle_dimensions_weight_clicked(struct gui_priv *this, struct widget *widget, void *data) {
    gui_internal_cmd_change_vehicle_dimensions_weight_do(this, widget->data);
}

void gui_internal_cmd_change_vehicle_dimensions_weight(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w,*wb,*wk,*wl,*we,*wnext;
    char *name=data;
    wb=gui_internal_menu(this,_("Change Weight"));
    w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
    gui_internal_widget_append(w, we);
    gui_internal_widget_append(we, wk=gui_internal_label_new(this, name));
    wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
    wk->background=this->background;
    wk->flags |= flags_expand|flags_fill;
    wk->func = gui_internal_call_linked_on_finish;
    wk->c=wm->c;
    gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
    wnext->state |= STATE_SENSITIVE;
    wnext->func = gui_internal_cmd_change_vehicle_dimensions_weight_clicked;
    wnext->data=wk;
    wk->data=wnext;
    wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, wl);
    if (this->keyboard)
        gui_internal_widget_append(w, gui_internal_keyboard(this,
                    VKBD_NUMERIC | gui_internal_keyboard_init_mode(getenv("LANG"))));
    else
        gui_internal_keyboard_show_native(this, w, VKBD_NUMERIC | gui_internal_keyboard_init_mode(getenv("LANG")),
                                          getenv("LANG"));
    gui_internal_menu_render(this);
}

static void gui_internal_cmd_change_vehicle_dimensions_axle_weight_do(struct gui_priv *this, struct widget *widget) {
    GList *l;
    struct attr attr;
    struct vehicleprofile *active_profile;
    dbg(lvl_debug,"text='%s'", widget->text);
    if (widget->text && strlen(widget->text)) {
        active_profile = navit_get_vehicleprofile(this->nav);
        attr.type = attr_vehicle_axle_weight;
        attr.u.num=atoi(widget->text);
        vehicleprofile_set_attr(active_profile, &attr);
        vehicleprofile_store_dimensions(active_profile);
    }
    g_free(widget->text);
    widget->text=NULL;
    l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
    gui_internal_prune_menu(this, l->data);
}

static void gui_internal_cmd_change_vehicle_dimensions_axle_weight_clicked(struct gui_priv *this, struct widget *widget, void *data) {
    gui_internal_cmd_change_vehicle_dimensions_axle_weight_do(this, widget->data);
}

void gui_internal_cmd_change_vehicle_dimensions_axle_weight(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w,*wb,*wk,*wl,*we,*wnext;
    char *name=data;
    wb=gui_internal_menu(this,_("Change Axle Weight"));
    w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
    gui_internal_widget_append(w, we);
    gui_internal_widget_append(we, wk=gui_internal_label_new(this, name));
    wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
    wk->background=this->background;
    wk->flags |= flags_expand|flags_fill;
    wk->func = gui_internal_call_linked_on_finish;
    wk->c=wm->c;
    gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
    wnext->state |= STATE_SENSITIVE;
    wnext->func = gui_internal_cmd_change_vehicle_dimensions_axle_weight_clicked;
    wnext->data=wk;
    wk->data=wnext;
    wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, wl);
    if (this->keyboard)
        gui_internal_widget_append(w, gui_internal_keyboard(this,
                    VKBD_NUMERIC | gui_internal_keyboard_init_mode(getenv("LANG"))));
    else
        gui_internal_keyboard_show_native(this, w, VKBD_NUMERIC | gui_internal_keyboard_init_mode(getenv("LANG")),
                                          getenv("LANG"));
    gui_internal_menu_render(this);
}

static void gui_internal_cmd_change_vehicle_dimensions_length_do(struct gui_priv *this, struct widget *widget) {
    GList *l;
    struct attr attr;
    struct vehicleprofile *active_profile;
    dbg(lvl_debug,"text='%s'", widget->text);
    if (widget->text && strlen(widget->text)) {
        active_profile = navit_get_vehicleprofile(this->nav);
        attr.type = attr_vehicle_length;
        attr.u.num=atoi(widget->text);
        vehicleprofile_set_attr(active_profile, &attr);
        vehicleprofile_store_dimensions(active_profile);
    }
    g_free(widget->text);
    widget->text=NULL;
    l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
    gui_internal_prune_menu(this, l->data);
}

static void gui_internal_cmd_change_vehicle_dimensions_length_clicked(struct gui_priv *this, struct widget *widget, void *data) {
    gui_internal_cmd_change_vehicle_dimensions_length_do(this, widget->data);
}

void gui_internal_cmd_change_vehicle_dimensions_length(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w,*wb,*wk,*wl,*we,*wnext;
    char *name=data;
    wb=gui_internal_menu(this,_("Change Length"));
    w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
    gui_internal_widget_append(w, we);
    gui_internal_widget_append(we, wk=gui_internal_label_new(this, name));
    wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
    wk->background=this->background;
    wk->flags |= flags_expand|flags_fill;
    wk->func = gui_internal_call_linked_on_finish;
    wk->c=wm->c;
    gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
    wnext->state |= STATE_SENSITIVE;
    wnext->func = gui_internal_cmd_change_vehicle_dimensions_length_clicked;
    wnext->data=wk;
    wk->data=wnext;
    wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, wl);
    if (this->keyboard)
        gui_internal_widget_append(w, gui_internal_keyboard(this,
                    VKBD_NUMERIC | gui_internal_keyboard_init_mode(getenv("LANG"))));
    else
        gui_internal_keyboard_show_native(this, w, VKBD_NUMERIC | gui_internal_keyboard_init_mode(getenv("LANG")),
                                          getenv("LANG"));
    gui_internal_menu_render(this);
}

static void gui_internal_cmd_change_vehicle_dimensions_width_do(struct gui_priv *this, struct widget *widget) {
    GList *l;
    struct attr attr;
    struct vehicleprofile *active_profile;
    dbg(lvl_debug,"text='%s'", widget->text);
    if (widget->text && strlen(widget->text)) {
        active_profile = navit_get_vehicleprofile(this->nav);
        attr.type = attr_vehicle_width;
        attr.u.num=atoi(widget->text);
        vehicleprofile_set_attr(active_profile, &attr);
        vehicleprofile_store_dimensions(active_profile);
    }
    g_free(widget->text);
    widget->text=NULL;
    l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
    gui_internal_prune_menu(this, l->data);
}

static void gui_internal_cmd_change_vehicle_dimensions_width_clicked(struct gui_priv *this, struct widget *widget, void *data) {
    gui_internal_cmd_change_vehicle_dimensions_width_do(this, widget->data);
}

void gui_internal_cmd_change_vehicle_dimensions_width(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w,*wb,*wk,*wl,*we,*wnext;
    char *name=data;
    wb=gui_internal_menu(this,_("Change Width"));
    w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
    gui_internal_widget_append(w, we);
    gui_internal_widget_append(we, wk=gui_internal_label_new(this, name));
    wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
    wk->background=this->background;
    wk->flags |= flags_expand|flags_fill;
    wk->func = gui_internal_call_linked_on_finish;
    wk->c=wm->c;
    gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
    wnext->state |= STATE_SENSITIVE;
    wnext->func = gui_internal_cmd_change_vehicle_dimensions_width_clicked;
    wnext->data=wk;
    wk->data=wnext;
    wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, wl);
    if (this->keyboard)
        gui_internal_widget_append(w, gui_internal_keyboard(this,
                    VKBD_NUMERIC | gui_internal_keyboard_init_mode(getenv("LANG"))));
    else
        gui_internal_keyboard_show_native(this, w, VKBD_NUMERIC | gui_internal_keyboard_init_mode(getenv("LANG")),
                                          getenv("LANG"));
    gui_internal_menu_render(this);
}

static void gui_internal_cmd_change_vehicle_dimensions_height_do(struct gui_priv *this, struct widget *widget) {
    GList *l;
    struct attr attr;
    struct vehicleprofile *active_profile;
    dbg(lvl_debug,"text='%s'", widget->text);
    if (widget->text && strlen(widget->text)) {
        active_profile = navit_get_vehicleprofile(this->nav);
        attr.type = attr_vehicle_height;
        attr.u.num=atoi(widget->text);
        vehicleprofile_set_attr(active_profile, &attr);
        vehicleprofile_store_dimensions(active_profile);
    }
    g_free(widget->text);
    widget->text=NULL;
    l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
    gui_internal_prune_menu(this, l->data);
}

static void gui_internal_cmd_change_vehicle_dimensions_height_clicked(struct gui_priv *this, struct widget *widget, void *data) {
    gui_internal_cmd_change_vehicle_dimensions_height_do(this, widget->data);
}

void gui_internal_cmd_change_vehicle_dimensions_height(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w,*wb,*wk,*wl,*we,*wnext;
    char *name=data;
    wb=gui_internal_menu(this,_("Change Height"));
    w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
    gui_internal_widget_append(w, we);
    gui_internal_widget_append(we, wk=gui_internal_label_new(this, name));
    wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
    wk->background=this->background;
    wk->flags |= flags_expand|flags_fill;
    wk->func = gui_internal_call_linked_on_finish;
    wk->c=wm->c;
    gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
    wnext->state |= STATE_SENSITIVE;
    wnext->func = gui_internal_cmd_change_vehicle_dimensions_height_clicked;
    wnext->data=wk;
    wk->data=wnext;
    wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, wl);
    if (this->keyboard)
        gui_internal_widget_append(w, gui_internal_keyboard(this,
                    VKBD_NUMERIC | gui_internal_keyboard_init_mode(getenv("LANG"))));
    else
        gui_internal_keyboard_show_native(this, w, VKBD_NUMERIC | gui_internal_keyboard_init_mode(getenv("LANG")),
                                          getenv("LANG"));
    gui_internal_menu_render(this);
}



