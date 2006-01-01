#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <glib.h>
#include "container.h"
#include "coord.h"
#include "map_data.h"
#include "file.h"
#include "block.h"
#include "route.h"
#include "street.h"
#include "street_data.h"
#include "street_name.h"
#include "display.h"
#include "draw_info.h"
#include "data_window.h"
#include "data.h"
#include "tree.h"


static void
street_draw_segment(struct container *co, struct segment *seg, unsigned char **pos, unsigned char *end, struct coord *ref, int bytes, int include, int disp)
{
        int j,flags,limit;
	struct coord f;
	struct coord l[2];
	struct street_str *str=seg->data[0];
	char *label;
	struct street_name name;
	struct param_list param[100];
	struct route_path_segment *route;
	struct display_list **displ=co->disp;
	int max=10000;
	struct point xpoints[max];

        flags=0;
	j=0;
        while (! flags && *pos < end) {
                flags=street_get_coord(pos, bytes, ref, &f);
		if (! j) {
			l[0]=f;
			l[1]=f;
		} else {
			if (include || !flags) {
				if (f.x < l[0].x) l[0].x=f.x;
				if (f.x > l[1].x) l[1].x=f.x;
				if (f.y > l[0].y) l[0].y=f.y;
				if (f.y < l[1].y) l[1].y=f.y;
			}
		}
		transform(co->trans, &f, &xpoints[j]);
		if (! j)
			flags=0;
                j++;
		assert(j < max);
        }
        if (! include)
                j--;
	if (is_visible(co->trans, l) && str->type) {
		label=NULL;
		if (str->nameid) {
			street_name_get_by_id(&name, seg->blk_inf.mdata, str->nameid);
			if (name.name2[0]) 
				label=name.name2;
			else
				label=name.name1;
		}
#if 0
		if (str->nameid && name.townassoc < 0 ) {
			char buffer[128];
			sprintf(buffer,"-0x%x", -name.townassoc);
			label=g_strdup(buffer);
		}
#endif
		limit=0;
		if (str->limit == 0x30)
			limit=1;
		if (str->limit == 0x03)
			limit=-1;
		if (str->type & 0x40)
			limit=-limit;
		display_add(&displ[disp], 2, limit, label, j, xpoints, NULL, seg, sizeof(*seg));
		if (co->route && (route=route_path_get(co->route, str->segid))) {
			if (! route->offset)
				display_add(&displ[display_street_route], 2, 0, label, j, xpoints, NULL, NULL, 0);
			else if (route->offset > 0) 
				display_add(&displ[display_street_route], 2, 0, label, route->offset, xpoints, NULL, NULL, 0);
			else 
				display_add(&displ[display_street_route], 2, 0, label, j+route->offset, xpoints-route->offset, NULL, NULL, 0);
		}
		if (co->data_window[data_window_type_street]) 
			data_window_add(co->data_window[data_window_type_street], param, street_get_param(seg, param, 100, 0));
	}
        *pos-=2*bytes;
}

static void
street_safety_check(struct street_str *str)
{
#if 0
	if (!((str->type & 0xf0) == 0x0 || (str->type & 0xf0) == 0x40)) {
		printf("str->type=0x%x\n", str->type);
	}
	assert((str->type & 0xf0) == 0x0 || (str->type & 0xf0) == 0x40);
#endif
	assert(str->type != 0xe && str->type != 0x4e && str->type != 0x40);
	assert(str->unknown2 == str->unknown3);
	assert(str->unknown2 == 0x40 || str->unknown2 == 0x44);
}

struct street_header_type {
	struct street_header *header;
	int type_count;
	struct street_type *type;
};

static void
street_header_type_get(struct block *blk, unsigned char **p_p, struct street_header_type *ret)
{
	unsigned char *p=*p_p;
	ret->header=(struct street_header *)p;
	p+=sizeof(struct street_header);
	ret->type_count=blk->count;
	ret->type=(struct street_type *)p;	
	p+=ret->type_count*sizeof(struct street_type);
	assert(ret->header->count == blk->count);
	*p_p=p;
}

static void
street_coord_get_begin(unsigned char **p_p)
{
	unsigned char *p=*p_p;
	struct street_str *str;

	str=(struct street_str *)p;
	while (str->segid) {
		str++;
	}
	p=(unsigned char *)str;
	p+=4;
	*p_p=p;
}

struct street_coord_handle
{
	unsigned char *p;
	unsigned char *p_rewind;
	unsigned char *next;
	unsigned char *end;
	int bytes;
	int status;
	int status_rewind;
	struct coord *ref;
};

