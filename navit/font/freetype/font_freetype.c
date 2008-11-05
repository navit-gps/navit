#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include <glib.h>
#include FT_FREETYPE_H
#include <freetype/ftglyph.h>
#include "config.h"
#include "point.h"
#include "graphics.h"
#include "debug.h"
#include "plugin.h"
#include "color.h"
#include "font_freetype.h"

struct font_freetype_font {
	FT_Face face;
};

struct font_priv {
};

static struct font_priv dummy;
static FT_Library library;
static int library_init;


static void
font_freetype_get_text_bbox(struct graphics_priv *gr,
			    struct font_freetype_font *font, char *text,
			    int dx, int dy, struct point *ret,
			    int estimate)
{
	char *p = text;
	FT_BBox bbox;
	FT_UInt glyph_index;
	FT_GlyphSlot slot = font->face->glyph;	// a small shortcut
	FT_Glyph glyph;
	FT_Matrix matrix;
	FT_Vector pen;
	pen.x = 0 * 64;
	pen.y = 0 * 64;
	matrix.xx = dx;
	matrix.xy = dy;
	matrix.yx = -dy;
	matrix.yy = dx;
	int n, len, x = 0, y = 0;

	bbox.xMin = bbox.yMin = 32000;
	bbox.xMax = bbox.yMax = -32000;
	FT_Set_Transform(font->face, &matrix, &pen);
	len = g_utf8_strlen(text, -1);
	for (n = 0; n < len; n++) {
		FT_BBox glyph_bbox;
		glyph_index =
		    FT_Get_Char_Index(font->face, g_utf8_get_char(p));
		p = g_utf8_next_char(p);
		FT_Load_Glyph(font->face, glyph_index, FT_LOAD_DEFAULT);
		FT_Get_Glyph(font->face->glyph, &glyph);
		FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_pixels,
				  &glyph_bbox);
		FT_Done_Glyph(glyph);
		glyph_bbox.xMin += x >> 6;
		glyph_bbox.xMax += x >> 6;
		glyph_bbox.yMin += y >> 6;
		glyph_bbox.yMax += y >> 6;
		x += slot->advance.x;
		y -= slot->advance.y;
		if (glyph_bbox.xMin < bbox.xMin)
			bbox.xMin = glyph_bbox.xMin;
		if (glyph_bbox.yMin < bbox.yMin)
			bbox.yMin = glyph_bbox.yMin;
		if (glyph_bbox.xMax > bbox.xMax)
			bbox.xMax = glyph_bbox.xMax;
		if (glyph_bbox.yMax > bbox.yMax)
			bbox.yMax = glyph_bbox.yMax;
	}
	if (bbox.xMin > bbox.xMax) {
		bbox.xMin = 0;
		bbox.yMin = 0;
		bbox.xMax = 0;
		bbox.yMax = 0;
	}
	ret[0].x = bbox.xMin;
	ret[0].y = -bbox.yMin;
	ret[1].x = bbox.xMin;
	ret[1].y = -bbox.yMax;
	ret[2].x = bbox.xMax;
	ret[2].y = -bbox.yMax;
	ret[3].x = bbox.xMax;
	ret[3].y = -bbox.yMin;
}

static struct font_freetype_text *
font_freetype_text_new(char *text, struct font_freetype_font *font, int dx,
		       int dy)
{
	FT_GlyphSlot slot = font->face->glyph;	// a small shortcut
	FT_Matrix matrix;
	FT_Vector pen;
	FT_UInt glyph_index;
	int y, n, len, w, h, pixmap_len;
	struct font_freetype_text *ret;
	struct font_freetype_glyph *curr;
	char *p = text;
	unsigned char *gl, *pm;

	len = g_utf8_strlen(text, -1);
	ret = g_malloc(sizeof(*ret) + len * sizeof(struct text_glyph *));
	ret->glyph_count = len;

	matrix.xx = dx;
	matrix.xy = dy;
	matrix.yx = -dy;
	matrix.yy = dx;

	pen.x = 0 * 64;
	pen.y = 0 * 64;
	FT_Set_Transform(font->face, &matrix, &pen);

	for (n = 0; n < len; n++) {

		glyph_index =
		    FT_Get_Char_Index(font->face, g_utf8_get_char(p));
		FT_Load_Glyph(font->face, glyph_index, FT_LOAD_DEFAULT);
		FT_Render_Glyph(font->face->glyph, ft_render_mode_normal);

		w = slot->bitmap.width;
		h = slot->bitmap.rows;
		if (w && h)
			pixmap_len = (w + 2) * (h + 2);
		else
			pixmap_len = 0;
		curr = g_malloc0(sizeof(*curr) + pixmap_len);
		if (pixmap_len) {
			curr->w = w;
			curr->h = h;
		}
		curr->pixmap = (unsigned char *) (curr + 1);
		ret->glyph[n] = curr;

		curr->x = slot->bitmap_left << 6;
		curr->y = -slot->bitmap_top << 6;
		for (y = 0; y < h; y++) {
			gl = slot->bitmap.buffer + y * slot->bitmap.pitch;
			pm = curr->pixmap + y * w;
			memcpy(pm, gl, w);
		}

		curr->dx = slot->advance.x;
		curr->dy = -slot->advance.y;
		p = g_utf8_next_char(p);
	}
	ret->glyph_count = len;
	return ret;
}

