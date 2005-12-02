#include "point.h"

struct popup_item;
struct graphics;
struct graphics_gc;
struct graphics_font;

struct display_list {
	struct display_list *next;
	void *data;
	int type;
	int attr;
	char *label;
	int count;
	void (*info)(struct display_list *list, struct popup_item **popup);
	struct point p[0];
};
void *display_add(struct display_list **head, int type, int attr, char *label, int count, struct point *p, void (*info)(struct display_list *list, struct popup_item **popup), void *data, int data_size);

void display_free(struct display_list **list, int count);

void display_draw(struct display_list *list, struct graphics *gr, struct graphics_gc *gc_fill, struct graphics_gc *gc_line);
void display_find(struct point *pnt, struct display_list **in, int in_count, int maxdist, struct display_list **out, int out_count);
void display_labels(struct display_list *list, struct graphics *gr, struct graphics_gc *fg, struct graphics_gc *bg, struct graphics_font *font);
