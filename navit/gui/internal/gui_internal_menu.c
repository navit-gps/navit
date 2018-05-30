#include <glib.h>
#include "color.h"
#include "coord.h"
#include "point.h"
#include "callback.h"
#include "graphics.h"
#include "debug.h"
#include "navit.h"
#include "navit_nls.h"
#include "xmlconfig.h"
#include "gui_internal.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_html.h"
#include "gui_internal_search.h"
#include "gui_internal_menu.h"

extern char *version;

static void gui_internal_menu_destroy(struct gui_priv *this, struct widget *w) {
    struct menu_data *menu_data=w->menu_data;
    if (menu_data) {
        if (menu_data->refresh_callback_obj.type) {
            struct object_func *func;
            func=object_func_lookup(menu_data->refresh_callback_obj.type);
            if (func && func->remove_attr)
                func->remove_attr(menu_data->refresh_callback_obj.u.data, &menu_data->refresh_callback);
        }
        if (menu_data->refresh_callback.u.callback)
            callback_destroy(menu_data->refresh_callback.u.callback);

        g_free(menu_data->href);
        g_free(menu_data);
    }
    gui_internal_widget_destroy(this, w);
    this->root.children=g_list_remove(this->root.children, w);
}

static void gui_internal_prune_menu_do(struct gui_priv *this, struct widget *w, int render) {
    GList *l;
    struct widget *wr,*wd;
    gui_internal_search_idle_end(this);
    while ((l = g_list_last(this->root.children))) {
        wd=l->data;
        if (wd == w) {
            void (*redisplay)(struct gui_priv *priv, struct widget *widget, void *data);
            if (!render)
                return;
            gui_internal_say(this, w, 0);
            redisplay=w->menu_data->redisplay;
            wr=w->menu_data->redisplay_widget;
            if (!w->menu_data->redisplay && !w->menu_data->href) {
                gui_internal_widget_render(this, w);
                return;
            }
            if (redisplay) {
                gui_internal_menu_destroy(this, w);
                redisplay(this, wr, wr->data);
            } else {
                char *href=g_strdup(w->menu_data->href);
                gui_internal_menu_destroy(this, w);
                gui_internal_html_load_href(this, href, 0);
                g_free(href);
            }
            return;
        }
        gui_internal_menu_destroy(this, wd);
    }
}

void gui_internal_prune_menu(struct gui_priv *this, struct widget *w) {
    gui_internal_prune_menu_do(this, w, 1);
}

void gui_internal_prune_menu_count(struct gui_priv *this, int count, int render) {
    GList *l=g_list_last(this->root.children);
    struct widget *w=NULL;
    while (l && count-- > 0)
        l=g_list_previous(l);
    if (l) {
        w=l->data;
        gui_internal_prune_menu_do(this, w, render);
    }
}


/**
 * @brief Initializes a GUI screen
 *
 * This method initializes the internal GUI's screen on which all other elements (such as HTML menus,
 * dialogs or others) are displayed.
 *
 * It sets up a view hierarchy, which includes a title bar and a client area to hold widgets defined by
 * the caller.
 *
 * @param this The GUI instance
 * @param label The label to display in the top bar
 *
 * @return The container for caller-defined widgets
 */
