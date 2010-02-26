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

#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <glib.h>
#include <string.h>
#include "config.h"
#include "coord.h"
#include "debug.h"
#include "item.h"
#include "map.h"
#include "transform.h"
#include "projection.h"
#include "point.h"

#define POST_SHIFT 8

struct transformation {
	int yaw;		/* Rotation angle */
	int pitch;
	int ddd;
 	int m00,m01,m02;	/* 3d transformation matrix */
 	int m10,m11,m12;	
 	int m20,m21,m22;	
	int xscale,yscale,wscale;
	int xscale3d,yscale3d,wscale3d;
#ifdef ENABLE_ROLL
	int roll;
	int hog;
#endif
 	navit_float im00,im01,im02;	/* inverse 3d transformation matrix */
 	navit_float im10,im11,im12;
 	navit_float im20,im21,im22;
	struct map_selection *map_sel;
	struct map_selection *screen_sel;
	struct point screen_center;
 	int screen_dist;
 	int offx,offy,offz;
	int znear,zfar;
	struct coord map_center;	/* Center of source rectangle */
	enum projection pro;
	navit_float scale;		/* Scale factor */
	int scale_shift;
	int order;
	int order_base;
};

#ifdef ENABLE_ROLL
#define HOG(t) ((t).hog)
#else
#define HOG(t) 0
#endif

static void
transform_set_screen_dist(struct transformation *t, int dist)
{
	t->screen_dist=dist;
	t->xscale3d=dist;
	t->yscale3d=dist;
	t->wscale3d=dist << POST_SHIFT;
}

static void
transform_setup_matrix(struct transformation *t)
{
	navit_float det;
	navit_float fac;
	navit_float yawc=navit_cos(-M_PI*t->yaw/180);
	navit_float yaws=navit_sin(-M_PI*t->yaw/180);
	navit_float pitchc=navit_cos(-M_PI*t->pitch/180);
	navit_float pitchs=navit_sin(-M_PI*t->pitch/180);
#ifdef ENABLE_ROLL	
	navit_float rollc=navit_cos(M_PI*t->roll/180);
	navit_float rolls=navit_sin(M_PI*t->roll/180);
#else
	navit_float rollc=1;
	navit_float rolls=0;
#endif

	int scale=t->scale;
	int order_dir=-1;

	dbg(1,"yaw=%d pitch=%d center=0x%x,0x%x\n", t->yaw, t->pitch, t->map_center.x, t->map_center.y);
	t->znear=1 << POST_SHIFT;
	t->zfar=300*t->znear;
	t->scale_shift=0;
	t->order=t->order_base;
	if (t->scale >= 1) {
		scale=t->scale;
	} else {
		scale=1.0/t->scale;
		order_dir=1;
	}
	while (scale > 1) {
		if (order_dir < 0)
			t->scale_shift++;
		t->order+=order_dir;
		scale >>= 1;
	}
	fac=(1 << POST_SHIFT) * (1 << t->scale_shift) / t->scale;
	dbg(1,"scale_shift=%d order=%d scale=%f fac=%f\n", t->scale_shift, t->order,t->scale,fac);
	
        t->m00=rollc*yawc*fac;
        t->m01=rollc*yaws*fac;
	t->m02=-rolls*fac;
	t->m10=(pitchs*rolls*yawc-pitchc*yaws)*(-fac);
	t->m11=(pitchs*rolls*yaws+pitchc*yawc)*(-fac);
	t->m12=pitchs*rollc*(-fac);
	t->m20=(pitchc*rolls*yawc+pitchs*yaws)*fac;
	t->m21=(pitchc*rolls*yaws-pitchs*yawc)*fac;
	t->m22=pitchc*rollc*fac;

	t->offx=t->screen_center.x;
	t->offy=t->screen_center.y;
	if (t->pitch) {
		t->ddd=1;
		t->offz=t->screen_dist;
		dbg(1,"near %d far %d\n",t->znear,t->zfar);
		t->xscale=t->xscale3d;
		t->yscale=t->yscale3d;
		t->wscale=t->wscale3d;
	} else {
		t->ddd=0;
		t->offz=0;
		t->xscale=1;
		t->yscale=1;
		t->wscale=1;
	}
	det=(navit_float)t->m00*(navit_float)t->m11*(navit_float)t->m22+
            (navit_float)t->m01*(navit_float)t->m12*(navit_float)t->m20+
            (navit_float)t->m02*(navit_float)t->m10*(navit_float)t->m21-
            (navit_float)t->m02*(navit_float)t->m11*(navit_float)t->m20-
            (navit_float)t->m01*(navit_float)t->m10*(navit_float)t->m22-
            (navit_float)t->m00*(navit_float)t->m12*(navit_float)t->m21;

	t->im00=(t->m11*t->m22-t->m12*t->m21)/det;
	t->im01=(t->m02*t->m21-t->m01*t->m22)/det;
	t->im02=(t->m01*t->m12-t->m02*t->m11)/det;
	t->im10=(t->m12*t->m20-t->m10*t->m22)/det;
	t->im11=(t->m00*t->m22-t->m02*t->m20)/det;
	t->im12=(t->m02*t->m10-t->m00*t->m12)/det;
	t->im20=(t->m10*t->m21-t->m11*t->m20)/det;
	t->im21=(t->m01*t->m20-t->m00*t->m21)/det;
	t->im22=(t->m00*t->m11-t->m01*t->m10)/det;
}

