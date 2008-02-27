#include <math.h>
#include <stdio.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
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
#include "navigation.h"
#include "map.h"

struct compass {
	struct point p;
	int w,h;
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
format_distance(char *buffer, double distance)
{
	if (distance >= 100000)
		sprintf(buffer,"%.0f km", distance/1000);
	else if (distance >= 10000)
		sprintf(buffer,"%.1f km", distance/1000);
	else
		sprintf(buffer,"%.2f km", distance/1000);
}

static void
osd_compass_draw(struct compass *this, struct navit *nav, struct vehicle *v)
{
	struct point p;
	struct attr attr_dir, destination_attr, position_attr;
	double dir,vdir=0;
	char buffer[16];
	struct coord c1, c2;
	enum projection pro;

	graphics_draw_mode(this->gr, draw_mode_begin);
	p.x=0;
	p.y=0;
	graphics_draw_rectangle(this->gr, this->bg, &p, this->w, this->h);
	p.x=30;
	p.y=30;
	graphics_draw_circle(this->gr, this->white, &p, 50);
	if (v && vehicle_position_attr_get(v, attr_position_direction, &attr_dir)) {
		vdir=*attr_dir.u.numd;
		handle(this->gr, this->white, &p, 20, -vdir);
	}
	if (navit_get_attr(nav, attr_destination, &destination_attr) && v && vehicle_position_attr_get(v, attr_position_coord_geo, &position_attr)) {
		pro=destination_attr.u.pcoord->pro;
		transform_from_geo(pro, position_attr.u.coord_geo, &c1);
		c2.x=destination_attr.u.pcoord->x;
		c2.y=destination_attr.u.pcoord->y;
		dir=atan2(c2.x-c1.x,c2.y-c1.y)*180.0/M_PI;
		dir-=vdir;
		handle(this->gr, this->green, &p, 20, dir);
		format_distance(buffer, transform_distance(pro, &c1, &c2));
		p.x=8;
		p.y=72;
		graphics_draw_text(this->gr, this->green, NULL, this->font, buffer, &p, 0x10000, 0);
	}
	graphics_draw_mode(this->gr, draw_mode_end);
}

static void
osd_compass_init(struct compass *this, struct navit *nav)
{
	struct graphics *navit_gr;
	struct color c;
	navit_gr=navit_get_graphics(nav);
	this->gr=graphics_overlay_new(navit_gr, &this->p, this->w, this->h);

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

	this->font=graphics_font_new(this->gr, 200);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_compass_draw), attr_position_coord_geo, this));

	osd_compass_draw(this, nav, NULL);
}

static struct osd_priv *
osd_compass_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs)
{
	struct compass *this=g_new0(struct compass, 1);
	struct attr *attr;
	this->p.x=20;
	this->p.y=20;
	this->w=60;
	this->h=80;
	attr=attr_search(attrs, NULL, attr_x);
	if (attr)
		this->p.x=attr->u.num;
	attr=attr_search(attrs, NULL, attr_y);
	if (attr)
		this->p.y=attr->u.num;
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_compass_init), attr_navit, this));
	return (struct osd_priv *) this;
}

struct eta {
	struct point p;
	int w,h;
	struct graphics *gr;
	struct graphics_gc *bg;
	struct graphics_gc *white;
	struct graphics_font *font;
	struct graphics_image *flag;
	int active;
	char last_eta[16];
	char last_distance[16];
};

