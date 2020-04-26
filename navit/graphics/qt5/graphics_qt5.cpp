/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2017 Navit Team
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
extern "C" {
#include "config.h"
#include "item.h" /* needs to be first, as attr.h depends on it */

#include "callback.h"
#include "color.h"
#include "debug.h"
#include "event.h"

#include "point.h" /* needs to be before graphics.h */

#include "graphics.h"
#include "plugin.h"
#include "window.h"
}

#include "event_qt5.h"
#include "graphics_qt5.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QFile>
#include <QFont>
#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QSvgRenderer>
#if USE_QML
#include "QNavitQuick.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#endif
#if USE_QWIDGET
#include "QNavitWidget.h"
#include <QApplication>
#endif
#if defined(WINDOWS) || defined(WIN32) || defined(HAVE_API_WIN32_CE)
#include <windows.h>
#endif

#if USE_QML
GraphicsPriv::GraphicsPriv(struct graphics_priv* gp) {
    this->gp = gp;
}

GraphicsPriv::~GraphicsPriv() {
}

void GraphicsPriv::emit_update() {
    emit update();
}
#endif

QGuiApplication* navit_app = NULL;

struct graphics_font_priv {
    QFont* font;
};

struct graphics_image_priv {
    QPixmap* pixmap;
};

static void graphics_destroy(struct graphics_priv* gr) {
//        dbg(lvl_debug,"enter");
#if HAVE_FREETYPE
    gr->freetype_methods.destroy();
#endif
    /* destroy painter */
    if (gr->painter != NULL)
        delete (gr->painter);
    /* destroy pixmap */
    if (gr->pixmap != NULL)
        delete (gr->pixmap);
    /* destroy widget if root window*/
    if (gr->root) {
#if USE_QWIDGET
        if (gr->widget != NULL)
            delete (gr->widget);
#endif
#if USE_QML
        if (gr->GPriv != NULL)
            delete (gr->GPriv);
#endif
    }
    /* unregister from parent, if any */
    if (gr->parent != NULL) {
        g_hash_table_remove(gr->parent->overlays, gr);
    }
#ifdef SAILFISH_OS
    if (gr->display_on_ev != NULL) {
        event_remove_timeout(gr->display_on_ev);
    }
    if (gr->display_on_cb != NULL) {
        g_free(gr->display_on_cb);
    }
#endif
    /* destroy overlays hash */
    g_hash_table_destroy(gr->overlays);
    /* destroy global application if destroying the last */
    if (gr->root) {
        if (navit_app != NULL) {
            delete (navit_app);
        }
        navit_app = NULL;
        /* destroy argv if any */
        while (gr->argc > 0) {
            gr->argc--;
            if (gr->argv[gr->argc] != NULL)
                g_free(gr->argv[gr->argc]);
        }
    }
    /* destroy self */
    g_free(gr);
}

static void font_destroy(struct graphics_font_priv* font) {
    //        dbg(lvl_debug,"enter");
    if (font->font != NULL)
        delete (font->font);
    g_free(font);
}

/**
 * @brief	font interface structure
 * This structure is preset with all function pointers provided by this implemention
 * to be returned as interface.
 */
static struct graphics_font_methods font_methods = {
    font_destroy
};

/**
 * List of font families to use, in order of preference
 */
static const char* fontfamilies[] = {
    "Liberation Sans",
    "Arial",
    "NcrBI4nh",
    "luximbi",
    "FreeSans",
    "DejaVu Sans",
    NULL,
};

/**
 * @brief	Allocate a font context
 * @param	gr	own private context
 * @param	meth	fill this structure with correct functions to be called with handle as interface to font
 * @param	font	font family e.g. "Arial"
 * @param	size	Font size in 16.6 fractional points @ 300dpi. This is bullsh***. The encoding is freetypes
 *          16.6 fixed point format usually giving points. One point is usually 72th part of an inch. But
 *          navit does not honor dpi correct. It's traditionally used freetype backend is fixed to 300 dpi.
 *          So this value is (300/72) pixels
 * @param	flags	Font flags (currently 1 if bold and 0 if not)
 *
 * @return	font handle
 *
 * Allocates a font handle and returnes filled interface stucture
 */
static struct graphics_font_priv* font_new(struct graphics_priv* gr, struct graphics_font_methods* meth, char* font,
        int size, int flags) {
    int a = 0;
    struct graphics_font_priv* font_priv;
    dbg(lvl_debug, "enter (font %s, %d, 0x%x)", font, size, flags);
    font_priv = g_new0(struct graphics_font_priv, 1);
    font_priv->font = new QFont(fontfamilies[0]);
    if (font != NULL)
        font_priv->font->setFamily(font);
    /* search for exact font match */
    while ((!font_priv->font->exactMatch()) && (fontfamilies[a] != NULL)) {
        font_priv->font->setFamily(fontfamilies[a]);
        a++;
    }
    if (font_priv->font->exactMatch()) {
        dbg(lvl_debug, "Exactly matching font: %s", font_priv->font->family().toUtf8().data());
    } else {
        /* set any font*/
        if (font != NULL) {
            font_priv->font->setFamily(font);
        } else {
            font_priv->font->setFamily(fontfamilies[0]);
        }
        dbg(lvl_debug, "No matching font. Resort to: %s", font_priv->font->family().toUtf8().data());
    }

    /* Convert silly font size to pixels. by 64 is to convert fixpoint to int. */
    dbg(lvl_debug, "(font %s, %d=%f, %d)", font, size,((float)size)/64.0, ((size * 300) / 72) / 64);
    font_priv->font->setPixelSize(((size * 300) / 72) / 64);
    //font_priv->font->setStyleStrategy(QFont::NoSubpixelAntialias);
    /* Check for bold font */
    if (flags) {
        font_priv->font->setBold(true);
    }

    *meth = font_methods;
    return font_priv;
}

