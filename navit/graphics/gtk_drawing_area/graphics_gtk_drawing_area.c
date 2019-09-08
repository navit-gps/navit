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

#define GDK_ENABLE_BROKEN
#include "config.h"
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <cairo.h>
#include <locale.h> /* For WIN32 */
#if !defined(GDK_Book) || !defined(GDK_Calendar)
#include <X11/XF86keysym.h>
#endif
#ifdef HAVE_IMLIB2
#include <Imlib2.h>
#endif

#ifndef _WIN32
#include <gdk/gdkx.h>
#endif
#include "event.h"
#include "debug.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "item.h"
#include "window.h"
#include "callback.h"
#include "keys.h"
#include "plugin.h"
#include "navit/font/freetype/font_freetype.h"
#include "navit.h"
#include <errno.h>

#ifndef GDK_Book
#define GDK_Book XF86XK_Book
#endif
#ifndef GDK_Calendar
#define GDK_Calendar XF86XK_Calendar
#endif


struct graphics_priv {
    GdkEventButton button_event;
    int button_timeout;
    GtkWidget *widget;
    GtkWidget *win;
    struct window window;
    cairo_t *cairo;
    struct point p;
    int width;
    int height;
    int win_w;
    int win_h;
    int visible;
    int overlay_disabled;
    int overlay_autodisabled;
    int wraparound;
    struct graphics_priv *parent;
    struct graphics_priv *overlays;
    struct graphics_priv *next;
    struct graphics_gc_priv *background_gc;
    struct callback_list *cbl;
    struct font_freetype_methods freetype_methods;
    struct navit *nav;
    int pid;
    struct timeval button_press[8];
    struct timeval button_release[8];
    int timeout;
    int delay;
    char *window_title;
};


struct graphics_gc_priv {
    struct graphics_priv *gr;
    struct color c;
    double linewidth;
    double *dashes;
    int ndashes;
    double offset;
};

struct graphics_image_priv {
    GdkPixbuf *pixbuf;
    int w;
    int h;
#ifdef HAVE_IMLIB2
    void *image;
#endif
};

static void graphics_destroy(struct graphics_priv *gr) {
    dbg(lvl_debug,"enter parent %p",gr->parent);
    gr->freetype_methods.destroy();
    if (!gr->parent) {
        dbg(lvl_debug,"enter win %p",gr->win);
        if (gr->win)
            gtk_widget_destroy(gr->win);
        dbg(lvl_debug,"widget %p",gr->widget);
        if (gr->widget)
            gtk_widget_destroy(gr->widget);
        g_free(gr->window_title);
    }
    g_free(gr);
}

static void gc_destroy(struct graphics_gc_priv *gc) {
    g_free(gc);
}

static void gc_set_linewidth(struct graphics_gc_priv *gc, int w) {
    gc->linewidth = w;
}

static void gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n) {
    int i;
    g_free(gc->dashes);
    gc->ndashes=n;
    gc->offset=offset;
    if(n) {
        gc->dashes=g_malloc_n(n, sizeof(double));
        for (i=0; i<n; i++) {
            gc->dashes[i]=dash_list[i];
        }
    } else {
        gc->dashes=NULL;
    }
}

static void gc_set_foreground(struct graphics_gc_priv *gc, struct color *c) {
    gc->c=*c;
}

static void gc_set_background(struct graphics_gc_priv *gc, struct color *c) {
}

static struct graphics_gc_methods gc_methods = {
    gc_destroy,
    gc_set_linewidth,
    gc_set_dashes,
    gc_set_foreground,
    gc_set_background,
};

static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth) {
    struct graphics_gc_priv *gc=g_new(struct graphics_gc_priv, 1);

    *meth=gc_methods;
    gc->gr=gr;

    gc->linewidth=1;
    gc->c.r=0;
    gc->c.g=0;
    gc->c.b=0;
    gc->c.a=0;
    gc->dashes=NULL;
    gc->ndashes=0;
    gc->offset=0;

    return gc;
}


