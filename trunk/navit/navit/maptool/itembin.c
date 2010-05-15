#include <string.h>
#include <stdlib.h>
#include "maptool.h"
#include "linguistics.h"
#include "file.h"
#include "debug.h"



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

int
attr_bin_write_data(struct attr_bin *ab, enum attr_type type, void *data, int size)
{
       int pad=(4-(size%4))%4;
       ab->type=type;
       memcpy(ab+1, data, size);
       memset((unsigned char *)(ab+1)+size, 0, pad);
       ab->len=(size+pad)/4+1;
       return ab->len+1;
}

int
attr_bin_write_attr(struct attr_bin *ab, struct attr *attr)
{
	return attr_bin_write_data(ab, attr->type, attr_data_get(attr), attr_data_size(attr));
}

void
item_bin_add_attr_data(struct item_bin *ib, enum attr_type type, void *data, int size)
{
	struct attr_bin *ab=(struct attr_bin *)((int *)ib+ib->len+1);
	ib->len+=attr_bin_write_data(ab, type, data, size);
}

void
item_bin_add_attr(struct item_bin *ib, struct attr *attr)
{
	struct attr_bin *ab=(struct attr_bin *)((int *)ib+ib->len+1);
	if (ATTR_IS_GROUP(attr->type)) {
		int i=0;
		int *abptr;
		ab->type=attr->type;
		ab->len=1;
		abptr=(int *)(ab+1);
                while (attr->u.attrs[i].type) {
                        int size=attr_bin_write_attr((struct attr_bin *)abptr, &attr->u.attrs[i]);
                        ab->len+=size;
                        abptr+=size;
                        i++;
                }
                ib->len+=ab->len+1;

	} else
		ib->len+=attr_bin_write_attr(ab, attr);
	
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

struct attr_bin *
item_bin_get_attr_bin_last(struct item_bin *ib)
{
	struct attr_bin *ab=NULL;
	unsigned char *s=(unsigned char *)ib;
	unsigned char *e=s+(ib->len+1)*4;
	s+=sizeof(struct item_bin)+ib->clen*4;
	while (s < e) {
		ab=(struct attr_bin *)s;
		s+=(ab->len+1)*4;
	}
	return ab;
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

struct item_bin *
item_bin_dup(struct item_bin *ib)
{
	int len=(ib->len+1)*4;
	struct item_bin *ret=g_malloc(len);
	memcpy(ret, ib, len);

	return ret;
}

void
item_bin_write_range(struct item_bin *ib, FILE *out, int min, int max)
{
	struct range r;

	r.min=min;
	r.max=max;
	fwrite(&r, sizeof(r), 1, out);
	item_bin_write(ib, out);
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

struct population_table {
	enum item_type type;
	int population;
};

static struct population_table town_population[] = {
	{type_town_label_0e0,0},
	{type_town_label_1e0,1},
	{type_town_label_2e0,2},
	{type_town_label_5e0,5},
	{type_town_label_1e1,10},
	{type_town_label_2e1,20},
	{type_town_label_5e1,50},
	{type_town_label_1e2,100},
	{type_town_label_2e2,200},
	{type_town_label_5e2,500},
	{type_town_label_1e3,1000},
	{type_town_label_2e3,2000},
	{type_town_label_5e3,5000},
	{type_town_label_1e4,10000},
	{type_town_label_2e4,20000},
	{type_town_label_5e4,50000},
	{type_town_label_1e5,100000},
	{type_town_label_2e5,200000},
	{type_town_label_5e5,500000},
	{type_town_label_1e6,1000000},
	{type_town_label_2e6,2000000},
	{type_town_label_5e6,5000000},
	{type_town_label_1e7,10000000},
};

static struct population_table district_population[] = {
	{type_district_label_0e0,0},
	{type_district_label_1e0,1},
	{type_district_label_2e0,2},
	{type_district_label_5e0,5},
	{type_district_label_1e1,10},
	{type_district_label_2e1,20},
	{type_district_label_5e1,50},
	{type_district_label_1e2,100},
	{type_district_label_2e2,200},
	{type_district_label_5e2,500},
	{type_district_label_1e3,1000},
	{type_district_label_2e3,2000},
	{type_district_label_5e3,5000},
	{type_district_label_1e4,10000},
	{type_district_label_2e4,20000},
	{type_district_label_5e4,50000},
	{type_district_label_1e5,100000},
	{type_district_label_2e5,200000},
	{type_district_label_5e5,500000},
	{type_district_label_1e6,1000000},
	{type_district_label_2e6,2000000},
	{type_district_label_5e6,5000000},
	{type_district_label_1e7,10000000},
};

void
item_bin_set_type_by_population(struct item_bin *ib, int population)
{
	struct population_table *table;
	int i,count;

	if (population < 0)
		population=0;
	if (item_is_district(*ib)) {
		table=district_population;
		count=sizeof(district_population)/sizeof(district_population[0]);
	} else {
		table=town_population;
		count=sizeof(town_population)/sizeof(town_population[0]);
	}
	for (i = 0 ; i < count ; i++) {
		if (population < table[i].population)
			break;
	}
	item_bin_set_type(ib, table[i-1].type);
}


void
item_bin_write_match(struct item_bin *ib, enum attr_type type, enum attr_type match, FILE *out)
{
	char *word=item_bin_get_attr(ib, type, NULL);
	int i,words=0,len=ib->len;
	if (!word)
		return;
	do  {
		for (i = 0 ; i < 3 ; i++) {
				char *str=linguistics_expand_special(word, i);
				if (str) {
					ib->len=len;
					if (i || words)
						item_bin_add_attr_string(ib, match, str);
					item_bin_write(ib, out);
					g_free(str);
				}
			}
			word=linguistics_next_word(word);
			words++;
	} while (word);
}

static int
item_bin_sort_compare(const void *p1, const void *p2)
{
	struct item_bin *ib1=*((struct item_bin **)p1),*ib2=*((struct item_bin **)p2);
	struct attr_bin *attr1,*attr2;
	char *s1,*s2;
	int ret;
#if 0
	dbg_assert(ib1->clen==2);
	dbg_assert(ib2->clen==2);
	attr1=(struct attr_bin *)((int *)(ib1+1)+ib1->clen);
	attr2=(struct attr_bin *)((int *)(ib2+1)+ib1->clen);
#else
	attr1=item_bin_get_attr_bin_last(ib1);
	attr2=item_bin_get_attr_bin_last(ib2);
#endif
#if 0
	dbg_assert(attr1->type == attr_town_name || attr1->type == attr_town_name_match);
	dbg_assert(attr2->type == attr_town_name || attr2->type == attr_town_name_match);
#endif
	s1=(char *)(attr1+1);
	s2=(char *)(attr2+1);
	if (attr1->type == attr_house_number && attr2->type == attr_house_number) {
		ret=atoi(s1)-atoi(s2);
		if (ret)
			return ret;
	}
	ret=strcmp(s1, s2);
	if (!ret) {
		int match1=0,match2=0;
		match1=(attr1->type == attr_town_name_match || attr1->type == attr_district_name_match);
		match2=(attr2->type == attr_town_name_match || attr2->type == attr_district_name_match);
		ret=match1-match2;
	}
#if 0
	fprintf(stderr,"sort_countries_compare p1=%p p2=%p %s %s\n",p1,p2,s1,s2);
#endif
	return ret;
}

int
item_bin_sort_file(char *in_file, char *out_file, struct rect *r, int *size)
{
	int j,count;
	struct coord *c;
	struct item_bin *ib;
	FILE *f;
	unsigned char *p,**idx,*buffer;
	if (file_get_contents(in_file, &buffer, size)) {
		ib=(struct item_bin *)buffer;
		p=buffer;
		count=0;
		while (p < buffer+*size) {
			count++;
			p+=(*((int *)p)+1)*4;
		}
		idx=malloc(count*sizeof(void *));
		dbg_assert(idx != NULL);
		p=buffer;
		for (j = 0 ; j < count ; j++) {
			idx[j]=p;
			p+=(*((int *)p)+1)*4;
		}
		qsort(idx, count, sizeof(void *), item_bin_sort_compare);
		f=fopen(out_file,"wb");
		for (j = 0 ; j < count ; j++) {
			ib=(struct item_bin *)(idx[j]);
			c=(struct coord *)(ib+1);
			fwrite(ib, (ib->len+1)*4, 1, f);
			if (r) {
				if (j) 
					bbox_extend(c, r);
				else {
					r->l=*c;
					r->h=*c;
				}
			}
		}
		fclose(f);
		return 1;
	}
	return 0;
}