static void gc_destroy(struct graphics_gc_priv* gc) {
    //        dbg(lvl_debug,"enter gc=%p", gc);
    delete (gc->pen);
    delete (gc->brush);
    g_free(gc);
}

static void gc_set_linewidth(struct graphics_gc_priv* gc, int w) {
    //        dbg(lvl_debug,"enter gc=%p, %d", gc, w);
    gc->pen->setWidth(w);
}

static void gc_set_dashes(struct graphics_gc_priv* gc, int w, int offset, unsigned char* dash_list, int n) {
    if (n <= 0) {
        dbg(lvl_error, "Refuse to set dashes without dash pattern");
    }
    /* use Qt dash feature */
    QVector<qreal> dashes;
    gc->pen->setWidth(w);
    gc->pen->setDashOffset(offset);
    for (int a = 0; a < n; a++) {
        dashes << dash_list[a];
    }
    /* Qt requires the pattern to have even element count. Add the last
         * element twice if n doesn't divide by two
         */
    if ((n % 2) != 0) {
        dashes << dash_list[n - 1];
    }
    gc->pen->setDashPattern(dashes);
}

static void gc_set_foreground(struct graphics_gc_priv* gc, struct color* c) {
    QColor col(c->r >> 8, c->g >> 8, c->b >> 8, c->a >> 8);
    //        dbg(lvl_debug,"context %p: color %02x%02x%02x",gc, c->r >> 8, c->g >> 8, c->b >> 8);
    gc->pen->setColor(col);
    gc->brush->setColor(col);
    //gc->c=*c;
}

static void gc_set_background(struct graphics_gc_priv* gc, struct color* c) {
    QColor col(c->r >> 8, c->g >> 8, c->b >> 8, c->a >> 8);
    //        dbg(lvl_debug,"context %p: color %02x%02x%02x",gc, c->r >> 8, c->g >> 8, c->b >> 8);
    //gc->pen->setColor(col);
    //gc->brush->setColor(col);
}

void gc_set_texture (struct graphics_gc_priv *gc, struct graphics_image_priv *img) {
    if(img == NULL) {
        //disable texture mode
        gc->brush->setStyle(Qt::SolidPattern);
    } else {
        //set and enable texture
        //Use a new pixmap
        QPixmap background(img->pixmap->size());
        //Use fill color
        background.fill(gc->brush->color());
        //Get a painter
        QPainter painter(&background);
        //Blit the (transparent) image on pixmap.
        painter.drawPixmap(0, 0, *(img->pixmap));
        //Set the texture to the brush.
        gc->brush->setTexture(background);
    }
}

static struct graphics_gc_methods gc_methods = {
    gc_destroy,
    gc_set_linewidth,
    gc_set_dashes,
    gc_set_foreground,
    gc_set_background,
    gc_set_texture
};

static struct graphics_gc_priv* gc_new(struct graphics_priv* gr, struct graphics_gc_methods* meth) {
    struct graphics_gc_priv* graphics_gc_priv = NULL;
    //        dbg(lvl_debug,"enter gr==%p", gr);
    graphics_gc_priv = g_new0(struct graphics_gc_priv, 1);
    graphics_gc_priv->graphics_priv = gr;
    graphics_gc_priv->pen = new QPen();
    graphics_gc_priv->brush = new QBrush(Qt::SolidPattern);

    *meth = gc_methods;
    return graphics_gc_priv;
}

static void image_destroy(struct graphics_image_priv* img) {
    //    dbg(lvl_debug, "enter");
    if (img->pixmap != NULL)
        delete (img->pixmap);
    g_free(img);
}

struct graphics_image_methods image_methods = {
    image_destroy
};

