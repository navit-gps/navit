#ifndef PLUGIN_C

#ifdef __cplusplus
extern "C" {
#endif

struct plugin;

enum plugin_type {
	plugin_type_graphics,
	plugin_type_gui,
	plugin_type_map,
	plugin_type_osd,
	plugin_type_speech,
	plugin_type_vehicle,
	plugin_type_last,
};
#endif

struct container;
struct popup;
struct popup_item;
#undef PLUGIN_FUNC1
#undef PLUGIN_FUNC3
#undef PLUGIN_FUNC4
#undef PLUGIN_TYPE
#define PLUGIN_PROTO(name,args...) void name(args)

#ifdef PLUGIN_C
#define PLUGIN_REGISTER(name,args...)						\
void										\
plugin_register_##name(PLUGIN_PROTO((*func),args))				\
{										\
        plugin_##name##_func=func;						\
}

#define PLUGIN_CALL(name,args...)						\
{										\
	if (plugin_##name##_func)						\
		(*plugin_##name##_func)(args);					\
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

GList *plugin_types[plugin_type_last];

#define PLUGIN_TYPE(type,newargs) \
struct type##_priv; \
struct type##_methods; \
void \
plugin_register_##type##_type(const char *name, struct type##_priv *(*new_) newargs) \
{ \
        struct name_val *nv; \
        nv=g_new(struct name_val, 1); \
        nv->name=g_strdup(name); \
	nv->val=new_; \
	plugin_types[plugin_type_##type]=g_list_append(plugin_types[plugin_type_##type], nv); \
} \
 \
void * \
plugin_get_##type##_type(const char *name) \
{ \
	return plugin_get_type(plugin_type_##type, name); \
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

#define PLUGIN_TYPE(type,newargs) \
struct type##_priv; \
struct type##_methods; \
void plugin_register_##type##_type(const char *name, struct type##_priv *(*new_) newargs); \
void *plugin_get_##type##_type(const char *name);

#endif

#include "plugin_def.h"

void plugin_init(void);
/* prototypes */
struct plugin *plugin_new(char *plugin);
int plugin_load(struct plugin *pl);
char *plugin_get_name(struct plugin *pl);
int plugin_get_active(struct plugin *pl);
void plugin_set_active(struct plugin *pl, int active);
void plugin_set_lazy(struct plugin *pl, int lazy);
void plugin_call_init(struct plugin *pl);
void plugin_unload(struct plugin *pl);
void plugin_destroy(struct plugin *pl);
struct plugins *plugins_new(void);
void plugins_add_path(struct plugins *pls, const char *path, int active, int lazy);
void plugins_init(struct plugins *pls);
void plugins_destroy(struct plugins *pls);
void *plugin_get_type(enum plugin_type type, const char *name);
/* end of prototypes */

#ifdef __cplusplus
}
#endif


