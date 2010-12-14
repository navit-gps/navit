/* graphics_sdl.c -- barebones sdl graphics layer

   copyright (c) 2008 bryan rittmeyer <bryanr@bryanr.org>
   license: GPLv2

   TODO:
   - dashed lines
   - ifdef DEBUG -> dbg()
   - proper image transparency (libsdl-image xpm does not work)
   - valgrind

   revision history:
   2008-06-01 initial
   2008-06-15 SDL_DOUBLEBUF+SDL_Flip for linux fb. fix FT leaks.
   2008-06-18 defer initial resize_callback
   2008-06-21 linux touchscreen
   2008-07-04 custom rastering
*/

#include <glib.h>
#include "config.h"
#include "debug.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"
#include "window.h"
#include "navit.h"
#include "keys.h"
#include "item.h"
#include "attr.h"
#include "callback.h"
#include "navit/font/freetype/font_freetype.h"

#include <SDL/SDL.h>
#include <math.h>

#define RASTER
#undef SDL_SGE
#undef SDL_GFX
#undef ALPHA

#undef SDL_TTF
#define SDL_IMAGE
#undef LINUX_TOUCHSCREEN

#define DISPLAY_W 800
#define DISPLAY_H 600


#undef DEBUG
#undef PROFILE

#define OVERLAY_MAX 16

#ifdef RASTER
#include "raster.h"
#endif

#ifdef SDL_SGE
#include <SDL/sge.h>
#endif

#ifdef SDL_GFX
#include <SDL/SDL_gfxPrimitives.h>
#endif

#ifdef SDL_TTF
#include <SDL/SDL_ttf.h>
#else
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftglyph.h>
#endif
#include <event.h>

#ifdef SDL_IMAGE
#include <SDL/SDL_image.h>
#endif

#ifdef LINUX_TOUCHSCREEN
/* we use Linux evdev directly for the touchscreen.  */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#endif

#include <alloca.h>

#ifdef PROFILE
#include <sys/time.h>
#include <time.h>
#endif


/* TODO: union overlay + non-overlay to reduce size */
struct graphics_priv;
struct graphics_priv {
    SDL_Surface *screen;
    int aa;
    /* video modes */
    uint32_t video_flags;
    int video_bpp;

    /* <overlay> */
    int overlay_mode;
    int overlay_x;
    int overlay_y;
    struct graphics_priv *overlay_parent;
    int overlay_idx;
    /* </overlay> */

    /* <main> */
    struct graphics_priv *overlay_array[OVERLAY_MAX];
    int overlay_enable;
    enum draw_mode_num draw_mode;

	int resize_callback_initial;

    struct navit *nav;
    struct callback_list *cbl;

#ifdef LINUX_TOUCHSCREEN
    int ts_fd;
    int32_t ts_hit;
    uint32_t ts_x;
    uint32_t ts_y;
#endif

#ifndef SDL_TTF
    FT_Library library;
#endif

#ifdef PROFILE
    struct timeval draw_begin_tv;
    unsigned long draw_time_peak;
#endif
  struct font_freetype_methods freetype_methods;
    /* </main> */
};

static int dummy;


struct graphics_font_priv {
#ifdef SDL_TTF
    TTF_Font *font;
#else
    FT_Face face;
#endif
};

struct graphics_gc_priv {
    struct graphics_priv *gr;
    Uint8 fore_r;
    Uint8 fore_g;
    Uint8 fore_b;
    Uint8 fore_a;
    Uint8 back_r;
    Uint8 back_g;
    Uint8 back_b;
    Uint8 back_a;
    int linewidth;
};

struct graphics_image_priv {
    SDL_Surface *img;
};


#ifdef LINUX_TOUCHSCREEN
static int input_ts_exit(struct graphics_priv *gr);
#endif

static void
graphics_destroy(struct graphics_priv *gr)
{
    dbg(0, "graphics_destroy %p %u\n", gr, gr->overlay_mode);

    if(gr->overlay_mode)
    {
        SDL_FreeSurface(gr->screen);
        gr->overlay_parent->overlay_array[gr->overlay_idx] = NULL;
    }
    else
    {
#ifdef SDL_TTF
        TTF_Quit();
#else
        FT_Done_FreeType(gr->library);
        FcFini();
#endif
#ifdef LINUX_TOUCHSCREEN
        input_ts_exit(gr);
#endif
        SDL_Quit();
    }

    g_free(gr);
}

/* graphics_font */
static char *fontfamilies[]={
	"Liberation Mono",
	"Arial",
	"DejaVu Sans",
	"NcrBI4nh",
	"luximbi",
	"FreeSans",
	NULL,
};

static void font_destroy(struct graphics_font_priv *gf)
{
#ifdef SDL_TTF
#else
    FT_Done_Face(gf->face);
#endif
    g_free(gf);
}

static struct graphics_font_methods font_methods = {
	font_destroy
};

/* graphics_gc */

static void
gc_destroy(struct graphics_gc_priv *gc)
{
    g_free(gc);
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
#ifdef DEBUG
    printf("gc_set_linewidth %p %d\n", gc, w);
#endif
    gc->linewidth = w;
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n)
{
    /* TODO */
}

static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
#ifdef DEBUG
    printf("gc_set_foreground: %p %d %d %d %d\n", gc, c->a, c->r, c->g, c->b);
#endif
    gc->fore_r = c->r/256;
    gc->fore_g = c->g/256;
    gc->fore_b = c->b/256;
    gc->fore_a = c->a/256;
}

static void
gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
#ifdef DEBUG
    printf("gc_set_background: %p %d %d %d %d\n", gc, c->a, c->r, c->g, c->b);
#endif
    gc->back_r = c->r/256;
    gc->back_g = c->g/256;
    gc->back_b = c->b/256;
    gc->back_a = c->a/256;
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
    struct graphics_gc_priv *gc=g_new0(struct graphics_gc_priv, 1);
	*meth=gc_methods;
    gc->gr=gr;
    gc->linewidth=1; /* upper layer should override anyway? */
	return gc;
}


#if 0 /* unused by core? */
static void image_destroy(struct graphics_image_priv *gi)
{
#ifdef SDL_IMAGE
    SDL_FreeSurface(gi->img);
    g_free(gi);
#endif
}

static struct graphics_image_methods gi_methods =
{
    image_destroy
};
#endif