static struct graphics_image_priv* image_new(struct graphics_priv* gr, struct graphics_image_methods* meth, char* path,
        int* w, int* h, struct point* hot, int rotation) {
    struct graphics_image_priv* image_priv;
    //        dbg(lvl_debug,"enter %s, %d %d", path, *w, *h);
    if (path[0] == 0) {
        dbg(lvl_debug, "Refuse to load image without path");
        return NULL;
    }
    QString key(path);
    QString renderer_key(key);
    int index = key.lastIndexOf(".");
    QString extension;
    if(index > 0) {
        extension = key.right(index);
    }
    QFile imagefile(key);
    if (!imagefile.exists()) {
        /* file doesn't exit. Either navit wants us to guess file name by
             * ommitting exstension, or the file does really not exist.
             */
        if (extension != "") {
            /*file doesn't exist. give up */
            dbg(lvl_debug, "File %s does not exist", path);
            return NULL;
        } else {
            /* add ".svg" for renderer to try .svg file first in renderer */
            dbg(lvl_debug, "Guess extension on %s", path);
            renderer_key += ".svg";
        }
    }
    image_priv = g_new0(struct graphics_image_priv, 1);
    *meth = image_methods;

    /* check if this can be rendered */
    if (renderer_key.endsWith("svg")) {
        QSvgRenderer renderer(renderer_key);
        if (renderer.isValid()) {
            dbg(lvl_debug, "render %s", path);
            /* try to render this */
            /* assume "standard" size if size is not given */
            if (*w <= 0)
                *w = renderer.defaultSize().width();
            if (*h <= 0)
                *h = renderer.defaultSize().height();
            image_priv->pixmap = new QPixmap(*w, *h);
            image_priv->pixmap->fill(Qt::transparent);
            QPainter painter(image_priv->pixmap);
            renderer.render(&painter);
        }
    }

    if (image_priv->pixmap == NULL) {
        /*cannot be rendered. try to load it */
        dbg(lvl_debug, "cannot render %s", path);
        image_priv->pixmap = new QPixmap(key);
    }

    /* check if we got image */
    if ((image_priv->pixmap == NULL) || (image_priv->pixmap->isNull())) {
        g_free(image_priv);
        return NULL;
    } else {
        /* check if we need to scale this */
        if ((*w > 0) && (*h > 0)) {
            if ((image_priv->pixmap->width() != *w) || (image_priv->pixmap->height() != *h)) {
                dbg(lvl_debug, "scale pixmap %s, %d->%d,%d->%d", path, image_priv->pixmap->width(), *w, image_priv->pixmap->height(),
                    *h);
                QPixmap* scaled = new QPixmap(image_priv->pixmap->scaled(*w, *h, Qt::IgnoreAspectRatio, Qt::FastTransformation));
                delete (image_priv->pixmap);
                image_priv->pixmap = scaled;
            }
        }
    }

    *w = image_priv->pixmap->width();
    *h = image_priv->pixmap->height();
    //        dbg(lvl_debug, "Got (%d,%d)", *w,*h);
    if (hot) {
        hot->x = *w / 2;
        hot->y = *h / 2;
    }

    return image_priv;
}

static void draw_lines(struct graphics_priv* gr, struct graphics_gc_priv* gc, struct point* p, int count) {
    int i;
    QPolygon polygon;
    //        dbg(lvl_debug,"enter gr=%p, gc=%p, (%d, %d)", gr, gc, p->x, p->y);
    if (gr->painter == NULL)
        return;

    for (i = 0; i < count; i++)
        polygon.putPoints(i, 1, p[i].x, p[i].y);
    gr->painter->setPen(*gc->pen);
    gr->painter->drawPolyline(polygon);
}

static void draw_polygon(struct graphics_priv* gr, struct graphics_gc_priv* gc, struct point* p, int count) {
    int i;
    QPolygon polygon;
    //        dbg(lvl_debug,"enter gr=%p, gc=%p, (%d, %d)", gr, gc, p->x, p->y);
    if (gr->painter == NULL)
        return;

    for (i = 0; i < count; i++)
        polygon.putPoints(i, 1, p[i].x, p[i].y);
    gr->painter->setPen(*gc->pen);
    gr->painter->setBrush(*gc->brush);

    gr->painter->drawPolygon(polygon);
}

static void draw_polygon_with_holes (struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count,
                                     int hole_count, int* ccount, struct point **holes) {
    int i;
    int j;
    QPainterPath path;
    QPainterPath inner;
    QPolygon polygon;
    //dbg(lvl_error,"enter gr=%p, gc=%p, (%d, %d) holes %d", gr, gc, p->x, p->y, hole_count);
    if (gr->painter == NULL)
        return;
    gr->painter->setPen(*gc->pen);
    gr->painter->setBrush(*gc->brush);
    /* construct outer polygon */
    for (i = 0; i < count; i++)
        polygon.putPoints(i, 1, p[i].x, p[i].y);
    /* add it to outer path */
    path.addPolygon(polygon);
    /* construct the polygons for the holes and add them to inner */
    for(j=0; j<hole_count; j ++) {
        QPolygon hole;
        for (i = 0; i < ccount[j]; i++)
            hole.putPoints(i, 1, holes[j][i].x, holes[j][i].y);
        inner.addPolygon(hole);
    }
    /* intersect */
    if(hole_count > 0)
        path = path.subtracted(inner);

    gr->painter->drawPath(path);
}

static void draw_rectangle(struct graphics_priv* gr, struct graphics_gc_priv* gc, struct point* p, int w, int h) {
    //	dbg(lvl_debug,"gr=%p gc=%p %d,%d,%d,%d", gr, gc, p->x, p->y, w, h);
    if (gr->painter == NULL)
        return;
    gr->painter->fillRect(p->x, p->y, w, h, *gc->brush);
}

