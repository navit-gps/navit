#ifndef NAVIT_NAVIGATION_H
#define NAVIT_NAVIGATION_H

#ifdef __cplusplus
extern "C" {
#endif
/* prototypes */
enum attr_type;
enum item_type;
struct callback;
struct item;
struct mapset;
struct navigation;
struct navigation_list;
struct route;
struct navigation *navigation_new(struct mapset *ms);
void navigation_set_mapset(struct navigation *this_, struct mapset *ms);
int navigation_set_announce(struct navigation *this_, enum item_type type, int *level);
struct navigation_list *navigation_list_new(struct navigation *this_);
struct item *navigation_list_get_item(struct navigation_list *this_);
void navigation_list_destroy(struct navigation_list *this_);
void navigation_update(struct navigation *this_, struct route *route);
void navigation_destroy(struct navigation *this_);
int navigation_register_callback(struct navigation *this_, enum attr_type type, struct callback *cb);
void navigation_unregister_callback(struct navigation *this_, enum attr_type type, struct callback *cb);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
