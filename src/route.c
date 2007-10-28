#include <stdio.h>
#include <string.h>
#if 0
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#endif
#include <glib.h>
#include "config.h"
#include "debug.h"
#include "profile.h"
#include "coord.h"
#include "projection.h"
#include "map.h"
#include "mapset.h"
#include "item.h"
#include "route.h"
#include "track.h"
#include "point.h"
#include "graphics.h"
#include "transform.h"
#include "fib.h"

#if 0
static int speed_list[]={
	10, /* street_0 */
	10, /* street_1_city */
	30, /* street_2_city */
	40, /* street_3_city */
	50, /* street_4_city */
	80, /* highway_city */
	60, /* street_1_land */
	65, /* street_2_land */
	70, /* street_3_land */
	80, /* street_4_land */
	120, /* street_n_lanes */
	120, /* highway_land */
	40, /* ramp */
	30, /* ferry */
};
#endif

int debug_route=0;


struct route_graph_point {
	struct route_graph_point *next;
	struct route_graph_point *hash_next;
	struct route_graph_segment *start;
	struct route_graph_segment *end;
	struct route_graph_segment *seg;
	struct fibheap_el *el;	
	int value;
	struct coord c;
};


struct route_graph_segment {
	struct route_graph_segment *next;
	struct route_graph_segment *start_next;
	struct route_graph_segment *end_next;
	struct route_graph_point *start;
	struct route_graph_point *end;
	struct item item;
	int limit;
	int len;
};

struct route_path_segment {
	struct item item;
	int time;
	int length;
	struct coord c[2];
	struct route_path_segment *next;
};

struct route_info {
	struct coord c;
	struct coord lp;
	int pos;

	int dist;
	int dir;

	struct street_data *street;
};

struct route_path {
	struct route_path_segment *path;
	struct route_path_segment *path_last;
	struct item_hash *path_hash;
};


struct route {
	int version;
	struct mapset *ms;
	struct route_info *pos;
	struct route_info *dst;

	struct route_graph *graph;
	struct route_path *path2;
	int speedlist[route_item_last-route_item_first+1];
};

struct route_graph {
	struct route_graph_point *route_points;
	struct route_graph_segment *route_segments;
#define HASH_SIZE 8192
	struct route_graph_point *hash[HASH_SIZE];
};

static struct route_info * route_find_nearest_street(struct mapset *ms, struct coord *c);
static struct route_graph_point *route_graph_get_point(struct route_graph *this, struct coord *c);
static void route_graph_update(struct route *this);
static struct route_path *route_path_new(struct route_graph *this, struct route_info *pos, struct route_info *dst, int *speedlist);
static void route_process_street_graph(struct route_graph *this, struct item *item);
static void route_graph_destroy(struct route_graph *this);
static void route_path_update(struct route *this);

static void
route_path_destroy(struct route_path *this)
{
	struct route_path_segment *c,*n;
	if (! this)
		return;
	if (this->path_hash) {
		item_hash_destroy(this->path_hash);
		this->path_hash=NULL;
	}
	c=this->path;	
	while (c) {
		n=c->next;
		g_free(c);
		c=n;
	}
	g_free(this);
}

struct route *
route_new(struct mapset *ms)
{
	struct route *this=g_new0(struct route, 1);
	this->ms=ms;
	return this;
}

void
route_set_mapset(struct route *this, struct mapset *ms)
{
	this->ms=ms;
}

struct mapset *
route_get_mapset(struct route *this)
{
	return this->ms;
}

struct route_info *
route_get_pos(struct route *this)
{
	return this->pos;
}

struct route_info *
route_get_dst(struct route *this)
{
	return this->dst;
}

int *
route_get_speedlist(struct route *this)
{
	return this->speedlist;
}

int
route_set_speed(struct route *this, enum item_type type, int value)
{
	if (type < route_item_first || type > route_item_last) {
		dbg(0,"street type %d out of range [%d,%d]", type, route_item_first, route_item_last);
		return 0;
	}
	this->speedlist[type-route_item_first]=value;
	return 1;
}

int
route_contains(struct route *this, struct item *item)
{
	if (! this->path2 || !this->path2->path_hash || !item_hash_lookup(this->path2->path_hash, item))
		return 0;
	return 1;
}

