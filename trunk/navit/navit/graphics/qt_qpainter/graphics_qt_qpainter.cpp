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
#include <stdio.h>
#include <stdlib.h>
#include "navit/../config.h"
#include "navit/point.h"
#include "navit/item.h"
#include "navit/graphics.h"
#include "navit/color.h"
#include "navit/debug.h"
#include "navit/plugin.h"
#include "navit/callback.h"
#include "navit/event.h"
#include "navit/window.h"
#include "navit/keys.h"
#include "navit/navit.h"

#include <qglobal.h>

#ifndef QT_QPAINTER_USE_FREETYPE
#define QT_QPAINTER_USE_FREETYPE 1
#endif

#if QT_QPAINTER_USE_FREETYPE
#include "navit/font/freetype/font_freetype.h"
#endif

#if QT_VERSION < 0x040000
#include <qwidget.h>
#include <qapplication.h>
#include <qpainter.h>
#include <qpen.h>
#include <qbrush.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qpixmapcache.h>
#include <qlistview.h>
#include <qobject.h>
#include <qsocketnotifier.h>
#ifdef HAVE_QPE
#include <qpe/qpeapplication.h>
#endif
#ifndef QT_QPAINTER_USE_EVENT_QT
#define QT_QPAINTER_USE_EVENT_QT 1
#endif
#else
#include <QResizeEvent>
#include <QApplication>
#if QT_VERSION >= 0x040200
#include <QGraphicsScene>
#include <QGraphicsView>
#endif
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QPixmap>
#include <QWidget>
#include <QPolygonF>
#include <QPixmapCache>
#include <QtGui>
#ifdef HAVE_QT_SVG
#include <QSvgRenderer>
#endif

#ifndef QT_QPAINTER_USE_EVENT_GLIB
#define QT_QPAINTER_USE_EVENT_GLIB 1
#endif

#ifdef Q_WS_X11
#ifndef QT_QPAINTER_USE_EMBEDDING
#define QT_QPAINTER_USE_EMBEDDING 1
#endif
#endif

#endif

#if QT_QPAINTER_USE_EMBEDDING
#include <QX11EmbedWidget>
#endif

#ifndef QT_QPAINTER_RENDERAREA_PARENT
#define QT_QPAINTER_RENDERAREA_PARENT QWidget
#endif

class RenderArea;

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct graphics_gc_priv {
	QPen *pen;
	QBrush *brush;
	struct color c;
};

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
	unsigned char rgba[4];
	enum draw_mode_num mode;
	struct graphics_priv *parent,*overlays,*next;
	struct point p,pclean;
	int cleanup;
	int overlay_disable;
	int wraparound;
#if QT_QPAINTER_USE_FREETYPE
	struct font_priv * (*font_freetype_new)(void *meth);
	struct font_freetype_methods freetype_methods;
#endif
	int w,h;
	struct navit* nav;
};

//##############################################################################################################
//# Description: RenderArea (QWidget) class for the main window (map, menu, ...) 
//# Comment: 
//# Authors: Martin Schaller (04/2008), Stefan Klumpp (04/2008)
//##############################################################################################################
class RenderArea : public QT_QPAINTER_RENDERAREA_PARENT
{
     Q_OBJECT
 public:
     RenderArea(struct graphics_priv *priv, QT_QPAINTER_RENDERAREA_PARENT *parent = 0, int w=800, int h=800, int overlay=0);
     void do_resize(QSize size);
     QPixmap *pixmap;
     struct callback_list *cbl;
     struct graphics_priv *gra;

#if QT_QPAINTER_USE_EVENT_QT
     GHashTable *timer_type;
     GHashTable *timer_callback;
     GHashTable *watches;
#endif
 protected:
     int is_overlay;
     QSize sizeHint() const;
     void paintEvent(QPaintEvent *event);
     void resizeEvent(QResizeEvent *event);
     void mouseEvent(int pressed, QMouseEvent *event);
     void mousePressEvent(QMouseEvent *event);
     void mouseReleaseEvent(QMouseEvent *event);
     void mouseMoveEvent(QMouseEvent *event);
     void wheelEvent(QWheelEvent *event);
     void keyPressEvent(QKeyEvent *event);
     void closeEvent(QCloseEvent *event);
     bool event(QEvent *event);
#if QT_QPAINTER_USE_EVENT_QT
     void timerEvent(QTimerEvent *event);
#endif
 protected slots:
     void watchEvent(int fd);
 };

