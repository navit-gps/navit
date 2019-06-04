/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2011 Navit Team
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
#include "config.h"
#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif
#include <ft2build.h>
#include <glib.h>
#include FT_FREETYPE_H
#ifndef USE_CACHING
#define USE_CACHING 1
#endif
#if USE_CACHING
#include FT_CACHE_H
#endif
#ifdef USE_FRIBIDI
#pragma GCC diagnostic push
// fribidi.h is a bit misbehaved...
#pragma GCC diagnostic ignored "-Wundef"
#include <fribidi.h>
#pragma GCC diagnostic pop
#endif
#include FT_GLYPH_H
#include "point.h"
#include "graphics.h"
#include "debug.h"
#include "plugin.h"
#include "color.h"
#include "atom.h"
#include "font_freetype.h"

#ifndef HAVE_LOOKUP_SCALER
#if FREETYPE_MAJOR * 10000 + FREETYPE_MINOR * 100 + FREETYPE_PATCH > 20304
#define HAVE_LOOKUP_SCALER 1
#else
#define HAVE_LOOKUP_SCALER 0
#endif
#endif

#define COLOR_BITDEPTH_OUTPUT 8
#define COL_SHIFT (COLOR_BITDEPTH-COLOR_BITDEPTH_OUTPUT)

struct font_freetype_font {
    int size;
#if USE_CACHING
#if HAVE_LOOKUP_SCALER
    FTC_ScalerRec scaler;
#else
    FTC_ImageTypeRec scaler;
#endif
    int charmap_index;
#else
    FT_Face face;
#endif
};

struct font_priv {
    int data;
};

static struct font_priv dummy;
static FT_Library library;
#if USE_CACHING
static FTC_Manager manager;
static FTC_ImageCache image_cache;
static FTC_CMapCache charmap_cache;
static FTC_SBitCache sbit_cache;
#endif
static int library_init = 0;
static int library_deinit = 0;


static void font_freetype_get_text_bbox(struct graphics_priv *gr, struct font_freetype_font *font, char *text, int dx, int dy, struct point *ret, int estimate) {
    char *p = text;
    FT_BBox bbox;
    FT_UInt glyph_index;
    FT_Glyph glyph;
    FT_Matrix matrix;
    FT_Vector pen;
    int i;
    struct point pt;
    int n, len, x = 0, y = 0;
    pen.x = 0 * 64;
    pen.y = 0 * 64;
#if 0
    matrix.xx = dx;
    matrix.xy = dy;
    matrix.yx = -dy;
    matrix.yy = dx;
#else
    matrix.xx = 0x10000;
    matrix.xy = 0;
    matrix.yx = 0;
    matrix.yy = 0x10000;
#endif

    len = g_utf8_strlen(text, -1);
    if (estimate) {
        bbox.xMin = 0;
        bbox.yMin = 0;
        bbox.yMax = 13*font->size/256;
        bbox.xMax = 9*font->size*len/256;
    } else {
        bbox.xMin = bbox.yMin = 32000;
        bbox.xMax = bbox.yMax = -32000;
#if !USE_CACHING
        FT_Set_Transform(font->face, &matrix, &pen);
#endif
        for (n = 0; n < len; n++) {
            FT_BBox glyph_bbox;
#if USE_CACHING
            FTC_Node anode = NULL;
            FT_Glyph cached_glyph;
            glyph_index = FTC_CMapCache_Lookup(charmap_cache, font->scaler.face_id, font->charmap_index, g_utf8_get_char(p));
#if HAVE_LOOKUP_SCALER
            FTC_ImageCache_LookupScaler(image_cache, &font->scaler, FT_LOAD_DEFAULT, glyph_index, &cached_glyph, &anode);
#else
            FTC_ImageCache_Lookup(image_cache, &font->scaler, glyph_index, &cached_glyph, &anode);
#endif /* HAVE_LOOKUP_SCALER */
            FT_Glyph_Copy(cached_glyph, &glyph);
            FT_Glyph_Transform(glyph, &matrix, &pen);
#else
            glyph_index = FT_Get_Char_Index(font->face, g_utf8_get_char(p));
            FT_Load_Glyph(font->face, glyph_index, FT_LOAD_DEFAULT);
            FT_Get_Glyph(font->face->glyph, &glyph);
#endif /* USE_CACHING */
            FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_pixels, &glyph_bbox);

            glyph_bbox.xMin += x >> 6;
            glyph_bbox.xMax += x >> 6;
            glyph_bbox.yMin += y >> 6;
            glyph_bbox.yMax += y >> 6;
            x += glyph->advance.x >> 10;
            y -= glyph->advance.y >> 10;
            if (glyph_bbox.xMin < bbox.xMin)
                bbox.xMin = glyph_bbox.xMin;
            if (glyph_bbox.yMin < bbox.yMin)
                bbox.yMin = glyph_bbox.yMin;
            if (glyph_bbox.xMax > bbox.xMax)
                bbox.xMax = glyph_bbox.xMax;
            if (glyph_bbox.yMax > bbox.yMax)
                bbox.yMax = glyph_bbox.yMax;
            p = g_utf8_next_char(p);

            FT_Done_Glyph(glyph);
#if USE_CACHING
            FTC_Node_Unref(anode, manager);
#endif
        }
        if (bbox.xMin > bbox.xMax) {
            bbox.xMin = 0;
            bbox.yMin = 0;
            bbox.xMax = 0;
            bbox.yMax = 0;
        }
    }
    ret[0].x = bbox.xMin;
    ret[0].y = -bbox.yMin;
    ret[1].x = bbox.xMin;
    ret[1].y = -bbox.yMax;
    ret[2].x = bbox.xMax;
    ret[2].y = -bbox.yMax;
    ret[3].x = bbox.xMax;
    ret[3].y = -bbox.yMin;
    if (dy != 0 || dx != 0x10000) {
        for (i = 0 ; i < 4 ; i++) {
            pt=ret[i];
            ret[i].x=(pt.x*dx-pt.y*dy)/0x10000;
            ret[i].y=(pt.y*dx+pt.x*dy)/0x10000;
        }
    }
}

