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

#include <glib.h>
#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "item.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"
#include "event.h"
#include "debug.h"
#include "window.h"
#include "callback.h"
#if defined(WINDOWS) || defined(WIN32) || defined (HAVE_API_WIN32_CE)
#include <windows.h>
# define sleep(i) Sleep(i * 1000)
#endif

static struct callback_list* callbacks;

static struct graphics_priv {
    int dummy;
} graphics_priv;

static struct graphics_font_priv {
    int dummy;
} graphics_font_priv;

static struct graphics_gc_priv {
    int dummy;
} graphics_gc_priv;

static struct graphics_image_priv {
    int dummy;
} graphics_image_priv;

static void graphics_destroy(struct graphics_priv *gr) {
}

static void font_destroy(struct graphics_font_priv *font) {

}

static struct graphics_font_methods font_methods = {
    font_destroy
};

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, char *font,
        int size, int flags) {
    *meth=font_methods;
    return &graphics_font_priv;
}

static void gc_destroy(struct graphics_gc_priv *gc) {
}

static void gc_set_linewidth(struct graphics_gc_priv *gc, int w) {
}

static void gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n) {
}

static void gc_set_foreground(struct graphics_gc_priv *gc, struct color *c) {
}

static void gc_set_background(struct graphics_gc_priv *gc, struct color *c) {
}

static struct graphics_gc_methods gc_methods = {
    gc_destroy,
    gc_set_linewidth,
    gc_set_dashes,
    gc_set_foreground,
    gc_set_background
};

static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth) {
    *meth=gc_methods;
    return &graphics_gc_priv;
}

static struct graphics_image_priv *image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *path,
        int *w, int *h, struct point *hot, int rotation) {
    return &graphics_image_priv;
}

static void draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count) {
}

static void draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count) {
}

static void draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h) {
}

static void draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r) {
}


static void draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg,
                      struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy) {
}

static void draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p,
                       struct graphics_image_priv *img) {
}

static void draw_drag(struct graphics_priv *gr, struct point *p) {
}

static void background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc) {
}

static void draw_mode(struct graphics_priv *gr, enum draw_mode_num mode) {
}

static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p,
        int w, int h, int wraparound);

static void resize_callback(int w, int h) {
    callback_list_call_attr_2(callbacks, attr_resize, GINT_TO_POINTER(1), GINT_TO_POINTER(1));
}

static int graphics_null_fullscreen(struct window *w, int on) {
    return 1;
}

static void graphics_null_disable_suspend(struct window *w) {
}

static void *get_data(struct graphics_priv *this, char const *type) {
    if (strcmp(type, "window") == 0) {
        struct window *win;
        win = g_new0(struct window, 1);
        win->priv = this;
        win->fullscreen = graphics_null_fullscreen;
        win->disable_suspend = graphics_null_disable_suspend;
        resize_callback(1,1);
        return win;
    }
    return NULL;
}

static void image_free(struct graphics_priv *gr, struct graphics_image_priv *priv) {
}

static void get_text_bbox(struct graphics_priv *gr, struct graphics_font_priv *font, char *text, int dx, int dy,
                          struct point *ret, int estimate) {
}

static void overlay_disable(struct graphics_priv *gr, int disable) {
}

static void overlay_resize(struct graphics_priv *gr, struct point *p, int w, int h, int wraparound) {
}

static struct graphics_methods graphics_methods = {
    graphics_destroy,
    draw_mode,
    draw_lines,
    draw_polygon,
    draw_rectangle,
    draw_circle,
    draw_text,
    draw_image,
    NULL,
    draw_drag,
    font_new,
    gc_new,
    background_gc,
    overlay_new,
    image_new,
    get_data,
    image_free,
    get_text_bbox,
    overlay_disable,
    overlay_resize,
    NULL, /* show_native_keyboard */
    NULL, /* hide_native_keyboard */
};

static struct graphics_priv *overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p,
        int w, int h, int wraparound) {
    *meth=graphics_methods;
    return &graphics_priv;
}


static struct graphics_priv *graphics_null_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs,
        struct callback_list *cbl) {
    struct attr *event_loop_system = NULL;
    *meth=graphics_methods;

    event_loop_system = attr_search(attrs, NULL, attr_event_loop_system);

    if (event_loop_system && event_loop_system->u.str) {
        dbg(lvl_debug, "event_system is %s", event_loop_system->u.str);
        if (!event_request_system(event_loop_system->u.str, "graphics_null"))
            return NULL;
    } else {
        if (!event_request_system("null", "graphics_null"))
            return NULL;
    }
    callbacks = cbl;
    resize_callback(1,1);
    return &graphics_priv;
}

static void event_null_main_loop_run(void) {

    dbg(lvl_debug,"enter");
    for (;;)
        sleep(1);

}

static void event_null_main_loop_quit(void) {
    dbg(lvl_debug,"enter");
}

static struct event_watch *event_null_add_watch(int h, enum event_watch_cond cond, struct callback *cb) {
    dbg(lvl_debug,"enter");
    return NULL;
}

static void event_null_remove_watch(struct event_watch *ev) {
    dbg(lvl_debug,"enter");
}


static struct event_timeout *event_null_add_timeout(int timeout, int multi, struct callback *cb) {
    dbg(lvl_debug,"enter");
    return NULL;
}

static void event_null_remove_timeout(struct event_timeout *to) {
    dbg(lvl_debug,"enter");
}


static struct event_idle *event_null_add_idle(int priority, struct callback *cb) {
    dbg(lvl_debug,"enter");
    return NULL;
}

static void event_null_remove_idle(struct event_idle *ev) {
    dbg(lvl_debug,"enter");
}

static void event_null_call_callback(struct callback_list *cb) {
    dbg(lvl_debug,"enter");
}

static struct event_methods event_null_methods = {
    event_null_main_loop_run,
    event_null_main_loop_quit,
    event_null_add_watch,
    event_null_remove_watch,
    event_null_add_timeout,
    event_null_remove_timeout,
    event_null_add_idle,
    event_null_remove_idle,
    event_null_call_callback,
};

static struct event_priv *event_null_new(struct event_methods *meth) {
    *meth=event_null_methods;
    return NULL;
}


void plugin_init(void) {
    plugin_register_category_graphics("null", graphics_null_new);
    plugin_register_category_event("null", event_null_new);
}
