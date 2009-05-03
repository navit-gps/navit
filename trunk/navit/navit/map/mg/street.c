/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
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

#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "mg.h"

int coord_debug;

static void
street_name_get(struct street_name *name, unsigned char **p)
{
	unsigned char *start=*p;
	name->len=get_u16_unal(p);
	name->country=get_u16_unal(p);
	name->townassoc=get_u32_unal(p);
	name->name1=get_string(p);
	name->name2=get_string(p);
	name->segment_count=get_u32_unal(p);
	name->segments=(struct street_name_segment *)(*p);
	(*p)+=(sizeof (struct street_name_segment))*name->segment_count;
	name->aux_len=name->len-(*p-start);
	name->aux_data=*p;
	name->tmp_len=name->aux_len;
	name->tmp_data=name->aux_data;
	*p=start+name->len;
}

static void
street_name_numbers_get(struct street_name_numbers *name_numbers, unsigned char **p)
{
	unsigned char *start=*p;
	name_numbers->len=get_u16_unal(p);
	name_numbers->tag=get_u8(p);
	name_numbers->dist=get_u32_unal(p);
	name_numbers->country=get_u32_unal(p);
	name_numbers->c=coord_get(p);
	name_numbers->first=get_u24_unal(p);
	name_numbers->last=get_u24_unal(p);
	name_numbers->segment_count=get_u32_unal(p);
	name_numbers->segments=(struct street_name_segment *)(*p);
	(*p)+=sizeof(struct street_name_segment)*name_numbers->segment_count;
	name_numbers->aux_len=name_numbers->len-(*p-start);
	name_numbers->aux_data=*p;
	name_numbers->tmp_len=name_numbers->aux_len;
	name_numbers->tmp_data=name_numbers->aux_data;
	*p=start+name_numbers->len;
}

static void
street_name_number_get(struct street_name_number *name_number, unsigned char **p)
{
	unsigned char *start=*p;
        name_number->len=get_u16_unal(p);
        name_number->tag=get_u8(p);
        name_number->c=coord_get(p);
        name_number->first=get_u24_unal(p);
        name_number->last=get_u24_unal(p);
        name_number->segment=(struct street_name_segment *)p;
	*p=start+name_number->len;
}

static void
street_name_get_by_id(struct street_name *name, struct file *file, unsigned long id)
{
	unsigned char *p;
	if (id) {
		p=file->begin+id+0x2000;
		street_name_get(name, &p);
	}
}

static int street_get_bytes(struct coord_rect *r)
{
	int bytes,dx,dy;
        bytes=2;
        dx=r->rl.x-r->lu.x;
        dy=r->lu.y-r->rl.y;
	dbg_assert(dx > 0);
	dbg_assert(dy > 0); 
        if (dx > 32767 || dy > 32767)
                bytes=3;
        if (dx > 8388608 || dy > 8388608)
                bytes=4;                  
	
	return bytes;
}

static int street_get_coord(unsigned char **pos, int bytes, struct coord *ref, struct coord *f)
{
	unsigned char *p;
	int x,y,flags=0;
		
	p=*pos;
        x=*p++;
        x|=(*p++) << 8;
        if (bytes == 2) {
		if (   x > 0x7fff) {
			x=0x10000-x;
			flags=1;
		}
	}
	else if (bytes == 3) {
		x|=(*p++) << 16;
		if (   x > 0x7fffff) {
			x=0x1000000-x;
			flags=1;
		}
	} else {
		x|=(*p++) << 16;
		x|=(*p++) << 24;
		if (x < 0) {
			x=-x;
			flags=1;
		}
	}
	y=*p++;
	y|=(*p++) << 8;
	if (bytes == 3) {
		y|=(*p++) << 16;
	} else if (bytes == 4) {
		y|=(*p++) << 16;
		y|=(*p++) << 24;
	}
	if (f) {
		f->x=ref[0].x+x;
		f->y=ref[1].y+y;
	}
	dbg(1,"0x%x,0x%x + 0x%x,0x%x = 0x%x,0x%x\n", x, y, ref[0].x, ref[1].y, f->x, f->y);
	*pos=p;
	return flags;
}

