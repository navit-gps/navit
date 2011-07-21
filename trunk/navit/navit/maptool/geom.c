/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2011 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */
#include <string.h>
#include <math.h>
#include "maptool.h"

void
geom_coord_copy(struct coord *from, struct coord *to, int count, int reverse)
{
	int i;
	if (!reverse) {
		memcpy(to, from, count*sizeof(struct coord));
		return;
	}
	from+=count;
	for (i = 0 ; i < count ; i++) 
		*to++=*--from;	
}

/**
  * Get coordinates of polyline middle point.
  * @param in *p array of poly vertex coordinates
  * @param in count count of poly vertexes
  * @param out *c coordinates of middle point
  * @returns number of first vertex of segment containing middle point
  */
int
geom_line_middle(struct coord *p, int count, struct coord *c)
{
	double length=0,half=0,len=0;
	int i;

	if(count==1) {
		*c=*p;
		return 0;
	}
	
	for (i=0; i<count-1; i++) {
		length+=sqrt(sq(p[i].x-p[i+1].x)+sq(p[i].y-p[i+1].y));
	}

	length/=2;
	for (i=0; (i<count-1) && (half<length); i++) {
		len=sqrt(sq(p[i].x-p[i+1].x)+sq(p[i].y-p[i+1].y));
		half+=len;
	}
	if (i > 0) {
		i--;
		half-=length;
		if (len) {
			c->x=(p[i].x*half+p[i+1].x*(len-half))/len;
			c->y=(p[i].y*half+p[i+1].y*(len-half))/len;
		} else
			*c=p[i];
	} else
		*c=p[0];
	return i;
}


void
geom_coord_revert(struct coord *c, int count)
{
	struct coord tmp;
	int i;

	for (i = 0 ; i < count/2 ; i++) {
		tmp=c[i];
		c[i]=c[count-1-i];
		c[count-1-i]=tmp;
	}
}


long long
geom_poly_area(struct coord *c, int count)
{
	long long area=0;
	int i,j=0;
#if 0
	fprintf(stderr,"count=%d\n",count);
#endif
	for (i=0; i<count; i++) {
		if (++j == count)
			j=0;
#if 0
		fprintf(stderr,"(%d+%d)*(%d-%d)=%d*%d="LONGLONG_FMT"\n",c[i].x,c[j].x,c[i].y,c[j].y,c[i].x+c[j].x,c[i].y-c[j].y,(long long)(c[i].x+c[j].x)*(c[i].y-c[j].y));
#endif
		area+=(long long)(c[i].x+c[j].x)*(c[i].y-c[j].y);
#if 0
		fprintf(stderr,"area="LONGLONG_FMT"\n",area);
#endif
	}
  	return area/2;
}

/**
  * Get poly centroid coordinates.
  * @param in *p array of poly vertex coordinates
  * @param in count count of poly vertexes
  * @param out *c coordinates of poly centroid
  * @returns 1 on success, 0 if poly area is 0
  */
int
geom_poly_centroid(struct coord *p, int count, struct coord *c)
{
	long long area=0;
	long long sx=0,sy=0,tmp;
	int i,j;
	long long x0=p[0].x, y0=p[0].y, xi, yi, xj, yj;
	
	/*fprintf(stderr,"area="LONGLONG_FMT"\n", area );*/
	for (i=0,j=0; i<count; i++) {
		if (++j == count)
			j=0;
		xi=p[i].x-x0;
		yi=p[i].y-y0;
		xj=p[j].x-x0;
		yj=p[j].y-y0;
		tmp=(xi*yj-xj*yi);
		sx+=(xi+xj)*tmp;
		sy+=(yi+yj)*tmp;
		area+=xi*yj-xj*yi;
	}
	if(area!=0) {
		c->x=x0+sx/3/area;
		c->y=y0+sy/3/area;
		return 1;
	}
	return 0;
}

