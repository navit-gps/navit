#include <math.h>
#include <glib.h>
#include "config.h"
#include <GL/glc.h>
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"

#include "debug.h"

#include <GL/glut.h>


void CALLBACK tessBeginCB(GLenum which);
void CALLBACK tessEndCB();
void CALLBACK tessErrorCB(GLenum errorCode);
void CALLBACK tessVertexCB(const GLvoid *data);
void CALLBACK tessVertexCB2(const GLvoid *data);
void CALLBACK tessCombineCB(const GLdouble newVertex[3], const GLdouble *neighborVertex[4],
                            const GLfloat neighborWeight[4], GLdouble **outData);
	

struct graphics_priv {
	int button_timeout;
	struct point p;
	int width;
	int height;
	int library_init;
	int visible;
	struct graphics_priv *parent;
	struct graphics_priv *overlays;
	struct graphics_priv *next;
	struct graphics_gc_priv *background_gc;
	enum draw_mode_num mode;
	void (*resize_callback)(void *data, int w, int h);
	void *resize_callback_data;
	void (*motion_callback)(void *data, struct point *p);
	void *motion_callback_data;
	void (*button_callback)(void *data, int press, int button, struct point *p);
	void *button_callback_data;
	GLuint DLid;
};

struct graphics_font_priv {
#if 0
        FT_Face face;
#endif
};

struct graphics_gc_priv {
	struct graphics_priv *gr;
	float fr,fg,fb,fa;
	float br,bg,bb,ba;
	int linewidth;
};

struct graphics_image_priv {
	int w;
	int h;
};

static void
graphics_destroy(struct graphics_priv *gr)
{
}

int frame=0;

// http://quesoglc.sourceforge.net/tutorial.php


static void font_destroy(struct graphics_font_priv *font)
{
	g_free(font);
	/* TODO: free font->face */
}

static struct graphics_font_methods font_methods = {
	font_destroy
};

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, int size)
{
#if 0
	char **filename=fontlist;
	struct graphics_font_priv *font=g_new(struct graphics_font_priv, 1);

	*meth=font_methods;

	if (!gr->library_init) {
		FT_Init_FreeType( &gr->library );
		gr->library_init=1;
	}

	while (*filename) {	
	    	if (!FT_New_Face( gr->library, *filename, 0, &font->face ))
			break;
		filename++;
	}
	if (! *filename) {
		g_warning("Failed to load font, no labelling");
		g_free(font);
		return NULL;
	}
        FT_Set_Char_Size(font->face, 0, size, 300, 300);
	FT_Select_Charmap(font->face, FT_ENCODING_UNICODE);
	return font;
#endif
	return NULL;
}

static void
gc_destroy(struct graphics_gc_priv *gc)
{
	g_free(gc);
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
	gc->linewidth=w;
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, unsigned char *dash_list, int n)
{
#if 0
	gdk_gc_set_dashes(gc->gc, 0, (gint8 *)dash_list, n);
	gdk_gc_set_line_attributes(gc->gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_ROUND, GDK_JOIN_ROUND);
#endif
}


static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
	gc->fr=c->r/65535.0;
	gc->fg=c->g/65535.0;
	gc->fb=c->b/65535.0;
	gc->fa=c->a/65535.0;
// 	printf("new alpha : %i\n",c->a);
}

static void
gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
	gc->br=c->r/65535.0;
	gc->bg=c->g/65535.0;
	gc->bb=c->b/65535.0;
	gc->ba=c->a/65535.0;
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
	struct graphics_gc_priv *gc=g_new(struct graphics_gc_priv, 1);

	*meth=gc_methods;
	gc->gr=gr;
	gc->linewidth=1;
	return gc;
}