static struct graphics_image_priv *image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name,
        int *w, int *h, struct point *hot, int rotation) {
    GdkPixbuf *pixbuf;
    struct graphics_image_priv *ret;
    const char *option;

    if (!strcmp(name,"buffer:")) {
        struct graphics_image_buffer *buffer=(struct graphics_image_buffer *)name;
        GdkPixbufLoader *loader=gdk_pixbuf_loader_new();
        if (!loader)
            return NULL;
        if (*w != IMAGE_W_H_UNSET || *h != IMAGE_W_H_UNSET)
            gdk_pixbuf_loader_set_size(loader, *w, *h);
        gdk_pixbuf_loader_write(loader, buffer->start, buffer->len, NULL);
        gdk_pixbuf_loader_close(loader, NULL);
        pixbuf=gdk_pixbuf_loader_get_pixbuf(loader);
        g_object_ref(pixbuf);
        g_object_unref(loader);
    } else {
        if (*w == IMAGE_W_H_UNSET && *h == IMAGE_W_H_UNSET)
            pixbuf=gdk_pixbuf_new_from_file(name, NULL);
        else
            pixbuf=gdk_pixbuf_new_from_file_at_size(name, *w, *h, NULL);
    }

    if (!pixbuf)
        return NULL;

    if (rotation) {
        GdkPixbuf *tmp;
        switch (rotation) {
        case 90:
            rotation=270;
            break;
        case 180:
            break;
        case 270:
            rotation=90;
            break;
        default:
            return NULL;
        }

        tmp=gdk_pixbuf_rotate_simple(pixbuf, rotation);

        if (!tmp) {
            g_object_unref(pixbuf);
            return NULL;
        }

        g_object_unref(pixbuf);
        pixbuf=tmp;
    }

    ret=g_new0(struct graphics_image_priv, 1);
    ret->pixbuf=pixbuf;
    ret->w=gdk_pixbuf_get_width(pixbuf);
    ret->h=gdk_pixbuf_get_height(pixbuf);
    *w=ret->w;
    *h=ret->h;
    if (hot) {
        option=gdk_pixbuf_get_option(pixbuf, "x_hot");
        if (option)
            hot->x=atoi(option);
        else
            hot->x=ret->w/2-1;
        option=gdk_pixbuf_get_option(pixbuf, "y_hot");
        if (option)
            hot->y=atoi(option);
        else
            hot->y=ret->h/2-1;
    }
    return ret;
}

static void image_free(struct graphics_priv *gr, struct graphics_image_priv *priv) {
    g_object_unref(priv->pixbuf);
    g_free(priv);
}

static void set_drawing_color(cairo_t *cairo, struct color c) {
    double col_max = 1<<COLOR_BITDEPTH;
    cairo_set_source_rgba(cairo, c.r/col_max, c.g/col_max, c.b/col_max, c.a/col_max);
}

static void set_stroke_params_from_gc(cairo_t *cairo, struct graphics_gc_priv *gc) {
    set_drawing_color(cairo, gc->c);
    cairo_set_dash(cairo, gc->dashes, gc->ndashes, gc->offset);
    cairo_set_line_width(cairo, gc->linewidth);
}

static void draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count) {
    int i;
    if (!count)
        return;
    cairo_move_to(gr->cairo, p[0].x, p[0].y);
    for (i=1; i<count; i++) {
        cairo_line_to(gr->cairo, p[i].x, p[i].y);
    }
    set_stroke_params_from_gc(gr->cairo, gc);
    cairo_stroke(gr->cairo);
}

static void draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count) {
    int i;
    set_drawing_color(gr->cairo, gc->c);
    cairo_move_to(gr->cairo, p[0].x, p[0].y);
    for (i=1; i<count; i++) {
        cairo_line_to(gr->cairo, p[i].x, p[i].y);
    }
    cairo_fill(gr->cairo);
}

static void draw_polygon_with_holes (struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count,
                                     int hole_count, int* ccount, struct point **holes) {
    int i;
    int j;
    cairo_fill_rule_t old_rule;
    set_drawing_color(gr->cairo, gc->c);
    /* remember current fill rule */
    old_rule = cairo_get_fill_rule (gr->cairo);
    /* set fill rule */
    cairo_set_fill_rule(gr->cairo, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_move_to(gr->cairo, p[0].x, p[0].y);
    for (i=1; i<count; i++) {
        cairo_line_to(gr->cairo, p[i].x, p[i].y);
    }
    for(j = 0; j < hole_count; j ++) {
        if(hole_count > 0) {
            cairo_move_to(gr->cairo, holes[j][0].x, holes[j][0].y);
            for(i=0; i < ccount[j]; i ++) {
                cairo_line_to(gr->cairo, holes[j][i].x, holes[j][i].y);
            }
        }
    }
    cairo_fill(gr->cairo);
    /* restore fill rule */
    cairo_set_fill_rule (gr->cairo,old_rule);
}

static void draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h) {
    cairo_save(gr->cairo);
    // Use OPERATOR_SOURCE to overwrite old contents even when drawing with transparency.
    // Necessary for OSD drawing.
    cairo_set_operator(gr->cairo, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(gr->cairo, p->x, p->y, w, h);
    set_drawing_color(gr->cairo, gc->c);
    cairo_fill(gr->cairo);
    cairo_restore(gr->cairo);
}

static void draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r) {
    cairo_arc (gr->cairo,  p->x, p->y, r/2, 0.0, 2*M_PI);
    set_stroke_params_from_gc(gr->cairo, gc);
    cairo_stroke(gr->cairo);
}

