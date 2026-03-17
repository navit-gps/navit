/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

/**
 * @file plugin_impl.h
 * @brief Plugin implementation macros — included only by plugin.c.
 *
 * This header provides the macro definitions that expand PLUGIN_FUNC* and
 * PLUGIN_CATEGORY into function bodies and global variable definitions.
 * It must be included exactly once, after plugin.h, in plugin.c.
 */

#ifndef NAVIT_PLUGIN_IMPL_H
#define NAVIT_PLUGIN_IMPL_H

#include <glib.h>
#include "plugin.h"

#undef PLUGIN_FUNC1
#undef PLUGIN_FUNC3
#undef PLUGIN_FUNC4
#undef PLUGIN_CATEGORY

#define PLUGIN_REGISTER(name, ...)                                                                                 \
    void plugin_register_##name(PLUGIN_PROTO((*func), __VA_ARGS__)) {                                              \
        plugin_##name##_func = func;                                                                               \
    }

#define PLUGIN_CALL(name, ...)                                                                                     \
    {                                                                                                              \
        if (plugin_##name##_func)                                                                                  \
            (*plugin_##name##_func)(__VA_ARGS__);                                                                  \
    }

#define PLUGIN_FUNC1(name, t1, p1)                                                                                 \
    PLUGIN_PROTO((*plugin_##name##_func), t1 p1);                                                                  \
    void plugin_call_##name(t1 p1) PLUGIN_CALL(name, p1) PLUGIN_REGISTER(name, t1 p1)

#define PLUGIN_FUNC3(name, t1, p1, t2, p2, t3, p3)                                                                 \
    PLUGIN_PROTO((*plugin_##name##_func), t1 p1, t2 p2, t3 p3);                                                    \
    void plugin_call_##name(t1 p1, t2 p2, t3 p3) PLUGIN_CALL(name, p1, p2, p3)                                     \
        PLUGIN_REGISTER(name, t1 p1, t2 p2, t3 p3)

#define PLUGIN_FUNC4(name, t1, p1, t2, p2, t3, p3, t4, p4)                                                         \
    PLUGIN_PROTO((*plugin_##name##_func), t1 p1, t2 p2, t3 p3, t4 p4);                                             \
    void plugin_call_##name(t1 p1, t2 p2, t3 p3, t4 p4) PLUGIN_CALL(name, p1, p2, p3, p4)                          \
        PLUGIN_REGISTER(name, t1 p1, t2 p2, t3 p3, t4 p4)

struct name_val {
    char *name;
    void *val;
};

GList *plugin_categories[plugin_category_last];

#define PLUGIN_CATEGORY(category, newargs)                                                                         \
    struct category##_priv;                                                                                        \
    struct category##_methods;                                                                                     \
    void plugin_register_category_##category(const char *name, struct category##_priv *(*new_)newargs) {           \
        struct name_val *nv;                                                                                       \
        nv = g_new(struct name_val, 1);                                                                            \
        nv->name = g_strdup(name);                                                                                 \
        nv->val = new_;                                                                                            \
        plugin_categories[plugin_category_##category] =                                                            \
            g_list_append(plugin_categories[plugin_category_##category], nv);                                      \
    }                                                                                                              \
                                                                                                                   \
    void *plugin_get_category_##category(const char *name) {                                                       \
        return plugin_get_category(plugin_category_##category, #category, name);                                   \
    }

#include "plugin_def.h"

#endif /* NAVIT_PLUGIN_IMPL_H */
