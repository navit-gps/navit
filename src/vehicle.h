/* prototypes */
enum projection;
struct callback;
struct coord;
struct vehicle;
enum projection vehicle_projection(struct vehicle *this);
struct coord *vehicle_pos_get(struct vehicle *this);
double *vehicle_speed_get(struct vehicle *this);
double *vehicle_dir_get(struct vehicle *this);
void vehicle_set_position(struct vehicle *this, struct coord *pos);
struct vehicle *vehicle_new(const char *url);
void vehicle_callback_add(struct vehicle *this, struct callback *cb);
void vehicle_callback_remove(struct vehicle *this, struct callback *cb);
void vehicle_destroy(struct vehicle *this);
/* end of prototypes */