static void draw_rgb_image_buffer(cairo_t *cairo, int buffer_width, int buffer_height, int draw_pos_x, int draw_pos_y,
                                  int stride, unsigned char *buffer) {
    cairo_surface_t *buffer_surface = cairo_image_surface_create_for_data(
                                          buffer, CAIRO_FORMAT_ARGB32, buffer_width, buffer_height, stride);
    cairo_set_source_surface(cairo, buffer_surface, draw_pos_x, draw_pos_y);
    cairo_paint(cairo);
    cairo_surface_destroy(buffer_surface);
}

static void display_text_draw(struct font_freetype_text *text, struct graphics_priv *gr, struct graphics_gc_priv *fg,
                              struct graphics_gc_priv *bg, struct point *p) {
    int i,x,y,stride;
    struct font_freetype_glyph *g, **gp;
    struct color transparent= {0x0,0x0,0x0,0x0};

    gp=text->glyph;
    i=text->glyph_count;
    x=p->x << 6;
    y=p->y << 6;
    while (i-- > 0) {
        g=*gp++;
        if (g->w && g->h && bg ) {
            unsigned char *shadow;
            stride=cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, g->w+2);
            shadow=g_malloc(stride*(g->h+2));
            gr->freetype_methods.get_shadow(g, shadow, stride, &bg->c, &transparent);
            draw_rgb_image_buffer(gr->cairo, g->w+2, g->h+2, ((x+g->x)>>6)-1, ((y+g->y)>>6)-1, stride, shadow);
            g_free(shadow);
        }
        x+=g->dx;
        y+=g->dy;
    }
    x=p->x << 6;
    y=p->y << 6;
    gp=text->glyph;
    i=text->glyph_count;
    while (i-- > 0) {
        g=*gp++;
        if (g->w && g->h) {
            unsigned char *glyph;
            stride=cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, g->w);
            glyph=g_malloc(stride*g->h);
            gr->freetype_methods.get_glyph(g, glyph, stride, &fg->c, bg?&bg->c:&transparent, &transparent);
            draw_rgb_image_buffer(gr->cairo, g->w, g->h, (x+g->x)>>6, (y+g->y)>>6, stride, glyph);
            g_free(glyph);
        }
        x+=g->dx;
        y+=g->dy;
    }
}

static void draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg,
                      struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy) {
    struct font_freetype_text *t;

    if (! font) {
        dbg(lvl_error,"no font, returning");
        return;
    }
#if 0 /* Temporarily disabled because it destroys text rendering of overlays and in gui internal in some places */
    /*
     This needs an improvement, no one checks if the strings are visible
    */
    if (p->x > gr->width-50 || p->y > gr->height-50) {
        return;
    }
    if (p->x < -50 || p->y < -50) {
        return;
    }
#endif
    if (bg && !bg->c.a)
        bg=NULL;
    t=gr->freetype_methods.text_new(text, (struct font_freetype_font *)font, dx, dy);
    display_text_draw(t, gr, fg, bg, p);
    gr->freetype_methods.text_destroy(t);
}

static void draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p,
                       struct graphics_image_priv *img) {
    gdk_cairo_set_source_pixbuf(gr->cairo, img->pixbuf, p->x, p->y);
    cairo_paint(gr->cairo);
}

#ifdef HAVE_IMLIB2
static unsigned char* create_buffer_with_stride_if_required(unsigned char *input_buffer, int w, int h,
        size_t bytes_per_pixel, size_t output_stride) {
    int line;
    size_t input_offset, output_offset;
    unsigned char *out_buf;
    size_t input_stride = w*bytes_per_pixel;
    if (input_stride == output_stride) {
        return NULL;
    }

    out_buf = g_malloc(h*output_stride);
    for (line = 0; line < h; line++) {
        input_offset =  line*input_stride;
        output_offset = line*output_stride;
        memcpy(out_buf+output_offset, input_buffer+input_offset, input_stride);
    }
    return out_buf;
}

