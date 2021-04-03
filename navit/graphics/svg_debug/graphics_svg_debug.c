/*
 * Navit, a modular navigation system.
 * Copyright (C) 2020 Navit Team
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
/**
 * This Plugin can act as a Plugin
 *
 */
#include "color.h"
#include <glib.h>
#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>
#include <gio/gio.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>

#include "config.h"
#include "debug.h"
#include "point.h"
#include "graphics.h"

#include "plugin.h"
#include "window.h"
#include "navit.h"
#include "keys.h"
#include "item.h"
#include "attr.h"
#include "item.h"
#include "attr.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "navit.h"
#include "event.h"
#include "debug.h"
#include "callback.h"
#include "util.h"

struct graphics_priv {

    unsigned int frame;
    int width;
    int height;
    int fullscreen;
    struct color bg_color;
    FILE *outfile;
    const char *outputdir;
    void *proxy_priv;
    struct graphics_methods *proxy_graphics_methods;
    struct navit *nav;
    struct callback_list *cbl;
};

struct graphics_font_priv {
    int size;
    char *font;
    struct graphics_font graphics_font_proxy;
};

struct graphics_gc_priv {
    struct graphics_priv *gr;
    unsigned int *dashed;
    int is_dashed;
    struct color fg;
    struct color bg;
    int linewidth;
    struct graphics_image_priv *img;
    struct graphics_gc_priv *graphics_gc_priv_proxy;
    struct graphics_gc_methods *graphics_gc_methods_proxy;
};

struct graphics_image_priv {
    int w, h;
    char *data;
    struct graphics_image_priv *graphics_image_priv_proxy;
    struct graphics_image_methods *graphics_image_methods_proxy;
};

static void svg_debug_graphics_destroy(struct graphics_priv *gr) {
    // TODO
}

static void svg_debug_font_destroy(struct graphics_font_priv *this);
void svg_debug_get_text_bbox(struct graphics_priv *gr,
                             struct graphics_font_priv *font, char *text, int dx, int dy,
                             struct point *ret, int estimate);

static struct graphics_font_methods font_methods = { .font_destroy =
        svg_debug_font_destroy,
};

static void resize_callback_do(struct graphics_priv *gr, int w, int h) {
    dbg(lvl_debug, "resize_callback w:%i h:%i", w, h)
    gr->width = w;
    gr->height = h;
}

static void svg_debug_font_destroy(struct graphics_font_priv *this) {
    dbg(lvl_debug, "enter font_destroy");
    if (this->graphics_font_proxy.meth.font_destroy) {
        this->graphics_font_proxy.meth.font_destroy(
            this->graphics_font_proxy.priv);
    }
    g_free(this);
}

static struct graphics_font_priv* svg_debug_font_new(struct graphics_priv *gr,
        struct graphics_font_methods *meth, char *font, int size, int flags) {
    struct graphics_font_priv *priv = g_new(struct graphics_font_priv, 1);

    *meth = font_methods;

    priv->size = size / 10;
    if (gr->proxy_graphics_methods->font_new) {
        priv->graphics_font_proxy.priv = gr->proxy_graphics_methods->font_new(
                                             gr->proxy_priv, &priv->graphics_font_proxy.meth, font, size,
                                             flags);
    } else {
        return priv;
    }
    if (priv->graphics_font_proxy.priv) {
        return priv;
    }
    return NULL;
}
void svg_debug_get_text_bbox(struct graphics_priv *gr,
                             struct graphics_font_priv *font, char *text, int dx, int dy,
                             struct point *ret, int estimate) {
    if (gr->proxy_graphics_methods->get_text_bbox) {
        gr->proxy_graphics_methods->get_text_bbox(gr->proxy_priv,
                font->graphics_font_proxy.priv, text, dx, dy, ret, estimate);
    }
}

static void svg_debug_gc_destroy(struct graphics_gc_priv *gc) {
    if (gc->graphics_gc_methods_proxy->gc_destroy) {
        gc->graphics_gc_methods_proxy->gc_destroy(gc->graphics_gc_priv_proxy);
    }
    g_free(gc->graphics_gc_methods_proxy);
    g_free(gc);
}

