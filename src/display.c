#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <glib.h>
#include "display.h"
#include "graphics.h"

struct graphics_image *icons;

static struct graphics_image *
get_icon(struct graphics *gr, char *name)
{
	struct graphics_image *curr=icons;
	while (curr) {
		if (! strcmp(curr->name, name) && curr->gr == gr)
			return curr;
		curr=curr->next;
	}
	curr=gr->image_new(gr, name);
	curr->next=icons;
	icons=curr;
	return curr;
}

static int
within_dist_point(struct point *p0, struct point *p1, int dist)
{
	if ((p0->x-p1->x)*(p0->x-p1->x) + (p0->y-p1->y)*(p0->y-p1->y) <= dist*dist) {
		return 1;
	}
	return 0;
}

static int
within_dist_line(struct point *p, struct point *line_p0, struct point *line_p1, int dist)
{
	int vx,vy,wx,wy;
	int c1,c2;
	struct point line_p;

	vx=line_p1->x-line_p0->x;
	vy=line_p1->y-line_p0->y;
	wx=p->x-line_p0->x;
	wy=p->y-line_p0->y;
	
	c1=vx*wx+vy*wy;
	if ( c1 <= 0 )
		return within_dist_point(p, line_p0, dist);
	c2=vx*vx+vy*vy;
	if ( c2 <= c1 )
		return within_dist_point(p, line_p1, dist);

	line_p.x=line_p0->x+vx*c1/c2;
	line_p.y=line_p0->y+vy*c1/c2;
	return within_dist_point(p, &line_p, dist);
}

static int
within_polygon(struct point *p, struct point *poly_pnt, int count)
{
	int i, j, c = 0;
	for (i = 0, j = count-1; i < count; j = i++) {
		if ((((poly_pnt[i].y <= p->y) && ( p->y < poly_pnt[j].y )) ||
		((poly_pnt[j].y <= p->y) && ( p->y < poly_pnt[i].y))) &&
		(poly_pnt->x < (poly_pnt[j].x - poly_pnt[i].x) * (p->y - poly_pnt[i].y) / (poly_pnt[j].y - poly_pnt[i].y) + poly_pnt[i].x)) {
       		   	c = !c;
		}
      	}
	return c;
}

static int
within_dist_lines(struct point *p, struct point *line_pnt, int count, int dist)
{
	int i;
	for (i = 0 ; i < count-1 ; i++) {
		if (within_dist_line(p,line_pnt+i,line_pnt+i+1,dist)) {
			return 1;
		}
	}
	return 0;
}

void
display_free(struct display_list **list, int count)
{
	struct display_list *curr,*next;
	while (count--) {
		curr=*list;
		while (curr) {
			next=curr->next;
			g_free(curr);
			curr=next;
		}
		*list++=NULL;
	}
}

void *
display_add(struct display_list **head, int type, int attr, char *label, int count, struct point *p, void (*info)(struct display_list *list, struct popup_item **item),void *data, int data_size)
{
	struct display_list *new;
	int label_len=0;

	if (! data)
		data_size=0;

	if (label)
		label_len=strlen(label)+1;
	new=g_malloc(sizeof(*new)+count*sizeof(*p)+label_len+data_size);
	new->type=type;
	new->attr=attr;
	new->info=info;
	if (label) {
		new->label=(char *)new+sizeof(*new)+count*sizeof(*p);
		strcpy(new->label, label);
	} else 
		new->label=NULL;
	new->count=count;
	memcpy(new->p, p, count*sizeof(*p));
	if (data_size) {
		new->data=(char *)new+sizeof(*new)+count*sizeof(*p)+label_len;
		memcpy(new->data, data, data_size);
	} else
		new->data=NULL;
	new->next=*head;
	*head=new;
	return new->data;
}

