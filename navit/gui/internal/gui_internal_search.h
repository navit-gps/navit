/* prototypes */
struct gui_priv;
struct widget;
void gui_internal_search_idle_end(struct gui_priv *this);
void gui_internal_search_list_destroy(struct gui_priv *this);
void gui_internal_search(struct gui_priv *this, const char *what, const char *type, int flags);
void gui_internal_search_house_number_in_street(struct gui_priv *this, struct widget *widget, void *data);
void gui_internal_search_street_in_town(struct gui_priv *this, struct widget *widget, void *data);
void gui_internal_search_town_in_country(struct gui_priv *this, struct widget *widget);
/* end of prototypes */