struct transformation *
transform_new(void)
{
	struct transformation *this_;

	this_=g_new0(struct transformation, 1);
	transform_set_screen_dist(this_, 100);
	this_->order_base=14;
#if 0
	this_->pitch=20;
#endif
#if 0
	this_->roll=30;
	this_->hog=1000;
#endif
	transform_setup_matrix(this_);
	return this_;
}

int
transform_get_hog(struct transformation *this_)
{
	return HOG(*this_);
}

void
transform_set_hog(struct transformation *this_, int hog)
{
#ifdef ENABLE_ROLL
	this_->hog=hog;
#else
	dbg(0,"not supported\n");
#endif

}

int
transform_get_attr(struct transformation *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	switch (type) {
#ifdef ENABLE_ROLL
	case attr_hog:
		attr->u.num=this_->hog;
		break;
#endif
	default:
		return 0;
	}
	attr->type=type;
	return 1;
}

int
transform_set_attr(struct transformation *this_, struct attr *attr)
{
	switch (attr->type) {
#ifdef ENABLE_ROLL
	case attr_hog:
		this_->hog=attr->u.num;
		return 1;
#endif
	default:
		return 0;
	}
}

int
transformation_get_order_base(struct transformation *this_)
{
	return this_->order_base;
}

void
transform_set_order_base(struct transformation *this_, int order_base)
{
	this_->order_base=order_base;
}


struct transformation *
transform_dup(struct transformation *t)
{
	struct transformation *ret=g_new0(struct transformation, 1);
	*ret=*t;
	return ret;
}

static const navit_float gar2geo_units = 360.0/(1<<24);
static const navit_float geo2gar_units = 1/(360.0/(1<<24));

void
transform_to_geo(enum projection pro, struct coord *c, struct coord_geo *g)
{
	int x,y,northern,zone;
	switch (pro) {
	case projection_mg:
		g->lng=c->x/6371000.0/M_PI*180;
		g->lat=navit_atan(exp(c->y/6371000.0))/M_PI*360-90;
		break;
	case projection_garmin:
		g->lng=c->x*gar2geo_units;
		g->lat=c->y*gar2geo_units;
		break;
	case projection_utm:
		x=c->x;
		y=c->y;
		northern=y >= 0;
		if (!northern) {
			y+=10000000;
		}
		zone=(x/1000000);
		x=x%1000000;
		transform_utm_to_geo(x, y, zone, northern, g);
		break;	
	default:
		break;
	}
}

void
transform_from_geo(enum projection pro, struct coord_geo *g, struct coord *c)
{
	switch (pro) {
	case projection_mg:
		c->x=g->lng*6371000.0*M_PI/180;
		c->y=log(navit_tan(M_PI_4+g->lat*M_PI/360))*6371000.0;
		break;
	case projection_garmin:
		c->x=g->lng*geo2gar_units;
		c->y=g->lat*geo2gar_units;
		break;
	default:
		break;
	}
}

void
transform_from_to_count(struct coord *cfrom, enum projection from, struct coord *cto, enum projection to, int count)
{
	struct coord_geo g;
	int i;

	for (i = 0 ; i < count ; i++) {
		transform_to_geo(from, cfrom, &g);
		transform_from_geo(to, &g, cto);
		cfrom++;
		cto++;
	}
}

void
transform_from_to(struct coord *cfrom, enum projection from, struct coord *cto, enum projection to)
{
	struct coord_geo g;
	transform_to_geo(from, cfrom, &g);
	transform_from_geo(to, &g, cto);
}

void
transform_geo_to_cart(struct coord_geo *geo, navit_float a, navit_float b, struct coord_geo_cart *cart)
{
	navit_float n,ee=1-b*b/(a*a);
	n = a/sqrtf(1-ee*navit_sin(geo->lat)*navit_sin(geo->lat));
        cart->x=n*navit_cos(geo->lat)*navit_cos(geo->lng);
        cart->y=n*navit_cos(geo->lat)*navit_sin(geo->lng);
        cart->z=n*(1-ee)*navit_sin(geo->lat);
}

void
transform_cart_to_geo(struct coord_geo_cart *cart, navit_float a, navit_float b, struct coord_geo *geo)
{
	navit_float lat,lati,n,ee=1-b*b/(a*a), lng = navit_tan(cart->y/cart->x);

        lat = navit_tan(cart->z / navit_sqrt((cart->x * cart->x) + (cart->y * cart->y)));
        do
        {
                lati = lat;

                n = a / navit_sqrt(1-ee*navit_sin(lat)*navit_sin(lat));
                lat = navit_atan((cart->z + ee * n * navit_sin(lat)) / navit_sqrt(cart->x * cart->x + cart->y * cart->y));
        }
        while (fabs(lat - lati) >= 0.000000000000001);

	geo->lng=lng/M_PI*180;
	geo->lat=lat/M_PI*180;
}


