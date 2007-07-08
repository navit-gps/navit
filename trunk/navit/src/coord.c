#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "debug.h"
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

void
coord_destroy(struct coord *c)
{
	g_free(c);
}

struct coord_rect *
coord_rect_new(struct coord *lu, struct coord *rl)
{
	struct coord_rect *r=g_new(struct coord_rect, 1);

	g_assert(lu->x <= rl->x);
	g_assert(lu->y >= rl->y);

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
	g_assert(r1->lu.x <= r1->rl.x);
	g_assert(r1->lu.y >= r1->rl.y);
	g_assert(r2->lu.x <= r2->rl.x);
	g_assert(r2->lu.y >= r2->rl.y);
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
	g_assert(r->lu.x <= r->rl.x);
	g_assert(r->lu.y >= r->rl.y);
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

int
coord_parse(const char *c_str, enum projection pro, struct coord *c_ret)
{
	int debug=0;
	char *proj=NULL,*s,*co;
	const char *str=c_str;
	int args,ret;
	struct coord_geo g;
	struct coord c;
	enum projection str_pro=projection_none;

	dbg(1,"enter('%s',%d,%p)\n", c_str, pro, c_ret);
	s=index(str,' ');
	co=index(str,':');
	if (co && co < s) {
		proj=malloc(co-str+1);
		strncpy(proj, str, co-str);
		proj[co-str]='\0';
		dbg(1,"projection=%s\n", proj);
		str=co+1;
		s=index(str,' ');
	}
	if (! s)
		return 0;
	while (*s == ' ') {
		s++;
	}
	if (!strncmp(str, "0x", 2) || !strncmp(str,"-0x", 3)) {
		args=sscanf(str, "%i %i%n",&c.x, &c.y, &ret);
		if (args < 2)
			return 0;
		dbg(1,"str='%s' x=0x%x y=0x%x c=%d\n", str, c.x, c.y, ret);
		dbg(1,"rest='%s'\n", str+ret);

		if (str_pro == projection_none) 
			str_pro=projection_mg;
		if (str_pro == projection_mg) {
			*c_ret=c;
		} else {
			printf("help\n");
		}
	} else if (*s == 'N' || *s == 'n' || *s == 'S' || *s == 's') {
		dbg(1,"str='%s'\n", str);
		double lng, lat;
		char ns, ew;
		args=sscanf(str, "%lf %c %lf %c%n", &lat, &ns, &lng, &ew, &ret);
		if (args < 4)
			return 0;
		if (str_pro == projection_none) {
			g.lat=floor(lat/100);
			lat-=g.lat*100;
			g.lat+=lat/60;
			g.lng=floor(lng/100);
			lng-=g.lng*100;
			g.lng+=lng/60;
			transform_from_geo(pro, &g, c_ret);
		}
		if (debug) {
			printf("str='%s' x=%f ns=%c y=%f ew=%c c=%d\n", str, lng, ns, lat, ew, ret);
			printf("rest='%s'\n", str+ret);
		}
	} else {
		double lng, lat;
		args=sscanf(str, "%lf %lf%n", &lng, &lat, &ret);
		if (args < 2)
			return 0;
		printf("str='%s' x=%f y=%f  c=%d\n", str, lng, lat, ret);
		printf("rest='%s'\n", str+ret);
	}
	if (debug)
		printf("rest='%s'\n", str+ret);
	ret+=str-c_str;
	if (debug) {
		printf("args=%d\n", args);
		printf("ret=%d delta=%d ret_str='%s'\n", ret, str-c_str, c_str+ret);
	}
	if (proj)
		free(proj);
	return ret;
}

/** @} */
