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
#include "track.h"

#endif

struct street_data {
        struct item item;
        int count;
        int limit;
        struct coord c[0];
};



struct track_line
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
	struct track_line *next;
	int angle[0];
};


struct track {
	struct mapset *ms;
#if 0
	struct transformation t;
#endif
	struct coord last_updated;
	struct track_line *lines;
#if 0
	struct track_line **last_ptr;
#endif
	struct track_line *curr_line;
	int pos;
	struct coord curr[2];
	struct coord last_in;
	struct coord last_out;
};


int angle_factor=10;
int connected_pref=-10;
int nostop_pref=10;


struct coord *
track_get_pos(struct track *tr)
{
	return &tr->last_out;
}

int
track_get_segment_pos(struct track *tr)
{
	return tr->pos;
}

struct street_data *
track_get_street_data(struct track *tr)
{
	return tr->curr_line->street;
}

static void
track_get_angles(struct track_line *tl)
{
	int i;
	struct street_data *sd=tl->street;
	for (i = 0 ; i < sd->count-1 ; i++) 
		tl->angle[i]=transform_get_angle_delta(&sd->c[i], &sd->c[i+1], 0);
}

static void
track_doupdate_lines(struct track *tr, struct coord *cc)
{
	int max_dist=1000;
	struct map_selection *sel=route_rect(18, cc, cc, 0, max_dist);
	struct mapset_handle *h;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	struct street_data *street;
	struct track_line *tl;
	struct coord c;

	dbg(0,"enter\n");
        h=mapset_open(tr->ms);
        while ((m=mapset_next(h,1))) {
		mr=map_rect_new(m, sel);
		while ((item=map_rect_get_item(mr))) {
			if (item->type >= type_street_0 && item->type <= type_ferry) {
				street=street_get_data(item);
				tl=g_malloc(sizeof(struct track_line)+(street->count-1)*sizeof(int));
				tl->street=street;
				track_get_angles(tl);
				tl->next=tr->lines;
				tr->lines=tl;
			} else 
				while (item_coord_get(item, &c, 1));
                }  
		map_rect_destroy(mr);
        }
        mapset_close(h);
	dbg(0, "exit\n");
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
track_free_lines(struct track *tr)
{
	struct track_line *tl=tr->lines,*next;
	dbg(0,"enter(tr=%p)\n", tr);

	while (tl) {
		next=tl->next;
		street_data_free(tl->street);
		g_free(tl);
		tl=next;
	}
	tr->lines=NULL;
}

static int
track_angle_abs_diff(int a1, int a2, int full)
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
track_angle_delta(int a1, int a2, int dir)
{
	if (! dir)
		return track_angle_abs_diff(a1, a2, 180);
	else
		return track_angle_abs_diff(a1, a2, 360);
}

static int
track_is_connected(struct coord *c1, struct coord *c2)
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
track_update(struct track *tr, struct coord *c, int angle)
{
	struct track_line *t;
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
		track_free_lines(tr);
		track_doupdate_lines(tr, c);
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
				value += track_angle_delta(angle, t->angle[i], 0)*angle_factor;
			if (track_is_connected(tr->curr, &sd->c[i]))
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
					track_angle_delta(angle, t->angle[i], 0)*angle_factor,
					track_is_connected(tr->curr, &sd->c[i]) ? connected_pref : 0,
					lpnt.x == tr->last_out.x && lpnt.y == tr->last_out.y ? nostop_pref : 0,
					value
				);
				tr->last_out=lpnt;
				min=value;
			}
		}
		t=t->next;
	}
	dbg(0,"tr->curr_line=%p\n", tr->curr_line);
	if (!tr->curr_line)
		return 0;
	dbg(0,"found 0x%x,0x%x\n", tr->last_out.x, tr->last_out.y);
	*c=tr->last_out;
	return 1;	
}

struct track *
track_new(struct mapset *ms)
{
	struct track *this=g_new0(struct track, 1);
	this->ms=ms;

	return this;
}

void
track_destroy(struct track *tr)
{
	track_free_lines(tr);
	g_free(tr);
}