void
transform_utm_to_geo(const double UTMEasting, const double UTMNorthing, int ZoneNumber, int NorthernHemisphere, struct coord_geo *geo)
{
//converts UTM coords to lat/long.  Equations from USGS Bulletin 1532 
//East Longitudes are positive, West longitudes are negative. 
//North latitudes are positive, South latitudes are negative
//Lat and Long are in decimal degrees. 
	//Written by Chuck Gantz- chuck.gantz@globalstar.com

	double Lat, Long;
	double k0 = 0.99960000000000004;
	double a = 6378137;
	double eccSquared = 0.0066943799999999998;
	double eccPrimeSquared;
	double e1 = (1-sqrt(1-eccSquared))/(1+sqrt(1-eccSquared));
	double N1, T1, C1, R1, D, M;
	double LongOrigin;
	double mu, phi1, phi1Rad;
	double x, y;
	double rad2deg = 180/M_PI;

	x = UTMEasting - 500000.0; //remove 500,000 meter offset for longitude
	y = UTMNorthing;

	if (!NorthernHemisphere) {
		y -= 10000000.0;//remove 10,000,000 meter offset used for southern hemisphere
	}

	LongOrigin = (ZoneNumber - 1)*6 - 180 + 3;  //+3 puts origin in middle of zone

	eccPrimeSquared = (eccSquared)/(1-eccSquared);

	M = y / k0;
	mu = M/(a*(1-eccSquared/4-3*eccSquared*eccSquared/64-5*eccSquared*eccSquared*eccSquared/256));
	phi1Rad = mu	+ (3*e1/2-27*e1*e1*e1/32)*sin(2*mu) 
				+ (21*e1*e1/16-55*e1*e1*e1*e1/32)*sin(4*mu)
				+(151*e1*e1*e1/96)*sin(6*mu);
	phi1 = phi1Rad*rad2deg;

	N1 = a/sqrt(1-eccSquared*sin(phi1Rad)*sin(phi1Rad));
	T1 = tan(phi1Rad)*tan(phi1Rad);
	C1 = eccPrimeSquared*cos(phi1Rad)*cos(phi1Rad);
	R1 = a*(1-eccSquared)/pow(1-eccSquared*sin(phi1Rad)*sin(phi1Rad), 1.5);
	D = x/(N1*k0);

	Lat = phi1Rad - (N1*tan(phi1Rad)/R1)*(D*D/2-(5+3*T1+10*C1-4*C1*C1-9*eccPrimeSquared)*D*D*D*D/24
					+(61+90*T1+298*C1+45*T1*T1-252*eccPrimeSquared-3*C1*C1)*D*D*D*D*D*D/720);
	Lat = Lat * rad2deg;

	Long = (D-(1+2*T1+C1)*D*D*D/6+(5-2*C1+28*T1-3*C1*C1+8*eccPrimeSquared+24*T1*T1)
					*D*D*D*D*D/120)/cos(phi1Rad);
	Long = LongOrigin + Long * rad2deg;

	geo->lat=Lat;
	geo->lng=Long;
}

void
transform_datum(struct coord_geo *from, enum map_datum from_datum, struct coord_geo *to, enum map_datum to_datum)
{
}

int
transform(struct transformation *t, enum projection pro, struct coord *c, struct point *p, int count, int mindist, int width, int *width_return)
{
	struct coord c1;
	int xcn, ycn; 
	struct coord_geo g;
	int xc, yc, zc=0, xco=0, yco=0, zco=0;
	int xm,ym,zct;
	int zlimit=t->znear;
	int visible, visibleo=-1;
	int i,j = 0,k=0;
	dbg(1,"count=%d\n", count);
	for (i=0; i < count; i++) {
		if (pro == t->pro) {
			xc=c[i].x;
			yc=c[i].y;
		} else {
			transform_to_geo(pro, &c[i], &g);
			transform_from_geo(t->pro, &g, &c1);
			xc=c1.x;
			yc=c1.y;
		}
		if (i != 0 && i != count-1 && mindist) {
			if (xc > c[k].x-mindist && xc < c[k].x+mindist && yc > c[k].y-mindist && yc < c[k].y+mindist &&
				(c[i+1].x != c[0].x || c[i+1].y != c[0].y))
				continue;
			k=i;
		}
		xm=xc;
		ym=yc;
//		dbg(2,"0x%x, 0x%x - 0x%x,0x%x contains 0x%x,0x%x\n", t->r.lu.x, t->r.lu.y, t->r.rl.x, t->r.rl.y, c->x, c->y);
//		ret=coord_rect_contains(&t->r, c);
		xc-=t->map_center.x;
		yc-=t->map_center.y;
		xc >>= t->scale_shift;
		yc >>= t->scale_shift;
		xm=xc;
		ym=yc;

		xcn=xc*t->m00+yc*t->m01+HOG(*t)*t->m02;
		ycn=xc*t->m10+yc*t->m11+HOG(*t)*t->m12;

		if (t->ddd) {
			zc=(xc*t->m20+yc*t->m21+HOG(*t)*t->m22);
			zct=zc;
			zc+=t->offz << POST_SHIFT;
			dbg(1,"zc=%d\n", zc);
			dbg(1,"zc(%d)=xc(%d)*m20(%d)+yc(%d)*m21(%d)\n", (xc*t->m20+yc*t->m21), xc, t->m20, yc, t->m21);
			/* visibility */
			visible=(zc < zlimit ? 0:1);
			dbg(1,"visible=%d old %d\n", visible, visibleo);
			if (visible != visibleo && visibleo != -1) { 
				dbg(1,"clipping (%d,%d,%d)-(%d,%d,%d) (%d,%d,%d)\n", xcn, ycn, zc, xco, yco, zco, xco-xcn, yco-ycn, zco-zc);
				if (zco != zc) {
					xcn=xcn+(long long)(xco-xcn)*(zlimit-zc)/(zco-zc);
					ycn=ycn+(long long)(yco-ycn)*(zlimit-zc)/(zco-zc);
				}
				dbg(1,"result (%d,%d,%d) * %d / %d\n", xcn,ycn,zc,zlimit-zc,zco-zc);
				zc=zlimit;
				xco=xcn;
				yco=ycn;
				zco=zc;
				if (visible)
					i--;
				visibleo=visible;
			} else {
				xco=xcn;
				yco=ycn;
				zco=zc;
				visibleo=visible;
				if (! visible)
					continue;
			}
			dbg(1,"zc=%d\n", zc);
			dbg(1,"xcn %d ycn %d\n", xcn, ycn);
			dbg(1,"%d,%d %d\n",xc,yc,zc);
#if 0
			dbg(0,"%d/%d=%d %d/%d=%d\n",xcn,xc,xcn/xc,ycn,yc,ycn/yc);
#endif
#if 1
			xc=(long long)xcn*t->xscale/zc;
			yc=(long long)ycn*t->yscale/zc;
#else
			xc=xcn/(1000+zc);
			yc=ycn/(1000+zc);
#endif
#if 0
			dbg(1,"%d,%d %d\n",xc,yc,zc);
#endif
		} else {
			xc=xcn;
			yc=ycn;
			xc>>=POST_SHIFT;
			yc>>=POST_SHIFT;
		}
		xc+=t->offx;
		yc+=t->offy;
		p[j].x=xc;
		p[j].y=yc;
		if (width_return) {
			if (t->ddd) 
				width_return[j]=width*t->wscale/zc;
			else 
				width_return[j]=width;
		}
		j++;
	}
	return j;
}

