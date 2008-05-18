#define RT_PEDESTRIAN		(1<<0)
#define RT_BYCYCLE		(1<<1)
#define RT_MOTORCYCLE		(1<<2)
#define RT_CAR			(1<<3)
#define RT_TRUCK		(1<<4)
#define RT_LONGTRUCK		(1<<5)

struct gar2navit {
	unsigned short id;
	unsigned short maxid;
	enum item_type ntype;
	int routable;
	char *descr;
	struct gar2navit *next;
};

#define G2N_POINT		1
#define G2N_POLYLINE		2
#define G2N_POLYGONE		3

struct gar2nav_conv {
	struct gar2navit *points;
	struct gar2navit *polylines;
	struct gar2navit *polygons;
};

struct gar2nav_conv *g2n_conv_load(char *file);
enum item_type g2n_get_type(struct gar2nav_conv *c, int type, unsigned short id);
char *g2n_get_descr(struct gar2nav_conv *c, int type, unsigned short id);
int g2n_get_routable(struct gar2nav_conv *c, int type, unsigned short id);
struct gar2nav_conv *g2n_default_conv(void);
