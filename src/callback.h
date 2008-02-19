#ifndef NAVIT_CALLBACK_H
#define NAVIT_CALLBACK_H

#include "item.h"
#include "attr.h"

#ifdef __cplusplus
extern "C" {
#endif
/* prototypes */
enum attr_type;
struct callback;
struct callback_list;
struct callback_list *callback_list_new(void);
struct callback * callback_new_attr(void (*func)(void), enum attr_type type, int pcount, void **p);
struct callback *callback_new(void (*func)(void), int pcount, void **p);
void callback_set_arg(struct callback *cb, int arg, void *p);
void callback_list_add(struct callback_list *l, struct callback *cb);
struct callback *callback_list_add_new(struct callback_list *l, void (*func)(void), int pcount, void **p);
void callback_list_remove(struct callback_list *l, struct callback *cb);
void callback_list_remove_destroy(struct callback_list *l, struct callback *cb);
void callback_call(struct callback *cb, int pcount, void **p);
void callback_list_call_attr(struct callback_list *l, enum attr_type type, int pcount, void **p);
void callback_list_call(struct callback_list *l, int pcount, void **p);
void callback_list_destroy(struct callback_list *l);
/* end of prototypes */

static inline struct callback *callback_new_attr_0(void (*func)(void), enum attr_type type)
{
	return callback_new_attr(func, type, 0, NULL);
}

static inline struct callback *callback_new_0(void (*func)(void))
{
	return callback_new(func, 0, NULL);
}

static inline struct callback *callback_new_attr_1(void (*func)(void), enum attr_type type, void *p1)
{
	void *p[1];
	p[0]=p1;
	return callback_new_attr(func, type, 1, p);
}

static inline struct callback *callback_new_1(void (*func)(void), void *p1)
{
	void *p[1];
	p[0]=p1;
	return callback_new(func, 1, p);
}

static inline struct callback *callback_new_2(void (*func)(void), void *p1, void *p2)
{
	void *p[2];
	p[0]=p1;
	p[1]=p2;
	return callback_new(func, 2, p);
}

static inline struct callback *callback_new_3(void (*func)(void), void *p1, void *p2, void *p3)
{
	void *p[3];
	p[0]=p1;
	p[1]=p2;
	p[2]=p3;
	return callback_new(func, 3, p);
}

static inline void callback_call_0(struct callback *cb)
{
	callback_call(cb, 0, NULL);
}

static inline void callback_call_1(struct callback *cb, void *p1)
{
	void *p[1];
	p[0]=p1;
	callback_call(cb, 1, p);
}

static inline void callback_list_call_0(struct callback_list *l)
{
	callback_list_call(l, 0, NULL);
}

static inline void callback_list_call_attr_1(struct callback_list *l, enum attr_type type, void *p1)
{
	void *p[1];
	p[0]=p1;
	callback_list_call_attr(l, type, 1, p);
}

static inline void callback_list_call_1(struct callback_list *l, void *p1)
{
	void *p[1];
	p[0]=p1;
	callback_list_call(l, 1, p);
}

static inline void callback_list_call_attr_2(struct callback_list *l, enum attr_type type, void *p1, void *p2)
{
	void *p[2];
	p[0]=p1;
	p[1]=p2;
	callback_list_call_attr(l, type, 2, p);
}

static inline void callback_list_call_2(struct callback_list *l, void *p1, void *p2)
{
	void *p[2];
	p[0]=p1;
	p[1]=p2;
	callback_list_call(l, 2, p);
}

#define callback_cast(x) (void (*)(void))(x)
#ifdef __cplusplus
}
#endif

#endif

