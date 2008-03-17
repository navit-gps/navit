#ifndef NAVIT_PROJECTION_H
#define NAVIT_PROJECTION_H

enum projection {
	projection_none, projection_mg, projection_garmin
};

enum projection projection_from_name(const char *name);
char * projection_to_name(enum projection proj);

#endif