struct widget *
gui_internal_menu(struct gui_priv *this, const char *label) {
    struct widget *menu,*w,*w1,*topbox;
    struct padding *padding = NULL;

    if (this->gra) {
        padding = graphics_get_data(this->gra, "padding");
    } else
        dbg(lvl_warning, "cannot get padding: this->gra is NULL");

    gui_internal_search_idle_end(this);
    topbox=gui_internal_box_new_with_label(this, 0, label);
    topbox->w=this->root.w;
    topbox->h=this->root.h;
    gui_internal_widget_append(&this->root, topbox);
    menu=gui_internal_box_new(this, gravity_left_center|orientation_vertical);

    if (padding) {
        menu->p.x = padding->left;
        menu->p.y = padding->top;
        menu->w = topbox->w - padding->left - padding->right;
        menu->h = topbox->h - padding->top - padding->bottom;
    } else {
        menu->p.x = 0;
        menu->p.y = 0;
        menu->w = topbox->w;
        menu->h = topbox->h;
    }
    menu->background=this->background;
    gui_internal_apply_config(this);
    topbox->menu_data=g_new0(struct menu_data, 1);
    gui_internal_widget_append(topbox, menu);
    w=gui_internal_top_bar(this);
    gui_internal_widget_append(menu, w);
    w=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_expand|flags_fill);
    w->spx=4*this->spacing;
    w->w=menu->w;
    gui_internal_widget_append(menu, w);
    if (this->flags & 16 && !(this->flags & 1024)) {
        struct widget *wlb,*wb,*wm=w;
        wm->flags=gravity_center|orientation_vertical|flags_expand|flags_fill;
        w=gui_internal_box_new(this, gravity_center|orientation_horizontal|flags_expand|flags_fill);
        dbg(lvl_info,"topbox->menu_data=%p", topbox->menu_data);
        gui_internal_widget_append(wm, w);
        wb=gui_internal_box_new(this, gravity_right_center|orientation_horizontal|flags_fill);
        wb->bl=6;
        wb->br=6;
        wb->bb=6;
        wb->bt=6;
        wb->spx=6;
        topbox->menu_data->button_bar=wb;
        gui_internal_widget_append(wm, wb);
        wlb=gui_internal_button_label(this,_("Back"),1);
        wlb->func=gui_internal_back;
        wlb->state |= STATE_SENSITIVE;
    }
    if (this->flags & 192) {
        menu=gui_internal_box_new(this, gravity_left_center|orientation_vertical);
        if (padding) {
            menu->p.x = padding->left;
            menu->p.y = padding->top;
            menu->w = topbox->w - padding->left - padding->right;
            menu->h = topbox->h - padding->top - padding->bottom;
        } else {
            menu->p.x = 0;
            menu->p.y = 0;
            menu->w = topbox->w;
            menu->h = topbox->h;
        }
        w1=gui_internal_time_help(this);
        gui_internal_widget_append(menu, w1);
        w1=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_expand|flags_fill);
        gui_internal_widget_append(menu, w1);
        gui_internal_widget_append(topbox, menu);
        menu->background=NULL;
    }
    gui_internal_widget_pack(this, topbox);
    gui_internal_widget_reset_pack(this, topbox);
    topbox->w=this->root.w;
    topbox->h=this->root.h;
    if (padding) {
        menu->p.x = padding->left;
        menu->p.y = padding->top;
        menu->w = topbox->w - padding->left - padding->right;
        menu->h = topbox->h - padding->top - padding->bottom;
    } else {
        menu->p.x = 0;
        menu->p.y = 0;
        menu->w = topbox->w;
        menu->h = topbox->h;
    }
    return w;
}

struct menu_data *
gui_internal_menu_data(struct gui_priv *this) {
    GList *l;
    struct widget *menu;

    l=g_list_last(this->root.children);
    menu=l->data;
    return menu->menu_data;
}

void gui_internal_menu_reset_pack(struct gui_priv *this) {
    GList *l;
    struct widget *top_box;

    l=g_list_last(this->root.children);
    top_box=l->data;
    gui_internal_widget_reset_pack(this, top_box);
}

void gui_internal_menu_render(struct gui_priv *this) {
    GList *l;
    struct widget *menu;

    l=g_list_last(this->root.children);
    menu=l->data;
    gui_internal_say(this, menu, 0);
    gui_internal_widget_pack(this, menu);
    gui_internal_widget_render(this, menu);
}

