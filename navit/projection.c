#include <string.h>
#include <glib.h>
#include "coord.h"
#include "debug.h"
#include "projection.h"

struct projection_name {
        enum projection projection;
        char *name;
};


struct projection_name projection_names[]={
	{projection_none, ""},
	{projection_mg, "mg"},
	{projection_garmin, "garmin"},
};


enum projection
projection_from_name(const char *name)
{
	int i;

	for (i=0 ; i < sizeof(projection_names)/sizeof(struct projection_name) ; i++) {
		if (! strcmp(projection_names[i].name, name))
			return projection_names[i].projection;
	}
	return projection_none;
}

char *
projection_to_name(enum projection proj)
{
	int i;

	for (i=0 ; i < sizeof(projection_names)/sizeof(struct projection_name) ; i++) {
		if (projection_names[i].projection == proj)
			return projection_names[i].name;
	}
	return NULL; 
}

