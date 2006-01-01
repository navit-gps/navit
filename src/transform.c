#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <glib.h>
#include "coord.h"
#include "transform.h"

int
transform(struct transformation *t, struct coord *c, struct point *p)
{
        double xc,yc;
	int ret=0;
        xc=c->x;
        yc=c->y;
        if (xc >= t->rect[0].x && xc <= t->rect[1].x && yc >= t->rect[1].y && yc <= t->rect[0].y)
                ret=1;
        xc-=t->center.x;
        yc-=t->center.y;
	yc=-yc;
	if (t->angle) {
	  	int xcn, ycn; 
	  	xcn=xc*t->cos_val+yc*t->sin_val;
	  	ycn=-xc*t->sin_val+yc*t->cos_val;
	  	xc=xcn;
	  	yc=ycn;
	}
        xc=xc*16.0/(double)(t->scale);
        yc=yc*16.0/(double)(t->scale);
#if 0
	{
		double zc=yc;
		if (zc  < 10 && zc > 10)
			zc=10;
			return 0;
		yc=300;
		xc/=(-zc+1000.0)/1000.0;
		yc/=(-zc+1000.0)/1000.0;
		xc+=t->width/2;
	}
#else
        yc+=t->height/2;
        xc+=t->width/2;
#endif
	if (xc < -0x8000)
		xc=-0x8000;
	if (xc > 0x7fff) {
		xc=0x7fff;
	}
	if (yc < -0x8000)
		yc=-0x8000;
	if (yc > 0x7fff)
		yc=0x7fff;
        p->x=xc;
        p->y=yc;
        return ret;
}

void
transform_reverse(struct transformation *t, struct point *p, struct coord *c)
{
        int xc,yc;
	xc=p->x;
	yc=p->y;
	xc-=t->width/2;
	yc-=t->height/2;
	xc=xc*t->scale/16;
	yc=-yc*t->scale/16;
	if (t->angle) {
	  	int xcn, ycn; 
	  	xcn=xc*t->cos_val+yc*t->sin_val;
	  	ycn=-xc*t->sin_val+yc*t->cos_val;
	  	xc=xcn;
	  	yc=ycn;
	}
	c->x=t->center.x+xc;
	c->y=t->center.y+yc;
}


static int
min4(int v1,int v2, int v3, int v4)
{
	int res=v1;
	if (v2 < res)
		res=v2;
	if (v3 < res)
		res=v3;
	if (v4 < res)
		res=v4;
	return res;
}

static int
max4(int v1,int v2, int v3, int v4)
{
	int res=v1;
	if (v2 > res)
		res=v2;
	if (v3 > res)
		res=v3;
	if (v4 > res)
		res=v4;
	return res;
}

void
transform_set_angle(struct transformation *t,int angle)
{
        t->angle=angle;
        t->cos_val=cos(M_PI*t->angle/180);
        t->sin_val=sin(M_PI*t->angle/180);
}

void
transform_setup(struct transformation *t, int x, int y, int scale, int angle)
{
        t->center.x=x;
        t->center.y=y;
        t->scale=scale;
	transform_set_angle(t, angle);
}

void
transform_setup_source_rect_limit(struct transformation *t, struct coord *center, int limit)
{
	t->center=*center;
	t->scale=1;
	t->angle=0;
	t->rect[0].x=center->x-limit;
	t->rect[1].x=center->x+limit;
	t->rect[1].y=center->y-limit;
	t->rect[0].y=center->y+limit;
}

void
transform_setup_source_rect(struct transformation *t)
{
	int i;
	struct coord screen[4];
	struct point screen_pnt[4];

	screen_pnt[0].x=0;
	screen_pnt[0].y=0;
	screen_pnt[1].x=t->width;
	screen_pnt[1].y=0;
	screen_pnt[2].x=0;
	screen_pnt[2].y=t->height;
	screen_pnt[3].x=t->width;
	screen_pnt[3].y=t->height;
	for (i = 0 ; i < 4 ; i++) {
		transform_reverse(t, &screen_pnt[i], &screen[i]);
	}
	t->rect[0].x=min4(screen[0].x,screen[1].x,screen[2].x,screen[3].x);
	t->rect[1].x=max4(screen[0].x,screen[1].x,screen[2].x,screen[3].x);
	t->rect[1].y=min4(screen[0].y,screen[1].y,screen[2].y,screen[3].y);
	t->rect[0].y=max4(screen[0].y,screen[1].y,screen[2].y,screen[3].y);
}

int
transform_get_scale(struct transformation *t)
{
	return t->scale/16;
}

void
transform_lng_lat(struct coord *c, struct coord_geo *g)
{
	g->lng=c->x/6371000.0/M_PI*180;
	g->lat=atan(exp(c->y/6371000.0))/M_PI*360-90;
#if 0
	printf("y=%d vs %f\n", c->y, log(tan(M_PI_4+*lat*M_PI/360))*6371020.0);
#endif
}

void
transform_geo_text(struct coord_geo *g, char *buffer)
{
	double lng=g->lng;
	double lat=g->lat;
	char lng_c='E';
	char lat_c='N';

	if (lng < 0) {
		lng=-lng;
		lng_c='W';
	}
	if (lat < 0) {
		lat=-lat;
		lat_c='S';
	}

	sprintf(buffer,"%02.0f%07.4f%c %03.0f%07.4f%c", floor(lat), fmod(lat*60,60), lat_c, floor(lng), fmod(lng*60,60), lng_c);

}