static struct graphics_image_priv *
image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name, int *w, int *h,
          struct point *hot, int rotation)
{
#ifdef SDL_IMAGE
    struct graphics_image_priv *gi;

    /* FIXME: meth is not used yet.. so gi leaks. at least xpm is small */

    gi = g_new0(struct graphics_image_priv, 1);
    gi->img = IMG_Load(name);
    if(gi->img)
    {
        /* TBD: improves blit performance? */
#if !SDL_VERSION_ATLEAST(1,3,0)
        SDL_SetColorKey(gi->img, SDL_RLEACCEL, gi->img->format->colorkey);
#endif
        *w=gi->img->w;
        *h=gi->img->h;
        hot->x=*w/2;
        hot->y=*h/2;
    }
    else
    {
        /* TODO: debug "colour parse errors" on xpm */
        printf("graphics_sdl: image_new on '%s' failed: %s\n", name, IMG_GetError());
        g_free(gi);
        gi = NULL;
    }

    return gi;
#else
    return NULL;
#endif
}

static void
image_free(struct graphics_priv *gr, struct graphics_image_priv * gi)
{
#ifdef SDL_IMAGE
    SDL_FreeSurface(gi->img);
    g_free(gi);
#endif
}

static void
get_text_bbox(struct graphics_priv *gr, struct graphics_font_priv *font, char *text, int dx, int dy, struct point *ret, int estimate)
{
	char *p=text;
	FT_BBox bbox;
	FT_UInt  glyph_index;
       	FT_GlyphSlot  slot = font->face->glyph;  // a small shortcut
	FT_Glyph glyph;
	FT_Matrix matrix;
	FT_Vector pen;
	pen.x = 0 * 64;
	pen.y = 0 * 64;
	matrix.xx = dx;
	matrix.xy = dy;
	matrix.yx = -dy;
	matrix.yy = dx;
	int n,len,x=0,y=0;

	bbox.xMin = bbox.yMin = 32000;
	bbox.xMax = bbox.yMax = -32000; 
	FT_Set_Transform( font->face, &matrix, &pen );
	len=g_utf8_strlen(text, -1);
	for ( n = 0; n < len; n++ ) {
		FT_BBox glyph_bbox;
		glyph_index = FT_Get_Char_Index(font->face, g_utf8_get_char(p));
		p=g_utf8_next_char(p);
		FT_Load_Glyph(font->face, glyph_index, FT_LOAD_DEFAULT );
		FT_Get_Glyph(font->face->glyph, &glyph);
		FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_pixels, &glyph_bbox );
		FT_Done_Glyph(glyph);
		glyph_bbox.xMin += x >> 6;
		glyph_bbox.xMax += x >> 6;
		glyph_bbox.yMin += y >> 6;
		glyph_bbox.yMax += y >> 6;
         	x += slot->advance.x;
         	y -= slot->advance.y;
		if ( glyph_bbox.xMin < bbox.xMin )
			bbox.xMin = glyph_bbox.xMin;
		if ( glyph_bbox.yMin < bbox.yMin )
			bbox.yMin = glyph_bbox.yMin;
		if ( glyph_bbox.xMax > bbox.xMax )
			bbox.xMax = glyph_bbox.xMax;
		if ( glyph_bbox.yMax > bbox.yMax )
			bbox.yMax = glyph_bbox.yMax;
	} 
	if ( bbox.xMin > bbox.xMax ) {
		bbox.xMin = 0;
		bbox.yMin = 0;
		bbox.xMax = 0;
		bbox.yMax = 0;
	}
	ret[0].x=bbox.xMin;
	ret[0].y=-bbox.yMin;
	ret[1].x=bbox.xMin;
	ret[1].y=-bbox.yMax;
	ret[2].x=bbox.xMax;
	ret[2].y=-bbox.yMax;
	ret[3].x=bbox.xMax;
	ret[3].y=-bbox.yMin;
}

static void
draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
  if ((gr->overlay_parent && !gr->overlay_parent->overlay_enable) || (gr->overlay_parent && gr->overlay_parent->overlay_enable && !gr->overlay_enable) )
    {
      return;
    }

    Sint16 *vx, *vy;
    Sint16 x, y;
    int i;

    vx = alloca(count * sizeof(Sint16));
    vy = alloca(count * sizeof(Sint16));

    for(i = 0; i < count; i++)
    {
        x = (Sint16)p[i].x;
        y = (Sint16)p[i].y;

#if 0
        if(x < 0)
        {
            x = 0;
        }
        if(y < 0)
        {
            y = 0;
        }
#endif

        vx[i] = x;
        vy[i] = y;

#ifdef DEBUG
        printf("draw_polygon: %p %i %d,%d\n", gc, i, p[i].x, p[i].y);
#endif
    }

#ifdef RASTER
    if(gr->aa)
    {
        raster_aapolygon(gr->screen, count, vx, vy,
                       SDL_MapRGBA(gr->screen->format,
                                  gc->fore_r,
                                  gc->fore_g,
                                  gc->fore_b,
                                  gc->fore_a));
    }
    else
    {
        raster_polygon(gr->screen, count, vx, vy,
                       SDL_MapRGBA(gr->screen->format,
                                  gc->fore_r,
                                  gc->fore_g,
                                  gc->fore_b,
                                  gc->fore_a));
    }
#else
#ifdef SDL_SGE
#ifdef ALPHA
    sge_FilledPolygonAlpha(gr->screen, count, vx, vy,
                           SDL_MapRGB(gr->screen->format,
                                      gc->fore_r,
                                      gc->fore_g,
                                      gc->fore_b),
                           gc->fore_a);
#else
#ifdef ANTI_ALIAS
    sge_AAFilledPolygon(gr->screen, count, vx, vy,
                           SDL_MapRGB(gr->screen->format,
                                      gc->fore_r,
                                      gc->fore_g,
                                      gc->fore_b));
#else
    sge_FilledPolygon(gr->screen, count, vx, vy,
                           SDL_MapRGB(gr->screen->format,
                                      gc->fore_r,
                                      gc->fore_g,
                                      gc->fore_b));
#endif
#endif
#else
    filledPolygonRGBA(gr->screen, vx, vy, count,
                      gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#endif
#endif
}



