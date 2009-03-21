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
 	int m00,m01,m10,m11;	/* 2d transformation matrix */
	int xyscale;
 	int m20,m21; 		/* additional 3d parameters */
#ifdef ENABLE_ROLL
	int roll;
 	int m02,m12,m22; 
	int hog;
 	navit_float im02,im12,im20,im21,im22;
#endif
 	navit_float im00,im01,im10,im11;	/* inverse 2d transformation matrix */
	struct map_selection *map_sel;
	struct map_selection *screen_sel;
	struct point screen_center;
 	int screen_dist;
 	int offx,offy,offz;
	struct coord map_center;	/* Center of source rectangle */
	enum projection pro;
	navit_float scale;		/* Scale factor */
	int scale_shift;
	int order;
	int order_base;
};



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
#endif

	int scale=t->scale;
	int order_dir=-1;

	dbg(1,"yaw=%d pitch=%d center=0x%x,0x%x\n", t->yaw, t->pitch, t->map_center.x, t->map_center.y);
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
	
#ifdef ENABLE_ROLL
        t->m00=rollc*yawc*fac;
        t->m01=rollc*yaws*fac;
	t->m02=-rolls*fac;
	t->m10=(pitchs*rolls*yawc-pitchc*yaws)*(-fac);
	t->m11=(pitchs*rolls*yaws+pitchc*yawc)*(-fac);
	t->m12=pitchs*rollc*(-fac);
	t->m20=(pitchc*rolls*yawc+pitchs*yaws)*fac;
	t->m21=(pitchc*rolls*yaws-pitchs*yawc)*fac;
	t->m22=pitchc*rollc*fac;
#else
        t->m00=yawc*fac;
        t->m01=yaws*fac;
	t->m10=(-pitchc*yaws)*(-fac);
	t->m11=pitchc*yawc*(-fac);
	t->m20=pitchs*yaws*fac;
	t->m21=(-pitchs*yawc)*fac;
#endif
	t->offz=0;
	t->xyscale=1;
	t->ddd=0;
	t->offx=t->screen_center.x;
	t->offy=t->screen_center.y;
	if (t->pitch) {
		t->ddd=1;
		t->offz=t->screen_dist;
		t->xyscale=t->offz;
	}
#ifdef ENABLE_ROLL
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
#else
	det=((navit_float)t->m00*(navit_float)t->m11-(navit_float)t->m01*(navit_float)t->m10);
	t->im00=t->m11/det;
	t->im01=-t->m01/det;
	t->im10=-t->m10/det;
	t->im11=t->m00/det;
#endif
}