struct street_coord_handle * 
street_coord_handle_new(struct block_info *blk_inf, struct street_str *str)
{
	struct street_coord_handle *ret;
	unsigned char *p=(unsigned char *)(blk_inf->block);
	struct block *blk;
	struct street_header_type hdr_type;
	struct street_str *str_curr;
	int num;

	ret=g_new(struct street_coord_handle,1);

	ret->end=p;
	blk=block_get(&p);
	ret->end+=blk->size;
	ret->bytes=street_get_bytes(blk);
	ret->ref=blk->c;

	street_header_type_get(blk, &p, &hdr_type);
	str_curr=(struct street_str *)p;
	num=str-str_curr;
	street_coord_get_begin(&p);
        street_get_coord(&p, ret->bytes, blk->c, NULL);
	while (num && str_curr->segid) {
        	while (! street_get_coord(&p, ret->bytes, blk->c, NULL) && p < ret->end)
			ret->p=p;
		str_curr++;
		num--;
	}
	ret->status=str[1].segid > 0 ? 0:1;
	ret->p_rewind=ret->p;
	ret->status_rewind=ret->status;

	return ret;
}

void
street_coord_handle_rewind(struct street_coord_handle *h)
{
	h->p=h->p_rewind;
	h->status=h->status_rewind;
}

int
street_coord_handle_get(struct street_coord_handle *h, struct coord *c)
{
	unsigned char *n;

	if (h->p >= h->end) 
		return 1;
	if (h->status >= 4)
		return 1;
	n=h->p;
	if (street_get_coord(&h->p, h->bytes, h->ref, c)) {
		h->next=n;
		h->status+=2;
		if (h->status == 5)
			return 1;
	}
	return 0;
}

void
street_coord_handle_destroy(struct street_coord_handle *handle)
{
	g_free(handle);
}

void
street_draw_block(struct block_info *blk_inf, unsigned char *p, unsigned char *end, void *data)
{
	struct street_str *str;
	struct street_header_type header_type;
	struct street_type *str_type;
	int include,count,ncount,bytes,offset;
	struct draw_info *drw_inf=data;
	struct segment seg;

	seg.blk_inf=*blk_inf;

	street_header_type_get(blk_inf->block, &p, &header_type);
	if (header_type.header->order >= drw_inf->limit)
		return;

	str_type=header_type.type;
	bytes=street_get_bytes(blk_inf->block);
	str=(struct street_str *)p;
	count=0;
	ncount=0;
	street_coord_get_begin(&p);
	str_type--;
	while (str->segid) {
		include=1;
              	if (str[1].segid < 0) {
			include=0;
			str_type++;
		}
		seg.data[0]=str;
		seg.data[1]=str_type;
		seg.data[2]=p;
		seg.data[3]=header_type.header;
		street_safety_check(str);
		offset=0;
		if (header_type.header->order < 0x2)
			offset=3;
		else if (header_type.header->order < 0x4 && str->type != 0x6 && str->type != 0x46) 
			offset=2;
		else if (header_type.header->order < 0x6) 
			offset=1;
		if (str->limit == 0x33) 
			offset=4;
		street_draw_segment(drw_inf->co, &seg, &p, end-4, blk_inf->block->c, bytes, include, drw_inf->display+offset);
		str++;
	}
}

void
street_bti_draw_block(struct block_info *blk_inf, unsigned char *p, unsigned char *end, void *data)
{
	struct draw_info *drw_inf=data;
	struct street_bti *str;
	struct point pnt;
	struct param_list param[100];
	struct segment seg;

	while (p < end) {
		str=(struct street_bti *)p;
		seg.data[0]=str;
		p+=sizeof(*str);
		if (transform(drw_inf->co->trans, &str->c, &pnt)) {
                        display_add(&drw_inf->co->disp[display_bti], 4, 0, NULL, 1, &pnt, NULL, &seg, sizeof(seg));
			if (drw_inf->co->data_window[data_window_type_point]) 
				data_window_add(drw_inf->co->data_window[data_window_type_point], param, street_bti_get_param(&seg, param, 100));
		}
	}
}

void
street_route_draw(struct container *co)
{
	struct route_path_segment *route=NULL;
	struct point xpoints[2];

	if (co->route)
		route=route_path_get_all(co->route);
	while (route) {
		if (!route->segid) {
			transform(co->trans, &route->c[0], &xpoints[0]);
			transform(co->trans, &route->c[1], &xpoints[1]);
			display_add(&co->disp[display_street_route], 2, 0, NULL, 2, xpoints, NULL, NULL, 0);
		}
		route=route->next;
	}
}