static void svg_debug_gc_set_linewidth(struct graphics_gc_priv *gc, int w) {
    gc->linewidth = w;
    if (gc->graphics_gc_methods_proxy->gc_set_linewidth) {
        gc->graphics_gc_methods_proxy->gc_set_linewidth(
            gc->graphics_gc_priv_proxy, w);
    }

}

static void svg_debug_gc_set_dashes(struct graphics_gc_priv *gc, int w,
                                    int offset, unsigned char *dash_list, int n) {
    gc->dashed = dash_list;
    gc->is_dashed = TRUE;
    if (gc->graphics_gc_methods_proxy->gc_set_dashes) {
        gc->graphics_gc_methods_proxy->gc_set_dashes(gc->graphics_gc_priv_proxy,
                w, offset, dash_list, n);
    }
}

static void svg_debug_gc_set_foreground(struct graphics_gc_priv *gc,
                                        struct color *c) {
    gc->fg.r = c->r / 256;
    gc->fg.g = c->g / 256;
    gc->fg.b = c->b / 256;
    gc->fg.a = c->a / 256;
    if (gc->graphics_gc_methods_proxy->gc_set_foreground) {
        gc->graphics_gc_methods_proxy->gc_set_foreground(
            gc->graphics_gc_priv_proxy, c);
    }
}

static void svg_debug_gc_set_background(struct graphics_gc_priv *gc,
                                        struct color *c) {
    gc->bg.r = c->r / 256;
    gc->bg.g = c->g / 256;
    gc->bg.b = c->b / 256;
    gc->bg.a = c->a / 256;
    if (gc->graphics_gc_methods_proxy->gc_set_foreground) {
        gc->graphics_gc_methods_proxy->gc_set_foreground(
            gc->graphics_gc_priv_proxy, c);
    }
}

static void svg_debug_gc_set_texture(struct graphics_gc_priv *gc,
                                     struct graphics_image_priv *img) {
    gc->img = img;
}

static struct graphics_gc_methods gc_methods = { .gc_destroy =
        svg_debug_gc_destroy, .gc_set_linewidth = svg_debug_gc_set_linewidth,
                              .gc_set_dashes = svg_debug_gc_set_dashes, .gc_set_foreground =
                                          svg_debug_gc_set_foreground, .gc_set_background =
                                                  svg_debug_gc_set_background, .gc_set_texture =
                                                          svg_debug_gc_set_texture,
};

static struct graphics_gc_priv* svg_debug_gc_new(struct graphics_priv *gr,
        struct graphics_gc_methods *meth) {
    struct graphics_gc_priv *gc = g_new0(struct graphics_gc_priv, 1);
    struct graphics_gc_priv *graphics_gc_priv_proxy = g_new0(
                struct graphics_gc_priv, 1);
    struct graphics_gc_methods *graphics_gc_methods_proxy = g_new0(
                struct graphics_gc_methods, 1);
    *meth = gc_methods;
    gc->gr = gr;
    gc->is_dashed = FALSE;
    gc->linewidth = 1;

    if (gr->proxy_graphics_methods->gc_new) {
        gr->proxy_graphics_methods->gc_new(gr->proxy_priv,
                                           graphics_gc_methods_proxy);
    }
    gc->graphics_gc_methods_proxy = graphics_gc_methods_proxy;
    gc->graphics_gc_priv_proxy = graphics_gc_priv_proxy;
    return gc;
}

void svg_debug_image_destroy(struct graphics_image_priv *img);
void svg_debug_image_destroy(struct graphics_image_priv *img) {
    dbg(lvl_debug, "enter image_destroy");
    g_free(img->data);
    if (img->graphics_image_methods_proxy->image_destroy) {
        img->graphics_image_methods_proxy->image_destroy(
            img->graphics_image_priv_proxy);
    }
    g_free(img->data);
    g_free(img->graphics_image_methods_proxy);
    g_free(img->graphics_image_priv_proxy);
    g_free(img);
}

static struct graphics_image_methods image_methods = { .image_destroy =
        svg_debug_image_destroy,
};