static void
route_path_update(struct route *this)
{
	route_path_destroy(this->path2);
	if (! this->pos || ! this->dst)
		return;
	if (! this->graph || !(this->path2=route_path_new(this->graph, this->pos, this->dst, this->speedlist))) {
		profile(0,NULL);
		route_graph_update(this);
		this->path2=route_path_new(this->graph, this->pos, this->dst, this->speedlist);
		profile(1,"route_path_new");
		profile(0,"end");
	}
}

void
route_set_position(struct route *this, struct coord *pos)
{
	if (this->pos)
		route_info_free(this->pos);
	this->pos=route_find_nearest_street(this->ms, pos);
	dbg(1,"this->pos=%p\n", this->pos);
	if (! this->pos)
		return;
	if (this->dst) 
		route_path_update(this);
}

void
route_set_position_from_tracking(struct route *this, struct tracking *tracking)
{
	struct coord *c;
	struct route_info *ret;

	dbg(2,"enter\n");
	c=tracking_get_pos(tracking);
	ret=g_new0(struct route_info, 1);
	if (this->pos)
		route_info_free(this->pos);
	ret->c=*c;
	ret->lp=*c;
	ret->pos=tracking_get_segment_pos(tracking);
	ret->dist=0;
	ret->dir=0;
	ret->street=street_data_dup(tracking_get_street_data(tracking));
	dbg(3,"c->x=0x%x, c->y=0x%x pos=%d item=(0x%x,0x%x)\n", c->x, c->y, ret->pos, ret->street->item.id_hi, ret->street->item.id_lo);
	dbg(3,"street 0=(0x%x,0x%x) %d=(0x%x,0x%x)\n", ret->street->c[0].x, ret->street->c[0].y, ret->street->count-1, ret->street->c[ret->street->count-1].x, ret->street->c[ret->street->count-1].y);
	this->pos=ret;
	if (this->dst) 
		route_path_update(this);
	dbg(2,"ret\n");
}

struct map_selection *route_selection;

struct map_selection *
route_rect(int order, struct coord *c1, struct coord *c2, int rel, int abs)
{
	int dx,dy,sx=1,sy=1,d,m;
	struct map_selection *sel=g_new(struct map_selection, 1);
	sel->order[layer_town]=0;
	sel->order[layer_poly]=0;
	sel->order[layer_street]=order;
	dbg(1,"%p %p\n", c1, c2);
	dx=c1->x-c2->x;
	dy=c1->y-c2->y;
	if (dx < 0) {
		sx=-1;
		sel->rect.lu.x=c1->x;
		sel->rect.rl.x=c2->x;
	} else {
		sel->rect.lu.x=c2->x;
		sel->rect.rl.x=c1->x;
	}
	if (dy < 0) {
		sy=-1;
		sel->rect.lu.y=c2->y;
		sel->rect.rl.y=c1->y;
	} else {
		sel->rect.lu.y=c1->y;
		sel->rect.rl.y=c2->y;
	}
	if (dx*sx > dy*sy) 
		d=dx*sx;
	else
		d=dy*sy;
	m=d*rel/100+abs;	
	sel->rect.lu.x-=m;
	sel->rect.rl.x+=m;
	sel->rect.lu.y+=m;
	sel->rect.rl.y-=m;
	sel->next=NULL;
	return sel;
}

static struct map_selection *
route_calc_selection(struct coord *c1, struct coord *c2)
{
	struct map_selection *ret,*sel;
	sel=route_rect(4, c1, c2, 25, 0);
	ret=sel;
	sel->next=route_rect(8, c1, c1, 0, 40000);
	sel=sel->next;
	sel->next=route_rect(18, c1, c1, 0, 10000);
	sel=sel->next;
	sel->next=route_rect(8, c2, c2, 0, 40000);
	sel=sel->next;
	sel->next=route_rect(18, c2, c2, 0, 10000);
	/* route_selection=ret; */
	return ret;
}

static void
route_free_selection(struct map_selection *sel)
{
	struct map_selection *next;
	while (sel) {
		next=sel->next;
		g_free(sel);
		sel=next;
	}
}



void
route_set_destination(struct route *this, struct coord *dst)
{
	profile(0,NULL);
	if (this->dst)
		route_info_free(this->dst);
	this->dst=NULL;
	if (dst)
		this->dst=route_find_nearest_street(this->ms, dst);
	profile(1,"find_nearest_street");

	route_graph_destroy(this->graph);
	this->graph=NULL;
	route_path_update(this);
	profile(0,"end");
}

