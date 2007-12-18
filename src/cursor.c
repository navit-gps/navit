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
#include "statusbar.h"
#include "menu.h"
#include "vehicle.h"
#include "navit.h"
#include "callback.h"
#include "color.h"
#include "cursor.h"
#include "compass.h"
/* #include "track.h" */



struct cursor {
	struct graphics *gra;
	struct graphics_gc *cursor_gc;
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
		graphics_draw_circle(gra, this_->cursor_gc, &cpnt[0], r*2);
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
			graphics_draw_lines(gra, this_->cursor_gc, cpnt, 3);
		} else {
			cpnt[1]=cpnt[0];
			graphics_draw_lines(gra, this_->cursor_gc, cpnt, 2);
		}
		graphics_draw_mode(gra, draw_mode_end);
	}
}

struct cursor *
cursor_new(struct graphics *gra, struct color *c)
{
	dbg(2,"enter gra=%p c=%p\n", gra, c);
	struct cursor *this=g_new(struct cursor,1);
	this->gra=gra;
	this->cursor_gc=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->cursor_gc, c);
	graphics_gc_set_linewidth(this->cursor_gc, 2);
	dbg(2,"ret=%p\n", this);
	return this;
}

void
cursor_destroy(struct cursor *this_)
{
	graphics_gc_destroy(this_->cursor_gc);
	g_free(this_);
}