/**
  * Get coordinates of polyline point c most close to given point p.
  * @param in *pl array of polyline vertex coordinates
  * @param in count count of polyline vertexes
  * @param in *p point coordinates 
  * @param out *c coordinates of polyline point most close to given point.
  * @returns first vertex number of polyline segment to which c belongs
  */
int
geom_poly_closest_point(struct coord *pl, int count, struct coord *p, struct coord *c)
{
	int i,vertex=0;
	long long x, y, xi, xj, yi, yj, u, d, dmin=0;
	if(count<2) {
		c->x=pl->x;
		c->y=pl->y;
		return 0;
	}
	for(i=0;i<count-1;i++) {
		xi=pl[i].x;
		yi=pl[i].y;
		xj=pl[i+1].x;
		yj=pl[i+1].y;
		u=(xj-xi)*(xj-xi)+(yj-yi)*(yj-yi);
		if(u!=0) {
			u=((p->x-xi)*(xj-xi)+(p->y-yi)*(yj-yi))*1000/u;
		}
		if(u<0) 
			u=0;
		else if (u>1000) 
			u=1000;
		x=xi+u*(xj-xi)/1000;
		y=yi+u*(yj-yi)/1000;
		d=(p->x-x)*(p->x-x)+(p->y-y)*(p->y-y);
		if(i==0 || d<dmin) {
			dmin=d;
			c->x=x;
			c->y=y;
			vertex=i;
		}
	}
	return vertex;
}

/**
  * Check if point is inside polgone.
  * @param in *cp array of polygon vertex coordinates
  * @param in count count of polygon vertexes
  * @param in *c point coordinates 
  * @returns 1 - inside, 0 - outside
  */
int
geom_poly_point_inside(struct coord *cp, int count, struct coord *c)
{
	int ret=0;
	struct coord *last=cp+count-1;
	while (cp < last) {
		if ((cp[0].y > c->y) != (cp[1].y > c->y) &&
			c->x < ((long long)cp[1].x-cp[0].x)*(c->y-cp[0].y)/(cp[1].y-cp[0].y)+cp[0].x) {
#if 0
			fprintf(stderr," cross 0x%x,0x%x-0x%x,0x%x %dx%d",cp,cp[0].x,cp[0].y,cp[1].x,cp[1].y,cp[1].x-cp[0].x,c->y-cp[0].y);
			printf("type=selected_line\n");
			coord_print(projection_mg, &cp[0], stdout);
			coord_print(projection_mg, &cp[1], stdout);
#endif
			ret=!ret;
		}
		cp++;
	}
	return ret;
}



GList *
geom_poly_segments_insert(GList *list, struct geom_poly_segment *first, struct geom_poly_segment *second, struct geom_poly_segment *third)
{
	int count;
	struct geom_poly_segment *ret;
	struct coord *pos;

	if (!second)
		return NULL;
	ret=g_new(struct geom_poly_segment, 1);
	ret->type=second->type;
	count=(second->last-second->first)+1;
	if (first) 
		count+=(first->last-first->first);
	if (third)
		count+=(third->last-third->first);
#if 0
	fprintf(stderr,"count=%d first=%p second=%p third=%p\n",count,first,second,third);	
	if (first) 
		fprintf(stderr,"first:0x%x,0x%x-0x%x,0x%x (%d)\n",first->first->x,first->first->y,first->last->x,first->last->y, first->last-first->first+1);
	if (second) 
		fprintf(stderr,"second:0x%x,0x%x-0x%x,0x%x (%d)\n",second->first->x,second->first->y,second->last->x,second->last->y, second->last-second->first+1);
	if (third) 
		fprintf(stderr,"third:0x%x,0x%x-0x%x,0x%x (%d)\n",third->first->x,third->first->y,third->last->x,third->last->y, third->last-third->first+1);
#endif
	ret->first=g_new(struct coord, count);
	pos=ret->first;
	if (first) {
		count=(first->last-first->first)+1;
		geom_coord_copy(first->first, pos, count, coord_is_equal(*first->first, *second->first));
		pos+=count-1;
	}
	count=(second->last-second->first)+1;
	geom_coord_copy(second->first, pos, count, 0);
	pos+=count;
	if (third) {
		pos--;
		count=(third->last-third->first)+1;
		geom_coord_copy(third->first, pos, count, coord_is_equal(*third->last, *second->last));
		pos+=count;
	}
	ret->last=pos-1;	
#if 0
	fprintf(stderr,"result:0x%x,0x%x-0x%x,0x%x (%d)\n",ret->first->x,ret->first->y,ret->last->x,ret->last->y, ret->last-ret->first+1);
#endif
	list=g_list_prepend(list, ret);
#if 0
	fprintf(stderr,"List=%p\n",list);
#endif
	return list;
}