static struct route_graph_point *
route_graph_get_point(struct route_graph *this, struct coord *c)
{
	struct route_graph_point *p=this->route_points;
	int hashval=(c->x +  c->y) & (HASH_SIZE-1);
	p=this->hash[hashval];
	while (p) {
		if (p->c.x == c->x && p->c.y == c->y) 
			return p;
		p=p->hash_next;
	}
	return NULL;
}


static struct route_graph_point *
route_graph_add_point(struct route_graph *this, struct coord *f)
{
	int hashval;
	struct route_graph_point *p;

	p=route_graph_get_point(this,f);
	if (!p) {
		hashval=(f->x +  f->y) & (HASH_SIZE-1);
		if (debug_route)
			printf("p (0x%x,0x%x)\n", f->x, f->y);
		p=g_new(struct route_graph_point,1);
		p->hash_next=this->hash[hashval];
		this->hash[hashval]=p;
		p->next=this->route_points;
		p->el=NULL;
		p->start=NULL;
		p->end=NULL;
		p->seg=NULL;
		p->value=INT_MAX;
		p->c=*f;
		this->route_points=p;
	}
	return p;
}


static void
route_graph_free_points(struct route_graph *this)
{
	struct route_graph_point *curr,*next;
	curr=this->route_points;
	while (curr) {
		next=curr->next;
		g_free(curr);
		curr=next;
	}
	this->route_points=NULL;
	memset(this->hash, 0, sizeof(this->hash));
}

static void
route_graph_add_segment(struct route_graph *this, struct route_graph_point *start, struct route_graph_point *end, int len, struct item *item, int limit)
{
	struct route_graph_segment *s;
	s=g_new0(struct route_graph_segment,1);
	s->start=start;
	s->start_next=start->start;
	start->start=s;
	s->end=end;
	s->end_next=end->end;
	end->end=s;
	g_assert(len >= 0);
	s->len=len;
	s->item=*item;
	s->limit=limit;
	s->next=this->route_segments;
	this->route_segments=s;
	if (debug_route)
		printf("l (0x%x,0x%x)-(0x%x,0x%x)\n", start->c.x, start->c.y, end->c.x, end->c.y);
	
}

static void
route_path_add_item(struct route_path *this, struct item *itm, struct coord *start, struct coord *end, int len, int time)
{
        struct route_path_segment *segment=g_new0(struct route_path_segment,1);
	item_hash_insert(this->path_hash,  itm, (void *)1);
	segment->item=*itm;
        segment->next=NULL;
	segment->length=len;
	segment->time=time;
        if (start)
                segment->c[0]=*start;
        if (end)
                segment->c[1]=*end;
	if (!this->path)
        	this->path=segment;
	if (this->path_last)
		this->path_last->next=segment;
	this->path_last=segment;
}


struct route_path_handle {
	struct route_path_segment *s;
};

struct route_path_handle *
route_path_open(struct route *this)
{
	struct route_path_handle *ret;

	if (! this->path2 || ! this->path2->path) 
		return NULL;

	ret=g_new(struct route_path_handle, 1);

	ret->s=this->path2->path;	
	return ret;
}

struct route_path_segment *
route_path_get_segment(struct route_path_handle *h)
{
	struct route_path_segment *ret=h->s;

	if (ret)
		h->s=ret->next;

	return ret;
}

struct coord *
route_path_segment_get_start(struct route_path_segment *s)
{
	return &s->c[0];
}

struct coord *
route_path_segment_get_end(struct route_path_segment *s)
{
	return &s->c[1];
}

struct item *
route_path_segment_get_item(struct route_path_segment *s)
{
	return &s->item;
}

int
route_path_segment_get_length(struct route_path_segment *s)
{
	return s->length;
}

int
route_path_segment_get_time(struct route_path_segment *s)
{
	return s->time;
}

void
route_path_close(struct route_path_handle *h)
{
	g_free(h);
}

struct route_path_coord_handle {
	struct route *route;
	int pos;	/* -1 = Begin, 0 = Middle, 1 = End */
	int dir;
	int spos;
	struct coord last;
	struct route_info_handle *ri;
	struct route_path_handle *rp;
	struct street_data *street_data;
};

struct route_path_coord_handle *
route_path_coord_open(struct route *this)
{
	struct route_path_coord_handle *ret;

	if (! route_get_pos(this) || ! route_get_dst(this))
		return NULL;

	ret=g_new0(struct route_path_coord_handle, 1);
	ret->route=this;
	ret->ri=route_info_open(route_get_pos(this), route_get_dst(this), 0);
	if (!ret->ri) {
		ret->ri=route_info_open(route_get_pos(this), NULL, 0);
		ret->pos=-1;
	}
	else
		ret->pos=1;
	return ret;
}