struct transformation *
transform_new(void)
{
	struct transformation *this_;

	this_=g_new0(struct transformation, 1);
	this_->screen_dist=100;
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

#ifdef ENABLE_ROLL

int
transform_get_hog(struct transformation *this_)
{
	return this_->hog;
}

void
transform_set_hog(struct transformation *this_, int hog)
{
	this_->hog=hog;
}

#else

int
transform_get_hog(struct transformation *this_)
{
	return 0;
}

void
transform_set_hog(struct transformation *this_, int hog)
{
	dbg(0,"not supported\n");
}

#endif

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
	switch (pro) {
	case projection_mg:
		g->lng=c->x/6371000.0/M_PI*180;
		g->lat=navit_atan(exp(c->y/6371000.0))/M_PI*360-90;
		break;
	case projection_garmin:
		g->lng=c->x*gar2geo_units;
		g->lat=c->y*gar2geo_units;
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
transform_datum(struct coord_geo *from, enum map_datum from_datum, struct coord_geo *to, enum map_datum to_datum)
{
}

int
transform(struct transformation *t, enum projection pro, struct coord *c, struct point *p, int count, int unique, int width, int *width_return)
{
	struct coord c1;
	int xcn, ycn; 
	struct coord_geo g;
	int xc, yc, zc=0, xco=0, yco=0, zco=0;
	int xm,ym,zct;
	int zlimit=1000;
	int visible, visibleo=-1;
	int i,j = 0;
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
		xm=xc;
		ym=yc;
//		dbg(2,"0x%x, 0x%x - 0x%x,0x%x contains 0x%x,0x%x\n", t->r.lu.x, t->r.lu.y, t->r.rl.x, t->r.rl.y, c->x, c->y);
//		ret=coord_rect_contains(&t->r, c);
		xc-=t->map_center.x;
		yc-=t->map_center.y;
		xc >>= t->scale_shift;
		yc >>= t->scale_shift;
#ifdef ENABLE_ROLL		
		xcn=xc*t->m00+yc*t->m01+t->hog*t->m02;
		ycn=xc*t->m10+yc*t->m11+t->hog*t->m12;
#else
		xcn=xc*t->m00+yc*t->m01;
		ycn=xc*t->m10+yc*t->m11;
#endif

		if (t->ddd) {
#ifdef ENABLE_ROLL		
			zc=(xc*t->m20+yc*t->m21+t->hog*t->m22);
#else
			zc=(xc*t->m20+yc*t->m21);
#endif
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
			xc=(long long)xcn*t->xyscale/zc;
			yc=(long long)ycn*t->xyscale/zc;
#else
			xc=xcn/(1000+zc);
			yc=ycn/(1000+zc);
#endif
			dbg(1,"%d,%d %d\n",xc,yc,zc);
		} else {
			xc=xcn;
			yc=ycn;
			xc>>=POST_SHIFT;
			yc>>=POST_SHIFT;
		}
		xc+=t->offx;
		yc+=t->offy;
		dbg(1,"xc=%d yc=%d\n", xc, yc);
		if (j == 0 || !unique || p[j-1].x != xc || p[j-1].y != yc) {
			p[j].x=xc;
			p[j].y=yc;
			if (width_return) {
				if (t->ddd) 
					width_return[j]=width*(t->offz << POST_SHIFT)/zc;
				else 
					width_return[j]=width;
			}
			j++;
		}
	}
	return j;
}

void
transform_reverse(struct transformation *t, struct point *p, struct coord *c)
{
        double zc,xc,yc,xcn,ycn,q;
	double offz=t->offz << POST_SHIFT;
	xc=p->x - t->offx;
	yc=p->y - t->offy;
	if (t->ddd) {
		xc/=t->xyscale;
		yc/=t->xyscale;
		double f00=xc*t->im00*t->m20;
		double f01=yc*t->im01*t->m20;
		double f10=xc*t->im10*t->m21;
		double f11=yc*t->im11*t->m21;
#ifdef ENABLE_ROLL	
		q=(1-f00-f01-t->im02*t->m20-f10-f11-t->im12*t->m21);
		if (q < 0) 
			q=0.15;
		zc=(offz*((f00+f01+f10+f11))+t->hog*t->m22)/q;
#else
		q=(1-f00-f01-f10-f11);
                if (q < 0) 
			q=0.15;
		zc=offz*(f00+f01+f10+f11)/q;
#endif
		xcn=xc*(zc+offz);
		ycn=yc*(zc+offz);
#ifdef ENABLE_ROLL	
		xc=xcn*t->im00+ycn*t->im01+zc*t->im02;
		yc=xcn*t->im10+ycn*t->im11+zc*t->im12;
#else
		xc=xcn*t->im00+ycn*t->im01;
		yc=xcn*t->im10+ycn*t->im11;
#endif

	} else {
		xcn=xc;
		ycn=yc;
		xc=(xcn*t->im00+ycn*t->im01)*(1 << POST_SHIFT);
		yc=(xcn*t->im10+ycn*t->im11)*(1 << POST_SHIFT);
	}
	c->x=xc*(1 << t->scale_shift)+t->map_center.x;
	c->y=yc*(1 << t->scale_shift)+t->map_center.y;
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

#ifdef ENABLE_ROLL
void
transform_set_roll(struct transformation *this_,int roll)
{
	this_->roll=roll;
	transform_setup_matrix(this_);
}

int
transform_get_roll(struct transformation *this_)
{
	return this_->roll;
}

#else

void
transform_set_roll(struct transformation *this_,int roll)
{
	dbg(0,"not supported\n");
}

int
transform_get_roll(struct transformation *this_)
{
	return 0;
}

#endif

void
transform_set_distance(struct transformation *this_,int distance)
{
	this_->screen_dist=distance;
	transform_setup_matrix(this_);
}

int
transform_get_distance(struct transformation *this_)
{
	return this_->screen_dist;
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
		screen_pnt[0].x=pr->lu.x;
		screen_pnt[0].y=pr->lu.y;
		screen_pnt[1].x=pr->rl.x;
		screen_pnt[1].y=pr->lu.y;
		screen_pnt[2].x=pr->lu.x;
		screen_pnt[2].y=pr->rl.y;
		screen_pnt[3].x=pr->rl.x;
		screen_pnt[3].y=pr->rl.y;
		for (i = 0 ; i < 4 ; i++) {
			transform_reverse(t, &screen_pnt[i], &screen[i]);
			dbg(1,"map(%d) %d,%d=0x%x,0x%x\n", i,screen_pnt[i].x, screen_pnt[i].y, screen[i].x, screen[i].y);
		}
		msm->u.c_rect.lu.x=min4(screen[0].x,screen[1].x,screen[2].x,screen[3].x);
		msm->u.c_rect.rl.x=max4(screen[0].x,screen[1].x,screen[2].x,screen[3].x);
		msm->u.c_rect.rl.y=min4(screen[0].y,screen[1].y,screen[2].y,screen[3].y);
		msm->u.c_rect.lu.y=max4(screen[0].y,screen[1].y,screen[2].y,screen[3].y);
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
	int dx,dy,f,scale=15539;
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


