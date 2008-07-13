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

#define OVERLAY_MAX 8

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

	void (*resize_callback)(void *data, int w, int h);
	void *resize_callback_data;
	int resize_callback_initial;

	void (*motion_callback)(void *data, struct point *p);
	void *motion_callback_data;

	void (*button_callback)(void *data, int press, int button, struct point *p);
	void *button_callback_data;

	void (*keypress_callback)(void *data, char *key);
	void *keypress_callback_data;

    struct navit *nav;

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

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, int size, int flags)
{
    struct graphics_font_priv *gf=g_new(struct graphics_font_priv, 1);

#ifdef SDL_TTF
    /* 'size' is in pixels, TTF_OpenFont wants pts. */
    size = size / 15;

//    gf->font = TTF_OpenFont("/usr/share/fonts/truetype/ttf-bitstream-vera/Vera.ttf", size);
    if(flags)
    {
        gf->font = TTF_OpenFont("/usr/share/fonts/truetype/LiberationMono-Bold.ttf", size);
    }
    else
    {
        gf->font = TTF_OpenFont("/usr/share/fonts/truetype/LiberationMono-Regular.ttf", size);
    }

    if(gf->font == NULL)
    {
        g_free(gf);
        return NULL;
    }

    if(flags)
    {
        /* apparently just means bold right now */
        TTF_SetFontStyle(gf->font, TTF_STYLE_BOLD);
    }
    else
    {
        TTF_SetFontStyle(gf->font, TTF_STYLE_NORMAL);
    }
#else
    /* copy-pasted from graphics_gtk_drawing_area.c

       FIXME: figure out some way to share this b/t gtk and sdl graphics plugin.
       new 'font' plugin type that uses an abstracted bitmap fmt to pass to gfx plugin?
    */

	int exact, found;
	char **family;

    if(gr->overlay_mode)
    {
        gr = gr->overlay_parent;
    }

	found=0;
	for (exact=1;!found && exact>=0;exact--) {
		family=fontfamilies;
		while (*family && !found) {
			dbg(1, "Looking for font family %s. exact=%d\n", *family, exact);
			FcPattern *required = FcPatternBuild(NULL, FC_FAMILY, FcTypeString, *family, NULL);
			if (flags)
				FcPatternAddInteger(required,FC_WEIGHT,FC_WEIGHT_BOLD);
			FcConfigSubstitute(FcConfigGetCurrent(), required, FcMatchFont);
			FcDefaultSubstitute(required);
			FcResult result;
			FcPattern *matched = FcFontMatch(FcConfigGetCurrent(), required, &result);
			if (matched) {
				FcValue v1, v2;
				FcChar8 *fontfile;
				int fontindex;
				FcPatternGet(required, FC_FAMILY, 0, &v1);
				FcPatternGet(matched, FC_FAMILY, 0, &v2);
				FcResult r1 = FcPatternGetString(matched, FC_FILE, 0, &fontfile);
				FcResult r2 = FcPatternGetInteger(matched, FC_INDEX, 0, &fontindex);
				if ((r1 == FcResultMatch) && (r2 == FcResultMatch) && (FcValueEqual(v1,v2) || !exact)) {
					dbg(2, "About to load font from file %s index %d\n", fontfile, fontindex);
					FT_New_Face( gr->library, (char *)fontfile, fontindex, &gf->face );
					found=1;
				}
				FcPatternDestroy(matched);
			}
			FcPatternDestroy(required);
			family++;
		}
	}
	if (!found) {
		g_warning("Failed to load font, no labelling");
		g_free(gf);
		return NULL;
	}
        FT_Set_Char_Size(gf->face, 0, size, 300, 300);
	FT_Select_Charmap(gf->face, FT_ENCODING_UNICODE);
#endif

	*meth=font_methods;

    return gf;
}


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
          struct point *hot)
{
#ifdef SDL_IMAGE
    struct graphics_image_priv *gi;

    /* FIXME: meth is not used yet.. so gi leaks. at least xpm is small */