void
geom_poly_segment_destroy(struct geom_poly_segment *seg)
{
	g_free(seg->first);
	g_free(seg);
}

GList *
geom_poly_segments_remove(GList *list, struct geom_poly_segment *seg)
{
	if (seg) {
		list=g_list_remove(list, seg);
		geom_poly_segment_destroy(seg);
	}
	return list;
}

int
geom_poly_segment_compatible(struct geom_poly_segment *s1, struct geom_poly_segment *s2, int dir)
{
	int same=0,opposite=0;
	if (s1->type == geom_poly_segment_type_none || s2->type == geom_poly_segment_type_none)
		return 0;
	if (s1->type == s2->type) {
		same=1;
		if (s1->type == geom_poly_segment_type_way_inner || s1->type == geom_poly_segment_type_way_outer) {
			opposite=1;
		}
	}
	if (s1->type == geom_poly_segment_type_way_left_side && s2->type == geom_poly_segment_type_way_right_side)
		opposite=1;
	if (s1->type == geom_poly_segment_type_way_right_side && s2->type == geom_poly_segment_type_way_left_side)
		opposite=1;
	if (s1->type == geom_poly_segment_type_way_unknown || s2->type == geom_poly_segment_type_way_unknown) {
		same=1;
		opposite=1;
	}
	if (dir < 0) {
		if ((opposite && coord_is_equal(*s1->first, *s2->first)) || (same && coord_is_equal(*s1->first, *s2->last))) 
			return 1;
	} else {
		if ((opposite && coord_is_equal(*s1->last, *s2->last)) || (same && coord_is_equal(*s1->last, *s2->first))) 
			return 1;
	}
	return 0;
}


GList *
geom_poly_segments_sort(GList *in, enum geom_poly_segment_type type)
{
	GList *ret=NULL;
	while (in) {
		struct geom_poly_segment *seg=in->data;
		GList *tmp=ret;
		struct geom_poly_segment *merge_first=NULL,*merge_last=NULL;
		while (tmp) {
			struct geom_poly_segment *cseg=tmp->data;	
			if (geom_poly_segment_compatible(seg, cseg, -1))
				merge_first=cseg;
			if (geom_poly_segment_compatible(seg, cseg, 1))
				merge_last=cseg;
			tmp=g_list_next(tmp);
		}
		if (merge_first == merge_last)
			merge_last=NULL;
		ret=geom_poly_segments_insert(ret, merge_first, seg, merge_last);
		ret=geom_poly_segments_remove(ret, merge_first);
		ret=geom_poly_segments_remove(ret, merge_last);
		in=g_list_next(in);
	}
	in=ret;
	while (in) {
		struct geom_poly_segment *seg=in->data;
		if (coord_is_equal(*seg->first, *seg->last)) {
			long long area=geom_poly_area(seg->first,seg->last-seg->first+1);
			if (type == geom_poly_segment_type_way_right_side && seg->type == geom_poly_segment_type_way_right_side) {
				seg->type=area > 0 ? geom_poly_segment_type_way_outer : geom_poly_segment_type_way_inner;
			}
		}
		in=g_list_next(in);
	}
	return ret;
}

