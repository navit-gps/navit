#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "display.h"
#include "coord.h"
#include "data_window.h"
#include "map_data.h"
#include "town.h"
#include "data.h"
#include "tree.h"
#include "block.h"
#include "file.h"
#include "draw_info.h"
#include "container.h"
#include "util.h"

void
town_get(struct town *town, unsigned char **p)
{
	town->id=get_long(p);
	town->c=coord_get(p);
	town->name=get_string(p);
	town->district=get_string(p);
	town->postal_code1=get_string(p);
	town->order=get_char(p);
	town->country=get_short(p);
	town->type=get_char(p);
	town->unknown2=get_long(p);
	town->size=get_char(p);
	town->street_assoc=get_long(p);
	town->unknown3=get_char(p);
	town->postal_code2=get_string(p);
	town->unknown4=get_long(p);
}

void
town_draw_block(struct block_info *blk_inf, unsigned char *start, unsigned char *end, void *data)
{
	unsigned char *p=start;
	struct town town;
	char *s;
	struct point pnt;
	struct param_list param[100];
	struct segment seg;
	struct draw_info *drw_inf=data;
	struct data_window *win=drw_inf->co->data_window[data_window_type_town];

	seg.blk_inf=*blk_inf;

	p+=4;
	while (p < end) {
		seg.data[0]=p;
		town_get(&town, &p);
		if (town.order < drw_inf->limit && transform(drw_inf->co->trans, town.c, &pnt)) {
			s=town.name;
			if (*town.district)
				s=town.district;
			display_add(&drw_inf->co->disp[drw_inf->display+town.order], 3, 0, s, 1, &pnt, NULL, &seg, sizeof(seg));
			if (win)
				data_window_add(win, param, town_get_param(&seg, param, 100));
		}
	}
}

struct town_search_priv {
	int country;
	const char *name;
	int name_len;
	int partial;
	int leaf;
	int (*func)(struct town *, void *data);
	void *data;
};

static int
town_tree_process(int version, int leaf, unsigned char **s2, struct map_data *mdat, void *data)
{
	int len, cmp, country;
	struct block *blk;
	struct town_search_priv *priv_data=data;
	struct file *f=mdat->file[file_town_twn];
	struct block_offset *blk_off;
	struct town town;
	unsigned char *p,*name;
	int ret,i,debug=0;

	country=get_short(s2);
	name=get_string(s2);
	if (debug) {
		printf("Country: 0x%x ", country);
		printf("Town: '%s' ", name);
	}
	len=get_long(s2);
	
	blk_off=(struct block_offset *)(*s2);
	*s2+=len*4;

	cmp=priv_data->country-country;
	if (! cmp) {
		if (leaf == -1)
			priv_data->leaf=0;
		if (leaf == 1)
			priv_data->leaf=1;

		if (debug) {
			printf("'%s' vs '%s' version=%d\n", priv_data->name, name, version);
		}
		if (priv_data->leaf && priv_data->partial)
			cmp=strncmp(priv_data->name,name,priv_data->name_len);
		else
			cmp=strcmp(priv_data->name,name);
	} else {
		if (debug) {
			printf("country mismatch\n");
		}
	}

	if (cmp > 0)
		return 2;
	if (cmp < 0)
		return -2;
	if (debug)
		printf("%s\n", name);
	for (i = 0 ; i < len ; i++) {	
		blk=block_get_byindex(f, blk_off->block, &p);
		p=(unsigned char *)blk+blk_off->offset;
		town_get(&town, &p);
		ret=(*priv_data->func)(&town, priv_data->data);
		if (ret)
			return 1;
		blk_off++;
	}
	return 0;
}

int
town_search_by(struct map_data *mdat, char *ext, int country, const char *search, int partial, int (*func)(struct town *, void *data), void *data)
{
	struct town_search_priv priv_data;
	priv_data.country=country;
	priv_data.name=search;
	priv_data.name_len=strlen(search);
	priv_data.partial=partial;
	priv_data.leaf=0;
	priv_data.func=func;
	priv_data.data=data;

	tree_search_map(mdat, file_town_twn, ext, town_tree_process, &priv_data);

	return 0;
}