    gi = g_new0(struct graphics_image_priv, 1);
    gi->img = IMG_Load(name);
    if(gi->img)
    {
        /* TBD: improves blit performance? */
        SDL_SetColorKey(gi->img, SDL_RLEACCEL, gi->img->format->colorkey);
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
get_text_bbox(struct graphics_priv *gr, struct graphics_font_priv *font, char *text, int dx, int dy, struct point *ret)
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
                       SDL_MapRGB(gr->screen->format,
                                  gc->fore_r,
                                  gc->fore_g,
                                  gc->fore_b));
    }
    else
    {
        raster_polygon(gr->screen, count, vx, vy,
                       SDL_MapRGB(gr->screen->format,
                                  gc->fore_r,
                                  gc->fore_g,
                                  gc->fore_b));
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
                SDL_MapRGB(gr->screen->format,
                           gc->fore_r,
                           gc->fore_g,
                           gc->fore_b));
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
                            SDL_MapRGB(gr->screen->format,
                                       gc->fore_r, gc->fore_g, gc->fore_b));
        }
        else
        {
            raster_circle(gr->screen, p->x, p->y, r,
                          SDL_MapRGB(gr->screen->format,
                                     gc->fore_r, gc->fore_g, gc->fore_b));
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
                         SDL_MapRGB(gr->screen->format,
                                    gc->fore_r, gc->fore_g, gc->fore_b));
            }
            else
            {
                raster_line(gr->screen, p[i].x, p[i].y, p[i+1].x, p[i+1].y,
                         SDL_MapRGB(gr->screen->format,
                                    gc->fore_r, gc->fore_g, gc->fore_b));
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


#ifdef SDL_TTF
static void
draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *gf, char *text, struct point *p, int dx, int dy)
{
    SDL_Surface *ss;
    SDL_Color f, b;
    SDL_Rect r;

#if 0
    /* correct? */
    f.r = bg->back_r;
    f.g = bg->back_g;
    f.b = bg->back_b;
    b.r = bg->back_r;
    b.g = bg->back_g;
    b.b = bg->back_b;
#else
    f.r = 0xff;
    f.g = 0xff;
    f.b = 0xff;
    b.r = 0x00;
    b.g = 0x00;
    b.b = 0x00;
#endif

    /* TODO: dx + dy? */

    ss = TTF_RenderUTF8_Solid(gf->font, text, b);
    if(ss)
    {
        r.x = p->x;
        r.y = p->y;
        r.w = ss->w;
        r.h = ss->h;
        //SDL_SetAlpha(ss, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
        SDL_BlitSurface(ss, NULL, gr->screen, &r);
    }
}
#else

struct text_glyph {
	int x,y,w,h;
    unsigned char *shadow;
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

static unsigned char *
display_text_render_shadow(struct text_glyph *g)
{
	int mask0, mask1, mask2, x, y, w=g->w, h=g->h;
	int str=(g->w+9)/8;
	unsigned char *shadow;
	unsigned char *p, *pm=g->pixmap;

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
	return shadow;
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
#if 0
        printf("slot->advance x %d y %d\n",
               slot->advance.x,
               slot->advance.y);
#endif

         	x += slot->advance.x;
         	y -= slot->advance.y;
		p=g_utf8_next_char(p);
	}
	return ret;
}

#if 0
static void hexdump(unsigned char *buf, unsigned int w, unsigned int h)
{
    int x, y;
    printf("hexdump %u %u\n", w, h);
    for(y = 0; y < h; y++)
    {
        for(x = 0; x < w; x++)
        {
            printf("%02x ", buf[y*w+x]);
        }
        printf("\n");
    }
}

static void bitdump(unsigned char *buf, unsigned int w, unsigned int h)
{
    int x, pos;
    printf("bitdump %u %u\n", w, h);
    pos = 0;
    for(x = 0; x < h * w; x++)
    {
        if(buf[pos] & (1 << (x&0x7)))
        {
            printf("00 ");
        }
        else
        {
            printf("ff ");
        }

        if((x & 0x7) == 0x7)
        {
            pos++;
        }

        if((x % w) == (w-1))
        {
            printf("\n");
        }
    }
    printf("\n");
}
#endif

