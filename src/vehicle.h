struct vehicle;

struct coord *vehicle_pos_get(struct vehicle *);
double *vehicle_dir_get(struct vehicle *);
double *vehicle_speed_get(struct vehicle *);
void vehicle_callback(struct vehicle *, void (*func)(),void *data);
void vehicle_set_position(struct vehicle *, struct coord *);
struct vehicle *vehicle_new(const char *url);