static void
draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
  if ((gr->overlay_parent && !gr->overlay_parent->overlay_enable) || (gr->overlay_parent && gr->overlay_parent->overlay_enable && !gr->overlay_enable) )
    {
      return;
    }

#ifdef DEBUG
        printf("draw_rectangle: %d %d %d %d r=%d g=%d b=%d a=%d\n", p->x, p->y, w, h,
               gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#endif
    if(w > gr->screen->w)
    {
        w = gr->screen->w;
    }
    if(h > gr->screen->h)
    {
        h = gr->screen->h;
    }

#ifdef RASTER
    raster_rect(gr->screen, p->x, p->y, w, h,
                SDL_MapRGBA(gr->screen->format,
                           gc->fore_r,
                           gc->fore_g,
                           gc->fore_b,
                           gc->fore_a));
#else
#ifdef SDL_SGE
#ifdef ALPHA
    sge_FilledRectAlpha(gr->screen, p->x, p->y, p->x + w, p->y + h,
                        SDL_MapRGB(gr->screen->format,
                                   gc->fore_r,
                                   gc->fore_g,
                                   gc->fore_b),
                        gc->fore_a);
#else
    /* no AA -- should use poly instead for that */
    sge_FilledRect(gr->screen, p->x, p->y, p->x + w, p->y + h,
                        SDL_MapRGB(gr->screen->format,
                                   gc->fore_r,
                                   gc->fore_g,
                                   gc->fore_b));
#endif
#else
    boxRGBA(gr->screen, p->x, p->y, p->x + w, p->y + h,
            gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#endif
#endif

}

static void
draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
  if ((gr->overlay_parent && !gr->overlay_parent->overlay_enable) || (gr->overlay_parent && gr->overlay_parent->overlay_enable && !gr->overlay_enable) )
    {
      return;
    }

#if 0
        if(gc->fore_a != 0xff)
        {
        dbg(0, "%d %d %d %u %u:%u:%u:%u\n", p->x, p->y, r, gc->linewidth,
            gc->fore_a, gc->fore_r, gc->fore_g, gc->fore_b);
        }
#endif

    /* FIXME: does not quite match gtk */

    /* hack for osd compass.. why is this needed!? */
    if(gr->overlay_mode)
    {
        r = r / 2;
    }

#ifdef RASTER
        if(gr->aa)
        {
            raster_aacircle(gr->screen, p->x, p->y, r,
                            SDL_MapRGBA(gr->screen->format,
                                       gc->fore_r,
                                       gc->fore_g,
                                       gc->fore_b,
                                       gc->fore_a));
        }
        else
        {
            raster_circle(gr->screen, p->x, p->y, r,
                          SDL_MapRGBA(gr->screen->format,
                                     gc->fore_r,
                                     gc->fore_g,
                                     gc->fore_b,
                                     gc->fore_a));
        }
#else
#ifdef SDL_SGE
#ifdef ALPHA
        sge_FilledCircleAlpha(gr->screen, p->x, p->y, r,
                         SDL_MapRGB(gr->screen->format,
                                    gc->fore_r, gc->fore_g, gc->fore_b),
                         gc->fore_a);
#else
#ifdef ANTI_ALIAS
        sge_AAFilledCircle(gr->screen, p->x, p->y, r,
                         SDL_MapRGB(gr->screen->format,
                                    gc->fore_r, gc->fore_g, gc->fore_b));
#else
        sge_FilledCircle(gr->screen, p->x, p->y, r,
                         SDL_MapRGB(gr->screen->format,
                                    gc->fore_r, gc->fore_g, gc->fore_b));
#endif
#endif
#else
#ifdef ANTI_ALIAS
        aacircleRGBA(gr->screen, p->x, p->y, r,
                   gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#else
        filledCircleRGBA(gr->screen, p->x, p->y, r,
                         gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#endif
#endif
#endif
}


static void
draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
  if ((gr->overlay_parent && !gr->overlay_parent->overlay_enable) || (gr->overlay_parent && gr->overlay_parent->overlay_enable && !gr->overlay_enable) )
    {
      return;
    }

    /* you might expect lines to be simpler than the other shapes.
       but, that would be wrong. 1 line can generate 1 polygon + 2 circles
       and even worse, we have to calculate their parameters!
       go dust off your trigonometry hat.
    */
#if 0
    int i, l, x_inc, y_inc, lw;

    lw = gc->linewidth;

    for(i = 0; i < count-1; i++)
    {
#ifdef DEBUG
        printf("draw_lines: %p %d %d,%d->%d,%d %d\n", gc, i, p[i].x, p[i].y, p[i+1].x, p[i+1].y, gc->linewidth);
#endif
        for(l = 0; l < lw; l++)
        {
            /* FIXME: center? */
#if 1
            if(p[i].x != p[i+1].x)
            {
                x_inc = l - (lw/2);
            }
            else
            {
                x_inc = 0;
            }

            if(p[i].y != p[i+1].y)
            {
                y_inc = l - (lw/2);
            }
            else
            {
                y_inc = 0;
            }
#else
            x_inc = 0;
            y_inc = 0;
#endif

#ifdef ANTI_ALIAS
            aalineRGBA(gr->screen, p[i].x + x_inc, p[i].y + y_inc, p[i+1].x + x_inc, p[i+1].y + y_inc,
                     gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#else
            lineRGBA(gr->screen, p[i].x + x_inc, p[i].y + y_inc, p[i+1].x + x_inc, p[i+1].y + y_inc,
                     gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#endif
        }
    }
#else
    /* sort of based on graphics_opengl.c::draw_lines */
    /* FIXME: should honor ./configure flag for no fp.
              this could be 100% integer code pretty easily,
              except that i am lazy
    */
    struct point vert[4];
    int lw = gc->linewidth;
    //int lw = 1;
    int i;

    for(i = 0; i < count-1; i++)
    {
		float dx=p[i+1].x-p[i].x;
		float dy=p[i+1].y-p[i].y;

#if 0
		float cx=(p[i+1].x+p[i].x)/2;
		float cy=(p[i+1].y+p[i].y)/2;
#endif

        float angle;

        int x_lw_adj, y_lw_adj;

        if(lw == 1)
        {
#ifdef RASTER
            if(gr->aa)
            {
                raster_aaline(gr->screen, p[i].x, p[i].y, p[i+1].x, p[i+1].y,
                         SDL_MapRGBA(gr->screen->format,
                                    gc->fore_r,
                                    gc->fore_g,
                                    gc->fore_b,
                                    gc->fore_a));
            }
            else
            {
                raster_line(gr->screen, p[i].x, p[i].y, p[i+1].x, p[i+1].y,
                         SDL_MapRGBA(gr->screen->format,
                                    gc->fore_r,
                                    gc->fore_g,
                                    gc->fore_b,
                                    gc->fore_a));
            }
#else
#ifdef SDL_SGE
#ifdef ALPHA
            sge_LineAlpha(gr->screen, p[i].x, p[i].y, p[i+1].x, p[i+1].y,
                     SDL_MapRGB(gr->screen->format,
                                gc->fore_r, gc->fore_g, gc->fore_b),
                     gc->fore_a);
#else
#ifdef ANTI_ALIAS
            sge_AALine(gr->screen, p[i].x, p[i].y, p[i+1].x, p[i+1].y,
                     SDL_MapRGB(gr->screen->format,
                                gc->fore_r, gc->fore_g, gc->fore_b));
#else
            sge_Line(gr->screen, p[i].x, p[i].y, p[i+1].x, p[i+1].y,
                     SDL_MapRGB(gr->screen->format,
                                gc->fore_r, gc->fore_g, gc->fore_b));
#endif
#endif
#else
#ifdef ANTI_ALIAS
            aalineRGBA(gr->screen, p[i].x, p[i].y, p[i+1].x, p[i+1].y,
                     gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#else
            lineRGBA(gr->screen, p[i].x, p[i].y, p[i+1].x, p[i+1].y,
                     gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#endif
#endif
#endif
        }
        else
        {
            /* there is probably a much simpler way but this works ok */

            /* FIXME: float + double mixture */
            /* FIXME: lrint(round())? */
            if(dy == 0.0)
            {
                angle = 0.0;
                x_lw_adj = 0;
                y_lw_adj = round((float)lw/2.0);
            }
            else if(dx == 0.0)
            {
                angle = 0.0;
                x_lw_adj = round((float)lw/2.0);
                y_lw_adj = 0;
            }
            else
            {
                angle = (M_PI/2.0) - atan(abs(dx)/abs(dy));
                x_lw_adj = round(sin(angle)*(float)lw/2.0);
                y_lw_adj = round(cos(angle)*(float)lw/2.0);
                if((x_lw_adj < 0) || (y_lw_adj < 0))
                {
                    printf("i=%d\n", i);
                    printf("   %d,%d->%d,%d\n", p[i].x, p[i].y, p[i+1].x, p[i+1].y);
                    printf("   lw=%d angle=%f\n", lw, 180.0 * angle / M_PI);
                    printf("   x_lw_adj=%d y_lw_adj=%d\n", x_lw_adj, y_lw_adj);
                }
            }

            if(p[i+1].x > p[i].x)
            {
                x_lw_adj = -x_lw_adj;
            }
            if(p[i+1].y > p[i].y)
            {
                y_lw_adj = -y_lw_adj;
            }

#if 0
            if(((y_lw_adj*y_lw_adj)+(x_lw_adj*x_lw_adj)) != (lw/2)*(lw/2))
            {
                printf("i=%d\n", i);
                printf("   %d,%d->%d,%d\n", p[i].x, p[i].y, p[i+1].x, p[i+1].y);
                printf("   lw=%d angle=%f\n", lw, 180.0 * angle / M_PI);
                printf("   x_lw_adj=%d y_lw_adj=%d\n", x_lw_adj, y_lw_adj);
            }
#endif

            /* FIXME: draw a circle/square if p[i]==p[i+1]? */
            /* FIXME: clipping, check for neg values. hoping sdl-gfx does this */
            vert[0].x = p[i].x + x_lw_adj;
            vert[0].y = p[i].y - y_lw_adj;
            vert[1].x = p[i].x - x_lw_adj;
            vert[1].y = p[i].y + y_lw_adj;
            vert[2].x = p[i+1].x - x_lw_adj;
            vert[2].y = p[i+1].y + y_lw_adj;
            vert[3].x = p[i+1].x + x_lw_adj;
            vert[3].y = p[i+1].y - y_lw_adj;

            draw_polygon(gr, gc, vert, 4);

            /* draw small circles at the ends. this looks better than nothing, and slightly
             * better than the triangle used by graphics_opengl, but is more expensive.
             * should have an ifdef/xml attr?
            */

            /* FIXME: should just draw a half circle */

            /* now some circular endcaps, if the width is over 2 */ 
            if(lw > 2)
            {
                if(i == 0)
                {
                    draw_circle(gr, gc, &p[i], lw/2);
                }
                /* we truncate on the divide on purpose, so we don't go outside the line */
                draw_circle(gr, gc, &p[i+1], lw/2);
            }
        }
    }
#endif
}
static void
display_text_draw(struct font_freetype_text *text,
		  struct graphics_priv *gr, struct graphics_gc_priv *fg,
		  struct graphics_gc_priv *bg, int color, struct point *p)
{
    int i, x, y, stride;
    struct font_freetype_glyph *g, **gp;
    unsigned char *shadow, *glyph;
    struct color transparent = { 0x0000, 0x0000, 0x0000, 0x0000 };
    struct color black =
	{ fg->fore_r * 255, fg->fore_g * 255, fg->fore_b * 255, fg->fore_a * 255
    };
    struct color white = { 0xffff, 0xffff, 0xffff, 0xffff };

    if (bg) {
	if (COLOR_IS_WHITE(black) && COLOR_IS_BLACK(white)) {
	    black.r = 65535;
	    black.g = 65535;
	    black.b = 65535;
	    black.a = 65535;

	    white.r = 0;
	    white.g = 0;
	    white.b = 0;
	    white.a = 65535;
	} else if (COLOR_IS_BLACK(black) && COLOR_IS_WHITE(white)) {
	    white.r = 65535;
	    white.g = 65535;
	    white.b = 65535;
	    white.a = 65535;

	    black.r = 0;
	    black.g = 0;
	    black.b = 0;
	    black.a = 65535;
	} else {
	    white.r = bg->fore_r * 255;
	    white.g = bg->fore_g * 255;
	    white.b = bg->fore_b * 255;
	    white.a = bg->fore_a * 255;
	}
    } else {
	white.r = 0;
	white.g = 0;
	white.b = 0;
	white.a = 0;
    }


    gp = text->glyph;
    i = text->glyph_count;
    x = p->x << 6;
    y = p->y << 6;
    while (i-- > 0) {
	g = *gp++;
	if (g->w && g->h && bg) {
	    stride = (g->w + 2) * 4;
	    if (color) {
		shadow = g_malloc(stride * (g->h + 2));
		gr->freetype_methods.get_shadow(g, shadow, 32, stride, &white, &transparent);

		SDL_Surface *glyph_surface =
		    SDL_CreateRGBSurfaceFrom(shadow, g->w + 2, g->h + 2,
					     32,
					     stride, 
					     0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		if (glyph_surface) {
		    SDL_Rect r;
		    r.x = (x + g->x) >> 6;
		    r.y = (y + g->y) >> 6;
		    r.w = g->w + 2;
		    r.h = g->h + 2;

		    SDL_BlitSurface(glyph_surface, NULL, gr->screen, &r);
		    SDL_FreeSurface(glyph_surface);
		}
		g_free(shadow);
	    }
	}
	x += g->dx;
	y += g->dy;
    }

    x = p->x << 6;
    y = p->y << 6;
    gp = text->glyph;
    i = text->glyph_count;
    while (i-- > 0) {
	g = *gp++;
	if (g->w && g->h) {
	    if (color) {
		stride = g->w;
		if (bg) {
		    glyph = g_malloc(stride * g->h * 4);
		    gr->freetype_methods.get_glyph(g, glyph, 32,
						   stride * 4, &black,
						   &white, &transparent);
		    SDL_Surface *glyph_surface =
			SDL_CreateRGBSurfaceFrom(glyph, g->w, g->h, 32,
						 stride * 4, 
						 0x000000ff,0x0000ff00, 0x00ff0000,0xff000000);
		    if (glyph_surface) {
			SDL_Rect r;
			r.x = (x + g->x) >> 6;
			r.y = (y + g->y) >> 6;
			r.w = g->w;
			r.h = g->h;

			SDL_BlitSurface(glyph_surface, NULL, gr->screen,&r);
		        SDL_FreeSurface(glyph_surface);
		    }
		    g_free(glyph);
		}
		stride *= 4;
		glyph = g_malloc(stride * g->h);
		gr->freetype_methods.get_glyph(g, glyph, 32, stride,
					       &black, &white,
					       &transparent);
		//convert ABGR to RGBA
		int j;
		unsigned char *pGlyph = glyph;
		for (j = 0; j < g->w * g->h; ++j) {
		    unsigned char tmp;
		    tmp = *(pGlyph + 0);
		    *(pGlyph + 0) = *(pGlyph + 2);
		    *(pGlyph + 2) = tmp;
		    pGlyph += 4;
		}
		SDL_Surface *glyph_surface =
		    SDL_CreateRGBSurfaceFrom(glyph, g->w, g->h, 32, 
		    				stride,
					     	0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
		int ii, jj;
		pGlyph = glyph;
		for (jj = 0; jj < g->h; ++jj) {
		    for (ii = 0; ii < g->w; ++ii) {
			int sx = (x + g->x) >> 6;
			int sy = (y + g->y) >> 6;
			sx = sx < 0 ? 0 : sx;
			sy = sy < 0 ? 0 : sy;
			int poff =
			    gr->screen->w * ((jj) + sy) + ((ii) + sx);
			poff *= gr->screen->format->BytesPerPixel;
			if(*(pGlyph + 3)>10) {  // display only non transparent pixels
                    *(Uint16 *)((Uint8*)gr->screen->pixels + poff) = 
                        SDL_MapRGBA(gr->screen->format,
                                     *(pGlyph + 0),
                                     *(pGlyph + 1),
                                     *(pGlyph + 2),
                                     *(pGlyph + 3));
			}

		    pGlyph += 4;
		    }
		}
		g_free(glyph);
	    }
	}
	x += g->dx;
	y += g->dy;
    }
}

static void
draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg,
	  struct graphics_gc_priv *bg, struct graphics_font_priv *font,
	  char *text, struct point *p, int dx, int dy)
{
    if ((gr->overlay_parent && !gr->overlay_parent->overlay_enable)
	|| (gr->overlay_parent && gr->overlay_parent->overlay_enable
	    && !gr->overlay_enable)) {
	return;
    }

    struct font_freetype_text *t;
    int color = 1;

    if (!font) {
	dbg(0, "no font, returning\n");
	return;
    }
    t = gr->freetype_methods.text_new(text,
				      (struct font_freetype_font *) font,
				      dx, dy);

    struct point p_eff;
    p_eff.x = p->x;
    p_eff.y = p->y;

    display_text_draw(t, gr, fg, bg, color, &p_eff);
    gr->freetype_methods.text_destroy(t);
}

static void
draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
  if ((gr->overlay_parent && !gr->overlay_parent->overlay_enable) || (gr->overlay_parent && gr->overlay_parent->overlay_enable && !gr->overlay_enable) )
    {
      return;
    }

#ifdef SDL_IMAGE
    SDL_Rect r;

    r.x = p->x;
    r.y = p->y;
    r.w = img->img->w;
    r.h = img->img->h;

    SDL_BlitSurface(img->img, NULL, gr->screen, &r);
#endif
}

static void
draw_image_warp(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, int count, char *data)
{
    /* TODO */
}

static void
draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
#ifdef DEBUG
    printf("draw_restore\n");
#endif
}

static void
background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
#ifdef DEBUG
    printf("background_gc\n");
#endif
}


static void
draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
#ifdef PROFILE
    struct timeval now;
    unsigned long elapsed;
#endif
    struct graphics_priv *ov;
    SDL_Rect rect;
    int i;

    if(gr->overlay_mode)
    {
        /* will be drawn below */
    }
    else
    {
#ifdef DEBUG
        printf("draw_mode: %d\n", mode);
#endif

#ifdef PROFILE
        if(mode == draw_mode_begin)
        {
            gettimeofday(&gr->draw_begin_tv, NULL);
        }
#endif

        if(mode == draw_mode_end)
        {
            if((gr->draw_mode == draw_mode_begin) && gr->overlay_enable)
            {
                for(i = 0; i < OVERLAY_MAX; i++)
                {
                    ov = gr->overlay_array[i];
                    if(ov && ov->overlay_enable)
                    {
                        rect.x = ov->overlay_x;
                        if(rect.x<0) rect.x += gr->screen->w;
                        rect.y = ov->overlay_y;
                        if(rect.y<0) rect.y += gr->screen->h;
                        rect.w = ov->screen->w;
                        rect.h = ov->screen->h;
                        SDL_BlitSurface(ov->screen, NULL,
                                        gr->screen, &rect);
                    }
                }
            }

            SDL_Flip(gr->screen);

#ifdef PROFILE
            gettimeofday(&now, NULL);
            elapsed = 1000000 * (now.tv_sec - gr->draw_begin_tv.tv_sec);
            elapsed += (now.tv_usec - gr->draw_begin_tv.tv_usec);
            if(elapsed >= gr->draw_time_peak)
            {
               dbg(0, "draw elapsed %u usec\n", elapsed);
               gr->draw_time_peak = elapsed;
            }
#endif
        }

        gr->draw_mode = mode;
    }
}

static void overlay_disable(struct graphics_priv *gr, int disable)
{
    gr->overlay_enable = !disable;
    struct graphics_priv *curr_gr = gr;
    if(gr->overlay_parent) {
      curr_gr = gr->overlay_parent;
    }
    draw_mode(curr_gr,draw_mode_end);
}

static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int alpha, int wraparound);

static int window_fullscreen(struct window *win, int on)
{
	struct graphics_priv *gr=(struct graphics_priv *)win->priv;

	/* Update video flags */
	if(on) {
		gr->video_flags |= SDL_FULLSCREEN;
	} else {
		gr->video_flags &= ~SDL_FULLSCREEN;
	}

	/* Update video mode */
	gr->screen = SDL_SetVideoMode(gr->screen->w, gr->screen->h, gr->video_bpp, gr->video_flags);
	if(gr->screen == NULL) {
		navit_destroy(gr->nav);
	} 
	else {
		callback_list_call_attr_2(gr->cbl, attr_resize, (void *)gr->screen->w, (void *)gr->screen->h);
	}
	return 1;
}

static void *
get_data(struct graphics_priv *this, char *type)
{
	if(strcmp(type, "window") == 0) {
		struct window *win;
		win=g_new(struct window, 1);
		win->priv=this;
		win->fullscreen=window_fullscreen;
		win->disable_suspend=NULL;
		return win;
	} else {
		return &dummy;
	}
}

static void draw_drag(struct graphics_priv *gr, struct point *p)
{
	if(p) {
	    gr->overlay_x = p->x;
	    gr->overlay_y = p->y;
	}
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
	draw_drag,
	NULL,
	gc_new,
	background_gc,
	overlay_new,
	image_new,
	get_data,
//	register_resize_callback,
//	register_button_callback,
//	register_motion_callback,
	image_free,
	get_text_bbox,
	overlay_disable,
//	register_keypress_callback
};

static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h,int alpha, int wraparound)
{
	struct graphics_priv *ov;
	Uint32 rmask, gmask, bmask, amask;
	int i;

	for(i = 0; i < OVERLAY_MAX; i++)
	{
		if(gr->overlay_array[i] == NULL)
		{
			break;
		}
	}
	if(i == OVERLAY_MAX)
	{
		dbg(0, "too many overlays! increase OVERLAY_MAX\n");
		return NULL;
	}

	dbg(1, "overlay_new %d %d %d %u %u (%x, %x, %x ,%x, %d)\n", i,
			p->x,
			p->y,
			w,
			h,
			gr->screen->format->Rmask,
			gr->screen->format->Gmask,
			gr->screen->format->Bmask,
			gr->screen->format->Amask,
			gr->screen->format->BitsPerPixel
	   );

	ov = g_new0(struct graphics_priv, 1);

	switch(gr->screen->format->BitsPerPixel) {
	case 8:
		rmask = 0xc0;
		gmask = 0x30;
		bmask = 0x0c;
		amask = 0x03;
		break;
	case 16:
		rmask = 0xf000;
		gmask = 0x0f00;
		bmask = 0x00f0;
		amask = 0x000f;
		break;
	case 32:
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
		break;
	default:
		rmask = gr->screen->format->Rmask;
		gmask = gr->screen->format->Gmask;
		bmask = gr->screen->format->Bmask;
		amask = gr->screen->format->Amask;
	}

	ov->screen = SDL_CreateRGBSurface(SDL_SWSURFACE, 
			w, h,
			gr->screen->format->BitsPerPixel,
			rmask, gmask, bmask, amask);

	ov->overlay_mode = 1;
	ov->overlay_enable = 1;
	ov->overlay_x = p->x;
	ov->overlay_y = p->y;
	ov->overlay_parent = gr;
	ov->overlay_idx = i;
	gr->overlay_array[i] = ov;


  struct font_priv *(*font_freetype_new) (void *meth);
  font_freetype_new = plugin_get_font_type ("freetype");

  if (!font_freetype_new)
    {
      return NULL;
    }


  font_freetype_new (&ov->freetype_methods);

	*meth=graphics_methods;

  meth->font_new =
    (struct graphics_font_priv *
     (*)(struct graphics_priv *, struct graphics_font_methods *, char *, int,
	 int)) ov->freetype_methods.font_new;
  meth->get_text_bbox = ov->freetype_methods.get_text_bbox;




	return ov;
}


