#ifndef COORD_H
#define COORD_H

struct coord {
	long x;
	long y;
};

struct coord_d {
	double x;
	double y;
};

struct coord_geo {
	double lng;
	double lat;
};

struct coord * coord_get(unsigned char **p);

#endif