static void draw_image_warp(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, int count,
                            struct graphics_image_priv *img) {
    int w,h;
    DATA32 *intermediate_buffer;
    unsigned char* intermediate_buffer_aligned;
    Imlib_Image intermediate_image;
    size_t stride;
    dbg(lvl_debug,"draw_image_warp data=%p", img);
    w = img->w;
    h = img->h;
    if (!img->image) {
        int x,y;
        img->image=imlib_create_image(w, h);
        imlib_context_set_image(img->image);
        if (gdk_pixbuf_get_colorspace(img->pixbuf) != GDK_COLORSPACE_RGB || gdk_pixbuf_get_bits_per_sample(img->pixbuf) != 8) {
            dbg(lvl_error,"implement me");
        } else if (gdk_pixbuf_get_has_alpha(img->pixbuf) && gdk_pixbuf_get_n_channels(img->pixbuf) == 4) {
            for (y=0 ; y < h ; y++) {
                unsigned int *dst=imlib_image_get_data()+y*w;
                unsigned char *src=gdk_pixbuf_get_pixels(img->pixbuf)+y*gdk_pixbuf_get_rowstride(img->pixbuf);
                for (x=0 ; x < w ; x++) {
                    *dst++=0xff000000|(src[0] << 16)|(src[1] << 8)|src[2];
                    src+=4;
                }
            }
        } else if (!gdk_pixbuf_get_has_alpha(img->pixbuf) && gdk_pixbuf_get_n_channels(img->pixbuf) == 3) {
            for (y=0 ; y < h ; y++) {
                unsigned int *dst=imlib_image_get_data()+y*w;
                unsigned char *src=gdk_pixbuf_get_pixels(img->pixbuf)+y*gdk_pixbuf_get_rowstride(img->pixbuf);
                for (x=0 ; x < w ; x++) {
                    *dst++=0xff000000|(src[0] << 16)|(src[1] << 8)|src[2];
                    src+=3;
                }
            }
        } else {
            dbg(lvl_error,"implement me");
        }

    }

    intermediate_buffer = g_malloc0(gr->width*gr->height*4);
    intermediate_image = imlib_create_image_using_data(gr->width, gr->height, intermediate_buffer);
    imlib_context_set_image(intermediate_image);
    imlib_image_set_has_alpha(1);

    if (count == 3) {
        /* 0 1
        	   2   */
        imlib_blend_image_onto_image_skewed(img->image, 1, 0, 0, w, h, p[0].x, p[0].y, p[1].x-p[0].x, p[1].y-p[0].y,
                                            p[2].x-p[0].x, p[2].y-p[0].y);
    }
    if (count == 2) {
        /* 0
        	     1 */
        imlib_blend_image_onto_image_skewed(img->image, 1, 0, 0, w, h, p[0].x, p[0].y, p[1].x-p[0].x, 0, 0, p[1].y-p[0].y);
    }
    if (count == 1) {
        /*
                   0
        	     */
        imlib_blend_image_onto_image_skewed(img->image, 1, 0, 0, w, h, p[0].x-w/2, p[0].y-h/2, w, 0, 0, h);
    }

    stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, gr->width);
    intermediate_buffer_aligned = create_buffer_with_stride_if_required(
                                      (unsigned char* )intermediate_buffer, gr->width, gr->height, sizeof(DATA32), stride);
    cairo_surface_t *buffer_surface = cairo_image_surface_create_for_data(
                                          intermediate_buffer_aligned ? intermediate_buffer_aligned : (unsigned char*)intermediate_buffer,
                                          CAIRO_FORMAT_ARGB32, gr->width, gr->height, stride);
    cairo_set_source_surface(gr->cairo, buffer_surface, 0, 0);
    cairo_paint(gr->cairo);

    cairo_surface_destroy(buffer_surface);
    imlib_free_image();
    g_free(intermediate_buffer);
    g_free(intermediate_buffer_aligned);
}
#endif

static void overlay_rect(struct graphics_priv *parent, struct graphics_priv *overlay, GdkRectangle *r) {
    r->x=overlay->p.x;
    r->y=overlay->p.y;
    r->width=overlay->width;
    r->height=overlay->height;
    if (!overlay->wraparound)
        return;
    if (r->x < 0)
        r->x += parent->width;
    if (r->y < 0)
        r->y += parent->height;
    if (r->width < 0)
        r->width += parent->width;
    if (r->height < 0)
        r->height += parent->height;
}

static void overlay_draw(struct graphics_priv *parent, struct graphics_priv *overlay, GdkRectangle *re,
                         cairo_t *cairo) {
    GdkRectangle or, ir;
    if (parent->overlay_disabled || overlay->overlay_disabled || overlay->overlay_autodisabled)
        return;
    overlay_rect(parent, overlay, &or);
    if (! gdk_rectangle_intersect(re, &or, &ir))
        return;
    or.x-=re->x;
    or.y-=re->y;
    cairo_surface_t *overlay_surface = cairo_get_target(overlay->cairo);
    cairo_set_source_surface(cairo, overlay_surface, or.x, or.y);
    cairo_paint(cairo);
}

