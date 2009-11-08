#include <string.h>
#include "maptool.h"
#include "debug.h"


struct item_bin *
read_item(FILE *in)
{
	struct item_bin *ib=(struct item_bin *) buffer;
	int r,s;
	r=fread(ib, sizeof(*ib), 1, in);
	if (r != 1)
		return NULL;
	bytes_read+=r;
	dbg_assert((ib->len+1)*4 < sizeof(buffer));
	s=(ib->len+1)*4-sizeof(*ib);
	r=fread(ib+1, s, 1, in);
	if (r != 1)
		return NULL;
	bytes_read+=r;
	return ib;
}

int
item_bin_read(struct item_bin *ib, FILE *in)
{
	if (fread(ib, 4, 1, in) == 0)
		return 0;
	if (!ib->len)
		return 1;
	if (fread((unsigned char *)ib+4, ib->len*4, 1, in))
		return 2;
	return 0;
}

void
item_bin_set_type(struct item_bin *ib, enum item_type type)
{
	ib->type=type;
}

void
item_bin_init(struct item_bin *ib, enum item_type type)
{
	ib->clen=0;
	ib->len=2;
	item_bin_set_type(ib, type);
}


void
item_bin_add_coord(struct item_bin *ib, struct coord *c, int count)
{
	struct coord *c2=(struct coord *)(ib+1);
	c2+=ib->clen/2;
	memcpy(c2, c, count*sizeof(struct coord));
	ib->clen+=count*2;
	ib->len+=count*2;
}

void
item_bin_bbox(struct item_bin *ib, struct rect *r)
{
	struct coord c;
	item_bin_add_coord(ib, &r->l, 1);
	c.x=r->h.x;
	c.y=r->l.y;
	item_bin_add_coord(ib, &c, 1);
	item_bin_add_coord(ib, &r->h, 1);
	c.x=r->l.x;
	c.y=r->h.y;
	item_bin_add_coord(ib, &c, 1);
	item_bin_add_coord(ib, &r->l, 1);
}

void
item_bin_copy_coord(struct item_bin *ib, struct item_bin *from, int dir)
{
	struct coord *c=(struct coord *)(from+1);
	int i,count=from->clen/2;
	if (dir >= 0) {
		item_bin_add_coord(ib, c, count);
		return;
	}
	for (i = 1 ; i <= count ; i++) 
		item_bin_add_coord(ib, &c[count-i], 1);
}

void
item_bin_add_coord_rect(struct item_bin *ib, struct rect *r)
{
	item_bin_add_coord(ib, &r->l, 1);
	item_bin_add_coord(ib, &r->h, 1);
}

void
item_bin_add_attr_data(struct item_bin *ib, enum attr_type type, void *data, int size)
{
	struct attr_bin *ab=(struct attr_bin *)((int *)ib+ib->len+1);
	int pad=(4-(size%4))%4;
	ab->type=type;
	memcpy(ab+1, data, size);
	memset((unsigned char *)(ab+1)+size, 0, pad);
	ab->len=(size+pad)/4+1;
	ib->len+=ab->len+1;
}

void
item_bin_add_attr(struct item_bin *ib, struct attr *attr)
{
	item_bin_add_attr_data(ib, attr->type, attr_data_get(attr), attr_data_size(attr));
}

void
item_bin_add_attr_int(struct item_bin *ib, enum attr_type type, int val)
{
	struct attr attr;
	attr.type=type;
	attr.u.num=val;
	item_bin_add_attr(ib, &attr);
}

void *
item_bin_get_attr(struct item_bin *ib, enum attr_type type, void *last)
{
	unsigned char *s=(unsigned char *)ib;
	unsigned char *e=s+(ib->len+1)*4;
	s+=sizeof(struct item_bin)+ib->clen*4;
	while (s < e) {
		struct attr_bin *ab=(struct attr_bin *)s;
		s+=(ab->len+1)*4;
		if (ab->type == type && (void *)(ab+1) > last) {
			return (ab+1);
		}
	}
	return NULL;
}