static void draw_circle(struct graphics_priv* gr, struct graphics_gc_priv* gc, struct point* p, int r) {
    //        dbg(lvl_debug,"enter gr=%p, gc=%p, (%d,%d) r=%d", gr, gc, p->x, p->y, r);
    if (gr->painter == NULL)
        return;
    gr->painter->setPen(*gc->pen);
    gr->painter->drawArc(p->x - r / 2, p->y - r / 2, r, r, 0, 360 * 16);
}

/**
 * @brief	Render given text
 * @param	gr	own private context
 * @param	fg	foreground drawing context (for color)
 * @param	bg	background drawing context (for color)
 * @param	font	font context to use (allocated by font_new)
 * @param	text	String to calculate bbox for
 * @param	p	offset on gr context to place this text.
 * @param	dx	transformation matrix (16.16 fixpoint)
 * @param	dy	transformation matrix (16.16 fixpoint)
 *
 * Renders given text on gr surface. Draws nice contrast outline around text.
 */
static void draw_text(struct graphics_priv* gr, struct graphics_gc_priv* fg, struct graphics_gc_priv* bg,
                      struct graphics_font_priv* font, char* text, struct point* p, int dx, int dy) {
    dbg(lvl_debug, "enter gc=%p, fg=%p, bg=%p pos(%d,%d) d(%d, %d) %s", gr, fg, bg, p->x, p->y, dx, dy, text);
    QPainter* painter = gr->painter;
    if (painter == NULL)
        return;
#if HAVE_FREETYPE
    struct font_freetype_text* t;
    struct font_freetype_glyph *g, **gp;
    struct color transparent = { 0x0000, 0x0000, 0x0000, 0x0000 };
    struct color fgc;
    struct color bgc;
    QColor temp;

    int i, x, y;

    if (!font)
        return;
    /* extract colors */
    fgc.r = fg->pen->color().red() << 8;
    fgc.g = fg->pen->color().green() << 8;
    fgc.b = fg->pen->color().blue() << 8;
    fgc.a = fg->pen->color().alpha() << 8;
    if (bg != NULL) {
        bgc.r = bg->pen->color().red() << 8;
        bgc.g = bg->pen->color().green() << 8;
        bgc.b = bg->pen->color().blue() << 8;
        bgc.a = bg->pen->color().alpha() << 8;
    } else {
        bgc = transparent;
    }

    t = gr->freetype_methods.text_new(text, (struct font_freetype_font*)font, dx, dy);
    x = p->x << 6;
    y = p->y << 6;
    gp = t->glyph;
    i = t->glyph_count;
    if (bg) {
        while (i-- > 0) {
            g = *gp++;
            if (g->w && g->h) {
                unsigned char* data;
                QImage img(g->w + 2, g->h + 2, QImage::Format_ARGB32_Premultiplied);
                data = img.bits();
                gr->freetype_methods.get_shadow(g, (unsigned char*)data, img.bytesPerLine(), &bgc, &transparent);

                painter->drawImage(((x + g->x) >> 6) - 1, ((y + g->y) >> 6) - 1, img);
            }
            x += g->dx;
            y += g->dy;
        }
    }
    x = p->x << 6;
    y = p->y << 6;
    gp = t->glyph;
    i = t->glyph_count;
    while (i-- > 0) {
        g = *gp++;
        if (g->w && g->h) {
            unsigned char* data;
            QImage img(g->w, g->h, QImage::Format_ARGB32_Premultiplied);
            data = img.bits();
            gr->freetype_methods.get_glyph(g, (unsigned char*)data, img.bytesPerLine(), &fgc, &bgc, &transparent);
            painter->drawImage((x + g->x) >> 6, (y + g->y) >> 6, img);
        }
        x += g->dx;
        y += g->dy;
    }
    gr->freetype_methods.text_destroy(t);
#else
    QString tmp = QString::fromUtf8(text);
    qreal m_dx = ((qreal)dx) / 65536.0;
    qreal m_dy = ((qreal)dy) / 65536.0;
    QMatrix sav = painter->worldMatrix();
    QMatrix m(m_dx, m_dy, -m_dy, m_dx, p->x, p->y);
    painter->setWorldMatrix(m, TRUE);
    painter->setFont(*font->font);
    if (bg) {
        QPen shadow;
        QPainterPath path;
        shadow.setColor(bg->pen->color());
        shadow.setWidth(3);
        painter->setPen(shadow);
        path.addText(0, 0, *font->font, tmp);
        painter->drawPath(path);
    }
    painter->setPen(*fg->pen);
    painter->drawText(0, 0, tmp);
    painter->setWorldMatrix(sav);
#endif
}

static void draw_image(struct graphics_priv* gr, struct graphics_gc_priv* fg, struct point* p,
                       struct graphics_image_priv* img) {
    //        dbg(lvl_debug,"enter");
    if (gr->painter != NULL)
        gr->painter->drawPixmap(p->x, p->y, *img->pixmap);
    else
        dbg(lvl_debug, "Try to draw image, but no painter");
}