static struct graphics_image_priv* svg_debug_image_new(struct graphics_priv *gr,
        struct graphics_image_methods *meth, char *path, int *w, int *h,
        struct point *hot, int rotation) {
    struct graphics_image_priv *image_priv;
    struct graphics_image_methods *graphics_image_methods;
    char *base64_data_url = NULL;
    char *base64_encoded_image = NULL;
    char *image_mime_type = NULL;
    char *contents = NULL;
    char fileext[3] = "";
    gsize img_size;

    image_priv = g_new0(struct graphics_image_priv, 1);
    graphics_image_methods = g_new0(struct graphics_image_methods, 1);
    *meth = image_methods;
    const char *data_url_template = "data:%s;base64,%s";

    if (g_file_get_contents(path, &contents, &img_size, NULL)) {
        dbg(lvl_debug, "image_new loaded %s", path);

        strtolower(fileext, &path[(strlen(path) - 3)]);
        if (strcmp(fileext, "png")) {
            image_mime_type = "image/png";
        } else if (strcmp(fileext, "jpg")) {
            image_mime_type = "image/jpeg";
        } else if (strcmp(fileext, "gif")) {
            image_mime_type = "image/gif";
        } else {
            image_mime_type = "application/octet-stream";
        }
        base64_encoded_image = g_base64_encode((guchar*) contents, img_size);

        base64_data_url = g_malloc0(
                              strlen(base64_encoded_image) + strlen(data_url_template)
                              + strlen(image_mime_type) + 1);
        sprintf(base64_data_url, data_url_template, image_mime_type,
                base64_encoded_image);
        g_free(base64_encoded_image);

        image_priv->data = base64_data_url;
        g_free(contents);
        image_priv->h = *h;
        image_priv->w = *w;
    } else {
        dbg(lvl_error, "image_new failed to load %s", path);
        image_priv->data =
            "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=";
        image_priv->h = 1;
        image_priv->w = 1;
    }
    if (gr->proxy_graphics_methods->image_new) {
        image_priv->graphics_image_priv_proxy =
            gr->proxy_graphics_methods->image_new(gr->proxy_priv,
                    graphics_image_methods, path, w, h, hot, rotation);
        image_priv->graphics_image_methods_proxy = graphics_image_methods;
    }
    if (image_priv->graphics_image_priv_proxy) {
        return image_priv;
    }
    if (base64_data_url != NULL) {
        g_free(base64_data_url);
    }
    g_free(graphics_image_methods);
    g_free(image_priv);
    return NULL;
}

static void svg_debug_draw_lines(struct graphics_priv *gr,
                                 struct graphics_gc_priv *gc, struct point *p, int count) {
    const char *line_template_start = "<polyline points=\"";
    const char *coord_template = "%i,%i ";
    const char *dashed_template_start = "\" stroke-dasharray=\"";
    const char *dashed_dasharray = "%i ";
    const char *dashed_template_end = "";
    const char *line_template_end =
        "\" style=\"fill:none;stroke:rgb(%i,%i,%i);stroke-width:%i\" />\n";

    fprintf(gr->outfile, line_template_start, "");
    int i;
    for (i = 0; i < count; i++) {
        fprintf(gr->outfile, coord_template, p[i].x, p[i].y);
    }

    if (gc->is_dashed) {
        fprintf(gr->outfile, dashed_template_start, "");
        int i;
        for (i = 0; i < 4; i++) {
            fprintf(gr->outfile, dashed_dasharray, gc->dashed[i]);
        }
        fprintf(gr->outfile, dashed_template_end, "");
    }

    fprintf(gr->outfile, line_template_end, gc->fg.r, gc->fg.g, gc->fg.b,
            gc->linewidth);

    if (gr->proxy_graphics_methods->draw_lines) {
        gr->proxy_graphics_methods->draw_lines(gr->proxy_priv,
                                               gc->graphics_gc_priv_proxy, p, count);
    }

}

static void svg_debug_draw_polygon(struct graphics_priv *gr,
                                   struct graphics_gc_priv *gc, struct point *p, int count) {
    const char *coord_template = "%i,%i ";
    const char *polygon_template_start = "<polygon points=\"%s";
    const char *polygon_template_end = "\" style=\"fill:rgb(%i,%i,%i)\" />\n";
    fprintf(gr->outfile, polygon_template_start, "");
    int i;
    for (i = 0; i < count; i++) {
        fprintf(gr->outfile, coord_template, p[i].x, p[i].y);
    }
    fprintf(gr->outfile, polygon_template_end, gc->fg.r, gc->fg.g, gc->fg.b);

    if (gr->proxy_graphics_methods->draw_polygon) {
        gr->proxy_graphics_methods->draw_polygon(gr->proxy_priv,
                gc->graphics_gc_priv_proxy, p, count);
    }

}