static void
street_coord_get_begin(unsigned char **p)
{
	struct street_str *str;

	str=(struct street_str *)(*p);
	while (L(str->segid)) {
		str++;
	}
	(*p)=(unsigned char *)str;
	(*p)+=4;
}


static void
street_coord_rewind(void *priv_data)
{
	/* struct street_priv *street=priv_data; */

}

static int
street_coord_get_helper(struct street_priv *street, struct coord *c)
{
	unsigned char *n;
	if (street->p+street->bytes*2 >= street->end) 
		return 0;
	if (street->status >= 4)
		return 0;
	n=street->p;
	if (street_get_coord(&street->p, street->bytes, street->ref, c)) {
		if (street->status)
			street->next=n;
		street->status+=2;
		if (street->status == 5)
			return 0;
	}
	return 1;
}

static int
street_coord_get(void *priv_data, struct coord *c, int count)
{
	struct street_priv *street=priv_data;
	int ret=0,i,scount;

	if (! street->p && count) {
		street->p=street->coord_begin;
		scount=street->str-street->str_start;
		for (i = 0 ; i < scount ; i++) {
			street->status=L(street->str[i+1].segid) >= 0 ? 0:1;
			while (street_coord_get_helper(street, c));
			street->p=street->next;
		}
		street->status_rewind=street->status=L(street->str[1].segid) >= 0 ? 0:1;
	}
	while (count > 0) {
		if (street_coord_get_helper(street, c)) {
			c++;
			ret++;
			count--;
		} else {
			street->more=0;
			return ret;
		}
	}
	return ret;
}

static void
street_attr_rewind(void *priv_data)
{
	/* struct street_priv *street=priv_data; */

}

static int
street_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct street_priv *street=priv_data;
	int nameid;

	dbg(1,"segid 0x%x\n", L(street->str->segid));
	attr->type=attr_type;
	switch (attr_type) {
	case attr_any:
		while (street->attr_next != attr_none) {
			if (street_attr_get(street, street->attr_next, attr))
				return 1;
		}
		return 0;
	case attr_label:
		street->attr_next=attr_street_name;
		nameid=L(street->str->nameid);
		if (! nameid)
			return 0;
		if (! street->name.len)
			street_name_get_by_id(&street->name,street->name_file,nameid);
		attr->u.str=street->name.name2;
		if (attr->u.str && attr->u.str[0])
			return 1;
		attr->u.str=street->name.name1;
		if (attr->u.str && attr->u.str[0])
			return 1;
		return 0;
	case attr_street_name:
		street->attr_next=attr_street_name_systematic;
		nameid=L(street->str->nameid);
		if (! nameid)
			return 0;
		if (! street->name.len)
			street_name_get_by_id(&street->name,street->name_file,nameid);
		attr->u.str=street->name.name2;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_street_name_systematic:
		street->attr_next=attr_flags;
		nameid=L(street->str->nameid);
		if (! nameid)
			return 0;
		if (! street->name.len)
			street_name_get_by_id(&street->name,street->name_file,nameid);
		attr->u.str=street->name.name1;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_flags:
		attr->u.num=street->flags;
		street->attr_next=attr_country_id;
		return 1;
	case attr_country_id:
		street->attr_next=attr_debug;
		nameid=L(street->str->nameid);
		if (! nameid)
			return 0;
		if (! street->name.len)
			street_name_get_by_id(&street->name,street->name_file,nameid);
		attr->u.num=mg_country_to_isonum(street->name.country);
		return 1;
	case attr_debug:
		street->attr_next=attr_none;
		{
		struct street_str *str=street->str;
		sprintf(street->debug,"order:0x%x\nsegid:0x%x\nlimit:0x%x\nunknown2:0x%x\nunknown3:0x%x\ntype:0x%x\nnameid:0x%x\ntownassoc:0x%x",street->header->order,str->segid,str->limit,str->unknown2,str->unknown3,str->type,str->nameid, street->name.len ? street->name.townassoc : 0);
		attr->u.str=street->debug;
		}
		return 1;
	default:
		return 0;
	}
	return 1;
}

