/* prototypes */
struct callback;
struct color;
struct coord;
struct cursor;
struct graphics;
struct transformation;
struct vehicle;
struct coord *cursor_pos_get(struct cursor *this);
void cursor_pos_set(struct cursor *this, struct coord *pos);
void cursor_redraw(struct cursor *this);
int cursor_get_dir(struct cursor *this);
int cursor_get_speed(struct cursor *this);
struct cursor *cursor_new(struct graphics *gra, struct vehicle *v, struct color *c, struct transformation *t);
void cursor_add_callback(struct cursor *this, struct callback *cb);
/* end of prototypes */