/**
 * @brief	Drag layer.
 * @param   gr private handle
 * @param	p	vector the bitmap is moved from base, or NULL to indicate 0:0 vector
 *
 * Move layer to new position. If drag_bitmap is enabled this may also be
 * called for root layer. There the content of the root layer is to be moved
 * by given vector. On root layer, NULL indicates the end of a drag.
 */
static void draw_drag(struct graphics_priv* gr, struct point* p) {
    struct point vector;

    if (p != NULL) {
        dbg(lvl_debug, "enter %p (%d,%d)", gr, p->x, p->y);
        vector = *p;
    } else {
        dbg(lvl_debug, "enter %p (NULL)", gr);
        vector.x = 0;
        vector.y = 0;
    }
    if (gr->root) {
        gr->scroll_x = vector.x;
        gr->scroll_y = vector.y;
    } else {
#if USE_QWIDGET
        int damage_x = gr->x;
        int damage_y = gr->y;
        int damage_w = gr->pixmap->width();
        int damage_h = gr->pixmap->height();
#endif
        gr->x = vector.x;
        gr->y = vector.y;
#if USE_QWIDGET
        /* call repaint on widget for stale area. */
        if (gr->widget != NULL)
            gr->widget->repaint(damage_x, damage_y, damage_w, damage_h);
#endif
#if USE_QML
// No need to emit update, as QNavitQuic always repaints everything.
//    if (gr->GPriv != NULL)
//        gr->GPriv->emit_update();
#endif
    }
}

static void background_gc(struct graphics_priv* gr, struct graphics_gc_priv* gc) {
    //        dbg(lvl_debug,"register context %p on %p", gc, gr);
    gr->background_graphics_gc_priv = gc;
}

static void draw_mode(struct graphics_priv* gr, enum draw_mode_num mode) {
    switch (mode) {
    case draw_mode_begin:
        dbg(lvl_debug, "Begin drawing on context %p (use == %d)", gr, gr->use_count);
        gr->use_count++;
        if (gr->painter == NULL) {
            if(gr->parent != NULL)
                gr->pixmap->fill(QColor(0,0,0,0));
            gr->painter = new QPainter(gr->pixmap);
        } else
            dbg(lvl_debug, "drawing on %p already active", gr);
        break;
    case draw_mode_end:
        dbg(lvl_debug, "End drawing on context %p (use == %d)", gr, gr->use_count);
        gr->use_count--;
        if (gr->use_count < 0)
            gr->use_count = 0;
        if (gr->use_count > 0) {
            dbg(lvl_debug, "drawing on %p still in use", gr);
        } else if (gr->painter != NULL) {
            gr->painter->end();
            delete (gr->painter);
            gr->painter = NULL;
        } else {
            dbg(lvl_debug, "Context %p not active!", gr)
        }
#if USE_QWIDGET
        /* call repaint on widget */
        if (gr->widget != NULL)
            gr->widget->repaint(gr->x, gr->y, gr->pixmap->width(), gr->pixmap->height());
#endif
#if USE_QML
        if (gr->GPriv != NULL)
            gr->GPriv->emit_update();

#endif

        break;
    default:
        dbg(lvl_debug, "Unknown drawing %d on context %p", mode, gr);
        break;
    }
}

static struct graphics_priv* overlay_new(struct graphics_priv* gr, struct graphics_methods* meth, struct point* p,
        int w, int h, int wraparound);

void resize_callback(struct graphics_priv* gr, int w, int h) {
    //        dbg(lvl_debug,"enter (%d, %d)", w, h);
    callback_list_call_attr_2(gr->callbacks, attr_resize, GINT_TO_POINTER(w), GINT_TO_POINTER(h));
}

static int graphics_qt5_fullscreen(struct window* w, int on) {
    struct graphics_priv* gr;
    //        dbg(lvl_debug,"enter");
    gr = (struct graphics_priv*)w->priv;
#if USE_QML
    if (gr->window != NULL) {
        if (on)
            gr->window->setWindowState(Qt::WindowFullScreen);
        else
            gr->window->setWindowState(Qt::WindowMaximized);
    }
#endif
#if USE_QWIDGET
    if (gr->widget != NULL) {
        if (on)
            gr->widget->setWindowState(Qt::WindowFullScreen);
        else
            gr->widget->setWindowState(Qt::WindowMaximized);
    }
#endif
    return 1;
}

#ifdef SAILFISH_OS
static void keep_display_on(struct graphics_priv* priv) {
    //        dbg(lvl_debug,"enter");
    QDBusConnection system = QDBusConnection::connectToBus(QDBusConnection::SystemBus, "system");
    QDBusInterface interface("com.nokia.mce", "/com/nokia/mce/request", "com.nokia.mce.request", system);

    interface.call(QLatin1String("req_display_blanking_pause"));
}
#endif

static void graphics_qt5_disable_suspend(struct window* w) {
//        dbg(lvl_debug,"enter");
#ifdef SAILFISH_OS
    struct graphics_priv* gr;
    gr = (struct graphics_priv*)w->priv;
    keep_display_on(gr);
    /* to keep display on, d-bus trigger must be called at least once per second.
         * to cope with fuzz, trigger it once per 30 seconds */
    gr->display_on_cb = callback_new_1(callback_cast(keep_display_on), gr);
    gr->display_on_ev = event_add_timeout(30000, 1, gr->display_on_cb);
#endif
}

