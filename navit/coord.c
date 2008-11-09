/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "debug.h"
#include "item.h"
#include "coord.h"
#include "transform.h"
#include "projection.h"
/**
 * @defgroup coord Coordinate handling functions
 * @{
 */

/**
 * Get a coordinate
 *
 * @param p Pointer to the coordinate
 * @returns the coordinate
 */

struct coord *
coord_get(unsigned char **p)
{
	struct coord *ret=(struct coord *)(*p);
	*p += sizeof(*ret);
	return ret;
}

struct coord *
coord_new(int x, int y)
{
	struct coord *c=g_new(struct coord, 1);

	c->x=x;
	c->y=y;

	return c;
}

struct coord *
coord_new_from_attrs(struct attr *parent, struct attr **attrs)
{
	struct attr *x,*y;
	x=attr_search(attrs, NULL, attr_x);
	y=attr_search(attrs, NULL, attr_y);
	if (!x || !y)
		return NULL;
	return coord_new(x->u.num, y->u.num);
}


void
coord_destroy(struct coord *c)
{
	g_free(c);
}

struct coord_rect *
coord_rect_new(struct coord *lu, struct coord *rl)
{
	struct coord_rect *r=g_new(struct coord_rect, 1);

	dbg_assert(lu->x <= rl->x);
	dbg_assert(lu->y >= rl->y);

	r->lu=*lu;
	r->rl=*rl;

	return r;
	
}

void
coord_rect_destroy(struct coord_rect *r)
{
	g_free(r);
}

int 
coord_rect_overlap(struct coord_rect *r1, struct coord_rect *r2)
{
	dbg_assert(r1->lu.x <= r1->rl.x);
	dbg_assert(r1->lu.y >= r1->rl.y);
	dbg_assert(r2->lu.x <= r2->rl.x);
	dbg_assert(r2->lu.y >= r2->rl.y);
	dbg(1,"0x%x,0x%x - 0x%x,0x%x vs 0x%x,0x%x - 0x%x,0x%x\n", r1->lu.x, r1->lu.y, r1->rl.x, r1->rl.y, r2->lu.x, r2->lu.y, r2->rl.x, r2->rl.y);
	if (r1->lu.x > r2->rl.x)
		return 0;
	if (r1->rl.x < r2->lu.x)
		return 0;
	if (r1->lu.y < r2->rl.y)
		return 0;
	if (r1->rl.y > r2->lu.y)
		return 0;
	return 1;
}

int
coord_rect_contains(struct coord_rect *r, struct coord *c)
{
	dbg_assert(r->lu.x <= r->rl.x);
	dbg_assert(r->lu.y >= r->rl.y);
	if (c->x < r->lu.x)
		return 0;
	if (c->x > r->rl.x)
		return 0;
	if (c->y < r->rl.y)
		return 0;
	if (c->y > r->lu.y)
		return 0;
	return 1;
}

void
coord_rect_extend(struct coord_rect *r, struct coord *c)
{
	if (c->x < r->lu.x)
		r->lu.x=c->x;
	if (c->x > r->rl.x)
		r->rl.x=c->x;
	if (c->y < r->rl.y)
		r->rl.y=c->y;
	if (c->y > r->lu.y)
		r->lu.y=c->y;
}

	/* [Proj:][Ð]DMM.ss[S][S]... N/S [D][D]DMM.ss[S][S]... E/W */
	/* [Proj:][-][D]D.d[d]... [-][D][D]D.d[d]... */
	/* [Proj:][-]0xX [-]0xX */
/*
 * Currently supported:
 * 	[Proj:]-0xX [-]0xX 
 * 	- where Proj can be mg/garmin, defaults to mg
 * 	[Proj:][D][D]Dmm.ss[S][S] N/S [D][D]DMM.ss[S][S]... E/W
 * 	[Proj:][-][D]D.d[d]... [-][D][D]D.d[d]
 * 	- where Proj can be geo
 */