#include "graphics_qt_qpainter.moc"

static void
overlay_rect(struct graphics_priv *parent, struct graphics_priv *overlay, int clean, QRect *r)
{
	struct point p;
	int w,h;
	if (clean) {
		p=overlay->pclean;
	} else {
		p=overlay->p;;
	}
	w=overlay->widget->pixmap->width();
	h=overlay->widget->pixmap->height();
	if (overlay->wraparound) {
		if (p.x < 0)
			p.x+=parent->widget->pixmap->width();
		if (p.y < 0)
			p.y+=parent->widget->pixmap->height();
		if (w < 0)
			w += parent->widget->pixmap->width();
		if (h < 0)
			h += parent->widget->pixmap->height();
	}
	r->setRect(p.x, p.y, w, h);
}

static void
qt_qpainter_draw(struct graphics_priv *gr, const QRect *r, int paintev)
{
	if (!paintev) {
#ifndef QT_QPAINTER_NO_WIDGET
		dbg(1,"update %d,%d %d x %d\n", r->x(), r->y(), r->width(), r->height());
		if (r->x() <= -r->width())
			return;
		if (r->y() <= -r->height())
			return;
		if (r->x() > gr->widget->pixmap->width())
			return;
		if (r->y() > gr->widget->pixmap->height())
			return;
		dbg(1,"update valid %d,%d %dx%d\n", r->x(), r->y(), r->width(), r->height());
		gr->widget->update(*r);
#endif
		return;
	}
	QPixmap pixmap(r->width(),r->height());
	QPainter painter(&pixmap);
	struct graphics_priv *overlay=NULL;
	if (! gr->overlay_disable)
		overlay=gr->overlays;
	if ((gr->p.x || gr->p.y) && gr->background_gc) {
		painter.setPen(*gr->background_gc->pen);
		painter.fillRect(0, 0, gr->widget->pixmap->width(), gr->widget->pixmap->height(), *gr->background_gc->brush);
	}
	painter.drawPixmap(QPoint(gr->p.x,gr->p.y), *gr->widget->pixmap, *r);
	while (overlay) {
		QRect ovr;
		overlay_rect(gr, overlay, 0, &ovr);
		if (!overlay->overlay_disable && r->intersects(ovr)) {
			unsigned char *data;
			int i,size=ovr.width()*ovr.height();
#if QT_VERSION < 0x040000
			QImage img=overlay->widget->pixmap->convertToImage();
			img.setAlphaBuffer(1);
#else
			QImage img=overlay->widget->pixmap->toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
#endif
			data=img.bits();
			for (i = 0 ; i < size ; i++) {
				if (data[0] == overlay->rgba[0] && data[1] == overlay->rgba[1] && data[2] == overlay->rgba[2]) 
					data[3]=overlay->rgba[3];
				data+=4;
			}
#if QT_VERSION < 0x040000
			painter.drawImage(QPoint(ovr.x()-r->x(),ovr.y()-r->y()), img, 0);
#else
			painter.drawImage(QPoint(ovr.x()-r->x(),ovr.y()-r->y()), img);
#endif
		}
		overlay=overlay->next;
	}
#ifndef QT_QPAINTER_NO_WIDGET
	QPainter painterw(gr->widget);
	painterw.drawPixmap(r->x(), r->y(), pixmap);
#endif
}


