#include <glib.h>
#include "track.h"
#include "debug.h"
#include "transform.h"
#include "coord.h"
#include "item.h"
#include "route.h"
#include "map.h"
#include "mapset.h"

#if 0
#include <stdio.h>
#include <glib.h>
#include "coord.h"
#include "block.h"
#include "street.h"
#include "profile.h"
#include "tracking.h"

#endif

struct street_data {
        struct item item;
        int count;
        int limit;
        struct coord c[0];
};



struct tracking_line
{
	struct street_data *street;
#if 0
	long segid;
	int linenum;
	struct coord c[2];
	struct coord lpnt;
	int value;
	int dir;
#endif
	struct tracking_line *next;
	int angle[0];
};


struct tracking {
	struct mapset *ms;
#if 0
	struct transformation t;
#endif
	struct coord last_updated;
	struct tracking_line *lines;
#if 0
	struct tracking_line **last_ptr;
#endif
	struct tracking_line *curr_line;
	int pos;
	struct coord curr[2];
	struct coord last_in;
	struct coord last_out;
};


int angle_factor=10;
int connected_pref=-10;
int nostop_pref=10;


struct coord *
tracking_get_pos(struct tracking *tr)
{
	return &tr->last_out;
}

int
tracking_get_segment_pos(struct tracking *tr)
{
	return tr->pos;
}

struct street_data *
tracking_get_street_data(struct tracking *tr)
{
	return tr->curr_line->street;
}

static void
tracking_get_angles(struct tracking_line *tl)
{
	int i;
	struct street_data *sd=tl->street;
	for (i = 0 ; i < sd->count-1 ; i++) 
		tl->angle[i]=transform_get_angle_delta(&sd->c[i], &sd->c[i+1], 0);
}

static void
tracking_doupdate_lines(struct tracking *tr, struct coord *cc)
{
	int max_dist=1000;
	struct map_selection *sel=route_rect(18, cc, cc, 0, max_dist);
	struct mapset_handle *h;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	struct street_data *street;
	struct tracking_line *tl;
	struct coord c;

	dbg(1,"enter\n");
        h=mapset_open(tr->ms);
        while ((m=mapset_next(h,1))) {
		mr=map_rect_new(m, sel);
		while ((item=map_rect_get_item(mr))) {
			if (item->type >= type_street_0 && item->type <= type_ferry) {
				street=street_get_data(item);
				tl=g_malloc(sizeof(struct tracking_line)+(street->count-1)*sizeof(int));
				tl->street=street;
				tracking_get_angles(tl);
				tl->next=tr->lines;
				tr->lines=tl;
			} else 
				while (item_coord_get(item, &c, 1));
                }  
		map_rect_destroy(mr);
        }
        mapset_close(h);
	dbg(1, "exit\n");
#if 0

	struct transformation t;

	tr->last_ptr=&tr->lines;
	transform_setup_source_rect_limit(&t,c,1000);
	transform_setup_source_rect_limit(&tr->t,c,1000);


	profile_timer(NULL);
	street_get_block(tr->ma,&t,tst_callback,tr);
	profile_timer("end");
#endif
}


static void
tracking_free_lines(struct tracking *tr)
{
	struct tracking_line *tl=tr->lines,*next;
	dbg(1,"enter(tr=%p)\n", tr);

	while (tl) {
		next=tl->next;
		street_data_free(tl->street);
		g_free(tl);
		tl=next;
	}
	tr->lines=NULL;
}

static int
tracking_angle_abs_diff(int a1, int a2, int full)
{
	int ret;

	if (a2 > a1)
		ret=(a2-a1)%full;
	else
		ret=(a1-a2)%full;
	if (ret > full/2)
		ret=full-ret;
	return ret;
}

static int
tracking_angle_delta(int a1, int a2, int dir)
{
	if (! dir)
		return tracking_angle_abs_diff(a1, a2, 180);
	else
		return tracking_angle_abs_diff(a1, a2, 360);
}

static int
tracking_is_connected(struct coord *c1, struct coord *c2)
{
	if (c1[0].x == c2[0].x && c1[0].y == c2[0].y)
		return 1;
	if (c1[0].x == c2[1].x && c1[0].y == c2[1].y)
		return 1;
	if (c1[1].x == c2[0].x && c1[1].y == c2[0].y)
		return 1;
	if (c1[1].x == c2[1].x && c1[1].y == c2[1].y)
		return 1;
	return 0;
}

int
tracking_update(struct tracking *tr, struct coord *c, int angle)
{
	struct tracking_line *t;
	int i,value,min=0;
	struct coord lpnt;
#if 0
	int min,dist;
	int debug=0;
#endif
	dbg(1,"enter(%p,%p,%d)\n", tr, c, angle);
	dbg(1,"c=0x%x,0x%x\n", c->x, c->y);

	if (c->x == tr->last_in.x && c->y == tr->last_in.y) {
		*c=tr->last_out;
		return 0;
	}
	tr->last_in=*c;
	if (!tr->lines || transform_distance_sq(&tr->last_updated, c) > 250000) {
		dbg(1, "update\n");
		tracking_free_lines(tr);
		tracking_doupdate_lines(tr, c);
		tr->last_updated=*c;
		dbg(1,"update end\n");
	}
		
	t=tr->lines;
	if (! t)
		return 0;
	tr->curr_line=NULL;
	while (t) {
		struct street_data *sd=t->street;
		for (i = 0; i < sd->count-1 ; i++) {
			value=transform_distance_line_sq(&sd->c[i], &sd->c[i+1], c, &lpnt);
			if (value < INT_MAX/2) 
				value += tracking_angle_delta(angle, t->angle[i], 0)*angle_factor;
			if (tracking_is_connected(tr->curr, &sd->c[i]))
				value += connected_pref;
			if (lpnt.x == tr->last_out.x && lpnt.y == tr->last_out.y)
				value += nostop_pref;
			if (! tr->curr_line || value < min) {
				tr->curr_line=t;
				tr->pos=i;
				tr->curr[0]=sd->c[i];
				tr->curr[1]=sd->c[i+1];
				dbg(1,"lpnt.x=0x%x,lpnt.y=0x%x %d+%d+%d+%d=%d\n", lpnt.x, lpnt.y, 
					transform_distance_line_sq(&sd->c[i], &sd->c[i+1], c, &lpnt),
					tracking_angle_delta(angle, t->angle[i], 0)*angle_factor,
					tracking_is_connected(tr->curr, &sd->c[i]) ? connected_pref : 0,
					lpnt.x == tr->last_out.x && lpnt.y == tr->last_out.y ? nostop_pref : 0,
					value
				);
				tr->last_out=lpnt;
				min=value;
			}
		}
		t=t->next;
	}
	dbg(1,"tr->curr_line=%p\n", tr->curr_line);
	if (!tr->curr_line)
		return 0;
	dbg(1,"found 0x%x,0x%x\n", tr->last_out.x, tr->last_out.y);
	*c=tr->last_out;
	return 1;	
}

struct tracking *
tracking_new(struct mapset *ms)
{
	struct tracking *this=g_new0(struct tracking, 1);
	this->ms=ms;

	return this;
}

void
tracking_set_mapset(struct tracking *this, struct mapset *ms)
{
	this->ms=ms;
}

void
tracking_destroy(struct tracking *tr)
{
	tracking_free_lines(tr);
	g_free(tr);
}
