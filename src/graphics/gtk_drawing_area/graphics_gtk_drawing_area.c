#define GDK_ENABLE_BROKEN
#include <gtk/gtk.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "point.h"
#include "coord.h"
#include "transform.h"
#include "graphics.h"
#include "statusbar.h"
#include "popup.h"
#include "container.h"

struct graphics_gra {
	GdkEventButton button_event;
	int button_timeout;
	GtkWidget *widget;
	GdkDrawable *drawable;
	GdkDrawable *background;
	GdkColormap *colormap;
	FT_Library library;
	struct point p;
	int width;
	int height;
	int library_init;
	int visible;
	struct graphics_gra *parent;
	struct graphics_gra *overlays;
	struct graphics_gra *next;
	enum draw_mode_num mode;
};

struct graphics_font {
        FT_Face face;
};

struct graphics_gc {
	GdkGC *gc;
	struct graphics_gra *gra;
};

struct graphics_image_gra {
	GdkPixbuf *pixbuf;
};


char *fontlist[]={
	"/usr/X11R6/lib/X11/fonts/msttcorefonts/arial.ttf",
	"/usr/X11R6/lib/X11/fonts/truetype/arial.ttf",
	"/usr/share/fonts/truetype/msttcorefonts/arial.ttf",
	NULL,
};


struct graphics * graphics_gtk_drawing_area_new(struct container *co, GtkWidget **widget);

static struct graphics_font *font_new(struct graphics *gr, int size)
{
	char **filename=fontlist;
	struct graphics_font *font=g_new(struct graphics_font, 1);
	if (!gr->gra->library_init) {
		FT_Init_FreeType( &gr->gra->library );
		gr->gra->library_init=1;
	}

	while (*filename) {	
	    	if (!FT_New_Face( gr->gra->library, *filename, 0, &font->face ))
			break;
		filename++;
	}
	if (! *filename) {
		g_warning("Failed to load font, no labelling");
		g_free(font);
		return NULL;
	}
        FT_Set_Char_Size(font->face, 0, size, 300, 300);
	return font;
}

static struct graphics_gc *gc_new(struct graphics *gr)
{
	struct graphics_gc *gc=g_new(struct graphics_gc, 1);

	gc->gc=gdk_gc_new(gr->gra->widget->window);
	gc->gra=gr->gra;
	return gc;
}

