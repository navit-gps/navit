#ifdef __cplusplus
extern "C" {
#endif
enum navigation_mode {
	navigation_mode_long,
	navigation_mode_long_exact,
	navigation_mode_short,
	navigation_mode_speech,
};

/* prototypes */
enum item_type;
enum navigation_mode;
struct callback;
struct mapset;
struct navigation;
struct navigation_list;
struct route;
struct navigation *navigation_new(struct mapset *ms);
void navigation_set_mapset(struct navigation *this_, struct mapset *ms);
int navigation_set_announce(struct navigation *this_, enum item_type type, int *level);
struct navigation_list *navigation_list_new(struct navigation *this_);
char *navigation_list_get(struct navigation_list *this_, enum navigation_mode mode);
void navigation_list_destroy(struct navigation_list *this_);
void navigation_update(struct navigation *this_, struct route *route);
void navigation_destroy(struct navigation *this_);
int navigation_register_callback(struct navigation *this_, enum navigation_mode mode, struct callback *cb);
void navigation_unregister_callback(struct navigation *this_, enum navigation_mode mode, struct callback *cb);
/* end of prototypes */
#ifdef __cplusplus
}
#endif
