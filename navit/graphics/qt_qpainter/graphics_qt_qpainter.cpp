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

//##############################################################################################################
//#
//# File: graphics_qt_qpainter.cpp
//# Description: Graphics interface for internal GUI using Qt (Trolltech.com)
//# Comment: 
//# Authors: Martin Schaller (04/2008), Stefan Klumpp (04/2008)
//#
//##############################################################################################################


#include <glib.h>
#include "config.h"
#include "point.h"
#include "item.h"
#include "graphics.h"
#include "color.h"
#include "debug.h"
#include "plugin.h"
#include "callback.h"
#include "event.h"
#include "window.h"

#include <qglobal.h>
#if QT_VERSION < 0x040000
#include <qwidget.h>
#include <qapplication.h>
#include <qpainter.h>
#include <qpen.h>
#include <qbrush.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qlistview.h>
#ifdef HAVE_QPE
#include <qpe/qpeapplication.h>
#endif
#else
#include <QResizeEvent>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QPixmap>
#include <QWidget>
#include <QPolygonF>
#include <QtGui>
#endif

//##############################################################################################################
//# Description: RenderArea (QWidget) class for the main window (map, menu, ...) 
//# Comment: 
//# Authors: Martin Schaller (04/2008), Stefan Klumpp (04/2008)
//##############################################################################################################
class RenderArea : public QWidget
 {
 public:
     RenderArea(QWidget *parent = 0);
     QPixmap *pixmap;
     struct callback_list *cbl;

#if QT_VERSION < 0x040000
     GHashTable *timer_type;
     GHashTable *timer_callback;
#endif
 protected:
     QSize sizeHint() const;
     void paintEvent(QPaintEvent *event);
     void resizeEvent(QResizeEvent *event);
     void mouseEvent(int pressed, QMouseEvent *event);
     void mousePressEvent(QMouseEvent *event);
     void mouseReleaseEvent(QMouseEvent *event);
     void mouseMoveEvent(QMouseEvent *event);
     void wheelEvent(QWheelEvent *event);
#if QT_VERSION < 0x040000
     void timerEvent(QTimerEvent *event);
#endif
 };

//##############################################################################################################
//# Description: Constructor
//# Comment: Using a QPixmap for rendering the graphics
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
RenderArea::RenderArea(QWidget *parent)
	: QWidget(parent)
{
	pixmap = new QPixmap(800, 600);
#if QT_VERSION > 0x040000
    setWindowTitle("Navit");
#endif
#if QT_VERSION < 0x040000
	setCaption("Navit");
	timer_type=g_hash_table_new(NULL, NULL);
	timer_callback=g_hash_table_new(NULL, NULL);
#endif
}

//##############################################################################################################
//# Description: QWidget:sizeHint
//# Comment: This property holds the recommended size for the widget
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
QSize RenderArea::sizeHint() const
{
	return QSize(800, 600);
}

//##############################################################################################################
//# Description: QWidget:paintEvent
//# Comment: A paint event is a request to repaint all or part of the widget.
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::paintEvent(QPaintEvent * event)
{
	QPainter painter(this);
    painter.drawPixmap(0, 0, *pixmap);
}


//##############################################################################################################
//# Description: QWidget::resizeEvent()
//# Comment: When resizeEvent() is called, the widget already has its new geometry.
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::resizeEvent(QResizeEvent * event)
{
	QSize size=event->size();
	delete pixmap;
	pixmap = new QPixmap(size);
	callback_list_call_attr_2(this->cbl, attr_resize, (void *)size.width(), (void *)size.height());
}

//##############################################################################################################
//# Description: Method to handle mouse clicks
//# Comment: Delegate of QWidget::mousePressEvent and QWidget::mouseReleaseEvent (see below)
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::mouseEvent(int pressed, QMouseEvent *event)
{
	struct point p;
	p.x=event->x();
	p.y=event->y();
	switch (event->button()) {
	case Qt::LeftButton:
		callback_list_call_attr_3(this->cbl, attr_button, (void *)pressed, (void *)1, (void *)&p);
		break;
	case Qt::MidButton:
		callback_list_call_attr_3(this->cbl, attr_button, (void *)pressed, (void *)2, (void *)&p);
		break;
	case Qt::RightButton:
		callback_list_call_attr_3(this->cbl, attr_button, (void *)pressed, (void *)3, (void *)&p);
		break;
	default:
		break;
	}
}