void
transform_mercator(double *lng, double *lat, struct coord *c)
{
	c->x=*lng*6371000.0*M_PI/180;
	c->y=log(tan(M_PI_4+*lat*M_PI/360))*6371000.0;
}

double
transform_scale(int y)
{
	struct coord c;
	struct coord_geo g;
	c.x=0;
	c.y=y;
	transform_lng_lat(&c, &g);
	return 1/cos(g.lat/180*M_PI);
}

double
transform_distance(struct coord *c1, struct coord *c2)
{
	double dx,dy,scale=transform_scale((c1->y+c2->y)/2);
	dx=c1->x-c2->x;
	dy=c1->y-c2->y;
	return sqrt(dx*dx+dy*dy)/scale;
}

int
transform_distance_sq(struct coord *c1, struct coord *c2)
{
	int dx=c1->x-c2->x;
	int dy=c1->y-c2->y;

	if (dx > 32767 || dy > 32767 || dx < -32767 || dy < -32767)
		return INT_MAX;
	else
		return dx*dx+dy*dy;
}

int
transform_distance_line_sq(struct coord *l0, struct coord *l1, struct coord *ref, struct coord *lpnt)
{
	int vx,vy,wx,wy;
	int c1,c2;
	int climit=1000000;
	struct coord l;

	vx=l1->x-l0->x;
	vy=l1->y-l0->y;
	wx=ref->x-l0->x;
	wy=ref->y-l0->y;

	c1=vx*wx+vy*wy;
	if ( c1 <= 0 ) {
		if (lpnt)
			*lpnt=*l0;
		return transform_distance_sq(l0, ref);
	}
	c2=vx*vx+vy*vy;
	if ( c2 <= c1 ) {
		if (lpnt)
			*lpnt=*l1;
		return transform_distance_sq(l1, ref);
	}
	while (c1 > climit || c2 > climit) {
		c1/=256;
		c2/=256;
	}
	l.x=l0->x+vx*c1/c2;
	l.y=l0->y+vy*c1/c2;
	if (lpnt)
		*lpnt=l;
	return transform_distance_sq(&l, ref);
}


void
transform_print_deg(double deg)
{
	printf("%2.0f:%2.0f:%2.4f", floor(deg), fmod(deg*60,60), fmod(deg*3600,60));
}

int 
is_visible(struct transformation *t, struct coord *c)
{
	struct coord *r=t->rect;

	assert(c[0].x <= c[1].x);
	assert(c[0].y >= c[1].y);
	assert(r[0].x <= r[1].x);
	assert(r[0].y >= r[1].y);
	if (c[0].x > r[1].x)
		return 0;
	if (c[1].x < r[0].x)
		return 0;
	if (c[0].y < r[1].y)
		return 0;
	if (c[1].y > r[0].y)
		return 0;
	return 1;
}

int
is_line_visible(struct transformation *t, struct coord *c)
{
	struct coord *r=t->rect;

	assert(r[0].x <= r[1].x);
	assert(r[0].y >= r[1].y);
	if (MIN(c[0].x,c[1].x) > r[1].x)
		return 0;
	if (MAX(c[0].x,c[1].x) < r[0].x)
		return 0;
	if (MAX(c[0].y,c[1].y) < r[1].y)
		return 0;
	if (MIN(c[0].y,c[1].y) > r[0].y)
		return 0;
	return 1;
}

int 
is_point_visible(struct transformation *t, struct coord *c)
{
	struct coord *r=t->rect;

	assert(r[0].x <= r[1].x);
	assert(r[0].y >= r[1].y);
	if (c->x > r[1].x)
		return 0;
	if (c->x < r[0].x)
		return 0;
	if (c->y < r[1].y)
		return 0;
	if (c->y > r[0].y)
		return 0;
	return 1;
}


int
is_too_small(struct transformation *t, struct coord *c, int limit)
{
	return 0;
	if ((c[1].x-c[0].x) < limit*t->scale/16) {
		return 1;
	}
	if ((c[0].y-c[1].y) < limit*t->scale/16) {
		return 1;
	}
	return 0;	
}


void transform_limit_extend(struct coord *rect, struct coord *c)
{
	if (c->x < rect[0].x) rect[0].x=c->x;
	if (c->x > rect[1].x) rect[1].x=c->x;
	if (c->y < rect[1].y) rect[1].y=c->y;
	if (c->y > rect[0].y) rect[0].y=c->y;
}



int
transform_get_angle(struct coord *c, int dir)
{
	double angle;
	int dx=c[1].x-c[0].x;
	int dy=c[1].y-c[0].y;
	angle=atan2(dx,dy);
	angle*=180/M_PI;
	if (dir == -1)
		angle=angle-180;
	if (angle < 0)
		angle+=360;
	return angle;
}

/*
Note: there are many mathematically equivalent ways to express these formulas. As usual, not all of them are computationally equivalent.

L = latitude in radians (positive north)
Lo = longitude in radians (positive east)
E = easting (meters)
N = northing (meters)

For the sphere

E = r Lo
N = r ln [ tan (pi/4 + L/2) ]

where 

r = radius of the sphere (meters)
ln() is the natural logarithm

For the ellipsoid

E = a Lo
N = a * ln ( tan (pi/4 + L/2) * ( (1 - e * sin (L)) / (1 + e * sin (L))) ** (e/2)  )


                                               e
                                               -
                   pi     L     1 - e sin(L)   2
    =  a ln( tan( ---- + ---) (--------------)   )
                   4      2     1 + e sin(L) 


where

a = the length of the semi-major axis of the ellipsoid (meters)
e = the first eccentricity of the ellipsoid


*/



