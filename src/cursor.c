
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <glib.h>
#include "coord.h"
#include "transform.h"
#include "graphics.h"
#include "statusbar.h"
#include "menu.h"
#include "vehicle.h"
#include "container.h"
#include "cursor.h"
#include "compass.h"


#include "route.h"

struct cursor {
	struct container *co;
	struct graphics_gc *cursor_gc;
	struct point cursor_pnt;
};

struct coord *
cursor_pos_get(struct cursor *this)
{
	return vehicle_pos_get(this->co->vehicle);
}

static void
cursor_draw(struct cursor *this, struct point *pnt, double *speed, double *dir)
{
	int x=this->cursor_pnt.x;
	int y=this->cursor_pnt.y;
	int r=12,lw=2;
	double dx,dy;
	double fac1,fac2;
	int dir_i=*dir-this->co->trans->angle;
	struct point cpnt[3];
	struct graphics *gra=this->co->gra;

	if (pnt && x == pnt->x && y == pnt->y)
		return;
	cpnt[0]=this->cursor_pnt;
	cpnt[0].x-=r+lw;
	cpnt[0].y-=r+lw;
	gra->draw_restore(gra, &cpnt[0], (r+lw)*2, (r+lw)*2);
	if (pnt) {
		gra->draw_mode(gra, draw_mode_cursor);
		this->cursor_pnt=*pnt;
		x=pnt->x;
		y=pnt->y;
		cpnt[0].x=x;
		cpnt[0].y=y;
		gra->draw_circle(gra, this->cursor_gc, &cpnt[0], r*2);
		if (*speed > 2.5) {
			dx=sin(M_PI*dir_i/180);
			dy=-cos(M_PI*dir_i/180);

			fac1=0.7*r;
			fac2=0.4*r;	
			cpnt[0].x=x-dx*fac1+dy*fac2;
			cpnt[0].y=y-dy*fac1-dx*fac2;
			cpnt[1].x=x+dx*r;
			cpnt[1].y=y+dy*r;
			cpnt[2].x=x-dx*fac1-dy*fac2;
			cpnt[2].y=y-dy*fac1+dx*fac2;
			gra->draw_lines(gra, this->cursor_gc, cpnt, 3);
		} else {
			cpnt[1]=cpnt[0];
			gra->draw_lines(gra, this->cursor_gc, cpnt, 2);
		}
		gra->draw_mode(gra, draw_mode_end);
	}
}

static void
cursor_map_reposition_screen(struct cursor *this, struct coord *c, double *dir, int x_new, int y_new)
{
	struct coord c_new;
	struct transformation tr;
	struct point pnt;
	unsigned long scale;
	long x,y;
	int dir_i;
	struct container *co=this->co;

	if (dir)
		dir_i=*dir;
	else
		dir_i=0;

	pnt.x=co->trans->width-x_new;
	pnt.y=co->trans->height-y_new;
	graphics_get_view(co, &x, &y, &scale);
	tr=*this->co->trans;
	transform_setup(&tr, c->x, c->y, scale, dir_i);
	transform_reverse(&tr, &pnt, &c_new);
	printf("%lx %lx vs %lx %lx\n", c->x, c->y, c_new.x, c_new.y);
	x=c_new.x;
	y=c_new.y;
	transform_set_angle(co->trans,dir_i);
	graphics_set_view(co, &x, &y, &scale);
}

static void
cursor_map_reposition(struct cursor *this, struct coord *c, double *dir)
{
	if (this->co->flags->orient_north) {
		graphics_set_view(this->co, &c->x, &c->y, NULL);
	} else {
		cursor_map_reposition_screen(this, c, dir, this->co->trans->width/2, this->co->trans->height*0.8);
	}
}

static int
cursor_map_reposition_boundary(struct cursor *this, struct coord *c, double *dir, struct point *pnt)
{
	struct point pnt_new;
	struct transformation *t=this->co->trans;

	pnt_new.x=-1;
	pnt_new.y=-1;
	if (pnt->x < 0.1*t->width) {
		pnt_new.x=0.8*t->width;
		pnt_new.y=t->height/2;
	}
	if (pnt->x > 0.9*t->width) {
		pnt_new.x=0.2*t->width;
		pnt_new.y=t->height/2;
	}
	if (pnt->y < (this->co->flags->orient_north ? 0.1 : 0.5)*t->height) {
		pnt_new.x=t->width/2;
		pnt_new.y=0.8*t->height;
	}
	if (pnt->y > 0.9*t->height) {
		pnt_new.x=t->width/2;
		pnt_new.y=0.2*t->height;
	}
	if (pnt_new.x != -1) {
		if (this->co->flags->orient_north) {
			cursor_map_reposition_screen(this, c, NULL, pnt_new.x, pnt_new.y);
		} else {
			cursor_map_reposition(this, c, dir);
		}
		return 1;
	}
	return 0;
}

static void
cursor_update(void *t)
{
	struct cursor *this=t;
	struct point pnt;
	struct coord *pos;
	struct vehicle *v=this->co->vehicle;
	double *dir;

	if (v) {
		pos=vehicle_pos_get(v);	
		dir=vehicle_dir_get(v);
		route_set_position(this->co->route, cursor_pos_get(this->co->cursor));
		if (!transform(this->co->trans, pos, &pnt)) {
			cursor_map_reposition(this, pos, dir);
			transform(this->co->trans, pos, &pnt);
		}
		if (pnt.x < 0 || pnt.y < 0 || pnt.x >= this->co->trans->width || pnt.y >= this->co->trans->height) {
			cursor_map_reposition(this, pos, dir);
			transform(this->co->trans, pos, &pnt);
		}
		if (cursor_map_reposition_boundary(this, pos, dir, &pnt))
			transform(this->co->trans, pos, &pnt);
		cursor_draw(this, &pnt, vehicle_speed_get(v), vehicle_dir_get(v));
	}
	compass_draw(this->co->compass, this->co);
}

extern void *vehicle;

struct cursor *
cursor_new(struct container *co, struct vehicle *v)
{
	struct cursor *this=g_new(struct cursor,1);
	this->co=co;
	this->cursor_gc=co->gra->gc_new(co->gra);
	co->gra->gc_set_foreground(this->cursor_gc, 0x0000, 0x0000, 0xffff);
	co->gra->gc_set_linewidth(this->cursor_gc, 2);
	vehicle_callback(v, cursor_update, this);
	return this;
}