static void draw_drag(struct graphics_priv *gr, struct point *p) {
    if (p)
        gr->p=*p;
    else {
        gr->p.x=0;
        gr->p.y=0;
    }
}


static void background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc) {
    gr->background_gc=gc;
}

static void draw_mode(struct graphics_priv *gr, enum draw_mode_num mode) {
    if (mode == draw_mode_end) {
        // Just invalidate the whole window. We could only the invalidate the area of
        // graphics_priv, but that is probably not significantly faster.
        gdk_window_invalidate_rect(gr->widget->window, NULL, TRUE);
    }
}

/* Events */

static gint configure(GtkWidget * widget, GdkEventConfigure * event, gpointer user_data) {
    struct graphics_priv *gra=user_data;
    if (! gra->visible)
        return TRUE;
#ifndef _WIN32
    dbg(lvl_debug,"window=%lu", GDK_WINDOW_XID(widget->window));
#endif
    gra->width=widget->allocation.width;
    gra->height=widget->allocation.height;
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, gra->width, gra->height);
    if (gra->cairo)
        cairo_destroy(gra->cairo);
    gra->cairo = cairo_create(surface);
    cairo_surface_destroy(surface);
    cairo_set_antialias (gra->cairo, CAIRO_ANTIALIAS_GOOD);
    callback_list_call_attr_2(gra->cbl, attr_resize, GINT_TO_POINTER(gra->width), GINT_TO_POINTER(gra->height));
    return TRUE;
}

static gint expose(GtkWidget * widget, GdkEventExpose * event, gpointer user_data) {
    struct graphics_priv *gra=user_data;
    struct graphics_gc_priv *background_gc=gra->background_gc;
    struct graphics_priv *overlay;

    gra->visible=1;
    if (! gra->cairo)
        configure(widget, NULL, user_data);

    cairo_t *cairo=gdk_cairo_create(widget->window);
    if (gra->p.x || gra->p.y) {
        set_drawing_color(cairo, background_gc->c);
        cairo_paint(cairo);
    }
    cairo_set_source_surface(cairo, cairo_get_target(gra->cairo), gra->p.x, gra->p.y);
    cairo_paint(cairo);

    overlay = gra->overlays;
    while (overlay) {
        overlay_draw(gra,overlay,&event->area,cairo);
        overlay=overlay->next;
    }

    cairo_destroy(cairo);
    return FALSE;
}

static int tv_delta(struct timeval *old, struct timeval *new) {
    if (new->tv_sec-old->tv_sec >= INT_MAX/1000)
        return INT_MAX;
    return (new->tv_sec-old->tv_sec)*1000+(new->tv_usec-old->tv_usec)/1000;
}

static gint button_press(GtkWidget * widget, GdkEventButton * event, gpointer user_data) {
    struct graphics_priv *this=user_data;
    struct point p;
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);

    if (event->button < 8) {
        if (tv_delta(&this->button_press[event->button], &tv) < this->timeout)
            return FALSE;
        this->button_press[event->button]= tv;
        this->button_release[event->button].tv_sec=0;
        this->button_release[event->button].tv_usec=0;
    }
    p.x=event->x;
    p.y=event->y;
    callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(event->button), (void *)&p);
    return FALSE;
}

static gint button_release(GtkWidget * widget, GdkEventButton * event, gpointer user_data) {
    struct graphics_priv *this=user_data;
    struct point p;
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);

    if (event->button < 8) {
        if (tv_delta(&this->button_release[event->button], &tv) < this->timeout)
            return FALSE;
        this->button_release[event->button]= tv;
        this->button_press[event->button].tv_sec=0;
        this->button_press[event->button].tv_usec=0;
    }
    p.x=event->x;
    p.y=event->y;
    callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(event->button), (void *)&p);
    return FALSE;
}



static gint scroll(GtkWidget * widget, GdkEventScroll * event, gpointer user_data) {
    struct graphics_priv *this=user_data;
    struct point p;
    int button;

    p.x=event->x;
    p.y=event->y;
    switch (event->direction) {
    case GDK_SCROLL_UP:
        button=4;
        break;
    case GDK_SCROLL_DOWN:
        button=5;
        break;
    default:
        button=-1;
        break;
    }
    if (button != -1) {
        callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(button), (void *)&p);
        callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(button), (void *)&p);
    }
    return FALSE;
}

