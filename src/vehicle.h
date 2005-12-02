struct coord *vehicle_pos_get(void *);
double *vehicle_dir_get(void *);
double *vehicle_speed_get(void *);
void vehicle_callback(void (*func)(),void *data);
void vehicle_set_position(void *, struct coord *);
void *vehicle_new(char *file);
