/*
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
#include <stdlib.h>
#include <assert.h>
#include "maptool.h"
#include "linguistics.h"
#include "file.h"
#include "debug.h"



int item_bin_read(struct item_bin *ib, FILE *in) {
    if (fread(ib, 4, 1, in) == 0)
        return 0;
    if (!ib->len)
        return 1;
    if (fread((unsigned char *)ib+4, ib->len*4, 1, in))
        return 2;
    return 0;
}

void item_bin_set_type(struct item_bin *ib, enum item_type type) {
    ib->type=type;
}

void item_bin_init(struct item_bin *ib, enum item_type type) {
    ib->clen=0;
    ib->len=2;
    item_bin_set_type(ib, type);
}


void item_bin_add_coord(struct item_bin *ib, struct coord *coords_to_add, int count) {
    struct coord *coord_list=(struct coord *)(ib+1);
    coord_list+=ib->clen/2;
    memcpy(coord_list, coords_to_add, count*sizeof(struct coord));
    ib->clen+=count*2;
    ib->len+=count*2;
}

void item_bin_add_coord_reverse(struct item_bin *ib, struct coord *c, int count) {
    int i;
    for (i = count-1 ; i >= 0 ; i--)
        item_bin_add_coord(ib, &c[i], 1);
}

void item_bin_bbox(struct item_bin *ib, struct rect *r) {
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

void item_bin_copy_coord(struct item_bin *ib, struct item_bin *from, int dir) {
    struct coord *c=(struct coord *)(from+1);
    int i,count=from->clen/2;
    if (dir >= 0) {
        item_bin_add_coord(ib, c, count);
        return;
    }
    for (i = 1 ; i <= count ; i++)
        item_bin_add_coord(ib, &c[count-i], 1);
}

void item_bin_copy_attr(struct item_bin *ib, struct item_bin *from, enum attr_type attr) {
    struct attr_bin *ab=item_bin_get_attr_bin(from, attr, NULL);
    if (ab)
        item_bin_add_attr_data(ib, ab->type, (void *)(ab+1), (ab->len-1)*4);
    assert(attr == attr_osm_wayid);
    assert(item_bin_get_wayid(ib) == item_bin_get_wayid(from));
}

void item_bin_add_coord_rect(struct item_bin *ib, struct rect *r) {
    item_bin_add_coord(ib, &r->l, 1);
    item_bin_add_coord(ib, &r->h, 1);
}

int attr_bin_write_data(struct attr_bin *ab, enum attr_type type, void *data, int size) {
    int pad=(4-(size%4))%4;
    ab->type=type;
    memcpy(ab+1, data, size);
    memset((unsigned char *)(ab+1)+size, 0, pad);
    ab->len=(size+pad)/4+1;
    return ab->len+1;
}

int attr_bin_write_attr(struct attr_bin *ab, struct attr *attr) {
    return attr_bin_write_data(ab, attr->type, attr_data_get(attr), attr_data_size(attr));
}

void item_bin_add_attr_data(struct item_bin *ib, enum attr_type type, void *data, int size) {
    struct attr_bin *ab=(struct attr_bin *)((int *)ib+ib->len+1);
    ib->len+=attr_bin_write_data(ab, type, data, size);
}

void item_bin_add_attr(struct item_bin *ib, struct attr *attr) {
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

void item_bin_remove_attr(struct item_bin *ib, void *ptr) {
    unsigned char *s=(unsigned char *)ib;
    unsigned char *e=s+(ib->len+1)*4;
    s+=sizeof(struct item_bin)+ib->clen*4;
    while (s < e) {
        struct attr_bin *ab=(struct attr_bin *)s;
        s+=(ab->len+1)*4;
        if ((void *)(ab+1) == ptr) {
            ib->len-=ab->len+1;
            memmove(ab,s,e-s);
            return;
        }
    }
}

void item_bin_add_attr_int(struct item_bin *ib, enum attr_type type, int val) {
    struct attr attr;
    attr.type=type;
    attr.u.num=val;
    item_bin_add_attr(ib, &attr);
}

void *item_bin_get_attr(struct item_bin *ib, enum attr_type type, void *last) {
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
item_bin_get_attr_bin(struct item_bin *ib, enum attr_type type, void *last) {
    unsigned char *s=(unsigned char *)ib;
    unsigned char *e=s+(ib->len+1)*4;
    s+=sizeof(struct item_bin)+ib->clen*4;
    while (s < e) {
        struct attr_bin *ab=(struct attr_bin *)s;
        s+=(ab->len+1)*4;
        if (ab->type == type && (void *)(ab+1) > last) {
            return ab;
        }
    }
    return NULL;
}

struct attr_bin *
item_bin_get_attr_bin_last(struct item_bin *ib) {
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

void item_bin_add_attr_longlong(struct item_bin *ib, enum attr_type type, long long val) {
    struct attr attr;
    attr.type=type;
    attr.u.num64=&val;
    item_bin_add_attr(ib, &attr);
}

void item_bin_add_attr_string(struct item_bin *ib, enum attr_type type, char *str) {
    struct attr attr;
    if (! str)
        return;
    attr.type=type;
    attr.u.str=str;
    item_bin_add_attr(ib, &attr);
}

void item_bin_add_attr_range(struct item_bin *ib, enum attr_type type, short min, short max) {
    struct attr attr;
    attr.type=type;
    attr.u.range.min=min;
    attr.u.range.max=max;
    item_bin_add_attr(ib, &attr);
}

/**
 * @brief add a "hole" to an item
 *
 * This function adds a "hole" (attr_poly_hole) to a map item. It adds the
 * coordinates and the coordinate count to the existing item.
 * WARNING: It does NOT allocate any memory, so the memory after the item
 * must be already allocated for that purpose.
 * @param[inout] ib item - to add hole to
 * @param[in] coord - hole coordinate array
 * @param[in] ccount - number of coordinates in coord
 */