static gint motion_notify(GtkWidget * widget, GdkEventMotion * event, gpointer user_data) {
    struct graphics_priv *this=user_data;
    struct point p;

    p.x=event->x;
    p.y=event->y;
    callback_list_call_attr_1(this->cbl, attr_motion, (void *)&p);
    return FALSE;
}

/* *
 * * Exit navit (X pressed)
 * * @param widget active widget
 * * @param event the event (delete_event)
 * * @param user_data Pointer to private data structure
 * * @returns TRUE
 * */
static gint delete(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    struct graphics_priv *this=user_data;
    dbg(lvl_debug,"enter this->win=%p",this->win);
    if (this->delay & 2) {
        if (this->win)
            this->win=NULL;
    } else {
        callback_list_call_attr_0(this->cbl, attr_window_closed);
    }
    return TRUE;
}

static gint keypress(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    struct graphics_priv *this=user_data;
    int len,ucode;
    char key[8];
    ucode=gdk_keyval_to_unicode(event->keyval);
    len=g_unichar_to_utf8(ucode, key);
    key[len]='\0';

    switch (event->keyval) {
    case GDK_Up:
        key[0]=NAVIT_KEY_UP;
        key[1]='\0';
        break;
    case GDK_Down:
        key[0]=NAVIT_KEY_DOWN;
        key[1]='\0';
        break;
    case GDK_Left:
        key[0]=NAVIT_KEY_LEFT;
        key[1]='\0';
        break;
    case GDK_Right:
        key[0]=NAVIT_KEY_RIGHT;
        key[1]='\0';
        break;
    case GDK_BackSpace:
        key[0]=NAVIT_KEY_BACKSPACE;
        key[1]='\0';
        break;
    case GDK_Tab:
        key[0]='\t';
        key[1]='\0';
        break;
    case GDK_Delete:
        key[0]=NAVIT_KEY_DELETE;
        key[1]='\0';
        break;
    case GDK_Escape:
        key[0]=NAVIT_KEY_BACK;
        key[1]='\0';
        break;
    case GDK_Return:
    case GDK_KP_Enter:
        key[0]=NAVIT_KEY_RETURN;
        key[1]='\0';
        break;
    case GDK_Book:
#ifdef USE_HILDON
    case GDK_F7:
#endif
        key[0]=NAVIT_KEY_ZOOM_IN;
        key[1]='\0';
        break;
    case GDK_Calendar:
#ifdef USE_HILDON
    case GDK_F8:
#endif
        key[0]=NAVIT_KEY_ZOOM_OUT;
        key[1]='\0';
        break;
    case GDK_Page_Up:
        key[0]=NAVIT_KEY_PAGE_UP;
        key[1]='\0';
        break;
    case GDK_Page_Down:
        key[0]=NAVIT_KEY_PAGE_DOWN;
        key[1]='\0';
        break;
    }
    if (key[0])
        callback_list_call_attr_1(this->cbl, attr_keypress, (void *)key);
    else
        dbg(lvl_debug,"keyval 0x%x", event->keyval);

    return FALSE;
}

static struct graphics_priv *graphics_gtk_drawing_area_new_helper(struct graphics_methods *meth);

static void overlay_disable(struct graphics_priv *gr, int disabled) {
    if (!gr->overlay_disabled != !disabled) {
        gr->overlay_disabled=disabled;
        if (gr->parent) {
            GdkRectangle r;
            overlay_rect(gr->parent, gr, &r);
            gdk_window_invalidate_rect(gr->parent->widget->window, &r, TRUE);
        }
    }
}

static void overlay_resize(struct graphics_priv *this, struct point *p, int w, int h, int wraparound) {
    //do not dereference parent for non overlay osds
    if(!this->parent) {
        return;
    }

    int changed = 0;
    int w2,h2;

    if (w == 0) {
        w2 = 1;
    } else {
        w2 = w;
    }

    if (h == 0) {
        h2 = 1;
    } else {
        h2 = h;
    }

    this->p = *p;
    if (this->width != w2) {
        this->width = w2;
        changed = 1;
    }

    if (this->height != h2) {
        this->height = h2;
        changed = 1;
    }

    this->wraparound = wraparound;

    if (changed) {
        cairo_destroy(this->cairo);
        cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w2, h2);
        this->cairo=cairo_create(surface);
        cairo_surface_destroy(surface);

        if ((w == 0) || (h == 0)) {
            this->overlay_autodisabled = 1;
        } else {
            this->overlay_autodisabled = 0;
        }

        callback_list_call_attr_2(this->cbl, attr_resize, GINT_TO_POINTER(this->width), GINT_TO_POINTER(this->height));
    }
}