void RenderArea::mousePressEvent(QMouseEvent *event)
{
	mouseEvent(1, event);
}

void RenderArea::mouseReleaseEvent(QMouseEvent *event)
{
	mouseEvent(0, event);
}

//##############################################################################################################
//# Description: QWidget::mouseMoveEvent
//# Comment: If mouse tracking is switched on, mouse move events occur even if no mouse button is pressed.
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::mouseMoveEvent(QMouseEvent *event)
{
	struct point p;
	p.x=event->x();
	p.y=event->y();
	callback_list_call_attr_1(this->cbl, attr_motion, (void *)&p);
}


//##############################################################################################################
//# Description: Qt Event :: Zoom in/out with the mouse's scrollwheel
//# Comment:
//# Authors: Stefan Klumpp (04/2008)
//##############################################################################################################
void RenderArea::wheelEvent(QWheelEvent *event)
{
	struct point p;
	int button;
	
	p.x=event->x();	// xy-coordinates of the mouse pointer
	p.y=event->y();
	
	if (event->delta() > 0)	// wheel movement away from the person
		button=4;
	else if (event->delta() < 0) // wheel movement towards the person
		button=5;
	else
		button=-1;
	
	if (button != -1) {
		callback_list_call_attr_3(this->cbl, attr_button, (void *)1, (void *)button, (void *)&p);
		callback_list_call_attr_3(this->cbl, attr_button, (void *)0, (void *)button, (void *)&p);
	}
	
	event->accept();
}

