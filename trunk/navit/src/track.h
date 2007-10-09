#ifndef NAVIT_TRACK_H
#define NAVIT_TRACK_H

#ifdef __cplusplus
extern "C" {
#endif
/* prototypes */
struct coord;
struct mapset;
struct street_data;
struct tracking;
struct coord *tracking_get_pos(struct tracking *tr);
int tracking_get_segment_pos(struct tracking *tr);
struct street_data *tracking_get_street_data(struct tracking *tr);
int tracking_update(struct tracking *tr, struct coord *c, int angle);
struct tracking *tracking_new(struct mapset *ms);
void tracking_set_mapset(struct tracking *this_, struct mapset *ms);
int tracking_get_current_attr(struct tracking *_this, enum attr_type type, struct attr *attr);
void tracking_destroy(struct tracking *tr);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
