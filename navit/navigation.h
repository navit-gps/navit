#ifndef NAVIT_NAVIGATION_H
#define NAVIT_NAVIGATION_H

#ifdef __cplusplus
extern "C" {
#endif
/* prototypes */
enum attr_type;
enum item_type;
struct attr;
struct callback;
struct map;
struct navigation;
struct route;
struct navigation *navigation_new(struct attr **attrs);
int navigation_set_announce(struct navigation *this_, enum item_type type, int *level);
void navigation_update(struct navigation *this_, struct route *route);
void navigation_flush(struct navigation *this_);
void navigation_destroy(struct navigation *this_);
int navigation_register_callback(struct navigation *this_, enum attr_type type, struct callback *cb);
void navigation_unregister_callback(struct navigation *this_, enum attr_type type, struct callback *cb);
struct map *navigation_get_map(struct navigation *this_);
void navigation_init(void);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