int
town_search_by_postal_code(struct map_data *mdat, int country, unsigned char *name, int partial, int (*func)(struct town *, void *data), void *data)
{
	unsigned char uname[strlen(name)+1];
	
	strtoupper(uname, name);
	return town_search_by(mdat, "b1", country, uname, partial, func, data);
}


int
town_search_by_name(struct map_data *mdat, int country, const char *name, int partial, int (*func)(struct town *, void *data), void *data)
{
	unsigned char uname[strlen(name)+1];
	
	strtolower(uname, (char *)name);
	return town_search_by(mdat, "b2", country, uname, partial, func, data);
}

int
town_search_by_district(struct map_data *mdat, int country, unsigned char *name, int partial, int (*func)(struct town *, void *data), void *data)
{
	unsigned char uname[strlen(name)+1];
	
	strtoupper(uname, name);
	return town_search_by(mdat, "b3", country, uname, partial, func, data);
}

int
town_search_by_name_phon(struct map_data *mdat, int country, unsigned char *name, int partial, int (*func)(struct town *, void *data), void *data)
{
	unsigned char uname[strlen(name)+1];
	
	strtoupper(uname, name);
	return town_search_by(mdat, "b4", country, uname, partial, func, data);
}

int
town_search_by_district_phon(struct map_data *mdat, int country, unsigned char *name, int partial, int (*func)(struct town *, void *data), void *data)
{
	unsigned char uname[strlen(name)+1];
	
	strtoupper(uname, name);
	return town_search_by(mdat, "b5", country, uname, partial, func, data);
}


void
town_get_by_id(struct town *town, struct map_data *mdat, int country, int id)
{
	struct map_data *mdat_res;
	struct file *f;
	int res,block,offset;
	struct block *blk;
	unsigned char *p;
	int search1,search2;
	unsigned long id2=id;

	if (id < 0)
		id=-id;
	search2 = id2 & 0xff;
	search1 = (id2 >> 8) | (country << 24);
	printf("country=0x%x id=0x%lx -id=0x%lx search1=0x%x search2=0x%x\n", country, id2, -id2, search1, search2);
	return;
	tree_search_hv_map(mdat, file_town_twn, search1, search2, &res, &mdat_res);
	printf("res=0x%x\n",res);
	block=res >> 16;
	offset=res & 0xffff;
	f=mdat_res->file[file_town_twn];
	printf("file %s block 0x%x offset 0x%x\n", f->name, block, offset);
	blk=block_get_byindex(f, block, &p);
	p=(unsigned char *)blk+offset;
	printf("addr 0x%x\n", p-f->begin);
	town_get(town, &p);
}

int
town_get_param(struct segment *seg, struct param_list *param, int count)
{
        int i=count;
	unsigned char *p;
	struct town town;

	p=(unsigned char *)(seg->blk_inf.block);
	p+=sizeof(*seg->blk_inf.block);
	
        param_add_hex("Block Unknown", get_long(&p), &param, &count);
	p=seg->data[0];
        param_add_hex("Address", p-seg->blk_inf.file->begin, &param, &count);
	town_get(&town, &p);
        param_add_hex("ID", town.id, &param, &count);
        param_add_hex_sig("X", town.c->x, &param, &count);
        param_add_hex_sig("Y", town.c->y, &param, &count);
        param_add_string("Name", town.name, &param, &count);
        param_add_string("District", town.district, &param, &count);
        param_add_string("PostalCode1", town.postal_code1, &param, &count);
        param_add_hex("Order", town.order, &param, &count);
        param_add_hex("Country", town.country, &param, &count);
        param_add_hex("Type", town.type, &param, &count);
        param_add_hex("Unknown2", town.unknown2, &param, &count);
        param_add_hex("Size", town.size, &param, &count);
        param_add_hex("StreetAssoc", town.street_assoc, &param, &count);
        param_add_hex("Unknown3", town.unknown3, &param, &count);
        param_add_string("PostalCode2", town.postal_code2, &param, &count);
        param_add_hex("Unknown4", town.unknown4, &param, &count);
	
        return i-count;
}      
