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

#include <stdlib.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "point.h"
#include "graphics.h"
#include "container.h"


struct graphics_gra {
	GtkWidget *widget;
	int width;
	int height;
	int library_init;
	int visible;
	int buffer;
};

struct graphics_font {
};

struct graphics_gc {
	double fr,fg,fb;
	double br,bg,bb;
	double width;
	struct graphics_gra *gra;
};

static struct graphics_font *font_new(struct graphics *gr, int size)
{
	struct graphics_font *font=g_new(struct graphics_font, 1);
	return font;
}

static struct graphics_gc *gc_new(struct graphics *gr)
{
	struct graphics_gc *gc=g_new(struct graphics_gc, 1);

	gc->fr=1;
	gc->fg=1;
	gc->fb=1;
	gc->br=0;
	gc->bg=0;
	gc->bb=0;
	gc->width=1;
	gc->gra=gr->gra;
	return gc;
}

static void
gc_set_linewidth(struct graphics_gc *gc, int w)
{
	gc->width=w;	
}

static void
gc_set_foreground(struct graphics_gc *gc, int r, int g, int b)
{
	gc->fr=r/65535.0;
	gc->fg=g/65535.0;
	gc->fb=b/65535.0;
}

static void
gc_set_background(struct graphics_gc *gc, int r, int g, int b)
{
	gc->br=r/65535.0;
	gc->bg=g/65535.0;
	gc->bb=b/65535.0;
}

static void
vertex(struct point *p)
{
	double x,y;
	x=p->x;
	y=p->y;
	x/=792;	
	y/=469;	
	x-=0.5;
	y=0.5-y;
	glVertex3f(x,y,0);
}

static void
draw_lines(struct graphics *gr, struct graphics_gc *gc, struct point *p, int count)
{
	int i;

	glLineWidth(gc->width);
	glColor3f(gc->fr, gc->fg, gc->fb);
	glBegin(GL_LINE_STRIP);
	for (i=0 ; i < count ; i++) 
		vertex(p++);
	glEnd();
}

static void
draw_polygon(struct graphics *gr, struct graphics_gc *gc, struct point *p, int count)
{
	int i;
	double x,y;
	glColor3f(gc->fr, gc->fg, gc->fb);
	glBegin(GL_POLYGON);
	for (i=0 ; i < count ; i++) 
		vertex(p++);
	glEnd();
}


static void
draw_circle(struct graphics *gr, struct graphics_gc *gc, struct point *p, int r)
{
	
}

static void
draw_text(struct graphics *gr, struct graphics_gc *fg, struct graphics_gc *bg, struct graphics_font *font, unsigned char *text, int x, int y, int dx, int dy)
{
}

static void
draw_begin(struct graphics *gr)
{
	printf("draw_begin\n");
	glClearColor(gr->gc[0]->br, gr->gc[0]->bg, gr->gc[0]->bb, 0);
	glNewList(1, GL_COMPILE);
	gr->gra->buffer=1;
}

static void
draw_end(struct graphics *gr)
{
	printf("draw_end\n");
	glEndList();
	gr->gra->buffer=0;
}

static void realize(GtkWidget * widget, gpointer data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

	GLUquadricObj *qobj;
	static GLfloat light_diffuse[] = { 1.0, 0.0, 0.0, 1.0 };
	static GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };

  /*** OpenGL BEGIN ***/
	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		return;

	qobj = gluNewQuadric();
	gluQuadricDrawStyle(qobj, GLU_FILL);
#if 0
	glNewList(1, GL_COMPILE);
	gluSphere(qobj, 1.0, 20, 20);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0,0.1,0.0);	
	glVertex3f(0.1,0.1,0.0);	
	glVertex3f(0.1,0.2,0.0);	
	glVertex3f(0.2,0.2,0.0);	
	glEnd();
	glEndList();
#endif

