#define GDK_ENABLE_BROKEN
#include "config.h"
#include <gtk/gtk.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#ifdef HAVE_IMLIB2
#include <Imlib2.h>
#endif
#include <gdk/gdkx.h>
#include "debug.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"

struct graphics_priv {
	GdkEventButton button_event;
	int button_timeout;
	GtkWidget *widget;
	GdkDrawable *drawable;
	GdkDrawable *background;
	int background_ready;
	GdkColormap *colormap;
	FT_Library library;
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
};

struct graphics_font_priv {
        FT_Face face;
};

struct graphics_gc_priv {
	GdkGC *gc;
	struct graphics_priv *gr;
};

struct graphics_image_priv {
	GdkPixbuf *pixbuf;
	int w;
	int h;
};

static void
graphics_destroy(struct graphics_priv *gr)
{
}

static char *fontpaths[]={
	"/usr/share/fonts",
	"/usr/X11R6/lib/X11/fonts/msttcorefonts",
	"/usr/X11R6/lib/X11/fonts/truetype",
	"/usr/X11R6/lib/X11/fonts/TTF",
	"/usr/share/fonts/truetype",
	"/usr/share/fonts/truetype/msttcorefonts",
	"/usr/share/fonts/ttf",
	"/usr/share/fonts/corefonts",
	NULL,
};

#if 0
static char *fontlist_bd[]={
	"/usr/X11R6/lib/X11/fonts/msttcorefonts/arialbd.ttf",
	"/usr/X11R6/lib/X11/fonts/truetype/arialbd.ttf",
	"/usr/share/fonts/truetype/msttcorefonts/arialbd.ttf",
	"/usr/share/fonts/ttf/arialbd.ttf",
	"/usr/share/fonts/corefonts/arialbd.ttf",
	NULL,
};
#endif



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
	char **filename=fontpaths;
	char fontpath [256];

	struct graphics_font_priv *font=g_new(struct graphics_font_priv, 1);

	*meth=font_methods;

	if (!gr->library_init) {
		FT_Init_FreeType( &gr->library );
		gr->library_init=1;
	}
	while (*filename) {
		// Trying the Liberation font first
		sprintf(fontpath,"%s/LiberationMono-Regular.ttf",*filename);
		dbg(1,"font : %s\n",fontpath);
	    	if (!FT_New_Face( gr->library, fontpath, 0, &font->face ))
			break;
		//Fallback to arial.ttf, in the same path
		sprintf(fontpath,"%s/arial.ttf",*filename);
		dbg(1,"font : %s\n",fontpath);
	    	if (!FT_New_Face( gr->library, fontpath, 0, &font->face ))
			break;
		sprintf(fontpath,"%s/NcrBI4nh.ttf",*filename);
		dbg(1,"font : %s\n",fontpath);
	    	if (!FT_New_Face( gr->library, fontpath, 0, &font->face ))
			break;
		sprintf(fontpath,"%s/luximbi.ttf",*filename);
		dbg(1,"font : %s\n",fontpath);
	    	if (!FT_New_Face( gr->library, fontpath, 0, &font->face ))
			break;
		sprintf(fontpath,"%s/DejaVuSans.ttf",*filename);
		dbg(1,"font : %s\n",fontpath);
	    	if (!FT_New_Face( gr->library, fontpath, 0, &font->face ))
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
}

static void
gc_destroy(struct graphics_gc_priv *gc)
{
	g_object_unref(gc->gc);
	g_free(gc);
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
	gdk_gc_set_line_attributes(gc->gc, w, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, unsigned char *dash_list, int n)
{
	gdk_gc_set_dashes(gc->gc, 0, (gint8 *)dash_list, n);
	gdk_gc_set_line_attributes(gc->gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_ROUND, GDK_JOIN_ROUND);
}

static void
gc_set_color(struct graphics_gc_priv *gc, struct color *c, int fg)
{
	GdkColor gdkc;
	gdkc.pixel=0;
	gdkc.red=c->r;
	gdkc.green=c->g;
	gdkc.blue=c->b;
	gdk_colormap_alloc_color(gc->gr->colormap, &gdkc, FALSE, TRUE);
	if (fg)
		gdk_gc_set_foreground(gc->gc, &gdkc);
	else
		gdk_gc_set_background(gc->gc, &gdkc);
}

static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
	gc_set_color(gc, c, 1);
}