struct coord *
route_path_coord_get(struct route_path_coord_handle *h)
{
	struct coord *c;
	struct route_path_segment *seg;
	struct item *item, *item2;
	struct map_rect *mr;

	switch(h->pos) {
	case -1:
		c=route_info_get(h->ri);
		if (c) {
			h->last=*c;
			return c;
		}
		h->pos=0;
		h->rp=route_path_open(h->route);
	case 0:
		if (! h->street_data) {
			seg=route_path_get_segment(h->rp);
			if (seg) {
				item=route_path_segment_get_item(seg);
				mr=map_rect_new(item->map,NULL);
				item2=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
				h->street_data=street_get_data(item2);
				map_rect_destroy(mr);
				if (h->street_data->c[0].x == h->last.x && h->street_data->c[0].y == h->last.y) {
					h->spos=1;
					h->dir=1;
				} else {
					h->spos=h->street_data->count-2;
					h->dir=-1;
				}
			}
		}
		if (h->street_data) {
			c=&h->street_data->c[h->spos];
			h->last=*c;
			h->spos+=h->dir;
			if (h->spos < 0 || h->spos >= h->street_data->count) {
				street_data_free(h->street_data);
				h->street_data=NULL;
			}
			return c;
		}
		h->pos=1;
	case 1:	
		c=route_info_get(h->ri);
		if (c) {
			h->last=*c;
			return c;
		}
		h->pos=2;
	default:
		return NULL;
	}

}

void
route_path_coord_close(struct route_path_coord_handle *h)
{
	if (h->street_data)
		street_data_free(h->street_data);	
	if (h->rp)
		route_path_close(h->rp);
	if (h->ri)
		route_info_close(h->ri);
	g_free(h);
}


static void
route_graph_free_segments(struct route_graph *this)
{
	struct route_graph_segment *curr,*next;
	curr=this->route_segments;
	while (curr) {
		next=curr->next;
		g_free(curr);
		curr=next;
	}
	this->route_segments=NULL;
}

static void
route_graph_destroy(struct route_graph *this)
{
	if (this) {
		route_graph_free_points(this);
		route_graph_free_segments(this);
		g_free(this);
	}
}

int
route_time(int *speedlist, struct item *item, int len)
{
	if (item->type < route_item_first || item->type > route_item_last) {
		dbg(0,"street type %d out of range [%d,%d]", item->type, route_item_first, route_item_last);
		return len*36;
	}
	return len*36/speedlist[item->type-route_item_first];
}


static int
route_value(int *speedlist, struct item *item, int len)
{
	int ret;
	if (len < 0) {
		printf("len=%d\n", len);
	}
	g_assert(len >= 0);
	ret=route_time(speedlist, item, len);
	dbg(1, "route_value(0x%x, %d)=%d\n", item->type, len, ret);
	return ret;
}

static void
route_process_street_graph(struct route_graph *this, struct item *item)
{
#ifdef AVOID_FLOAT
	int len=0;
#else
	double len=0;
#endif
	struct route_graph_point *s_pnt,*e_pnt;
	struct coord c,l;
	struct attr attr;


	if (item_coord_get(item, &l, 1)) {
		s_pnt=route_graph_add_point(this,&l);
		while (item_coord_get(item, &c, 1)) {
			len+=transform_distance(&l, &c);
			l=c;
		}
		e_pnt=route_graph_add_point(this,&l);
		g_assert(len >= 0);
		if (item_attr_get(item, attr_limit, &attr)) 
			route_graph_add_segment(this, s_pnt, e_pnt, len, item, attr.u.num);
		else
			route_graph_add_segment(this, s_pnt, e_pnt, len, item, 0);
	}
}

static int
compare(void *v1, void *v2)
{
	struct route_graph_point *p1=v1;
	struct route_graph_point *p2=v2;
#if 0
	if (debug_route)
		printf("compare %d (%p) vs %d (%p)\n", p1->value,p1,p2->value,p2);
#endif
	return p1->value-p2->value;
}

int
route_info_length(struct route_info *pos, struct route_info *dst, int dir)
{
	struct route_info_handle *h;
	struct coord *c,*l;
	int ret=0;

	dbg(2,"enter pos=%p dst=%p dir=%d\n", pos, dst, dir);
	h=route_info_open(pos, dst, dir);
	if (! h) {
		dbg(2,"ret=-1\n");
		return -1;
	}
	l=route_info_get(h);
	while ((c=route_info_get(h))) {
		dbg(3,"c=%p\n", c);
		ret+=transform_distance(c, l);
		l=c;
	}
	dbg(2,"ret=%d\n", ret);
	return ret;
}

