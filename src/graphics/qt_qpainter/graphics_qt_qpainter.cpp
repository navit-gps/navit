#include <glib.h>
#include "config.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "debug.h"
#include "plugin.h"

#if 0
#define QWS
#define NO_DEBUG
#endif

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
#else
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


class RenderArea : public QWidget
 {
 public:
     RenderArea(QWidget *parent = 0);
     QPixmap *pixmap;
     void (*resize_callback)(void *data, int w, int h);
     void *resize_callback_data;
     void (*motion_callback)(void *data, struct point *p);
     void *motion_callback_data;
     void (*button_callback)(void *data, int press, int button, struct point *p);
     void *button_callback_data;


 protected:
     QSize sizeHint() const;
     void paintEvent(QPaintEvent *event);
     void resizeEvent(QResizeEvent *event);
     void mouseEvent(int pressed, QMouseEvent *event);
     void mousePressEvent(QMouseEvent *event);
     void mouseReleaseEvent(QMouseEvent *event);
     void mouseMoveEvent(QMouseEvent *event);

 };

RenderArea::RenderArea(QWidget *parent)
     : QWidget(parent)
 {
     pixmap = new QPixmap(800, 600);
 }

 QSize RenderArea::sizeHint() const
 {
     return QSize(800, 600);
 }

void RenderArea::paintEvent(QPaintEvent * event)
{
     QPainter painter(this);
     painter.drawPixmap(0, 0, *pixmap);
}

void RenderArea::resizeEvent(QResizeEvent * event)
{
	QSize size=event->size();
	delete pixmap;
	pixmap = new QPixmap(size);
	  if (this->resize_callback)
                (this->resize_callback)(this->resize_callback_data, size.width(), size.height());
}

void RenderArea::mouseEvent(int pressed, QMouseEvent *event)
{
	struct point p;
	if (!this->button_callback)
		return;
	p.x=event->x();
	p.y=event->y();
	switch (event->button()) {
	case Qt::LeftButton:
		(this->button_callback)(this->button_callback_data, pressed, 1, &p);
		break;
	case Qt::MidButton:
		(this->button_callback)(this->button_callback_data, pressed, 2, &p);
		break;
	case Qt::RightButton:
		(this->button_callback)(this->button_callback_data, pressed, 3, &p);
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

void RenderArea::mouseMoveEvent(QMouseEvent *event)
{
	struct point p;
	if (!this->motion_callback)
		return;
	p.x=event->x();
	p.y=event->y();
	(this->motion_callback)(this->motion_callback_data, &p);
}



static int dummy;

struct graphics_priv {
	QApplication *app;
	RenderArea *widget;
	QPainter *painter;
	struct graphics_gc_priv *background_gc;
	enum draw_mode_num mode;
};

static struct graphics_font_priv {
	int dummy;
} graphics_font_priv;

static struct graphics_gc_priv {
	QPen *pen;
	QBrush *brush;
} graphics_gc_priv;

static struct graphics_image_priv {
	QImage *image;
} graphics_image_priv;

static void
graphics_destroy(struct graphics_priv *gr)
{
}

static void font_destroy(struct graphics_font_priv *font)
{

}

static struct graphics_font_methods font_methods = {
	font_destroy
};

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, int size, int flags)
{
	*meth=font_methods;
	return &graphics_font_priv;
}

static void
gc_destroy(struct graphics_gc_priv *gc)
{
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
	gc->pen->setWidth(w);
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n)
{
}

static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
#if QT_VERSION >= 0x040000
	QColor col(c->r >> 8, c->g >> 8, c->b >> 8, c->a >> 8); 
#else
	QColor col(c->r >> 8, c->g >> 8, c->b >> 8); 
#endif
	gc->pen->setColor(col);
	gc->brush->setColor(col);
}

static void
gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
}

static struct graphics_gc_methods gc_methods = {
	gc_destroy,
	gc_set_linewidth,
	gc_set_dashes,	
	gc_set_foreground,	
	gc_set_background	
};

static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth)
{
	*meth=gc_methods;
	struct graphics_gc_priv *ret=g_new0(struct graphics_gc_priv, 1);
	ret->pen=new QPen();
	ret->brush=new QBrush(Qt::SolidPattern);
	return ret;
}


static struct graphics_image_priv *
image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *path, int *w, int *h, struct point *hot)
{
	struct graphics_image_priv *ret=g_new0(struct graphics_image_priv, 1);

	ret->image=new QImage(path);
	*w=ret->image->width();
	*h=ret->image->height();
	if (hot) {
		hot->x=*w/2;
		hot->y=*h/2;
	}
	return ret;
}

static void
draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
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

static void
draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
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

static void
draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
	gr->painter->fillRect(p->x,p->y, w, h, *gc->brush);
}

static void
draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
	gr->painter->setPen(*gc->pen);
	gr->painter->drawArc(p->x-r, p->y-r, r*2, r*2, 0, 360*16);
	
}


static void
draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
	gr->painter->drawText(p->x, p->y, text);
}

static void
draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
	gr->painter->drawImage(p->x, p->y, *img->image);
}

static void
draw_image_warp(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, int count, char *data)
{
}

static void
draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
}

static void
background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
	gr->background_gc=gc;
}

static void
draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
	dbg(0,"mode=%d\n", mode);
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

static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h);

static int argc=1;
static char *argv[]={"navit",NULL};
static void *
get_data(struct graphics_priv *this_, char *type)
{
	if (strcmp(type, "window"))
		return NULL;
	this_->painter=new QPainter;
	this_->widget->show();
	return &dummy;
}



static void
register_resize_callback(struct graphics_priv *this_, void (*callback)(void *data, int w, int h), void *data)
{
	this_->widget->resize_callback=callback;
        this_->widget->resize_callback_data=data;
}

static void
register_motion_callback(struct graphics_priv *this_, void (*callback)(void *data, struct point *p), void *data)
{
	this_->widget->motion_callback=callback;
        this_->widget->motion_callback_data=data;
}

static void
register_button_callback(struct graphics_priv *this_, void (*callback)(void *data, int press, int button, struct point *p), void *data)
{
	this_->widget->button_callback=callback;
        this_->widget->button_callback_data=data;
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
	draw_image_warp,
	draw_restore,
	font_new,
	gc_new,
	background_gc,
	overlay_new,
	image_new,
	get_data,
	register_resize_callback,
	register_button_callback,
	register_motion_callback,
};

static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h)
{
	*meth=graphics_methods;
	return NULL;
}

#if QT_VERSION < 0x040000
static gboolean
graphics_qt_qpainter_idle(void *data)
{
	struct graphics_priv *gr=(struct graphics_priv *)data;
	gr->app->processOneEvent();
	return TRUE;
}
#endif


static struct graphics_priv *
graphics_qt_qpainter_new(struct graphics_methods *meth, struct attr **attrs)
{
	struct graphics_priv *ret=g_new0(struct graphics_priv, 1);
	*meth=graphics_methods;
	ret->app = new QApplication(argc, argv);
	ret->widget= new RenderArea();
#if QT_VERSION < 0x040000
	g_idle_add(graphics_qt_qpainter_idle, ret);
#endif
	
	return ret;
}

void
plugin_init(void)
{
        plugin_register_graphics_type("qt_qpainter", graphics_qt_qpainter_new);
}
