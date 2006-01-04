#include <math.h>
#include <stdio.h>
#include <glib.h>
#include "point.h"
#include "coord.h"
#include "graphics.h"
#include "transform.h"
#include "route.h"
#include "vehicle.h"
#include "container.h"
#include "compass.h"
#include "compass.h"

struct compass {
	struct graphics *gr;
	struct graphics_gc *bg;
	struct graphics_gc *white;
	struct graphics_gc *green;
	struct graphics_font *font;
};

static void
transform_rotate(struct point *center, int angle, struct point *p, int count)
{
	int i,x,y;
	double dx,dy;
	for (i = 0 ; i < count ; i++)
	{
		dx=sin(M_PI*angle/180.0);
        	dy=cos(M_PI*angle/180.0);
		x=dy*p->x-dx*p->y;	
		y=dx*p->x+dy*p->y;
			
		p->x=center->x+x;
		p->y=center->y+y;
		p++;
	}
}

static void
handle(struct graphics *gr, struct graphics_gc *gc, struct point *p, int r, int dir)
{
	struct point ph[3];
	int l=r*0.4;

	ph[0].x=0;
	ph[0].y=r;
	ph[1].x=0;
	ph[1].y=-r;
	transform_rotate(p, dir, ph, 2);
	gr->draw_lines(gr, gc, ph, 2);
	ph[0].x=-l;
	ph[0].y=-r+l;
	ph[1].x=0;
	ph[1].y=-r;
	ph[2].x=l;
	ph[2].y=-r+l;
	transform_rotate(p, dir, ph, 3);
	gr->draw_lines(gr, gc, ph, 3);
}

void
compass_draw(struct compass *comp, struct container *co)
{
	struct point p;
	struct coord *pos, *dest;
	double *vehicle_dir,dir,distance;
	int dx,dy;
	char buffer[16];

	if (! co->vehicle)
		return;

	vehicle_dir=vehicle_dir_get(co->vehicle);
	comp->gr->draw_mode(comp->gr, draw_mode_begin);
	p.x=0;
	p.y=0;
	comp->gr->draw_rectangle(comp->gr, comp->bg, &p, 60, 80);
	p.x=30;
	p.y=30;
	comp->gr->draw_circle(comp->gr, comp->white, &p, 50);
	if (co->flags->orient_north)
		handle(comp->gr,comp->white, &p, 20,0);
	else
		handle(comp->gr, comp->white, &p, 20, -*vehicle_dir);
	dest=route_get_destination(co->route);
	if (dest) {
		pos=vehicle_pos_get(co->vehicle);	
		dx=dest->x-pos->x;
		dy=dest->y-pos->y;
		dir=atan2(dx,dy)*180.0/M_PI;
#if 0
		printf("dx %d dy %d dir=%f vehicle_dir=%f\n", dx, dy, dir, *vehicle_dir);
#endif
		if (! co->flags->orient_north)
			dir-=*vehicle_dir;
		handle(comp->gr, comp->green, &p, 20, dir);
		p.x=8;
		p.y=72;
		distance=transform_distance(pos, dest)/1000.0;
		if (distance >= 100)
			sprintf(buffer,"%.0f km", distance);
		else if (distance >= 10)
			sprintf(buffer,"%.1f km", distance);
		else
			sprintf(buffer,"%.2f km", distance);

		comp->gr->draw_text(comp->gr, comp->green, NULL, comp->font, buffer, &p, 0x10000, 0);
	}
	comp->gr->draw_mode(comp->gr, draw_mode_end);
}

struct compass *
compass_new(struct container *co)
{
	struct compass *this=g_new0(struct compass, 1);
	struct point p;
	p.x=10;
	p.y=10;
	this->gr=co->gra->overlay_new(co->gra, &p, 60, 80);
	this->bg=this->gr->gc_new(this->gr);
	this->gr->gc_set_foreground(this->bg, 0, 0, 0);
	this->white=this->gr->gc_new(this->gr);
	this->gr->gc_set_foreground(this->white, 0xffff, 0xffff, 0xffff);
	this->gr->gc_set_linewidth(this->white, 2);
	this->green=this->gr->gc_new(this->gr);
	this->gr->gc_set_foreground(this->green, 0x0, 0xffff, 0x0);
	this->gr->gc_set_linewidth(this->green, 2);
	
	this->font=this->gr->font_new(this->gr, 200);
	compass_draw(this, co);
	return this;	
}