//##############################################################################################################
//# Description: Constructor
//# Comment: Using a QPixmap for rendering the graphics
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
RenderArea::RenderArea(struct graphics_priv *priv, QT_QPAINTER_RENDERAREA_PARENT *parent, int w, int h, int overlay)
	: QT_QPAINTER_RENDERAREA_PARENT(parent)
{
	pixmap = new QPixmap(w, h);
#ifndef QT_QPAINTER_NO_WIDGET
	if (!overlay) {
#if QT_VERSION >= 0x040700                                                 
		grabGesture(Qt::PinchGesture);
		grabGesture(Qt::SwipeGesture);
		grabGesture(Qt::PanGesture);
#endif
#if QT_VERSION >= 0x040000
		setWindowTitle("Navit");
#else
		setCaption("Navit");
#endif
	}
#endif
	is_overlay=overlay;
	gra=priv;
#if QT_QPAINTER_USE_EVENT_QT
	timer_type=g_hash_table_new(NULL, NULL);
	timer_callback=g_hash_table_new(NULL, NULL);
	watches=g_hash_table_new(NULL, NULL);
#ifndef QT_QPAINTER_NO_WIDGET
	setBackgroundMode(NoBackground);
#endif
#endif
}

//##############################################################################################################
//# Description: QWidget:closeEvent
//# Comment: Deletes navit object and stops event loop on graphics shutdown
//##############################################################################################################
void RenderArea::closeEvent(QCloseEvent* event) 
{
	callback_list_call_attr_0(this->cbl, attr_window_closed);
}

bool RenderArea::event(QEvent *event)
{
#if QT_VERSION >= 0x040700                                                 
	if (event->type() == QEvent::Gesture) {
		dbg(0,"gesture\n");
		return true;
	}
#endif
	return QWidget::event(event);
}
//##############################################################################################################
//# Description: QWidget:sizeHint
//# Comment: This property holds the recommended size for the widget
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
QSize RenderArea::sizeHint() const
{
	return QSize(gra->w, gra->h);
}

//##############################################################################################################
//# Description: QWidget:paintEvent
//# Comment: A paint event is a request to repaint all or part of the widget.
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::paintEvent(QPaintEvent * event)
{
	qt_qpainter_draw(gra, &event->rect(), 1);
#if 0
	QPainter painter(this);
	painter.drawPixmap(0, 0, *pixmap);
#endif
}

void RenderArea::do_resize(QSize size)
{
	delete pixmap;
	pixmap=new QPixmap(size);
	pixmap->fill();
	QPainter painter(pixmap);
	QBrush brush;
	painter.fillRect(0, 0, size.width(), size.height(), brush);
	dbg(0,"size %dx%d\n", size.width(), size.height());
	dbg(0,"pixmap %p %dx%d\n", pixmap, pixmap->width(), pixmap->height());
	callback_list_call_attr_2(this->cbl, attr_resize, (void *)size.width(), (void *)size.height());
}