static void* get_data(struct graphics_priv* this_priv, char const* type) {
    //        dbg(lvl_debug,"enter: %s", type);
    if (strcmp(type, "window") == 0) {
        struct window* win;
        //                dbg(lvl_debug,"window detected");
        win = g_new0(struct window, 1);
        win->priv = this_priv;
        win->fullscreen = graphics_qt5_fullscreen;
        win->disable_suspend = graphics_qt5_disable_suspend;
        resize_callback(this_priv, this_priv->pixmap->width(), this_priv->pixmap->height());
        return win;
    }
    if (strcmp(type, "engine") == 0) {
        dbg(lvl_debug, "Hand over QQmlApplicationEngine");
        return (this_priv->engine);
    }
    return NULL;
}

static void image_free(struct graphics_priv* gr, struct graphics_image_priv* priv) {
    //        dbg(lvl_debug,"enter");
    delete (priv->pixmap);
    g_free(priv);
}

/**
 * @brief	Calculate pixel space required for font display.
 * @param	gr	own private context
 * @param	font	font context to use (allocated by font_new)
 * @param	text	String to calculate bbox for
 * @param	dx	transformation matrix (16.16 fixpoint)
 * @param	dy	transformation matrix (16.16 fixpoint)
 * @param	ret	point array to fill. (low left, top left, top right, low right)
 * @param	estimate	???
 *
 * Calculates the bounding box around the given text.
 */
static void get_text_bbox(struct graphics_priv* gr, struct graphics_font_priv* font, char* text, int dx, int dy,
                          struct point* ret, int estimate) {
    int i;
    struct point pt;
    QString tmp = QString::fromUtf8(text);
    QRect r;
    //        dbg(lvl_debug,"enter %s %d %d", text, dx, dy);

    /* use QFontMetrix for bbox calculation as we do not always have a painter */
    QFontMetrics fm(*font->font);
    r = fm.boundingRect(tmp);

    /* low left */
    ret[0].x = r.left();
    ret[0].y = r.bottom();
    /* top left */
    ret[1].x = r.left();
    ret[1].y = r.top();
    /* top right */
    ret[2].x = r.right();
    ret[2].y = r.top();
    /* low right */
    ret[3].x = r.right();
    ret[3].y = r.bottom();
    /* transform bbox if rotated */
    if (dy != 0 || dx != 0x10000) {
        for (i = 0; i < 4; i++) {
            pt = ret[i];
            ret[i].x = (pt.x * dx - pt.y * dy) / 0x10000;
            ret[i].y = (pt.y * dx + pt.x * dy) / 0x10000;
        }
    }
}

static void overlay_disable(struct graphics_priv* gr, int disable) {
    //dbg(lvl_error,"enter gr=%p, %d", gr, disable);
    gr->disable = disable;
#if USE_QWIDGET
    /* call repaint on widget */
    if (gr->widget != NULL)
        gr->widget->repaint(gr->x, gr->y, gr->pixmap->width(), gr->pixmap->height());
#endif
#if USE_QML
    if (gr->GPriv != NULL)
        gr->GPriv->emit_update();

#endif
}

static void overlay_resize(struct graphics_priv* gr, struct point* p, int w, int h, int wraparound) {
    //        dbg(lvl_debug,"enter %d %d %d %d %d", p->x, p->y, w, h, wraparound);
    gr->x = p->x;
    gr->y = p->y;
    if (gr->painter != NULL) {
        delete (gr->painter);
    }
    /* replacing the pixmap clears the content. Only neccesary if size actually changes */
    if((gr->pixmap->height() != h) || (gr->pixmap->width() != w)) {
        delete (gr->pixmap);
        gr->pixmap = new QPixmap(w, h);
        gr->pixmap->fill(Qt::transparent);
    }
    if (gr->painter != NULL)
        gr->painter = new QPainter(gr->pixmap);
#if USE_QWIDGET
    /* call repaint on widget */
    if (gr->widget != NULL)
        gr->widget->repaint(gr->x, gr->y, gr->pixmap->width(), gr->pixmap->height());
#endif
#if USE_QML
    if (gr->GPriv != NULL)
        gr->GPriv->emit_update();

#endif
}

/**
 * @brief Return number of dots per inch
 * @param gr self handle
 * @return dpi value
 */
static navit_float get_dpi(struct graphics_priv * gr) {
    qreal dpi = 96;
    QScreen* primary = navit_app->primaryScreen();
    if (primary != NULL) {
        dpi = primary->physicalDotsPerInch();
    }
    return (navit_float)dpi;
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
    NULL, //set_attr
    NULL, //show_native_keyboard
    NULL, //hide_native_keyboard
    get_dpi,
    draw_polygon_with_holes
};

