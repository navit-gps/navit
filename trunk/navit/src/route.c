#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <glib.h>
#include "coord.h"
#include "param.h"
#include "map_data.h"
#include "block.h"
#include "street.h"
#include "street_data.h"
#include "display.h"
#include "transform.h"
#include "route.h"
#include "phrase.h"
#include "navigation.h"
#include "fib-1.0/fib.h"
#include "time.h"

/*	Node1:	4 */
/*	Node2:  4 */
/*	Segid:	4 */
/*	len:	4 */
/*	type:	1 */
/*	limit/order:	1 */
/*	18 Bytes  */

extern void *speech_handle;
static int speed_list[16]={120,120,80,110,90,80,60,90,80,70,60,50,30,10,0,60};

int debug_route=0;

#define GC_TEXT_FG 7
#define GC_TEXT_BG 8
#define GC_BLACK 9  
#define GC_RED 21

int hx=0x1416bc;	/* 447C9E dx=3E18, dxmg=5E09*/
int hy=0x5f224c;	/* 5282B5 dy=5E07, dymg=8F1C*/
int segid_h=0x20461961;

int hkx=0x1474c5;	/* 44BAB6 */
int hky=0x5fb168;	/* 52E0BC */

int lx=0x141ac3;
int ly=0x5f2d7a;

int trace;
/* 0x141b53, 0x5f2065 */

struct coord3d {
	struct coord xy;
	int h;
};


struct route_point {
	struct route_point *next;
	struct route_point *hash_next;
	struct route_segment *start;
	struct route_segment *end;
	struct route_segment *seg;
#if 0
	int conn;
	int id;
#endif
	struct fibheap_el *el;	
	int value;
	struct coord3d c;
};


struct route_segment {
	struct route_segment *next;
	struct route_segment *start_next;
	struct route_segment *end_next;
	struct route_point *start;
	struct route_point *end;
	struct street_str *str;
	int limit;
	int len;
	int offset;
};

struct street_info {
	struct street_header *hdr;
	struct street_type *typ;
	struct street_str *str;
	unsigned char *p;
	int bytes;
	int include;
};

struct route_info {
	int mode;
	struct coord3d seg1,seg2,line1,line2,pos,click;
	int seg1_len,seg2_len;
	int offset;
	int dist;
	struct block_info blk_inf;
	struct street_info str_inf;
};

struct route {
	struct map_data *map_data;
	double route_time_val;
	double route_len_val;
	struct route_path_segment *path;
	struct route_path_segment *path_last;
	struct route_info *pos;
	struct route_info *dst;
	struct route_segment *route_segments;
	struct route_point *route_points;
	struct block_list *blk_lst;
#define HASH_SIZE 8192
	struct route_point *hash[HASH_SIZE];
};

struct route *
route_new(void)
{
	struct route *this=g_new0(struct route, 1);
	return this;
}

static void
route_path_free(struct route *this)
{
        struct route_path_segment *curr, *next;
        curr=this->path;

        while (curr) {
                next=curr->next;
                g_free(curr);
                curr=next;
        }
        this->path=NULL;
	this->path_last=NULL;
}

void
route_mapdata_set(struct route *this, struct map_data *mdata)
{
	this->map_data=mdata;
}

struct map_data *
route_mapdata_get(struct route *this)
{
	return this->map_data;
}

