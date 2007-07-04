struct route;
struct statusbar_priv;
struct point;
struct transformation;

struct statusbar_methods {
	void (*statusbar_destroy)(struct statusbar_priv *this);
	void (*statusbar_mouse_update)(struct statusbar_priv *this, struct transformation *tr, struct point *p);
	void (*statusbar_route_update)(struct statusbar_priv *this, struct route *route);
	void (*statusbar_gps_update)(struct statusbar_priv *this, int sats, int qual, double lng, double lat, double height, double direction, double speed);
};

struct statusbar {
	struct statusbar_methods meth;
	struct statusbar_priv *priv;
};

/* prototypes */
