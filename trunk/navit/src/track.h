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
void tracking_set_mapset(struct tracking *this, struct mapset *ms);
void tracking_destroy(struct tracking *tr);
/* end of prototypes */
