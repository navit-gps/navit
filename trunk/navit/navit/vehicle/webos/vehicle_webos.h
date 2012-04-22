#include "coord.h"

struct vehicle_priv {
	char *address;
	char *source;
	char *spp_address;
	char *buffer;
	char *nmea_data_buf;
	char *nmea_data;
	char fixiso8601[128];
	double track, speed, altitude, radius;
	int pdk_version;
	int spp_instance_id;
	int buffer_pos;
	int delta;
	time_t fix_time;
	struct attr ** attrs;
	struct callback *event_cb;
	struct callback *timeout_cb;
	struct callback_list *cbl;
	struct coord_geo geo;
	struct event_timeout *ev_timeout;
};

extern void vehicle_webos_close(struct vehicle_priv *priv);