#if 0
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
#endif

	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClearDepth(1.0);

	glViewport(0, 0,
		   widget->allocation.width, widget->allocation.height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(19.0, 1.0, 1.0, 10.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glTranslatef(0.0, 0.0, 0.0);

	gdk_gl_drawable_gl_end(gldrawable);
  /*** OpenGL END ***/
}

static gboolean
configure(GtkWidget * widget, GdkEventConfigure * event, gpointer user_data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);
	struct container *co=user_data;
	struct graphics_gra *gra=co->gra->gra;


	printf("configure %d %d\n",gra->width, gra->height);
	gra->width=widget->allocation.width;
        gra->height=widget->allocation.height;

  /*** OpenGL BEGIN ***/
	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		return FALSE;

	glViewport(0, 0,
		   widget->allocation.width, widget->allocation.height);

	gdk_gl_drawable_gl_end(gldrawable);
  /*** OpenGL END ***/
	if (gra->visible)
   	     graphics_resize(co, gra->width, gra->height);

	return TRUE;
}

static gboolean
expose(GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);
	struct container *co=user_data;
	struct graphics_gra *gra=co->gra->gra;

	printf("expose\n");
	if (! gra->visible) {
		gra->visible=1;
		configure(widget, NULL, user_data);
	}
  /*** OpenGL BEGIN ***/
	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		return FALSE;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCallList(1);

	if (gdk_gl_drawable_is_double_buffered(gldrawable))
		gdk_gl_drawable_swap_buffers(gldrawable);
	else
		glFlush();

	gdk_gl_drawable_gl_end(gldrawable);
  /*** OpenGL END ***/

	return TRUE;
}


struct graphics *
graphics_gtk_gl_area_new(struct container *co, GtkWidget **widget)
{
	GdkGLConfig *glconfig;
	gint major, minor;
	GtkWidget *drawing_area;

	struct graphics *this=g_new0(struct graphics,1);
	this->draw_lines=draw_lines;
	this->draw_polygon=draw_polygon;
	this->draw_circle=draw_circle;
	this->draw_text=draw_text;
#if 0
	this->draw_begin=draw_begin;
	this->draw_end=draw_end;
#endif
	this->gc_new=gc_new;
	this->gc_set_linewidth=gc_set_linewidth;
	this->gc_set_foreground=gc_set_foreground;
	this->gc_set_background=gc_set_background;
	this->font_new=font_new;
	this->gra=g_new0(struct graphics_gra, 1);

	/*
	 * Init GtkGLExt.
	 */

	gtk_gl_init(NULL, NULL);

	/*
	 * Query OpenGL extension version.
	 */

	gdk_gl_query_version(&major, &minor);
	g_print("OpenGL extension version - %d.%d\n", major, minor);

	/*
	 * Configure OpenGL-capable visual.
	 */

	/* Try double-buffered visual */
	glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB |
					     GDK_GL_MODE_DEPTH |
					     GDK_GL_MODE_DOUBLE);
	if (glconfig == NULL) {
		g_print("*** Cannot find the double-buffered visual.\n");
		g_print("*** Trying single-buffered visual.\n");

		/* Try single-buffered visual */
		glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB |
						     GDK_GL_MODE_DEPTH);
		if (glconfig == NULL) {
			g_print
			    ("*** No appropriate OpenGL-capable visual found.\n");
			exit(1);
		}
	}


	drawing_area = gtk_drawing_area_new();

	/* Set OpenGL-capability to the widget. */
	gtk_widget_set_gl_capability(drawing_area,
				     glconfig,
				     NULL, TRUE, GDK_GL_RGBA_TYPE);

	g_signal_connect_after(G_OBJECT(drawing_area), "realize",
			       G_CALLBACK(realize), NULL);
	g_signal_connect(G_OBJECT(drawing_area), "configure_event",
			 G_CALLBACK(configure), co);
	g_signal_connect(G_OBJECT(drawing_area), "expose_event",
			 G_CALLBACK(expose), co);

	*widget=drawing_area;
	this->gra->widget=drawing_area;
	return this;
}