int
geom_poly_segments_point_inside(GList *in, struct coord *c)
{
	int open_matches=0,closed_matches=0;
	struct coord *cp;
#if 0
	fprintf(stderr,"try 0x%x,0x%x:",c->x,c->y);
#endif
	while (in) {
		struct geom_poly_segment *seg=in->data;
		cp=seg->first;
		if (geom_poly_point_inside(seg->first, seg->last-seg->first+1, c)) {
#if 0
			fprintf(stderr," inside");
#endif
			if (coord_is_equal(*seg->first,*seg->last)) 
				closed_matches++;
			else
				open_matches++;
		} else {
#if 0
			fprintf(stderr," outside");
#endif
		}
		in=g_list_next(in);
	}
#if 0
	fprintf(stderr,"\n");
	fprintf(stderr,"open_matches %d closed_matches %d\n",open_matches,closed_matches);
#endif
	if (closed_matches) {
		if (closed_matches & 1)
			return 1;
		else
			return 0;
	}
	if (open_matches) {
		if (open_matches & 1)
			return -1;
		else
			return 0;
	}
	return 0;
}

struct geom_poly_segment *
item_bin_to_poly_segment(struct item_bin *ib, int type)
{
	struct geom_poly_segment *ret=g_new(struct geom_poly_segment, 1);
	int count=ib->clen*sizeof(int)/sizeof(struct coord);
	ret->type=type;
	ret->first=g_new(struct coord, count);
	ret->last=ret->first+count-1;
	geom_coord_copy((struct coord *)(ib+1), ret->first, count, 0);
	return ret;
}


static int
clipcode(struct coord *p, struct rect *r)
{
	int code=0;
	if (p->x < r->l.x)
		code=1;
	if (p->x > r->h.x)
		code=2;
	if (p->y < r->l.y)
		code |=4;
	if (p->y > r->h.y)
		code |=8;
	return code;
}


static int
clip_line_code(struct coord *p1, struct coord *p2, struct rect *r)
{
	int code1,code2,ret=1;
	long long dx,dy;
	code1=clipcode(p1, r);
	if (code1)
		ret |= 2;
	code2=clipcode(p2, r);
	if (code2)
		ret |= 4;
	dx=p2->x-p1->x;
	dy=p2->y-p1->y;
	while (code1 || code2) {
		if (code1 & code2)
			return 0;
		if (code1 & 1) {
			p1->y+=(r->l.x-p1->x)*dy/dx;
			p1->x=r->l.x;
		} else if (code1 & 2) {
			p1->y+=(r->h.x-p1->x)*dy/dx;
			p1->x=r->h.x;
		} else if (code1 & 4) {
			p1->x+=(r->l.y-p1->y)*dx/dy;
			p1->y=r->l.y;
		} else if (code1 & 8) {
			p1->x+=(r->h.y-p1->y)*dx/dy;
			p1->y=r->h.y;
		}
		code1=clipcode(p1, r);
		if (code1 & code2)
			return 0;
		if (code2 & 1) {
			p2->y+=(r->l.x-p2->x)*dy/dx;
			p2->x=r->l.x;
		} else if (code2 & 2) {
			p2->y+=(r->h.x-p2->x)*dy/dx;
			p2->x=r->h.x;
		} else if (code2 & 4) {
			p2->x+=(r->l.y-p2->y)*dx/dy;
			p2->y=r->l.y;
		} else if (code2 & 8) {
			p2->x+=(r->h.y-p2->y)*dx/dy;
			p2->y=r->h.y;
		}
		code2=clipcode(p2, r);
	}
	if (p1->x == p2->x && p1->y == p2->y)
		ret=0;
	return ret;
}