/* create new graphics context on given context */
static struct graphics_priv* overlay_new(struct graphics_priv* gr, struct graphics_methods* meth, struct point* p,
        int w, int h, int wraparound) {
    struct graphics_priv* graphics_priv = NULL;
    graphics_priv = g_new0(struct graphics_priv, 1);
    *meth = graphics_methods;
#if HAVE_FREETYPE
    if (gr->font_freetype_new) {
        graphics_priv->font_freetype_new = gr->font_freetype_new;
        gr->font_freetype_new(&graphics_priv->freetype_methods);
        meth->font_new = (struct graphics_font_priv * (*)(struct graphics_priv*, struct graphics_font_methods*, char*, int,
                          int))graphics_priv->freetype_methods.font_new;
        meth->get_text_bbox = (void (*)(struct graphics_priv*, struct graphics_font_priv*, char*, int, int, struct point*,
                                        int))graphics_priv->freetype_methods.get_text_bbox;
    }
#endif
#if USE_QML
    graphics_priv->window = gr->window;
    graphics_priv->GPriv = gr->GPriv;
#endif
#if USE_QWIDGET
    graphics_priv->widget = gr->widget;
#endif
    graphics_priv->x = p->x;
    graphics_priv->y = p->y;
    graphics_priv->disable = false;
    graphics_priv->callbacks = gr->callbacks;
    graphics_priv->pixmap = new QPixmap(w, h);
    graphics_priv->pixmap->fill(Qt::transparent);
    graphics_priv->painter = NULL;
    graphics_priv->use_count = 0;
    graphics_priv->parent = gr;
    graphics_priv->overlays = g_hash_table_new(NULL, NULL);
    graphics_priv->scroll_x = 0;
    graphics_priv->scroll_y = 0;
    graphics_priv->root = false;
    graphics_priv->argc = 0;
    graphics_priv->argv[0] = NULL;
    /* register on parent */
    g_hash_table_insert(gr->overlays, graphics_priv, graphics_priv);
    //        dbg(lvl_debug,"New overlay: %p", graphics_priv);

    return graphics_priv;
}