static void
gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
	gc_set_color(gc, c, 0);
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
	gc->gc=gdk_gc_new(gr->widget->window);
	gc->gr=gr;
	return gc;
}


static struct graphics_image_priv *
image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name, int *w, int *h, struct point *hot)
{
	GdkPixbuf *pixbuf;
	struct graphics_image_priv *ret;
	const char *option;

	pixbuf=gdk_pixbuf_new_from_file(name, NULL);
	if (! pixbuf)
		return NULL;
	ret=g_new0(struct graphics_image_priv, 1);
	ret->pixbuf=pixbuf;
	ret->w=gdk_pixbuf_get_width(pixbuf);
	ret->h=gdk_pixbuf_get_height(pixbuf);
	*w=ret->w;
	*h=ret->h;
	if (hot) {
		option=gdk_pixbuf_get_option(pixbuf, "x_hot");
		if (option) 
			hot->x=atoi(option);
		else
			hot->x=ret->w/2-1;
		option=gdk_pixbuf_get_option(pixbuf, "y_hot");
		if (option) 
			hot->y=atoi(option);
		else
			hot->y=ret->h/2-1;
	}
	return ret;
}

static void
draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	if (gr->mode == draw_mode_begin || gr->mode == draw_mode_end) 
		gdk_draw_lines(gr->drawable, gc->gc, (GdkPoint *)p, count);
	if (gr->mode == draw_mode_end || gr->mode == draw_mode_cursor)
		gdk_draw_lines(gr->widget->window, gc->gc, (GdkPoint *)p, count);
}

static void
draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	if (gr->mode == draw_mode_begin || gr->mode == draw_mode_end)
		gdk_draw_polygon(gr->drawable, gc->gc, TRUE, (GdkPoint *)p, count);
	if (gr->mode == draw_mode_end || gr->mode == draw_mode_cursor)
		gdk_draw_polygon(gr->widget->window, gc->gc, TRUE, (GdkPoint *)p, count);
}

static void
draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
	gdk_draw_rectangle(gr->drawable, gc->gc, TRUE, p->x, p->y, w, h);
}

static void
draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
	if (gr->mode == draw_mode_begin || gr->mode == draw_mode_end)
		gdk_draw_arc(gr->drawable, gc->gc, FALSE, p->x-r/2, p->y-r/2, r, r, 0, 64*360);
	if (gr->mode == draw_mode_end || gr->mode == draw_mode_cursor)
		gdk_draw_arc(gr->widget->window, gc->gc, FALSE, p->x-r/2, p->y-r/2, r, r, 0, 64*360);
}


struct text_glyph {
	int x,y,w,h;
	GdkImage *shadow;
	unsigned char pixmap[0];		
};

struct text_render {
	int x1,y1;
	int x2,y2;
	int x3,y3;
	int x4,y4;
	int glyph_count;
	struct text_glyph *glyph[0];
};

static GdkImage *
display_text_render_shadow(struct text_glyph *g)
{
	int mask0, mask1, mask2, x, y, w=g->w, h=g->h;
	int str=(g->w+9)/8;
	unsigned char *shadow;
	unsigned char *p, *pm=g->pixmap;
	GdkImage *ret;

	shadow=malloc(str*(g->h+2)); /* do not use g_malloc() here */
	memset(shadow, 0, str*(g->h+2));
	for (y = 0 ; y < h ; y++) {	
		p=shadow+str*y;
		mask0=0x4000;
		mask1=0xe000;
		mask2=0x4000;
		for (x = 0 ; x < w ; x++) {
			if (pm[x+y*w]) {
				p[0]|=(mask0 >> 8);
				if (mask0 & 0xff) 
					p[1]|=mask0;

				p[str]|=(mask1 >> 8);
				if (mask1 & 0xff) 
					p[str+1]|=mask1;
				p[str*2]|=(mask2 >> 8);
				if (mask2 & 0xff) 
					p[str*2+1]|=mask2;
			}
			mask0 >>= 1;
			mask1 >>= 1;
			mask2 >>= 1;
			if (!((mask0 >> 8) | (mask1 >> 8) | (mask2 >> 8))) {
				mask0<<=8;
				mask1<<=8;
				mask2<<=8;
				p++;
			}
		}
	}
	ret=gdk_image_new_bitmap(gdk_visual_get_system(), shadow, g->w+2, g->h+2);
	return ret;
}