static struct item_methods street_meth = {
	street_coord_rewind,
	street_coord_get,
	street_attr_rewind,
	street_attr_get,
};

static void
street_get_data(struct street_priv *street, unsigned char **p)
{
	street->header=(struct street_header *)(*p);
	(*p)+=sizeof(struct street_header);
	street->type_count=street->header->count;
	street->type=(struct street_type *)(*p);	
	(*p)+=street->type_count*sizeof(struct street_type);
}


                            /*0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 */
static unsigned char limit[]={0,0,1,1,1,2,2,4,6,6,12,13,14,20,20,20,20,20,20};

int
street_get(struct map_rect_priv *mr, struct street_priv *street, struct item *item)
{	
	int *flags;
	for (;;) {
		while (street->more) {
			struct coord c;
			street_coord_get(street, &c, 1);
		}
		if (mr->b.p == mr->b.p_start) {
			street_get_data(street, &mr->b.p);
			street->name_file=mr->m->file[file_strname_stn];
			if (mr->cur_sel && street->header->order > limit[mr->cur_sel->order])
				return 0;
			street->end=mr->b.end;
			street->ref=&mr->b.b->r.lu;
			street->bytes=street_get_bytes(&mr->b.b->r);
			street->str_start=street->str=(struct street_str *)mr->b.p;
			street->coord_begin=mr->b.p;
			street_coord_get_begin(&street->coord_begin);
			street->p=street->coord_begin;
			street->type--; 
			item->meth=&street_meth;
			item->priv_data=street;
		} else {
			street->str++;
			street->p=street->next;
		}
		if (! L(street->str->segid))
			return 0;
		if (L(street->str->segid) < 0)
			street->type++;
#if 0
		dbg_assert(street->p != NULL);
#endif
		street->next=NULL;
		street->status_rewind=street->status=L(street->str[1].segid) >= 0 ? 0:1;
#if 0
		if (street->type->country != 0x31) {
			printf("country=0x%x\n", street->type->country);
		}
#endif
		item->id_hi=street->type->country | (mr->current_file << 16);
		item->id_lo=L(street->str->segid) > 0 ? L(street->str->segid) : -L(street->str->segid);
		switch(street->str->type & 0x1f) {
		case 0xf: /* very small street */
			if (street->str->limit == 0x33) 
				item->type=type_street_nopass;
			else
				item->type=type_street_0;
			break;
		case 0xd:
			item->type=type_ferry;
			break;
		case 0xc: /* small street */
			item->type=type_street_1_city;
			break;
		case 0xb:
			item->type=type_street_2_city;
			break;
		case 0xa:
			if ((street->str->limit == 0x03 || street->str->limit == 0x30) && street->header->order < 4)
				item->type=type_street_4_city;
			else	
				item->type=type_street_3_city;
			break;
		case 0x9:
			if (street->header->order < 5)
				item->type=type_street_4_city;
			else if (street->header->order < 7)
				item->type=type_street_2_city;
			else
				item->type=type_street_1_city;
			break;
		case 0x8:
			item->type=type_street_2_land;
			break;
		case 0x7:
			if ((street->str->limit == 0x03 || street->str->limit == 0x30) && street->header->order < 4)
				item->type=type_street_4_city;
			else
				item->type=type_street_3_land;
			break;
		case 0x6:
			item->type=type_ramp;
			break;
		case 0x5:
			item->type=type_street_4_land;
			break;
		case 0x4:
			item->type=type_street_4_land;
			break;
		case 0x3:
			item->type=type_street_n_lanes;
			break;
		case 0x2:
			item->type=type_highway_city;
			break;
		case 0x1:
			item->type=type_highway_land;
			break;
		default:
			item->type=type_street_unkn;
			dbg(0,"unknown type 0x%x\n",street->str->type);
		}
		flags=item_get_default_flags(item->type);
		if (flags)
			street->flags=*flags;
		else
			street->flags=0;
		if (street->str->type & 0x40) {
			street->flags|=(street->str->limit & 0x30) ? AF_ONEWAYREV:0;
			street->flags|=(street->str->limit & 0x03) ? AF_ONEWAY:0;
		} else {
			street->flags|=(street->str->limit & 0x30) ? AF_ONEWAY:0;
			street->flags|=(street->str->limit & 0x03) ? AF_ONEWAYREV:0;
		}
#if 0
		coord_debug=(street->str->unknown2 != 0x40 || street->str->unknown3 != 0x40);
		if (coord_debug) {
			item->type=type_street_unkn;
			printf("%d %02x %02x %02x %02x\n", street->str->segid, street->str->type, street->str->limit, street->str->unknown2, street->str->unknown3);
		}
#endif
		street->p_rewind=street->p;
		street->name.len=0;
		street->attr_next=attr_label;
		street->more=1;
		if (!map_selection_contains_item(mr->cur_sel, 0, item->type)) 
			continue;
		return 1;
	}
}