static struct graphics_image_priv *
image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name, int *w, int *h)
{
#if 0
	GdkPixbuf *pixbuf;
	struct graphics_image_priv *ret;

	pixbuf=gdk_pixbuf_new_from_file(name, NULL);
	if (! pixbuf)
		return NULL;
	ret=g_new0(struct graphics_image_priv, 1);
	ret->pixbuf=pixbuf;
	ret->w=gdk_pixbuf_get_width(pixbuf);
	ret->h=gdk_pixbuf_get_height(pixbuf);
	*w=ret->w;
	*h=ret->h;
	return ret;
#endif
	return NULL;
}

static void
draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	 int i;
/*
	if (gr->mode == draw_mode_begin || gr->mode == draw_mode_end) 
		gdk_draw_lines(gr->drawable, gc->gc, (GdkPoint *)p, count);
	if (gr->mode == draw_mode_end || gr->mode == draw_mode_cursor)
		gdk_draw_lines(gr->widget->window, gc->gc, (GdkPoint *)p, count);
*/
	/*	
	if(gr->mode == draw_mode_begin){
		printf("B");
	} else if (gr->mode == draw_mode_end){
		printf("E");
	} else {
		printf("x");
	}
*/	

	for (i = 0 ; i < count-1 ; i++) {

// 	  	glEnable( GL_POLYGON_SMOOTH );
// 	 	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
// 		glEnable( GL_BLEND );

		float dx=p[i+1].x-p[i].x;
		float dy=p[i+1].y-p[i].y;

		float cx=(p[i+1].x+p[i].x)/2;
		float cy=(p[i+1].y+p[i].y)/2;
//		printf("(%lx,%lx) -> (%lx,%lx) : (%lx,%lx)\n",p[i].x,p[i].y,p[i+1].x,p[i+1].y,dx,dy);

		int w=round(sqrt(pow((dx),2)+pow((dy),2)));

		float angle=atan (dy/dx) * 180 / M_PI;

		glPushMatrix();
		glTranslatef(cx,cy,1);
	//  	glColor4f( 0,0,0,1);
	//  	glRasterPos2f( 1,1 );
		glRotatef(angle,0.0,0.0,1.0);

		glColor4f( gc->fr, gc->fg, gc->fb, gc->fa);

		int linewidth=gc->linewidth;

		glBegin( GL_POLYGON );
				glVertex2f( -w/2,-linewidth/2 );
				glVertex2f( -w/2-4,0 );
				glVertex2f( -w/2,+linewidth/2 );
				glVertex2f( +w/2,+linewidth/2 );
				glVertex2f( +w/2+4,0 );
				glVertex2f( +w/2,-linewidth/2 );
				glVertex2f( -w/2,+linewidth/2 );
		glEnd();

		
		// FIXME Roads label can maybe be drawn here, avoid the display_label loop, when playing with Z axis position.
		/*
		if(attr==1){
			glcRenderStyle(GLC_TEXTURE);
			glColor3f(0., 0., 0.);
			glScalef(12, 12, 0.);
			glcRenderString(">>");
		} else if(attr==-1){
			glcRenderStyle(GLC_TEXTURE);
			glColor3f(0., 0., 0.);
			glScalef(12, 12, 0.);
			glcRenderString("<<");
		}

		*/
		glPopMatrix();
	}
//  	glDisable( GL_BLEND );
// 	glDisable( GL_POLYGON_SMOOTH );

/*
	if(label){
		if((strlen(label)*6)<w){
			SDL_print(label,cx, cy,-angle);
		}
	}
*/
}


const char* getPrimitiveType(GLenum type)
{
    switch(type)
    {
    case 0x0000:
        return "GL_POINTS"; 
        break;
    case 0x0001:
        return "GL_LINES";
        break;
    case 0x0002:
        return "GL_LINE_LOOP";
        break;
    case 0x0003:
        return "GL_LINE_STRIP";
        break;
    case 0x0004:
        return "GL_TRIANGLES";
        break;
    case 0x0005:
        return "GL_TRIANGLE_STRIP";
        break;
    case 0x0006:
        return "GL_TRIANGLE_FAN";
        break;
    case 0x0007:
        return "GL_QUADS";
        break;
    case 0x0008:
        return "GL_QUAD_STRIP";
        break;
    case 0x0009:
        return "GL_POLYGON";
        break;
    }
}

