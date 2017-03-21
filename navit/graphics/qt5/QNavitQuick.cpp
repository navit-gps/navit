#include "QNavitQuick.h"
#include <QPainter>

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
#endif
#include "graphics_qt5.h"

QNavitQuick::QNavitQuick(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    graphics_priv = NULL;
}

void QNavitQuick::setGraphicContext(GraphicsPriv *gp)
{
    dbg(lvl_debug,"enter\n");
    graphics_priv = gp->gp;
    QObject::connect(gp, SIGNAL(update()), this, SLOT(update()));
}

static void paintOverlays(QPainter * painter, struct graphics_priv * gp, QPaintEvent * event)
{
        GHashTableIter iter;
        struct graphics_priv * key, * value;
        g_hash_table_iter_init (&iter, gp->overlays);
        while (g_hash_table_iter_next (&iter, (void **)&key, (void **)&value))
        {
                if(! value->disable)
                {
                         QRect rr(value->x, value->y, value->pixmap->width(), value->pixmap->height());
                         if(event->rect().intersects(rr))
                         {
                                 dbg(lvl_debug,"draw overlay (%d, %d, %d, %d)\n",value->x, value->y, value->pixmap->width(), value->pixmap->height());
                                 painter->drawPixmap(value->x, value->y, *value->pixmap);
                                 /* draw overlays of overlay if any by recursive calling */
                                 paintOverlays(painter, value, event);
                         }
                }
        } 
}

void QNavitQuick::paint(QPainter *painter)
{
    QPaintEvent event = QPaintEvent(QRect(boundingRect().x(), boundingRect().y(), boundingRect().width(), boundingRect().height()));

    dbg(lvl_debug,"enter (%f, %f, %f, %f)\n",boundingRect().x(), boundingRect().y(), boundingRect().width(), boundingRect().height());

    /* color background if any */
    if (graphics_priv->background_graphics_gc_priv != NULL)
    {
            painter->setPen(*graphics_priv->background_graphics_gc_priv->pen);
            painter->fillRect(boundingRect(),*graphics_priv->background_graphics_gc_priv->brush);
    }
    /* draw base */
    painter->drawPixmap(0,0,*graphics_priv->pixmap,
                       boundingRect().x(), boundingRect().y(),
                       boundingRect().width(), boundingRect().height());
    paintOverlays(painter, graphics_priv, &event);
}


void QNavitQuick::keyPressEvent(QKeyEvent *event)
{
    dbg(lvl_debug,"enter\n");
}

void QNavitQuick::keyReleaseEvent(QKeyEvent *event)
{
    dbg(lvl_debug,"enter\n");
}

void QNavitQuick::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    dbg(lvl_debug,"enter\n");
    QQuickPaintedItem::geometryChanged(newGeometry,oldGeometry);
    QPainter * painter = NULL;
    if(graphics_priv == NULL)
    {
        dbg(lvl_debug,"Context not set, aborting\n");
	return;
    }
    if(graphics_priv->pixmap != NULL)
    {
            delete graphics_priv->pixmap;
            graphics_priv->pixmap = NULL;
    }
    
    graphics_priv->pixmap=new QPixmap(width(), height());
    painter = new QPainter(graphics_priv->pixmap);
    if(painter != NULL)
    {
        QBrush brush;
        painter->fillRect(0, 0, width(), height(), brush);
        delete painter;
    }
    dbg(lvl_debug,"size %fx%f\n", width(), height());
    dbg(lvl_debug,"pixmap %p %dx%d\n", graphics_priv->pixmap, graphics_priv->pixmap->width(), graphics_priv->pixmap->height());
    /* if the root window got resized, tell navit about it */
    if(graphics_priv->root)
        resize_callback(graphics_priv,width(),height());
}

void QNavitQuick::mouseEvent(int pressed, QMouseEvent *event)
{
    struct point p;
    dbg(lvl_debug,"enter\n");
    p.x=event->x();
    p.y=event->y();
    switch (event->button()) {
    case Qt::LeftButton:
        callback_list_call_attr_3(graphics_priv->callbacks, attr_button, GINT_TO_POINTER(pressed), GINT_TO_POINTER(1), GINT_TO_POINTER(&p));
        break;
    case Qt::MidButton:
        callback_list_call_attr_3(graphics_priv->callbacks, attr_button, GINT_TO_POINTER(pressed), GINT_TO_POINTER(2), GINT_TO_POINTER(&p));
        break;
    case Qt::RightButton:
        callback_list_call_attr_3(graphics_priv->callbacks, attr_button, GINT_TO_POINTER(pressed), GINT_TO_POINTER(3), GINT_TO_POINTER(&p));
        break;
    default:
        break;
    }
}

void QNavitQuick::mousePressEvent(QMouseEvent *event)
{
    dbg(lvl_debug,"enter\n");
    mouseEvent(1, event);
}

void QNavitQuick::mouseReleaseEvent(QMouseEvent *event)
{
    dbg(lvl_debug,"enter\n");
    mouseEvent(0, event);
}

void QNavitQuick::mouseMoveEvent(QMouseEvent *event)
{
    dbg(lvl_debug,"enter\n");
    struct point p;
    p.x=event->x();
    p.y=event->y();
    callback_list_call_attr_1(graphics_priv->callbacks, attr_motion, (void *)&p);
}

void QNavitQuick::wheelEvent(QWheelEvent *event)
{
   struct point p;
	int button;
	dbg(lvl_debug,"enter\n");
	p.x=event->x();	// xy-coordinates of the mouse pointer
	p.y=event->y();
	
	if (event->delta() > 0)	// wheel movement away from the person
		button=4;
	else if (event->delta() < 0) // wheel movement towards the person
		button=5;
	else
		button=-1;
	
	if (button != -1) {
		callback_list_call_attr_3(graphics_priv->callbacks, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(button), GINT_TO_POINTER(&p));
		callback_list_call_attr_3(graphics_priv->callbacks, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(button), GINT_TO_POINTER(&p));
	}
	
	event->accept();
}

