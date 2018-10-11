struct widget;
struct graphics_image;
struct gui_priv;

#define STATE_INVISIBLE 1
#define STATE_SELECTED 2
#define STATE_HIGHLIGHTED 4
#define STATE_SENSITIVE 8
#define STATE_EDIT 16
#define STATE_CLEAR 32
#define STATE_EDITABLE 64
#define STATE_SCROLLABLE 128
#define STATE_OFFSCREEN 256

#define GESTURE_RINGSIZE 100

enum widget_type {
    widget_box=1,
    widget_button,
    widget_label,
    widget_image,
    widget_table,
    widget_table_row
};

enum flags {
    gravity_none=0x00,
    gravity_left=1,
    gravity_xcenter=2,
    gravity_right=4,
    gravity_top=8,
    gravity_ycenter=16,
    gravity_bottom=32,
    gravity_left_top=gravity_left|gravity_top,
    gravity_top_center=gravity_xcenter|gravity_top,
    gravity_right_top=gravity_right|gravity_top,
    gravity_left_center=gravity_left|gravity_ycenter,
    gravity_center=gravity_xcenter|gravity_ycenter,
    gravity_right_center=gravity_right|gravity_ycenter,
    gravity_left_bottom=gravity_left|gravity_bottom,
    gravity_bottom_center=gravity_xcenter|gravity_bottom,
    gravity_right_bottom=gravity_right|gravity_bottom,
    flags_expand=0x100,
    flags_fill=0x200,
    flags_swap=0x400,
    flags_scrollx=0x800,
    flags_scrolly=0x1000,
    orientation_horizontal=0x10000,
    orientation_vertical=0x20000,
    orientation_horizontal_vertical=0x40000,
};


struct gui_internal_methods {
    void (*add_callback)(struct gui_priv *priv, struct callback *cb);
    void (*remove_callback)(struct gui_priv *priv, struct callback *cb);
    void (*menu_render)(struct gui_priv *this);
    struct graphics_image * (*image_new_xs)(struct gui_priv *this, const char *name);
    struct graphics_image * (*image_new_l)(struct gui_priv *this, const char *name);
};

struct gui_internal_widget_methods {
    void (*append)(struct widget *parent, struct widget *child);
    struct widget * (*button_new)(struct gui_priv *this, const char *text, struct graphics_image *image, enum flags flags);
    struct widget * (*button_new_with_callback)(struct gui_priv *this, const char *text, struct graphics_image *image,
            enum flags flags, void(*func)(struct gui_priv *priv, struct widget *widget, void *data), void *data);
    struct widget * (*box_new)(struct gui_priv *this, enum flags flags);
    struct widget * (*label_new)(struct gui_priv *this, const char *text);
    struct widget * (*image_new)(struct gui_priv *this, struct graphics_image *image);
    struct widget * (*keyboard)(struct gui_priv *this, int mode);
    struct widget * (*menu)(struct gui_priv *this, const char *label);
    enum flags (*get_flags)(struct widget *widget);
    void (*set_flags)(struct widget *widget, enum flags flags);
    int (*get_state)(struct widget *widget);
    void (*set_state)(struct widget *widget, int state);
    void (*set_func)(struct widget *widget, void (*func)(struct gui_priv *priv, struct widget *widget, void *data));
    void (*set_data)(struct widget *widget, void *data);
    void (*set_default_background)(struct gui_priv *this, struct widget *widget);

};

struct gui_internal_data {
    struct gui_priv *priv;
    struct gui_internal_methods *gui;
    struct gui_internal_widget_methods *widget;
};
