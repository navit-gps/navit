#ifndef NAVIT_COORD_H
#define NAVIT_COORD_H
#include "projection.h"

/*! A integer mercator coordinate */
struct coord {
	int x; /*!< X-Value */
	int y; /*!< Y-Value */
};

/*! A integer mercator coordinate carrying its projection */
struct pcoord {
	enum projection pro;
	int x; /*!< X-Value */
	int y; /*!< Y-Value */
};

struct coord_rect {
	struct coord lu;
	struct coord rl;
};

//! A double mercator coordinate
struct coord_d {
	double x; /*!< X-Value */
	double y; /*!< Y-Value */
};

//! A WGS84 coordinate
struct coord_geo {
	double lng; /*!< Longitude */
	double lat; /*!< Latitude */
};

enum projection;

struct coord * coord_get(unsigned char **p);
struct coord * coord_new(int x, int y);
void coord_destroy(struct coord *c);
int coord_parse(const char *c_str, enum projection pro, struct coord *c_ret);
struct coord_rect * coord_rect_new(struct coord *lu, struct coord *rl);
void coord_rect_destroy(struct coord_rect *r);
int coord_rect_overlap(struct coord_rect *r1, struct coord_rect *r2);
int coord_rect_contains(struct coord_rect *r, struct coord *c);
void coord_rect_extend(struct coord_rect *r, struct coord *c);


#endif
