void plugin_load(void);
int plugin_init(void);

struct container;
struct popup;
struct popup_item;
#undef PLUGIN_FUNC1
#undef PLUGIN_FUNC3
#undef PLUGIN_FUNC4
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
#endif

PLUGIN_FUNC1(draw, struct container *, co)
PLUGIN_FUNC3(popup, struct container *, map, struct popup *, p, struct popup_item **, list)
