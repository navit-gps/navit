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

#ifndef __graphics_qt_h
#define __graphics_qt_h

#ifndef USE_QWIDGET
#define USE_QWIDGET 1
#endif

#ifndef USE_QML
#define USE_QML 0
#endif

#include <QBrush>
#include <QGuiApplication>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <glib.h>
#if USE_QML
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#endif
#if USE_QWIDGET
#include "QNavitWidget.h"
#endif

#ifndef HAVE_FREETYPE
#define HAVE_FREETYPE 0
#endif

#ifndef SAILFISH_OS
#define SAILFISH_OS 1
#endif

#if HAVE_FREETYPE
#include "navit/font/freetype/font_freetype.h"
#endif

struct graphics_gc_priv;
struct graphics_priv;

#if USE_QML
class GraphicsPriv : public QObject {
    Q_OBJECT
public: GraphicsPriv(struct graphics_priv* gp);
    ~GraphicsPriv();
    void emit_update();

    struct graphics_priv* gp;

signals:
    void update();
};
#endif

struct graphics_priv {
#if USE_QML
    QQmlApplicationEngine* engine;
    GraphicsPriv* GPriv;
    QQuickWindow* window;
#endif
#if USE_QWIDGET
    QNavitWidget* widget;
#endif
    QPixmap* pixmap;
    QPainter* painter;
    int use_count;
    int disable;
    int x;
    int y;
    int scroll_x;
    int scroll_y;
    struct graphics_gc_priv* background_graphics_gc_priv;
#if HAVE_FREETYPE
    struct font_priv* (*font_freetype_new)(void* meth);
    struct font_freetype_methods freetype_methods;
#endif
#ifdef SAILFISH_OS
    struct callback* display_on_cb;
    struct event_timeout* display_on_ev;
#endif
    struct callback_list* callbacks;
    GHashTable* overlays;
    struct graphics_priv* parent;
    bool root;
    int argc;
    char* argv[4];
};

struct graphics_gc_priv {
    struct graphics_priv* graphics_priv;
    QPen* pen;
    QBrush* brush;
};
/* central exported application info */
extern QGuiApplication* navit_app;

void resize_callback(struct graphics_priv* gr, int w, int h);

#endif
