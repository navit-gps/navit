#include "debug.h"
#include "mg.h"

static void
poly_coord_rewind(void *priv_data)
{
	struct poly_priv *poly=priv_data;

	poly->p=poly->subpoly_start;	

}

static int
poly_coord_get(void *priv_data, struct coord *c, int count)
{
	struct poly_priv *poly=priv_data;
	int ret=0;

	while (count--) {
		if (poly->p >= poly->subpoly_next)
			break;
		c->x=get_u32_unal(&poly->p);
		c->y=get_u32_unal(&poly->p);
		c++;
		ret++;
	}
	return ret;
}

static void 
poly_attr_rewind(void *priv_data)
{
	struct poly_priv *poly=priv_data;

	poly->aidx=0;
}

static int
poly_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct poly_priv *poly=priv_data;

	attr->type=attr_type;
	switch (attr_type) {
	case attr_any:
		while (poly->attr_next != attr_none) {
			if (poly_attr_get(poly, poly->attr_next, attr))
				return 1;
		}
		return 0;
	case attr_label:
                attr->u.str=poly->name;
                poly->attr_next=attr_none;
                if (attr->u.str[0])
                        return 1;
		return 0;
	default:
		return 0;
	}
	return 1;
}

static struct item_methods poly_meth = {
	poly_coord_rewind,
	poly_coord_get,
	poly_attr_rewind,
	poly_attr_get,
};

static void
poly_get_data(struct poly_priv *poly, unsigned char **p)
{
	poly->c[0].x=get_u32_unal(p);
	poly->c[0].y=get_u32_unal(p);
	poly->c[1].x=get_u32_unal(p);
	poly->c[1].y=get_u32_unal(p);
	*p+=sizeof(struct coord);
	poly->name=(char *)(*p);
	while (**p) {
		(*p)++;
	}
	(*p)++;
	poly->order=*(*p)++;
	poly->type=*(*p)++;
	poly->polys=get_u32_unal(p);
	poly->count=(unsigned int *)(*p); (*p)+=poly->polys*sizeof(unsigned int);
	poly->count_sum=get_u32_unal(p);
}

int
poly_get(struct map_rect_priv *mr, struct poly_priv *poly, struct item *item)
{
	struct coord_rect r;

        for (;;) {
                if (mr->b.p >= mr->b.end)
                        return 0;
		if (mr->b.p == mr->b.p_start) {
			poly->poly_num=0;
			poly->subpoly_num=0;
			poly->subpoly_num_all=0;
			poly->poly_next=mr->b.p;
			item->meth=&poly_meth;
		}
		if (poly->poly_num >= mr->b.b->count)
			return 0;
		if (!poly->subpoly_num) {
			mr->b.p=poly->poly_next;
			item->id_lo=mr->b.p-mr->file->begin;
			poly_get_data(poly, &mr->b.p);
			poly->poly_next=mr->b.p+poly->count_sum*sizeof(struct coord);
			poly->poly_num++;
			r.lu=poly->c[0];
			r.rl=poly->c[1];
			if (mr->cur_sel && (poly->order > mr->cur_sel->order[layer_poly]*3 || !coord_rect_overlap(&mr->cur_sel->u.c_rect, &r))) {
				poly->subpoly_num_all+=poly->polys;
				mr->b.p=poly->poly_next;
				continue;
			}
			switch(poly->type) {
			case 0x13:
				item->type=type_wood;
				break;
			case 0x14:
				item->type=type_town_poly;
				break;
			case 0x15:
				item->type=type_cemetery_poly;
				break;
			case 0x16:
				item->type=type_building_poly;
				break;
			case 0x17:
				item->type=type_museum_poly;
				break;
			case 0x19:
				item->type=type_place_poly;
				break;
			case 0x1b:
				item->type=type_commercial_center;
				break;
			case 0x1e:
				item->type=type_industry_poly;
				break;
			case 0x23:
				/* FIXME: what is this ?*/
				item->type=type_place_poly;
				break;
			case 0x24:
				item->type=type_parking_lot_poly;
				break;
			case 0x28:
				item->type=type_airport_poly;
				break;
			case 0x29:
				item->type=type_station_poly;
				break;
			case 0x2d:
				item->type=type_hospital_poly;
				break;
			case 0x2e:
				item->type=type_hospital_poly;
				break;
			case 0x2f:
				item->type=type_university;
				break;
			case 0x30:
				item->type=type_university;
				break;
			case 0x32:
				item->type=type_park_poly;
				break;
			case 0x34:
				item->type=type_sport_poly;
				break;
			case 0x35:
				item->type=type_sport_poly;
				break;
			case 0x37:
				item->type=type_golf_course;
				break;
			case 0x38:
				item->type=type_national_park;
				break;
			case 0x39:
				item->type=type_nature_park;
				break;
			case 0x3c:
				item->type=type_water_poly;
				break;
			case 0xbc:
				item->type=type_water_line;
				break;
			case 0xc3:
				/* FIXME: what is this ?*/
				item->type=type_border_state;
				break;
			case 0xc6:
				item->type=type_border_country;
				break;
			case 0xc7:
				item->type=type_border_state;
				break;
			case 0xd0:
				item->type=type_rail;
				break;
			default:
				dbg(0,"Unknown poly type 0x%x '%s' 0x%x,0x%x\n", poly->type,poly->name,r.lu.x,r.lu.y);
				item->type=type_street_unkn;
			}
		} else 
			mr->b.p=poly->subpoly_next;
		dbg(1,"%d %d %s\n", poly->subpoly_num_all, mr->b.block_num, poly->name);
		item->id_lo=poly->subpoly_num_all | (mr->b.block_num << 16);
		item->id_hi=(mr->current_file << 16);
		dbg(1,"0x%x 0x%x\n", item->id_lo, item->id_hi);
		poly->subpoly_next=mr->b.p+L(poly->count[poly->subpoly_num])*sizeof(struct coord);
		poly->subpoly_num++;
		poly->subpoly_num_all++;
		if (poly->subpoly_num >= poly->polys) 
			poly->subpoly_num=0;
		poly->subpoly_start=poly->p=mr->b.p;
		item->priv_data=poly;
		poly->attr_next=attr_label;
		return 1;
        }
}

int
poly_get_byid(struct map_rect_priv *mr, struct poly_priv *poly, int id_hi, int id_lo, struct item *item)
{
	int count=id_lo & 0xffff;
	int ret=0;
        block_get_byindex(mr->m->file[mr->current_file], id_lo >> 16, &mr->b);
	while (count-- >= 0) {
		ret=poly_get(mr, poly, item);
	}
	return ret;
}