void CALLBACK tessBeginCB(GLenum which)
{
    glBegin(which);

    dbg(1,"glBegin( %s );\n",getPrimitiveType(which));
}



void CALLBACK tessEndCB()
{
    glEnd();

    dbg(1,"glEnd();\n");
}



void CALLBACK tessVertexCB(const GLvoid *data)
{
    // cast back to double type
    const GLdouble *ptr = (const GLdouble*)data;

    glVertex3dv(ptr);

    dbg(1,"  glVertex3d();\n");
}


static void
draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	int i;

	GLUtesselator *tess = gluNewTess(); // create a tessellator
	if(!tess) return 0;  // failed to create tessellation object, return 0

	GLdouble quad1[count][3];
	for (i = 0 ; i < count ; i++) {
		quad1[i][0]=(GLdouble)(p[i].x);
		quad1[i][1]=(GLdouble)(p[i].y);
		quad1[i][2]=0;
	}


	// register callback functions
	gluTessCallback(tess, GLU_TESS_BEGIN, (void (*)(void))tessBeginCB);
	gluTessCallback(tess, GLU_TESS_END, (void (*)(void))tessEndCB);
	//     gluTessCallback(tess, GLU_TESS_ERROR, (void (*)(void))tessErrorCB);
	gluTessCallback(tess, GLU_TESS_VERTEX, (void (*)(void))tessVertexCB);
	
	// tessellate and compile a concave quad into display list
	// gluTessVertex() takes 3 params: tess object, pointer to vertex coords,
	// and pointer to vertex data to be passed to vertex callback.
	// The second param is used only to perform tessellation, and the third
	// param is the actual vertex data to draw. It is usually same as the second
	// param, but It can be more than vertex coord, for example, color, normal
	// and UV coords which are needed for actual drawing.
	// Here, we are looking at only vertex coods, so the 2nd and 3rd params are
	// pointing same address.
	glColor4f( gc->fr, gc->fg, gc->fb, gc->fa);
	gluTessBeginPolygon(tess, 0);                   // with NULL data
		gluTessBeginContour(tess);
		for (i = 0 ; i < count ; i++) {
		gluTessVertex(tess, quad1[i], quad1[i]);
		}
		gluTessEndContour(tess);
	gluTessEndPolygon(tess);
	
	gluDeleteTess(tess);        // delete after tessellation

}

static void
draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
#if 0
	gdk_draw_rectangle(gr->drawable, gc->gc, TRUE, p->x, p->y, w, h);
#endif
}

static void
draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
#if 0
	if (gr->mode == draw_mode_begin || gr->mode == draw_mode_end)
		gdk_draw_arc(gr->drawable, gc->gc, FALSE, p->x-r/2, p->y-r/2, r, r, 0, 64*360);
	if (gr->mode == draw_mode_end || gr->mode == draw_mode_cursor)
		gdk_draw_arc(gr->widget->window, gc->gc, FALSE, p->x-r/2, p->y-r/2, r, r, 0, 64*360);
#endif
}



void SDL_print(char * label,int x, int y, double angle)
{
	glPushMatrix();
	glcRenderStyle(GLC_TEXTURE);
	glColor4f(0,0,0,1);
	glTranslatef(x, y, 1);
    	glRotatef(180,1,0,0);
   	glRotatef(angle,0,0,1);

	glScalef(14, 14, 14);
	// FIXME : add some error checking : glcGetError()
	glcRenderString(label);
	glPopMatrix();

}

static void
draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
//	dbg(0,"%s : %i,%i, %f\n",text,dx,dy,(180*atan2(dx,dy)/3.14));
	SDL_print(text,p->x,p->y,(180*atan2(dx,dy)/3.14)-90);
}

