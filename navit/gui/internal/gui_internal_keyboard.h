/* prototypes */
struct gui_priv;
struct widget;
struct widget *gui_internal_keyboard_do(struct gui_priv *this, struct widget *wkbdb, int mode);
struct widget *gui_internal_keyboard(struct gui_priv *this, int mode);
int gui_internal_keyboard_init_mode(char *lang);
void gui_internal_keyboard_to_upper_case(struct gui_priv *this);
void gui_internal_keyboard_to_lower_case(struct gui_priv *this);
/* end of prototypes */
