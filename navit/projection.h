#ifndef NAVIT_PROJECTION_H
#define NAVIT_PROJECTION_H

enum projection {
	projection_none, projection_mg, projection_garmin
};

enum map_datum {
	map_datum_none, map_datum_wgs84, map_datum_dhdn
};

enum projection projection_from_name(const char *name);
char * projection_to_name(enum projection proj);

#endif

