/* graphics_sdl.c -- barebones sdl graphics layer

   copyright (c) 2008 bryan rittmeyer <bryanr@bryanr.org>
   license: GPLv2

   TODO:
   - kb events
   - raw linux touchscreen
   - resize events (cannot be generated right now)
   - SDL window title for X
   - text visibility cleanup/tweaks
   - dashed lines
   - ifdef DEBUG -> dbg()
   - valgrind

   revision history:
   2008-06-01 initial
*/

#include <glib.h>
#include "config.h"
#include "debug.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

#undef SDL_TTF

#ifdef SDL_TTF
#include <SDL/SDL_ttf.h>
#else
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftglyph.h>
#endif

#define SDL_IMAGE
#ifdef SDL_IMAGE
#include <SDL/SDL_image.h>
#endif

#include <alloca.h>

#define DISPLAY_W 800
#define DISPLAY_H 600

#undef ANTI_ALIAS

#undef DEBUG


struct graphics_priv {
    SDL_Surface *screen;

	void (*resize_callback)(void *data, int w, int h);
	void *resize_callback_data;
	void (*motion_callback)(void *data, struct point *p);
	void *motion_callback_data;
	void (*button_callback)(void *data, int press, int button, struct point *p);
	void *button_callback_data;

#ifndef SDL_TTF
    FT_Library library;
    int library_init;
#endif
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


static void
graphics_destroy(struct graphics_priv *gr)
{
#ifdef SDL_TTF
    TTF_Quit();
#endif
    SDL_Quit();
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
    /* TODO: free FT stuff? */
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

	if (!gr->library_init) {
		FT_Init_FreeType( &gr->library );
		gr->library_init=1;
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
    struct graphics_gc_priv *gc=g_new(struct graphics_gc_priv, 1);
	*meth=gc_methods;
    gc->gr=gr;
    gc->linewidth=1; /* upper layer should override anyway? */
	return gc;
}


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
        *meth = gi_methods;
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
image_free(struct graphics_priv *gr, struct graphics_image_priv * img)
{
	dbg(0,"TODO");
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
    int i;

    vx = alloca(count * sizeof(Sint16));
    vy = alloca(count * sizeof(Sint16));

    for(i = 0; i < count; i++)
    {
        vx[i] = (Sint16)p[i].x;
        vy[i] = (Sint16)p[i].y;
#ifdef DEBUG
        printf("draw_polygon: %p %i %d,%d\n", gc, i, p[i].x, p[i].y);
#endif
    }

    filledPolygonRGBA(gr->screen, vx, vy, count,
                      gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
}



static void
draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
#ifdef DEBUG
        printf("draw_rectangle: %d %d %d %d r=%d g=%d b=%d a=%d\n", p->x, p->y, w, h,
               gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#endif
    if(w > DISPLAY_W)
    {
        w = DISPLAY_W-1;
    }
    if(h > DISPLAY_H)
    {
        h = DISPLAY_H-1;
    }
    boxRGBA(gr->screen, p->x, p->y, p->x + w, p->y + h,
            gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);

}

static void
draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
    int l, a;
#ifdef DEBUG
        printf("draw_circle: %d %d %d\n", p->x, p->y, r);
#endif

    /* FIXME: does not quite match gtk */

    /* looks like GTK counts the outline in r,
       sdl-gfx does not?
    */
    r = r - 1;

    for(l = 0; l < gc->linewidth; l++)
    {
        a = l - (gc->linewidth/2);
#ifdef ANTI_ALIAS
        aacircleRGBA(gr->screen, p->x, p->y, r - l,
                   gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#else
        circleRGBA(gr->screen, p->x, p->y, r - l,
                   gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#endif
    }
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

		int l=round(sqrt(dx*dx+dy*dy));
        float angle;

        int x_lw_adj, y_lw_adj;

        if(lw == 1)
        {
#ifdef ANTI_ALIAS
            aalineRGBA(gr->screen, p[i].x, p[i].y, p[i+1].x, p[i+1].y,
                     gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
#else
            lineRGBA(gr->screen, p[i].x, p[i].y, p[i+1].x, p[i+1].y,
                     gc->fore_r, gc->fore_g, gc->fore_b, gc->fore_a);
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

#ifdef DEBUG
            printf("i=%d\n", i);
            printf("   %d,%d->%d,%d\n", p[i].x, p[i].y, p[i+1].x, p[i+1].y);
            printf("   lw=%d angle=%f\n", lw, 180.0 * angle / M_PI);
            printf("   x_lw_adj=%d y_lw_adj=%d\n", x_lw_adj, y_lw_adj);
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
            /* FIXME?: need extra copy of vert[0] here to "close" the poly? apparently not! */

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

#ifndef _WIN32
static unsigned char *
display_text_render_shadow(struct text_glyph *g)
{
	int mask0, mask1, mask2, x, y, w=g->w, h=g->h;
	int str=(g->w+9)/8;
	unsigned char *shadow;
	unsigned char *p, *pm=g->pixmap;

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
	return shadow;
}
#else
/* TODO FIXME */
static GdkImage *
display_text_render_shadow(struct text_glyph *g)
{
	int mask0, mask1, mask2, x, y, w=g->w, h=g->h;
	int str=(g->w+9)/8;
	unsigned char *p, *pm=g->pixmap;
	GdkImage *ret;

	ret=gdk_image_new( GDK_IMAGE_NORMAL , gdk_visual_get_system(), w+2, h+2);

	for (y = 0 ; y < h ; y++) {
		p=ret->mem+str*y;

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
			}
		}
	}
	return ret;
}
#endif

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

static void hexdump(unsigned char *buf, unsigned int n)
{
    int i;
    for(i = 0; i < n; i++)
    {
        if((i&0xf)==0)
        {
            printf("\n%02x: ", i);
        }
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

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
#endif

static void sdl_scale_pal_set(SDL_Surface *ss, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Color c;
    int i;


    for(i = 0; i < 256; i++)
    {
        c.r = 255 - (i * r) / 256;
        c.g = 255 - (i * g) / 256;
        c.b = 255 - (i * b) / 256;
        SDL_SetPalette(ss, SDL_LOGPAL, &c, i, 1); 
    }
}

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
	int i;
	struct text_glyph *g, **gp;
    SDL_Surface *ss;
    SDL_Rect r;
    SDL_Color b, f;

	gp=text->glyph;
	i=text->glyph_count;
	while (i-- > 0)
	{
		g=*gp++;
        if(g->shadow && bg)
        {
            ss = SDL_CreateRGBSurfaceFrom(g->pixmap, g->w+2, g->h+2, 8, g->w+2, 0, 0, 0, 0);
            if(ss)
            {
                //sdl_inv_grayscale_pal_set(ss);
                sdl_scale_pal_set(ss, bg->back_r, bg->back_g, bg->back_b);
                r.x = g->x-1;
                r.y = g->y-1;
                r.w = g->w+2;
                r.h = g->h+2;
                //printf("%d %d %d %d\n", g->x, g->y, g->w, g->h);
                SDL_SetAlpha(ss, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
                //SDL_SetColorKey(ss, SDL_SRCCOLORKEY, 0x00);
                SDL_BlitSurface(ss, NULL, gr->screen, &r);
                SDL_FreeSurface(ss);
            }
        }
		//if (g->shadow && bg)
			//gdk_draw_image(gr->drawable, bg->gc, g->shadow, 0, 0, g->x-1, g->y-1, g->w+2, g->h+2);
	}
	gp=text->glyph;
	i=text->glyph_count;
	while (i-- > 0)
	{
		g=*gp++;
		if (g->w && g->h)
        {
            //hexdump(g->pixmap, g->w * g->h);

            ss = SDL_CreateRGBSurfaceFrom(g->pixmap, g->w, g->h, 8, g->w, 0, 0, 0, 0);
            if(ss)
            {
                //sdl_inv_grayscale_pal_set(ss);
                sdl_scale_pal_set(ss, fg->fore_r, fg->fore_g, fg->fore_b);
                //sdl_inv_bw_pal_set(ss);
                r.x = g->x;
                r.y = g->y;
                r.w = g->w;
                r.h = g->h;
                //printf("%d %d %d %d\n", g->x, g->y, g->w, g->h);
                //SDL_SetAlpha(ss, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
                SDL_SetColorKey(ss, SDL_SRCCOLORKEY, 0x00);
                SDL_BlitSurface(ss, NULL, gr->screen, &r);
                SDL_FreeSurface(ss);
            }
        }
			//gdk_draw_gray_image(gr->drawable, fg->gc, g->x, g->y, g->w, g->h, GDK_RGB_DITHER_NONE, g->pixmap, g->w);
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
    SDL_Rect rect;

#ifdef DEBUG
    printf("draw_mode: %d\n", mode);
#endif

    if(mode == draw_mode_end)
    {
        /* TBD: just update the modified rects? that may be slower, actually */
        rect.x = 0;
        rect.y = 0;
        rect.w = DISPLAY_W;
        rect.h = DISPLAY_H;
        SDL_UpdateRects(gr->screen, 1, &rect);
    }
}

static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h);

static void *
get_data(struct graphics_priv *this, char *type)
{
    printf("get_data: %s\n", type);
	return &dummy;
}



static void
register_resize_callback(struct graphics_priv *this, void (*callback)(void *data, int w, int h), void *data)
{
	this->resize_callback=callback;
	this->resize_callback_data=data;

    if(this->resize_callback)
    {
        this->resize_callback(this->resize_callback_data, DISPLAY_W, DISPLAY_H);
    }
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
};

static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h)
{
    return NULL;
}

static gboolean graphics_sdl_idle(void *data)
{
    struct graphics_priv *gr = (struct graphics_priv *)data;
    struct point p;
    SDL_Event ev;
    int ret;

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
                //g_main_loop_quit(GMainLoop);
                break;
            }

#if 0
            case SDL_VIDEOEXPOSE:
            {
                /* hack to repair damage */
                if(gr->resize_callback)
                {
                    gr->resize_callback(gr->resize_callback_data, DISPLAY_W, DISPLAY_H);
                }
                break;
            }
#endif

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
graphics_sdl_new(struct graphics_methods *meth, struct attr **attrs)
{
    struct graphics_priv *this=g_new0(struct graphics_priv, 1);
    int ret;

#ifndef SDL_TTF
    this->library_init = 0;
#endif

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
#endif

    this->screen = SDL_SetVideoMode(DISPLAY_W, DISPLAY_H, 32, SDL_HWSURFACE);
    if(this->screen == NULL)
    {
        g_free(this);
        SDL_Quit();
        return NULL;
    }

    SDL_WM_SetCaption("navit", NULL);

	*meth=graphics_methods;

    g_timeout_add(10, graphics_sdl_idle, this);

	return this;
}

void
plugin_init(void)
{
        plugin_register_graphics_type("sdl", graphics_sdl_new);
}

