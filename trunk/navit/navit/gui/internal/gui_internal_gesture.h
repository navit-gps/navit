/* prototypes */
struct gui_priv;
struct point;
void gui_internal_gesture_ring_clear(struct gui_priv *this);
void gui_internal_gesture_ring_add(struct gui_priv *this, struct point *p);
int gui_internal_gesture_get_vector(struct gui_priv *this, long long msec, struct point *p0, int *dx, int *dy);
int gui_internal_gesture_do(struct gui_priv *this);
/* end of prototypes */
