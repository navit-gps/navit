struct route;
struct statusbar_gui;

struct statusbar {
	void (*statusbar_destroy)(struct statusbar *this);
	void (*statusbar_mouse_update)(struct statusbar *this, struct transformation *tr, struct point *p);
	void (*statusbar_route_update)(struct statusbar *this, struct route *route);
	void (*statusbar_gps_update)(struct statusbar *this, int sats, int qual, double lng, double lat, double height, double direction, double speed);
	struct statusbar_gui *gui;
};
