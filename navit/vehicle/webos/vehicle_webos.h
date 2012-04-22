#include "coord.h"

#define GPS_TYPE_NONE 0
#define GPS_TYPE_INT  1
#define GPS_TYPE_BT   2

struct vehicle_priv {
	char *address;
	char *source;
	char *spp_address;
	char *buffer;
	char *nmea_data_buf;
	char *nmea_data;
	char fixiso8601[128];
	double track, speed, altitude, radius;
	double hdop;
	int gps_type;
	int pdk_version;
	int spp_instance_id;
	int buffer_pos;
	int delta;
	int sats_used;
	int sats_visible;
	int magnetic_direction;
	int status;
	int valid;
	time_t fix_time;
	struct attr ** attrs;
	struct callback *event_cb;
	struct callback *timeout_cb;
	struct callback_list *cbl;
	struct coord_geo geo;
	struct event_timeout *ev_timeout;
};

extern void vehicle_webos_close(struct vehicle_priv *priv);