//##############################################################################################################
//# Description: QWidget::resizeEvent()
//# Comment: When resizeEvent() is called, the widget already has its new geometry.
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::resizeEvent(QResizeEvent * event)
{
	if (!this->is_overlay) {
		RenderArea::do_resize(event->size());
	}
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

void RenderArea::keyPressEvent(QKeyEvent *event)
{
#if QT_VERSION < 0x040000
	QString str=event->text();
	QCString cstr=str.utf8();
	const char *text=cstr;
	dbg(0,"enter text='%s' 0x%x (%d) key=%d\n", text, text[0], strlen(text), event->key());
	if (!text || !text[0] || text[0] == 0x7f) {
		dbg(0,"special key\n");
		switch (event->key()) {
		case 4099:
			{
				text=(char []){NAVIT_KEY_BACKSPACE,'\0'};
			}
			break;
		case 4101:
			{
				text=(char []){NAVIT_KEY_RETURN,'\0'};
			}
			break;
		case 4114:
			{
				text=(char []){NAVIT_KEY_LEFT,'\0'};
			}
			break;
		case 4115:
			{
				text=(char []){NAVIT_KEY_UP,'\0'};
			}
			break;
		case 4116:
			{
				text=(char []){NAVIT_KEY_RIGHT,'\0'};
			}
			break;
		case 4117:
			{
				text=(char []){NAVIT_KEY_DOWN,'\0'};
			}
			break;
		}
	}
	callback_list_call_attr_1(this->cbl, attr_keypress, (void *)text);
	event->accept();
#endif
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
struct graphics_font_priv {
	QFont *font;
};


//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct graphics_image_priv {
	QPixmap *pixmap;
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
	delete gc->pen;
	delete gc->brush;
	g_free(gc);
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
	QColor col(c->r >> 8, c->g >> 8, c->b >> 8 /* , c->a >> 8 */); 
#else
	QColor col(c->r >> 8, c->g >> 8, c->b >> 8); 
#endif
	gc->pen->setColor(col);
	gc->brush->setColor(col);
	gc->c=*c;
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
	QPixmap *cachedPixmap;
	QString key(path);

	ret=g_new0(struct graphics_image_priv, 1);

	cachedPixmap=QPixmapCache::find(key);
	if (!cachedPixmap) {
#ifdef HAVE_QT_SVG
                if(key.endsWith(".svg", Qt::CaseInsensitive)) {
                    QSvgRenderer renderer(key);
                    if (!renderer.isValid()) {
                       g_free(ret);
                       return NULL;
                    }
                    ret->pixmap=new QPixmap(renderer.defaultSize());
                    ret->pixmap->fill(Qt::transparent);
                    QPainter painter(ret->pixmap); 
                    renderer.render(&painter);

                } else {

		    ret->pixmap=new QPixmap(path);

                }
#else 
		ret->pixmap=new QPixmap(path);
#endif /* QT__VERSION */
                if (ret->pixmap->isNull()) {
                        g_free(ret);
                        return NULL;
                }
                   
		QPixmapCache::insert(key,QPixmap(*ret->pixmap));
	} else {
		ret->pixmap=new QPixmap(*cachedPixmap);
	}

	*w=ret->pixmap->width();
	*h=ret->pixmap->height();
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
	dbg(1,"gr=%p gc=%p %d,%d,%d,%d\n", gr, gc, p->x, p->y, w, h);
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
#if !QT_QPAINTER_USE_FREETYPE
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
#else
	struct font_freetype_text *t;
	struct font_freetype_glyph *g, **gp;
	struct color transparent = {0x0000, 0x0000, 0x0000, 0x0000};
	struct color *fgc=&fg->c, *bgc=&bg->c;

	int i,x,y;
	
	if (! font)
		return;
	t=gr->freetype_methods.text_new(text, (struct font_freetype_font *)font, dx, dy);
	x=p->x << 6;
	y=p->y << 6;
	gp=t->glyph;
	i=t->glyph_count;
	if (bg) {
		while (i-- > 0) {
			g=*gp++;
			if (g->w && g->h) {
				unsigned char *data;
#if QT_VERSION < 0x040000
				QImage img(g->w+2, g->h+2, 32);
				img.setAlphaBuffer(1);
				data=img.bits();
				gr->freetype_methods.get_shadow(g,(unsigned char *)(img.jumpTable()),32,0,bgc,&transparent);
#else
				QImage img(g->w+2, g->h+2, QImage::Format_ARGB32_Premultiplied);
				data=img.bits();
				gr->freetype_methods.get_shadow(g,(unsigned char *)data,32,img.bytesPerLine(),bgc,&transparent);
#endif

				painter->drawImage(((x+g->x)>>6)-1, ((y+g->y)>>6)-1, img);
			}
			x+=g->dx;
			y+=g->dy;
		}
	} else
		bgc=&transparent;
	x=p->x << 6;
	y=p->y << 6;
	gp=t->glyph;
	i=t->glyph_count;
	while (i-- > 0) {
		g=*gp++;
		if (g->w && g->h) {
			unsigned char *data;
#if QT_VERSION < 0x040000
			QImage img(g->w, g->h, 32);
			img.setAlphaBuffer(1);
			data=img.bits();
			gr->freetype_methods.get_glyph(g,(unsigned char *)(img.jumpTable()),32,0,fgc,bgc,&transparent);
#else
			QImage img(g->w, g->h, QImage::Format_ARGB32_Premultiplied);
			data=img.bits();
			gr->freetype_methods.get_glyph(g,(unsigned char *)data,32,img.bytesPerLine(),fgc,bgc,&transparent);
#endif
			painter->drawImage((x+g->x)>>6, (y+g->y)>>6, img);
		}
                x+=g->dx;
                y+=g->dy;
	}
	gr->freetype_methods.text_destroy(t);
#endif
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
	gr->painter->drawPixmap(p->x, p->y, *img->pixmap);
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


static void
draw_drag(struct graphics_priv *gr, struct point *p)
{
	if (!gr->cleanup) {
		gr->pclean=gr->p;
		gr->cleanup=1;
	}
	if (p)
		gr->p=*p;
	else {
		gr->p.x=0;
		gr->p.y=0;
	}
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
	gr->background_gc=gc;
	gr->rgba[2]=gc->c.r >> 8;
	gr->rgba[1]=gc->c.g >> 8;
	gr->rgba[0]=gc->c.b >> 8;
	gr->rgba[3]=gc->c.a >> 8;
}


//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
	dbg(1,"mode for %p %d\n", gr, mode);
	QRect r;
	if (mode == draw_mode_begin) {
#if QT_VERSION >= 0x040000
		if (gr->widget->pixmap->paintingActive()) {
			gr->widget->pixmap->paintEngine()->painter()->end();
		}
#endif
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
#if 0
		if (gr->mode == draw_mode_begin) {
#endif
			gr->painter->end();
			if (gr->parent) {
				if (gr->cleanup) {
					overlay_rect(gr->parent, gr, 1, &r);
					qt_qpainter_draw(gr->parent, &r, 0);
					gr->cleanup=0;
				}
				overlay_rect(gr->parent, gr, 0, &r);	
				qt_qpainter_draw(gr->parent, &r, 0);
			} else {
				r.setRect(0, 0, gr->widget->pixmap->width(), gr->widget->pixmap->height());
				qt_qpainter_draw(gr, &r, 0);
			}
#if 0
		} else {
#if QT_VERSION < 0x040000
			gr->painter->end();
#endif
		}
#endif
		if (!gr->parent)
			QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents|QEventLoop::ExcludeSocketNotifiers|QEventLoop::DeferredDeletion|QEventLoop::X11ExcludeTimers);
	}
#if QT_VERSION >= 0x040000
	if (mode == draw_mode_end_lazy)
		gr->painter->end();
#endif
	gr->mode=mode;
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h,int alpha, int wraparound);

