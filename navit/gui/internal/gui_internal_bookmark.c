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
#include "gui_internal_bookmark.h"

static void gui_internal_cmd_add_bookmark_do(struct gui_priv *this, struct widget *widget) {
    GList *l;
    struct attr attr;
    dbg(lvl_debug,"text='%s'", widget->text);
    if (widget->text && strlen(widget->text)) {
        navit_get_attr(this->nav, attr_bookmarks, &attr, NULL);
        bookmarks_add_bookmark(attr.u.bookmarks, &widget->c, widget->text);
    }
    g_free(widget->text);
    widget->text=NULL;
    l=g_list_previous(g_list_last(this->root.children));
    gui_internal_prune_menu(this, l->data);
}

static void gui_internal_cmd_add_bookmark_folder_do(struct gui_priv *this, struct widget *widget) {
    GList *l;
    struct attr attr;
    dbg(lvl_debug,"text='%s'", widget->text);
    if (widget->text && strlen(widget->text)) {
        navit_get_attr(this->nav, attr_bookmarks, &attr, NULL);
        bookmarks_add_bookmark(attr.u.bookmarks, NULL, widget->text);
    }
    g_free(widget->text);
    widget->text=NULL;
    l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
    gui_internal_prune_menu(this, l->data);
}

static void gui_internal_cmd_add_bookmark_clicked(struct gui_priv *this, struct widget *widget, void *data) {
    gui_internal_cmd_add_bookmark_do(this, widget->data);
}

static void gui_internal_cmd_add_bookmark_folder_clicked(struct gui_priv *this, struct widget *widget, void *data) {
    gui_internal_cmd_add_bookmark_folder_do(this, widget->data);
}

static void gui_internal_cmd_rename_bookmark_clicked(struct gui_priv *this, struct widget *widget,void *data) {
    struct widget *w=(struct widget*)widget->data;
    GList *l;
    struct attr attr;
    dbg(lvl_debug,"text='%s'", w->text);
    if (w->text && strlen(w->text)) {
        navit_get_attr(this->nav, attr_bookmarks, &attr, NULL);
        bookmarks_rename_bookmark(attr.u.bookmarks, w->name, w->text);
    }
    g_free(w->text);
    g_free(w->name);
    w->text=NULL;
    w->name=NULL;
    l=g_list_previous(g_list_previous(g_list_previous(g_list_last(this->root.children))));
    gui_internal_prune_menu(this, l->data);
}

void gui_internal_cmd_add_bookmark2(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w,*wb,*wk,*wl,*we,*wnext;
    char *name=data;
    wb=gui_internal_menu(this,_("Add Bookmark"));
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
    wnext->func = gui_internal_cmd_add_bookmark_clicked;
    wnext->data=wk;
    wk->data=wnext;
    wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, wl);
    if (this->keyboard)
        gui_internal_widget_append(w, gui_internal_keyboard(this,
                                   VKBD_FLAG_2 | gui_internal_keyboard_init_mode(getenv("LANG"))));
    else
        gui_internal_keyboard_show_native(this, w, VKBD_FLAG_2 | gui_internal_keyboard_init_mode(getenv("LANG")),
                                          getenv("LANG"));
    gui_internal_menu_render(this);
}

void gui_internal_cmd_add_bookmark_folder2(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w,*wb,*wk,*wl,*we,*wnext;
    char *name=data;
    wb=gui_internal_menu(this,_("Add Bookmark folder"));
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
    wnext->func = gui_internal_cmd_add_bookmark_folder_clicked;
    wnext->data=wk;
    wk->data=wnext;
    wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, wl);
    if (this->keyboard)
        gui_internal_widget_append(w, gui_internal_keyboard(this,
                                   VKBD_FLAG_2 | gui_internal_keyboard_init_mode(getenv("LANG"))));
    else
        gui_internal_keyboard_show_native(this, w, VKBD_FLAG_2 | gui_internal_keyboard_init_mode(getenv("LANG")),
                                          getenv("LANG"));
    gui_internal_menu_render(this);
}

void gui_internal_cmd_rename_bookmark(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w,*wb,*wk,*wl,*we,*wnext;
    char *name=wm->text;
    wb=gui_internal_menu(this,_("Rename"));
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
    wk->name=g_strdup(name);
    gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
    wnext->state |= STATE_SENSITIVE;
    wnext->func = gui_internal_cmd_rename_bookmark_clicked;
    wnext->data=wk;
    wk->data=wnext;
    wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, wl);
    if (this->keyboard)
        gui_internal_widget_append(w, gui_internal_keyboard(this,
                                   VKBD_FLAG_2 | gui_internal_keyboard_init_mode(getenv("LANG"))));
    else
        gui_internal_keyboard_show_native(this, w, VKBD_FLAG_2 | gui_internal_keyboard_init_mode(getenv("LANG")),
                                          getenv("LANG"));
    gui_internal_menu_render(this);
}

