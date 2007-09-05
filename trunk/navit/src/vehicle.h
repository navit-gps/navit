#ifdef __cplusplus
extern "C" {
#endif
/* prototypes */
enum projection;
struct callback;
struct coord;
struct vehicle;
enum projection vehicle_projection(struct vehicle *this_);
struct coord *vehicle_pos_get(struct vehicle *this_);
double *vehicle_speed_get(struct vehicle *this_);
double *vehicle_height_get(struct vehicle *this_);
double *vehicle_dir_get(struct vehicle *this_);
double *vehicle_quality_get(struct vehicle *this_);
int *vehicle_sats_get(struct vehicle *this_);
void vehicle_set_position(struct vehicle *this_, struct coord *pos);
struct vehicle *vehicle_new(const char *url);
void vehicle_callback_add(struct vehicle *this_, struct callback *cb);
void vehicle_callback_remove(struct vehicle *this_, struct callback *cb);
void vehicle_destroy(struct vehicle *this_);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