void
clip_line(struct item_bin *ib, struct rect *r, struct tile_parameter *param, struct item_bin_sink *out)
{
	char *buffer=g_alloca(sizeof(char)*(ib->len*4+32));
	struct item_bin *ib_new=(struct item_bin *)buffer;
	struct coord *pa=(struct coord *)(ib+1);
	int count=ib->clen/2;
	struct coord p1,p2;
	int i,code;
	item_bin_init(ib_new, ib->type);
	for (i = 0 ; i < count ; i++) {
		if (i) {
			p1.x=pa[i-1].x;
			p1.y=pa[i-1].y;
			p2.x=pa[i].x;
			p2.y=pa[i].y;
			/* 0 = invisible, 1 = completely visible, 3 = start point clipped, 5 = end point clipped, 7 both points clipped */
			code=clip_line_code(&p1, &p2, r);
#if 1
			if (((code == 1 || code == 5) && ib_new->clen == 0) || (code & 2)) {
				item_bin_add_coord(ib_new, &p1, 1);
			}
			if (code) {
				item_bin_add_coord(ib_new, &p2, 1);
			}
			if (i == count-1 || (code & 4)) {
				if (ib_new->clen)
					item_bin_write_clipped(ib_new, param, out);
				item_bin_init(ib_new, ib->type);
			}
#else
			if (code) {
				item_bin_init(ib_new, ib->type);
				item_bin_add_coord(ib_new, &p1, 1);
				item_bin_add_coord(ib_new, &p2, 1);
				item_bin_write_clipped(ib_new, param, out);
			}
#endif
		}
	}
}

static int
is_inside(struct coord *p, struct rect *r, int edge)
{
	switch(edge) {
	case 0:
		return p->x >= r->l.x;
	case 1:
		return p->x <= r->h.x;
	case 2:
		return p->y >= r->l.y;
	case 3:
		return p->y <= r->h.y;
	default:
		return 0;
	}
}

static void
poly_intersection(struct coord *p1, struct coord *p2, struct rect *r, int edge, struct coord *ret)
{
	int dx=p2->x-p1->x;
	int dy=p2->y-p1->y;
	switch(edge) {
	case 0:
		ret->y=p1->y+(r->l.x-p1->x)*dy/dx;
		ret->x=r->l.x;
		break;
	case 1:
		ret->y=p1->y+(r->h.x-p1->x)*dy/dx;
		ret->x=r->h.x;
		break;
	case 2:
		ret->x=p1->x+(r->l.y-p1->y)*dx/dy;
		ret->y=r->l.y;
		break;
	case 3:
		ret->x=p1->x+(r->h.y-p1->y)*dx/dy;
		ret->y=r->h.y;
		break;
	}
}

void
clip_polygon(struct item_bin *ib, struct rect *r, struct tile_parameter *param, struct item_bin_sink *out)
{
	int count_in=ib->clen/2;
	struct coord *pin,*p,*s,pi;
	char *buffer1=g_alloca(sizeof(char)*(ib->len*4+ib->clen*7+32));
	struct item_bin *ib1=(struct item_bin *)buffer1;
	char *buffer2=g_alloca(sizeof(char)*(ib->len*4+ib->clen*7+32));
	struct item_bin *ib2=(struct item_bin *)buffer2;
	struct item_bin *ib_in,*ib_out;
	int edge,i;
	ib_out=ib1;
	ib_in=ib;
	for (edge = 0 ; edge < 4 ; edge++) {
		count_in=ib_in->clen/2;
		pin=(struct coord *)(ib_in+1);
		p=pin;
		s=pin+count_in-1;
		item_bin_init(ib_out, ib_in->type);
		for (i = 0 ; i < count_in ; i++) {
			if (is_inside(p, r, edge)) {
				if (! is_inside(s, r, edge)) {
					poly_intersection(s,p,r,edge,&pi);
					item_bin_add_coord(ib_out, &pi, 1);
				}
				item_bin_add_coord(ib_out, p, 1);
			} else {
				if (is_inside(s, r, edge)) {
					poly_intersection(p,s,r,edge,&pi);
					item_bin_add_coord(ib_out, &pi, 1);
				}
			}
			s=p;
			p++;
		}
		if (ib_in == ib1) {
			ib_in=ib2;
			ib_out=ib1;
		} else {
		       ib_in=ib1;
			ib_out=ib2;
		}
	}
	if (ib_in->clen)
		item_bin_write_clipped(ib_in, param, out);
}