static void get_data_window(struct graphics_priv *this, unsigned int xid) {
    if (!xid)
        this->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    else
        this->win = gtk_plug_new(xid);
    if (!gtk_widget_get_parent(this->widget))
        gtk_widget_ref(this->widget);
    gtk_window_set_default_size(GTK_WINDOW(this->win), this->win_w, this->win_h);
    dbg(lvl_debug,"h= %i, w= %i",this->win_h, this->win_w);
    gtk_window_set_title(GTK_WINDOW(this->win), this->window_title);
    gtk_window_set_wmclass (GTK_WINDOW (this->win), "navit", this->window_title);
    gtk_widget_realize(this->win);
    if (gtk_widget_get_parent(this->widget))
        gtk_widget_reparent(this->widget, this->win);
    else
        gtk_container_add(GTK_CONTAINER(this->win), this->widget);
    gtk_widget_show_all(this->win);
    GTK_WIDGET_SET_FLAGS (this->widget, GTK_CAN_FOCUS);
    gtk_widget_set_sensitive(this->widget, TRUE);
    gtk_widget_grab_focus(this->widget);
    g_signal_connect(G_OBJECT(this->widget), "key-press-event", G_CALLBACK(keypress), this);
    g_signal_connect(G_OBJECT(this->win), "delete_event", G_CALLBACK(delete), this);
}

static int set_attr(struct graphics_priv *gr, struct attr *attr) {
    dbg(lvl_debug,"enter");
    switch (attr->type) {
    case attr_windowid:
        get_data_window(gr, attr->u.num);
        return 1;
    default:
        return 0;
    }
}

static struct graphics_priv *overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p,
        int w, int h, int wraparound) {
    int w2,h2;
    struct graphics_priv *this=graphics_gtk_drawing_area_new_helper(meth);
    this->widget=gr->widget;
    this->p=*p;
    this->width=w;
    this->height=h;
    this->parent=gr;

    /* If either height or width is 0, we set it to 1 to avoid warnings, and
     * disable the overlay. */
    if (h == 0) {
        h2 = 1;
    } else {
        h2 = h;
    }

    if (w == 0) {
        w2 = 1;
    } else {
        w2 = w;
    }

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w2, h2);
    this->cairo=cairo_create(surface);
    cairo_surface_destroy(surface);

    if ((w == 0) || (h == 0)) {
        this->overlay_autodisabled = 1;
    } else {
        this->overlay_autodisabled = 0;
    }

    this->next=gr->overlays;
    this->wraparound=wraparound;
    gr->overlays=this;
    return this;
}

static int gtk_argc;
static char **gtk_argv= {NULL};


static int graphics_gtk_drawing_area_fullscreen(struct window *w, int on) {
    struct graphics_priv *gr=w->priv;
    if (on)
        gtk_window_fullscreen(GTK_WINDOW(gr->win));
    else
        gtk_window_unfullscreen(GTK_WINDOW(gr->win));
    return 1;
}

static void graphics_gtk_drawing_area_disable_suspend(struct window *w) {
    struct graphics_priv *gr=w->priv;

#ifndef _WIN32
    if (gr->pid)
        kill(gr->pid, SIGWINCH);
#else
    dbg(lvl_warning, "failed to kill() under Windows");
#endif
}


static void *get_data(struct graphics_priv *this, char const *type) {
    FILE *f;
    if (!strcmp(type,"gtk_widget"))
        return this->widget;
#ifndef _WIN32
    if (!strcmp(type,"xwindow_id"))
        return (void *)GDK_WINDOW_XID(this->win ? this->win->window : this->widget->window);
#endif
    if (!strcmp(type,"window")) {
        char *cp = getenv("NAVIT_XID");
        unsigned xid = 0;
        if (cp)
            xid = strtol(cp, NULL, 0);
        if (!(this->delay & 1))
            get_data_window(this, xid);
        this->window.fullscreen=graphics_gtk_drawing_area_fullscreen;
        this->window.disable_suspend=graphics_gtk_drawing_area_disable_suspend;
        this->window.priv=this;
#if !defined(_WIN32) && !defined(__CEGCC__)
        f=popen("pidof /usr/bin/ipaq-sleep","r");
        if (f) {
            int fscanf_result;
            fscanf_result = fscanf(f,"%d",&this->pid);
            if ((fscanf_result == EOF) || (fscanf_result == 0)) {
                dbg(lvl_warning, "Failed to open iPaq sleep file. Error-Code: %d", errno);
            }
            dbg(lvl_debug,"ipaq_sleep pid=%d", this->pid);
            pclose(f);
        }
#endif
        return &this->window;
    }
    return NULL;
}