static void
draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
#if 0
	gdk_draw_pixbuf(gr->drawable, fg->gc, img->pixbuf, 0, 0, p->x, p->y,
		    img->w, img->h, GDK_RGB_DITHER_NONE, 0, 0);
#endif
}

#ifdef HAVE_IMLIB2
static void
draw_image_warp(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, int count, char *data)
{
#if 0
	void *image;
	int w,h;
	printf("draw_image_warp data=%s\n", data);
	image = imlib_load_image(data);
	imlib_context_set_display(gdk_x11_drawable_get_xdisplay(gr->widget->window));
	imlib_context_set_colormap(gdk_x11_colormap_get_xcolormap(gtk_widget_get_colormap(gr->widget)));
	imlib_context_set_visual(gdk_x11_visual_get_xvisual(gtk_widget_get_visual(gr->widget)));
	imlib_context_set_drawable(gdk_x11_drawable_get_xid(gr->drawable));
	imlib_context_set_image(image);
	w = imlib_image_get_width();
	h = imlib_image_get_height();
	if (count == 3) {
		imlib_render_image_on_drawable_skewed(0, 0, w, h, p[0].x, p[0].y, p[1].x-p[0].x, p[1].y-p[0].y, p[2].x-p[0].x, p[2].y-p[0].y);
	}
	if (count == 2) {
		imlib_render_image_on_drawable_skewed(0, 0, w, h, p[0].x, p[0].y, p[1].x-p[0].x, 0, 0, p[1].y-p[0].y);
	}
#endif
}
#endif

static void
overlay_draw(struct graphics_priv *parent, struct graphics_priv *overlay, int window)
{
#if 0
	GdkPixbuf *pixbuf,*pixbuf2;
	GtkWidget *widget=parent->widget;
	guchar *pixels1, *pixels2, *p1, *p2;
	int x,y;
	int rowstride1,rowstride2;
	int n_channels1,n_channels2;

	if (! parent->drawable)
		return;

	pixbuf=gdk_pixbuf_get_from_drawable(NULL, overlay->drawable, NULL, 0, 0, 0, 0, overlay->width, overlay->height);
	pixbuf2=gdk_pixbuf_new(gdk_pixbuf_get_colorspace(pixbuf), TRUE, gdk_pixbuf_get_bits_per_sample(pixbuf), 
				gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));

	rowstride1 = gdk_pixbuf_get_rowstride (pixbuf);
	rowstride2 = gdk_pixbuf_get_rowstride (pixbuf2);
	pixels1=gdk_pixbuf_get_pixels (pixbuf);	
	pixels2=gdk_pixbuf_get_pixels (pixbuf2);	
	n_channels1 = gdk_pixbuf_get_n_channels (pixbuf);
	n_channels2 = gdk_pixbuf_get_n_channels (pixbuf2);
	for (y = 0 ; y < overlay->height ; y++) {
		for (x = 0 ; x < overlay->width ; x++) {
			p1 = pixels1 + y * rowstride1 + x * n_channels1;
			p2 = pixels2 + y * rowstride2 + x * n_channels2;
			p2[0]=p1[0];
			p2[1]=p1[1];
			p2[2]=p1[2];
			p2[3]=127;
		}
	}
	if (window)
		gdk_draw_pixmap(parent->drawable, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], overlay->background, 0, 0, overlay->p.x, overlay->p.y, overlay->width, overlay->height);
	else
		gdk_draw_pixmap(overlay->background, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], parent->drawable, overlay->p.x, overlay->p.y, 0, 0, overlay->width, overlay->height);
	gdk_draw_pixbuf(parent->drawable, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], pixbuf2, 0, 0, overlay->p.x, overlay->p.y, overlay->width, overlay->height, GDK_RGB_DITHER_NONE, 0, 0);
	if (window)
		gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], parent->drawable, overlay->p.x, overlay->p.y, overlay->p.x, overlay->p.y, overlay->width, overlay->height);
