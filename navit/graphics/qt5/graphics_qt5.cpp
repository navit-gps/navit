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
#include "event_qt5.h"
#include "QNavitWidget.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QSvgRenderer>
#include <QPixmapCache>



struct callback_list* callbacks;
QApplication * navit_app = NULL;

struct graphics_font_priv {
	QFont * font;
};

struct graphics_image_priv {
	QPixmap * pixmap;
};

static void
graphics_destroy(struct graphics_priv *gr)
{
//        dbg(lvl_debug,"enter\n");
#ifdef QT_QPAINTER_USE_FREETYPE
	gr->freetype_methods.destroy();
#endif
        /* destroy painter */
        if(gr->painter != NULL)
                delete(gr->painter);
        /* destroy pixmap */
        if(gr->pixmap != NULL)
                delete(gr->pixmap);
        /* destroy widget */
        delete(gr->widget);
        /* unregister from parent, if any */
        if(gr->parent != NULL)
        {
            g_hash_table_remove(gr->parent->overlays, gr);
        }
        /* destroy overlays hash */
        g_hash_table_destroy(gr->overlays);
        /* destroy self */
        g_free(gr);
}

static void font_destroy(struct graphics_font_priv *font)
{
//        dbg(lvl_debug,"enter\n");
        if(font->font != NULL)
            delete(font->font);
        g_free(font);

}

static struct graphics_font_methods font_methods = {
	font_destroy
};

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, char *font,  int size, int flags)
{
        struct graphics_font_priv *font_priv;
//        dbg(lvl_debug,"enter (font %s, %d)\n", font, size);
        font_priv = g_new0(struct graphics_font_priv, 1);
        font_priv->font=new QFont(font,size/16);
        font_priv->font->setStyleStrategy(QFont::NoAntialias);
	*meth=font_methods;
	return font_priv;
}

static void
gc_destroy(struct graphics_gc_priv *gc)
{
//        dbg(lvl_debug,"enter gc=%p\n", gc);
        delete(gc->pen);
        delete(gc->brush);
        g_free(gc);
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
        //dbg(lvl_debug,"enter gc=%p, %d\n", gc, w);
	gc->pen->setWidth(w);
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n)
{
        /* use Qt dash feature */
        QVector<qreal> dashes;
        gc->pen->setWidth(w);
        for(int a = 0; a < n; a ++)
                dashes << dash_list[n];
        gc->pen->setDashPattern(dashes);
}

static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
	QColor col(c->r >> 8, c->g >> 8, c->b >> 8, c->a >> 8 ); 
        //dbg(lvl_debug,"context %p: color %02x%02x%02x\n",gc, c->r >> 8, c->g >> 8, c->b >> 8);
	gc->pen->setColor(col);
	gc->brush->setColor(col);
	//gc->c=*c;
}

static void
gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
	QColor col(c->r >> 8, c->g >> 8, c->b >> 8, c->a >> 8 ); 
        //dbg(lvl_debug,"context %p: color %02x%02x%02x\n",gc, c->r >> 8, c->g >> 8, c->b >> 8);
	//gc->pen->setColor(col);
	//gc->brush->setColor(col);
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
        struct graphics_gc_priv * graphics_gc_priv = NULL;
//        dbg(lvl_debug,"enter gr==%p\n", gr);
        graphics_gc_priv = g_new0(struct graphics_gc_priv, 1);
        graphics_gc_priv->graphics_priv = gr;
        graphics_gc_priv->pen=new QPen();
	graphics_gc_priv->brush=new QBrush(Qt::SolidPattern);

	*meth=gc_methods;
	return graphics_gc_priv;
}

static void image_destroy(struct graphics_image_priv *img)
{
//    dbg(lvl_debug, "enter\n");
    if(img->pixmap != NULL)
        delete(img->pixmap);
    g_free(img);
}

struct graphics_image_methods image_methods ={
	image_destroy
};


