#include <stdlib.h>
#include <assert.h>
#include "coord.h"
#include "map_data.h"
#include "file.h"
#include "block.h"
#include "poly.h"
#include "display.h"
#include "draw_info.h"
#include "data_window.h"
#include "container.h"

extern struct data_window poly_window;


void poly_draw_segment(struct container *co, struct segment *seg, int disp, unsigned char **p, int limit);
int poly_get_hdr(unsigned char **p,struct poly_hdr *poly_hdr);

int 
poly_get_hdr(unsigned char **p,struct poly_hdr *poly_hdr)
{
	poly_hdr->addr=*p;
	poly_hdr->c=(struct coord *) (*p);
	*p+=3*sizeof(struct coord);
	poly_hdr->name=*p;
	while (**p) {
		(*p)++;
	}
	(*p)++;
	poly_hdr->order=*(*p)++;
	poly_hdr->type=*(*p)++;
	poly_hdr->polys=*(unsigned long *)(*p); (*p)+=sizeof(unsigned long);
	poly_hdr->count=(unsigned long *)(*p); (*p)+=poly_hdr->polys*sizeof(unsigned long);
	poly_hdr->count_sum=*(unsigned long *)(*p); (*p)+=sizeof(unsigned long);
	return 0;
}

void
poly_draw_segment(struct container *co, struct segment *seg, int disp, unsigned char **p, int limit)
{
	struct poly_hdr poly_hdr;
	struct coord *coord;
	unsigned int j,k,o;
	struct point pnt;
	struct param_list param[100];
	int max=20000;
	struct point xpoints[max];

	seg->data[0]=*p;
	poly_get_hdr(p, &poly_hdr);
	if (poly_hdr.order < limit && is_visible(co->trans, poly_hdr.c)) {
		transform(co->trans,&poly_hdr.c[2],&pnt);
		for (k = 0 ; k < poly_hdr.polys ; k++) {
			assert(poly_hdr.count[k] < max);
			for (j = 0 ; j < poly_hdr.count[k] ; j++) {
				transform(co->trans, coord_get(p), &xpoints[j]);
			}
			if (poly_hdr.type < 0x80) {
				o=0;
				if (poly_hdr.type == 0x1e)
					o=1;
				else if (poly_hdr.type == 0x2d)
					o=2;
				else if (poly_hdr.type == 0x32)
					o=3;
				display_add(&co->disp[disp+o], 0, 0, poly_hdr.name, poly_hdr.count[k], xpoints, NULL, seg, sizeof(*seg));
				
			} else {
				display_add(&co->disp[disp], 1, 0, poly_hdr.name, poly_hdr.count[k], xpoints, NULL, seg, sizeof(*seg));
			}
			if (co->data_window[data_window_type_poly])
				data_window_add(co->data_window[data_window_type_poly], param, poly_get_param(seg, param, 100)); 
		}
	} else
		(*p)+=poly_hdr.count_sum*sizeof(*coord);
}

void
poly_draw_block(struct block_info *blk_inf, unsigned char *p, unsigned char *end, void *data)
{
	struct draw_info *drw_inf=data;
	struct segment seg;
	int i;

	seg.blk_inf=*blk_inf;

	for (i = 0 ; i < blk_inf->block->count ; i++)
		poly_draw_segment(drw_inf->co, &seg, drw_inf->display, &p, drw_inf->limit);
}


int
poly_get_param(struct segment *seg, struct param_list *param, int count)
{
	int i=count;
	unsigned char *p=seg->data[0];
	struct poly_hdr poly_hdr;


	param_add_hex("Addr", p-seg->blk_inf.file->begin, &param, &count);
	poly_get_hdr(&p, &poly_hdr);
	param_add_string("Name", poly_hdr.name, &param, &count);
	param_add_hex("L", poly_hdr.c[0].x, &param, &count);
	param_add_hex("T", poly_hdr.c[0].y, &param, &count);
	param_add_hex("R", poly_hdr.c[1].x, &param, &count);
	param_add_hex("B", poly_hdr.c[1].y, &param, &count);
	param_add_hex("X", poly_hdr.c[2].x, &param, &count);
	param_add_hex("Y", poly_hdr.c[2].y, &param, &count);
	param_add_hex("Order", poly_hdr.order, &param, &count);
	param_add_hex("Type", poly_hdr.type, &param, &count);
	param_add_hex("Polys", poly_hdr.polys, &param, &count);
	return i-count;
}