int
street_get_byid(struct map_rect_priv *mr, struct street_priv *street, int id_hi, int id_lo, struct item *item)
{
        int country=id_hi & 0xffff;
        int res;
	dbg(1,"enter(%p,%p,0x%x,0x%x,%p)\n", mr, street, id_hi, id_lo, item);
	if (! country)
		return 0;
        tree_search_hv(mr->m->dirname, "street", (id_lo >> 8) | (country << 24), id_lo & 0xff, &res);
	dbg(1,"res=0x%x (blk=0x%x)\n", res, res >> 12);
        block_get_byindex(mr->m->file[mr->current_file], res >> 12, &mr->b);
	street_get_data(street, &mr->b.p);
	street->name_file=mr->m->file[file_strname_stn];
	street->end=mr->b.end;
	street->ref=&mr->b.b->r.lu;
	street->bytes=street_get_bytes(&mr->b.b->r);
	street->str_start=street->str=(struct street_str *)mr->b.p;
	street->coord_begin=mr->b.p;
	street_coord_get_begin(&street->coord_begin);
	street->p=street->coord_begin;
	street->type--;
	item->meth=&street_meth;
	item->priv_data=street;
	street->str+=(res & 0xfff)-1;
	dbg(1,"segid 0x%x\n", L(street->str[1].segid));
	return street_get(mr, street, item);
#if 0
        mr->b.p=mr->b.block_start+(res & 0xffff);
        return town_get(mr, twn, item);
#endif

	return 0;
}
     

struct street_name_index {
	int block;
        unsigned short country;
        int town_assoc;
        char name[0];
} __attribute__((packed));

static unsigned char 
latin1_tolower(unsigned char c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 'a';
	if (c == 0xc4 || c == 0xc9 || c == 0xd6 || c == 0xdc)
		return c+0x20;
	return c;
}

static unsigned char
latin1_tolower_ascii(unsigned char c)
{
	unsigned char ret=latin1_tolower(c);
	dbg(0,"ret=0x%x\n",ret);
	switch (ret) {
	case 0xe4:
		return 'a';
	case 0xe9:
		return 'e';
	case 0xf6:
		return 'o';
	case 0xfc:
		return 'u';
	default:
		if (ret >= 0x80)
			dbg(0,"ret=0x%x\n",c);
		return ret;
	}
}

static int
strncasecmp_latin1(char *str1, char *str2, int len)
{
	int d;
	while (len--) {
		d=latin1_tolower((unsigned char)(*str1))-latin1_tolower((unsigned char)(*str2));
		if (d)
			return d;
		if (! *str1)
			return 0;
		str1++;
		str2++;
	}
	return 0;
}

static int
strncasecmp_latin1_ascii(char *str1, char *str2, int len)
{
	int d;
	while (len--) {
		d=latin1_tolower_ascii((unsigned char)(*str1))-latin1_tolower_ascii((unsigned char)(*str2));
		if (d)
			return d;
		if (! *str1)
			return 0;
		str1++;
		str2++;
	}
	return 0;
}