static void svg_debug_draw_rectangle(struct graphics_priv *gr,
                                     struct graphics_gc_priv *gc, struct point *p, int w, int h) {
    const char *rectangle_template =
        "<rect x=\"%i\" y=\"%i\" width=\"%i\" height=\"%i\" style=\"fill:rgb(%i,%i,%i)\"></rect>\n";
    fprintf(gr->outfile, rectangle_template, p->x, p->y, w, h, gc->fg.r,
            gc->fg.g, gc->fg.b);

    if (gr->proxy_graphics_methods->draw_rectangle) {
        gr->proxy_graphics_methods->draw_rectangle(gr->proxy_priv,
                gc->graphics_gc_priv_proxy, p, w, h);
    }
}

static void svg_debug_draw_circle(struct graphics_priv *gr,
                                  struct graphics_gc_priv *gc, struct point *p, int r) {
    const char *circle_template =
        "<circle cx=\"%i\" cy=\"%i\" r=\"%i\" fill=\"rgb(%i,%i,%i)\" />\n";
    fprintf(gr->outfile, circle_template, p->x, p->y, r / 2, gc->fg.r, gc->fg.g,
            gc->fg.b);

    if (gr->proxy_graphics_methods->draw_circle) {
        gr->proxy_graphics_methods->draw_circle(gr->proxy_priv,
                                                gc->graphics_gc_priv_proxy, p, r);
    }

}

static void svg_debug_draw_text(struct graphics_priv *gr,
                                struct graphics_gc_priv *fg, struct graphics_gc_priv *bg,
                                struct graphics_font_priv *font, char *text, struct point *p, int dx,
                                int dy) {
    const char *image_template =
        "<text x=\"%i\" y=\"%i\" fill=\"rgb(%i,%i,%i)\" style=\"font-size: %ipt;\">%s</text>\n";
    if (dx == 0x10000 || dy == 0) {
        fprintf(gr->outfile, image_template, p->x, p->y, fg->fg.r, fg->fg.g,
                fg->fg.b, font ? font->size : 0, text);
    }
    if (gr->proxy_graphics_methods->draw_text && font) {
        gr->proxy_graphics_methods->draw_text(gr->proxy_priv,
                                              fg->graphics_gc_priv_proxy, bg->graphics_gc_priv_proxy,
                                              font->graphics_font_proxy.priv, text, p, dx, dy);
    }
}

static void svg_debug_draw_image(struct graphics_priv *gr,
                                 struct graphics_gc_priv *fg, struct point *p,
                                 struct graphics_image_priv *img) {
    // Write already encoded image to file
    const char *image_template =
        "<image x=\"%i\" y=\"%i\" width=\"%i\" height=\"%i\" xlink:href=\"%s\"></image>\n";
    fprintf(gr->outfile, image_template, p->x, p->y, img->w, img->h, img->data);
    if (gr->proxy_graphics_methods->draw_image) {
        gr->proxy_graphics_methods->draw_image(gr->proxy_priv, fg, p,
                                               img->graphics_image_priv_proxy);
    }
}

static void svg_debug_draw_drag(struct graphics_priv *gr, struct point *p) {
    if (gr->proxy_graphics_methods->draw_drag) {
        gr->proxy_graphics_methods->draw_drag(gr->proxy_priv, p);
    }
}

static void svg_debug_background_gc(struct graphics_priv *gr,
                                    struct graphics_gc_priv *gc) {
    if (gr->proxy_graphics_methods->background_gc) {
        gr->proxy_graphics_methods->background_gc(gr->proxy_priv,
                gc->graphics_gc_priv_proxy);
    }
}

