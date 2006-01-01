struct track;

void track_update(struct track *tr, struct coord *c, int angle);
struct track * track_new(struct map_data *ma);
void track_destroy(struct track *tr);