static void
route_add_path_segment(struct route *this, int segid, int offset, struct coord *start, struct coord *end, int dir, int len, int time)
{
        struct route_path_segment *segment=g_new0(struct route_path_segment,1);
        segment->next=NULL;
        segment->segid=segid;
        segment->offset=offset;
	segment->dir=dir;
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

void
route_set_position(struct route *this, struct coord *pos)
{
	struct route_info *rt;

	route_path_free(this);
	rt=route_find_nearest_street(this->map_data, pos);
	route_find_point_on_street(rt);
	if (this->pos)
		g_free(this->pos);
	this->pos=rt;
	if (this->dst) {
		route_find(this, this->pos, this->dst);
	}
}

struct route_path_segment *
route_path_get_all(struct route *this)
{
	return this->path;
}

struct route_path_segment *
route_path_get(struct route *this, int segid)
{
	struct route_path_segment *curr=this->path;

	while (curr) {
                if (curr->segid == segid)
			return curr;
		curr=curr->next;
	}
        return NULL;

}

void
route_set_destination(struct route *this, struct coord *dest)
{
	struct route_info *rt;

	rt=route_find_nearest_street(this->map_data, dest);
	route_find_point_on_street(rt);
	if (this->dst)
		g_free(this->dst);
	this->dst=rt;
	route_do_start(this, this->pos, this->dst);
}

struct coord *
route_get_destination(struct route *this)
{
	if (! this->dst)
		return NULL;
	return &this->dst->click.xy;
}

static void
route_street_foreach(struct block_info *blk_inf, unsigned char *p, unsigned char *end, void *data,
			void(*func)(struct block_info *, struct street_info *, unsigned char **, unsigned char *, void *))
{
	struct street_info str_inf;
	struct street_str *str,*str_tmp;

	if (blk_inf->block_number == 0x10c6) {
		printf("route_street_foreach p=%p\n", p);
	}
	str_inf.hdr=(struct street_header *)p;
	p+=sizeof(struct street_header);
	assert(str_inf.hdr->count == blk_inf->block->count);

	str_inf.bytes=street_get_bytes(blk_inf->block);	
	
	str_inf.typ=(struct street_type *)p;
	p+=blk_inf->block->count*sizeof(struct street_type);  

	str=(struct street_str *)p;
	str_tmp=str;
	while (str_tmp->segid)
		str_tmp++;

	p=(unsigned char *)str_tmp;	
	p+=4;
	
	while (str->segid) {
		str_inf.include=(str[1].segid > 0);
		str_inf.str=str;
		str_inf.p=p;
		func(blk_inf, &str_inf, &p, end-4, data);
		if (str_inf.include)
			str_inf.typ++;
		str++;
	}
}


static struct route_point *
route_get_point(struct route *this, struct coord3d *c)
{
	struct route_point *p=this->route_points;
	int hashval=(c->xy.x +  c->xy.y + c->h) & (HASH_SIZE-1);
	p=this->hash[hashval];
	while (p) {
		if (p->c.xy.x == c->xy.x && p->c.xy.y == c->xy.y && p->c.h == c->h) 
			return p;
		p=p->hash_next;
	}
	return NULL;
}


static struct route_point *
route_point_add(struct route *this, struct coord3d *f, int conn)
{
	int hashval;
	struct route_point *p;

	p=route_get_point(this,f);
	if (p) {
#if 0
		p->conn+=conn;
#endif
	} else {
		hashval=(f->xy.x +  f->xy.y + f->h) & (HASH_SIZE-1);
		if (debug_route)
			printf("p (0x%lx,0x%lx,0x%x)\n", f->xy.x, f->xy.y, f->h);
		p=g_new(struct route_point,1);
		p->hash_next=this->hash[hashval];
		this->hash[hashval]=p;
		p->next=this->route_points;
#if 0
		p->conn=conn;
		p->id=++id;
#endif
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
route_points_free(struct route *this)
{
	struct route_point *curr,*next;
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
route_segment_add(struct route *this, struct route_point *start, struct route_point *end, int len, struct street_str *str, int offset, int limit)
{
	struct route_segment *s;
	s=g_new(struct route_segment,1);
	s->start=start;
	s->start_next=start->start;
	start->start=s;
	s->end=end;
	s->end_next=end->end;
	end->end=s;
	s->len=len;
	s->str=str;
	s->offset=offset;
	s->limit=limit;
	s->next=this->route_segments;
	this->route_segments=s;
	if (debug_route)
		printf("l (0x%lx,0x%lx0x%x)-(0x%lx,0x%lx,0x%x)\n", start->c.xy.x, start->c.xy.y, start->c.h, end->c.xy.x, end->c.xy.y, end->c.h);
	
}

static void
route_segments_free(struct route *this)
{
	struct route_segment *curr,*next;
	curr=this->route_segments;
	while (curr) {
		next=curr->next;
		g_free(curr);
		curr=next;
	}
	this->route_segments=NULL;
}

void
route_display_points(struct route *this, struct container *co)
{
#if 0
	GtkMap *map=co->map;
	struct route_point *p=this->route_points;
	int r=5;
	struct point pnt; 
	char text[256];

	while (p) {
		if (transform(co->trans, &p->c.xy, &pnt)) {
			gdk_draw_arc(GTK_WIDGET(map)->window, map->gc[GC_BLACK], FALSE, pnt.x-r/2, pnt.y-r/2, r, r, 0, 64*360);
			if (p->value != -1) {
				sprintf(text,"%d", p->value);
#if 0
				display_text(GTK_WIDGET(map)->window, map->gc[GC_TEXT_FG], map->gc[GC_TEXT_BG], map->face[0], text, pnt.x+6, pnt.y+4, 0x10000, 0);
#endif
			}
		}
		p=p->next;
	}
#endif
}

static int
route_time(int type, int len)
{
	return len*36/speed_list[type & 0x3f];
}

static int
route_value(int type, int len)
{
	return route_time(type, len);
}

static int
route_get_height(int segid, struct coord *c)
{
	if (c->x == 0x141b53 && c->y == 0x5f2065 && (segid == 0x4fad2fa || segid == 0x4fad155)) 
		return 1;
	if (c->x == 0x1477a7 && c->y == 0x5fac38 && (segid == 0x32adac2 || segid == 0x40725c6)) 
		return 1;
	if (c->x == 0x147a4c && c->y == 0x5fb194 && (segid == 0x32adb17 || segid == 0x32adb16)) 
		return 1;
	return 0;
}

static void
route_process_street_graph(struct block_info *blk_inf, struct street_info *str_inf, unsigned char **p, unsigned char *end, void *data)
{
	struct route *this=data;
	int limit,flags=0;
	double len=0;
	struct coord3d f,o,l;
	struct route_point *s_pnt,*e_pnt;

	street_get_coord(p, str_inf->bytes, blk_inf->block->c, &f.xy);
	f.h=route_get_height(str_inf->str->segid, &f.xy);
	s_pnt=route_point_add(this,&f, 1);

	l=f;
	o=f;
	while (*p < end) {
		flags=street_get_coord(p, str_inf->bytes, blk_inf->block->c, &f.xy);
		if (flags && !str_inf->include)
			break;
		len+=transform_distance(&l.xy, &o.xy);
		l=o;
		o=f;
		if (flags)
			break;
	}
	len+=transform_distance(&l.xy, &o.xy);
	o.h=route_get_height(str_inf->str->segid, &o.xy);
	e_pnt=route_point_add(this,&o, 1);

	limit=str_inf->str->limit;
	if (str_inf->str->limit == 0x30 && (str_inf->str->type & 0x40))
		limit=0x03;
	if (str_inf->str->limit == 0x03 && (str_inf->str->type & 0x40))
		limit=0x30;

	if (str_inf->str->limit != 0x33)
		route_segment_add(this, s_pnt, e_pnt, len, str_inf->str, 0, limit);
	debug_route=0;
	*p-=2*str_inf->bytes;
}

static int
compare(void *v1, void *v2)
{
	struct route_point *p1=v1;
	struct route_point *p2=v2;
	return p1->value-p2->value;
}

static void
route_flood(struct route *this, struct route_info *rt_end)
{
	struct route_point *end;
	struct route_point *p_min;
	struct route_segment *s;
	int min,new,old,val;
	struct fibheap *heap;
 
	heap = fh_makeheap();   

	fh_setcmp(heap, compare);

	end=route_get_point(this, &rt_end->pos);
	assert(end != 0);
	end->value=0;
	end->el=fh_insert(heap, end);
	for (;;) {
		p_min=fh_extractmin(heap);
		if (! p_min)
			break;
		min=p_min->value;
		if (debug_route)
			printf("min=%d, 0x%lx, 0x%lx\n", min, p_min->c.xy.x, p_min->c.xy.y);
		s=p_min->start;
		while (s) {
			val=route_value(s->str->type, s->len);
#if 0
			val+=val*2*street_route_contained(s->str->segid);
#endif
			new=min+val;
			if (debug_route)
				printf("begin %d (0x%lx,0x%lx) ",new,s->end->c.xy.x, s->end->c.xy.y);
			if (new < s->end->value && !(s->limit & 0x30)) {
				s->end->value=new;
				s->end->seg=s;
				if (! s->end->el) {
					if (debug_route)
						printf("insert");
					s->end->el=fh_insert(heap, s->end);
				}
				else {
					if (debug_route)
						printf("replace");
					fh_replacedata(heap, s->end->el, s->end);
				}
			}
			if (debug_route)
				printf("\n");
			s=s->start_next;
		}
		s=p_min->end;
		while (s) {
			new=min+route_value(s->str->type, s->len);
			if (debug_route)
				printf("end %d vs %d (0x%lx,0x%lx) ",new,s->start->value,s->start->c.xy.x, s->start->c.xy.y);
			if (new < s->start->value && !(s->limit & 0x03)) {
				old=s->start->value;
				s->start->value=new;
				s->start->seg=s;
				if (! s->start->el) {
					if (debug_route)
						printf("insert");
					s->start->el=fh_insert(heap, s->start);
				}
				else {
					if (debug_route)
						printf("replace");
					fh_replacedata(heap, s->start->el, s->start);
				}
			}
			if (debug_route)
				printf("\n");
			s=s->end_next;
		}
	}
}

int
route_find(struct route *this, struct route_info *rt_start, struct route_info *rt_end)
{
	struct route_point *start,*start1,*start2;
	struct route_segment *s=NULL;
	double len=0,slen;
	int ret,hr,min,time=0,seg_time,dir,type;
	unsigned int val1=0xffffffff,val2=0xffffffff;

	start1=route_get_point(this, &rt_start->seg1);	
	start2=route_get_point(this, &rt_start->seg2);	
	assert(start1 != 0);
	assert(start2 != 0);
	if (start1->value != -1) 
		val1=start1->value+route_value(rt_start->str_inf.str->type, rt_start->seg1_len);
	if (start2->value != -1) 
		val2=start2->value+route_value(rt_start->str_inf.str->type, rt_start->seg2_len);

	route_add_path_segment(this, 0, 0, &rt_start->click.xy, &rt_start->pos.xy, 1, 0, 0);
	type=rt_start->str_inf.str->type;
	if (val1 < val2) {
		ret=1;
		start=start1;
		slen=transform_distance(&rt_start->pos.xy, &rt_start->line1.xy);
		route_add_path_segment(this, 0, 0, &rt_start->pos.xy, &rt_start->line1.xy, 1, slen, route_time(type, slen));
		route_add_path_segment(this, rt_start->str_inf.str->segid, rt_start->offset, NULL, NULL, -1, rt_start->seg1_len, route_time(type, rt_start->seg1_len));
	}
	else {
		ret=2;
		start=start2;
		slen=transform_distance(&rt_start->pos.xy, &rt_start->line2.xy);
		route_add_path_segment(this, 0, 0, &rt_start->pos.xy, &rt_start->line2.xy, 1, slen, route_time(type, slen));
		route_add_path_segment(this, rt_start->str_inf.str->segid, -rt_start->offset, NULL, NULL, 1, rt_start->seg2_len, route_time(type, rt_start->seg2_len));
	}

	while (start->value) {
		s=start->seg;
		if (! s) {
			printf("No Route found\n");
			break;
		}
		if (s->start == start) {
			start=s->end;
			dir=1;
		}
		else {
			start=s->start;
			dir=-1;
		}
		len+=s->len;
		seg_time=route_time(s->str->type, s->len);
		time+=seg_time;
		route_add_path_segment(this, s->str->segid, s->offset, NULL, NULL, dir, s->len, seg_time);
	}
	if (s) {
		if (s->start->c.xy.x == rt_end->seg1.xy.x && s->start->c.xy.y == rt_end->seg1.xy.y) 
			route_add_path_segment(this, 0, 0, &rt_end->pos.xy, &rt_end->line1.xy, -1, 0, 0);
		else
			route_add_path_segment(this, 0, 0, &rt_end->pos.xy, &rt_end->line2.xy, -1, 0, 0);
		route_add_path_segment(this, 0, 0, &rt_end->click.xy, &rt_end->pos.xy, -1, 0, 0);
		printf("len %5.3f\n", len/1000);
		this->route_time_val=time/10;
		time/=10;
		this->route_len_val=len;
		min=time/60;
		time-=min*60;
		hr=min/60;
		min-=hr*60;
		printf("time %02d:%02d:%02d\n", hr, min, time);
#if 1
		navigation_path_description(this);
#endif
	}
	return ret;
}

struct block_list {
	struct block_info blk_inf;
	unsigned char *p;
	unsigned char *end;
	struct block_list *next;
};

static void
route_process_street_block_graph(struct block_info *blk_inf, unsigned char *p, unsigned char *end, void *data)
{
	struct route *this=data;
	struct block_list *blk_lst=this->blk_lst;
	
	while (blk_lst) {
		if (blk_lst->blk_inf.block_number == blk_inf->block_number && blk_lst->blk_inf.file == blk_inf->file)
			return;
		blk_lst=blk_lst->next;
	}
	blk_lst=g_new(struct block_list,1);
	blk_lst->blk_inf=*blk_inf;
	blk_lst->p=p;
	blk_lst->end=end;
	blk_lst->next=this->blk_lst;
	this->blk_lst=blk_lst;
#if 0
	route_street_foreach(blk_inf, p, end, data, route_process_street_graph);
#endif
}

static void
route_blocklist_free(struct route *this)
{
	struct block_list *curr,*next;
	curr=this->blk_lst;
	while (curr) {
		next=curr->next;
		g_free(curr);
		curr=next;
	}
}

static void
route_build_graph(struct route *this, struct map_data *mdata, struct coord *c, int coord_count)
{
	struct coord rect[2];
	struct transformation t;

	int i,j,max_dist,max_coord_dist;
	int ranges[7]={0,1024000,512000,256000,128000,64000,32000};
	this->blk_lst=NULL;
	struct block_list *blk_lst_curr;

	rect[0]=c[0];	
	rect[1]=c[0];	
	for (i = 1 ; i < coord_count ; i++) {
		if (c[i].x < rect[0].x)
			rect[0].x=c[i].x;
		if (c[i].x > rect[1].x)
			rect[1].x=c[i].x;
		if (c[i].y > rect[0].y)
			rect[0].y=c[i].y;
		if (c[i].y < rect[1].y)
			rect[1].y=c[i].y;
	}
	max_coord_dist=rect[1].x-rect[0].x;
	if (max_coord_dist < rect[0].y-rect[1].y)
		max_coord_dist=rect[0].y-rect[1].y;
	max_coord_dist+=10000+max_coord_dist/2;
	
	printf("Collecting Blocks\n");
	for (i = 0 ; i < coord_count ; i++) {
		for (j = 0 ; j < 7 ; j++) {
			printf("range %d,%d\n", i, j);
			max_dist=ranges[j];
			if (max_dist == 0 || max_dist > max_coord_dist)
				max_dist=max_coord_dist;

			transform_setup_source_rect_limit(&t,&c[i],max_dist);

			map_data_foreach(mdata, file_street_str, &t, j+1, route_process_street_block_graph, this);
		}
	}
	blk_lst_curr=this->blk_lst;
	i=0;
	while (blk_lst_curr) {
		i++;
		blk_lst_curr=blk_lst_curr->next;
	}
	printf("Block Count %d\n", i);
	blk_lst_curr=this->blk_lst; 

	j=0;
	while (blk_lst_curr) {
		j++;
		printf("%d/%d\n", j, i);
		route_street_foreach(&blk_lst_curr->blk_inf, blk_lst_curr->p, blk_lst_curr->end, this, route_process_street_graph);
		blk_lst_curr=blk_lst_curr->next;
	}
}

static void
route_process_street3(struct block_info *blk_inf, struct street_info *str_inf, unsigned char **p, unsigned char *end, void *data)
{
	int flags=0;
	int i,ldist;
	struct coord3d first,f,o,l;
	struct coord3d cret;
	int match=0;
	double len=0,len_p=0;
	struct route_info *rt_inf=(struct route_info *)data;

	street_get_coord(p, str_inf->bytes, blk_inf->block->c, &f.xy);
	f.h=route_get_height(str_inf->str->segid, &f.xy);

	l=f;
	o=f;
	first=f;
	i=0;

	while (*p < end) {
		flags=street_get_coord(p, str_inf->bytes, blk_inf->block->c, &f.xy);
		f.h=route_get_height(str_inf->str->segid, &f.xy);
		if (flags && !str_inf->include)
			break;
		
		if (i++) {
			ldist=transform_distance_line_sq(&l.xy, &o.xy, &rt_inf->click.xy, &cret.xy);
			if (ldist < rt_inf->dist) {
				rt_inf->dist=ldist;
				rt_inf->seg1=first;
				rt_inf->line1=l;
				rt_inf->pos=cret;
				rt_inf->blk_inf=*blk_inf;
				rt_inf->str_inf=*str_inf;
				rt_inf->line2=o;
				rt_inf->offset=i-1;
				len_p=len;
				match=1;
			}
			if (rt_inf->mode == 1)
				len+=transform_distance(&l.xy, &o.xy);
		}
		l=o;
		o=f;
		if (flags)
			break;
        }
	ldist=transform_distance_line_sq(&l.xy, &o.xy, &rt_inf->click.xy, &cret.xy);
	if (ldist < rt_inf->dist) {
		rt_inf->dist=ldist;
		rt_inf->seg1=first;
		rt_inf->line1=l;
		rt_inf->pos=cret;
		rt_inf->blk_inf=*blk_inf;
		rt_inf->str_inf=*str_inf;
		rt_inf->line2=o;
		rt_inf->offset=i;
		len_p=len;
		match=1;
	}
	if (match) {
		rt_inf->seg2=o;
		if (rt_inf->mode == 1) {
			len+=transform_distance(&l.xy, &o.xy);
			len_p+=transform_distance(&rt_inf->pos.xy, &rt_inf->line1.xy);
			rt_inf->seg1_len=len_p;
			rt_inf->seg2_len=len-len_p;
		}
	}
	*p-=2*str_inf->bytes;
}


static void
route_process_street_block(struct block_info *blk_inf, unsigned char *p, unsigned char *end, void *data)
{
	route_street_foreach(blk_inf, p, end, data, route_process_street3);
}

struct street_str *
route_info_get_street(struct route_info *rt)
{	
	return rt->str_inf.str;
}

struct block_info *
route_info_get_block(struct route_info *rt)
{
	return &rt->blk_inf;
}

struct route_info *
route_find_nearest_street(struct map_data *mdata, struct coord *c)
{
	struct route_info *ret=g_new0(struct route_info,1);
	struct transformation t;
	int max_dist=1000;

	transform_setup_source_rect_limit(&t,c,max_dist);
	
	ret->click.xy=*c;
	ret->dist=INT_MAX;	
	ret->mode=0;

	map_data_foreach(mdata, file_street_str, &t, 48, route_process_street_block, ret);

	return ret;
}

void
route_find_point_on_street(struct route_info *rt_inf)
{
	unsigned char *p,*end;
	
	rt_inf->dist=INT_MAX;	
	rt_inf->mode=1;

	p=rt_inf->str_inf.p;
	end=(unsigned char *)rt_inf->blk_inf.block;
	end+=rt_inf->blk_inf.block->size;

	route_process_street3(&rt_inf->blk_inf, &rt_inf->str_inf, &p, end, rt_inf);
}


struct route_info *start,*end;
int count;

void
route_click(struct route *this, struct container *co, int x, int y)
{
#if 0
	GtkMap *map=co->map;
	struct point pnt;
	GdkBitmap *flag_mask;
	GdkPixmap *flag;
	struct coord c;
	struct route_info *rt_inf;
	GdkGC *gc;
	

	pnt.x=x;
	pnt.y=y;
	transform_reverse(co->trans, &pnt, &c);
	transform(co->trans, &c, &pnt);
	rt_inf=route_find_nearest_street(co->map_data, &c);


	route_find_point_on_street(rt_inf);

	flag=gdk_pixmap_create_from_xpm_d(GTK_WIDGET(map)->window, &flag_mask, NULL, flag_xpm);
	gc=gdk_gc_new(map->DrawingBuffer);      

	gdk_gc_set_clip_origin(gc,pnt.x, pnt.y-15);
	gdk_gc_set_clip_mask(gc,flag_mask);
	gdk_draw_pixmap(GTK_WIDGET(map)->window,
                        gc,
                        flag,
                        0, 0, pnt.x, pnt.y-15, 16, 16);            	
	printf("Segment ID 0x%lx\n", rt_inf->str_inf.str->segid);
#if 0
	printf("Segment Begin 0x%lx, 0x%lx, 0x%x\n", route_info.seg1.xy.x, route_info.seg1.xy.y, route_info.seg1.h);
	printf("Segment End 0x%lx, 0x%lx, 0x%x\n", route_info.seg2.xy.x, route_info.seg2.xy.y, route_info.seg2.h);
#endif

#if 0
	transform(map, &route_info.seg1.xy, &pnt); gdk_draw_arc(GTK_WIDGET(map)->window, map->gc[GC_BLACK], TRUE, pnt.x-r/2, pnt.y-r/2, r, r, 0, 64*360);
	transform(map, &route_info.line1.xy, &pnt); gdk_draw_arc(GTK_WIDGET(map)->window, map->gc[GC_BLACK], TRUE, pnt.x-r/2, pnt.y-r/2, r, r, 0, 64*360);
	transform(map, &route_info.seg2.xy, &pnt); gdk_draw_arc(GTK_WIDGET(map)->window, map->gc[GC_BLACK], TRUE, pnt.x-r/2, pnt.y-r/2, r, r, 0, 64*360);
	transform(map, &route_info.line2.xy, &pnt); gdk_draw_arc(GTK_WIDGET(map)->window, map->gc[GC_BLACK], TRUE, pnt.x-r/2, pnt.y-r/2, r, r, 0, 64*360);
	transform(map, &route_info.pos.xy, &pnt); gdk_draw_arc(GTK_WIDGET(map)->window, map->gc[GC_BLACK], TRUE, pnt.x-r/2, pnt.y-r/2, r, r, 0, 64*360);
#endif
	printf("offset=%d\n", rt_inf->offset);
	printf("seg1_len=%d\n", rt_inf->seg1_len);
	printf("seg2_len=%d\n", rt_inf->seg2_len);

	if (trace) {
		start=rt_inf;
		count=0;
		route_path_free(this);
		route_find(this, start, end);
		map_redraw(map);
	} else {
		if (! count) {
			start=rt_inf;
			count=1;
		}
		else {
			end=rt_inf;
			count=0;
		}
	}
#endif
}

void
route_start(struct route *this, struct container *co)
{
	route_do_start(this, end, start);
}

void
route_trace(struct container *co)
{
	trace=1-trace;
}

static void
route_data_free(void *t)
{
	route_blocklist_free(t);
	route_path_free(t);
	route_points_free(t);
	route_segments_free(t);
}

void
route_do_start(struct route *this, struct route_info *rt_start, struct route_info *rt_end)
{
	int res;
	struct route_point *seg1,*seg2,*pos;
	struct coord c[2];
	struct timeval tv[4];

	phrase_route_calc(speech_handle);
	route_data_free(this);
	gettimeofday(&tv[0], NULL);
	c[0]=rt_start->pos.xy;
	c[1]=rt_end->pos.xy;
	route_build_graph(this,this->map_data,c,2);
	gettimeofday(&tv[1], NULL);
	seg1=route_point_add(this, &rt_end->seg1, 1);
	pos=route_point_add(this, &rt_end->pos, 2);
	seg2=route_point_add(this ,&rt_end->seg2, 1);
	route_segment_add(this, seg1, pos, rt_end->seg1_len, rt_end->str_inf.str, rt_end->offset, 0);
	route_segment_add(this, seg2, pos, rt_end->seg2_len, rt_end->str_inf.str, -rt_end->offset, 0);

	printf("flood\n");
	route_flood(this, rt_end);
	gettimeofday(&tv[2], NULL);
	printf("find\n");
	res=route_find(this, rt_start, rt_end);
	printf("ok\n");
	gettimeofday(&tv[3], NULL);

	printf("graph time %ld\n", (tv[1].tv_sec-tv[0].tv_sec)*1000+(tv[1].tv_usec-tv[0].tv_usec)/1000);
	printf("flood time %ld\n", (tv[2].tv_sec-tv[1].tv_sec)*1000+(tv[2].tv_usec-tv[1].tv_usec)/1000);
	printf("find time %ld\n", (tv[3].tv_sec-tv[2].tv_sec)*1000+(tv[3].tv_usec-tv[2].tv_usec)/1000);
	phrase_route_calculated(speech_handle, this);
	
}


int
route_destroy(void *t)
{
	struct route *this=t;

	route_data_free(t);
	if (this->pos)
		g_free(this->pos);
	if (this->dst)
		g_free(this->dst);
	g_free(this);
	return 0;
}


struct tm *
route_get_eta(struct route *this)
{
	time_t eta;

        eta=time(NULL)+this->route_time_val;

        return localtime(&eta);
}

double
route_get_len(struct route *this)
{
	return this->route_len_val;
}

struct route_crossings *
route_crossings_get(struct route *this, struct coord *c)
{
	struct coord3d c3;
	struct route_point *pnt;
	struct route_segment *seg;
	int crossings=0;
	struct route_crossings *ret;

	c3.xy=*c;
	c3.h=0;	
	pnt=route_get_point(this, &c3);
	seg=pnt->start;
	while (seg) {
		crossings++;
		seg=seg->start_next;
	}
	seg=pnt->end;
	while (seg) {
		crossings++;
		seg=seg->end_next;
	}
	ret=g_malloc(sizeof(struct route_crossings)+crossings*sizeof(struct route_crossing));
	ret->count=crossings;
	return ret;
}
