struct route_path_segment {
	struct route_path_segment *next;
	long segid;
	int offset;
	int dir;
	int time;
	int length;
	struct coord c[2];
};

struct route_crossing {
	long segid;
	int dir;
};

struct route_crossings {
	int count;
	struct route_crossing crossing[0];
};

struct route;
struct map_data;
struct container;
struct route_info;

struct route *route_new(void);
int route_destroy(void *t);
void route_mapdata_set(struct route *this, struct map_data *mdata);
struct map_data* route_mapdata_get(struct route *this);
void route_display_points(struct route *this, struct container *co);
void route_click(struct route *this, struct container *co, int x, int y);
void route_start(struct route *this, struct container *co);
void route_set_position(struct route *this, struct coord *pos);
void route_set_destination(struct route *this, struct coord *dst);
struct coord *route_get_destination(struct route *this);
struct route_path_segment *route_path_get(struct route *, int segid);
struct route_path_segment *route_path_get_all(struct route *this);
void route_trace(struct container *co);
struct street_str *route_info_get_street(struct route_info *rt);
struct block_info *route_info_get_block(struct route_info *rt);
struct route_info *route_find_nearest_street(struct map_data *mdata, struct coord *c);
void route_find_point_on_street(struct route_info *rt_inf);
void route_do_start(struct route *this, struct route_info *rt_start, struct route_info *rt_end);
int route_find(struct route *this, struct route_info *rt_start, struct route_info *rt_end);
struct tm *route_get_eta(struct route *this);
double route_get_len(struct route *this);
struct route_crossings *route_crossings_get(struct route *this, struct coord *c);