static void
route_graph_flood(struct route_graph *this, struct route_info *dst, int *speedlist)
{
	struct route_graph_point *p_min,*end=NULL;
	struct route_graph_segment *s;
	int min,new,old,val;
	struct fibheap *heap;
	struct street_data *sd=dst->street;

	heap = fh_makeheap();   
	fh_setcmp(heap, compare);

	if (! (sd->limit & 2)) {
		end=route_graph_get_point(this, &sd->c[0]);
		g_assert(end != 0);
		end->value=route_value(speedlist, &sd->item, route_info_length(NULL, dst, -1));
		end->el=fh_insert(heap, end);
	}

	if (! (sd->limit & 1)) {
		end=route_graph_get_point(this, &sd->c[sd->count-1]);
		g_assert(end != 0);
		end->value=route_value(speedlist, &sd->item, route_info_length(NULL, dst, 1));
		end->el=fh_insert(heap, end);
	}

	dbg(1,"0x%x,0x%x\n", end->c.x, end->c.y);
	for (;;) {
		p_min=fh_extractmin(heap);
		if (! p_min)
			break;
		min=p_min->value;
		if (debug_route)
			printf("extract p=%p free el=%p min=%d, 0x%x, 0x%x\n", p_min, p_min->el, min, p_min->c.x, p_min->c.y);
		p_min->el=NULL;
		s=p_min->start;
		while (s) {
			val=route_value(speedlist, &s->item, s->len);
#if 0
			val+=val*2*street_route_contained(s->str->segid);
#endif
			new=min+val;
			if (debug_route)
				printf("begin %d len %d vs %d (0x%x,0x%x)\n",new,val,s->end->value, s->end->c.x, s->end->c.y);
			if (new < s->end->value && !(s->limit & 1)) {
				s->end->value=new;
				s->end->seg=s;
				if (! s->end->el) {
					if (debug_route)
						printf("insert_end p=%p el=%p val=%d ", s->end, s->end->el, s->end->value);
					s->end->el=fh_insert(heap, s->end);
					if (debug_route)
						printf("el new=%p\n", s->end->el);
				}
				else {
					if (debug_route)
						printf("replace_end p=%p el=%p val=%d\n", s->end, s->end->el, s->end->value);
					fh_replacedata(heap, s->end->el, s->end);
				}
			}
			if (debug_route)
				printf("\n");
			s=s->start_next;
		}
		s=p_min->end;
		while (s) {
			val=route_value(speedlist, &s->item, s->len);
			new=min+val;
			if (debug_route)
				printf("end %d len %d vs %d (0x%x,0x%x)\n",new,val,s->start->value,s->start->c.x, s->start->c.y);
			if (new < s->start->value && !(s->limit & 2)) {
				old=s->start->value;
				s->start->value=new;
				s->start->seg=s;
				if (! s->start->el) {
					if (debug_route)
						printf("insert_start p=%p el=%p val=%d ", s->start, s->start->el, s->start->value);
					s->start->el=fh_insert(heap, s->start);
					if (debug_route)
						printf("el new=%p\n", s->start->el);
				}
				else {
					if (debug_route)
						printf("replace_start p=%p el=%p val=%d\n", s->start, s->start->el, s->start->value);
					fh_replacedata(heap, s->start->el, s->start);
				}
			}
			if (debug_route)
				printf("\n");
			s=s->end_next;
		}
	}
	fh_deleteheap(heap);
}

