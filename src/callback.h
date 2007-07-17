#ifdef __cplusplus
extern "C" {
#endif
/* prototypes */
struct callback;
struct callback_list;
struct callback_list *callback_list_new(void);
struct callback *callback_new(void (*func)(void), int pcount, void **p);
void callback_list_add(struct callback_list *l, struct callback *cb);
struct callback *callback_list_add_new(struct callback_list *l, void (*func)(void), int pcount, void **p);
void callback_list_remove(struct callback_list *l, struct callback *cb);
void callback_list_remove_destroy(struct callback_list *l, struct callback *cb);
void callback_list_call(struct callback_list *l, int pcount, void **p);
void callback_list_destroy(struct callback_list *l);

static inline struct callback *callback_new_0(void (*func)(void))
{
	return callback_new(func, 0, NULL);
}

static inline struct callback *callback_new_1(void (*func)(void), void *p1)
{
	void *p[1];
	p[0]=p1;
	return callback_new(func, 1, p);
}

static inline void callback_list_call_1(struct callback_list *l, void *p1)
{
	void *p[1];
	p[0]=p1;
	callback_list_call(l, 1, p);
}

static inline void callback_list_call_2(struct callback_list *l, void *p1, void *p2)
{
	void *p[2];
	p[0]=p1;
	p[1]=p2;
	callback_list_call(l, 2, p);
}

#define callback_cast(x) (void (*)(void))(x)
/* end of prototypes */
#ifdef __cplusplus
}
#endif