//##############################################################################################################
// General comments:
// -----------------
// gr = graphics = draw area
// gc = graphics context = pen to paint on the draw area
//##############################################################################################################

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct graphics_priv {
#ifdef HAVE_QPE
	QPEApplication *app;
#else
	QApplication *app;
#endif
	RenderArea *widget;
	QPainter *painter;
	struct graphics_gc_priv *background_gc;
	enum draw_mode_num mode;
};

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct graphics_font_priv {
	QFont *font;
};

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct graphics_gc_priv {
	QPen *pen;
	QBrush *brush;
};

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct graphics_image_priv {
	QImage *image;
};

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void graphics_destroy(struct graphics_priv *gr)
{
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void font_destroy(struct graphics_font_priv *font)
{

}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_font_methods font_methods = {
	font_destroy
};

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, char *fontfamily, int size, int flags)
{
	struct graphics_font_priv *ret=g_new0(struct graphics_font_priv, 1);
	ret->font=new QFont("Arial",size/20);
	*meth=font_methods;
	return ret;
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gc_destroy(struct graphics_gc_priv *gc)
{
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
	gc->pen->setWidth(w);
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n)
{
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
#if QT_VERSION >= 0x040000
	QColor col(c->r >> 8, c->g >> 8, c->b >> 8, c->a >> 8); 
#else
	QColor col(c->r >> 8, c->g >> 8, c->b >> 8); 
#endif
	gc->pen->setColor(col);
	gc->brush->setColor(col);
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_gc_methods gc_methods = {
	gc_destroy,
	gc_set_linewidth,
	gc_set_dashes,	
	gc_set_foreground,	
	gc_set_background	
};

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth)
{
	*meth=gc_methods;
	struct graphics_gc_priv *ret=g_new0(struct graphics_gc_priv, 1);
	ret->pen=new QPen();
	ret->brush=new QBrush(Qt::SolidPattern);
	return ret;
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_image_priv * image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *path, int *w, int *h, struct point *hot, int rotation)
{
	struct graphics_image_priv *ret;

	if (strlen(path) > 4 && !strcmp(path+strlen(path)-4, ".svg")) {
		return NULL;
	}
	ret=g_new0(struct graphics_image_priv, 1);

	ret->image=new QImage(path);
	*w=ret->image->width();
	*h=ret->image->height();
	if (hot) {
		hot->x=*w/2;
		hot->y=*h/2;
	}
	return ret;
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	int i;
#if QT_VERSION >= 0x040000
	QPolygon polygon;
#else
	QPointArray polygon;
#endif

	for (i = 0 ; i < count ; i++)
		polygon.putPoints(i, 1, p[i].x, p[i].y);
	gr->painter->setPen(*gc->pen);
	gr->painter->drawPolyline(polygon);
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	int i;
#if QT_VERSION >= 0x040000
	QPolygon polygon;
#else
	QPointArray polygon;
#endif

	for (i = 0 ; i < count ; i++)
		polygon.putPoints(i, 1, p[i].x, p[i].y);
	gr->painter->setPen(*gc->pen);
	gr->painter->setBrush(*gc->brush);
	gr->painter->drawPolygon(polygon);
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
	gr->painter->fillRect(p->x,p->y, w, h, *gc->brush);
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
	gr->painter->setPen(*gc->pen);
	gr->painter->drawArc(p->x-r/2, p->y-r/2, r, r, 0, 360*16);
	
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
	QPainter *painter=gr->painter;
	QString tmp=QString::fromUtf8(text);
#ifndef QT_NO_TRANSFORMATIONS
#if QT_VERSION >= 0x040000
	QMatrix sav=gr->painter->worldMatrix();
	QMatrix m(dx/65535.0,dy/65535.0,-dy/65535.0,dx/65535.0,p->x,p->y);
	painter->setWorldMatrix(m,TRUE);
#else
	QWMatrix sav=gr->painter->worldMatrix();
	QWMatrix m(dx/65535.0,dy/65535.0,-dy/65535.0,dx/65535.0,p->x,p->y);
	painter->setWorldMatrix(m,TRUE);
#endif
	painter->setPen(*fg->pen);
	painter->setFont(*font->font);
	painter->drawText(0, 0, tmp);
	painter->setWorldMatrix(sav);
#else
	painter->setPen(*fg->pen);
	painter->setFont(*font->font);
	painter->drawText(p->x, p->y, tmp);
#endif
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
	gr->painter->drawImage(p->x, p->y, *img->image);
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void draw_image_warp(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, int count, char *data)
{
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
	gr->background_gc=gc;
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
	dbg(1,"mode=%d\n", mode);
	if (mode == draw_mode_begin) {
		gr->painter->begin(gr->widget->pixmap);
#if 0
		gr->painter->fillRect(QRect(QPoint(0,0), gr->widget->size()), *gr->background_gc->brush);
#endif
	}
#if QT_VERSION < 0x040000
	if (mode == draw_mode_cursor) {
		gr->painter->begin(gr->widget);
	}
#endif
	if (mode == draw_mode_end) {
		if (gr->mode == draw_mode_begin) {
			gr->painter->end();
			gr->widget->update();
		} else {
#if QT_VERSION < 0x040000
			gr->painter->end();
#endif
		}
	}
	gr->mode=mode;
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h,int alpha);

static int argc=1;
static char *argv[]={(char *)"navit",NULL};

static int
fullscreen(struct window *win, int on)
{
	struct graphics_priv *this_=(struct graphics_priv *)win->priv;
	if (on)
		this_->widget->showFullScreen();
	else
		this_->widget->showMaximized();
	return 1;
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void * get_data(struct graphics_priv *this_, char *type)
{
	struct window *win;
	if (strcmp(type, "window"))
		return NULL;
	win=g_new(struct window, 1);
	this_->painter=new QPainter;
	this_->widget->showMaximized();
	win->priv=this_;
	win->fullscreen=fullscreen;
	return win;
}

static void
image_free(struct graphics_priv *gr, struct graphics_image_priv *priv)
{
	delete priv->image;
	g_free(priv);
}

static void
get_text_bbox(struct graphics_priv *gr, struct graphics_font_priv *font, char *text, int dx, int dy, struct point *ret)
{
	QPainter *painter=gr->painter;
	QString tmp=QString::fromUtf8(text);
	painter->setFont(*font->font);
	QRect r=painter->boundingRect(0,0,gr->widget->width(),gr->widget->height(),0,tmp);
	ret[0].x=0;
	ret[0].y=-r.height();
	ret[1].x=0;
	ret[1].y=0;
	ret[2].x=r.width();
	ret[2].y=0;
	ret[3].x=r.width();
	ret[3].y=-r.height();
}


//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void overlay_disable(struct graphics_priv *gr, int disable)
{
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_methods graphics_methods = {
	graphics_destroy,
	draw_mode,
	draw_lines,
	draw_polygon,
	draw_rectangle,
	draw_circle,
	draw_text,
	draw_image,
	draw_image_warp,
	draw_restore,
	NULL,
	font_new,
	gc_new,
	background_gc,
	overlay_new,
	image_new,
	get_data,
	image_free,
        get_text_bbox,
        overlay_disable,
};

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h,int alpha)
{
	*meth=graphics_methods;
	return NULL;
}

#if QT_VERSION < 0x040000


static struct graphics_priv *event_gr;

static void
event_qt_main_loop_run(void)
{
	event_gr->app->exec();
}

static void event_qt_main_loop_quit(void)
{
	dbg(0,"enter\n");
}

static struct event_watch *
event_qt_add_watch(struct file *f, enum event_watch_cond cond, struct callback *cb)
{
	dbg(0,"enter\n");
	return NULL;
}

static void
event_qt_remove_watch(struct event_watch *ev)
{
	dbg(0,"enter\n");
}

static struct event_timeout *
event_qt_add_timeout(int timeout, int multi, struct callback *cb)
{
	int id;
	id=event_gr->widget->startTimer(timeout);
	g_hash_table_insert(event_gr->widget->timer_callback, (void *)id, cb);
	g_hash_table_insert(event_gr->widget->timer_type, (void *)id, (void *)!!multi);
	return (struct event_timeout *)id;
}

static void
event_qt_remove_timeout(struct event_timeout *ev)
{
	event_gr->widget->killTimer((int) ev);
	g_hash_table_remove(event_gr->widget->timer_callback, ev);
	g_hash_table_remove(event_gr->widget->timer_type, ev);
}

static struct event_idle *
event_qt_add_idle(struct callback *cb)
{
	dbg(0,"enter\n");
	return NULL;
}

static void
event_qt_remove_idle(struct event_idle *ev)
{
	dbg(0,"enter\n");
}

static struct event_methods event_qt_methods = {
	event_qt_main_loop_run,
	event_qt_main_loop_quit,
	event_qt_add_watch,
	event_qt_remove_watch,
	event_qt_add_timeout,
	event_qt_remove_timeout,
	event_qt_add_idle,
	event_qt_remove_idle,
};

struct event_priv {
};

struct event_priv *
event_qt_new(struct event_methods *meth)
{
	dbg(0,"enter\n");
	*meth=event_qt_methods;
	return NULL;
}

void RenderArea::timerEvent(QTimerEvent *event)
{
	int id=event->timerId();
	struct callback *cb=(struct callback *)g_hash_table_lookup(timer_callback, (void *)id);
	if (cb) 
		callback_call_0(cb);
	if (!g_hash_table_lookup(timer_type, (void *)id))
		event_qt_remove_timeout((struct event_timeout *)id);
}

#endif

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_priv * graphics_qt_qpainter_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	struct graphics_priv *ret;
	dbg(0,"enter\n");
#if QT_VERSION < 0x040000
	if (event_gr)
		return NULL;
	if (! event_request_system("qt","graphics_qt_qpainter_new"))
		return NULL;
#endif

	ret=g_new0(struct graphics_priv, 1);
	*meth=graphics_methods;
#ifdef HAVE_QPE
	ret->app = new QPEApplication(argc, argv);
#else
	ret->app = new QApplication(argc, argv);
#endif
	ret->widget= new RenderArea();
	ret->widget->cbl=cbl;
#if QT_VERSION < 0x040000
	event_gr=ret;
#endif

	dbg(0,"return\n");
	return ret;
}


//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void plugin_init(void)
{
        plugin_register_graphics_type("qt_qpainter", graphics_qt_qpainter_new);
#if QT_VERSION < 0x040000
        plugin_register_event_type("qt", event_qt_new);
#endif
}




// *** EOF *** 
