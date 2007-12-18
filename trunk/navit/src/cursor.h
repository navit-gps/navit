#ifndef NAVIT_CURSOR_H
#define NAVIT_CURSOR_H

/* prototypes */
struct color;
struct cursor;
struct graphics;
struct point;
void cursor_draw(struct cursor *this_, struct point *pnt, int dir, int draw_dir, int force);
struct cursor *cursor_new(struct graphics *gra, struct color *c);
void cursor_destroy(struct cursor *this_);
/* end of prototypes */

#endif