static struct route_path *
route_path_new(struct route_graph *this, struct route_info *pos, struct route_info *dst, int *speedlist)
{
	struct route_graph_point *start1=NULL,*start2=NULL,*start;
	struct route_graph_segment *s=NULL;
	int len=0,segs=0;
	int ilen,seg_time,seg_len;
#if 0
	int time=0,hr,min,sec
#endif
	unsigned int val1=0xffffffff,val2=0xffffffff;
	struct street_data *sd=pos->street;
	struct route_path *ret;

	if (! (sd->limit & 1)) {
		start1=route_graph_get_point(this, &sd->c[0]);
		if (! start1)
			return NULL;
		val1=start1->value+route_value(speedlist, &sd->item, route_info_length(pos, NULL, -1));
		dbg(1,"start1: %d(route)+%d=%d\n", start1->value, val1-start1->value, val1);
	}
	if (! (sd->limit & 2)) {
		start2=route_graph_get_point(this, &sd->c[sd->count-1]);
		if (! start2)
			return NULL;
		val2=start2->value+route_value(speedlist, &sd->item, route_info_length(pos, NULL, 1));
		dbg(1,"start2: %d(route)+%d=%d\n", start2->value, val2-start2->value, val2);
	}
	dbg(1,"val1=%d val2=%d\n", val1, val2);
	if (val1 == val2) {
		val1=start1->start->start->value;
		val2=start2->end->end->value;
	}
	if (start1 && (val1 < val2)) {
		start=start1;
		pos->dir=-1;
	} else {
		if (start2) {
			start=start2;
			pos->dir=1;
		} else {
			printf("no route found, pos blocked\n");
			return NULL;
		}
	}
	ret=g_new0(struct route_path, 1);	
	ret->path_hash=item_hash_new();
	dbg(1,"dir=%d\n", pos->dir);	
	while ((s=start->seg)) {
		segs++;
#if 0
		printf("start->value=%d 0x%x,0x%x\n", start->value, start->c.x, start->c.y);
#endif
		seg_len=s->len;
		seg_time=route_time(speedlist, &s->item, seg_len);
#if 0
		time+=seg_time;
#endif
		len+=seg_len;
		if (s->start == start) {
			route_path_add_item(ret, &s->item, &s->start->c, &s->end->c, seg_len, seg_time);
			start=s->end;
		} else {
			route_path_add_item(ret, &s->item, &s->end->c, &s->start->c, seg_len, seg_time);
			start=s->start;
		}
	}
	sd=dst->street;
	dbg(1,"start->value=%d 0x%x,0x%x\n", start->value, start->c.x, start->c.y);
	dbg(1,"dst sd->limit=%d sd->c[0]=0x%x,0x%x sd->c[sd->count-1]=0x%x,0x%x\n", sd->limit, sd->c[0].x,sd->c[0].y, sd->c[sd->count-1].x, sd->c[sd->count-1].y);
	if (start->c.x == sd->c[0].x && start->c.y == sd->c[0].y)
		dst->dir=-1;
	else if (start->c.x == sd->c[sd->count-1].x && start->c.y == sd->c[sd->count-1].y)
		dst->dir=1;
	else {
		printf("no route found\n");
		route_path_destroy(ret);
		return NULL;
	}
	ilen=route_info_length(pos, NULL, 0);
#if 0
	time+=route_time(&pos->street->item, ilen);
#endif
	len+=ilen;

	ilen=route_info_length(NULL, dst, 0);
#if 0
	time+=route_time(&dst->street->item, ilen);
#endif
	len+=ilen;

	dbg(1, "%d segments\n", segs);
#if 0
	dbg(1, "len %5.3f\n", len/1000.0);
	time/=10;
	sec=time;
	min=time/60;
	time-=min*60;
	hr=min/60;
	min-=hr*60;
	dbg(1, "time %02d:%02d:%02d (%d sec)\n", hr, min, time, sec);
	dbg(1, "speed %f km/h\n", len/sec*3.6);
#endif
	return ret;
}

static struct route_graph *
route_graph_build(struct mapset *ms, struct coord *c1, struct coord *c2)
{
	struct route_graph *ret=g_new0(struct route_graph, 1);
	struct map_selection *sel;
	struct mapset_handle *h;
	struct map_rect *mr;
	struct map *m;
	struct item *item;
	struct coord e;

	sel=route_calc_selection(c1, c2);
        h=mapset_open(ms);
        while ((m=mapset_next(h,1))) {
		mr=map_rect_new(m, sel);
		while ((item=map_rect_get_item(mr))) {
			if (item->type >= type_street_0 && item->type <= type_ferry) {
				route_process_street_graph(ret, item);
			} else 
				while (item_coord_get(item, &e, 1));
		}
		map_rect_destroy(mr);
        }
        mapset_close(h);
	route_free_selection(sel);

	return ret;
}

static void
route_graph_update(struct route *this)
{
	route_graph_destroy(this->graph);
	profile(1,"graph_free");
	this->graph=route_graph_build(this->ms, &this->pos->c, &this->dst->c);
	profile(1,"route_graph_build");
	route_graph_flood(this->graph, this->dst, this->speedlist);
	profile(1,"route_graph_flood");
	this->version++;
}