static struct font_freetype_text *font_freetype_text_new(char *text, struct font_freetype_font *font, int dx, int dy) {
    FT_Matrix matrix;
    FT_Vector pen;
    FT_UInt glyph_index;
    int y, n, len, w, h, pixmap_len;
    struct font_freetype_text *ret;
    struct font_freetype_glyph *curr;
    char *p = text;
    unsigned char *gl, *pm;
    FT_BitmapGlyph glyph_bitmap;
    FT_Glyph glyph;

    len = g_utf8_strlen(text, -1);
    ret = g_malloc(sizeof(*ret) + len * sizeof(struct text_glyph *));
    ret->glyph_count = len;

    matrix.xx = dx;
    matrix.xy = dy;
    matrix.yx = -dy;
    matrix.yy = dx;

    pen.x = 0 * 64;
    pen.y = 0 * 64;
#if !USE_CACHING
    FT_Set_Transform(font->face, &matrix, &pen);
#endif

#ifdef USE_FRIBIDI
    // Need to use fribidi to handle the string properly
    char visual_text[len*4+1];
    {
        FriBidiChar unicode_text[len+2];
        FriBidiChar visual_unicode_text[len+2];
        FriBidiStrIndex textlen =  strlen(text);
#ifdef FRIBIDIOLD
        FriBidiCharType base = FRIBIDI_TYPE_LTR;
#else
        FriBidiParType base = FRIBIDI_PAR_LTR;
#endif

        FriBidiStrIndex unicode_len =
#ifdef FRIBIDIOLD
            fribidi_utf8_to_unicode(text, textlen, unicode_text);
#else
            fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8, text, textlen, unicode_text);
#endif
        fribidi_log2vis(unicode_text, unicode_len, &base, visual_unicode_text, NULL, NULL, NULL);
        // TODO: check return value
#ifdef FRIBIDIOLD
        fribidi_unicode_to_utf8(visual_unicode_text, unicode_len, visual_text);
#else
        fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, visual_unicode_text, unicode_len, visual_text);
#endif
        p = visual_text;
    }