static void
transform_apply_inverse_matrix(struct transformation *t, struct coord_geo_cart *in, struct coord_geo_cart *out)
{
	out->x=in->x*t->im00+in->y*t->im01+in->z*t->im02;
	out->y=in->x*t->im10+in->y*t->im11+in->z*t->im12;
	out->z=in->x*t->im20+in->y*t->im21+in->z*t->im22;
}

static int
transform_zplane_intersection(struct coord_geo_cart *p1, struct coord_geo_cart *p2, navit_float z, struct coord_geo_cart *result)
{
	navit_float dividend=z-p1->z;
	navit_float divisor=p2->z-p1->z;
	navit_float q;
	if (!divisor) {
		if (dividend) 
			return 0;	/* no intersection */
		else
			return 3;	/* identical planes */
	}
	q=dividend/divisor;
	result->x=p1->x+q*(p2->x-p1->x);
	result->y=p1->y+q*(p2->y-p1->y);
	result->z=z;
	if (q >= 0 && q <= 1)
		return 1;	/* intersection within [p1,p2] */
	return 2; /* intersection without [p1,p2] */
}

static void
transform_screen_to_3d(struct transformation *t, struct point *p, navit_float z, struct coord_geo_cart *cg)
{
	double xc,yc;
	double offz=t->offz << POST_SHIFT;
	xc=p->x - t->offx;
	yc=p->y - t->offy;
	cg->x=xc*z/t->xscale;
	cg->y=yc*z/t->yscale;
	cg->z=z-offz;
}

static int
transform_reverse_near_far(struct transformation *t, struct point *p, struct coord *c, int near, int far)
{
        double xc,yc;
	dbg(1,"%d,%d\n",p->x,p->y);
	if (t->ddd) {
		struct coord_geo_cart nearc,farc,nears,fars,intersection;
		transform_screen_to_3d(t, p, near, &nearc);	
		transform_screen_to_3d(t, p, far, &farc);
		transform_apply_inverse_matrix(t, &nearc, &nears);
		transform_apply_inverse_matrix(t, &farc, &fars);
		if (transform_zplane_intersection(&nears, &fars, HOG(*t), &intersection) != 1)
			return 0;
		xc=intersection.x;
		yc=intersection.y;
	} else {
        	double xcn,ycn;
		xcn=p->x - t->offx;
		ycn=p->y - t->offy;
		xc=(xcn*t->im00+ycn*t->im01)*(1 << POST_SHIFT);
		yc=(xcn*t->im10+ycn*t->im11)*(1 << POST_SHIFT);
	}
	c->x=xc*(1 << t->scale_shift)+t->map_center.x;
	c->y=yc*(1 << t->scale_shift)+t->map_center.y;
	return 1;
}

int
transform_reverse(struct transformation *t, struct point *p, struct coord *c)
{
	return transform_reverse_near_far(t, p, c, t->znear, t->zfar);
}

enum projection
transform_get_projection(struct transformation *this_)
{
	return this_->pro;
}