struct street_data *
street_get_data (struct item *item)
{
	struct coord c[1000];
	int count=0;
	struct street_data *ret;
	struct attr attr;

	while (count < 1000) {
		if (!item_coord_get(item, &c[count], 1))
			break;
		count++;
	}
	g_assert(count < 1000);
	ret=g_malloc(sizeof(struct street_data)+count*sizeof(struct coord));
	ret->item=*item;
	ret->count=count;
	if (item_attr_get(item, attr_limit, &attr)) 
		ret->limit=attr.u.num;
	else
		ret->limit=0;
	memcpy(ret->c, c, count*sizeof(struct coord));

	return ret;
	
}

struct street_data *
street_data_dup(struct street_data *orig)
{
	struct street_data *ret;
	int size=sizeof(struct street_data)+orig->count*sizeof(struct coord);

	ret=g_malloc(size);
	memcpy(ret, orig, size);

	return ret;
}

void
street_data_free(struct street_data *sd)
{
	g_free(sd);
}

static struct route_info *
route_find_nearest_street(struct mapset *ms, struct coord *c)
{
	struct route_info *ret=NULL;
	int max_dist=1000;
	struct map_selection *sel=route_rect(18, c, c, 0, max_dist);
	int dist,pos;
	struct mapset_handle *h;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	struct coord lp, sc[1000];
	struct street_data *sd;

        h=mapset_open(ms);
        while ((m=mapset_next(h,1))) {
		mr=map_rect_new(m, sel);
               while ((item=map_rect_get_item(mr))) {
			if (item->type >= type_street_0 && item->type <= type_ferry) {
				sd=street_get_data(item);
				dist=transform_distance_polyline_sq(sd->c, sd->count, c, &lp, &pos);
				if (!ret || dist < ret->dist) {
					if (ret) {
						street_data_free(ret->street);
						g_free(ret);
					}
					ret=g_new(struct route_info, 1);
					ret->c=*c;
					ret->lp=lp;
					ret->pos=pos;
					ret->dist=dist;
					ret->dir=0;
					ret->street=sd;
					dbg(1,"dist=%d id 0x%x 0x%x pos=%d\n", dist, item->id_hi, item->id_lo, pos);
				} else 
					street_data_free(sd);		
			} else 
				while (item_coord_get(item, &sc[0], 1));
                }  
		map_rect_destroy(mr);
        }
        mapset_close(h);
	
	return ret;
}

void
route_info_free(struct route_info *inf)
{
	if (inf->street)
		street_data_free(inf->street);
	g_free(inf);
}


#include "point.h"

struct street_data *
route_info_street(struct route_info *rinf)
{
	return rinf->street;
}

struct coord *
route_info_point(struct route_info *rinf, int point)
{
	struct street_data *sd=rinf->street;
	int dir;

	switch(point) {
	case -1:
	case 2:
		dir=point == 2 ? rinf->dir : -rinf->dir;
		if (dir > 0)
			return &sd->c[sd->count-1];
		else
			return &sd->c[0];
	case 0:
		return &rinf->c;
	case 1:
		return &rinf->lp;
	}
	return NULL;

}

struct route_info_handle {
	struct route_info *start;
	struct route_info *curr;
	struct route_info *end;
	struct coord *last;
	int count;
	int iter;
	int pos;
	int endpos;
	int dir;
};

struct route_info_handle *
route_info_open(struct route_info *start, struct route_info *end, int dir)
{
	struct route_info_handle *ret=g_new0(struct route_info_handle, 1);

	struct route_info *curr;
	dbg(2,"enter start=%p end=%p dir=%d\n", start, end, dir);
	ret->start=start;
	ret->end=end;
	if (start) 
		curr=start;
	else
		curr=end;
	ret->endpos=-2;
	if (start && end) {
		if (start->street->item.map != end->street->item.map || start->street->item.id_hi != end->street->item.id_hi || start->street->item.id_lo != end->street->item.id_lo) {
			dbg(1,"return NULL\n");
			return NULL;
		}
		printf("trivial start=%d end=%d start dir %d end dir %d\n", start->pos, end->pos, start->dir, end->dir);
		if (start->pos == end->pos) {
			printf("fixme\n");
			start->dir=0;
			end->dir=0;
		}
		if (start->pos > end->pos) {
			printf("fixme\n");
			start->dir=-1;
			end->dir=1;
		}
		if (start->pos < end->pos) {
			printf("fixme\n");
			start->dir=1;
			end->dir=-1;
		}
		printf("trivial now start=%d end=%d start dir %d end dir %d\n", start->pos, end->pos, start->dir, end->dir);
		ret->endpos=end->pos;
	}

	if (!dir)
		dir=curr->dir;
	dbg(2,"dir=%d\n", dir);
	ret->dir=dir;
	ret->curr=curr;
	ret->pos=curr->pos;
	if (dir > 0) {
		ret->pos++;
		ret->endpos++;
	}
	dbg(2,"ret=%p\n",ret);
	return ret;
}