/**
 * List of font families to use, in order of preference
 */
static char *fontfamilies[] = {
	"Liberation Sans",
	"Arial",
	"NcrBI4nh",
	"luximbi",
	"FreeSans",
	"DejaVu Sans",
	NULL,
};

static void
font_destroy(struct graphics_font_priv *font)
{
	g_free(font);
	/* TODO: free font->face */
}

static struct graphics_font_methods font_methods = {
	font_destroy
};


static void
font_freetype_text_destroy(struct font_freetype_text *text)
{
	int i;
	struct font_freetype_glyph **gp;

	gp = text->glyph;
	i = text->glyph_count;
	while (i-- > 0)
		g_free(*gp++);
	g_free(text);
}


/**
 * Load a new font using the fontconfig library.
 * First search for each of the font families and require and exact match on family
 * If no font found, let fontconfig pick the best match
 * @param graphics_priv FIXME
 * @param graphics_font_methods FIXME
 * @param fontfamily the preferred font family
 * @param size requested size of fonts
 * @param flags extra flags for the font (bold,etc)
 * @returns <>
*/
static struct font_freetype_font *
font_freetype_font_new(struct graphics_priv *gr,
		       struct graphics_font_methods *meth,
		       char *fontfamily, int size, int flags)
{
	struct font_freetype_font *font =
	    g_new(struct font_freetype_font, 1);

	*meth = font_methods;
	int exact, found;
	char **family;

	if (!library_init) {
		FT_Init_FreeType(&library);
		library_init = 1;
	}
	found = 0;
	dbg(2, " about to search for fonts, prefered = %s\n", fontfamily);
	for (exact = 1; !found && exact >= 0; exact--) {
		if (fontfamily) {
			/* prepend the font passed so we look for it first */
			family =
			    malloc(sizeof(fontfamilies) +
				   sizeof(fontfamily));
			if (!family) {
				dbg(0,
				    "Out of memory while creating the font families table\n");
				return NULL;
			}
			memcpy(family, &fontfamily, sizeof(fontfamily));
			memcpy(family + 1, fontfamilies,
			       sizeof(fontfamilies));
		} else {
			family = fontfamilies;
		}


		while (*family && !found) {
			dbg(2, "Looking for font family %s. exact=%d\n",
			    *family, exact);
			FcPattern *required =
			    FcPatternBuild(NULL, FC_FAMILY, FcTypeString,
					   *family, NULL);
			if (flags)
				FcPatternAddInteger(required, FC_WEIGHT,
						    FC_WEIGHT_BOLD);
			FcConfigSubstitute(FcConfigGetCurrent(), required,
					   FcMatchFont);
			FcDefaultSubstitute(required);
			FcResult result;
			FcPattern *matched =
			    FcFontMatch(FcConfigGetCurrent(), required,
					&result);
			if (matched) {
				FcValue v1, v2;
				FcChar8 *fontfile;
				int fontindex;
				FcPatternGet(required, FC_FAMILY, 0, &v1);
				FcPatternGet(matched, FC_FAMILY, 0, &v2);
				FcResult r1 =
				    FcPatternGetString(matched, FC_FILE, 0,
						       &fontfile);
				FcResult r2 =
				    FcPatternGetInteger(matched, FC_INDEX,
							0, &fontindex);
				if ((r1 == FcResultMatch)
				    && (r2 == FcResultMatch)
				    && (FcValueEqual(v1, v2) || !exact)) {
					dbg(2,
					    "About to load font from file %s index %d\n",
					    fontfile, fontindex);
					FT_New_Face(library,
						    (char *) fontfile,
						    fontindex,
						    &font->face);
					found = 1;
				}
				FcPatternDestroy(matched);
			}
			FcPatternDestroy(required);
			family++;
		}
	}
	if (!found) {
		g_warning("Failed to load font, no labelling");
		g_free(font);
		return NULL;
	}
	FT_Set_Char_Size(font->face, 0, size, 300, 300);
	FT_Select_Charmap(font->face, FT_ENCODING_UNICODE);
	return font;
}