void
transform_set_projection(struct transformation *this_, enum projection pro)
{
	this_->pro=pro;
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

struct map_selection *
transform_get_selection(struct transformation *this_, enum projection pro, int order)
{

	struct map_selection *ret,*curri,*curro;
	struct coord_geo g;
	
	ret=map_selection_dup(this_->map_sel);
	curri=this_->map_sel;
	curro=ret;
	while (curri) {
		if (this_->pro != pro) {
			transform_to_geo(this_->pro, &curri->u.c_rect.lu, &g);
			transform_from_geo(pro, &g, &curro->u.c_rect.lu);
			dbg(1,"%f,%f", g.lat, g.lng);
			transform_to_geo(this_->pro, &curri->u.c_rect.rl, &g);
			transform_from_geo(pro, &g, &curro->u.c_rect.rl);
			dbg(1,": - %f,%f\n", g.lat, g.lng);
		}
		dbg(1,"transform rect for %d is %d,%d - %d,%d\n", pro, curro->u.c_rect.lu.x, curro->u.c_rect.lu.y, curro->u.c_rect.rl.x, curro->u.c_rect.rl.y);
		curro->order+=order;
		curro->u.c_rect.lu.x-=500;
		curro->u.c_rect.lu.y+=500;
		curro->u.c_rect.rl.x+=500;
		curro->u.c_rect.rl.y-=500;
		curro->range=item_range_all;
		curri=curri->next;
		curro=curro->next;
	}
	return ret;
}

struct coord *
transform_center(struct transformation *this_)
{
	return &this_->map_center;
}

struct coord *
transform_get_center(struct transformation *this_)
{
	return &this_->map_center;
}

void
transform_set_center(struct transformation *this_, struct coord *c)
{
	this_->map_center=*c;
}


void
transform_set_yaw(struct transformation *t,int yaw)
{
	t->yaw=yaw;
	transform_setup_matrix(t);
}

int
transform_get_yaw(struct transformation *this_)
{
	return this_->yaw;
}

void
transform_set_pitch(struct transformation *this_,int pitch)
{
	this_->pitch=pitch;
	transform_setup_matrix(this_);
}
int
transform_get_pitch(struct transformation *this_)
{
	return this_->pitch;
}

void
transform_set_roll(struct transformation *this_,int roll)
{
#ifdef ENABLE_ROLL
	this_->roll=roll;
	transform_setup_matrix(this_);
#else
	dbg(0,"not supported\n");
#endif
}

int
transform_get_roll(struct transformation *this_)
{
#ifdef ENABLE_ROLL
	return this_->roll;
#else
	return 0;
#endif
}

void
transform_set_distance(struct transformation *this_,int distance)
{
	transform_set_screen_dist(this_, distance);
	transform_setup_matrix(this_);
}

int
transform_get_distance(struct transformation *this_)
{
	return this_->screen_dist;
}

void
transform_set_scales(struct transformation *this_, int xscale, int yscale, int wscale)
{
	this_->xscale3d=xscale;
	this_->yscale3d=yscale;
	this_->wscale3d=wscale;
}

void
transform_set_screen_selection(struct transformation *t, struct map_selection *sel)
{
	map_selection_destroy(t->screen_sel);
	t->screen_sel=map_selection_dup(sel);
	if (sel) {
		t->screen_center.x=(sel->u.p_rect.rl.x-sel->u.p_rect.lu.x)/2;
		t->screen_center.y=(sel->u.p_rect.rl.y-sel->u.p_rect.lu.y)/2;
		transform_setup_matrix(t);
	}
}

void
transform_set_screen_center(struct transformation *t, struct point *p)
{
	t->screen_center=*p;
}

#if 0
void
transform_set_size(struct transformation *t, int width, int height)
{
	t->width=width;
	t->height=height;
}
#endif

void
transform_get_size(struct transformation *t, int *width, int *height)
{
	struct point_rect *r;
	if (t->screen_sel) {
		r=&t->screen_sel->u.p_rect;
		*width=r->rl.x-r->lu.x;
		*height=r->rl.y-r->lu.y;
	}
}

void
transform_setup(struct transformation *t, struct pcoord *c, int scale, int yaw)
{
	t->pro=c->pro;
	t->map_center.x=c->x;
	t->map_center.y=c->y;
	t->scale=scale/16.0;
	transform_set_yaw(t, yaw);
}

#if 0

void
transform_setup_source_rect_limit(struct transformation *t, struct coord *center, int limit)
{
	t->center=*center;
	t->scale=1;
	t->angle=0;
	t->r.lu.x=center->x-limit;
	t->r.rl.x=center->x+limit;
	t->r.rl.y=center->y-limit;
	t->r.lu.y=center->y+limit;
}
#endif

void
transform_setup_source_rect(struct transformation *t)
{
	int i;
	struct coord screen[4];
	struct point screen_pnt[4];
	struct point_rect *pr;
	struct map_selection *ms,*msm,*next,**msm_last;
	ms=t->map_sel;
	while (ms) {
		next=ms->next;
		g_free(ms);
		ms=next;
	}
	t->map_sel=NULL;
	msm_last=&t->map_sel;
	ms=t->screen_sel;
	while (ms) {
		msm=g_new0(struct map_selection, 1);
		*msm=*ms;
		pr=&ms->u.p_rect;
		screen_pnt[0].x=pr->lu.x;	/* left upper */
		screen_pnt[0].y=pr->lu.y;
		screen_pnt[1].x=pr->rl.x;	/* right upper */
		screen_pnt[1].y=pr->lu.y;
		screen_pnt[2].x=pr->rl.x;	/* right lower */
		screen_pnt[2].y=pr->rl.y;
		screen_pnt[3].x=pr->lu.x;	/* left lower */
		screen_pnt[3].y=pr->rl.y;
		if (t->ddd) {
			struct coord_geo_cart tmp,cg[8];
			struct coord c;
			int valid=0;
			char edgenodes[]={
				0,1,
				1,2,
				2,3,
				3,0,
				4,5,
				5,6,
				6,7,
				7,4,
				0,4,
				1,5,
				2,6,
				3,7};	
			for (i = 0 ; i < 8 ; i++) {
				transform_screen_to_3d(t, &screen_pnt[i%4], (i >= 4 ? t->zfar:t->znear), &tmp);
				transform_apply_inverse_matrix(t, &tmp, &cg[i]);
			}
			msm->u.c_rect.lu.x=0;
			msm->u.c_rect.lu.y=0;
			msm->u.c_rect.rl.x=0;
			msm->u.c_rect.rl.y=0;
			for (i = 0 ; i < 12 ; i++) {
				if (transform_zplane_intersection(&cg[edgenodes[i*2]], &cg[edgenodes[i*2+1]], HOG(*t), &tmp) == 1) {
					c.x=tmp.x*(1 << t->scale_shift)+t->map_center.x;
					c.y=tmp.y*(1 << t->scale_shift)+t->map_center.y;
					dbg(1,"intersection with edge %d at 0x%x,0x%x\n",i,c.x,c.y);
					if (valid)
						coord_rect_extend(&msm->u.c_rect, &c);
					else {
						msm->u.c_rect.lu=c;
						msm->u.c_rect.rl=c;
						valid=1;
					}
					dbg(1,"rect 0x%x,0x%x - 0x%x,0x%x\n",msm->u.c_rect.lu.x,msm->u.c_rect.lu.y,msm->u.c_rect.rl.x,msm->u.c_rect.rl.y);
				}
			}
		} else {
			for (i = 0 ; i < 4 ; i++) {
				transform_reverse(t, &screen_pnt[i], &screen[i]);
				dbg(1,"map(%d) %d,%d=0x%x,0x%x\n", i,screen_pnt[i].x, screen_pnt[i].y, screen[i].x, screen[i].y);
			}
			msm->u.c_rect.lu.x=min4(screen[0].x,screen[1].x,screen[2].x,screen[3].x);
			msm->u.c_rect.rl.x=max4(screen[0].x,screen[1].x,screen[2].x,screen[3].x);
			msm->u.c_rect.rl.y=min4(screen[0].y,screen[1].y,screen[2].y,screen[3].y);
			msm->u.c_rect.lu.y=max4(screen[0].y,screen[1].y,screen[2].y,screen[3].y);
		}
		dbg(1,"%dx%d\n", msm->u.c_rect.rl.x-msm->u.c_rect.lu.x,
				 msm->u.c_rect.lu.y-msm->u.c_rect.rl.y);
		*msm_last=msm;
		msm_last=&msm->next;
		ms=ms->next;
	}
}

long
transform_get_scale(struct transformation *t)
{
	return (int)(t->scale*16);
}

void
transform_set_scale(struct transformation *t, long scale)
{
	t->scale=scale/16.0;
	transform_setup_matrix(t);
}


int
transform_get_order(struct transformation *t)
{
	dbg(1,"order %d\n", t->order);
	return t->order;
}



#define TWOPI (M_PI*2)
#define GC2RAD(c) ((c) * TWOPI/(1<<24))
#define minf(a,b) ((a) < (b) ? (a) : (b))

static double
transform_distance_garmin(struct coord *c1, struct coord *c2)
{
#ifdef USE_HALVESINE
	static const int earth_radius = 6371*1000; //m change accordingly
// static const int earth_radius  = 3960; //miles
 
//Point 1 cords
	navit_float lat1  = GC2RAD(c1->y);
	navit_float long1 = GC2RAD(c1->x);

//Point 2 cords
	navit_float lat2  = GC2RAD(c2->y);
	navit_float long2 = GC2RAD(c2->x);

//Haversine Formula
	navit_float dlong = long2-long1;
	navit_float dlat  = lat2-lat1;

	navit_float sinlat  = navit_sin(dlat/2);
	navit_float sinlong = navit_sin(dlong/2);
 
	navit_float a=(sinlat*sinlat)+navit_cos(lat1)*navit_cos(lat2)*(sinlong*sinlong);
	navit_float c=2*navit_asin(minf(1,navit_sqrt(a)));
#ifdef AVOID_FLOAT
	return round(earth_radius*c);
#else
	return earth_radius*c;
#endif
#else
#define GMETER 2.3887499999999999
	navit_float dx,dy;
	dx=c1->x-c2->x;
	dy=c1->y-c2->y;
	return navit_sqrt(dx*dx+dy*dy)*GMETER;
#undef GMETER
#endif
}

double
transform_scale(int y)
{
	struct coord c;
	struct coord_geo g;
	c.x=0;
	c.y=y;
	transform_to_geo(projection_mg, &c, &g);
	return 1/navit_cos(g.lat/180*M_PI);
}

#ifdef AVOID_FLOAT
static int
tab_sqrt[]={14142,13379,12806,12364,12018,11741,11517,11333,11180,11051,10943,10850,10770,10701,10640,10587,10540,10499,10462,10429,10400,10373,10349,10327,10307,10289,10273,10257,10243,10231,10219,10208};

static int tab_int_step = 0x20000;
static int tab_int_scale[]={10000,10002,10008,10019,10033,10052,10076,10103,10135,10171,10212,10257,10306,10359,10417,10479,10546,10617,10693,10773,10858,10947,11041,11140,11243,11352,11465,11582,11705,11833,11965,12103,12246,12394,12547,12706,12870,13039,13214,13395,13581,13773,13971,14174,14384,14600,14822,15050,15285,15526,15774,16028,16289,16557,16832,17114,17404,17700,18005,18316,18636,18964,19299,19643,19995,20355,20724,21102,21489,21885,22290,22705,23129,23563,24007,24461,24926,25401,25886,26383,26891};

int transform_int_scale(int y)
{
	int a=tab_int_step,i,size = sizeof(tab_int_scale)/sizeof(int);
	if (y < 0)
		y=-y;
	i=y/tab_int_step;
	if (i < size-1) 
		return tab_int_scale[i]+((tab_int_scale[i+1]-tab_int_scale[i])*(y-i*tab_int_step))/tab_int_step;
	return tab_int_scale[size-1];
}
#endif

double
transform_distance(enum projection pro, struct coord *c1, struct coord *c2)
{
	if (pro == projection_mg) {
#ifndef AVOID_FLOAT 
	double dx,dy,scale=transform_scale((c1->y+c2->y)/2);
	dx=c1->x-c2->x;
	dy=c1->y-c2->y;
	return sqrt(dx*dx+dy*dy)/scale;
#else
	int dx,dy,f,scale=transform_int_scale((c1->y+c2->y)/2);
	dx=c1->x-c2->x;
	dy=c1->y-c2->y;
	if (dx < 0)
		dx=-dx;
	if (dy < 0)
		dy=-dy;
	while (dx > 20000 || dy > 20000) {
		dx/=10;
		dy/=10;
		scale/=10;
	}
	if (! dy)
		return dx*10000/scale;
	if (! dx)
		return dy*10000/scale;
	if (dx > dy) {
		f=dx*8/dy-8;
		if (f >= 32)
			return dx*10000/scale;
		return dx*tab_sqrt[f]/scale;
	} else {
		f=dy*8/dx-8;
		if (f >= 32)
			return dy*10000/scale;
		return dy*tab_sqrt[f]/scale;
	}
#endif
	} else if (pro == projection_garmin) {
		return transform_distance_garmin(c1, c2);
	} else {
		dbg(0,"Unknown projection: %d\n", pro);
		return 0;
	}
}

void
transform_project(enum projection pro, struct coord *c, int distance, int angle, struct coord *res)
{
	double scale;
	switch (pro) {
	case projection_mg:
		scale=transform_scale(c->y);
		res->x=c->x+distance*sin(angle*M_PI/180)*scale;
		res->y=c->y+distance*cos(angle*M_PI/180)*scale;
		break;
	default:
		dbg(0,"Unsupported projection: %d\n", pro);
		return;
	}
	
}


double
transform_polyline_length(enum projection pro, struct coord *c, int count)
{
	double ret=0;
	int i;

	for (i = 0 ; i < count-1 ; i++) 
		ret+=transform_distance(pro, &c[i], &c[i+1]);
	return ret;
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
transform_distance_sq_pc(struct pcoord *c1, struct pcoord *c2)
{
	struct coord p1,p2;
	p1.x = c1->x; p1.y = c1->y;
	p2.x = c2->x; p2.y = c2->y;
	return transform_distance_sq(&p1, &p2);
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

int
transform_distance_polyline_sq(struct coord *c, int count, struct coord *ref, struct coord *lpnt, int *pos)
{
	int i,dist,distn;
	struct coord lp;
	if (count < 2)
		return INT_MAX;
	if (pos)
		*pos=0;
	dist=transform_distance_line_sq(&c[0], &c[1], ref, lpnt);
	for (i=2 ; i < count ; i++) {
		distn=transform_distance_line_sq(&c[i-1], &c[i], ref, &lp);
		if (distn < dist) {
			dist=distn;
			if (lpnt)
				*lpnt=lp;
			if (pos)
				*pos=i-1;
		}
	}
	return dist;
}

int
transform_douglas_peucker(struct coord *in, int count, int dist_sq, struct coord *out)
{
	int ret=0;
	int i,d,dmax=0, idx=0;
	for (i = 1; i < count-1 ; i++) {
		d=transform_distance_line_sq(&in[0], &in[count-1], &in[i], NULL);
		if (d > dmax) {
			idx=i;
			dmax=d;
		}
	}
	if (dmax > dist_sq) {
		ret=transform_douglas_peucker(in, idx+1, dist_sq, out)-1;
		ret+=transform_douglas_peucker(in+idx, count-idx, dist_sq, out+ret);
	} else {
		if (count > 0)
			out[ret++]=in[0];
		if (count > 1)
			out[ret++]=in[count-1];
	}
	return ret;
}


void
transform_print_deg(double deg)
{
	printf("%2.0f:%2.0f:%2.4f", floor(deg), fmod(deg*60,60), fmod(deg*3600,60));
}

#ifdef AVOID_FLOAT
static int tab_atan[]={0,262,524,787,1051,1317,1584,1853,2126,2401,2679,2962,3249,3541,3839,4142,4452,4770,5095,5430,5774,6128,6494,6873,7265,7673,8098,8541,9004,9490,10000,10538};

static int
atan2_int_lookup(int val)
{
	int len=sizeof(tab_atan)/sizeof(int);
	int i=len/2;
	int p=i-1;
	for (;;) {
		i>>=1;
		if (val < tab_atan[p])
			p-=i;
		else
			if (val < tab_atan[p+1])
				return p+(p>>1);
			else
				p+=i;
	}
}

static int
atan2_int(int dx, int dy)
{
	int f,mul=1,add=0,ret;
	if (! dx) {
		return dy < 0 ? 180 : 0;
	}
	if (! dy) {
		return dx < 0 ? -90 : 90;
	}
	if (dx < 0) {
		dx=-dx;
		mul=-1;
	}
	if (dy < 0) {
		dy=-dy;
		add=180*mul;
		mul*=-1;
	}
	while (dx > 20000 || dy > 20000) {
		dx/=10;
		dy/=10;
	}
	if (dx > dy) {
		ret=90-atan2_int_lookup(dy*10000/dx);
	} else {
		ret=atan2_int_lookup(dx*10000/dy);
	}
	return ret*mul+add;
}
#endif

int
transform_get_angle_delta(struct coord *c1, struct coord *c2, int dir)
{
	int dx=c2->x-c1->x;
	int dy=c2->y-c1->y;
#ifndef AVOID_FLOAT 
	double angle;
	angle=atan2(dx,dy);
	angle*=180/M_PI;
#else
	int angle;
	angle=atan2_int(dx,dy);
#endif
	if (dir == -1)
		angle=angle-180;
	if (angle < 0)
		angle+=360;
	return angle;
}

int
transform_within_border(struct transformation *this_, struct point *p, int border)
{
	struct map_selection *ms=this_->screen_sel;
	while (ms) {
		struct point_rect *r=&ms->u.p_rect;
		if (p->x >= r->lu.x+border && p->x <= r->rl.x-border &&
		    p->y >= r->lu.y+border && p->y <= r->rl.y-border)
			return 1;
		ms=ms->next;
	}
	return 0;
}

int
transform_within_dist_point(struct coord *ref, struct coord *c, int dist)
{
	if (c->x-dist > ref->x)
		return 0;
	if (c->x+dist < ref->x)
		return 0;
	if (c->y-dist > ref->y)
		return 0;
	if (c->y+dist < ref->y)
		return 0;
	if ((c->x-ref->x)*(c->x-ref->x) + (c->y-ref->y)*(c->y-ref->y) <= dist*dist) 
		return 1;
        return 0;
}

int
transform_within_dist_line(struct coord *ref, struct coord *c0, struct coord *c1, int dist)
{
	int vx,vy,wx,wy;
	int n1,n2;
	struct coord lc;

	if (c0->x < c1->x) {
		if (c0->x-dist > ref->x)
			return 0;
		if (c1->x+dist < ref->x)
			return 0;
	} else {
		if (c1->x-dist > ref->x)
			return 0;
		if (c0->x+dist < ref->x)
			return 0;
	}
	if (c0->y < c1->y) {
		if (c0->y-dist > ref->y)
			return 0;
		if (c1->y+dist < ref->y)
			return 0;
	} else {
		if (c1->y-dist > ref->y)
			return 0;
		if (c0->y+dist < ref->y)
			return 0;
	}
	vx=c1->x-c0->x;
	vy=c1->y-c0->y;
	wx=ref->x-c0->x;
	wy=ref->y-c0->y;

	n1=vx*wx+vy*wy;
	if ( n1 <= 0 )
		return transform_within_dist_point(ref, c0, dist);
	n2=vx*vx+vy*vy;
	if ( n2 <= n1 )
		return transform_within_dist_point(ref, c1, dist);

	lc.x=c0->x+vx*n1/n2;
	lc.y=c0->y+vy*n1/n2;
	return transform_within_dist_point(ref, &lc, dist);
}

int
transform_within_dist_polyline(struct coord *ref, struct coord *c, int count, int close, int dist)
{
	int i;
	for (i = 0 ; i < count-1 ; i++) {
		if (transform_within_dist_line(ref,c+i,c+i+1,dist)) {
			return 1;
		}
	}
	if (close)
		return (transform_within_dist_line(ref,c,c+count-1,dist));
	return 0;
}

int
transform_within_dist_polygon(struct coord *ref, struct coord *c, int count, int dist)
{
	int i, j, ci = 0;
	for (i = 0, j = count-1; i < count; j = i++) {
		if ((((c[i].y <= ref->y) && ( ref->y < c[j].y )) ||
		((c[j].y <= ref->y) && ( ref->y < c[i].y))) &&
		(ref->x < (c[j].x - c[i].x) * (ref->y - c[i].y) / (c[j].y - c[i].y) + c[i].x))
			ci = !ci;
	}
	if (! ci) {
		if (dist)
			return transform_within_dist_polyline(ref, c, count, dist, 1);
		else
			return 0;
	}
	return 1;
}

int
transform_within_dist_item(struct coord *ref, enum item_type type, struct coord *c, int count, int dist)
{
	if (type < type_line)
		return transform_within_dist_point(ref, c, dist);
	if (type < type_area)
		return transform_within_dist_polyline(ref, c, count, 0, dist);
	return transform_within_dist_polygon(ref, c, count, dist);
}

void
transform_destroy(struct transformation *t)
{
	g_free(t);
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