static struct graphics_image_priv *
image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *path, int *w, int *h, struct point *hot, int rotation)
{
        struct graphics_image_priv * image_priv;
        dbg(lvl_debug,"enter %s, %d %d\n", path, *w, *h);
        if(*w == -1) *w = 16;
        if(*h == -1) *h = 16;
//	QPixmap *cachedPixmap;
	QString key(path);
        image_priv = g_new0(struct graphics_image_priv, 1);
        *meth = image_methods;
//	cachedPixmap=QPixmapCache::find(key);
//	if (!cachedPixmap) {
                if(1/*key.endsWith(".svg", Qt::CaseInsensitive)*/) {
                    QSvgRenderer renderer(key);
                    if (!renderer.isValid()) {
                       g_free(image_priv);
                       return NULL;
                    }
                    image_priv->pixmap=new QPixmap(/*renderer.defaultSize()*/*w, *h);
                    image_priv->pixmap->fill(Qt::transparent);
                    QPainter painter(image_priv->pixmap); 
                    renderer.render(&painter);

                } else {
		    image_priv->pixmap=new QPixmap(path);
                }
                if (image_priv->pixmap->isNull()) {
                        g_free(image_priv);
                        return NULL;
                }
                   
//		QPixmapCache::insert(key,QPixmap(*image_priv->pixmap));
//	} else {
//		image_priv->pixmap=new QPixmap(*cachedPixmap);
//	}

	*w=image_priv->pixmap->width();
	*h=image_priv->pixmap->height();
//        dbg(lvl_debug, "Got (%d,%d)\n", *w,*h);
	if (hot) {
		hot->x=*w/2;
		hot->y=*h/2;
	}

	return image_priv;
}

static void
draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	int i;
	QPolygon polygon;
//        dbg(lvl_debug,"enter gr=%p, gc=%p, (%d, %d)\n", gr, gc, p->x, p->y);

	for (i = 0 ; i < count ; i++)
		polygon.putPoints(i, 1, p[i].x, p[i].y);
	gr->painter->setPen(*gc->pen);
	gr->painter->drawPolyline(polygon);
        
}

static void
draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	int i;
	QPolygon polygon;
//        dbg(lvl_debug,"enter gr=%p, gc=%p, (%d, %d)\n", gr, gc, p->x, p->y);

	for (i = 0 ; i < count ; i++)
		polygon.putPoints(i, 1, p[i].x, p[i].y);
	gr->painter->setPen(*gc->pen);
	gr->painter->setBrush(*gc->brush);
	gr->painter->drawPolygon(polygon);
}

static void
draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
//	dbg(lvl_debug,"gr=%p gc=%p %d,%d,%d,%d\n", gr, gc, p->x, p->y, w, h);
	gr->painter->fillRect(p->x,p->y, w, h, *gc->brush);
}

static void
draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
//        dbg(lvl_debug,"enter gr=%p, gc=%p, (%d,%d) r=%d\n", gr, gc, p->x, p->y, r);
        gr->painter->setPen(*gc->pen);
	gr->painter->drawArc(p->x-r/2, p->y-r/2, r, r, 0, 360*16);
}


static void
draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
//        dbg(lvl_debug,"enter gc=%p, fg=%p, bg=%p pos(%d,%d) %s\n", gr, fg, bg, p->x, p->y, text);
	QPainter *painter=gr->painter;