#endif /* USE_FRIBIDI */

    for (n = 0; n < len; n++) {

#if USE_CACHING
        FTC_Node anode=NULL;
        FT_Glyph cached_glyph;
        glyph_index = FTC_CMapCache_Lookup(charmap_cache, font->scaler.face_id, font->charmap_index, g_utf8_get_char(p));
#if HAVE_LOOKUP_SCALER
        FTC_ImageCache_LookupScaler(image_cache, &font->scaler, FT_LOAD_DEFAULT, glyph_index, &cached_glyph, &anode);
#else
        FTC_ImageCache_Lookup(image_cache, &font->scaler, glyph_index, &cached_glyph, &anode);
#endif
        FT_Glyph_Copy(cached_glyph, &glyph);
        FT_Glyph_Transform(glyph, &matrix, &pen);
        FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, NULL, TRUE);
#else
        glyph_index = FT_Get_Char_Index(font->face, g_utf8_get_char(p));
        FT_Load_Glyph(font->face, glyph_index, FT_LOAD_DEFAULT);
        FT_Get_Glyph(font->face->glyph, &glyph);
#endif
        FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, NULL, TRUE);
        glyph_bitmap = (FT_BitmapGlyph)glyph;

        w = glyph_bitmap->bitmap.width;
        h = glyph_bitmap->bitmap.rows;
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

        curr->x = glyph_bitmap->left << 6;
        curr->y = -glyph_bitmap->top << 6;
        for (y = 0; y < h; y++) {
            gl = glyph_bitmap->bitmap.buffer + y * glyph_bitmap->bitmap.pitch;
            pm = curr->pixmap + y * w;
            memcpy(pm, gl, w);
        }

        curr->dx = glyph->advance.x >> 10;
        curr->dy = -glyph->advance.y >> 10;
        FT_Done_Glyph(glyph);
#if USE_CACHING
        FTC_Node_Unref(anode, manager);
#endif
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

static void font_destroy(struct graphics_font_priv *font) {
    g_free(font);
    /* TODO: free font->face */
}

static struct graphics_font_methods font_methods = {
    font_destroy
};


static void font_freetype_text_destroy(struct font_freetype_text *text) {
    int i;
    struct font_freetype_glyph **gp;

    gp = text->glyph;
    i = text->glyph_count;
    while (i-- > 0)
        g_free(*gp++);
    g_free(text);
}

#if USE_CACHING
static FT_Error face_requester( FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face* aface ) {
    FT_Error ret;
    char *fontfile,*fontindex;
    if (! face_id)
        return FT_Err_Invalid_Handle;
    fontfile=g_strdup((char *)face_id);
    dbg(lvl_debug,"fontfile=%s", fontfile);
    fontindex=strrchr(fontfile,'/');
    if (! fontindex) {
        g_free(fontfile);
        return FT_Err_Invalid_Handle;
    }
    *fontindex++='\0';
    dbg(lvl_debug,"new face %s %d", fontfile, atoi(fontindex));
    ret = FT_New_Face( library, fontfile, atoi(fontindex), aface );
    if(ret) {
        dbg(lvl_error,"Error while creating freetype face: %d", ret);
        return ret;
    }
    if((ret = FT_Select_Charmap(*aface, FT_ENCODING_UNICODE))) {
        dbg(lvl_error,"Error while creating freetype face: %d", ret);
    }
    return 0;
}
#endif

/** Implementation of font_freetype_methods.font_new */
static struct font_freetype_font *font_freetype_font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, char *fontfamily, int size, int flags) {
    struct font_freetype_font *font =
        g_new(struct font_freetype_font, 1);
    int exact, found=0;
    char **family, **family_sav;
#if USE_CACHING
    char *idstr;
    FT_Face face;
#endif
#ifndef HAVE_FONTCONFIG
    char *name;
#endif
    *meth = font_methods;

