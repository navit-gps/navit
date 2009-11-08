/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef NAVIT_COORD_H
#define NAVIT_COORD_H

#include <stdio.h>
#include "config.h"
#include "projection.h"

#define coord_is_equal(a,b) ((a).x==(b).x && (a).y==(b).y)

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


#ifdef AVOID_FLOAT
/**
 * On platforms where we are trying to avoid floats, sometimes we can't.
 * It is better on these platforms to use single precision floating points
 * over double percision ones since performance is much better.
 */
typedef float navit_float;
#define navit_sin(x) sinf(x)
#define navit_cos(x) cosf(x)
#define navit_tan(x) tanf(x)
#define navit_atan(x) atanf(x)
#define navit_acos(x) acosf(x)
#define navit_asin(x) asinf(x)
#define navit_sqrt(x) sqrtf(x)
#else
typedef  double navit_float;
#define navit_sin(x) sin(x)
#define navit_cos(x) cos(x)
#define navit_tan(x) tan(x)
#define navit_atan(x) atan(x)
#define navit_acos(x) acos(x)
#define navit_asin(x) asin(x)
#define navit_sqrt(x) sqrt(x)
#endif


//! A double mercator coordinate
struct coord_d {
	double x; /*!< X-Value */
	double y; /*!< Y-Value */
};

//! A WGS84 coordinate
struct coord_geo {
	navit_float lng; /*!< Longitude */
	navit_float lat; /*!< Latitude */
};

//! A cartesian coordinate 
struct coord_geo_cart {
	navit_float x; /*!< X-Value */
	navit_float y; /*!< Y-Value */
	navit_float z; /*!< Z-Value */
};

/**
 * An enumeration of formats for printing geographic coordinates in.
 *
 */
enum coord_format 
{
	/**
	 * Degrees with decimal places.
	 * Ie 20.5000 N 110.5000 E
	 */
	DEGREES_DECIMAL,

	/**
	 * Degrees and minutes.
	 * ie 20 30.00 N 110 30.00 E
	 */
	DEGREES_MINUTES,
	/**
	 * Degrees, minutes and seconds.
	 * ie 20 30 30.00 N 110 30 30 E
	 */
	DEGREES_MINUTES_SECONDS	
};

enum projection;
struct attr;

struct coord * coord_get(unsigned char **p);
struct coord * coord_new(int x, int y);
struct coord * coord_new_from_attrs(struct attr *parent, struct attr **attrs);
void coord_destroy(struct coord *c);
int coord_parse(const char *c_str, enum projection pro, struct coord *c_ret);
int pcoord_parse(const char *c_str, enum projection pro, struct pcoord *c_ret);
void coord_print(enum projection pro, struct coord *c, FILE *out);
struct coord_rect * coord_rect_new(struct coord *lu, struct coord *rl);
void coord_rect_destroy(struct coord_rect *r);
int coord_rect_overlap(struct coord_rect *r1, struct coord_rect *r2);
int coord_rect_contains(struct coord_rect *r, struct coord *c);
void coord_rect_extend(struct coord_rect *r, struct coord *c);
void coord_format(float lat,float lng, enum coord_format, char * buffer, int size);

#endif
/* prototypes */
enum coord_format;
enum projection;
struct attr;
struct coord;
struct coord_rect;
struct pcoord;
struct coord *coord_get(unsigned char **p);
struct coord *coord_new(int x, int y);
struct coord *coord_new_from_attrs(struct attr *parent, struct attr **attrs);
void coord_destroy(struct coord *c);
struct coord_rect *coord_rect_new(struct coord *lu, struct coord *rl);
void coord_rect_destroy(struct coord_rect *r);
int coord_rect_overlap(struct coord_rect *r1, struct coord_rect *r2);
int coord_rect_contains(struct coord_rect *r, struct coord *c);
void coord_rect_extend(struct coord_rect *r, struct coord *c);
int coord_parse(const char *c_str, enum projection pro, struct coord *c_ret);
int pcoord_parse(const char *c_str, enum projection pro, struct pcoord *pc_ret);
void coord_print(enum projection pro, struct coord *c, FILE *out);
void coord_format(float lat, float lng, enum coord_format fmt, char *buffer, int size);
unsigned int coord_hash(const void *key);
int coord_equal(const void *a, const void *b);
/* end of prototypes */