#ifdef QT_QPAINTER_USE_FREETYPE
	struct font_freetype_text *t;
	struct font_freetype_glyph *g, **gp;
	struct color transparent = {0x0000, 0x0000, 0x0000, 0x0000};
	struct color fgc;
        struct color bgc;
        QColor temp;

	int i,x,y;
	
	if (! font)
		return;
        /* extract colors */
        fgc.r = fg->pen->color().red() << 8;
        fgc.g = fg->pen->color().green() << 8;
        fgc.b = fg->pen->color().blue() << 8;
        fgc.a = fg->pen->color().alpha() << 8;
        if(bg != NULL)
        {
                bgc.r = bg->pen->color().red() << 8;
                bgc.g = bg->pen->color().green() << 8;
                bgc.b = bg->pen->color().blue() << 8;
                bgc.a = bg->pen->color().alpha() << 8;
        }
        else
                bgc = transparent;
        
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
				QImage img(g->w+2, g->h+2, QImage::Format_ARGB32_Premultiplied);
				data=img.bits();
				gr->freetype_methods.get_shadow(g,(unsigned char *)data,img.bytesPerLine(),&bgc,&transparent);

				painter->drawImage(((x+g->x)>>6)-1, ((y+g->y)>>6)-1, img);
			}
			x+=g->dx;
			y+=g->dy;
		}
	}
        x=p->x << 6;
	y=p->y << 6;
	gp=t->glyph;
	i=t->glyph_count;
	while (i-- > 0) {
		g=*gp++;
		if (g->w && g->h) {
			unsigned char *data;
			QImage img(g->w, g->h, QImage::Format_ARGB32_Premultiplied);
			data=img.bits();
			gr->freetype_methods.get_glyph(g,(unsigned char *)data,img.bytesPerLine(),&fgc,&bgc,&transparent);
			painter->drawImage((x+g->x)>>6, (y+g->y)>>6, img);
		}
                x+=g->dx;
                y+=g->dy;
	}
	gr->freetype_methods.text_destroy(t);
#else
	QString tmp=QString::fromUtf8(text);
	QMatrix sav=gr->painter->worldMatrix();
	QMatrix m(dx/65535.0,dy/65535.0,-dy/65535.0,dx/65535.0,p->x,p->y);
	painter->setWorldMatrix(m,TRUE);
	painter->setPen(*fg->pen);
	painter->setFont(*font->font);
	painter->drawText(0, 0, tmp);
        painter->setWorldMatrix(sav);
#endif
}

static void
draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
//        dbg(lvl_debug,"enter\n");
        if(gr->painter != NULL)
            gr->painter->drawPixmap(p->x, p->y, *img->pixmap);
        else
            dbg(lvl_debug, "Try to draw image, but no painter\n");
}

static void draw_drag(struct graphics_priv *gr, struct point *p)
{
        if(p != NULL)
        {
            dbg(lvl_debug,"enter %p (%d,%d)\n", gr, p->x, p->y);
            gr->widget->move(p->x, p->y);
        }
        else
        {
            dbg(lvl_debug,"enter %p (NULL)\n", gr);
            gr->widget->move(0,0);
        }
}

static void
background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
//        dbg(lvl_debug,"register context %p on %p\n", gc, gr);
        gr->background_graphics_gc_priv = gc;
}

static void
draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
        switch (mode)
        {
                case draw_mode_begin:
                    gr->use_count ++;
                    dbg(lvl_debug,"Begin drawing on context %p\n", gr);
                    if(gr->painter == NULL)
                        gr->painter = new QPainter(gr->pixmap);
                    else
                        dbg(lvl_debug, "drawing on %p already active\n", gr);
                    break;
                case draw_mode_end:
                    dbg(lvl_debug,"End drawing on context %p\n", gr);
                    gr->use_count --;
                    if(gr->painter != NULL && gr->use_count == 0)
                    {
                            gr->painter->end();
                            delete(gr->painter);
                            gr->painter = NULL;
                            /* call repaint on widget */
                            gr->widget->repaint();
                    }
                    else
                       dbg(lvl_debug, "Context %p not active!", gr)
                    
                    break;
                default:
                    dbg(lvl_debug,"Unknown drawing %d on context %p\n", mode, gr);
                    break;
        }
}

static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int wraparound);

