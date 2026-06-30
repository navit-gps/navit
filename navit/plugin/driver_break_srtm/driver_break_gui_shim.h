/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Runtime resolution of gui_internal symbols for Driver Break dialogs.
 * Include after gui/internal/gui_internal_menu.h.
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

#    define gui_internal_back(p, w, d)                                                                                 \
        do {                                                                                                           \
            driver_break_gui_shim_resolve();                                                                           \
            if (driver_break_gui_shim_back_ptr)                                                                        \
                driver_break_gui_shim_back_ptr(p, w, d);                                                               \
        } while (0)
#    define gui_internal_enter(p, i)                                                                                   \
        do {                                                                                                           \
            driver_break_gui_shim_resolve();                                                                           \
            if (driver_break_gui_shim_enter_ptr)                                                                       \
                driver_break_gui_shim_enter_ptr(p, i);                                                                 \
        } while (0)
#    define gui_internal_leave(p)                                                                                      \
        do {                                                                                                           \
            driver_break_gui_shim_resolve();                                                                           \
            if (driver_break_gui_shim_leave_ptr)                                                                       \
                driver_break_gui_shim_leave_ptr(p);                                                                    \
        } while (0)
#    define gui_internal_set_click_coord(p, pt)                                                                        \
        do {                                                                                                           \
            driver_break_gui_shim_resolve();                                                                           \
            if (driver_break_gui_shim_set_click_coord_ptr)                                                             \
                driver_break_gui_shim_set_click_coord_ptr(p, pt);                                                      \
        } while (0)
#    define gui_internal_enter_setup(p)                                                                                \
        do {                                                                                                           \
            driver_break_gui_shim_resolve();                                                                           \
            if (driver_break_gui_shim_enter_setup_ptr)                                                                 \
                driver_break_gui_shim_enter_setup_ptr(p);                                                              \
        } while (0)
#    define gui_internal_menu(p, s)                                                                                    \
        (driver_break_gui_shim_resolve(),                                                                              \
         driver_break_gui_shim_menu_ptr ? driver_break_gui_shim_menu_ptr((p), (s)) : NULL)
#    define gui_internal_box_new(p, f)                                                                                 \
        (driver_break_gui_shim_resolve(),                                                                              \
         driver_break_gui_shim_box_new_ptr ? driver_break_gui_shim_box_new_ptr((p), (f)) : NULL)
#    define gui_internal_label_new(p, s)                                                                               \
        (driver_break_gui_shim_resolve(),                                                                              \
         driver_break_gui_shim_label_new_ptr ? driver_break_gui_shim_label_new_ptr((p), (s)) : NULL)
#    define gui_internal_button_new_with_callback(p, s, w, f, cb, d)                                                   \
        (driver_break_gui_shim_resolve(),                                                                              \
         driver_break_gui_shim_button_new_with_callback_ptr                                                            \
             ? driver_break_gui_shim_button_new_with_callback_ptr((p), (s), (w), (f), (cb), (d))                       \
             : NULL)
#    define gui_internal_widget_append(w1, w2)                                                                         \
        do {                                                                                                           \
            driver_break_gui_shim_resolve();                                                                           \
            if (driver_break_gui_shim_widget_append_ptr)                                                               \
                driver_break_gui_shim_widget_append_ptr((w1), (w2));                                                   \
        } while (0)
#    define gui_internal_menu_render(p)                                                                                \
        do {                                                                                                           \
            driver_break_gui_shim_resolve();                                                                           \
            if (driver_break_gui_shim_menu_render_ptr)                                                                 \
                driver_break_gui_shim_menu_render_ptr(p);                                                              \
        } while (0)

#endif /* !HAVE_API_ANDROID */

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_GUI_SHIM_H */