void
item_bin_add_attr_longlong(struct item_bin *ib, enum attr_type type, long long val)
{
	struct attr attr;
	attr.type=type;
	attr.u.num64=&val;
	item_bin_add_attr(ib, &attr);
}

void
item_bin_add_attr_string(struct item_bin *ib, enum attr_type type, char *str)
{
	struct attr attr;
	if (! str)
		return;
	attr.type=type;
	attr.u.str=str;
	item_bin_add_attr(ib, &attr);
}

void
item_bin_add_attr_range(struct item_bin *ib, enum attr_type type, short min, short max)
{
	struct attr attr;
	attr.type=type;
	attr.u.range.min=min;
	attr.u.range.max=max;
	item_bin_add_attr(ib, &attr);
}

void
item_bin_write(struct item_bin *ib, FILE *out)
{
	fwrite(ib, (ib->len+1)*4, 1, out);
}

void
item_bin_write_clipped(struct item_bin *ib, struct tile_parameter *param, struct item_bin_sink *out)
{
	struct tile_data tile_data;
	int i;
	bbox((struct coord *)(ib+1), ib->clen/2, &tile_data.item_bbox);
	tile_data.buffer[0]='\0';
	tile_data.tile_depth=tile(&tile_data.item_bbox, NULL, tile_data.buffer, param->max, param->overlap, &tile_data.tile_bbox);
	if (tile_data.tile_depth == param->max || tile_data.tile_depth >= param->min) {
		item_bin_write_to_sink(ib, out, &tile_data);
		return;
	}
	for (i = 0 ; i < 4 ; i++) {
		struct rect clip_rect;
		tile_data.buffer[tile_data.tile_depth]='a'+i;
		tile_data.buffer[tile_data.tile_depth+1]='\0';
		tile_bbox(tile_data.buffer, &clip_rect, param->overlap);
		if (ib->type < type_area)
			clip_line(ib, &clip_rect, param, out);
		else
			clip_polygon(ib, &clip_rect, param, out);
	}
}

static char *
coord_to_str(struct coord *c)
{
	int x=c->x;
	int y=c->y;
	char *sx="";
	char *sy="";
	if (x < 0) {
		sx="-";
		x=-x;
	}
	if (y < 0) {
		sy="-";
		y=-y;
	}
	return g_strdup_printf("%s0x%x %s0x%x",sx,x,sy,y);
}

static void
dump_coord(struct coord *c, FILE *out)
{
	char *str=coord_to_str(c);
	fprintf(out,"%s",str);
	g_free(str);
}


void
item_bin_dump(struct item_bin *ib, FILE *out)
{
	struct coord *c;
	struct attr_bin *a;
	struct attr attr;
	int *attr_start;
	int *attr_end;
	int i;
	char *str;

	c=(struct coord *)(ib+1);
	if (ib->type < type_line) {
		dump_coord(c,out);
		fprintf(out, " ");
	}
	attr_start=(int *)(ib+1)+ib->clen;
	attr_end=(int *)ib+ib->len+1;
	fprintf(out,"type=%s", item_to_name(ib->type));
	while (attr_start < attr_end) {
		a=(struct attr_bin *)(attr_start);
		attr_start+=a->len+1;
		attr.type=a->type;
		attr_data_set(&attr, (a+1));
		str=attr_to_text(&attr, NULL, 1);
		fprintf(out," %s=\"%s\"", attr_to_name(a->type), str);
		g_free(str);
	}
	fprintf(out," debug=\"length=%d\"", ib->len);
	fprintf(out,"\n");
	if (ib->type >= type_line) {
		for (i = 0 ; i < ib->clen/2 ; i++) {
			dump_coord(c+i,out);
			fprintf(out,"\n");
		}
	}
}

void
dump_itembin(struct item_bin *ib)
{
	item_bin_dump(ib, stdout);
}

