#ifdef __cplusplus
extern "C" {
#endif
/* prototypes */
struct callback;
struct callback_list;
struct callback_list *callback_list_new(void);
struct callback *callback_list_add_new(struct callback_list *l, void (*func)(void), int pcount, void **p);
void callback_list_remove_destroy(struct callback_list *l, struct callback *cb);
void callback_list_call(struct callback_list *l, int pcount, void **p);
void callback_list_destroy(struct callback_list *l);
/* end of prototypes */
#ifdef __cplusplus
}
#endif