static void
gc_set_linewidth(struct graphics_gc *gc, int w)
{
	gdk_gc_set_line_attributes(gc->gc, w, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
}

static void
gc_set_color(struct graphics_gc *gc, int r, int g, int b, int fg)
{
	GdkColor c;
	c.pixel=0;
	c.red=r;
	c.green=g;
	c.blue=b;
	gdk_colormap_alloc_color(gc->gra->colormap, &c, FALSE, TRUE);
	if (fg)
		gdk_gc_set_foreground(gc->gc, &c);
	else
		gdk_gc_set_background(gc->gc, &c);
}

static void
gc_set_foreground(struct graphics_gc *gc, int r, int g, int b)
{
	gc_set_color(gc, r, g, b, 1);
}

static void
gc_set_background(struct graphics_gc *gc, int r, int g, int b)
{
	gc_set_color(gc, r, g, b, 0);
}

static struct graphics_image *
image_new(struct graphics *gr, char *name)
{
	GdkPixbuf *pixbuf;
	struct graphics_image *ret;

	pixbuf=gdk_pixbuf_new_from_file(name, NULL);
	if (! pixbuf)
		return NULL;
	ret=g_new0(struct graphics_image, 1);
	ret->gr=gr;
	ret->name=strdup(name);
	ret->gra=g_new0(struct graphics_image_gra, 1);
	ret->gra->pixbuf=pixbuf;
	ret->width=gdk_pixbuf_get_width(ret->gra->pixbuf);
	ret->height=gdk_pixbuf_get_height(ret->gra->pixbuf);
	return ret;
}

static void
draw_lines(struct graphics *gr, struct graphics_gc *gc, struct point *p, int count)
{
	if (gr->gra->mode == draw_mode_begin || gr->gra->mode == draw_mode_end)
		gdk_draw_lines(gr->gra->drawable, gc->gc, (GdkPoint *)p, count);
	if (gr->gra->mode == draw_mode_end || gr->gra->mode == draw_mode_cursor)
		gdk_draw_lines(gr->gra->widget->window, gc->gc, (GdkPoint *)p, count);
}

static void
draw_polygon(struct graphics *gr, struct graphics_gc *gc, struct point *p, int count)
{
	if (gr->gra->mode == draw_mode_begin || gr->gra->mode == draw_mode_end)
		gdk_draw_polygon(gr->gra->drawable, gc->gc, TRUE, (GdkPoint *)p, count);
	if (gr->gra->mode == draw_mode_end || gr->gra->mode == draw_mode_cursor)
		gdk_draw_polygon(gr->gra->widget->window, gc->gc, TRUE, (GdkPoint *)p, count);
}

static void
draw_rectangle(struct graphics *gr, struct graphics_gc *gc, struct point *p, int w, int h)
{
	gdk_draw_rectangle(gr->gra->drawable, gc->gc, TRUE, p->x, p->y, w, h);
}

static void
draw_circle(struct graphics *gr, struct graphics_gc *gc, struct point *p, int r)
{
	if (gr->gra->mode == draw_mode_begin || gr->gra->mode == draw_mode_end)
		gdk_draw_arc(gr->gra->drawable, gc->gc, FALSE, p->x-r/2, p->y-r/2, r, r, 0, 64*360);
	if (gr->gra->mode == draw_mode_end || gr->gra->mode == draw_mode_cursor)
		gdk_draw_arc(gr->gra->widget->window, gc->gc, FALSE, p->x-r/2, p->y-r/2, r, r, 0, 64*360);
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

	shadow=malloc(str*(g->h+2));
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
display_text_render(char *text, struct graphics_font *font, int dx, int dy, int x, int y)
{
       	FT_GlyphSlot  slot = font->face->glyph;  // a small shortcut
	FT_Matrix matrix;
	FT_Vector pen;
	FT_UInt  glyph_index;
	int n,len=strlen(text);
	struct text_render *ret=malloc(sizeof(*ret)+len*sizeof(struct text_glyph *));
	struct text_glyph *curr;

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

		glyph_index = FT_Get_Char_Index(font->face, text[n]);
		FT_Load_Glyph(font->face, glyph_index, FT_LOAD_DEFAULT );
		FT_Render_Glyph(font->face->glyph, ft_render_mode_normal );
        
		curr=malloc(sizeof(*curr)+slot->bitmap.rows*slot->bitmap.pitch);
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
	}
	return ret;
}

static void
display_text_draw(struct text_render *text, struct graphics *gr, struct graphics_gc *fg, struct graphics_gc *bg)
{
	int i;
	struct text_glyph *g, **gp;

	gp=text->glyph;
	i=text->glyph_count;
	while (i-- > 0)
	{
		g=*gp++;
		if (g->shadow && bg) 
			gdk_draw_image(gr->gra->drawable, bg->gc, g->shadow, 0, 0, g->x-1, g->y-1, g->w+2, g->h+2);
	}
	gp=text->glyph;
	i=text->glyph_count;
	while (i-- > 0)
	{
		g=*gp++;
		if (g->w && g->h) 
			gdk_draw_gray_image(gr->gra->drawable, fg->gc, g->x, g->y, g->w, g->h, GDK_RGB_DITHER_NONE, g->pixmap, g->w);
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
			gdk_image_destroy((*gp)->shadow);
#if 0
			free((*gp)->shadow->mem);
#endif
		}
		free(*gp++);
	}
	free(text);
}