int
coord_parse(const char *c_str, enum projection pro, struct coord *c_ret)
{
	int debug=0;
	char *proj=NULL,*s,*co;
	const char *str=c_str;
	int args,ret = 0;
	struct coord_geo g;
	struct coord c;
	enum projection str_pro=projection_none;

	dbg(1,"enter('%s',%d,%p)\n", c_str, pro, c_ret);
	s=strchr(str,' ');
	co=strchr(str,':');
	if (co && co < s) {
		proj=malloc(co-str+1);
		strncpy(proj, str, co-str);
		proj[co-str]='\0';
		dbg(1,"projection=%s\n", proj);
		str=co+1;
		s=strchr(str,' ');
		if (!strcmp(proj, "mg"))
			str_pro = projection_mg;
		else if (!strcmp(proj, "garmin"))
			str_pro = projection_garmin;
		else if (!strcmp(proj, "geo"))
			str_pro = projection_none;
		else {
			dbg(0, "Unknown projection: %s\n", proj);
			goto out;
		}
	}
	if (! s)
		return 0;
	while (*s == ' ') {
		s++;
	}
	if (!strncmp(s, "0x", 2) || !strncmp(s, "-0x", 3)) {
		args=sscanf(str, "%i %i%n",&c.x, &c.y, &ret);
		if (args < 2)
			goto out;
		dbg(1,"str='%s' x=0x%x y=0x%x c=%d\n", str, c.x, c.y, ret);
		dbg(1,"rest='%s'\n", str+ret);

		if (str_pro == projection_none) 
			str_pro=projection_mg;
		if (str_pro != pro) {
			transform_to_geo(str_pro, &c, &g);
			transform_from_geo(pro, &g, &c);
		}
		*c_ret=c;
	} else if (*s == 'N' || *s == 'n' || *s == 'S' || *s == 's') {
		double lng, lat;
		char ns, ew;
		dbg(1,"str='%s'\n", str);
		args=sscanf(str, "%lf %c %lf %c%n", &lat, &ns, &lng, &ew, &ret);
		if (args < 4)
			goto out;
		if (str_pro == projection_none) {
			g.lat=floor(lat/100);
			lat-=g.lat*100;
			g.lat+=lat/60;
			g.lng=floor(lng/100);
			lng-=g.lng*100;
			g.lng+=lng/60;
			if (ns == 's' || ns == 'S')
				g.lat=-g.lat;
			if (ew == 'w' || ew == 'W')
				g.lng=-g.lng;
			transform_from_geo(pro, &g, c_ret);
		}
		dbg(3,"str='%s' x=%f ns=%c y=%f ew=%c c=%d\n", str, lng, ns, lat, ew, ret);
		dbg(3,"rest='%s'\n", str+ret);
	} else {
		double lng, lat;
		args=sscanf(str, "%lf %lf%n", &lng, &lat, &ret);
		if (args < 2)
			goto out;
		dbg(1,"str='%s' x=%f y=%f  c=%d\n", str, lng, lat, ret);
		dbg(1,"rest='%s'\n", str+ret);
		g.lng=lng;
		g.lat=lat;
		transform_from_geo(pro, &g, c_ret);
	}
	if (debug)
		printf("rest='%s'\n", str+ret);
	ret+=str-c_str;
	if (debug) {
		printf("args=%d\n", args);
		printf("ret=%d delta=%d ret_str='%s'\n", ret, str-c_str, c_str+ret);
	}
out:
	if (proj)
		free(proj);
	return ret;
}

void
coord_print(enum projection pro, struct coord *c, FILE *out) {
	unsigned int x;
	unsigned int y;
	char *sign_x = "";
	char *sign_y = "";
	
	if ( c->x < 0 ) {
		x = -c->x;
		sign_x = "-";
	} else {
		x = c->x;
	}
	if ( c->y < 0 ) {
		y = -c->y;
		sign_y = "-";
	} else {
		y = c->y;
	}
	fprintf( out, "%s: %s0x%x %s0x%x\n",
		 projection_to_name( pro ),
		 sign_x, x,
		 sign_y, y );
	return;
}

/**
 * @brief Converts a lat/lon into a text formatted text string.
 * @param lat The latitude
 * @param lng The longitude
 * @param fmt The format to use. 
 *    @li DEGREES=>(45.5000N 100.9000S)
 *    @li DEGREES_MINUTES=>(45 30.))00N 100 120.54.0000S)
 *    @li DEGREES_MINUTES_SECONDS=>(4530.0000N 12054.0000S)
 *           
 * 
 * @param buffer  A buffer large enough to hold the output + a terminating NULL (26 bytes)
 * @param size The size of the buffer
 *
 */
void coord_format(float lat,float lng, enum coord_format fmt, char * buffer, int size)
{

	char lat_c='N';
  	char lng_c='E';
	float lat_deg,lat_min,lat_sec;
	float lng_deg,lng_min,lng_sec;

	if (lng < 0) {
		lng=-lng;
		lng_c='W';
	}
	if (lat < 0) {
		lat=-lat;
		lat_c='S';
	}
	lat_deg=lat;
	lat_min=(lat-floor(lat_deg))*60;
	lat_sec=fmod(lat*3600,60);
	lng_deg=lng;
	lng_min=(lng-floor(lng_deg))*60;
	lng_sec=fmod(lng*3600,60);
	switch(fmt)
	{

	case DEGREES_DECIMAL:
	  snprintf(buffer,size,"%02.6f%c %03.7f%c",lat,lat_c,lng,lng_c);
	  break;
	case DEGREES_MINUTES:
	  snprintf(buffer,size,"%02.0f %07.4f%c %03.0f %07.4f%c",floor(lat_deg),lat_min , lat_c, floor(lng), lng_min, lng_c);
		   break;
	case DEGREES_MINUTES_SECONDS:
	  snprintf(buffer,size,"%02.0f%07.4f%c %03.0f%07.4f%c",floor(lat), fmod(lat*60,60), lat_c, floor(lng), fmod(lng*60,60), lng_c);
	  break;
	  
	
	}
	
}

/** @} */
