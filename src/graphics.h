
struct point;
struct container;
struct graphics_gc;
struct graphics_font;
struct graphics_image_gra;

struct graphics_image {
	struct graphics_image *next;
	struct graphics *gr;
	char *name;
	int height;
	int width;
	struct graphics_image_gra *gra;
};

void container_init_gra(struct container *co);

void graphics_get_view(struct container *co, long *x, long *y, unsigned long *scale);
void graphics_set_view(struct container *co, long *x, long *y, unsigned long *scale);
void graphics_resize(struct container *co, int w, int h);
void graphics_redraw(struct container *co);

enum draw_mode_num {
	draw_mode_begin, draw_mode_end, draw_mode_cursor
};

struct graphics
{
	struct graphics_gra *gra;
	struct graphics_font **font;
	struct graphics_gc **gc;

	void (*draw_mode)(struct graphics *gr, enum draw_mode_num mode);
	void (*draw_lines)(struct graphics *gr, struct graphics_gc *gc, struct point *p, int count);
	void (*draw_polygon)(struct graphics *gr, struct graphics_gc *gc, struct point *p, int count);
	void (*draw_rectangle)(struct graphics *gr, struct graphics_gc *gc, struct point *p, int w, int h);
	void (*draw_circle)(struct graphics *gr, struct graphics_gc *gc, struct point *p, int r);
	void (*draw_text)(struct graphics *gr, struct graphics_gc *fg, struct graphics_gc *bg, struct graphics_font *font, char *text, struct point *p, int dx, int dy);
	void (*draw_image)(struct graphics *gr, struct graphics_gc *fg, struct point *p, struct graphics_image *img);
	void (*draw_restore)(struct graphics *gr, struct point *p, int w, int h);

	struct graphics_font *(*font_new)(struct graphics *gr, int size);
	struct graphics_gc *(*gc_new)(struct graphics *gr);
	void (*gc_set_linewidth)(struct graphics_gc *gc, int width);
	void (*gc_set_foreground)(struct graphics_gc *gc, int r, int g, int b);
	void (*gc_set_background)(struct graphics_gc *gc, int r, int g, int b);
	struct graphics_image *(*image_new)(struct graphics *gr, char *path);
	struct graphics *(*overlay_new)(struct graphics *gr, struct point *p, int w, int h);
};
