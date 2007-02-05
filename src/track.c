#include <stdio.h>
#include <glib.h>
#include "coord.h"
#include "block.h"
#include "street.h"
#include "profile.h"
#include "track.h"


struct track_line
{
	long segid;
	int linenum;
	struct coord c[2];
	struct coord lpnt;
	int value;
	int angle;
	int dir;
	struct track_line *next;
};

struct track {
	struct map_data *ma;
	struct transformation t;
	struct coord last_updated;
	struct track_line *lines;
	struct track_line **last_ptr;
	struct coord curr[2];
	struct coord last_in;
	struct coord last_out;
};

int angle_factor=5;
int connected_pref=-10;
int nostop_pref=10;


struct track_line **last;

static void
tst_callback(struct street_str *str, void *handle, void *data)
{
	struct coord c[2];
	int visible=0,count=0;
	struct track *tr=data;
	struct track_line *lines;
	int debug_segid=0;
	int debug=0;

	/* printf("tst_callback id=0x%x ",str->segid < 0 ? -str->segid : str->segid); */
	if (street_coord_handle_get(handle, &c[0], 1)) {
		if (str->segid == debug_segid)
			printf("0x%lx,0x%lx ", c->x, c->y); 
		c[1]=c[0];
		while (street_coord_handle_get(handle, &c[0], 1)) {
			if (is_line_visible(&tr->t, c)) {
				visible=1;
			}
			c[1]=c[0];
			count++;
			if (str->segid == debug_segid)
				printf("0x%lx,0x%lx ", c->x, c->y); 
		}
	}
	if (visible) {
		lines=g_new(struct track_line, count);
		street_coord_handle_rewind(handle);
		street_coord_handle_get(handle, &c[0], 1);
		count=0;
		while (street_coord_handle_get(handle, &c[1], 1)) {
			*(tr->last_ptr)=lines;
			tr->last_ptr=&lines->next;
			lines->segid=str->segid;
			lines->linenum=count;
			lines->c[0]=c[0];
			lines->c[1]=c[1];
			lines->angle=transform_get_angle(c,0);
			lines->next=NULL;
			lines++;
			count++;
			c[0]=c[1];	
		}
		if (debug)
			printf("%d lines\n", count);
	}		
	/* printf("\n"); */
}

static void
track_doupdate_lines(struct track *tr, struct coord *c)
{

	struct transformation t;

	tr->last_ptr=&tr->lines;
	transform_setup_source_rect_limit(&t,c,1000);
	transform_setup_source_rect_limit(&tr->t,c,1000);


	profile_timer(NULL);
	street_get_block(tr->ma,&t,tst_callback,tr);
	profile_timer("end");
}

static void
track_free_lines(struct track *tr)
{
	struct track_line *tl=tr->lines,*next;
#ifdef DEBUG
	printf("track_free_lines(tr=%p)\n", tr);
#endif
	while (tl) {
		next=tl->next;
		if (! tl->linenum) {
			g_free(tl);
		}
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

void
track_update(struct track *tr, struct coord *c, int angle)
{
	struct track_line *t,*tm;
	int min,dist;
	int debug=0;

	if (c->x == tr->last_in.x && c->y == tr->last_in.y) {
		*c=tr->last_out;
		return;
	}
	if (transform_distance_sq(&tr->last_updated, c) > 250000 || !tr->lines) {
		printf("update\n");
		track_free_lines(tr);
		track_doupdate_lines(tr, c);
		tr->last_updated=*c;
	}
	profile_timer(NULL);
		
	t=tr->lines;
	g_assert(t != NULL);

	if (debug) printf("0x%lx,0x%lx (%d deg)\n", c->x, c->y, angle);
	while (t) {
		if (debug) printf("0x%lx 0x%lx,0x%lx - 0x%lx,0x%lx (%d deg) ", -t->segid, t->c[0].x, t->c[0].y, t->c[1].x, t->c[1].y, t->angle);
		t->value=transform_distance_line_sq(&t->c[0], &t->c[1], c, &t->lpnt);
		if (t->value < INT_MAX/2) 
			 t->value += track_angle_delta(angle, t->angle, 0)*angle_factor;
		if (track_is_connected(tr->curr, t->c))
			t->value += connected_pref;
		if (t->lpnt.x == tr->last_out.x && t->lpnt.y == tr->last_out.y)
			t->value += nostop_pref;
		if (debug) printf(" %d\n", t->value);
		t=t->next;
	}
	t=tr->lines;
	tm=t;
	min=t->value;
	while (t) {
		if (t->value < min) {
			min=t->value;
			tm=t;	
		}
		t=t->next;
	}
	dist=transform_distance_sq(&tm->lpnt, c);
	if (debug) printf("dist=%d id=0x%lx\n", dist, tm->segid);
	*c=tm->lpnt;
	tr->curr[0]=tm->c[0];
	tr->curr[1]=tm->c[1];
	tr->last_out=tm->lpnt;

	// printf("pos 0x%lx,0x%lx value %d dist %d angle %d vs %d (%d)\n", c->x, c->y, tm->value, dist, angle, tm->angle, track_angle_delta(angle, tm->angle, 0));
	g_assert(dist < 10000);
#if 0
	profile_timer("track_update end");
#endif
	
}

struct track *
track_new(struct map_data *ma)
{
	struct track *this=g_new0(struct track, 1);
	this->ma=ma;

	return this;
}

void
track_destroy(struct track *tr)
{
	track_free_lines(tr);
	g_free(tr);
}
