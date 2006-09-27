#ifndef COORD_H
#define COORD_H

#include "types.h"

/*! A integer mercator coordinate */
struct coord {
	s32 x; /*!< X-Value */
	s32 y; /*!< Y-Value */
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

struct coord * coord_get(unsigned char **p);

#endif