void item_bin_add_hole(struct item_bin * ib, struct coord * coord, int ccount) {
    /* get space for next attr in buffer */
    int * buffer = ((int *) ib) + ib->len +1;
    /* get the attr heder in binary file */
    struct attr_bin * attr = (struct attr_bin *)buffer;
    /* fill header */
    attr->len = (ccount *2) + 2;
    attr->type = attr_poly_hole;
    /* get the first attr byte in buffer */
    buffer = (int *)(attr +1);
    /* for poly_hole, the first 4 bytes are the coordinate count */
    *buffer = ccount;
    /* coordinates are behind that */
    buffer ++;
    /* copy in the coordinates */
    memcpy(buffer,coord, ccount * sizeof(struct coord));
    /* add the hole to the total size */
    ib->len += attr->len +1;
}

void item_bin_write(struct item_bin *ib, FILE *out) {
    dbg_assert(fwrite(ib, (ib->len+1)*4, 1, out)==1);
}

struct item_bin *
item_bin_dup(struct item_bin *ib) {
    int len=(ib->len+1)*4;
    struct item_bin *ret=g_malloc(len);
    memcpy(ret, ib, len);

    return ret;
}

void item_bin_write_clipped(struct item_bin *ib, struct tile_parameter *param, struct item_bin_sink *out) {
    struct tile_data tile_data;
    int i;
    bbox((struct coord *)(ib+1), ib->clen/2, &tile_data.item_bbox);
    tile_data.buffer[0]='\0';
    tile_data.tile_depth=tile(&tile_data.item_bbox, NULL, tile_data.buffer, param->max, param->overlap,
                              &tile_data.tile_bbox);
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

static char *coord_to_str(struct coord *c) {
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

static void dump_coord(struct coord *c, FILE *out) {
    char *str=coord_to_str(c);
    fprintf(out,"%s",str);
    g_free(str);
}


void item_bin_dump(struct item_bin *ib, FILE *out) {
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

void dump_itembin(struct item_bin *ib) {
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

void item_bin_set_type_by_population(struct item_bin *ib, int population) {
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


void item_bin_write_match(struct item_bin *ib, enum attr_type type, enum attr_type match, int maxdepth, FILE *out) {
    char *word=item_bin_get_attr(ib, type, NULL);
    int i,words=0,len;
    char tilename[32]="";

    if (!word)
        return;

    if(maxdepth && ib->clen>0) {
        struct rect r;
        struct coord *c=(struct coord *)(ib+1);
        r.l=c[0];
        r.h=c[0];
        for (i = 1 ; i < ib->clen/2 ; i++)
            bbox_extend(&c[i], &r);
        tile(&r,NULL,tilename,maxdepth,overlap,NULL);
    }

    /* insert attr_tile_name attribute before the attribute used as alphabetical key (of type type) */
    if(maxdepth) {
        item_bin_add_attr_string(ib, attr_tile_name, tilename);
        item_bin_add_attr_string(ib, type, word);
        item_bin_remove_attr(ib,word);
        word=item_bin_get_attr(ib, type, NULL);
    }
    len=ib->len;
    do  {
        if (linguistics_search(word)) {
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
            words++;
        }
        word=linguistics_next_word(word);
    } while (word);
}

static int item_bin_sort_compare(const void *p1, const void *p2) {
    struct item_bin *ib1=*((struct item_bin **)p1),*ib2=*((struct item_bin **)p2);
    struct attr_bin *attr1,*attr2;
    char *s1,*s2;
    int ret;

    attr1=item_bin_get_attr_bin(ib1, attr_tile_name, NULL);
    attr2=item_bin_get_attr_bin(ib2, attr_tile_name, NULL);
    if(attr1&&attr2) {
        s1=(char *)(attr1+1);
        s2=(char *)(attr2+1);
        ret=g_strcmp0(s1,s2);
        if(ret)
            return ret;
    }
    attr1=item_bin_get_attr_bin_last(ib1);
    attr2=item_bin_get_attr_bin_last(ib2);
    s1=(char *)(attr1+1);
    s2=(char *)(attr2+1);
    if (attr1->type == attr_house_number && attr2->type == attr_house_number) {
        ret=atoi(s1)-atoi(s2);
        if (ret)
            return ret;
    }

    s1=linguistics_casefold(s1);
    s2=linguistics_casefold(s2);

    ret=g_strcmp0(s1, s2);
    g_free(s1);
    g_free(s2);

    if (!ret) {
        int match1=0,match2=0;
        match1=(attr1->type == attr_town_name_match || attr1->type == attr_district_name_match);
        match2=(attr2->type == attr_town_name_match || attr2->type == attr_district_name_match);
        ret=match1-match2;
    }
    return ret;
}

int item_bin_sort_file(char *in_file, char *out_file, struct rect *r, int *size) {
    int j,k,count,rc=0;
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
        idx=g_malloc(count*sizeof(void *));
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
            dbg_assert(fwrite(ib, (ib->len+1)*4, 1, f)==1);
            if (r) {
                for (k = 0 ; k < ib->clen/2 ; k++) {
                    if (rc)
                        bbox_extend(&c[k], r);
                    else {
                        r->l=c[k];
                        r->h=c[k];
                    }
                    rc++;
                }
            }
        }
        fclose(f);
        g_free(idx);
        g_free(buffer);
        return 1;
    }
    return 0;
}

struct geom_poly_segment *
item_bin_to_poly_segment(struct item_bin *ib, int type) {
    struct geom_poly_segment *ret=g_new(struct geom_poly_segment, 1);
    int count=ib->clen*sizeof(int)/sizeof(struct coord);
    ret->type=type;
    ret->first=g_new(struct coord, count);
    ret->last=ret->first+count-1;
    geom_coord_copy((struct coord *)(ib+1), ret->first, count, 0);
    return ret;
}

void clip_line(struct item_bin *ib, struct rect *r, struct tile_parameter *param, struct item_bin_sink *out) {
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
            code=geom_clip_line_code(&p1, &p2, r);
#if 1
            if (((code == 1 || code == 5) && ib_new->clen == 0) || (code & 2)) {
                item_bin_add_coord(ib_new, &p1, 1);
            }
            if (code) {
                item_bin_add_coord(ib_new, &p2, 1);
            }
            if (i == count-1 || (code & 4)) {
                if (param->attr_to_copy)
                    item_bin_copy_attr(ib_new, ib, param->attr_to_copy);
                if (ib_new->clen)
                    item_bin_write_clipped(ib_new, param, out);
                item_bin_init(ib_new, ib->type);
            }
#else
            if (code) {
                item_bin_init(ib_new, ib->type);
                item_bin_add_coord(ib_new, &p1, 1);
                item_bin_add_coord(ib_new, &p2, 1);
                if (param->attr_to_copy)
                    item_bin_copy_attr(ib_new, ib, param->attr_to_copy);
                item_bin_write_clipped(ib_new, param, out);
            }
#endif
        }
    }
}

void clip_polygon(struct item_bin *ib, struct rect *r, struct tile_parameter *param, struct item_bin_sink *out) {
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
            if (geom_is_inside(p, r, edge)) {
                if (! geom_is_inside(s, r, edge)) {
                    geom_poly_intersection(s,p,r,edge,&pi);
                    item_bin_add_coord(ib_out, &pi, 1);
                }
                item_bin_add_coord(ib_out, p, 1);
            } else {
                if (geom_is_inside(s, r, edge)) {
                    geom_poly_intersection(p,s,r,edge,&pi);
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
    if (ib_in->clen) {
        if (param->attr_to_copy)
            item_bin_copy_attr(ib_in, ib, param->attr_to_copy);
        item_bin_write_clipped(ib_in, param, out);
    }
}