#ifdef LINUX_TOUCHSCREEN

#define EVFN "/dev/input/eventX"

static int input_ts_init(struct graphics_priv *gr)
{
    struct input_id ii;
    char fn[32];
#if 0
    char name[64];
#endif
    int n, fd, ret;

    gr->ts_fd = -1;
    gr->ts_hit = -1;
    gr->ts_x = 0;
    gr->ts_y = 0;

    strcpy(fn, EVFN);
    n = 0;
    while(1)
    {
        fn[sizeof(EVFN)-2] = '0' + n;

        fd = open(fn, O_RDONLY);
        if(fd >= 0)
        {
#if 0
            ret = ioctl(fd, EVIOCGNAME(64), (void *)name);
            if(ret > 0)
            {
                printf("input_ts: %s\n", name);
            }
#endif

            ret = ioctl(fd, EVIOCGID, (void *)&ii);
            if(ret == 0)
            {
#if 1
                printf("bustype %04x vendor %04x product %04x version %04x\n",
                       ii.bustype,
                       ii.vendor,
                       ii.product,
                       ii.version);
#endif

                if((ii.bustype == BUS_USB) &&
                   (ii.vendor == 0x0eef) &&
                   (ii.product == 0x0001))
                {
                    ret = fcntl(fd, F_SETFL, O_NONBLOCK);
                    if(ret == 0)
                    {
                        gr->ts_fd = fd;
                    }
                    else
                    {
                        close(fd);
                    }

                    break;
                }
            }

            close(fd);
        }

        n = n + 1;

        /* FIXME: should check all 32 minors */
        if(n == 10)
        {
            /* not found */
            ret = -1;
            break;
        }
    }

    return ret;
}