struct street_coord *
street_coord_get(struct block_info *blk_inf, struct street_str *str)
{
	struct block *blk;
	unsigned char *p=(unsigned char *)(blk_inf->block),*end,*p_sav;
	struct street_header_type hdr_type;
	struct street_str *str_curr;
	struct coord f,*c;
	struct street_coord *ret;
	int bytes,num,points,include;
	int debug=0;

	end=p;
	blk=block_get(&p);
	end+=blk->size;

	street_header_type_get(blk, &p, &hdr_type);
	str_curr=(struct street_str *)p;
	num=str-str_curr;
	street_coord_get_begin(&p);
	bytes=street_get_bytes(blk);
	if (debug) {
		printf("num=%d\n", num);
	}
	street_get_coord(&p, bytes, blk->c, &f);
	while (num && str_curr->segid) {
        	while (! street_get_coord(&p, bytes, blk->c, &f) && p < end);
		str_curr++;
		num--;
	}
	include=(str[1].segid > 0);	
	p_sav=p-2*bytes;
	points=1+include;
       	while (! street_get_coord(&p, bytes, blk->c, &f) && p < end)
		points++;

	if (debug)			
		printf("p=%p points=%d\n",p_sav, points);
	p=p_sav;
	ret=malloc(sizeof(struct street_coord)+points*sizeof(struct coord));
	ret->count=points;
	c=ret->c;	
	while (points) {
		street_get_coord(&p, bytes, blk->c, c);
		c++;
		points--;
	}
	return ret;
}

struct street_get_block_param
{
	void *data;
	int (*callback)(struct street_str *str, struct street_coord_handle *h, void *data);
};

static void
street_get_block_process(struct block_info *blk_inf, unsigned char *p, unsigned char *end, void *data)
{
	struct street_header_type header_type;
	struct street_coord_handle h;
	struct street_str *str;
	int count;
	struct street_get_block_param *param=(struct street_get_block_param *)data;

	street_header_type_get(blk_inf->block, &p, &header_type);

	h.bytes=street_get_bytes(blk_inf->block);
	h.end=end;
	h.ref=blk_inf->block->c;

	str=(struct street_str *)p;
	count=0;
	street_coord_get_begin(&p);
	h.p=p;

	while (str->segid) {
		h.status=str[1].segid > 0 ? 0:1;
		h.p_rewind=h.p;
		h.status_rewind=h.status;
		param->callback(str, &h, param->data);
		while (! h.next && street_coord_handle_get(&h, NULL));
		h.p=h.next;
		str++;
	}
}

void
street_get_block(struct map_data *mdata, struct transformation *t, void (*callback)(void *data), void *data)
{
	struct street_get_block_param param;

	param.callback=callback;
	param.data=data;
        map_data_foreach(mdata, file_street_str, t, 48, street_get_block_process, &param);

}

int
street_get_by_id(struct map_data *mdat, int id, struct block_info *res_blk_inf, struct street_str **res_str)
{
	int debug=0;
	int res,block,num;
	struct map_data *mdat_res;
	struct block *blk;
	unsigned char *p;
	struct street_header_type hdr_type;
	struct street_str *str;

	if (tree_search_hv_map(mdat, file_street_str, (id >> 8) | 0x31000000, id & 0xff, &res, &mdat_res)) {
		return 1;
	}

	block=res >> 12;
	num=res & 0xfff;
	if (debug) {
		printf("block=0x%x\n", block);
		printf("num=0x%x\n", num);	
	}
	blk=block_get_byindex(mdat_res->file[file_street_str], block, &p);
	res_blk_inf->mdata=mdat_res;
	res_blk_inf->file=mdat_res->file[file_street_str];
	res_blk_inf->block=blk;
	if (debug) {
		printf("blk->count=0x%x\n", blk->count);
	}
	street_header_type_get(blk, &p, &hdr_type);
	str=(struct street_str *)p;
	str+=num;
	*res_str=str;
	return 0;
}

