/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Runtime resolution of gui_internal symbols for Driver Break dialogs.
 */

#include "driver_break_gui_shim.h"
#include "debug.h"

#ifndef HAVE_API_ANDROID

#    include <dlfcn.h>

void (*driver_break_gui_shim_back_ptr)(struct gui_priv *, struct widget *, void *) = NULL;
void (*driver_break_gui_shim_enter_ptr)(struct gui_priv *, int) = NULL;
void (*driver_break_gui_shim_leave_ptr)(struct gui_priv *) = NULL;
void (*driver_break_gui_shim_set_click_coord_ptr)(struct gui_priv *, struct point *) = NULL;
void (*driver_break_gui_shim_enter_setup_ptr)(struct gui_priv *) = NULL;
struct widget *(*driver_break_gui_shim_menu_ptr)(struct gui_priv *, const char *) = NULL;
struct widget *(*driver_break_gui_shim_box_new_ptr)(struct gui_priv *, enum flags) = NULL;
struct widget *(*driver_break_gui_shim_label_new_ptr)(struct gui_priv *, const char *) = NULL;
struct widget *(*driver_break_gui_shim_button_new_with_callback_ptr)(
    struct gui_priv *, const char *, struct widget *, enum flags, void (*)(struct gui_priv *, struct widget *, void *),
    void *) = NULL;
void (*driver_break_gui_shim_widget_append_ptr)(struct widget *, struct widget *) = NULL;
void (*driver_break_gui_shim_menu_render_ptr)(struct gui_priv *) = NULL;

static int driver_break_gui_shim_resolved;

void driver_break_gui_shim_resolve(void) {
    void *handle = NULL;
    const char *lib_paths[] = {"libgui_internal.so",
                               "libnavit_gui_internal.so",
                               "/usr/local/lib64/navit/gui/libgui_internal.so",
                               "/usr/local/lib/navit/gui/libgui_internal.so",
                               "/usr/lib64/navit/gui/libgui_internal.so",
                               "/usr/lib/navit/gui/libgui_internal.so",
                               "/usr/local/lib64/navit/libgui_internal.so",
                               "/usr/local/lib/navit/libgui_internal.so",
                               NULL};
    int i;
    char *error;

    if (driver_break_gui_shim_resolved)
        return;
    driver_break_gui_shim_resolved = 1;

    dlerror();
    for (i = 0; lib_paths[i] != NULL; i++) {
        handle = dlopen(lib_paths[i], RTLD_LAZY | RTLD_GLOBAL);
        if (handle) {
            dbg(lvl_debug, "Driver Break plugin: Opened gui_internal library: %s", lib_paths[i]);
            break;
        }
        error = dlerror();
        if (error)
            dbg(lvl_debug, "Driver Break plugin: Failed to open %s: %s", lib_paths[i], error);
    }

    if (!handle) {
        dbg(lvl_debug, "Driver Break plugin: Could not open gui_internal library explicitly, trying RTLD_DEFAULT");
        handle = RTLD_DEFAULT;
    }

    dlerror();
    driver_break_gui_shim_back_ptr =
        (void (*)(struct gui_priv *, struct widget *, void *))dlsym(handle, "gui_internal_back");
    driver_break_gui_shim_enter_ptr = (void (*)(struct gui_priv *, int))dlsym(handle, "gui_internal_enter");
    driver_break_gui_shim_leave_ptr = (void (*)(struct gui_priv *))dlsym(handle, "gui_internal_leave");
    driver_break_gui_shim_set_click_coord_ptr =
        (void (*)(struct gui_priv *, struct point *))dlsym(handle, "gui_internal_set_click_coord");
    driver_break_gui_shim_enter_setup_ptr = (void (*)(struct gui_priv *))dlsym(handle, "gui_internal_enter_setup");
    driver_break_gui_shim_menu_ptr = (struct widget * (*)(struct gui_priv *, const char *))
        dlsym(handle, "gui_internal_menu");
    driver_break_gui_shim_box_new_ptr = (struct widget * (*)(struct gui_priv *, enum flags))
        dlsym(handle, "gui_internal_box_new");
    driver_break_gui_shim_label_new_ptr = (struct widget * (*)(struct gui_priv *, const char *))
        dlsym(handle, "gui_internal_label_new");
    driver_break_gui_shim_button_new_with_callback_ptr =
        (struct widget
         * (*)(struct gui_priv *, const char *, struct widget *, enum flags,
               void (*)(struct gui_priv *, struct widget *, void *), void *))
            dlsym(handle, "gui_internal_button_new_with_callback");
    driver_break_gui_shim_widget_append_ptr =
        (void (*)(struct widget *, struct widget *))dlsym(handle, "gui_internal_widget_append");
    driver_break_gui_shim_menu_render_ptr = (void (*)(struct gui_priv *))dlsym(handle, "gui_internal_menu_render");

    error = dlerror();
    if (error)
        dbg(lvl_debug, "Driver Break plugin: dlerror after symbol resolution: %s", error);

    if (!driver_break_gui_shim_menu_render_ptr) {
        dbg(lvl_error,
            "Driver Break plugin: Failed to resolve gui_internal functions - internal GUI may not be available");
    } else {
        dbg(lvl_debug, "Driver Break plugin: Successfully resolved all gui_internal functions");
    }
}

#endif /* !HAVE_API_ANDROID */