static void svg_debug_draw_mode(struct graphics_priv *gr,
                                enum draw_mode_num mode) {
    const char *svg_start_template =
        "<svg height=\"%i\" width=\"%i\" xmlns= \"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n";
    const char *svg_end_template = "</svg>\n";
    char filename[255];
    switch (mode) {
    case draw_mode_begin:
        if (gr->outfile) {
            dbg(lvl_debug, "Finished drawing %s/svg_debug_after_frame_%u.svg",
                gr->outputdir, gr->frame)
            fprintf(gr->outfile, svg_end_template, "");
            fclose(gr->outfile);
            gr->frame += 1;
        }
        sprintf(filename, "%s/svg_debug_frame_%u.svg", gr->outputdir,
                gr->frame);
        gr->outfile = fopen(filename, "w");
        fprintf(gr->outfile, svg_start_template, gr->height, gr->width);
        break;
    case draw_mode_end:
        dbg(lvl_debug, "Finished drawing %s/svg_debug_after_frame_%u.svg",
            gr->outputdir, gr->frame)
        fprintf(gr->outfile, svg_end_template, "");
        fclose(gr->outfile);
        gr->frame += 1;
        sprintf(filename, "%s/svg_debug_after_frame_%u.svg", gr->outputdir,
                gr->frame);
        gr->outfile = fopen(filename, "w");
        fprintf(gr->outfile, svg_start_template, gr->height, gr->width);
        break;
    default:
        break;
    }
    if (gr->proxy_graphics_methods->draw_mode) {
        gr->proxy_graphics_methods->draw_mode(gr->proxy_priv, mode);
    }
}

static void graphics_svg_debug_overlay_draw_mode(struct graphics_priv *gr,
        enum draw_mode_num mode) {
    // TODO
    // https://stackoverflow.com/questions/5451135/embed-svg-in-svg
    // more or less like in draw_mode but with different format string
    // probably overlay_num so main svg can import it
}

static struct graphics_priv* graphics_svg_debug_overlay_new(
    struct graphics_priv *gr, struct graphics_methods *meth,
    struct point *p, int w, int h, int wraparound);

static int graphics_svg_debug_fullscreen(struct window *win, int on) {
    struct graphics_priv *graphics_priv = (struct graphics_priv*) win->priv;
    struct window *proxy_win;
    if (!graphics_priv->proxy_graphics_methods->get_data) {
        return 0;
    }
    proxy_win = graphics_priv->proxy_graphics_methods->get_data(
                    graphics_priv->proxy_priv, "window");
    if (proxy_win) {
        return proxy_win->fullscreen(proxy_win, on);
    }
    return 0;
}

static gboolean graphics_svg_debug_idle(void *data) {
    struct graphics_priv *gr = (struct graphics_priv*) data;
    static int first_run = TRUE;
    if (first_run) {
        callback_list_call_attr_2(gr->cbl, attr_resize,
                                  GINT_TO_POINTER(gr->width), GINT_TO_POINTER(gr->height));
        first_run = FALSE;
    }
    return TRUE;
}

static void graphics_svg_debug_disable_suspend(struct window *w) {
    struct graphics_priv *graphics_priv = (struct graphics_priv*) w->priv;
    struct window *proxy_win;
    if (!graphics_priv->proxy_graphics_methods->get_data) {
        return;
    }
    proxy_win = graphics_priv->proxy_graphics_methods->get_data(
                    graphics_priv->proxy_priv, "window");
    if (proxy_win) {
        proxy_win->disable_suspend(proxy_win);
    }
    return;
}

static void* svg_debug_get_data(struct graphics_priv *this, char const *type) {
    if (strcmp(type, "window") == 0) {
        struct window *win;
        win = g_new0(struct window, 1);
        win->priv = this;
        win->fullscreen = graphics_svg_debug_fullscreen;
        win->disable_suspend = graphics_svg_debug_disable_suspend;
        return win;
    }
    if (this->proxy_graphics_methods->get_data) {
        // FIXME: This will leak the proxied graphics_priv... But it works for now
        return this->proxy_graphics_methods->get_data(this->proxy_priv, type);
    }
    return NULL;
}

static void svg_debug_image_free(struct graphics_priv *gr,
                                 struct graphics_image_priv *img) {
    dbg(lvl_debug, "enter image_free");
    if (img->graphics_image_methods_proxy->image_destroy) {
        img->graphics_image_methods_proxy->image_destroy(
            img->graphics_image_priv_proxy);
    }
    if (gr->proxy_graphics_methods->image_free) {
        gr->proxy_graphics_methods->image_free(gr->proxy_priv,
                                               img->graphics_image_priv_proxy);
    }
    g_free(img->graphics_image_methods_proxy);
    g_free(img->data);
    g_free(img);
}