#if 0
	gdk_draw_pixmap(gr->gra->drawable,
                        gr->gra->widget->style->fg_gc[GTK_WIDGET_STATE(gr->gra->widget)],
                        img->gra->drawable,
                        0, 0, p->x, p->y, img->gra->width, img->gra->height);
#endif
#endif
}

static void
draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
#if 0
	GtkWidget *widget=gr->widget;
	gdk_draw_pixmap(widget->window,
                        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                        gr->drawable,
                        p->x, p->y, p->x, p->y, w, h);
#endif

}

static void
background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
	gr->background_gc=gc;
}

static void
draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
	if (gr->DLid) {
		if (mode == draw_mode_begin)
			glNewList(gr->DLid,GL_COMPILE);
		if (mode == draw_mode_end)
			glEndList();
	}

#if 0
	struct graphics_priv *overlay;
	GtkWidget *widget=gr->widget;

	if (mode == draw_mode_begin) {
		if (! gr->parent && gr->background_gc)
			gdk_draw_rectangle(gr->drawable, gr->background_gc->gc, TRUE, 0, 0, gr->width, gr->height);
	}
	if (mode == draw_mode_end && gr->mode == draw_mode_begin) {
		if (gr->parent) {
			overlay_draw(gr->parent, gr, 1);
		} else {
			overlay=gr->overlays;
			while (overlay) {
				overlay_draw(gr, overlay, 0);
				overlay=overlay->next;
			}
			gdk_draw_pixmap(widget->window,
                	        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                	        gr->drawable,
                	        0, 0, 0, 0, gr->width, gr->height);
		}
	}
	gr->mode=mode;
#endif
}

#if 0
/* Events */

static gint
configure(GtkWidget * widget, GdkEventConfigure * event, gpointer user_data)
{
	struct graphics_priv *gra=user_data;
	if (! gra->visible)
		return TRUE;
	if (gra->drawable != NULL) {
                gdk_pixmap_unref(gra->drawable);
        }
	gra->width=widget->allocation.width;
	gra->height=widget->allocation.height;
        gra->drawable = gdk_pixmap_new(widget->window, gra->width, gra->height, -1);
	if (gra->resize_callback) 
		(*gra->resize_callback)(gra->resize_callback_data, gra->width, gra->height);
	return TRUE;
}

static gint
expose(GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
	struct graphics_priv *gra=user_data;

	gra->visible=1;
	if (! gra->drawable)
		configure(widget, NULL, user_data);
        gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                        gra->drawable, event->area.x, event->area.y,
                        event->area.x, event->area.y,
                        event->area.width, event->area.height);

	return FALSE;
}

#if 0
static gint
button_timeout(gpointer user_data)
{
#if 0
	struct container *co=user_data;
	int x=co->gra->gra->button_event.x; 
	int y=co->gra->gra->button_event.y; 
	int button=co->gra->gra->button_event.button;

	co->gra->gra->button_timeout=0;
	popup(co, x, y, button);

	return FALSE;
#endif
}
#endif

static gint
button_press(GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
	struct graphics_priv *this=user_data;
	struct point p;

	p.x=event->x;
	p.y=event->y;
	if (this->button_callback) 
		(*this->button_callback)(this->button_callback_data, 1, event->button, &p);
	return FALSE;
}

static gint
button_release(GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
	struct graphics_priv *this=user_data;
	struct point p;

	p.x=event->x;
	p.y=event->y;
	if (this->button_callback) 
		(*this->button_callback)(this->button_callback_data, 0, event->button, &p);
	return FALSE;
}

static gint
scroll(GtkWidget * widget, GdkEventScroll * event, gpointer user_data)
{
	struct graphics_priv *this=user_data;
	struct point p;
	int button;

	p.x=event->x;
	p.y=event->y;
	if (this->button_callback) {
		switch (event->direction) {
		case GDK_SCROLL_UP:
			button=4;
			break;
		case GDK_SCROLL_DOWN:
			button=5;
			break;
		default:
			button=-1;
			break;
		}
		if (button != -1) {
			(*this->button_callback)(this->button_callback_data, 1, button, &p);
			(*this->button_callback)(this->button_callback_data, 0, button, &p);
		}
	}
	return FALSE;
}