static int argc=1;
static char *argv[]={(char *)"navit",NULL};

static int
fullscreen(struct window *win, int on)
{
#ifndef QT_QPAINTER_NO_WIDGET
	struct graphics_priv *this_=(struct graphics_priv *)win->priv;
	QWidget* _outerWidget;
#ifdef QT_QPAINTER_USE_EMBEDDING
	_outerWidget=(QWidget*)this_->widget->parent();
#else
	_outerWidget=this_->widget;
#endif /* QT_QPAINTER_USE_EMBEDDING */
	if (on)
		_outerWidget->showFullScreen();
	else
		_outerWidget->showMaximized();
#endif
	return 1;
}

static void
disable_suspend(struct window *win)
{
#ifdef HAVE_QPE
	struct graphics_priv *this_=(struct graphics_priv *)win->priv;
	this_->app->setTempScreenSaverMode(QPEApplication::DisableLightOff);
#endif
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void * get_data(struct graphics_priv *this_, const char *type)
{
	struct window *win;
	QString xid;
	bool ok;

	if (!strcmp(type, "resize")) {
		dbg(0,"resize %d %d\n",this_->w,this_->h);
		QSize size(this_->w,this_->h);
		this_->widget->do_resize(size);
	}
	if (!strcmp(type, "qt_widget")) 
	    return this_->widget;
	if (!strcmp(type, "qt_pixmap")) 
	    return this_->widget->pixmap;
	if (!strcmp(type, "window")) {
		win=g_new(struct window, 1);
#ifndef QT_QPAINTER_NO_WIDGET
#ifdef QT_QPAINTER_USE_EMBEDDING
		QX11EmbedWidget* _outerWidget=new QX11EmbedWidget();
#if QT_VERSION >= 0x040000
		_outerWidget->setWindowTitle("Navit");
#else
		_outerWidget->setCaption("Navit");
#endif
		QStackedLayout* _outerLayout = new QStackedLayout(_outerWidget);
		_outerWidget->setLayout(_outerLayout);
		_outerLayout->addWidget(this_->widget);
		_outerLayout->setCurrentWidget(this_->widget);
		_outerWidget->show();
		xid=getenv("NAVIT_XID");
		if (xid.length()>0) {
			_outerWidget->embedInto(xid.toULong(&ok,0));
		}
#endif /* QT_QPAINTER_USE_EMBEDDING */
		if (this_->w && this_->h)
			this_->widget->show();
		else
			this_->widget->showMaximized();
#endif /* QT_QPAINTER_NO_WIDGET */
		win->priv=this_;
		win->fullscreen=fullscreen;
		win->disable_suspend=disable_suspend;
		return win;
	}
	return NULL;
}

static void
image_free(struct graphics_priv *gr, struct graphics_image_priv *priv)
{
	delete priv->pixmap;
	g_free(priv);
}

static void
get_text_bbox(struct graphics_priv *gr, struct graphics_font_priv *font, char *text, int dx, int dy, struct point *ret, int estimate)
{
	QPainter *painter=gr->painter;
	QString tmp=QString::fromUtf8(text);
	painter->setFont(*font->font);
	QRect r=painter->boundingRect(0,0,gr->w,gr->h,0,tmp);
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
	gr->overlay_disable=disable;
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static int set_attr(struct graphics_priv *gr, struct attr *attr)
{
	switch (attr->type) {
	case attr_w:
		gr->w=attr->u.num;
		if (gr->w != 0 && gr->h != 0) {
			QSize size(gr->w,gr->h);
			gr->widget->do_resize(size);
		}
		break;
	case attr_h:
		gr->h=attr->u.num;
		if (gr->w != 0 && gr->h != 0) {
			QSize size(gr->w,gr->h);
			gr->widget->do_resize(size);
		}
		break;
	default:
		return 0;
	}
	return 1;
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
	NULL,
	set_attr,
	
};

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h,int alpha,int wraparound)
{
	*meth=graphics_methods;
	struct graphics_priv *ret=g_new0(struct graphics_priv, 1);
#if QT_QPAINTER_USE_FREETYPE
	if (gr->font_freetype_new) {
		ret->font_freetype_new=gr->font_freetype_new;
        	gr->font_freetype_new(&ret->freetype_methods);
		meth->font_new=(struct graphics_font_priv *(*)(struct graphics_priv *, struct graphics_font_methods *, char *,  int, int))ret->freetype_methods.font_new;
		meth->get_text_bbox=(void (*)(struct graphics_priv*, struct graphics_font_priv*, char*, int, int, struct point*, int))ret->freetype_methods.get_text_bbox;
	}
#endif
	ret->widget= new RenderArea(ret,gr->widget,w,h,1);
	ret->wraparound=wraparound;
	ret->painter=new QPainter;
	ret->p=*p;
	ret->parent=gr;
	ret->next=gr->overlays;
	gr->overlays=ret;
#ifndef QT_QPAINTER_NO_WIDGET
	ret->widget->hide();
#endif
	return ret;
}

#if QT_QPAINTER_USE_EVENT_QT


static struct graphics_priv *event_gr;

static void
event_qt_main_loop_run(void)
{
	event_gr->app->exec();
}

static void event_qt_main_loop_quit(void)
{
	dbg(0,"enter\n");
	exit(0);
}

struct event_watch {
	QSocketNotifier *sn;
	struct callback *cb;
	void *fd;
};

static struct event_watch *
event_qt_add_watch(void *fd, enum event_watch_cond cond, struct callback *cb)
{
	dbg(1,"enter fd=%d\n",(int)(long)fd);
	struct event_watch *ret=g_new0(struct event_watch, 1);
	ret->fd=fd;
	ret->cb=cb;
	g_hash_table_insert(event_gr->widget->watches, fd, ret);
	ret->sn=new QSocketNotifier((int)(long)fd, QSocketNotifier::Read, event_gr->widget);
	QObject::connect(ret->sn, SIGNAL(activated(int)), event_gr->widget, SLOT(watchEvent(int)));
	return ret;
}

static void
event_qt_remove_watch(struct event_watch *ev)
{
	g_hash_table_remove(event_gr->widget->watches, ev->fd);
	delete(ev->sn);
	g_free(ev);

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
	event_gr->widget->killTimer((int)(long)ev);
	g_hash_table_remove(event_gr->widget->timer_callback, ev);
	g_hash_table_remove(event_gr->widget->timer_type, ev);
}

static struct event_idle *
event_qt_add_idle(int priority, struct callback *cb)
{
	dbg(0,"enter\n");
	return (struct event_idle *)event_qt_add_timeout(0, 1, cb);
}

static void
event_qt_remove_idle(struct event_idle *ev)
{
	dbg(0,"enter\n");
	event_qt_remove_timeout((struct event_timeout *) ev);
}

static void
event_qt_call_callback(struct callback_list *cb)
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
	event_qt_call_callback,
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

void RenderArea::watchEvent(int fd)
{
#if QT_QPAINTER_USE_EVENT_QT
	struct event_watch *ev=(struct event_watch *)g_hash_table_lookup(watches, (void *)fd);
	dbg(1,"fd=%d ev=%p cb=%p\n", fd, ev, ev->cb);
	callback_call_0(ev->cb);
#endif
}


//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_priv * graphics_qt_qpainter_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	struct graphics_priv *ret;
	struct font_priv * (*font_freetype_new)(void *meth);
	struct attr *attr;

	dbg(0,"enter\n");
#if QT_QPAINTER_USE_EVENT_QT
	if (event_gr)
		return NULL;
	if (! event_request_system("qt","graphics_qt_qpainter_new"))
		return NULL;
#endif
#if QT_QPAINTER_USE_EVENT_GLIB
	if (! event_request_system("glib","graphics_qt_qpainter_new"))
		return NULL;
#endif
#if QT_QPAINTER_USE_FREETYPE
	font_freetype_new=(struct font_priv *(*)(void *))plugin_get_font_type("freetype");
	if (!font_freetype_new)
		return NULL;
#endif
	ret=g_new0(struct graphics_priv, 1);
	*meth=graphics_methods;
	ret->nav=nav;
#if QT_QPAINTER_USE_FREETYPE
	ret->font_freetype_new=font_freetype_new;
	font_freetype_new(&ret->freetype_methods);
	meth->font_new=(struct graphics_font_priv *(*)(struct graphics_priv *, struct graphics_font_methods *, char *,  int, int))ret->freetype_methods.font_new;
	meth->get_text_bbox=(void (*)(struct graphics_priv*, struct graphics_font_priv*, char*, int, int, struct point*, int))ret->freetype_methods.get_text_bbox;
#endif

#if QT_QPAINTER_USE_EMBEDDING && QT_VERSION >= 0x040500
	if ((attr=attr_search(attrs, NULL, attr_gc_type)))
		QApplication::setGraphicsSystem(attr->u.str);
	else
		QApplication::setGraphicsSystem("raster");
#endif

#ifndef QT_QPAINTER_NO_APP
#ifdef HAVE_QPE
	ret->app = new QPEApplication(argc, argv);
#else
	ret->app = new QApplication(argc, argv);
#endif
#endif
	ret->widget= new RenderArea(ret);
	ret->widget->cbl=cbl;
	ret->painter = new QPainter;
#if QT_QPAINTER_USE_EVENT_QT
	event_gr=ret;
#endif
	ret->w=800;
	ret->h=600;
	if ((attr=attr_search(attrs, NULL, attr_w)))
		ret->w=attr->u.num;
	if ((attr=attr_search(attrs, NULL, attr_h)))
		ret->h=attr->u.num;

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
#if QT_QPAINTER_USE_EVENT_QT
        plugin_register_event_type("qt", event_qt_new);
#endif
}




// *** EOF *** 
