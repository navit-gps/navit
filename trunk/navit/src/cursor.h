/* prototypes */
struct color;
struct coord;
struct cursor;
struct graphics;
struct transformation;
struct vehicle;
struct coord *cursor_pos_get(struct cursor *this);
void cursor_pos_set(struct cursor *this, struct coord *pos);
int cursor_get_dir(struct cursor *this);
int cursor_get_speed(struct cursor *this);
struct cursor *cursor_new(struct graphics *gra, struct vehicle *v, struct color *c, struct transformation *t);
void cursor_register_offscreen_callback(struct cursor *this, void (*func)(struct cursor *cursor, void *data), void *data);
void cursor_register_update_callback(struct cursor *this, void (*func)(struct cursor *cursor, void *data), void *data);