static gint
motion_notify(GtkWidget * widget, GdkEventMotion * event, gpointer user_data)
{
	struct graphics_priv *this=user_data;
	struct point p;

	p.x=event->x;
	p.y=event->y;
	if (this->motion_callback) 
		(*this->motion_callback)(this->motion_callback_data, &p);
	return FALSE;
}

#endif


static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h)
{
#if 0
	struct graphics_priv *this=graphics_gtk_drawing_area_new_helper(meth);
	this->drawable=gdk_pixmap_new(gr->widget->window, w, h, -1);
	this->colormap=gr->colormap;
	this->widget=gr->widget;
	this->p=*p;
	this->width=w;
	this->height=h;
	this->parent=gr;
	this->background=gdk_pixmap_new(gr->widget->window, w, h, -1);
	this->next=gr->overlays;
	gr->overlays=this;
	return this;
#endif
	return NULL;
}

static void *
get_data(struct graphics_priv *this, char *type)
{
	if (strcmp(type,"opengl_displaylist"))
		return NULL;
#if 0
        dbg(1,"Creating the DL from driver\n");
        this->DLid = glGenLists(1);
#endif
        return &this->DLid;
}

static void
register_resize_callback(struct graphics_priv *this, void (*callback)(void *data, int w, int h), void *data)
{
	this->resize_callback=callback;
	this->resize_callback_data=data;
}

static void
register_motion_callback(struct graphics_priv *this, void (*callback)(void *data, struct point *p), void *data)
{
	this->motion_callback=callback;
	this->motion_callback_data=data;
}

static void
register_button_callback(struct graphics_priv *this, void (*callback)(void *data, int press, int button, struct point *p), void *data)
{
	this->button_callback=callback;
	this->button_callback_data=data;
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
#ifdef HAVE_IMLIB2
	draw_image_warp,
#else
	NULL,
#endif
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
	NULL,	// image_free
};

static struct graphics_priv *
graphics_opengl_new(struct graphics_methods *meth, struct attr **attrs)
{
	struct graphics_priv *this=g_new0(struct graphics_priv,1);
	*meth=graphics_methods;
	
// 	GtkWidget *draw;

// 	draw=gtk_drawnig_area_new();
	

	// Initialize the fonts
	int ctx = 0;
	int font = 0;
	ctx = glcGenContext();
	glcContext(ctx);
	font = glcNewFontFromFamily(glcGenFontID(), "Verdana");
	glcFont(font);
	glcStringType(GLC_UTF8_QSO);
// 	glcFontFace(font, "Italic");


/*
	this->widget=draw;
	
	this->colormap=gdk_colormap_new(gdk_visual_get_system(),FALSE);
	gtk_widget_set_events(draw, GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_POINTER_MOTION_MASK|GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(draw), "expose_event", G_CALLBACK(expose), this); 
        g_signal_connect(G_OBJECT(draw), "configure_event", G_CALLBACK(configure), this);
#if 0
        g_signal_connect(G_OBJECT(draw), "realize_event", G_CALLBACK(realize), co);
#endif
	g_signal_connect(G_OBJECT(draw), "button_press_event", G_CALLBACK(button_press), this);
	g_signal_connect(G_OBJECT(draw), "button_release_event", G_CALLBACK(button_release), this);
	g_signal_connect(G_OBJECT(draw), "scroll_event", G_CALLBACK(scroll), this);
	g_signal_connect(G_OBJECT(draw), "motion_notify_event", G_CALLBACK(motion_notify), this);
	*/
	return this;
}

void
plugin_init(void)
{
        plugin_register_graphics_type("opengl", graphics_opengl_new);
}