void
resize_callback(int w, int h)
{
        dbg(lvl_debug,"enter (%d, %d)\n", w, h);
	callback_list_call_attr_2(callbacks, attr_resize,
				  GINT_TO_POINTER(w), GINT_TO_POINTER(h));
}

static int
graphics_qt5_fullscreen(struct window *w, int on)
{
        struct graphics_priv * gr;
        dbg(lvl_debug,"enter\n");
        gr = (struct graphics_priv *) w->priv;
        if(on)
            gr->widget->setWindowState(Qt::WindowFullScreen);
        else
            gr->widget->setWindowState(Qt::WindowMaximized);
	return 1;
}

static void
graphics_qt5_disable_suspend(struct window *w)
{
        dbg(lvl_debug,"enter\n");
}

static void *
get_data(struct graphics_priv *this_priv, char const *type)
{
        dbg(lvl_debug,"enter: %s\n", type);
	if (strcmp(type, "window") == 0) {
		struct window *win;
//                dbg(lvl_debug,"window detected\n");
		win = g_new0(struct window, 1);
		win->priv = this_priv;
		win->fullscreen = graphics_qt5_fullscreen;
		win->disable_suspend = graphics_qt5_disable_suspend;
                resize_callback(this_priv->widget->width(),this_priv->widget->height());
		return win;
	}
	return NULL;
}

static void image_free(struct graphics_priv *gr, struct graphics_image_priv *priv)
{
        //dbg(lvl_debug,"enter\n");
        delete(priv->pixmap);
        g_free(priv);
}

static void get_text_bbox(struct graphics_priv *gr, struct graphics_font_priv *font, char *text, int dx, int dy, struct point *ret, int estimate)
{
//        dbg(lvl_debug,"enter %s %d %d\n", text, dx, dy);
	QPainter *painter=gr->painter;
	QString tmp=QString::fromUtf8(text);
        if(gr->painter != NULL)
        {
            gr->painter->setFont(*font->font);
            QRect r=painter->boundingRect(0,0,gr->pixmap->width(),gr->pixmap->height(),0,tmp);
//            dbg (lvl_debug, "Text bbox: %d %d (%d,%d),(%d,%d)\n",dx, dy, r.left(), r.top(), r.right(), r.bottom());
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
        }
}

static void overlay_disable(struct graphics_priv *gr, int disable)
{
        GHashTableIter iter;
        struct graphics_priv * key, * value;
//        dbg(lvl_debug,"enter gr=%p, %d\n", gr, disable);

        g_hash_table_iter_init (&iter, gr->overlays);
        while (g_hash_table_iter_next (&iter, (void **)&key, (void **)&value))
        {
                /* disable or enable all overlays of this pane */
                value->widget->setVisible(!disable);
        }
}

static void overlay_resize(struct graphics_priv *gr, struct point *p, int w, int h, int wraparound)
{
        dbg(lvl_debug,"enter\n");
        
        gr->widget->move(p->x, p->y);
        gr->widget->setFixedSize(w, h);
        if(gr->painter != NULL)
        {
                delete(gr->painter);
        }
        delete(gr->pixmap);
        gr->pixmap = new QPixmap(gr->widget->size());
        if(gr->painter != NULL)
                gr->painter = new QPainter (gr->pixmap);
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
};

