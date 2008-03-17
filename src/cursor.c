#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <glib.h>
#include "debug.h"
#include "coord.h"
#include "transform.h"
#include "projection.h"
#include "point.h"
#include "graphics.h"
#include "vehicle.h"
#include "navit.h"
#include "callback.h"
#include "color.h"
#include "cursor.h"
#include "compass.h"
/* #include "track.h" */



struct cursor {
	struct graphics *gra;
#define NUM_GC 5
	struct graphics_gc *cursor_gc[NUM_GC];
	struct graphics_gc *cursor_gc2[NUM_GC];
	int current_gc;
	int last_dir;
	int last_draw_dir;
	guint animate_timer;	
	struct point cursor_pnt;
};


void
cursor_draw(struct cursor *this_, struct point *pnt, int dir, int draw_dir, int force)
{
	int x=this_->cursor_pnt.x;
	int y=this_->cursor_pnt.y;
	int r=12,lw=2;
	double dx,dy;
	double fac1,fac2;
	struct point cpnt[3];
	struct graphics *gra=this_->gra;

	if (pnt && x == pnt->x && y == pnt->y && !force)
		return;
	if (!graphics_ready(gra))
		return;
	this_->last_dir = dir;
	this_->last_draw_dir = draw_dir;
	cpnt[0]=this_->cursor_pnt;
	cpnt[0].x-=r+lw;
	cpnt[0].y-=r+lw;
	graphics_draw_restore(gra, &cpnt[0], (r+lw)*2, (r+lw)*2);
	if (pnt) {
		graphics_draw_mode(gra, draw_mode_cursor);
		this_->cursor_pnt=*pnt;
		x=pnt->x;
		y=pnt->y;
		cpnt[0].x=x;
		cpnt[0].y=y;
		graphics_draw_circle(gra, this_->cursor_gc[this_->current_gc], &cpnt[0], r*2);
		if (this_->cursor_gc2[this_->current_gc])
			graphics_draw_circle(gra, this_->cursor_gc2[this_->current_gc], &cpnt[0], r*2-4);
		if (draw_dir) {
			dx=sin(M_PI*dir/180.0);
			dy=-cos(M_PI*dir/180.0);

			fac1=0.7*r;
			fac2=0.4*r;	
			cpnt[0].x=x-dx*fac1+dy*fac2;
			cpnt[0].y=y-dy*fac1-dx*fac2;
			cpnt[1].x=x+dx*r;
			cpnt[1].y=y+dy*r;
			cpnt[2].x=x-dx*fac1-dy*fac2;
			cpnt[2].y=y-dy*fac1+dx*fac2;
			graphics_draw_lines(gra, this_->cursor_gc[this_->current_gc], cpnt, 3);

			if (this_->cursor_gc2[this_->current_gc]) {
				r-=4;
				fac1=0.7*r;
				fac2=0.4*r;	
				cpnt[0].x=x-dx*fac1+dy*fac2;
				cpnt[0].y=y-dy*fac1-dx*fac2;
				cpnt[1].x=x+dx*r;
				cpnt[1].y=y+dy*r;
				cpnt[2].x=x-dx*fac1-dy*fac2;
				cpnt[2].y=y-dy*fac1+dx*fac2;
				graphics_draw_lines(gra, this_->cursor_gc2[this_->current_gc], cpnt, 3);
			}
		} else {
			cpnt[1]=cpnt[0];
			graphics_draw_lines(gra, this_->cursor_gc[this_->current_gc], cpnt, 2);
			if (this_->cursor_gc2[this_->current_gc])
				graphics_draw_circle(gra, this_->cursor_gc2[this_->current_gc], &cpnt[0], 4);
		}
		graphics_draw_mode(gra, draw_mode_end);
	}
}

static gboolean cursor_animate(struct cursor * this)
{
	this->current_gc++;
	if (this->current_gc >= NUM_GC)
		this->current_gc=0;
	struct point p;
	p.x = this->cursor_pnt.x;
	p.y = this->cursor_pnt.y;
	cursor_draw(this, &p, this->last_dir, this->last_draw_dir, 1);
	return TRUE;
}

struct cursor *
cursor_new(struct graphics *gra, struct color *c, struct color *c2, int animate)
{
	unsigned char dash_list[] = { 4, 6 };
	int i;
	dbg(2,"enter gra=%p c=%p\n", gra, c);
	struct cursor *this=g_new(struct cursor,1);
	this->gra=gra;
	this->animate_timer=0;
	for (i=0;i<NUM_GC;i++) {
		this->cursor_gc[i]=NULL;
		this->cursor_gc2[i]=NULL;
	}
	this->current_gc=0;
	for (i=0;i<NUM_GC;i++) {
		this->cursor_gc[i]=graphics_gc_new(gra);
		graphics_gc_set_foreground(this->cursor_gc[i], c);
		graphics_gc_set_linewidth(this->cursor_gc[i], 2);
		if (c2) {
			this->cursor_gc2[i]=graphics_gc_new(gra);
			graphics_gc_set_foreground(this->cursor_gc2[i], c2);
		}
		if (animate) {
			graphics_gc_set_dashes(this->cursor_gc[i], 2, (NUM_GC-i)*2, dash_list, 2);
			if(this->cursor_gc2[i])
				graphics_gc_set_dashes(this->cursor_gc2[i], 2, i*2, dash_list, 2);
		} else {
			graphics_gc_set_linewidth(this->cursor_gc[i], 2);
			if(this->cursor_gc2[i])
				graphics_gc_set_linewidth(this->cursor_gc2[i], 2);
			break; // no need to create other GCs if we are not animating
		}
	}
	if (animate)
		this->animate_timer=g_timeout_add(250, (GSourceFunc)cursor_animate, (gpointer *)this);	
	dbg(2,"ret=%p\n", this);
	return this;
}

void
cursor_destroy(struct cursor *this_)
{
	int i;
	if (this_->animate_timer)
		g_source_remove(this_->animate_timer);
	for (i=0;i<NUM_GC;i++)
		if(this_->cursor_gc[i])
			graphics_gc_destroy(this_->cursor_gc[i]);
		if(this_->cursor_gc2[i])
			graphics_gc_destroy(this_->cursor_gc2[i]);
	g_free(this_);
}