void
display_draw(struct display_list *list, struct graphics *gr, struct graphics_gc *gc_fill, struct graphics_gc *gc_line)
{
	struct graphics_image *icon;
	int r=3;
	struct point p;
	while (list) {
		switch (list->type) {
		case 0:
			gr->draw_polygon(gr, gc_fill, list->p, list->count);
			if (gc_line) 
				gr->draw_lines(gr, gc_line, list->p, list->count);
			break;
		case 1:
		case 2:
			gr->draw_lines(gr, gc_fill, list->p, list->count);
			break;
		case 3:
		case 4:
			gr->draw_circle(gr, gc_fill, list->p, r);
			break;
		case 5:
			icon=get_icon(gr, list->label);
			if (icon) {
				p.x=list->p[0].x - icon->width/2;
				p.y=list->p[0].y - icon->height/2;
				gr->draw_image(gr, gc_fill, &p, icon);
			}
			else
				printf("invalid icon '%s'\n", list->label);
			break;
		}
		list=list->next;
	}
}

void
display_find(struct point *p, struct display_list **in, int in_count, int maxdist, struct display_list **out, int out_count)
{
	int i=0;
	struct display_list *curr;

	while (in_count--) {
		curr=*in++;
		while (curr) {
			switch (curr->type) {
			case 0:
				if (within_polygon(p, curr->p, curr->count) ||
					within_dist_lines(p, curr->p, curr->count, maxdist))
				{
					if (i < out_count)
						out[i++]=curr;
				}
				break;
			case 1:
			case 2:
				if (within_dist_lines(p, curr->p, curr->count, maxdist))
				{
					if (i < out_count)
						out[i++]=curr;
				}
				break;
			case 3:
			case 4:
			case 5:
				if (within_dist_point(p, curr->p, 8))
				{
					if (i < out_count)
						out[i++]=curr;
				}
				break;
			}
			curr=curr->next;
		}
	}
	if (i < out_count)
		out[i]=NULL;
}


static void
label_line(struct graphics *gr, struct graphics_gc *fg, struct graphics_gc *bg, struct graphics_font *font, struct point *p, int count, char *label)
{
	int i,x,y,tl;
	double dx,dy,l;
	struct point p_t;
	char *utf8;

	tl=strlen(label)*400;
	for (i = 0 ; i < count-1 ; i++) {
		dx=p[i+1].x-p[i].x;
		dx*=100;
		dy=p[i+1].y-p[i].y;
		dy*=100;
		l=(int)sqrt((float)(dx*dx+dy*dy));	
		if (l > tl) {
			x=p[i].x;
			y=p[i].y;
			if (dx < 0) {
				dx=-dx;
				dy=-dy;
				x=p[i+1].x;
				y=p[i+1].y;
			}
			x+=(l-tl)*dx/l/200;
			y+=(l-tl)*dy/l/200;
			x-=dy*45/l/10;
			y+=dx*45/l/10;
			p_t.x=x;
			p_t.y=y;
#if 0
			printf("display_text: '%s', %d, %d, %d, %d %d\n", label, x, y, dx*0x10000/l, dy*0x10000/l, l);
#endif
			if(!g_utf8_validate(label,-1,NULL)){
				gr->draw_text(gr, fg, bg, font, label, &p_t, dx*0x10000/l, dy*0x10000/l);
			} else {
				utf8=g_convert(label, -1, "utf8", "iso8859-1", NULL, NULL, NULL);		
				gr->draw_text(gr, fg, bg, font, utf8, &p_t, dx*0x10000/l, dy*0x10000/l);
				g_free(utf8);
			}
		}
	}	
}

void
display_labels(struct display_list *list, struct graphics *gr, struct graphics_gc *fg, struct graphics_gc *bg, struct graphics_font *font)
{
	struct point p;
	char *utf8;
	while (list) {
		if (list->label) {
			switch (list->type) {
			case 1:
			case 2:
				label_line(gr, fg, bg, font, list->p, list->count, list->label);
				break;
			case 3:
				p.x=list->p[0].x+3;
				p.y=list->p[0].y+10;

				if(g_utf8_validate(list->label,-1, NULL)){
					utf8=g_convert(list->label, -1, "utf8", "iso8859-1", NULL, NULL, NULL);		
					gr->draw_text(gr, fg, bg, font, utf8, &p, 0x10000, 0);
					g_free(utf8);
				} else {
					gr->draw_text(gr, fg, bg, font, list->label, &p, 0x10000, 0);
				}
				break;
			}
		}
		list=list->next;
	}
}
