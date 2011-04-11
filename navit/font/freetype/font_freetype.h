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
struct font_freetype_font;
struct font_freetype_glyph;

struct font_freetype_methods {
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
	int (*get_shadow) (struct font_freetype_glyph * glyph,
			   unsigned char *data, int depth, int stride, struct color *fg, struct color *tr);
	int (*get_glyph) (struct font_freetype_glyph * glyph,
			   unsigned char *data, int depth, int stride,
			   struct color * fg, struct color * bg, struct color *tr);
};

struct font_freetype_glyph {
	int x, y, w, h, dx, dy;
	unsigned char *pixmap;
};

struct font_freetype_text {
	int x1, y1;
	int x2, y2;
	int x3, y3;
	int x4, y4;
	int glyph_count;
	struct font_freetype_glyph *glyph[0];
};