static void
osd_eta_draw(struct eta *this, struct navit *navit, struct vehicle *v)
{
	struct point p;
	char eta[16];
	char distance[16];
	int days=0,do_draw=1;
	time_t etat;
        struct tm tm,eta_tm,eta_tm0;
	struct attr attr;
	struct navigation *nav=NULL;
	struct map *map=NULL;
	struct map_rect *mr=NULL;
	struct item *item=NULL;


	eta[0]='\0';
	distance[0]='\0';

	if (navit)
                nav=navit_get_navigation(navit);
        if (nav)
                map=navigation_get_map(nav);
        if (map)
                mr=map_rect_new(map, NULL);
        if (mr)
                item=map_rect_get_item(mr);
        if (item) {
                if (item_attr_get(item, attr_destination_length, &attr)) {
                        format_distance(distance, attr.u.num);
		}
                if (item_attr_get(item, attr_destination_time, &attr)) {
			etat=time(NULL);
			tm=*localtime(&etat);
                        etat+=attr.u.num/10;
                        eta_tm=*localtime(&etat);
			if (tm.tm_year != eta_tm.tm_year || tm.tm_mon != eta_tm.tm_mon || tm.tm_mday != eta_tm.tm_mday) {
				eta_tm0=eta_tm;
				eta_tm0.tm_sec=0;
				eta_tm0.tm_min=0;
				eta_tm0.tm_hour=0;
				tm.tm_sec=0;
				tm.tm_min=0;
				tm.tm_hour=0;
				days=(mktime(&eta_tm0)-mktime(&tm)+43200)/86400;
			}
			if (days) 
				sprintf(eta, "%d+%02d:%02d", days, eta_tm.tm_hour, eta_tm.tm_min);
			else
				sprintf(eta, "  %02d:%02d", eta_tm.tm_hour, eta_tm.tm_min);
                }
		if (this->active != 1 || strcmp(this->last_distance, distance) || strcmp(this->last_eta, eta)) {
			this->active=1;
			strcpy(this->last_distance, distance);
			strcpy(this->last_eta, eta);
			do_draw=1;
		}
        } else {
		if (this->active != 0) {
			this->active=0;
			do_draw=1;
		}
	}
        if (mr)
                map_rect_destroy(mr);

	if (do_draw) {
		graphics_draw_mode(this->gr, draw_mode_begin);
		p.x=0;
		p.y=0;
		graphics_draw_rectangle(this->gr, this->bg, &p, this->w, this->h);
		p.x=6;
		p.y=6;
		graphics_draw_image(this->gr, this->white, &p, this->flag);
		if (eta[0]) {
			p.x=28;
			p.y=28;
			graphics_draw_text(this->gr, this->white, NULL, this->font, "ETA", &p, 0x10000, 0);
			p.x=6;
			p.y=42;
			graphics_draw_text(this->gr, this->white, NULL, this->font, eta, &p, 0x10000, 0);
		}
		if (distance[0]) {
			p.x=6;
			p.y=56;
			graphics_draw_text(this->gr, this->white, NULL, this->font, distance, &p, 0x10000, 0);
		}
		graphics_draw_mode(this->gr, draw_mode_end);
	}	
}

static void
osd_eta_init(struct eta *this, struct navit *nav)
{
	struct graphics *navit_gr;
	struct color c;
	char *flag=g_strjoin(NULL,getenv("NAVIT_SHAREDIR"), "/xpm/flag_wh_bk.xpm", NULL);
	navit_gr=navit_get_graphics(nav);
	this->gr=graphics_overlay_new(navit_gr, &this->p, this->w, this->h);

	this->bg=graphics_gc_new(this->gr);
	c.r=0; c.g=0; c.b=0;
	graphics_gc_set_foreground(this->bg, &c);

	this->white=graphics_gc_new(this->gr);
	c.r=65535; c.g=65535; c.b=65535;
	graphics_gc_set_foreground(this->white, &c);
	graphics_gc_set_linewidth(this->white, 2);

	this->font=graphics_font_new(this->gr, 200);
	this->flag=graphics_image_new(this->gr, flag);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_eta_draw), attr_position_coord_geo, this));

	osd_eta_draw(this, nav, NULL);
	g_free(flag);
}

static struct osd_priv *
osd_eta_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs)
{
	struct eta *this=g_new0(struct eta, 1);
	struct attr *attr;
	this->p.x=-80;
	this->p.y=20;
	this->w=60;
	this->h=60;
	this->active=-1;
	attr=attr_search(attrs, NULL, attr_x);
	if (attr)
		this->p.x=attr->u.num;
	attr=attr_search(attrs, NULL, attr_y);
	if (attr)
		this->p.y=attr->u.num;
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_eta_init), attr_navit, this));
	return (struct osd_priv *) this;
}

void
plugin_init(void)
{
	plugin_register_osd_type("compass", osd_compass_new);
	plugin_register_osd_type("eta", osd_eta_new);
}

