/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
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

#ifndef PLUGIN_C

#ifdef __cplusplus
extern "C" {
#endif

struct plugin;

/**
 * @brief All existing plugin categories.
 *
 * Plugins are grouped into categories. Plugins within one category offer the same
 * functionality (GUI, graphics, map etc.). Consequently the category determines the API
 * offered by a plugin.
 */
enum plugin_category {
	/** Category for plugins which implement a graphics backend. */
	plugin_category_graphics,
	/** Category for plugins which implement a GUI frontend. */
	plugin_category_gui,
	/** Category for plugins which implement a driver for providing/loading map data. */
	plugin_category_map,
	/** Category for plugins which implement an OSD. */
	plugin_category_osd,
	/** Category for plugins which implement speech output. */
	plugin_category_speech,
	/** Category for plugins which supply position data (typically from a GPS receiver). */
	plugin_category_vehicle,
	/** Category for plugins which implement/wrap an event subsystem. */
	plugin_category_event,
	/** Category for plugins which load fonts. */
	plugin_category_font,
	/** Dummy for last entry. */
	plugin_category_last,
};
#endif

struct container;
struct popup;
struct popup_item;
#undef PLUGIN_FUNC1
#undef PLUGIN_FUNC3
#undef PLUGIN_FUNC4
#undef PLUGIN_CATEGORY
#define PLUGIN_PROTO(name,...) void name(__VA_ARGS__)

#ifdef PLUGIN_C
#define PLUGIN_REGISTER(name,...)						\
void										\
plugin_register_##name(PLUGIN_PROTO((*func),__VA_ARGS__))				\
{										\
        plugin_##name##_func=func;						\
}

#define PLUGIN_CALL(name,...)						\
{										\
	if (plugin_##name##_func)						\
		(*plugin_##name##_func)(__VA_ARGS__);					\
}

#define PLUGIN_FUNC1(name,t1,p1)				\
PLUGIN_PROTO((*plugin_##name##_func),t1 p1);			\
void plugin_call_##name(t1 p1) PLUGIN_CALL(name,p1)		\
PLUGIN_REGISTER(name,t1 p1)

#define PLUGIN_FUNC3(name,t1,p1,t2,p2,t3,p3)					\
PLUGIN_PROTO((*plugin_##name##_func),t1 p1,t2 p2,t3 p3);				\
void plugin_call_##name(t1 p1,t2 p2, t3 p3) PLUGIN_CALL(name,p1,p2,p3)	\
PLUGIN_REGISTER(name,t1 p1,t2 p2,t3 p3)

#define PLUGIN_FUNC4(name,t1,p1,t2,p2,t3,p3,t4,p4)					\
PLUGIN_PROTO((*plugin_##name##_func),t1 p1,t2 p2,t3 p3,t4 p4);				\
void plugin_call_##name(t1 p1,t2 p2, t3 p3, t4 p4) PLUGIN_CALL(name,p1,p2,p3,p4)	\
PLUGIN_REGISTER(name,t1 p1,t2 p2,t3 p3,t4 p4)

struct name_val {
	char *name;
	void *val;
};

GList *plugin_categories[plugin_category_last];

#define PLUGIN_CATEGORY(category,newargs) \
struct category##_priv; \
struct category##_methods; \
void \
plugin_register_category_##category(const char *name, struct category##_priv *(*new_) newargs) \
{ \
        struct name_val *nv; \
        nv=g_new(struct name_val, 1); \
        nv->name=g_strdup(name); \
	nv->val=new_; \
	plugin_categories[plugin_category_##category]=g_list_append(plugin_categories[plugin_category_##category], nv); \
} \
 \
void * \
plugin_get_category_##category(const char *name) \
{ \
	return plugin_get_category(plugin_category_##category, #category, name); \
}

#else
#define PLUGIN_FUNC1(name,t1,p1)			\
void plugin_register_##name(void(*func)(t1 p1));	\
void plugin_call_##name(t1 p1);

#define PLUGIN_FUNC3(name,t1,p1,t2,p2,t3,p3)			\
void plugin_register_##name(void(*func)(t1 p1,t2 p2,t3 p3));	\
void plugin_call_##name(t1 p1,t2 p2,t3 p3);

#define PLUGIN_FUNC4(name,t1,p1,t2,p2,t3,p3,t4,p4)			\
void plugin_register_##name(void(*func)(t1 p1,t2 p2,t3 p3,t4 p4));	\
void plugin_call_##name(t1 p1,t2 p2,t3 p3,t4 p4);

#define PLUGIN_CATEGORY(category,newargs) \
struct category##_priv; \
struct category##_methods; \
void plugin_register_category_##category(const char *name, struct category##_priv *(*new_) newargs); \
void *plugin_get_category_##category(const char *name);

#endif

#include "plugin_def.h"

#ifndef USE_PLUGINS
#define plugin_module_cat3(pre,mod,post) pre##mod##post
#define plugin_module_cat2(pre,mod,post) plugin_module_cat3(pre,mod,post)
#define plugin_module_cat(pre,post) plugin_module_cat2(pre,MODULE,post)
#define plugin_init plugin_module_cat(module_,_init)
#endif

struct attr;

/* prototypes */
void plugin_init(void);
int plugin_load(struct plugin *pl);
char *plugin_get_name(struct plugin *pl);
int plugin_get_active(struct plugin *pl);
void plugin_set_active(struct plugin *pl, int active);
void plugin_set_lazy(struct plugin *pl, int lazy);
void plugin_call_init(struct plugin *pl);
void plugin_unload(struct plugin *pl);
void plugin_destroy(struct plugin *pl);
struct plugins *plugins_new(void);
struct plugin *plugin_new(struct attr *parent, struct attr ** attrs);
void plugins_init(struct plugins *pls);
void plugins_destroy(struct plugins *pls);
void *plugin_get_category(enum plugin_category category, const char *category_name, const char *name);
/* end of prototypes */

#ifdef __cplusplus
}
#endif


