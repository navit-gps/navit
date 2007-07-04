/* prototypes */
struct coord;
struct mapset;
struct street_data;
struct track;
struct coord *track_get_pos(struct track *tr);
int track_get_segment_pos(struct track *tr);
struct street_data *track_get_street_data(struct track *tr);
int track_update(struct track *tr, struct coord *c, int angle);
struct track *track_new(struct mapset *ms);
void track_destroy(struct track *tr);