static void
draw_text(struct graphics *gr, struct graphics_gc *fg, struct graphics_gc *bg, struct graphics_font *font, char *text, struct point *p, int dx, int dy)
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
draw_image(struct graphics *gr, struct graphics_gc *fg, struct point *p, struct graphics_image *img)
{
	printf("draw_image1 \n");
	gdk_draw_pixbuf(gr->gra->drawable, fg->gc, img->gra->pixbuf, 0, 0, p->x, p->y,
		    img->width, img->height, GDK_RGB_DITHER_NONE, 0, 0);
	printf("draw_image1 end\n");
}

static void
overlay_draw(struct graphics_gra *parent, struct graphics_gra *overlay, int window)
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
}

static void
draw_restore(struct graphics *gr, struct point *p, int w, int h)
{
	struct graphics_gra *gra=gr->gra;
	GtkWidget *widget=gra->widget;
	gdk_draw_pixmap(widget->window,
                        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                        gra->drawable,
                        p->x, p->y, p->x, p->y, w, h);

}

static void
draw_mode(struct graphics *gr, enum draw_mode_num mode)
{
	struct graphics_gra *gra=gr->gra;
	struct graphics_gra *overlay;
	GtkWidget *widget=gra->widget;

	if (mode == draw_mode_begin) {
		if (! gra->parent)
			gdk_draw_rectangle(gr->gra->drawable, gr->gc[0]->gc, TRUE, 0, 0, gr->gra->width, gr->gra->height);
	}
	if (mode == draw_mode_end && gr->gra->mode == draw_mode_begin) {
		if (gra->parent) {
			overlay_draw(gra->parent, gra, 1);
		} else {
			overlay=gra->overlays;
			while (overlay) {
				overlay_draw(gra, overlay, 0);
				overlay=overlay->next;
			}
			gdk_draw_pixmap(widget->window,
                	        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                	        gra->drawable,
                	        0, 0, 0, 0, gra->width, gra->height);
		}
	}
	gr->gra->mode=mode;
}

/* Events */

static gint
configure(GtkWidget * widget, GdkEventConfigure * event, gpointer user_data)
{
	struct container *co=user_data;
	struct graphics_gra *gra=co->gra->gra;

	if (! gra->visible)
		return TRUE;
	if (gra->drawable != NULL) {
                gdk_pixmap_unref(gra->drawable);
        }
	gra->width=widget->allocation.width;
	gra->height=widget->allocation.height;
        gra->drawable = gdk_pixmap_new(widget->window, gra->width, gra->height, -1);
	graphics_resize(co, gra->width, gra->height);
	return TRUE;
}

static gint
expose(GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
	struct container *co=user_data;
	struct graphics *gr=co->gra;

	gr->gra->visible=1;
	if (! gr->gra->drawable)
		configure(widget, NULL, user_data);
        gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                        gr->gra->drawable, event->area.x, event->area.y,
                        event->area.x, event->area.y,
                        event->area.width, event->area.height);


	return FALSE;
}

static gint
button_timeout(gpointer user_data)
{
	struct container *co=user_data;
	int x=co->gra->gra->button_event.x; 
	int y=co->gra->gra->button_event.y; 
	int button=co->gra->gra->button_event.button;

	co->gra->gra->button_timeout=0;
	popup(co, x, y, button);

	return FALSE;
}

static gint
button_press(GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
	struct container *co=user_data;
	int x=event->x;
	int y=event->y;
	int button=event->button;
	int border=16;
	long map_x,map_y,x_new,y_new;
	unsigned long scale;

	if (button == 3)
		popup(co, x, y, button);
	if (button == 1) {
		graphics_get_view(co, &map_x, &map_y, &scale);
		if (x < border) {
			x_new=map_x-co->trans->width*scale/32;	
			graphics_set_view(co, &x_new, NULL, NULL);
		} else if (x >= co->trans->width-border) {
			x_new=map_x+co->trans->width*scale/32;
			graphics_set_view(co, &x_new, NULL, NULL);
		} else if (y < border) {
			y_new=map_y+co->trans->height*scale/32;
			graphics_set_view(co, NULL, &y_new, NULL);
		} else if (y >= co->trans->height-border) {
			y_new=map_y-co->trans->height*scale/32;
			graphics_set_view(co, NULL, &y_new, NULL);
		} else {
			co->gra->gra->button_event=*event;
			co->gra->gra->button_timeout=g_timeout_add(500, button_timeout, co);
		}
	}			
	return FALSE;
}

