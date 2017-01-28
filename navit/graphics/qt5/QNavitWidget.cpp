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
#endif
#include "graphics_qt5.h"
#include "QNavitWidget.h"
#include "QNavitWidget.moc"

QNavitWidget :: QNavitWidget(struct graphics_priv *my_graphics_priv,
                             QWidget * parent,
                             Qt::WindowFlags flags): QWidget(parent, flags)
{
    graphics_priv = my_graphics_priv;
}

bool QNavitWidget::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
        dbg(lvl_debug, "Gesture event caught");
        //return gestureEvent(static_cast<QGestureEvent*>(event));
    return QWidget::event(event);
}

void QNavitWidget :: paintEvent(QPaintEvent * event)
{
//        dbg(lvl_debug,"enter\n");
        QPainter painter(this);
        /* color background if any */
    	if (graphics_priv->background_graphics_gc_priv != NULL)
        {
                painter.setPen(*graphics_priv->background_graphics_gc_priv->pen);
                painter.fillRect(0, 0, graphics_priv->pixmap->width(),
                                       graphics_priv->pixmap->height(),
                                       *graphics_priv->background_graphics_gc_priv->brush);
        }
        painter.drawPixmap(0,0,*graphics_priv->pixmap);
        
}

void QNavitWidget::resizeEvent(QResizeEvent * event)
{
        QPainter * painter = NULL;
        if(graphics_priv->pixmap != NULL)
        {
                delete graphics_priv->pixmap;
                graphics_priv->pixmap = NULL;
        }
    
        graphics_priv->pixmap=new QPixmap(size());
        graphics_priv->pixmap->fill();
        painter = new QPainter(graphics_priv->pixmap);
        QBrush brush;
        painter->fillRect(0, 0, width(), height(), brush);
        if(painter != NULL)
           delete painter;
        dbg(lvl_debug,"size %dx%d\n", width(), height());
        dbg(lvl_debug,"pixmap %p %dx%d\n", graphics_priv->pixmap, graphics_priv->pixmap->width(), graphics_priv->pixmap->height());
        /* if the root window got resized, tell navit about it */
        if(graphics_priv->root)
                resize_callback(width(),height());
}

void QNavitWidget::mouseEvent(int pressed, QMouseEvent *event)
{
	struct point p;
//        dbg(lvl_debug,"enter\n");
	p.x=event->x();
	p.y=event->y();
	switch (event->button()) {
	case Qt::LeftButton:
		callback_list_call_attr_3(callbacks, attr_button, GINT_TO_POINTER(pressed), GINT_TO_POINTER(1), GINT_TO_POINTER(&p));
		break;
	case Qt::MidButton:
		callback_list_call_attr_3(callbacks, attr_button, GINT_TO_POINTER(pressed), GINT_TO_POINTER(2), GINT_TO_POINTER(&p));
		break;
	case Qt::RightButton:
		callback_list_call_attr_3(callbacks, attr_button, GINT_TO_POINTER(pressed), GINT_TO_POINTER(3), GINT_TO_POINTER(&p));
		break;
	default:
		break;
	}
}

void QNavitWidget::mousePressEvent(QMouseEvent *event)
{
//        dbg(lvl_debug,"enter\n");
	mouseEvent(1, event);
}

void QNavitWidget::mouseReleaseEvent(QMouseEvent *event)
{
//        dbg(lvl_debug,"enter\n");
	mouseEvent(0, event);
}

void QNavitWidget::mouseMoveEvent(QMouseEvent *event)
{
	struct point p;
//        dbg(lvl_debug,"enter\n");
	p.x=event->x();
	p.y=event->y();
	callback_list_call_attr_1(callbacks, attr_motion, (void *)&p);
}

void QNavitWidget::wheelEvent(QWheelEvent *event)
{
	struct point p;
	int button;
	dbg(lvl_debug,"enter");
	p.x=event->x();	// xy-coordinates of the mouse pointer
	p.y=event->y();
	
	if (event->delta() > 0)	// wheel movement away from the person
		button=4;
	else if (event->delta() < 0) // wheel movement towards the person
		button=5;
	else
		button=-1;
	
	if (button != -1) {
		callback_list_call_attr_3(callbacks, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(button), GINT_TO_POINTER(&p));
		callback_list_call_attr_3(callbacks, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(button), GINT_TO_POINTER(&p));
	}
	
	event->accept();
}
