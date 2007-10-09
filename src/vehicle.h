#ifndef NAVIT_VEHICLE_H
#define NAVIT_VEHICLE_H

#ifdef __cplusplus
extern "C" {
#endif
/* prototypes */
enum projection;
struct attr;
struct callback;
struct coord;
struct log;
struct navit;
struct vehicle;
enum projection vehicle_projection(struct vehicle *this_);
struct coord *vehicle_pos_get(struct vehicle *this_);
double *vehicle_speed_get(struct vehicle *this_);
double *vehicle_height_get(struct vehicle *this_);
double *vehicle_dir_get(struct vehicle *this_);
int *vehicle_status_get(struct vehicle *this_);
int *vehicle_sats_get(struct vehicle *this_);
int *vehicle_sats_used_get(struct vehicle *this_);
double *vehicle_pdop_get(struct vehicle *this_);
void vehicle_set_position(struct vehicle *this_, struct coord *pos);
void vehicle_set_navit(struct vehicle *this_, struct navit *nav);
struct vehicle *vehicle_new(const char *url);
void vehicle_callback_add(struct vehicle *this_, struct callback *cb);
void vehicle_callback_remove(struct vehicle *this_, struct callback *cb);
int vehicle_add_log(struct vehicle *this_, struct log *log, struct attr **attrs);
void vehicle_destroy(struct vehicle *this_);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif

