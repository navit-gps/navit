struct vehicle;

struct coord *vehicle_pos_get(struct vehicle *);
double *vehicle_dir_get(struct vehicle *);
double *vehicle_speed_get(struct vehicle *);
double *vehicle_height_get(struct vehicle *this);
void * vehicle_callback_register(struct vehicle *this, void (*func)(struct vehicle *, void *data), void *data);
void vehicle_callback_unregister(struct vehicle *this, void *handle);
void vehicle_set_position(struct vehicle *, struct coord *);
struct vehicle *vehicle_new(const char *url);
void vehicle_destroy(struct vehicle *);