/* create application and initial graphics context */
static struct graphics_priv* graphics_qt5_new(struct navit* nav, struct graphics_methods* meth, struct attr** attrs,
        struct callback_list* cbl) {
    struct graphics_priv* graphics_priv = NULL;
    struct attr* event_loop_system = NULL;
    struct attr* platform = NULL;
    struct attr* fullscreen = NULL;
    struct attr* attr_widget = NULL;
    bool use_qml = USE_QML;
    bool use_qwidget = USE_QWIDGET;

    //dbg(lvl_debug,"enter");

    /* get qt widget attr */
    if ((attr_widget = attr_search(attrs, NULL, attr_qt5_widget))) {
        /* check if we shall use qml */
        if (strcmp(attr_widget->u.str, "qwidget") == 0) {
            use_qml = false;
        }
        /* check if we shall use qwidget */
        if (strcmp(attr_widget->u.str, "qml") == 0) {
            use_qwidget = false;
        }
    }
    if (use_qml && use_qwidget) {
        /* both are possible, default to QML */
        use_qwidget = false;
    }

    /*register graphic methods by copying in our predefined ones */
    *meth = graphics_methods;

    /* get event loop from config and request event loop*/
    event_loop_system = attr_search(attrs, NULL, attr_event_loop_system);
    if (event_loop_system && event_loop_system->u.str) {
        //dbg(lvl_debug, "event_system is %s", event_loop_system->u.str);
        if (!event_request_system(event_loop_system->u.str, "graphics_qt5"))
            return NULL;
    } else {
        /* no event system requested by config. Default to our own */
        if (!event_request_system("qt5", "graphics_qt5"))
            return NULL;
    }

#if HAVE_FREETYPE
    struct font_priv* (*font_freetype_new)(void* meth);
    /* get font plugin if present */
    font_freetype_new = (struct font_priv * (*)(void*))plugin_get_category_font("freetype");
    if (!font_freetype_new) {
        dbg(lvl_error, "no freetype");
        return NULL;
    }
#endif

    /* create root graphics layer */
    graphics_priv = g_new0(struct graphics_priv, 1);
    /* Prepare argc and argv to call Qt application*/
    graphics_priv->root = true;
    graphics_priv->argc = 0;
    graphics_priv->argv[graphics_priv->argc] = g_strdup("navit");
    graphics_priv->argc++;
    /* Get qt platform from config */
    if ((platform = attr_search(attrs, NULL, attr_qt5_platform))) {
        graphics_priv->argv[graphics_priv->argc] = g_strdup("-platform");
        graphics_priv->argc++;
        graphics_priv->argv[graphics_priv->argc] = g_strdup(platform->u.str);
        graphics_priv->argc++;
    }
    /* create surrounding application */
#if USE_QWIDGET
    QApplication* internal_app = new QApplication(graphics_priv->argc, graphics_priv->argv);
    navit_app = internal_app;
#else
    navit_app = new QGuiApplication(graphics_priv->argc, graphics_priv->argv);
#endif

#if HAVE_FREETYPE
    graphics_priv->font_freetype_new = font_freetype_new;
    font_freetype_new(&graphics_priv->freetype_methods);
    meth->font_new = (struct graphics_font_priv * (*)(struct graphics_priv*, struct graphics_font_methods*, char*, int,
                      int))graphics_priv->freetype_methods.font_new;
    meth->get_text_bbox = (void (*)(struct graphics_priv*, struct graphics_font_priv*, char*, int, int, struct point*,
                                    int))graphics_priv->freetype_methods.get_text_bbox;
#endif
    graphics_priv->callbacks = cbl;
    graphics_priv->pixmap = NULL;
    graphics_priv->use_count = 0;
    graphics_priv->painter = NULL;
    graphics_priv->parent = NULL;
    graphics_priv->overlays = g_hash_table_new(NULL, NULL);
    graphics_priv->x = 0;
    graphics_priv->y = 0;
    graphics_priv->disable = 0;
    graphics_priv->scroll_x = 0;
    graphics_priv->scroll_y = 0;
#if USE_QML
    graphics_priv->engine = NULL;
    graphics_priv->window = NULL;
    graphics_priv->GPriv = NULL;
    if (use_qml) {
        /* register our QtQuick widget to allow it's usage within QML */
        qmlRegisterType<QNavitQuick>("com.navit.graphics_qt5", 1, 0, "QNavitQuick");
        /* get our qml application from embedded resources. May be replaced by the
             * QtQuick gui component if enabled */
        graphics_priv->engine = new QQmlApplicationEngine();
        if (graphics_priv->engine != NULL) {
            graphics_priv->GPriv = new GraphicsPriv(graphics_priv);
            QQmlContext* context = graphics_priv->engine->rootContext();
            context->setContextProperty("graphics_qt5_context", graphics_priv->GPriv);
            graphics_priv->engine->load(QUrl("qrc:///loader.qml"));
            /* Get the engine's root window (for resizing) */
            QObject* toplevel = graphics_priv->engine->rootObjects().value(0);
            graphics_priv->window = qobject_cast<QQuickWindow*>(toplevel);
        }
    }
#endif
#if USE_QWIDGET
    graphics_priv->widget = NULL;
    if (use_qwidget) {
        graphics_priv->widget = new QNavitWidget(graphics_priv, NULL, Qt::Window);
    }
#endif
    if ((fullscreen = attr_search(attrs, NULL, attr_fullscreen)) && (fullscreen->u.num)) {
        /* show this maximized */
#if USE_QML
        if (graphics_priv->window != NULL)
            graphics_priv->window->setWindowState(Qt::WindowFullScreen);
#endif
#if USE_QWIDGET
        if (graphics_priv->widget != NULL)
            graphics_priv->widget->setWindowState(Qt::WindowFullScreen);
#endif
    } else {
        /* not maximized. Check what size to use then */
        struct attr* w = NULL;
        struct attr* h = NULL;
        /* default to desktop size if nothing else is given */
        QRect geomet;
        geomet.setHeight(100);
        geomet.setWidth(100);
        /* get desktop size */
        QScreen* primary = navit_app->primaryScreen();
        if (primary != NULL) {
            geomet = primary->availableGeometry();
        }
        /* check for height */
        if ((h = attr_search(attrs, NULL, attr_h)) && (h->u.num > 100))
            geomet.setHeight(h->u.num);
        /* check for width */
        if ((w = attr_search(attrs, NULL, attr_w)) && (w->u.num > 100))
            geomet.setWidth(w->u.num);
#if USE_QML
        if (graphics_priv->window != NULL) {
            graphics_priv->window->resize(geomet.width(), geomet.height());
            //graphics_priv->window->setFixedSize(geomet.width(), geomet.height());
        }
#endif
#if USE_QWIDGET
        if (graphics_priv->widget != NULL) {
            graphics_priv->widget->resize(geomet.width(), geomet.height());
            //graphics_priv->widget->setFixedSize(geomet.width(), geomet.height());
        }
#endif
    }
    /* generate initial pixmap same size as window */
    if (graphics_priv->pixmap == NULL) {
#if USE_QML
        if (graphics_priv->window != NULL)
            graphics_priv->pixmap = new QPixmap(graphics_priv->window->size());
#endif
#if USE_QWIDGET
        if (graphics_priv->widget != NULL)
            graphics_priv->pixmap = new QPixmap(graphics_priv->widget->size());
#endif
        if (graphics_priv->pixmap == NULL)
            graphics_priv->pixmap = new QPixmap(100, 100);
        graphics_priv->pixmap->fill(Qt::black);
    }

    /* tell Navit our geometry */
    resize_callback(graphics_priv, graphics_priv->pixmap->width(), graphics_priv->pixmap->height());

    /* show our window */
#if USE_QML
    if (graphics_priv->window != NULL)
        graphics_priv->window->show();
#endif
#if USE_QWIDGET
    if (graphics_priv->widget != NULL)
        graphics_priv->widget->show();
#endif

    return graphics_priv;
}

void plugin_init(void) {
#if USE_QML
    Q_INIT_RESOURCE(graphics_qt5);
#endif
    //        dbg(lvl_debug,"enter");
    plugin_register_category_graphics("qt5", graphics_qt5_new);
    qt5_event_init();
}
