/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Runtime resolution of gui_internal symbols for Driver Break dialogs.
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_GUI_SHIM_H
#define NAVIT_PLUGIN_DRIVER_BREAK_GUI_SHIM_H

#ifndef HAVE_API_ANDROID

struct gui_priv;
struct widget;
struct point;
enum flags;

void driver_break_gui_shim_resolve(void);

extern void (*driver_break_gui_shim_back_ptr)(struct gui_priv *, struct widget *, void *);
extern void (*driver_break_gui_shim_enter_ptr)(struct gui_priv *, int);
extern void (*driver_break_gui_shim_leave_ptr)(struct gui_priv *);
extern void (*driver_break_gui_shim_set_click_coord_ptr)(struct gui_priv *, struct point *);
extern void (*driver_break_gui_shim_enter_setup_ptr)(struct gui_priv *);
extern struct widget *(*driver_break_gui_shim_menu_ptr)(struct gui_priv *, const char *);
extern struct widget *(*driver_break_gui_shim_box_new_ptr)(struct gui_priv *, enum flags);
extern struct widget *(*driver_break_gui_shim_label_new_ptr)(struct gui_priv *, const char *);
extern struct widget *(*driver_break_gui_shim_button_new_with_callback_ptr)(
    struct gui_priv *, const char *, struct widget *, enum flags, void (*)(struct gui_priv *, struct widget *, void *),
    void *);
extern void (*driver_break_gui_shim_widget_append_ptr)(struct widget *, struct widget *);
extern void (*driver_break_gui_shim_menu_render_ptr)(struct gui_priv *);

#endif /* !HAVE_API_ANDROID */

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_GUI_SHIM_H */