/* returns 0-based display coordinate for the given ts coord */
static void input_ts_map(int *disp_x, int *disp_y,
                         uint32_t ts_x, uint32_t ts_y)
{
    /* Dynamix 7" (eGalax TS)
       top left  = 1986,103
       top right =   61,114
       bot left  = 1986,1897
       bot right =   63,1872

       calibrate your TS using input_event_dump
       and touching all four corners. use the most extreme values. 
    */

#define INPUT_TS_LEFT 1978
#define INPUT_TS_RIGHT  48
#define INPUT_TS_TOP   115
#define INPUT_TS_BOT  1870

    /* clamp first */
    if(ts_x > INPUT_TS_LEFT)
    {
        ts_x = INPUT_TS_LEFT;
    }
    if(ts_x < INPUT_TS_RIGHT)
    {
        ts_x = INPUT_TS_RIGHT;
    }

    ts_x = ts_x - INPUT_TS_RIGHT;

    *disp_x = ((DISPLAY_W-1) * ts_x) / (INPUT_TS_LEFT - INPUT_TS_RIGHT);
    *disp_x = (DISPLAY_W-1) - *disp_x;


    if(ts_y > INPUT_TS_BOT)
    {
        ts_y = INPUT_TS_BOT;
    }
    if(ts_y < INPUT_TS_TOP)
    {
        ts_y = INPUT_TS_TOP;
    }

    ts_y = ts_y - INPUT_TS_TOP;

    *disp_y = ((DISPLAY_H-1) * ts_y) / (INPUT_TS_BOT - INPUT_TS_TOP);
/*  *disp_y = (DISPLAY_H-1) - *disp_y; */
}

