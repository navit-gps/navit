/*
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

/** @file
 * @brief Interface to the FreeType 2 library, to render text as bitmaps.
 *
 * This plugin supplies functions to render text (encoded as UTF-8) to bitmaps with the help of
 * FreeType 2. Typical usage:
 * @li create a struct font_freetype_font by calling font_freetype_methods.font_new()
 * @li for the text to render, create a struct font_freetype_text by calling font_freetype_methods.text_new()
 * @li obtain glyph bitmaps by calling font_freetype_methods.get_glyph() on the glyphs inside
 * struct font_freetype_text
 * @li optionally, obtain a "shadow" of the glyph from font_freetype_methods.get_shadow(), to make
 * the text easier to read against a colored background (like the map)
 */
struct font_freetype_font;
struct font_freetype_glyph;

/** Methods provided by this plugin. */
struct font_freetype_methods {
    void (*destroy)(void);
	/**
	 * @brief Load a font, preferring one with the given font family.
	 *
	 * Try to load a font, trying to match either the given font family, or one from a list
	 * of hardcoded families, or let fontconfig pick the best match.
	 * @param graphics_priv unused
	 * @param graphics_font_methods used to return font methods
	 * @param fontfamily the preferred font family
	 * @param size requested size of font
	 * @param flags extra flags for the font (bold,etc)
	 * @returns loaded font, or NULL
	*/
	struct font_freetype_font *(*font_new) (struct graphics_priv * gr,
						struct
						graphics_font_methods *
						meth, char *font, int size,
						int flags);
	void (*get_text_bbox) (struct graphics_priv * gr,
			       struct font_freetype_font * font,
			       char *text, int dx, int dy,
			       struct point * ret, int estimate);
	struct font_freetype_text *(*text_new) (char *text,
						struct font_freetype_font *
						font, int dx, int dy);
	void (*text_destroy) (struct font_freetype_text * text);
	/**
	 * @brief Get glyph "shadow", a glyph bitmap to be used as background.
	 *
	 * This method returns a glyph shadow, a bitmap with the glyph, where the glyph has been
	 * "fattened" by expanding it by one pixel on each side.
	 * In Navit, the shadow is used as the background behind the glyph bitmaps (returned by
	 * font_freetype_methods.get_glyph() ), to make the text easier to read.
	 *
	 * @param g glyph to render, usually created via font_freetype_methods.text_new()
	 * @param data buffer for result image bitmap. Size must be at least (4 * stride * (g->h+2)).
	 * @param stride see font_freetype_methods.get_glyph(). Minimum: g->w+2.
	 * @param foreground color for rendering the "shadow"
	 * @param background color for rest of the bitmap (typically set to transparent)
	 * @returns 0 if depth is invalid, 1 otherwise
	 */
	int (*get_shadow) (struct font_freetype_glyph * glyph,
			   unsigned char *data, int stride, struct color *fg, struct color *tr);
	/**
	 * @brief Get a glyph bitmap.
	 *
	 * This method returns a bitmap for rendering the supplied glyph.
	 *
	 * @param g glyph to render, usually obtained from a struct font_freetype_text created via font_freetype_methods.text_new()
	 * @param data buffer for result image bitmap. Size must be at least (4 * stride * g->h).
	 * @param stride stride (bytes per data row) for result bitmap; must be at least g->w, but may include padding.
	 * Special case:
	 * If set to 0, 'data' is interpreted as an array of pointers to image data rows (i.e. unsigned char**).
	 * @param fg color for rendering the glyph
	 * @param bg color to alpha blend with fg for semi-transparent glyph pixels
	 * @param transparent color for background pixels
	 * @returns 0 if depth is invalid, 1 otherwise
	 */
	int (*get_glyph) (struct font_freetype_glyph * glyph,
			   unsigned char *data, int stride,
			   struct color * fg, struct color * bg, struct color *tr);
};

struct font_freetype_glyph {
	int x, y, w, h, dx, dy;
	unsigned char *pixmap;
};

struct font_freetype_text {
	int glyph_count;
	struct font_freetype_glyph *glyph[0];
};