struct widget *
gui_internal_top_bar(struct gui_priv *this) {
    struct widget *w,*wm,*wh,*wc,*wcn;
    int dots_len, sep_len;
    GList *res=NULL,*l;
    int width,width_used=0,use_sep=0,incomplete=0;
    struct graphics_gc *foreground=(this->flags & 256 ? this->background : this->text_foreground);
    /* flags
            1:Don't expand bar to screen width
            2:Don't show Map Icon
            4:Don't show Home Icon
            8:Show only current menu
            16:Don't use menu titles as button
            32:Show navit version
    	64:Show time
    	128:Show help
    	256:Use background for menu headline
    	512:Set osd_configuration and zoom to route when setting position
    	1024:Don't show back button
    	2048:No highlighting of keyboard
    	4096:Center menu title
    */

    w=gui_internal_box_new(this, (this->flags & 4096 ? gravity_center : gravity_left_center)|orientation_horizontal|
                           (this->flags & 1 ? 0:flags_fill));
    w->bl=this->spacing;
    w->spx=this->spacing;
    w->background=this->background2;
    if ((this->flags & 6) == 6) {
        w->bl=10;
        w->br=10;
        w->bt=6;
        w->bb=6;
    }
    width=this->root.w-w->bl;
    if (! (this->flags & 2)) {
        wm=gui_internal_button_new_with_callback(this, NULL,
                image_new_s(this, "gui_map"), gravity_center|orientation_vertical,
                gui_internal_cmd_return, NULL);
        wm->speech=g_strdup(_("Back to map"));
        gui_internal_widget_pack(this, wm);
        gui_internal_widget_append(w, wm);
        width-=wm->w;
    }
    if (! (this->flags & 4)) {
        wh=gui_internal_button_new_with_callback(this, NULL,
                image_new_s(this, "gui_home"), gravity_center|orientation_vertical,
                gui_internal_cmd_main_menu, NULL);
        wh->speech=g_strdup(_("Main Menu"));
        gui_internal_widget_pack(this, wh);
        gui_internal_widget_append(w, wh);
        width-=wh->w;
    }
    if (!(this->flags & 6))
        width-=w->spx;
    l=g_list_last(this->root.children);
    wcn=gui_internal_label_new(this,".. »");
    wcn->foreground=foreground;
    dots_len=wcn->w;
    gui_internal_widget_destroy(this, wcn);
    wcn=gui_internal_label_new(this,"»");
    wcn->foreground=foreground;
    sep_len=wcn->w;
    gui_internal_widget_destroy(this, wcn);
    while (l) {
        if (g_list_previous(l) || !g_list_next(l)) {
            wc=l->data;
            wcn=gui_internal_label_new(this, wc->text);
            wcn->foreground=foreground;
            if (g_list_next(l))
                use_sep=1;
            else
                use_sep=0;
            dbg(lvl_debug,"%d (%s) + %d + %d + %d > %d", wcn->w, wc->text, width_used, w->spx, use_sep ? sep_len : 0, width);
            if (wcn->w + width_used + w->spx + (use_sep ? sep_len : 0) + (g_list_previous(l) ? dots_len : 0) > width) {
                incomplete=1;
                gui_internal_widget_destroy(this, wcn);
                break;
            }
            if (use_sep) {
                struct widget *wct=gui_internal_label_new(this, "»");
                wct->foreground=foreground;
                res=g_list_prepend(res, wct);
                width_used+=sep_len+w->spx;
            }
            width_used+=wcn->w;
            if (!(this->flags & 16)) {
                wcn->func=gui_internal_cmd_return;
                wcn->data=wc;
                wcn->state |= STATE_SENSITIVE;
            }
            res=g_list_prepend(res, wcn);
            if (this->flags & 8)
                break;
        }
        l=g_list_previous(l);
    }
    if (incomplete) {
        if (! res) {
            wcn=gui_internal_label_new_abbrev(this, wc->text, width-width_used-w->spx-dots_len);
            wcn->foreground=foreground;
            wcn->func=gui_internal_cmd_return;
            wcn->data=wc;
            wcn->state |= STATE_SENSITIVE;
            res=g_list_prepend(res, wcn);
            l=g_list_previous(l);
            wc=l?l->data:NULL;
        }
        if(wc) {
            wcn=gui_internal_label_new(this, ".. »");
            wcn->foreground=foreground;
            wcn->func=gui_internal_cmd_return;
            wcn->data=wc;
            wcn->state |= STATE_SENSITIVE;
            res=g_list_prepend(res, wcn);
        }
    }
    l=res;
    while (l) {
        gui_internal_widget_append(w, l->data);
        l=g_list_next(l);
    }
    if (this->flags & 32) {
        char *version_text=g_strdup_printf("Navit %s",version);
        wcn=gui_internal_label_new(this, version_text);
        g_free(version_text);
        wcn->flags=gravity_right_center|flags_expand;
        gui_internal_widget_append(w, wcn);
    }
#if 0
    if (dots)
        gui_internal_widget_destroy(this, dots);
#endif
    return w;
}