struct coord *
route_info_get(struct route_info_handle *h)
{
	struct coord *new;
	for (;;) {
		new=NULL;
		dbg(1,"iter=%d\n", h->iter);
		switch(h->iter) {
		case 0:
			if (h->start) {
				new=&h->start->c;
				h->iter++;
				break;
			} else {
				h->iter=2;
				continue;
			}
		case 1:
			new=&h->start->lp;
			h->iter++;
			break;
		case 2:
			dbg(1,"h->pos=%d\n", h->pos);
			if (h->dir && h->pos >= 0 && h->pos < h->curr->street->count && (h->end == NULL || h->endpos!=h->pos)) {
				new=&h->curr->street->c[h->pos];
				h->pos+=h->dir;
			} else {
				h->iter++;
				continue;
			}
			break;	
		case 3:
			if (h->end) {
				new=&h->end->lp;
				h->iter++;
				break;
			}
			break;
		case 4:
			new=&h->end->c;
			h->iter++;
			break;
			
		}
		if (new) {
			dbg(1,"new=%p (0x%x,0x%x) last=%p\n", new, new->x, new->y, h->last);
			if (h->last && new->x == h->last->x && new->y == h->last->y)
				continue;
			h->last=new;
			return new;	
		}
		return NULL;
	}
}

void
route_info_close(struct route_info_handle *h)
{
	g_free(h);
}




static int
route_draw_route_info(struct route_info *pos, struct route_info *dst, struct transformation *t, struct displaylist *dsp)
{
	struct route_info_handle *h;
	struct coord *c;
	struct coord_rect r;
	struct item item;
	struct point pnt[100];
	int count=0;

	item.id_lo=0;
	item.id_hi=0;
	item.map=NULL;
	item.type=type_street_route;

	dbg(1, "enter\n");
	h=route_info_open(pos, dst, 0);
	dbg(1,"h=%p\n", h);
	if (! h) {
		dbg(1, "return 0\n");
		return 0;
	}
	if (pos)
		dbg(1, "pos=%p pos->dir=%d pos->pos=%d\n", pos, pos->dir, pos->pos);
	c=route_info_get(h);
	r.lu=*c;
	r.rl=*c;
	while (c && count < 100) {
		dbg(1,"c=%p (0x%x,0x%x)\n", c, c->x, c->y);
		transform(t, projection_mg, c, &pnt[count++]);
		coord_rect_extend(&r, c);
		c=route_info_get(h);
	
	}
	if (count && transform_contains(t, projection_mg, &r))
		display_add(dsp, &item, count, pnt, "Route");
	route_info_close(h);
	dbg(1, "return 1\n");
	return 1;
}

void
route_draw(struct route *this, struct transformation *t, struct displaylist *dsp)
{
	dbg(1,"enter\n");
	if (! this->pos || ! this->dst)
		return;
	if (! route_draw_route_info(this->pos, this->dst, t, dsp)) {
		route_draw_route_info(this->pos, NULL, t, dsp);
		route_draw_route_info(NULL, this->dst, t, dsp);
	}
	dbg(1,"exit\n");
}

#if 0
struct route_crossings *
route_crossings_get(struct route *this, struct coord *c)
{
      struct route_point *pnt;
      struct route_segment *seg;
      int crossings=0;
      struct route_crossings *ret;

       pnt=route_graph_get_point(this, c);
       seg=pnt->start;
       while (seg) {
		printf("start: 0x%x 0x%x\n", seg->item.id_hi, seg->item.id_lo);
               crossings++;
               seg=seg->start_next;
       }
       seg=pnt->end;
       while (seg) {
		printf("end: 0x%x 0x%x\n", seg->item.id_hi, seg->item.id_lo);
               crossings++;
               seg=seg->end_next;
       }
       ret=g_malloc(sizeof(struct route_crossings)+crossings*sizeof(struct route_crossing));
       ret->count=crossings;
       return ret;
}
#endif
