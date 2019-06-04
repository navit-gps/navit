enum gui_internal_reason {
    gui_internal_reason_click=1,
    gui_internal_reason_keypress,
    gui_internal_reason_keypress_finish
};

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct widget {
    enum widget_type type;
    struct graphics_gc *background,*text_background;
    struct graphics_gc *foreground_frame;
    struct graphics_gc *foreground;
    char *text;
    struct graphics_image *img;
    /**
     * A function to be invoked on actions.
     * @param widget The widget that is receiving the button press.
     *
     */
    void (*func)(struct gui_priv *priv, struct widget *widget, void *data);
    /**
     * A function to be invoked on resize or move
     * @param widget The widget that is resized
     */
    void (*on_resize)(struct gui_priv *priv, struct widget *widget, void *data, int neww, int newh);
    enum gui_internal_reason reason;
    int datai;
    void *data;
    /**
     * @brief A function to deallocate data
     */
    void (*data_free)(void *data);

    /**
     * @brief a function that will be called as the widget is being destroyed.
     * This function can act as a destructor for the widget. It allows for
     * on deallocation actions to be specified on a per widget basis.
     * This function will call g_free on the widget (if required).
     */
    void (*wfree) (struct gui_priv *this_, struct widget * w);
    char *prefix;
    char *name;
    char *speech;
    char *command;
    struct pcoord c;
    struct item item;
    int selection_id;
    int state;
    struct point p;
    int wmin,hmin;
    int w,h;
    int textw,texth;
    int font_idx;
    int bl,br,bt,bb,spx,spy;
    int border;
    int packed;
    /**
     * The number of widgets to layout horizontally when doing
     * a orientation_horizontal_vertical layout
     */
    int cols;
    enum flags flags;
    int flags2;
    void *instance;
    int (*set_attr)(void *, struct attr *);
    int (*get_attr)(void *, enum attr_type, struct attr *, struct attr_iter *);
    void (*remove_cb)(void *, struct callback *cb);
    struct callback *cb;
    struct attr on;
    struct attr off;
    int deflt;
    int is_on;
    int redraw;
    struct menu_data *menu_data;
    struct form *form;
    GList *children;
    struct widget *parent;
    struct scroll_buttons *scroll_buttons;
};

struct scroll_buttons {
    /**
    * Button box should not be displayed if button_box_hide is not zero.
    */
    int button_box_hide;
    /**
    * A container box that is the child of the table widget that contains+groups
    * the next and previous button.
    */
    struct widget * button_box;
    /**
    * A button widget to handle 'next page' requests
    */
    struct widget * next_button;
    /**
    * A button widget to handle 'previous page' requests.
    */
    struct widget * prev_button;
    /**
    * a pointer to the gui context.
    * This is needed by the free function to destroy the buttons.
    */
    struct  gui_priv *  this;
};

/**
 * @brief A structure to store information about a table.
 *
 * The table_data widget stores pointers to extra information needed by the
 * table widget.
 *
 * The table_data structure needs to be freed with data_free along with the widget.
 *
 */
struct table_data {
    /**
     * A GList pointer into a widget->children list that indicates the row
     * currently being rendered at the top of the table.
     */
    GList * top_row;
    /**
     * A Glist pointer into a widget->children list that indicates the row
     * currently being rendered at the bottom of the table.
     */
    GList * bottom_row;

    struct scroll_buttons scroll_buttons;

};

/**
 * A data structure that holds information about a column that makes up a table.
 *
 *
 */
struct table_column_desc {

    /**
     * The computed height of a cell in the table.
     */
    int height;

    /**
     * The computed width of a cell in the table.
     */
    int width;
};
/* prototypes */
enum flags;
struct graphics_image;
struct gui_priv;
struct point;
struct table_data;
struct widget;
void gui_internal_widget_swap(struct gui_priv *this, struct widget *first, struct widget *second);
void gui_internal_widget_move(struct gui_priv *this, struct widget *dst, struct widget *src);
struct widget *gui_internal_label_font_new(struct gui_priv *this, const char *text, int font);
struct widget *gui_internal_label_new(struct gui_priv *this, const char *text);
struct widget *gui_internal_label_new_abbrev(struct gui_priv *this, const char *text, int maxwidth);
struct widget *gui_internal_image_new(struct gui_priv *this, struct graphics_image *image);
struct widget *gui_internal_text_font_new(struct gui_priv *this, const char *text, int font, enum flags flags);
struct widget *gui_internal_text_new(struct gui_priv *this, const char *text, enum flags flags);
struct widget *gui_internal_button_font_new_with_callback(struct gui_priv *this, const char *text, int font,
        struct graphics_image *image, enum flags flags, void (*func)(struct gui_priv *priv, struct widget *widget, void *data),
        void *data);
struct widget *gui_internal_button_new_with_callback(struct gui_priv *this, const char *text,
        struct graphics_image *image, enum flags flags, void (*func)(struct gui_priv *priv, struct widget *widget, void *data),
        void *data);
struct widget *gui_internal_button_new(struct gui_priv *this, const char *text, struct graphics_image *image,
                                       enum flags flags);
struct widget *gui_internal_find_widget(struct widget *wi, struct point *p, int flags);
void gui_internal_highlight_do(struct gui_priv *this, struct widget *found);
void gui_internal_highlight(struct gui_priv *this);
struct widget *gui_internal_box_new_with_label(struct gui_priv *this, enum flags flags, const char *label);
struct widget *gui_internal_box_new(struct gui_priv *this, enum flags flags);
void gui_internal_box_resize(struct gui_priv *this, struct widget *w, void *data, int wnew, int hnew);
void gui_internal_widget_reset_pack(struct gui_priv *this, struct widget *w);
void gui_internal_widget_append(struct widget *parent, struct widget *child);
void gui_internal_widget_prepend(struct widget *parent, struct widget *child);
void gui_internal_widget_insert_sorted(struct widget *parent, struct widget *child, GCompareFunc func);
void gui_internal_widget_children_destroy(struct gui_priv *this, struct widget *w);
void gui_internal_widget_destroy(struct gui_priv *this, struct widget *w);
void gui_internal_widget_render(struct gui_priv *this, struct widget *w);
void gui_internal_widget_resize(struct gui_priv *this, struct widget *w, int wnew, int hnew);
void gui_internal_widget_pack(struct gui_priv *this, struct widget *w);
struct widget *gui_internal_button_label(struct gui_priv *this, const char *label, int mode);
struct widget *gui_internal_widget_table_new(struct gui_priv *this, enum flags flags, int buttons);
void gui_internal_widget_table_clear(struct gui_priv *this, struct widget *table);
GList *gui_internal_widget_table_next_row(GList *row);
GList *gui_internal_widget_table_prev_row(GList *row);
GList *gui_internal_widget_table_first_row(GList *row);
GList *gui_internal_widget_table_top_row(struct gui_priv *this, struct widget *table);
GList *gui_internal_widget_table_set_top_row(struct gui_priv *this, struct widget * table, struct widget *row);
struct widget *gui_internal_widget_table_row_new(struct gui_priv *this, enum flags flags);
void gui_internal_table_pack(struct gui_priv *this, struct widget *w);
void gui_internal_table_hide_rows(struct table_data *table_data);
void gui_internal_table_render(struct gui_priv *this, struct widget *w);
void gui_internal_table_button_next(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_table_button_prev(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_table_data_free(void *p);
/* end of prototypes */
