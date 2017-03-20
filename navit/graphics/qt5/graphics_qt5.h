#ifndef __graphics_qt_h
#define __graphics_qt_h

#ifndef USE_QWIDGET
#define USE_QWIDGET 1
#endif

#ifndef USE_QML
#define USE_QML 0
#endif

#include <glib.h>
#include <QGuiApplication>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QBrush>
#if USE_QML
#include <QQuickWindow>
#include <QObject>
#endif
#if USE_QWIDGET
#include "QNavitWidget.h"
#endif

#ifndef QT_QPAINTER_USE_FREETYPE
#define QT_QPAINTER_USE_FREETYPE 1
#endif

#ifndef SAILFISH_OS
#define SAILFISH_OS 1
#endif


#ifdef QT_QPAINTER_USE_FREETYPE
#include "navit/font/freetype/font_freetype.h"
#endif

struct graphics_gc_priv;
struct graphics_priv;

#if USE_QML
class GraphicsPriv : public QObject
{
   Q_OBJECT
public:
   GraphicsPriv(struct graphics_priv * gp);
   ~GraphicsPriv();
   void emit_update();

   struct graphics_priv * gp;

signals:
   void update();
};
#endif

struct graphics_priv {
#if USE_QML
   GraphicsPriv * GPriv;
	QQuickWindow * window;
#endif
#if USE_QWIDGET
        QNavitWidget * widget;
#endif
        QPixmap * pixmap;
        QPainter * painter;
        int use_count;
        int disable;
        int x;
        int y;
        struct graphics_gc_priv * background_graphics_gc_priv;
#ifdef QT_QPAINTER_USE_FREETYPE
	struct font_priv * (*font_freetype_new)(void *meth);
	struct font_freetype_methods freetype_methods;
#endif
#ifdef SAILFISH_OS
        struct callback *display_on_cb;
        struct event_timeout *display_on_ev;
#endif
        struct callback_list* callbacks;
        GHashTable *overlays;
        struct graphics_priv * parent;
        bool root;
        int argc;
        char * argv[4];
};


struct graphics_gc_priv {
        struct graphics_priv * graphics_priv;
        QPen * pen;
        QBrush * brush;
};
/* central exported application info */
extern QGuiApplication * navit_app;

void resize_callback(struct graphics_priv * gr, int w, int h);

#endif

