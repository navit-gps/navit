#if 0
//#include <math.h>
#include <stdio.h>
#include <glib.h>
#include "point.h"
//#include "coord.h"
#include "graphics.h"
//#include "transform.h"
//#include "route.h"
#include "vehicle.h"
#include "container.h"
#include "osd.h"

void
osd_set_next_command(char *new_command,char *new_road){
	extern struct container *co;
	// struct osd *this=co->osd;
	strcpy(co->osd->command,new_command);
	strcpy(co->osd->road_name,new_road);
	osd_draw(co->osd, co);
}

void
osd_draw(struct osd *osd, struct container *co)
{
	double *speed;
	struct point p;

	if (! co->vehicle)
		return;

	speed=vehicle_speed_get(co->vehicle);
	osd->gr->draw_mode(osd->gr, draw_mode_begin);
	p.x=0;
	p.y=0;
	osd->gr->draw_rectangle(osd->gr, osd->bg, &p, 360, 80);
	p.x=10;
	p.y=20;
//	osd->gr->draw_text(osd->gr, osd->green, NULL, osd->font, osd->command, &p, 0x13000, 0);
	char osd_data[256];
	sprintf(osd_data,"%.1f km/h",(*speed));
	osd->gr->draw_text(osd->gr, osd->green, NULL, osd->font, osd_data, &p, 0x13000, 0);
	p.x=10;
	p.y=40;
	osd->gr->draw_text(osd->gr, osd->green, NULL, osd->font, osd->command, &p, 0x13000, 0);
//	osd->gr->draw_text(osd->gr, osd->green, NULL, osd->font, "Overlay", &p, 0x10000, 0);
//	printf("Text %s should appear\n",osd->command);
	osd->gr->draw_mode(osd->gr, draw_mode_end);
}


struct osd *
osd_new(struct container *co)
{	
	printf("Spawning an OSD\n");
	struct osd *this=g_new0(struct osd, 1);
	struct point p;
	p.x=100;
	p.y=10;
	this->gr=co->gra->overlay_new(co->gra, &p, 360, 80);
	this->bg=this->gr->gc_new(this->gr);
	this->gr->gc_set_foreground(this->bg, 0, 0, 0);
	this->white=this->gr->gc_new(this->gr);
	this->gr->gc_set_foreground(this->white, 0xffff, 0xffff, 0xffff);
	this->gr->gc_set_linewidth(this->white, 10);
	this->green=this->gr->gc_new(this->gr);
	this->gr->gc_set_foreground(this->green, 0x0, 0xffff, 0x0);
	this->gr->gc_set_linewidth(this->green, 10);
	
	this->font=this->gr->font_new(this->gr, 200);
	osd_draw(this, co);
	return this;	
}
#endif

#include <glib.h>
#include "debug.h"
#include "plugin.h"
#include "osd.h"


struct osd {
	struct osd_methods meth;
	struct osd_priv *priv;
};

struct osd *
osd_new(struct navit *nav, const char *type, struct attr **attrs)
{
        struct osd *o;
        struct osd_priv *(*new)(struct navit *nav, struct osd_methods *meth, struct attr **attrs);

        new=plugin_get_osd_type(type);
        if (! new)
                return NULL;
        o=g_new0(struct osd, 1);
        o->priv=new(nav, &o->meth, attrs);
        return o;
}