static int
street_search_compare_do(struct map_rect_priv *mr, int country, int town_assoc, char *name)
{
        int d;

	dbg(1,"enter");
	dbg(1,"country 0x%x town_assoc 0x%x name '%s'\n", country, town_assoc, name);
	d=(mr->search_item.id_hi & 0xffff)-country;
	dbg(1,"country %d (%d vs %d)\n", d, mr->search_item.id_hi & 0xffff, country);
	if (!d) {
		if (mr->search_item.id_lo == town_assoc ) {
			dbg(1,"town_assoc match (0x%x)\n", town_assoc);
			if (mr->search_partial)
				d=strncasecmp_latin1_ascii(mr->search_str, name, strlen(mr->search_str));
			else
				d=strncasecmp_latin1_ascii(mr->search_str, name, INT_MAX);
			dbg(1,"string %d\n", d);
		} else {
			if (town_assoc < mr->search_item.id_lo)
				d=1;
			else
				d=-1;
			dbg(1,"assoc %d 0x%x-0x%x\n",d, mr->search_item.id_lo, town_assoc);
		}
	}
	dbg(1,"d=%d\n", d);
	return d;	
}

static int
street_search_compare(unsigned char **p, struct map_rect_priv *mr)
{
	struct street_name_index *i;
	int ret;

	dbg(1,"enter\n");
	i=(struct street_name_index *)(*p);
	*p+=sizeof(*i)+strlen(i->name)+1;

	dbg(1,"block 0x%x\n", i->block);
	
	ret=street_search_compare_do(mr, i->country, i->town_assoc, i->name);
	if (ret <= 0)
		mr->search_block=i->block;
	return ret;
}

static void
street_name_numbers_coord_rewind(void *priv_data)
{
	/* struct street_priv *street=priv_data; */

}

static void
street_name_numbers_attr_rewind(void *priv_data)
{
	/* struct street_priv *street=priv_data; */

}

static int
street_name_numbers_coord_get(void *priv_data, struct coord *c, int count)
{
	return 0;
}

static int
street_name_numbers_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	attr->type=attr_type;
	switch (attr_type) {
	default:
		dbg(0,"unknown item\n");
		return 0;
	}
}





static struct item_methods street_name_numbers_meth = {
	street_name_numbers_coord_rewind,
	street_name_numbers_coord_get,
	street_name_numbers_attr_rewind,
	street_name_numbers_attr_get,
};


static void
street_name_coord_rewind(void *priv_data)
{
	/* struct street_priv *street=priv_data; */

}

static void
street_name_attr_rewind(void *priv_data)
{
	/* struct street_priv *street=priv_data; */

}

static int
street_name_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *mr=priv_data;
	struct street_name_numbers snns;
	unsigned char *p=mr->street.name.aux_data;

	dbg(1,"aux_data=%p\n", p);
	if (count) {
		street_name_numbers_get(&snns, &p);
		street_name_numbers_get_coord(&snns, c);
		return 1;
	}
	
	return 0;
}

static int
street_name_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct map_rect_priv *mr=priv_data;
	struct item *item;

	attr->type=attr_type;
	switch (attr_type) {
	case attr_street_name:
		attr->u.str=mr->street.name.name2;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_street_name_systematic:
		attr->u.str=mr->street.name.name1;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_street_name_numbers_item:
		item=&mr->item3.item;
		attr->u.item=item;
		item->type=type_street_name_numbers;
		item->id_hi=0;
		item->id_lo=1;
		item->meth=&street_name_numbers_meth;
		item->map=NULL;
		item->priv_data=mr;
		{
			int i;
			struct street_name_numbers nns;
			unsigned char *p=mr->street.name.aux_data;
			unsigned char *end=p+mr->street.name.aux_len;
			printf("len=0x%x\n", mr->street.name.aux_len);
			for (i = 0 ; i < mr->street.name.aux_len ; i++) {
				printf("%02x ",mr->street.name.aux_data[i]);
			}
			printf("\n");
			{
				while (p < end) {
					unsigned char *pn,*pn_end;
					struct street_name_number nn;
					street_name_numbers_get(&nns, &p);
					printf("name_numbers:\n");
					printf("  len 0x%x\n", nns.len);
					printf("  tag 0x%x\n", nns.tag);
					printf("  dist 0x%x\n", nns.dist);
					printf("  country 0x%x\n", nns.country);
					printf("  coord 0x%x,0x%x\n", nns.c->x, nns.c->y);
					printf("  first %d\n", nns.first);
					printf("  last %d\n", nns.last);
					printf("  segment count 0x%x\n", nns.segment_count);
					printf("  aux_len 0x%x\n", nns.aux_len);
					pn=nns.aux_data;
					pn_end=nns.aux_data+nns.aux_len;
					while (pn < pn_end) {
						printf("  number:\n");
						street_name_number_get(&nn, &pn);
						printf("    len 0x%x\n", nn.len);
						printf("    tag 0x%x\n", nn.tag);
						printf("    coord 0x%x,0x%x\n", nn.c->x, nn.c->y);
						printf("    first %d\n", nn.first);
						printf("    last %d\n", nn.last);
					}
				}
			}
		}
		return 1;
	default:
		dbg(0,"unknown item\n");
		return 0;
	}
}