/**
 * @brief Return number of dots per inch
 * @param gr self handle
 * @return dpi value
 */
static navit_float get_dpi(struct graphics_priv * gr) {
    gdouble dpi = 96;
    GdkScreen *screen = gtk_widget_get_screen(gr->widget);
    if(screen != NULL) {
        dpi = gdk_screen_get_resolution (screen);
    }
    return (navit_float) dpi;
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
#ifdef HAVE_IMLIB2
    draw_image_warp,
#else
    NULL,
#endif
    draw_drag,
    NULL, /* font_new */
    gc_new,
    background_gc,
    overlay_new,
    image_new,
    get_data,
    image_free,
    NULL, /* get_text_bbox */
    overlay_disable,
    overlay_resize,
    set_attr,
    NULL, /* show_native_keyboard */
    NULL, /* hide_native_keyboard */
    get_dpi, /* get dpi */
    draw_polygon_with_holes
};

static struct graphics_priv *graphics_gtk_drawing_area_new_helper(struct graphics_methods *meth) {
    struct font_priv * (*font_freetype_new)(void *meth);
    font_freetype_new=plugin_get_category_font("freetype");
    if (!font_freetype_new)
        return NULL;
    struct graphics_priv *this=g_new0(struct graphics_priv,1);
    font_freetype_new(&this->freetype_methods);
    *meth=graphics_methods;
    meth->font_new=(struct graphics_font_priv *(*)(struct graphics_priv *, struct graphics_font_methods *, char *,  int,
                    int))this->freetype_methods.font_new;
    meth->get_text_bbox=(void(*)(struct graphics_priv*, struct graphics_font_priv *, char *, int, int, struct point *,
                                 int))this->freetype_methods.get_text_bbox;
    return this;
}

static struct graphics_priv *graphics_gtk_drawing_area_new(struct navit *nav, struct graphics_methods *meth,
        struct attr **attrs, struct callback_list *cbl) {
    int i;
    GtkWidget *draw;
    struct attr *attr;

    if (! event_request_system("glib","graphics_gtk_drawing_area_new"))
        return NULL;

    draw=gtk_drawing_area_new();
    struct graphics_priv *this=graphics_gtk_drawing_area_new_helper(meth);
    this->nav = nav;
    this->widget=draw;
    this->win_w=792;
    if ((attr=attr_search(attrs, NULL, attr_w)))
        this->win_w=attr->u.num;
    this->win_h=547;
    if ((attr=attr_search(attrs, NULL, attr_h)))
        this->win_h=attr->u.num;
    this->timeout=100;
    if ((attr=attr_search(attrs, NULL, attr_timeout)))
        this->timeout=attr->u.num;
    this->delay=0;
    if ((attr=attr_search(attrs, NULL, attr_delay)))
        this->delay=attr->u.num;
    if ((attr=attr_search(attrs, NULL, attr_window_title)))
        this->window_title=g_strdup(attr->u.str);
    else
        this->window_title=g_strdup("Navit");
    this->cbl=cbl;
    gtk_widget_set_events(draw, GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_POINTER_MOTION_MASK|GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(draw), "expose_event", G_CALLBACK(expose), this);
    g_signal_connect(G_OBJECT(draw), "configure_event", G_CALLBACK(configure), this);
    g_signal_connect(G_OBJECT(draw), "button_press_event", G_CALLBACK(button_press), this);
    g_signal_connect(G_OBJECT(draw), "button_release_event", G_CALLBACK(button_release), this);
    g_signal_connect(G_OBJECT(draw), "scroll_event", G_CALLBACK(scroll), this);
    g_signal_connect(G_OBJECT(draw), "motion_notify_event", G_CALLBACK(motion_notify), this);
    g_signal_connect(G_OBJECT(draw), "delete_event", G_CALLBACK(delete), nav);

    for (i = 0; i < 8; i++) {
        this->button_press[i].tv_sec = 0;
        this->button_press[i].tv_usec = 0;
        this->button_release[i].tv_sec = 0;
        this->button_release[i].tv_usec = 0;
    }

    return this;
}

void plugin_init(void) {
    gtk_init(&gtk_argc, &gtk_argv);
    gtk_set_locale();
#ifdef HAVE_API_WIN32
    setlocale(LC_NUMERIC, "C"); /* WIN32 gtk resets LC_NUMERIC */
#endif
    plugin_register_category_graphics("gtk_drawing_area", graphics_gtk_drawing_area_new);
}
