#include <math.h>
#include <stdio.h>
#include <glib.h>
#include "config.h"
#include "item.h"
#include "point.h"
#include "coord.h"
#include "graphics.h"
#include "transform.h"
#include "route.h"
#include "navit.h"
#include "plugin.h"
#include "debug.h"
#include "callback.h"
#include "color.h"
#include "vehicle.h"

struct compass {
	struct point p;
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
	graphics_draw_lines(gr, gc, ph, 2);
	ph[0].x=-l;
	ph[0].y=-r+l;
	ph[1].x=0;
	ph[1].y=-r;
	ph[2].x=l;
	ph[2].y=-r+l;
	transform_rotate(p, dir, ph, 3);
	graphics_draw_lines(gr, gc, ph, 3);
}

static void
osd_compass_draw(struct compass *this, struct vehicle *v)
{
	struct point p;
	struct coord *pos, *dest;
	double *vehicle_dir,dir,distance;
	int dx,dy;
	char buffer[16];

	graphics_draw_mode(this->gr, draw_mode_begin);
	p.x=0;
	p.y=0;
	graphics_draw_rectangle(this->gr, this->bg, &p, 60, 80);
	p.x=30;
	p.y=30;
	graphics_draw_circle(this->gr, this->white, &p, 50);
	if (v) {
		vehicle_dir=vehicle_dir_get(v);
		handle(this->gr, this->white, &p, 20, -*vehicle_dir);
#if 0 /* FIXME */
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
#endif
	}
	graphics_draw_mode(this->gr, draw_mode_end);
}

static void
osd_compass_init(struct compass *this, struct navit *nav)
{
	struct point p;
	struct graphics *navit_gr;
	struct color c;
	navit_gr=navit_get_graphics(nav);
	this->gr=graphics_overlay_new(navit_gr, &this->p, 60, 80);

	this->bg=graphics_gc_new(this->gr);
	c.r=0; c.g=0; c.b=0;
	graphics_gc_set_foreground(this->bg, &c);

	this->white=graphics_gc_new(this->gr);
	c.r=65535; c.g=65535; c.b=65535;
	graphics_gc_set_foreground(this->white, &c);
	graphics_gc_set_linewidth(this->white, 2);

	this->green=graphics_gc_new(this->gr);
	c.r=0; c.g=65535; c.b=0;
	graphics_gc_set_foreground(this->green, &c);
	graphics_gc_set_linewidth(this->green, 2);

	osd_compass_draw(this, NULL);
}

static struct osd_priv *
osd_compass_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs)
{
	struct compass *this=g_new0(struct compass, 1);
	struct attr *attr;
	this->p.x=20;
	this->p.y=20;
	attr=attr_search(attrs, NULL, attr_x);
	if (attr)
		this->p.x=attr->u.num;
	attr=attr_search(attrs, NULL, attr_y);
	if (attr)
		this->p.y=attr->u.num;
	navit_add_init_cb(nav, callback_new_1(osd_compass_init, this));
	return (struct osd_priv *) this;
}

void
plugin_init(void)
{
	plugin_register_osd_type("compass", osd_compass_new);
}