static struct item_methods street_name_meth = {
	street_name_coord_rewind,
	street_name_coord_get,
	street_name_attr_rewind,
	street_name_attr_get,
};


struct item *
street_search_get_item(struct map_rect_priv *mr)
{
	int dir=1,leaf;
	unsigned char *last;

	dbg(1,"enter\n");
	if (! mr->search_blk_count) {
		dbg(1,"partial 0x%x '%s' ***\n", mr->town.street_assoc, mr->search_str);
		if (mr->search_linear)
			return NULL;
		dbg(1,"tree_search_next\n");
		mr->search_block=-1;
		while ((leaf=tree_search_next(&mr->ts, &mr->search_p, dir)) != -1) {
			dir=street_search_compare(&mr->search_p, mr);
		}
		dbg(1,"dir=%d mr->search_block=0x%x\n", dir, mr->search_block);
		if (mr->search_block == -1)
			return NULL;
		mr->search_blk_count=1;
		block_get_byindex(mr->m->file[file_strname_stn], mr->search_block, &mr->b);
		mr->b.p=mr->b.block_start+12;
	}
	dbg(1,"name id 0x%x\n", mr->b.p-mr->m->file[file_strname_stn]->begin);
	if (! mr->search_blk_count)
		return NULL;
	for (;;) {
		if (mr->b.p >= mr->b.end) {
			if (!block_next_lin(mr)) {
				dbg(1,"end of blocks in %p, %p\n", mr->m->file[file_strname_stn]->begin, mr->m->file[file_strname_stn]->end);
				return NULL;
			}
			mr->b.p=mr->b.block_start+12;
		}
		while (mr->b.p < mr->b.end) {
			last=mr->b.p;
			street_name_get(&mr->street.name, &mr->b.p);
			dir=street_search_compare_do(mr, mr->street.name.country, mr->street.name.townassoc, mr->street.name.name2);
			dbg(1,"country 0x%x assoc 0x%x name1 '%s' name2 '%s' dir=%d\n", mr->street.name.country, mr->street.name.townassoc, mr->street.name.name1, mr->street.name.name2, dir);
			if (dir < 0) {
				dbg(1,"end of data\n");
				mr->search_blk_count=0;
				return NULL;
			}
			if (!dir) {
				dbg(1,"result country 0x%x assoc 0x%x name1 '%s' name2 '%s' dir=%d aux_data=%p len=0x%x\n", mr->street.name.country, mr->street.name.townassoc, mr->street.name.name1, mr->street.name.name2, dir, mr->street.name.aux_data, mr->street.name.aux_len);
				mr->item.type = type_street_name;
				mr->item.id_hi=mr->street.name.country | (mr->current_file << 16) | 0x10000000;
				mr->item.id_lo=last-mr->m->file[mr->current_file]->begin;
				mr->item.meth=&street_name_meth;
				mr->item.map=NULL;
				mr->item.priv_data=mr;
				return &mr->item;
			}
		}
	}
}
 