    if (!library_init) {
        FT_Init_FreeType(&library);
#if USE_CACHING
        FTC_Manager_New( library, 0, 0, 0, &face_requester, NULL, &manager);
        FTC_ImageCache_New( manager, &image_cache);
        FTC_CMapCache_New( manager, &charmap_cache);
        FTC_SBitCache_New( manager, &sbit_cache);
#endif
        library_init = 1;
    }
    font->size=size;
#ifdef HAVE_FONTCONFIG
    dbg(lvl_info, " about to search for fonts, preferred = %s", fontfamily);
    family = g_malloc(sizeof(fontfamilies) + sizeof(fontfamily));
    if (fontfamily) {
        memcpy(family, &fontfamily, sizeof(fontfamily));
        memcpy(family + 1, fontfamilies, sizeof(fontfamilies));
    } else {
        memcpy(family, fontfamilies, sizeof(fontfamilies));
    }
    family_sav=family;
    for (exact = 1; !found && exact >= 0; exact--) {
        family=family_sav;


        while (*family && !found) {
            dbg(lvl_info, "Looking for font family %s. exact=%d", *family, exact);
            FcPattern *required = FcPatternBuild(NULL, FC_FAMILY, FcTypeString, *family, NULL);
            if (flags)
                FcPatternAddInteger(required, FC_WEIGHT, FC_WEIGHT_BOLD);
            FcConfigSubstitute(FcConfigGetCurrent(), required, FcMatchFont);
            FcDefaultSubstitute(required);
            FcResult result;
            FcPattern *matched =
                FcFontMatch(FcConfigGetCurrent(), required, &result);
            if (matched) {
                FcValue v1, v2;
                FcChar8 *fontfile;
                int fontindex;
                FcPatternGet(required, FC_FAMILY, 0, &v1);
                FcPatternGet(matched, FC_FAMILY, 0, &v2);
                FcResult r1 =
                    FcPatternGetString(matched, FC_FILE, 0, &fontfile);
                FcResult r2 =
                    FcPatternGetInteger(matched, FC_INDEX, 0, &fontindex);
                if ((r1 == FcResultMatch)
                        && (r2 == FcResultMatch)
                        && (FcValueEqual(v1, v2) || !exact)) {
                    dbg(lvl_info, "About to load font from file %s index %d", fontfile, fontindex);
#if USE_CACHING
                    idstr=g_strdup_printf("%s/%d", fontfile, fontindex);
                    font->scaler.face_id=(FTC_FaceID)atom(idstr);
                    g_free(idstr);
#if HAVE_LOOKUP_SCALER
                    font->scaler.width=0;
                    font->scaler.height=size;
                    font->scaler.pixel=0;
                    font->scaler.x_res=300;
                    font->scaler.y_res=300;
#else
                    font->scaler.width=size/15;
                    font->scaler.height=size/15;
                    font->scaler.flags=FT_LOAD_DEFAULT;
#endif
                    FTC_Manager_LookupFace(manager, font->scaler.face_id, &face);
                    font->charmap_index=face->charmap ? FT_Get_Charmap_Index(face->charmap) : 0;
#else /* USE_CACHING */
                    FT_New_Face(library, (char *) fontfile, fontindex, &font->face);
#endif /* USE_CACHING */
                    found = 1;
                }
                FcPatternDestroy(matched);
            }
            FcPatternDestroy(required);
            family++;
        }
    }
    g_free(family_sav);
#else /* HAVE_FONTCONFIG */
#ifdef FREETYPE_FONTS
    {
        char *fonts[]= {FREETYPE_FONTS};
        name=g_strdup(fonts[flags ? 1:0]);
    }
#else
    name=g_strdup_printf("%s/fonts/%s-%s.ttf",getenv("NAVIT_SHAREDIR"),"LiberationSans",flags ? "Bold":"Regular");
#endif

#if USE_CACHING
    idstr=g_strdup_printf("%s/%d", name, 0);
    font->scaler.face_id=(FTC_FaceID)atom(idstr);
    g_free(idstr);
    FTC_Manager_LookupFace(manager, font->scaler.face_id, &face);
    font->charmap_index=face->charmap ? FT_Get_Charmap_Index(face->charmap) : 0;
#if HAVE_LOOKUP_SCALER
    font->scaler.width=0;
    font->scaler.height=size;
    font->scaler.pixel=0;
    font->scaler.x_res=300;
    font->scaler.y_res=300;
#else
    font->scaler.width=size/15;
    font->scaler.height=size/15;
    font->scaler.flags=FT_LOAD_DEFAULT;
#endif
    found=1;
    FTC_Manager_LookupFace(manager, font->scaler.face_id, &face);
    font->charmap_index=face->charmap ? FT_Get_Charmap_Index(face->charmap) : 0;
#else /* USE_CACHING */
    if (!FT_New_Face(library, name, 0, &font->face))
        found=1;
#endif /* USE_CACHING */
    g_free(name);
#endif /* HAVE_FONTCONFIG */
    if (!found) {
        dbg(lvl_error,"Failed to load font, no labelling");
        g_free(font);
        return NULL;
    }
#if !USE_CACHING
    FT_Set_Char_Size(font->face, 0, size, 300, 300);
    FT_Select_Charmap(font->face, FT_ENCODING_UNICODE);
#endif
    return font;
}