static struct text_render *
display_text_render(char *text, struct graphics_font_priv *font, int dx, int dy, int x, int y)
{
       	FT_GlyphSlot  slot = font->face->glyph;  // a small shortcut
	FT_Matrix matrix;
	FT_Vector pen;
	FT_UInt  glyph_index;
	int n,len;
	struct text_render *ret;
	struct text_glyph *curr;
	char *p=text;

	len=g_utf8_strlen(text, -1);
	ret=g_malloc(sizeof(*ret)+len*sizeof(struct text_glyph *));
	ret->glyph_count=len;

	matrix.xx = dx;
	matrix.xy = dy;
	matrix.yx = -dy;
	matrix.yy = dx;

	pen.x = 0 * 64;
	pen.y = 0 * 64;
	x <<= 6;
	y <<= 6;
	FT_Set_Transform( font->face, &matrix, &pen );
	


	for ( n = 0; n < len; n++ )
	{

		glyph_index = FT_Get_Char_Index(font->face, g_utf8_get_char(p));
		FT_Load_Glyph(font->face, glyph_index, FT_LOAD_DEFAULT );
		FT_Render_Glyph(font->face->glyph, ft_render_mode_normal );
        
		curr=g_malloc(sizeof(*curr)+slot->bitmap.rows*slot->bitmap.pitch);
		ret->glyph[n]=curr;

		curr->x=(x>>6)+slot->bitmap_left;
		curr->y=(y>>6)-slot->bitmap_top;
		curr->w=slot->bitmap.width;
		curr->h=slot->bitmap.rows;
		if (slot->bitmap.width && slot->bitmap.rows) {
			memcpy(curr->pixmap, slot->bitmap.buffer, slot->bitmap.rows*slot->bitmap.pitch);
			curr->shadow=display_text_render_shadow(curr);
		}
		else 
			curr->shadow=NULL;
#if 0
		printf("height=%d\n", slot->metrics.height);	
		printf("height2=%d\n", face->height);	
		printf("bbox %d %d %d %d\n", face->bbox.xMin, face->bbox.yMin, face->bbox.xMax, face->bbox.yMax);
#endif
         	x += slot->advance.x;
         	y -= slot->advance.y;
		p=g_utf8_next_char(p);
	}
	return ret;
}

static void
display_text_draw(struct text_render *text, struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg)
{
	int i;
	struct text_glyph *g, **gp;

	gp=text->glyph;
	i=text->glyph_count;
	while (i-- > 0)
	{
		g=*gp++;
		if (g->shadow && bg) 
			gdk_draw_image(gr->drawable, bg->gc, g->shadow, 0, 0, g->x-1, g->y-1, g->w+2, g->h+2);
	}
	gp=text->glyph;
	i=text->glyph_count;
	while (i-- > 0)
	{
		g=*gp++;
		if (g->w && g->h) 
			gdk_draw_gray_image(gr->drawable, fg->gc, g->x, g->y, g->w, g->h, GDK_RGB_DITHER_NONE, g->pixmap, g->w);
	}
}

static void
display_text_free(struct text_render *text)
{
	int i;
	struct text_glyph **gp;

	gp=text->glyph;
	i=text->glyph_count;
	while (i-- > 0) {
		if ((*gp)->shadow) {
			g_object_unref((*gp)->shadow);
		}
		g_free(*gp++);
	}
	g_free(text);
}