static int
font_freetype_glyph_get_shadow(struct font_freetype_glyph *g,
			       unsigned char *data, int depth, int stride, struct color *foreground, struct color *background)
{
	int mask0, mask1, mask2, x, y, w = g->w, h = g->h;
	unsigned int bg, fg;
	unsigned char *pm, *psp,*ps,*psn;
	switch (depth) {
	case 1:
		fg=0xff;
		bg=0x00;
		break;
	case 8:
		fg=foreground->a>>8;
		bg=background->a>>8;
		break;
	case 32:
		fg=((foreground->a>>8)<<24)|
		   ((foreground->r>>8)<<16)|
		   ((foreground->g>>8)<<8)|
		   ((foreground->b>>8)<<0);
		bg=((background->a>>8)<<24)|
		   ((background->r>>8)<<16)|
		   ((background->g>>8)<<8)|
		   ((background->b>>8)<<0);
		break;
	default:
		return 0;
	}
	for (y = 0; y < h+2; y++) {
		if (stride) {
			ps = data + stride * y;
		} else {
			unsigned char **dataptr=(unsigned char **)data;
			ps = dataptr[y];
		}
		switch (depth) {
		case 1:
			memset(ps, bg, (w+9)/2);
			break;
		case 8:
			memset(ps, bg, w+2);
			break;
		case 32:
			for (x = 0 ; x < w+2 ; x++) 
				((unsigned int *)ps)[x]=bg;
			break;
		}
	}
	for (y = 0; y < h; y++) {
		pm = g->pixmap + y * w;
		if (stride) {
			psp = data + stride * y;
			ps = psp + stride;
			psn = ps + stride;
		} else {
			unsigned char **dataptr=(unsigned char **)data;
			psp = dataptr[y];
			ps = dataptr[y+1];
			psn = dataptr[y+2];
		}
		switch (depth) {
		case 1:
			mask0 = 0x4000;
			mask1 = 0xe000;
			mask2 = 0x4000;
			for (x = 0; x < w; x++) {
				if (*pm) {
					psp[0] |= (mask0 >> 8);
					if (mask0 & 0xff)
						psp[1] |= mask0;
					ps[0] |= (mask1 >> 8);
					if (mask1 & 0xff)
						ps[1] |= mask1;
					psn[0] |= (mask2 >> 8);
					if (mask2 & 0xff)
						psn[1] |= mask2;
				}
				mask0 >>= 1;
				mask1 >>= 1;
				mask2 >>= 1;
				if (!
				    ((mask0 >> 8) | (mask1 >> 8) |
				     (mask2 >> 8))) {
					mask0 <<= 8;
					mask1 <<= 8;
					mask2 <<= 8;
					psp++;
					ps++;
					psn++;
				}
				pm++;
			}
			break;
		case 8:
			for (x = 0; x < w; x++) {
				if (*pm) {
					psp[1] = fg;
					ps[0] = fg;
					ps[1] = fg;
					ps[2] = fg;
					psn[1] = fg;
				}
				psp++;
				ps++;
				psn++;
				pm++;
			}
			break;
		case 32:
			for (x = 0; x < w; x++) {
				if (*pm) {
					((unsigned int *)psp)[1]=fg;
					((unsigned int *)ps)[0]=fg;
					((unsigned int *)ps)[1]=fg;
					((unsigned int *)ps)[2]=fg;
					((unsigned int *)psn)[1]=fg;
				}
				psp+=4;
				ps+=4;
				psn+=4;
				pm++;
			}
			break;
		}
	}
	return 1;
}

static int
font_freetype_glyph_get_glyph(struct font_freetype_glyph *g,
			       unsigned char *data, int depth, int stride, struct color *fg, struct color *bg, struct color *transparent)
{
	int x, y, w = g->w, h = g->h;
	unsigned int tr;
	unsigned char v,vi,*pm, *ps;
	switch (depth) {
	case 32:
		tr=((transparent->a>>8)<<24)|
		   ((transparent->r>>8)<<16)|
		   ((transparent->g>>8)<<8)|
		   ((transparent->b>>8)<<0);
		break;
	default:
		return 0;
	}
	for (y = 0; y < h; y++) {
		pm = g->pixmap + y * w;
		if (stride) {
			ps = data + stride*y;
		} else {
			unsigned char **dataptr=(unsigned char **)data;
			ps = dataptr[y];
		}
		switch (depth) {
		case 32:
			for (x = 0; x < w; x++) {
				v=*pm;
				if (v) {
					vi=255-v;
					((unsigned int *)ps)[0]=
						((((fg->a*v+bg->a*vi)/255)>>8)<<24)|
						((((fg->r*v+bg->r*vi)/255)>>8)<<16)|
						((((fg->g*v+bg->g*vi)/255)>>8)<<8)|
						((((fg->b*v+bg->b*vi)/255)>>8)<<0);
				} else
					((unsigned int *)ps)[0]=tr;
				ps+=4;
				pm++;
			}
			break;
		}
	}
	return 1;
}

static struct font_freetype_methods methods = {
	font_freetype_font_new,
	font_freetype_get_text_bbox,
	font_freetype_text_new,
	font_freetype_text_destroy,
	font_freetype_glyph_get_shadow,
	font_freetype_glyph_get_glyph,	
};

static struct font_priv *
font_freetype_new(void *meth)
{
	*((struct font_freetype_methods *) meth) = methods;
	return &dummy;
}

void
plugin_init(void)
{
	plugin_register_font_type("freetype", font_freetype_new);
	FcInit();
}