/** Implementation of font_freetype_methods.get_shadow. */
static int font_freetype_glyph_get_shadow(struct font_freetype_glyph *g, unsigned char *data, int stride, struct color *foreground, struct color *background) {
    int x, y, w = g->w, h = g->h;
    unsigned int bg, fg;
    unsigned char *pm, *psp,*ps,*psn;
    fg=((foreground->a>>COL_SHIFT)<<24)|
       ((foreground->r>>COL_SHIFT)<<16)|
       ((foreground->g>>COL_SHIFT)<<8)|
       ((foreground->b>>COL_SHIFT)<<0);
    bg=((background->a>>COL_SHIFT)<<24)|
       ((background->r>>COL_SHIFT)<<16)|
       ((background->g>>COL_SHIFT)<<8)|
       ((background->b>>COL_SHIFT)<<0);
    for (y = 0; y < h+2; y++) {
        if (stride) {
            ps = data + stride * y;
        } else {
            unsigned char **dataptr=(unsigned char **)data;
            ps = dataptr[y];
        }
        for (x = 0 ; x < w+2 ; x++)
            ((unsigned int *)ps)[x]=bg;
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
    }
    return 1;
}

/** Implementation of font_freetype_methods.get_glyph. */
static int font_freetype_glyph_get_glyph(struct font_freetype_glyph *g, unsigned char *data, int stride, struct color *fg, struct color *bg, struct color *transparent) {
    int x, y, w = g->w, h = g->h;
    unsigned int tr;
    unsigned char v,vi,*pm, *ps;
    tr=((transparent->a>>COL_SHIFT)<<24)|
       ((transparent->r>>COL_SHIFT)<<16)|
       ((transparent->g>>COL_SHIFT)<<8)|
       ((transparent->b>>COL_SHIFT)<<0);
    for (y = 0; y < h; y++) {
        pm = g->pixmap + y * w;
        if (stride) {
            ps = data + stride*y;
        } else {
            unsigned char **dataptr=(unsigned char **)data;
            ps = dataptr[y];
        }
        for (x = 0; x < w; x++) {
            v=*pm;
            if (v) {
                vi=255-v;
                ((unsigned int *)ps)[0]=
                    ((((fg->a*v+bg->a*vi)/255)>>COL_SHIFT)<<24)|
                    ((((fg->r*v+bg->r*vi)/255)>>COL_SHIFT)<<16)|
                    ((((fg->g*v+bg->g*vi)/255)>>COL_SHIFT)<<8)|
                    ((((fg->b*v+bg->b*vi)/255)>>COL_SHIFT)<<0);
            } else
                ((unsigned int *)ps)[0]=tr;
            ps+=4;
            pm++;
        }
    }
    return 1;
}

static void font_freetype_destroy(void) {
    // Do not call FcFini here: GdkPixbuf also (indirectly) uses fontconfig (for SVGs with
    // text), but does not properly deallocate all objects, so FcFini assert()s.
    if (!library_deinit) {
#if USE_CACHING
        FTC_Manager_Done(manager);
#endif
        FT_Done_FreeType(library);
    }
    library_deinit = 1;
}

static struct font_freetype_methods methods = {
    font_freetype_destroy,
    font_freetype_font_new,
    font_freetype_get_text_bbox,
    font_freetype_text_new,
    font_freetype_text_destroy,
    font_freetype_glyph_get_shadow,
    font_freetype_glyph_get_glyph,
};

static struct font_priv *font_freetype_new(void *meth) {
    *((struct font_freetype_methods *) meth) = methods;
    return &dummy;
}

void plugin_init(void) {
    plugin_register_category_font("freetype", font_freetype_new);
#ifdef HAVE_FONTCONFIG
    FcInit();
#endif
}
