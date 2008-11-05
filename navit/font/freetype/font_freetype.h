
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