static void graphics_svg_debug_overlay_disable(struct graphics_priv *gr,
        int disable) {
    // TODO
}

static void graphics_svg_debug_overlay_resize(struct graphics_priv *gr,
        struct point *p, int w, int h, int wraparound) {
    // TODO
}

static struct graphics_methods graphics_methods = {
    .graphics_destroy = svg_debug_graphics_destroy,
    .draw_mode = svg_debug_draw_mode,
    .draw_lines = svg_debug_draw_lines,
    .draw_polygon = svg_debug_draw_polygon,
    .draw_rectangle = svg_debug_draw_rectangle,
    .draw_circle = svg_debug_draw_circle,
    .draw_text = svg_debug_draw_text,
    // FIXME: Text size calculation is hard, because the svg is
    // interpreted by the viewer, so we don't know its size

    .draw_image = svg_debug_draw_image,
    .draw_image_warp = NULL,
    .draw_drag = svg_debug_draw_drag,
    .font_new = svg_debug_font_new,
    .gc_new = svg_debug_gc_new,
    .background_gc = svg_debug_background_gc,
    .overlay_new = NULL, //graphics_svg_debug_overlay_new, // TODO
    .image_new = svg_debug_image_new,
    .get_data = svg_debug_get_data,
    .image_free = svg_debug_image_free,
    .get_text_bbox = svg_debug_get_text_bbox,
    .overlay_disable = NULL, // graphics_svg_debug_overlay_disable, // TODO
    .overlay_resize = NULL, // graphics_svg_debug_overlay_resize,  // TODO
    .set_attr = NULL, // TODO add proxy
    .show_native_keyboard = NULL, // TODO add proxy
    .hide_native_keyboard = NULL, // TODO add proxy
    .get_dpi = NULL, // TODO add proxy
    .draw_polygon_with_holes = NULL, // TODO add proxy

};

static struct graphics_priv* graphics_svg_debug_overlay_new(
    struct graphics_priv *gr, struct graphics_methods *meth,
    struct point *p, int w, int h, int wraparound) {
    struct graphics_priv *this = g_new0(struct graphics_priv, 1);
    *meth = graphics_methods;
    meth->draw_mode = graphics_svg_debug_overlay_draw_mode;

    // TODO

    return this;
}

