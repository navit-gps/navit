/* prototypes */
struct gui_priv;
struct widget;
void gui_internal_cmd_add_bookmark2(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_add_bookmark_folder2(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_rename_bookmark(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_cut_bookmark(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_copy_bookmark(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_paste_bookmark(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_delete_bookmark_folder(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_load_bookmarks_as_waypoints(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_replace_bookmarks_from_waypoints(struct gui_priv *this, struct widget *wm, void *data);
/* end of prototypes */