/* create new graphics context on given context */
static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int wraparound)
{
        struct graphics_priv * graphics_priv = NULL;
        graphics_priv = g_new0(struct graphics_priv, 1);
#ifdef QT_QPAINTER_USE_FREETYPE
	if (gr->font_freetype_new) {
		graphics_priv->font_freetype_new=gr->font_freetype_new;
        	gr->font_freetype_new(&graphics_priv->freetype_methods);
		meth->font_new=(struct graphics_font_priv *(*)(struct graphics_priv *, struct graphics_font_methods *, char *,  int, int))graphics_priv->freetype_methods.font_new;
		meth->get_text_bbox=(void (*)(struct graphics_priv*, struct graphics_font_priv*, char*, int, int, struct point*, int))graphics_priv->freetype_methods.get_text_bbox;
	}
#endif
        graphics_priv->widget = new QNavitWidget(graphics_priv, gr->widget, Qt::Widget);
        graphics_priv->widget->move(p->x, p->y);
        graphics_priv->widget->setFixedSize(w, h);
        graphics_priv->widget->setVisible(true);
        graphics_priv->pixmap = new QPixmap(graphics_priv->widget->size());
        graphics_priv->painter = NULL;
        graphics_priv->use_count = 0;
        graphics_priv->parent = gr;
        graphics_priv->overlays=g_hash_table_new(NULL, NULL);
        /* register on parent */
        g_hash_table_insert(gr->overlays, graphics_priv, graphics_priv);
        dbg(lvl_debug,"New overlay: %p\n", graphics_priv);

	*meth=graphics_methods;
	return graphics_priv;
}

static int argc=3;
static char *argv[]={"navit","-platform","wayland", NULL};

/* create application and initial graphics context */
static struct graphics_priv *
graphics_qt5_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
        struct graphics_priv * graphics_priv = NULL;
	struct attr *event_loop_system = NULL;
	*meth=graphics_methods;
        dbg(lvl_debug,"enter\n");

        event_loop_system = attr_search(attrs, NULL, attr_event_loop_system);

	if (event_loop_system && event_loop_system->u.str) {
		dbg(lvl_debug, "event_system is %s\n", event_loop_system->u.str);
		if (!event_request_system(event_loop_system->u.str, "graphics_qt5"))
			return NULL;
	} else {
		if (!event_request_system("qt5", "graphics_qt5"))
                        return NULL;
	}
#ifdef QT_QPAINTER_USE_FREETYPE
	struct font_priv * (*font_freetype_new)(void *meth);
        /* get font plugin if present */
	font_freetype_new=(struct font_priv *(*)(void *))plugin_get_font_type("freetype");
	if (!font_freetype_new) {
		dbg(lvl_error,"no freetype\n");
		return NULL;
	}
#endif

        
        /* create surrounding application */
        navit_app = new QApplication(argc, argv);
        /* create root graphics layer */
        graphics_priv = g_new0(struct graphics_priv, 1);

#ifdef QT_QPAINTER_USE_FREETYPE
	graphics_priv->font_freetype_new=font_freetype_new;
	font_freetype_new(&graphics_priv->freetype_methods);
	meth->font_new=(struct graphics_font_priv *(*)(struct graphics_priv *, struct graphics_font_methods *, char *,  int, int))graphics_priv->freetype_methods.font_new;
	meth->get_text_bbox=(void (*)(struct graphics_priv*, struct graphics_font_priv*, char*, int, int, struct point*, int))graphics_priv->freetype_methods.get_text_bbox;
#endif

        graphics_priv->widget = new QNavitWidget(graphics_priv,NULL,Qt::Window);
        QRect geomet = navit_app->desktop()->screenGeometry(graphics_priv->widget);
        graphics_priv->widget->setFixedSize(geomet.width(), geomet.height());
        /* on most platforms we may want to show this maximized at least */
        //graphics_priv->widget->setWindowState(Qt::WindowFullScreen);
        /* generate initial pixmap same size as window */
        graphics_priv->pixmap = new QPixmap(graphics_priv->widget->size());
        graphics_priv->use_count = 0;
        graphics_priv->painter = NULL;
        graphics_priv->widget->show();
        graphics_priv->parent = NULL;
        graphics_priv->overlays=g_hash_table_new(NULL, NULL);
	callbacks = cbl;
	resize_callback(graphics_priv->widget->width(),graphics_priv->widget->height());
	return graphics_priv;
}

void
plugin_init(void)
{
        dbg(lvl_debug,"enter\n");
        plugin_register_graphics_type("qt5", graphics_qt5_new);
        qt5_event_init();
}