void gui_internal_cmd_cut_bookmark(struct gui_priv *this, struct widget *wm, void *data) {
    struct attr mattr;
    GList *l;
    navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL);
    bookmarks_cut_bookmark(mattr.u.bookmarks,wm->text);
    l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
    gui_internal_prune_menu(this, l->data);
}

void gui_internal_cmd_copy_bookmark(struct gui_priv *this, struct widget *wm, void *data) {
    struct attr mattr;
    GList *l;
    navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL);
    bookmarks_copy_bookmark(mattr.u.bookmarks,wm->text);
    l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
    gui_internal_prune_menu(this, l->data);
}

void gui_internal_cmd_paste_bookmark(struct gui_priv *this, struct widget *wm, void *data) {
    struct attr mattr;
    GList *l;
    navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL);
    bookmarks_paste_bookmark(mattr.u.bookmarks);
    l=g_list_previous(g_list_last(this->root.children));
    if(l)
        gui_internal_prune_menu(this, l->data);
}

void gui_internal_cmd_delete_bookmark_folder(struct gui_priv *this, struct widget *wm, void *data) {
    struct attr mattr;
    GList *l;
    navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL);
    bookmarks_move_up(mattr.u.bookmarks);
    bookmarks_delete_bookmark(mattr.u.bookmarks,wm->prefix);
    l=g_list_first(this->root.children);
    gui_internal_prune_menu(this, l->data);
}

void gui_internal_cmd_load_bookmarks_as_waypoints(struct gui_priv *this, struct widget *wm, void *data) {
    struct attr mattr;

    if(navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL) ) {
        struct attr attr;
        struct item *item;
        struct coord c;
        struct pcoord *pc;
        enum projection pro=bookmarks_get_projection(mattr.u.bookmarks);
        int i, bm_count;

        navit_set_destination(this->nav, NULL, NULL, 0);

        bm_count=bookmarks_get_bookmark_count(mattr.u.bookmarks);
        pc=g_alloca(bm_count*sizeof(struct pcoord));
        bookmarks_item_rewind(mattr.u.bookmarks);
        i=0;
        while ((item=bookmarks_get_item(mattr.u.bookmarks))) {
            if (!item_attr_get(item, attr_label, &attr))
                continue;
            if (item->type == type_bookmark) {
                if (item_coord_get(item, &c, 1)) {
                    pc[i].x=c.x;
                    pc[i].y=c.y;
                    pc[i].pro=pro;
                    navit_add_destination_description(this->nav,&pc[i],attr.u.str);
                    i++;
                }
            }
        }
        bm_count=i;

        if (bm_count>0) {
            navit_set_destinations(this->nav, pc, bm_count, wm->prefix, 1);
            if (this->flags & 512) {
                struct attr follow;
                follow.type=attr_follow;
                follow.u.num=180;
                navit_set_attr(this->nav, &this->osd_configuration);
                navit_set_attr(this->nav, &follow);
                navit_zoom_to_route(this->nav, 0);
            }
        }
    }

    gui_internal_prune_menu(this, NULL);
}

void gui_internal_cmd_replace_bookmarks_from_waypoints(struct gui_priv *this, struct widget *wm, void *data) {
    struct attr mattr;

    if(navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL) ) {
        struct attr attr;
        char *desc=NULL;
        struct pcoord *pc;
        int i, bm_count;

        if (bookmarks_get_bookmark_count(mattr.u.bookmarks)>0) {
            struct item *item;
            bookmarks_item_rewind(mattr.u.bookmarks);

            while ((item=bookmarks_get_item(mattr.u.bookmarks))) {

                if (!item_attr_get(item, attr_label, &attr))
                    continue;

                if (item->type == type_bookmark)
                    bookmarks_delete_bookmark(mattr.u.bookmarks,  attr.u.str);

                bookmarks_move_down(mattr.u.bookmarks,wm->prefix);
            }
        }
        bookmarks_item_rewind(mattr.u.bookmarks);

        bm_count=navit_get_destination_count(this->nav);
        pc=g_alloca(bm_count*sizeof(struct pcoord));
        navit_get_destinations(this->nav, pc, bm_count);

        for (i=0; i<bm_count; i++) {
            char *tmp=navit_get_destination_description(this->nav, i);
            desc=g_strdup_printf("%s WP%d", tmp, i+1);
            g_free(tmp);
            navit_get_attr(this->nav, attr_bookmarks, &attr, NULL);
            bookmarks_add_bookmark(attr.u.bookmarks, &pc[i], desc);
            bookmarks_move_down(mattr.u.bookmarks,wm->prefix);
            g_free(desc);
        }
    }

    gui_internal_prune_menu(this, NULL);
}