static void
draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
	struct text_render *t;

	if (! font)
		return;
	if (bg) {
		gdk_gc_set_function(fg->gc, GDK_AND_INVERT);
		gdk_gc_set_function(bg->gc, GDK_OR);
	}

	t=display_text_render(text, font, dx, dy, p->x, p->y);
	display_text_draw(t, gr, fg, bg);
	display_text_free(t);
	if (bg) {
		gdk_gc_set_function(fg->gc, GDK_COPY);
        	gdk_gc_set_function(bg->gc, GDK_COPY);
	}
}

static void
draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
	gdk_draw_pixbuf(gr->drawable, fg->gc, img->pixbuf, 0, 0, p->x, p->y,
		    img->w, img->h, GDK_RGB_DITHER_NONE, 0, 0);
}

#ifdef HAVE_IMLIB2
static void
draw_image_warp(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, int count, char *data)
{
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
}
#endif

static void
overlay_draw(struct graphics_priv *parent, struct graphics_priv *overlay, int window)
{
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
	if (window) {
		if (overlay->background_ready)
			gdk_draw_drawable(parent->drawable, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], overlay->background, 0, 0, overlay->p.x, overlay->p.y, overlay->width, overlay->height);
	}
	else {
		gdk_draw_drawable(overlay->background, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], parent->drawable, overlay->p.x, overlay->p.y, 0, 0, overlay->width, overlay->height);
		overlay->background_ready=1;
	}
	gdk_draw_pixbuf(parent->drawable, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], pixbuf2, 0, 0, overlay->p.x, overlay->p.y, overlay->width, overlay->height, GDK_RGB_DITHER_NONE, 0, 0);
	if (window)
		gdk_draw_drawable(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], parent->drawable, overlay->p.x, overlay->p.y, overlay->p.x, overlay->p.y, overlay->width, overlay->height);
#if 0
	gdk_draw_drawable(gr->gra->drawable,
                        gr->gra->widget->style->fg_gc[GTK_WIDGET_STATE(gr->gra->widget)],
                        img->gra->drawable,
                        0, 0, p->x, p->y, img->gra->width, img->gra->height);
#endif
}

static void
draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
	GtkWidget *widget=gr->widget;
	gdk_draw_drawable(widget->window,
                        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                        gr->drawable,
                        p->x, p->y, p->x, p->y, w, h);

}

static void
background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
	gr->background_gc=gc;
}

static void
draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
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
			gdk_draw_drawable(widget->window,
                	        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                	        gr->drawable,
                	        0, 0, 0, 0, gr->width, gr->height);
		}
	}
	gr->mode=mode;
}

/* Events */

static gint
configure(GtkWidget * widget, GdkEventConfigure * event, gpointer user_data)
{
	struct graphics_priv *gra=user_data;
	if (! gra->visible)
		return TRUE;
	if (gra->drawable != NULL) {
                g_object_unref(gra->drawable);
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
        gdk_draw_drawable(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
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

static struct graphics_priv *graphics_gtk_drawing_area_new_helper(struct graphics_methods *meth);

static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h)
{
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
}

static void *
get_data(struct graphics_priv *this, char *type)
{
	if (strcmp(type,"gtk_widget"))
		return NULL;
	return this->widget;
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
};

static struct graphics_priv *
graphics_gtk_drawing_area_new_helper(struct graphics_methods *meth)
{
	struct graphics_priv *this=g_new0(struct graphics_priv,1);
	*meth=graphics_methods;

	return this;
}

static struct graphics_priv *
graphics_gtk_drawing_area_new(struct graphics_methods *meth, struct attr **attrs)
{
	GtkWidget *draw;

	draw=gtk_drawing_area_new();
	struct graphics_priv *this=graphics_gtk_drawing_area_new_helper(meth);
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
	return this;
}

void
plugin_init(void)
{
        plugin_register_graphics_type("gtk_drawing_area", graphics_gtk_drawing_area_new);
}