static gint
button_release(GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
	struct container *co=user_data;
	if (co->gra->gra->button_timeout)
		g_source_remove(co->gra->gra->button_timeout);
	return FALSE;
}

static gint
motion_notify(GtkWidget * widget, GdkEventMotion * event, gpointer user_data)
{
	struct container *co=user_data;
	struct point p;

	if (co->statusbar && co->statusbar->statusbar_mouse_update) {
		p.x=event->x;
		p.y=event->y;
		co->statusbar->statusbar_mouse_update(co->statusbar, co->trans, &p);
	}
	return FALSE;
}

static struct graphics *graphics_new(void);

static struct graphics *
overlay_new(struct graphics *gr, struct point *p, int w, int h)
{
	struct graphics *this=graphics_new();
	this->gra->drawable=gdk_pixmap_new(gr->gra->widget->window, w, h, -1);
	this->gra->colormap=gr->gra->colormap;
	this->gra->widget=gr->gra->widget;
	this->gra->p=*p;
	this->gra->width=w;
	this->gra->height=h;
	this->gra->parent=gr->gra;
	this->gra->background=gdk_pixmap_new(gr->gra->widget->window, w, h, -1);
	this->gra->next=gr->gra->overlays;
	gr->gra->overlays=this->gra;
	return this;
}



static struct graphics *
graphics_new(void)
{
	struct graphics *this=g_new0(struct graphics,1);
	this->draw_mode=draw_mode;
	this->draw_lines=draw_lines;
	this->draw_polygon=draw_polygon;
	this->draw_rectangle=draw_rectangle;
	this->draw_circle=draw_circle;
	this->draw_text=draw_text;
	this->draw_image=draw_image;
	this->draw_restore=draw_restore;
	this->gc_new=gc_new;
	this->gc_set_linewidth=gc_set_linewidth;
	this->gc_set_foreground=gc_set_foreground;
	this->gc_set_background=gc_set_background;
	this->font_new=font_new;
	this->image_new=image_new;
	this->overlay_new=overlay_new;
	this->gra=g_new0(struct graphics_gra, 1);

	return this;
}

struct graphics *
graphics_gtk_drawing_area_new(struct container *co, GtkWidget **widget)
{
	GtkWidget *draw;

	draw=gtk_drawing_area_new();
	struct graphics *this=graphics_new();
	this->gra->widget=draw;
	this->gra->colormap=gdk_colormap_new(gdk_visual_get_system(),FALSE);
	gtk_widget_set_events(draw, GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_POINTER_MOTION_MASK);
	gtk_signal_connect(GTK_OBJECT(draw), "expose_event", GTK_SIGNAL_FUNC(expose), co); 
        gtk_signal_connect(GTK_OBJECT(draw), "configure_event", GTK_SIGNAL_FUNC(configure), co);
#if 0
        gtk_signal_connect(GTK_OBJECT(draw), "realize_event", GTK_SIGNAL_FUNC(realize), co);
#endif
	gtk_signal_connect(GTK_OBJECT(draw), "button_press_event", GTK_SIGNAL_FUNC(button_press), co);
	gtk_signal_connect(GTK_OBJECT(draw), "button_release_event", GTK_SIGNAL_FUNC(button_release), co);
	gtk_signal_connect(GTK_OBJECT(draw), "motion_notify_event", GTK_SIGNAL_FUNC(motion_notify), co);
	*widget=draw;
	return this;
}

