/* prototypes */
struct gui_priv;
struct menu_data;
struct widget;
int gui_internal_menu_needs_resizing(struct gui_priv *this, struct widget *w, int wdisp, int hdisp);
void gui_internal_menu_destroy(struct gui_priv *this, struct widget *w);
int gui_internal_widget_reload_href(struct gui_priv *this, struct widget *w);
void gui_internal_prune_menu(struct gui_priv *this, struct widget *w);
void gui_internal_prune_menu_count(struct gui_priv *this, int count, int render);
void gui_internal_menu_menu_resize(struct gui_priv *this, struct widget *w, void *data, int neww, int newh);
struct widget *gui_internal_menu(struct gui_priv *this, const char *label);
struct menu_data *gui_internal_menu_data(struct gui_priv *this);
void gui_internal_menu_reset_pack(struct gui_priv *this);
void gui_internal_menu_render(struct gui_priv *this);
void gui_internal_menu_resize(struct gui_priv *this, int w, int h);
struct widget *gui_internal_top_bar(struct gui_priv *this);
/* end of prototypes */