#if 0
static void input_event_dump(struct input_event *ie)
{
    printf("input_event:\n"
           "\ttv_sec\t%u\n"
           "\ttv_usec\t%lu\n"
           "\ttype\t%u\n"
           "\tcode\t%u\n"
           "\tvalue\t%d\n",
           (unsigned int)ie->time.tv_sec,
           ie->time.tv_usec,
           ie->type,
           ie->code,
           ie->value);
}
#endif

static int input_ts_exit(struct graphics_priv *gr)
{
    close(gr->ts_fd);
    gr->ts_fd = -1;

    return 0;
}
#endif


static gboolean graphics_sdl_idle(void *data)
{
    struct graphics_priv *gr = (struct graphics_priv *)data;
    struct point p;
    SDL_Event ev;
#ifdef LINUX_TOUCHSCREEN
    struct input_event ie;
    ssize_t ss;
#endif
    int ret, key;
    char keybuf[2];

    /* generate the initial resize callback, so the gui knows W/H

       its unsafe to do this directly inside register_resize_callback;
       graphics_gtk does it during Configure, but SDL does not have
       an equivalent event, so we use our own flag
    */
    if(gr->resize_callback_initial != 0)
    {
        callback_list_call_attr_2(gr->cbl, attr_resize, (void *)gr->screen->w, (void *)gr->screen->h);
        gr->resize_callback_initial = 0;
    }

#ifdef LINUX_TOUCHSCREEN
    if(gr->ts_fd >= 0)
    {
        ss = read(gr->ts_fd, (void *)&ie, sizeof(ie));
        if(ss == sizeof(ie))
        {
            /* we (usually) get three events on a touchscreen hit:
              1: type =EV_KEY
                 code =330 [BTN_TOUCH]
                 value=1

              2: type =EV_ABS
                 code =0 [X]
                 value=X pos

              3: type =EV_ABS
                 code =1 [Y]
                 value=Y pos

              4: type =EV_SYN

              once hit, if the contact point changes, we'll get more 
              EV_ABS (for 1 or both axes), followed by an EV_SYN.

              and, on a lift:

              5: type =EV_KEY
                 code =330 [BTN_TOUCH]
                 value=0

              6: type =EV_SYN
            */
            switch(ie.type)
            {
                case EV_KEY:
                {
                    if(ie.code == BTN_TOUCH)
                    {
                        gr->ts_hit = ie.value;
                    }

                    break;
                }

                case EV_ABS:
                {
                    if(ie.code == 0)
                    {
                        gr->ts_x = ie.value;
                    }
                    else if(ie.code == 1)
                    {
                        gr->ts_y = ie.value;
                    }

                    break;
                }

                case EV_SYN:
                {
                    input_ts_map(&p.x, &p.y, gr->ts_x, gr->ts_y);

                    /* always send MOUSE_MOTION (first) */
		    callback_list_call_attr_1(gr->cbl, attr_motion, (void *)&p); 
                    if(gr->ts_hit > 0)
                    {
		        callback_list_call_attr_3(gr->cbl, attr_button, (void *)1, (void *)SDL_BUTTON_LEFT, (void *)&p); 
                    }
                    else if(gr->ts_hit == 0)
                    {
		        callback_list_call_attr_3(gr->cbl, attr_button, (void *)0, (void *)SDL_BUTTON_LEFT, (void *)&p); 
                    }

                    /* reset ts_hit */
                    gr->ts_hit = -1;

                    break;
                }

                default:
                {
                    break;
                }
            }
        }
    }
#endif

    while(1)
    {
        ret = SDL_PollEvent(&ev);
        if(ret == 0)
        {
            break;
        }

        switch(ev.type)
        {
            case SDL_MOUSEMOTION:
            {
                p.x = ev.motion.x;
                p.y = ev.motion.y;
		callback_list_call_attr_1(gr->cbl, attr_motion, (void *)&p); 
                break;
            }

            case SDL_KEYDOWN:
            {
                switch(ev.key.keysym.sym)
                {
                    case SDLK_LEFT:
                    {
                        key = NAVIT_KEY_LEFT;
                        break;
                    }
                    case SDLK_RIGHT:
                    {
                        key = NAVIT_KEY_RIGHT;
                        break;
                    }
                    case SDLK_BACKSPACE:
                    {
                        key = NAVIT_KEY_BACKSPACE;
                        break;
                    }
                    case SDLK_RETURN:
                    {
                        key = NAVIT_KEY_RETURN;
                        break;
                    }
                    case SDLK_DOWN:
                    {
                        key = NAVIT_KEY_DOWN;
                        break;
                    }
                    case SDLK_PAGEUP:
                    {
                        key = NAVIT_KEY_ZOOM_OUT;
                        break;
                    }
                    case SDLK_UP:
                    {
                        key = NAVIT_KEY_UP;
                        break;
                    }
                    case SDLK_PAGEDOWN:
                    {
                        key = NAVIT_KEY_ZOOM_IN;
                        break;
                    }
                    default:
                    {
                        /* return unicode chars when they can be converted to ascii */
                        key = ev.key.keysym.unicode<=127 ? ev.key.keysym.unicode : 0;
                        break;
                    }
                }

                keybuf[0] = key;
                keybuf[1] = '\0';
		callback_list_call_attr_1(gr->cbl, attr_keypress, (void *)keybuf);

                break;
            }

            case SDL_KEYUP:
            {
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
            {
#ifdef DEBUG
                printf("SDL_MOUSEBUTTONDOWN %d %d %d %d %d\n",
                       ev.button.which,
                       ev.button.button,
                       ev.button.state,
                       ev.button.x,
                       ev.button.y);
#endif

                p.x = ev.button.x;
                p.y = ev.button.y;
		callback_list_call_attr_3(gr->cbl, attr_button, (void *)1, (void *)(int)ev.button.button, (void *)&p); 
                break;
            }

            case SDL_MOUSEBUTTONUP:
            {
#ifdef DEBUG
                printf("SDL_MOUSEBUTTONUP %d %d %d %d %d\n",
                       ev.button.which,
                       ev.button.button,
                       ev.button.state,
                       ev.button.x,
                       ev.button.y);
#endif

                p.x = ev.button.x;
                p.y = ev.button.y;
		callback_list_call_attr_3(gr->cbl, attr_button, (void *)0, (void *)(int)ev.button.button, (void *)&p); 
                break;
            }

            case SDL_QUIT:
            {
                navit_destroy(gr->nav);
                break;
            }

            case SDL_VIDEORESIZE:
            {

                gr->screen = SDL_SetVideoMode(ev.resize.w, ev.resize.h, gr->video_bpp, gr->video_flags);
                if(gr->screen == NULL)
                {
                    navit_destroy(gr->nav);
                }
                else
                {
		    callback_list_call_attr_2(gr->cbl, attr_resize, (void *)gr->screen->w, (void *)gr->screen->h);
                }

                break;
            }

            default:
            {
#ifdef DEBUG
                printf("SDL_Event %d\n", ev.type);
#endif
                break;
            }
        }
    }

    return TRUE;
}


static struct graphics_priv *
graphics_sdl_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
    struct graphics_priv *this=g_new0(struct graphics_priv, 1);
    struct attr *attr;
    int ret;
    int w=DISPLAY_W,h=DISPLAY_H;

    struct font_priv *(*font_freetype_new) (void *meth);
    font_freetype_new = plugin_get_font_type ("freetype");

    if (!font_freetype_new)
      {
        return NULL;
      }

    font_freetype_new (&this->freetype_methods);

    this->nav = nav;
    this->cbl = cbl;

    ret = SDL_Init(SDL_INIT_VIDEO);
    if(ret < 0)
    {
        g_free(this);
        return NULL;
    }

    if (! event_request_system("glib","graphics_sdl_new"))
        return NULL;

    this->video_bpp = 16;
    this->video_flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE;

    if ((attr=attr_search(attrs, NULL, attr_w)))
        w=attr->u.num;
    if ((attr=attr_search(attrs, NULL, attr_h)))
        h=attr->u.num;
    if ((attr=attr_search(attrs, NULL, attr_bpp)))
        this->video_bpp=attr->u.num;
    if ((attr=attr_search(attrs, NULL, attr_flags))) {
	if (attr->u.num & 1) 
	    this->video_flags = SDL_SWSURFACE;
    }
    if ((attr=attr_search(attrs, NULL, attr_frame))) {
        if(!attr->u.num)
            this->video_flags |= SDL_NOFRAME;
    }

    this->screen = SDL_SetVideoMode(w, h, this->video_bpp, this->video_flags);

    /* Use screen size instead of requested */
    w = this->screen->w;
    h = this->screen->h;

    if(this->screen == NULL)
    {
        g_free(this);
        SDL_Quit();
        return NULL;
    }

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    SDL_EnableUNICODE(1);
    SDL_WM_SetCaption("navit", NULL);

#ifdef LINUX_TOUCHSCREEN
    input_ts_init(this);
    if(this->ts_fd >= 0)
    {
        /* mouse cursor does not always display correctly in Linux FB.
           anyway, it is unnecessary w/ a touch screen
        */
        SDL_ShowCursor(0);
    }
#endif

#ifdef SDL_SGE
    sge_Update_OFF();
    sge_Lock_ON();
#endif

    *meth=graphics_methods;


    meth->font_new =
       (struct graphics_font_priv *
       (*)(struct graphics_priv *, struct graphics_font_methods *, char *, int,
        int)) this->freetype_methods.font_new;
    meth->get_text_bbox = this->freetype_methods.get_text_bbox;



    g_timeout_add(G_PRIORITY_DEFAULT+10, graphics_sdl_idle, this);

    this->overlay_enable = 1;

    this->aa = 1;
    if((attr=attr_search(attrs, NULL, attr_antialias)))
        this->aa = attr->u.num;

    this->resize_callback_initial=1;
    return this;
}

void
plugin_init(void)
{
        plugin_register_graphics_type("sdl", graphics_sdl_new);
}