#if 0
static void sdl_inv_grayscale_pal_set(SDL_Surface *ss)
{
    SDL_Color c;
    int i;

    for(i = 0; i < 256; i++)
    {
        c.r = 255-i;
        c.g = 255-i;
        c.b = 255-i;
        SDL_SetPalette(ss, SDL_LOGPAL, &c, i, 1); 
    }
}

static void sdl_scale_pal_set(SDL_Surface *ss, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Color c;
    int i;

    for(i = 0; i < 256; i++)
    {
        c.r = (i * r) / 256;
        c.g = (i * g) / 256;
        c.b = (i * b) / 256;
        SDL_SetPalette(ss, SDL_LOGPAL, &c, i, 1); 
    }
}
#endif

#if 0
static void sdl_fixed_pal_set(SDL_Surface *ss, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Color c;
    int i;

    c.r = r;
    c.g = g;
    c.b = b;
    for(i = 0; i < 256; i++)
    {
        SDL_SetPalette(ss, SDL_LOGPAL, &c, i, 1); 
    }
}
#endif

static void
display_text_draw(struct text_render *text, struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg)
{
    int i, x, y, poff, soff, col_lev;
    struct text_glyph *gly, **gp;
    Uint32 pix;
    Uint8 r, g, b;

#if 0
    dbg(0,"%u %u %u %u, %u %u %u %u\n",
        fg->fore_a, fg->fore_r, fg->fore_g, fg->fore_b,
        fg->back_a, fg->back_r, fg->back_g, fg->back_b);

    dbg(0,"%u %u %u %u, %u %u %u %u\n",
        bg->fore_a, bg->fore_r, bg->fore_g, bg->fore_b,
        bg->back_a, bg->back_r, bg->back_g, bg->back_b);
#endif
    
    if(bg)
    {
        col_lev = bg->fore_r + bg->fore_g + bg->fore_b;
    }
    else
    {
        col_lev = 0;
    }

    /* TODO: lock/unlock in draw_mode() to reduce overhead */
    SDL_LockSurface(gr->screen);
	gp=text->glyph;
	i=text->glyph_count;
	while (i-- > 0)
	{
		gly=*gp++;
		if (gly->w && gly->h)
        {
#if 0
            if(gly->shadow && bg)
            {
                hexdump(gly->pixmap, gly->w, gly->h);
                bitdump(gly->shadow, gly->w+2, gly->h+2);    
            }
#endif

            for(y = 0; y < gly->h + 2; y++)
            {
                if(gly->y - 1 + y < 0)
                {
                    continue;
                }

                if(((gly->y-1) + y) >= gr->screen->h)
                {
                    break;
                }

                for(x = 0; x < gly->w + 2; x++)
                {
                    if(gly->x - 1 + x < 0)
                    {
                        continue;
                    }

                    if(((gly->x-1) + x) >= gr->screen->w)
                    {
                        break;
                    }

                    poff = gr->screen->w * ((gly->y-1) + y) + ((gly->x-1) + x);
                    poff = poff * gr->screen->format->BytesPerPixel;

                    switch(gr->screen->format->BytesPerPixel)
                    {
                        case 2:
                        {
                            pix = *(Uint16 *)((Uint8*)gr->screen->pixels + poff);
                            break;
                        }
                        case 4:
                        {
                            pix = *(Uint32 *)((Uint8*)gr->screen->pixels + poff);
                            break;
                        }
                        default:
                        {
                            pix = 0;
                            break;
                        }
                    }

                    SDL_GetRGB(pix,
                               gr->screen->format,
                               &r,
                               &g,
                               &b);

#ifdef DEBUG
                    printf("%u %u -> %u off\n",
                           gly->x,
                           gly->y,
                           off);

                    printf("%u,%u: %u %u %u in\n",
                           x, y,
                           r, g, b, off);
#endif



                    if(gly->shadow && bg)
                    {
                        soff = (8 * ((gly->w+9)/8) * y) + x;
                        pix = gly->shadow[soff/8];

                        if(pix & (1 << (7-(soff&0x7))))
                        {
                            if(col_lev >= 3*0x80)
                            {
                                r |= bg->fore_r;
                                g |= bg->fore_g;
                                b |= bg->fore_b;
                            }
                            else
                            {
                                r &= ~bg->fore_r;
                                g &= ~bg->fore_g;
                                b &= ~bg->fore_b;
                            }
                        }
                    }

                    /* glyph */
                    if((x > 0) && (x <= gly->w) &&
                       (y > 0) && (y <= gly->h))
                    {
                        if(bg && (col_lev >= 3*0x80))
                        {
                            r &= ~gly->pixmap[gly->w * (y-1) + (x-1)];
                            g &= ~gly->pixmap[gly->w * (y-1) + (x-1)];
                            b &= ~gly->pixmap[gly->w * (y-1) + (x-1)];
                        }
                        else
                        {
                            r |= gly->pixmap[gly->w * (y-1) + (x-1)];
                            g |= gly->pixmap[gly->w * (y-1) + (x-1)];
                            b |= gly->pixmap[gly->w * (y-1) + (x-1)];
                        }
                    }

#ifdef DEBUG
                    printf("%u,%u: %u %u %u out\n",
                           x, y,
                           r, g, b);
#endif

                    pix = SDL_MapRGB(gr->screen->format,
                                     r,
                                     g,
                                     b);

                    switch(gr->screen->format->BytesPerPixel)
                    {
                        case 2:
                        {
                            *(Uint16 *)((Uint8*)gr->screen->pixels + poff) = pix;
                            break;
                        }
                        case 4:
                        {
                            *(Uint32 *)((Uint8*)gr->screen->pixels + poff) = pix;
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                }
            }

            //dbg(0, "glyph %d %d %d %d\n", g->x, g->y, g->w, g->h);
        }
	}
    SDL_UnlockSurface(gr->screen);
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
           free((*gp)->shadow);
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

#if 0
	if (bg) {
		gdk_gc_set_function(fg->gc, GDK_AND_INVERT);
		gdk_gc_set_function(bg->gc, GDK_OR);
	}
#endif

	t=display_text_render(text, font, dx, dy, p->x, p->y);
	display_text_draw(t, gr, fg, bg);
	display_text_free(t);
#if 0
	if (bg) {
		gdk_gc_set_function(fg->gc, GDK_COPY);
        	gdk_gc_set_function(bg->gc, GDK_COPY);
	}
#endif
}
#endif



static void
draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
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
                    if(ov)
                    {
                        rect.x = ov->overlay_x;
                        rect.y = ov->overlay_y;
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

static struct graphics_methods graphics_methods;

static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h)
{
    struct graphics_priv *ov;
    int i, x, y;

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
    dbg(0, "x %d y %d\n", p->x, p->y);

    if(p->x < 0)
    {
        x = gr->screen->w + p->x;
    }
    else
    {
        x = p->x;
    }
    if(p->y < 0)
    {
        y = gr->screen->h + p->y;
    }
    else
    {
        y = p->y;
    }

    dbg(1, "overlay_new %d %d %d %u %u\n", i,
        x,
        y,
        w,
        h);

    ov = g_new0(struct graphics_priv, 1);

    ov->screen = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA, 
                                      w, h,
#if 1 
                                      gr->screen->format->BitsPerPixel,
                                      gr->screen->format->Rmask,
                                      gr->screen->format->Gmask,
                                      gr->screen->format->Bmask,
                                      0x00000000);
#else
                                      0x00ff0000,
                                      0x0000ff00,
                                      0x000000ff,
                                      0xff000000);
#endif
    SDL_SetAlpha(ov->screen, SDL_SRCALPHA, 128);

    ov->overlay_mode = 1;
    ov->overlay_x = x;
    ov->overlay_y = y;
    ov->overlay_parent = gr;
    ov->overlay_idx = i;

    gr->overlay_array[i] = ov;

	*meth=graphics_methods;

    return ov;
}