static struct graphics_priv* graphics_svg_debug_new(struct navit *nav,
        struct graphics_methods *meth, struct attr **attrs,
        struct callback_list *cbl) {
    struct graphics_priv *this = g_new0(struct graphics_priv, 1);
    struct graphics_methods *proxy_graphics_methods = g_new0(
                struct graphics_methods, 1);
    struct attr *attr;
    void *proxy_priv = NULL;
    struct graphics_priv* (*proxy_gra)(struct navit *nav,
                                       struct graphics_methods *meth, struct attr **attrs,
                                       struct callback_list *cbl);

    *meth = graphics_methods;

    // Save Parameters for later
    this->nav = nav;
    this->cbl = cbl;

    // Read configuration
    this->width = 32;
    if ((attr = attr_search(attrs, attr_w)))
        this->width = attr->u.num;
    this->height = 32;
    if ((attr = attr_search(attrs, attr_h)))
        this->height = attr->u.num;

    this->outputdir = g_get_tmp_dir();
    if ((attr = attr_search(attrs, attr_outputdir)))
        this->outputdir = attr->u.str;

    // Get plugin to proxy
    proxy_gra = NULL;
    if ((attr = attr_search(attrs, attr_name))) {
        if (attr->u.str[0] != '\0') {
            proxy_gra = plugin_get_category_graphics(attr->u.str);
        }
        if (proxy_gra) {
            // Call proxy plugin
            proxy_priv = (*proxy_gra)(nav, proxy_graphics_methods, attrs, cbl);
        } else {
            dbg(lvl_error, "Failed to load graphics plugin %s.", attr->u.str);
            return NULL;
        }
    } else {
        if (!event_request_system("glib", "graphics_sdl_new")) {
            dbg(lvl_error, "event_request_system failed");
            g_free(this);
            return NULL;
        }
    }

    // Save proxy to call it later
    this->proxy_priv = proxy_priv;
    this->proxy_graphics_methods = proxy_graphics_methods;
    this->frame = 0;

    // If something tries to write before calling draw_mode with start for the first time
    this->outfile = fopen("/dev/null", "w");

    //callbacks = cbl;
    g_timeout_add(G_PRIORITY_DEFAULT + 10, graphics_svg_debug_idle, this);

    if (!proxy_gra) {
        dbg(lvl_debug, "No Proxied plugin, so do not set functions to NULL")
        // see comment below
        callback_list_call_attr_2(cbl, attr_resize,
                                  GINT_TO_POINTER(this->width), GINT_TO_POINTER(this->height));
        return this;
    }

    // The rest of this function makes sure that svg_debug only supports what the
    // proxied graphics plugin supports as well
    // for example some graphics plugins don't support circles, but svg_debug does.
    // if navit core sees that we support circles we would see it inside the svg, but not
    // in the proxied graphics
    //
    // get_data and draw_mode may not been set to null because they need to exist
    if (!this->proxy_graphics_methods->graphics_destroy) {
        meth->graphics_destroy = NULL;
    }
    if (!this->proxy_graphics_methods->draw_lines) {
        meth->draw_lines = NULL;
    }
    if (!this->proxy_graphics_methods->draw_polygon) {
        meth->draw_polygon = NULL;
    }
    if (!this->proxy_graphics_methods->draw_rectangle) {
        meth->draw_rectangle = NULL;
    }
    if (!this->proxy_graphics_methods->draw_circle) {
        meth->draw_circle = NULL;
    }
    if (!this->proxy_graphics_methods->draw_text) {
        meth->draw_text = NULL;
    }
    if (!this->proxy_graphics_methods->draw_image) {
        meth->draw_image = NULL;
    }
    if (!this->proxy_graphics_methods->draw_image_warp) {
        meth->draw_image_warp = NULL;
    }
    if (!this->proxy_graphics_methods->draw_drag) {
        meth->draw_drag = NULL;
    }
    if (!this->proxy_graphics_methods->font_new) {
        meth->font_new = NULL;
    }
    if (!this->proxy_graphics_methods->gc_new) {
        meth->gc_new = NULL;
    }
    if (!this->proxy_graphics_methods->background_gc) {
        meth->background_gc = NULL;
    }
    if (!this->proxy_graphics_methods->overlay_new) {
        meth->overlay_new = NULL;
    }
    if (!this->proxy_graphics_methods->image_new) {
        meth->image_new = NULL;
    }
    if (!this->proxy_graphics_methods->image_free) {
        meth->image_free = NULL;
    }
    if (!this->proxy_graphics_methods->get_text_bbox) {
        meth->get_text_bbox = NULL;
    }
    if (!this->proxy_graphics_methods->overlay_disable) {
        meth->overlay_disable = NULL;
    }
    if (!this->proxy_graphics_methods->overlay_resize) {
        meth->overlay_resize = NULL;
    }
    if (!this->proxy_graphics_methods->set_attr) {
        meth->set_attr = NULL;
    }
    if (!this->proxy_graphics_methods->show_native_keyboard) {
        meth->show_native_keyboard = NULL;
    }
    if (!this->proxy_graphics_methods->hide_native_keyboard) {
        meth->hide_native_keyboard = NULL;
    }
    if (!this->proxy_graphics_methods->get_dpi) {
        meth->get_dpi = NULL;
    }
    if (!this->proxy_graphics_methods->draw_polygon_with_holes) {
        meth->draw_polygon_with_holes = NULL;
    }

    // Add resize callback so we get called when window is resized so we can adjust the svg size
    struct callback *callback = callback_new_attr_1(callback_cast(resize_callback_do), attr_resize, this);
    callback_list_add(cbl, callback);
    return this;
}

void plugin_init(void) {
    dbg(lvl_error, "enter svg_debug plugin_init")
    plugin_register_category_graphics("svg_debug", graphics_svg_debug_new);
    //plugin_register_category_event("svg_debug", event_svg_debug_new);
}
