#ifndef __graphics_qt_h
#define __graphics_qt_h
#include <glib.h>
#include <QApplication>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include "QNavitWidget.h"

#ifndef QT_QPAINTER_USE_FREETYPE
#define QT_QPAINTER_USE_FREETYPE 1
#endif

#ifdef QT_QPAINTER_USE_FREETYPE
#include "navit/font/freetype/font_freetype.h"
#endif

struct graphics_gc_priv;
struct graphics_priv;

struct graphics_priv {
        QNavitWidget * widget;
        QPixmap * pixmap;
        QPainter * painter;
        int use_count;
        struct graphics_gc_priv * background_graphics_gc_priv;
#ifdef QT_QPAINTER_USE_FREETYPE
	struct font_priv * (*font_freetype_new)(void *meth);
	struct font_freetype_methods freetype_methods;
#endif
        GHashTable *overlays;
        struct graphics_priv * parent;
};

struct graphics_gc_priv {
        struct graphics_priv * graphics_priv;
        QPen * pen;
        QBrush * brush;
};
/* central exported application info */
extern QApplication * navit_app;

/* navit callback list */
extern struct callback_list* callbacks;

void
resize_callback(int w, int h);

#endif