static void overlay_disable(struct graphics_priv *gr, int disable)
{
    gr->overlay_enable = !disable;
}


static int window_fullscreen(struct window *win, int on)
{
    /* TODO */
    return 0;
}

static struct window sdl_win =
{
    NULL,
    window_fullscreen

};

static void *
get_data(struct graphics_priv *this, char *type)
{
    printf("get_data: %s\n", type);

    if(strcmp(type, "window") == 0)
    {
        return &sdl_win;
    }
    else
    {
    	return &dummy;
    }
}



static void
register_resize_callback(struct graphics_priv *this, void (*callback)(void *data, int w, int h), void *data)
{
	this->resize_callback=callback;
	this->resize_callback_data=data;
    this->resize_callback_initial=1;
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

static void
register_keypress_callback(struct graphics_priv *this,
                            void (*callback)(void *data, char *key),
                            void *data)
{
    this->keypress_callback=callback;
    this->keypress_callback_data=data;
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
	image_free,
	get_text_bbox,
	overlay_disable,
	register_keypress_callback
};


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
        if(gr->resize_callback)
        {
            gr->resize_callback(gr->resize_callback_data, gr->screen->w, gr->screen->h);
        }

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
                    if(gr->motion_callback)
                    {
                        gr->motion_callback(gr->motion_callback_data, &p);
                    }

                    if(gr->ts_hit > 0)
                    {
                        if(gr->button_callback)
                        {
                            gr->button_callback(gr->button_callback_data, 1, SDL_BUTTON_LEFT, &p);
                        }
                    }
                    else if(gr->ts_hit == 0)
                    {
                        if(gr->button_callback)
                        {
                            gr->button_callback(gr->button_callback_data, 0, SDL_BUTTON_LEFT, &p);
                        }
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
                if(gr->motion_callback)
                {
                    gr->motion_callback(gr->motion_callback_data, &p);
                }
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
                        key = 0;
                        break;
                    }
                }

                if(gr->keypress_callback)
                {
                    keybuf[0] = key;
                    keybuf[1] = '\0';
                    gr->keypress_callback(gr->keypress_callback_data, keybuf);
                }

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
                if(gr->button_callback)
                {
                    gr->button_callback(gr->button_callback_data, 1, ev.button.button, &p);
                }
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
                if(gr->button_callback)
                {
                    gr->button_callback(gr->button_callback_data, 0, ev.button.button, &p);
                }
                break;
            }

            case SDL_QUIT:
            {
                navit_destroy(gr->nav);
                break;
            }

            case SDL_VIDEORESIZE:
            {

                gr->screen = SDL_SetVideoMode(ev.resize.w, ev.resize.h, 16, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);
                if(gr->screen == NULL)
                {
                    navit_destroy(gr->nav);
                }
                else
                {
                    if(gr->resize_callback)
                    {
                        gr->resize_callback(gr->resize_callback_data, gr->screen->w, gr->screen->h);
                    }
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
graphics_sdl_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs)
{
    struct graphics_priv *this=g_new0(struct graphics_priv, 1);
    struct attr *attr;
    int ret;

    this->nav = nav;

    ret = SDL_Init(SDL_INIT_VIDEO);
    if(ret < 0)
    {
        g_free(this);
        return NULL;
    }

#ifdef SDL_TTF
    ret = TTF_Init();
    if(ret < 0)
    {
        g_free(this);
        SDL_Quit();
        return NULL;
    }
#else
    FT_Init_FreeType( &this->library );
#endif

    /* TODO: xml params for W/H/BPP */

    this->screen = SDL_SetVideoMode(DISPLAY_W, DISPLAY_H, 16, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);
    if(this->screen == NULL)
    {
        g_free(this);
        SDL_Quit();
        return NULL;
    }

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

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

    g_timeout_add(10, graphics_sdl_idle, this);

    this->overlay_enable = 1;

    this->aa = 1;
    if((attr=attr_search(attrs, NULL, attr_antialias)))
        this->aa = attr->u.num;

    return this;
}

void
plugin_init(void)
{
        plugin_register_graphics_type("sdl", graphics_sdl_new);
}