int
street_get_param(struct segment *seg, struct param_list *param, int count, int verbose)
{
	char buffer[1024];
	int i=count,j;
	struct street_str *str=seg->data[0];
	struct street_type *type=seg->data[1];
	struct street_name name;
	struct street_name_info name_info;
#if 0
	struct street_name_number_info name_number_info;
#endif

	param_add_hex("Type-Addr", (unsigned char *)type-seg->blk_inf.file->begin, &param, &count);
	param_add_hex("Order", type->order, &param, &count);
	param_add_hex("Country", type->country, &param, &count);

	param_add_hex("Addr", (unsigned char *)str-seg->blk_inf.file->begin, &param, &count);
	param_add_hex_sig("Seg-Id", str->segid, &param, &count);
	param_add_hex("Limit", str->limit, &param, &count);
	param_add_hex("Unknown2", str->unknown2, &param, &count);
	param_add_hex("Unknown3", str->unknown3, &param, &count);
	param_add_hex("Type", str->type, &param, &count);
	param_add_hex("Name-Id", str->nameid, &param, &count);
	if (str->segid) {
		street_name_get_by_id(&name, seg->blk_inf.mdata, str->nameid);

		param_add_hex("Len", name.len, &param, &count);
		param_add_hex("Country", name.country, &param, &count);
		param_add_hex_sig("TownAssoc", name.townassoc, &param, &count);
		printf("TownAssoc 0x%lx\n", name.townassoc+str->segid);
		param_add_string("Name1", name.name1, &param, &count);
		param_add_string("Name2", name.name2, &param, &count);
		param_add_hex("Segments", name.segment_count, &param, &count);
		if (verbose) {
			for (j = 0 ; j < name.segment_count ; j++) {
				sprintf(buffer,"0x%x 0x%x", name.segments[j].country, name.segments[j].segid);
				param_add_string("Segment", buffer, &param, &count);
			}
			param_add_hex("Len", name.aux_len, &param, &count);
			while (street_name_get_info(&name_info, &name)) {
				param_add_hex("Len", name_info.len, &param, &count);
				param_add_hex("Tag", name_info.tag, &param, &count);
				param_add_hex("Dist", name_info.dist, &param, &count);
				param_add_hex("Country", name_info.country, &param, &count);
				param_add_hex("X", name_info.c->x, &param, &count);
				param_add_hex("Y", name_info.c->y, &param, &count);
				param_add_dec("First", name_info.first, &param, &count);
				param_add_dec("Last", name_info.last, &param, &count);
				param_add_hex("Segments", name_info.segment_count, &param, &count);
			}
#if 0
				int tag;
				int k, segs;
				printf("\n");
				printf("Len 0x%x\n",get_short(&stn));
				tag=*stn++;
				printf("Tag 0x%x\n",tag);
				if (tag == 0xc0 || tag == 0xd0 || tag == 0xe0) {
					printf("DistAssoc 0x%lx\n",get_long(&stn));
					printf("Country 0x%lx\n",get_long(&stn));
					printf("X 0x%lx\n",get_long(&stn));
					printf("Y 0x%lx\n",get_long(&stn));
					printf("First %ld\n",get_triple(&stn));
					printf("Last %ld\n",get_triple(&stn));
					segs=get_long(&stn);
					printf("Segs 0x%x\n",segs);
					for (k = 0 ; k < 0 ; k++) {
						printf("SegId 0x%lx\n", get_long(&stn));
						printf("Country 0x%lx\n",get_long(&stn));
					}
				} else if (tag == 0x8f || tag == 0xaa || tag == 0xab || tag == 0xae || tag == 0xaf || tag == 0x9a || tag == 0x9e || tag == 0x9f) {
					printf("X 0x%lx\n",get_long(&stn));
					printf("Y 0x%lx\n",get_long(&stn));
					printf("First %ld\n",get_triple(&stn));
					printf("Last %ld\n",get_triple(&stn));
					printf("SegId 0x%lx\n",get_long(&stn));
					printf("Country 0x%lx\n",get_long(&stn));
				} else {
					printf("Unknown tag 0x%x\n", tag);
					break;
				}
			}
#endif
		}
	} else {
		if (!verbose) {
			param_add_string("Len", "", &param, &count);
			param_add_string("Country", "", &param, &count);
			param_add_string("TownAssoc", "", &param, &count);
			param_add_string("Name1", "", &param, &count);
			param_add_string("Name2", "", &param, &count);
			param_add_string("Segments", "", &param, &count);
		}
	}
	return i-count;
}

int
street_bti_get_param(struct segment *seg, struct param_list *param, int count)
{
	int i=count;
	struct street_bti *str=seg->data[0];

	param_add_hex("Addr", (unsigned char *)str-seg->blk_inf.file->begin, &param, &count);
	param_add_hex("Unknown1", str->unknown1, &param, &count);
	param_add_hex("Segid1", str->segid1, &param, &count);
	param_add_hex("Country1", str->country1, &param, &count);
	param_add_hex("Segid2", str->segid2, &param, &count);
	param_add_hex("Country2", str->country2, &param, &count);
	param_add_hex("Unknown5", str->unknown5, &param, &count);
	param_add_hex("X", str->c.x, &param, &count);
	param_add_hex("Y", str->c.y, &param, &count);

	return i-count;
}
