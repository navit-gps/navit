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

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "plugin.h"
#include "data.h"
#include "projection.h"
#include "map.h"
#include "maptype.h"
#include "item.h"
#include "attr.h"
#include "coord.h"
#include "transform.h"
#include <stdio.h>
#include "attr.h"
#include "coord.h"

struct file {
    FILE *f;
    int offset;
};


int shift=5;
int subdiv_next=0x10;


static void *file_read(struct file *f, int offset, int size) {
    void *ptr;
    int ret;

    ptr=calloc(size, 1);
    if (! ptr)
        return ptr;
    fseek(f->f, f->offset+offset, SEEK_SET);
    ret=fread(ptr, size, 1, f->f);
    if (ret != 1) {
        printf("fread %d vs %d offset %d+%d(0x%x)\n", ret, size, f->offset, offset,offset);
        g_assert(1==0);
    }
    return ptr;
}

static void file_free(void *ptr) {
    free(ptr);
}

struct offset_len {
    int offset;
    int length;
} __attribute ((packed));

static void dump_offset_len(struct offset_len *off_len) {
    printf("offset: 0x%x(%d) length 0x%x(%d)\n", off_len->offset, off_len->offset, off_len->length, off_len->length);
}

struct timestamp {
    short creation_year;
    char creation_month;
    char creation_day;
    char creation_hour;
    char creation_minute;
    char creation_second;
} __attribute__((packed));

struct img_header {
    char xor;
    char zero1[9];
    char update_month;
    char update_year;
    char zero2[3];
    char checksum[1];
    char signature[7];
    char unknown1[1];
    char unknown2[2];
    char unknown3[2];
    char unknown4[2];
    char unknown5[2];
    char zero3[25];
    struct timestamp ts;
    char unknown6;
    char map_file_identifier[7];
    char unknown12;
    char map_description1[20];
    short unknown13;
    short unknown14;
    char e1;
    char e2;
    char other[413];
    char zero4[512];
    char unknown7;
    char unknown8[11];
    int file_offset;
    char unknown9;
    char unknown10[15];
    char unknown11[480];
} __attribute__((packed));

static void dump_ts(struct timestamp *ts) {
    printf("%d-%02d-%02d %02d:%02d:%02d\n", ts->creation_year, ts->creation_month, ts->creation_day, ts->creation_hour,
           ts->creation_minute, ts->creation_second);
}

#if 0
static void dump_img(struct img_header *img_hdr) {
    printf("signature: '%s'\n", img_hdr->signature);
    printf("creation: ");
    dump_ts(&img_hdr->ts);
    printf("map_file_identifier: '%s'\n", img_hdr->map_file_identifier);
    printf("file_offset: 0x%x\n", img_hdr->file_offset);
    printf("e1: 0x%x(%d)\n", img_hdr->e1, img_hdr->e1);
    printf("e2: 0x%x(%d)\n", img_hdr->e2, img_hdr->e2);
    printf("offset 0x%x\n", (int) &img_hdr->e1 - (int) img_hdr);
    printf("size %d\n", sizeof(*img_hdr));
}
#endif

struct fat_block {
    char flag;
    char filename[8];
    char type[3];
    int size;
    char zero1;
    char part;
    char zero[14];
    unsigned short blocks[240];
} __attribute__((packed));

#if 0
static void dump_fat_block(struct fat_block *fat_blk) {
    int i=0;
    char name[9];
    char type[4];
    printf("flag: 0x%x(%d)\n", fat_blk->flag, fat_blk->flag);
    strcpy(name, fat_blk->filename);
    name[8]='\0';
    strcpy(type, fat_blk->type);
    type[3]='\0';
    printf("name: '%s.%s'\n", name, type);
    printf("size: 0x%x(%d)\n", fat_blk->size, fat_blk->size);
    printf("part: 0x%x(%d)\n", fat_blk->part, fat_blk->part);
    printf("blocks: ");
    while (i < 240) {
        printf("0x%x(%d) ",fat_blk->blocks[i], fat_blk->blocks[i]);
        if (fat_blk->blocks[i] == 0xffff)
            break;
        i++;
    }
    printf("size: %d\n", sizeof(*fat_blk));

}
#endif

struct file_header {
    short header_len;
    char type[10];
    char unknown1;
    char unknown2;
    struct timestamp ts;
} __attribute__((packed));

static void dump_file(struct file_header *fil_hdr) {
    printf("header_len: %d\n", fil_hdr->header_len);
    printf("type: '%s'\n", fil_hdr->type);
    printf("unknown1: 0x%x(%d)\n", fil_hdr->unknown1, fil_hdr->unknown1);
    printf("unknown2: 0x%x(%d)\n", fil_hdr->unknown2, fil_hdr->unknown2);
    printf("creation: ");
    dump_ts(&fil_hdr->ts);
    printf("size %d\n", sizeof(*fil_hdr));
}

struct region_header {
    struct file_header fil_hdr;
    struct offset_len offset_len;
} __attribute__((packed));

#if 0
static void dump_region(struct region_header *rgn_hdr) {
    dump_offset_len(&rgn_hdr->offset_len);
}
#endif

struct map_priv {
    int id;
    char *filename;
};

struct map_rect_priv {
    struct coord_rect r;
    int limit;

    struct file tre;
    struct tree_header *tre_hdr;
    struct file rgn;
    struct region_header *rgn_hdr;
    struct file lbl;
    struct label_header *lbl_hdr;
    char *label;

    int subdiv_level_count;
    int subdiv_pos;
    char *subdiv;

    int rgn_offset;
    int rgn_end;
    struct rgn_point *pnt;
    struct rgn_poly *ply;
    unsigned char *ply_data;
    int ply_bitpos;
    int ply_bitcount;
    int ply_lngbits;
    int ply_latbits;
    int ply_lng;
    int ply_lat;
    int ply_lnglimit;
    int ply_latlimit;
    int ply_lngsign;
    int ply_latsign;
    struct offset_len rgn_items[4];
    int rgn_type;

    int count;

    FILE *f;
    long pos;
    char line[256];
    int attr_pos;
    enum attr_type attr_last;
    char attrs[256];
    char attr[256];
    double lat,lng;
    char lat_c,lng_c;
    int eoc;
    struct map_priv *m;
    struct item item;
};

static int map_id;

static int contains_coord(char *line) {
    return g_ascii_isdigit(line[0]);
}

static int debug=1;

static int get_tag(char *line, char *name, int *pos, char *ret) {
    int len,quoted;
    char *p,*e,*n;

    if (debug)
        printf("get_tag %s from %s\n", name, line);
    if (! name)
        return 0;
    len=strlen(name);
    if (pos)
        p=line+*pos;
    else
        p=line;
    for(;;) {
        while (*p == ' ') {
            p++;
        }
        if (! *p)
            return 0;
        n=p;
        e=index(p,'=');
        if (! e)
            return 0;
        p=e+1;
        quoted=0;
        while (*p) {
            if (*p == ' ' && !quoted)
                break;
            if (*p == '"')
                quoted=1-quoted;
            p++;
        }
        if (e-n == len && !strncmp(n, name, len)) {
            e++;
            len=p-e;
            if (e[0] == '"') {
                e++;
                len-=2;
            }
            strncpy(ret, e, len);
            ret[len]='\0';
            if (pos)
                *pos=p-line;
            return 1;
        }
    }
    return 0;
}

static void get_line(struct map_rect_priv *mr) {
    mr->pos=ftell(mr->f);
    fgets(mr->line, 256, mr->f);
}

static void map_destroy_garmin_img(struct map_priv *m) {
    if (debug)
        printf("map_destroy_garmin_img\n");
    g_free(m);
}

static char *map_charset_garmin_img(struct map_priv *m) {
    return "iso8859-1";
}

static enum projection map_projection_garmin_img(struct map_priv *m) {
    return projection_garmin;
}

struct label_data_offset {
    struct offset_len offset_len;
    char multiplier;
    char data;
} __attribute ((packed));

#if 0
static void dump_label_data_offset(struct label_data_offset *lbl_dat) {
    dump_offset_len(&lbl_dat->offset_len);
    printf("multiplier 0x%x(%d)\n", lbl_dat->multiplier, lbl_dat->multiplier);
    printf("data 0x%x(%d)\n", lbl_dat->data, lbl_dat->data);
}
#endif

struct label_data {
    struct offset_len offset_len;
    short size;
    int zero;
} __attribute ((packed));

static void dump_label_data(struct label_data *lbl_dat) {
    dump_offset_len(&lbl_dat->offset_len);
    printf("size 0x%x(%d)\n", lbl_dat->size, lbl_dat->size);
}

struct tree_header {
    struct file_header fil_hdr;
    char boundary[12];
    struct offset_len level;
    struct offset_len subdivision;
    struct label_data copyright;
    struct offset_len tre7;
    short unknown1;
    char zero1;
    struct label_data polyline;
    struct label_data polygon;
    struct label_data point;
    int mapid;
};

static void dump_tree_header(struct tree_header *tre_hdr) {
    printf("tree_header:\n");
    dump_file(&tre_hdr->fil_hdr);
    printf("level: ");
    dump_offset_len(&tre_hdr->level);
    printf("subdivision: ");
    dump_offset_len(&tre_hdr->subdivision);
    printf("copyright: ");
    dump_label_data(&tre_hdr->copyright);
    printf("polyline: ");
    dump_label_data(&tre_hdr->polyline);
    printf("polygon: ");
    dump_label_data(&tre_hdr->polygon);
    printf("point: ");
    dump_label_data(&tre_hdr->point);
    printf("len: 0x%x(%d)\n", sizeof(*tre_hdr), sizeof(*tre_hdr));
}

struct label_header {
    struct file_header fil_hdr;
    struct label_data_offset label;
    struct label_data country;
    struct label_data region;
    struct label_data city;
    struct label_data poi_index;
    struct label_data_offset poi_properties;
    short zero1;
    char zero2;
    struct label_data poi_types;
    struct label_data zip;
    struct label_data hway;
    struct label_data exit;
    struct label_data hway_data;
    int unknown1;
    short unknown2;
    struct offset_len sort_descriptor;
    struct label_data lbl13;
    struct label_data lbl14;
} __attribute((packed));

#if 0
static void dump_label(struct label_header *lbl_hdr) {
    dump_file(&lbl_hdr->fil_hdr);
    printf("label:\n");
    dump_label_data_offset(&lbl_hdr->label);
    printf("country:\n");
    dump_label_data(&lbl_hdr->country);
    printf("region:\n");
    dump_label_data(&lbl_hdr->region);
    printf("city:\n");
    dump_label_data(&lbl_hdr->city);
    printf("poi_index:\n");
    dump_label_data(&lbl_hdr->poi_index);
    printf("poi_properties:\n");
    dump_label_data_offset(&lbl_hdr->poi_properties);
    printf("poi_types:\n");
    dump_label_data(&lbl_hdr->poi_types);
    printf("zip:\n");
    dump_label_data(&lbl_hdr->zip);
    printf("hway:\n");
    dump_label_data(&lbl_hdr->hway);
    printf("exit:\n");
    dump_label_data(&lbl_hdr->exit);
    printf("hway_data:\n");
    dump_label_data(&lbl_hdr->hway_data);
    printf("lbl13:\n");
    dump_label_data(&lbl_hdr->lbl13);
    printf("lbl14:\n");
    dump_label_data(&lbl_hdr->lbl14);
    printf("len: 0x%x(%d)\n", sizeof(*lbl_hdr), sizeof(*lbl_hdr));
}
#endif

struct triple {
    unsigned char data[3];
} __attribute((packed));

static unsigned int triple_u(struct triple *t) {
    return t->data[0] | (t->data[1] << 8)  | (t->data[2] << 16);
}

static int triple(struct triple *t) {
    int ret=t->data[0] | (t->data[1] << 8)  | (t->data[2] << 16);
    if (ret > 1<<23)
        ret=ret-(1<<24);
    return ret;
}

static void dump_triple_u(struct triple *t) {
    int val=triple_u(t);
    printf("0x%x(%d)\n", val, val);
}

struct tcoord {
    struct triple lng,lat;
} __attribute((packed));

static void dump_tcoord(struct tcoord *t) {
    printf ("0x%x(%d),0x%x(%d)\n", triple_u(&t->lng), triple_u(&t->lng), triple_u(&t->lat), triple_u(&t->lat));
}

struct level {
    unsigned char zoom;
    unsigned char bits_per_coord;
    unsigned short subdivisions;
} __attribute((packed));

static void dump_level(struct level *lvl) {
    printf("level:\n");
    printf("\tzoom 0x%x(%d)\n", lvl->zoom, lvl->zoom);
    printf("\tbits_per_coord 0x%x(%d)\n", lvl->bits_per_coord, lvl->bits_per_coord);
    printf("\tsubdivisions 0x%x(%d)\n", lvl->subdivisions, lvl->subdivisions);
}

struct subdivision {
    struct triple rgn_offset;
    unsigned char types;
    struct tcoord center;
    unsigned short width;
    unsigned short height;
    unsigned short next;
} __attribute((packed));

static void dump_subdivision(struct subdivision *sub) {
    printf("subdivision:\n");
    printf("\trgn_offset: ");
    dump_triple_u(&sub->rgn_offset);
    printf("\ttypes: 0x%x(%d)\n", sub->types, sub->types);
    printf("\tcenter: ");
    dump_tcoord(&sub->center);
    printf("\tsize: 0x%x(%d)x0x%x(%d) %s\n",sub->width & 0x7fff, sub->width & 0x7fff, sub->height, sub->height,
           sub->width & 0x8000 ? "Terminating" : "");
    printf("\tnext: 0x%x(%d)\n",sub->next, sub->next);

    printf("\tlen: 0x%x(%d)\n", sizeof(*sub), sizeof(*sub));
}

struct rgn_point {
    unsigned char info;
    struct triple lbl_offset;
    short lng_delta;
    short lat_delta;
    unsigned char subtype;
} __attribute((packed));

static void dump_point(struct rgn_point *pnt) {
    printf("point:\n");
    printf("\tinfo 0x%x(%d)\n", pnt->info, pnt->info);
    printf("\tlbl_offset 0x%x(%d)\n", triple_u(&pnt->lbl_offset), triple_u(&pnt->lbl_offset));
    printf("\tlng_delta 0x%x(%d)\n", pnt->lng_delta, pnt->lng_delta);
    printf("\tlat_delta 0x%x(%d)\n", pnt->lat_delta, pnt->lat_delta);
    printf("\tsubtype 0x%x(%d)\n", pnt->subtype, pnt->subtype);
    printf("\tlen: 0x%x(%d)\n", sizeof(*pnt), sizeof(*pnt));
}

struct rgn_poly {
    unsigned char info;
    struct triple lbl_offset;
    short lng_delta;
    short lat_delta;
    union {
        struct {
            unsigned char bitstream_len;
            unsigned char bitstream_info;
        } __attribute((packed)) p1;
        struct {
            unsigned short bitstream_len;
            unsigned char bitstream_info;
        } __attribute((packed)) p2;
    } __attribute((packed)) u;
} __attribute((packed));

static void dump_poly(struct rgn_poly *ply) {
    printf("poly:\n");
    printf("\tinfo 0x%x(%d)\n", ply->info, ply->info);
    printf("\tlbl_offset 0x%x(%d)\n", triple_u(&ply->lbl_offset), triple_u(&ply->lbl_offset));
    printf("\tlng_delta 0x%x(%d)\n", ply->lng_delta, ply->lng_delta);
    printf("\tlat_delta 0x%x(%d)\n", ply->lat_delta, ply->lat_delta);
    if (ply->info & 0x80) {
        printf("\tbitstream_len 0x%x(%d)\n", ply->u.p2.bitstream_len, ply->u.p2.bitstream_len);
        printf("\tbitstream_info 0x%x(%d)\n", ply->u.p2.bitstream_info, ply->u.p2.bitstream_info);
    } else {
        printf("\tbitstream_len 0x%x(%d)\n", ply->u.p1.bitstream_len, ply->u.p1.bitstream_len);
        printf("\tbitstream_info 0x%x(%d)\n", ply->u.p1.bitstream_info, ply->u.p1.bitstream_info);
    }
    printf("\tlen: 0x%x(%d)\n", sizeof(*ply), sizeof(*ply));
}

static void dump_hex(void *ptr, int len) {
    unsigned char *c=ptr;
    while (len--) {
        printf("%02x ", *c++);
    }
    printf("\n");
}

static void dump_hex_r(void *ptr, int len, int rec) {
    unsigned char *c=ptr;
    int l=rec;
    while (len--) {
        printf("%02x ", *c++);
        if (! --l) {
            printf("\n");
            l=rec;
        }
    }
    printf("\n");
}

#if 0
static void dump_label_offset(struct map_rect_priv *mr, int offset) {
    void *p;
    p=file_read(&mr->lbl, mr->lbl_hdr->label.offset_len.offset+offset, 128);
    printf("%s\n", (char *)p);
}
#endif


#if 0
static void dump_region_item(struct subdivision *sub, struct file *rgn, struct map_rect_priv *mr) {
    int offset,item_offset,i,j;
    unsigned short count=0;
    unsigned short *offsets[4];
    unsigned short *file_offsets;
    struct rgn_point *pnt;

    offset=triple_u(&sub->rgn_offset)+mr->rgn_hdr->offset_len.offset;
    file_offsets=file_read(rgn, offset, 90*sizeof(unsigned short));
    printf("0x%x ", offset);
    dump_hex(file_offsets, 90);
    for (i=0 ; i < 4 ; i++) {
        printf("i=%d\n", i);
        if (sub->types & (0x10 << i)) {
            if (count) {
                offsets[i]=&file_offsets[count-1];
            } else
                offsets[i]=&count;
            count++;
        } else
            offsets[i]=NULL;

    }
    count--;
    count*=2;
    for (i=0 ; i < 4 ; i++) {
        printf("i=%d\n", i);
        if (offsets[i]) {
            printf("offset[%d]=0x%x(%d)\n", i, *offsets[i], *offsets[i]);
            switch (i) {
            case 0:
                printf("point\n");
                break;
            case 1:
                printf("indexed point\n");
                break;
            case 2:
                printf("polyline\n");
                break;
            case 3:
                printf("polygon\n");
                break;
            }
            item_offset=offset+*offsets[i];
            switch (i) {
            case 0:
            case 1:
                for (j = 0 ; j < 10 ; j++) {
                    struct coord_geo g;
                    char buffer[1024];
                    double conv=180.0/(1UL<<23);
                    pnt=file_read(rgn, item_offset, sizeof(*pnt)*20);
                    // printf("0x%x ", item_offset); dump_hex(pnt, 32);
                    dump_point(pnt);
                    g.lng=(triple(&sub->center.lng)+(pnt->lng_delta << shift))*conv;
                    g.lat=(triple(&sub->center.lat)+(pnt->lat_delta << shift))*conv;
                    printf("%f %f\n", g.lng, g.lat);
                    coord_format(g.lat,g.lng,DEGREES_MINUTES_SECONDS,
                                 buffer,sizeof(buffer));
                    printf("%s\n", buffer);
                    dump_label_offset(mr, triple_u(&pnt->lbl_offset));
                    if (pnt->info & 0x80)
                        item_offset+=sizeof(*pnt);
                    else
                        item_offset+=sizeof(*pnt)-1;
                }
            }
        } else {
            printf("offset[%d] doesn't exist\n", i);
        }
    }
    file_free(file_offsets);
}

#endif

static void dump_levels(struct map_rect_priv *mr) {
    int i,offset;
    struct level *lvl;

    offset=mr->tre_hdr->level.offset;
    for (i = 0 ; i < mr->tre_hdr->level.length/sizeof(*lvl) ; i++) {
        lvl=file_read(&mr->tre, offset, sizeof(*lvl));
        dump_level(lvl);
        offset+=sizeof(*lvl);
    }
}

#if 0
static void dump_tree(struct file *f, struct file *rgn, struct map_rect_priv *mr) {
    struct tree_header *tre_hdr;
    struct subdivision *sub;
    int i,offset;

    tre_hdr=file_read(f, 0, sizeof(*tre_hdr));
    dump_tree_header(tre_hdr);
    offset=tre_hdr->subdivision.offset;
    sub=file_read(f, offset, sizeof(*sub));
    dump_subdivision(sub);
    offset+=sizeof(*sub);
    for (i = 1 ; i < tre_hdr->subdivision.length/sizeof(*sub) ; i++) {
        printf("i=%d\n", i);
        sub=file_read(f, offset, sizeof(*sub));
        dump_subdivision(sub);
        dump_region_item(sub, rgn, mr);
        if (sub->width & 0x8000)
            break;
        offset+=sizeof(*sub);
    }
    file_free(tre_hdr);
}
#endif

#if 0
static void dump_labels(struct file *f) {
    struct label_header *lbl_hdr;

    lbl_hdr=file_read(f, 0, sizeof(*lbl_hdr));
    printf("**labels**\n");
    dump_label(lbl_hdr);
    file_free(lbl_hdr);
#if 0
    labels=alloca(lbl_hdr.label_length);
    file_read(f, lbl_hdr.label_offset, labels, lbl_hdr.label_length);
    l=labels;
    while (l < labels+lbl_hdr.label_length) {
        printf("'%s'(%d)\n", l, strlen(l));
        l+=strlen(l)+1;
    }
#endif

}
#endif

static void garmin_img_coord_rewind(void *priv_data) {
}

static void parse_line(struct map_rect_priv *mr) {
    int pos=0;
    sscanf(mr->line,"%lf %c %lf %c %n",&mr->lat,&mr->lat_c,&mr->lng,&mr->lng_c,&pos);
    if (pos < strlen(mr->line)) {
        strcpy(mr->attrs, mr->line+pos);
    }
}

static int get_bits(struct map_rect_priv *mr, int bits) {
    unsigned long ret;
    ret=L(*((unsigned long *)(mr->ply_data+mr->ply_bitpos/8)));
    ret >>= (mr->ply_bitpos & 7);
    ret &= (1 << bits)-1;
    mr->ply_bitpos+=bits;
    return ret;
}

static int garmin_img_coord_get(void *priv_data, struct coord *c, int count) {
    struct map_rect_priv *mr=priv_data;
    struct subdivision *sub=(struct subdivision *)(mr->subdiv+mr->subdiv_pos);
    int ret=0;
    int debug=0;
    if (debug)
        printf("garmin_img_coord_get %d\n",count);
    if (debug)
        dump_subdivision(sub);
    while (count--) {
        if (mr->rgn_type < 2) {
            c->x=triple(&sub->center.lng)+(mr->pnt->lng_delta << shift);
            c->y=triple(&sub->center.lat)+(mr->pnt->lat_delta << shift);
        } else {
            if (! mr->ply_bitpos) {
                if (mr->ply->info & 0x80) {
                    mr->ply_bitcount=mr->ply->u.p2.bitstream_len*8;
                    mr->ply_lngbits=mr->ply->u.p2.bitstream_info & 0xf;
                    mr->ply_latbits=mr->ply->u.p2.bitstream_info >> 4;
                } else {
                    mr->ply_bitcount=mr->ply->u.p1.bitstream_len*8;
                    mr->ply_lngbits=mr->ply->u.p1.bitstream_info & 0xf;
                    mr->ply_latbits=mr->ply->u.p1.bitstream_info >> 4;
                }
                if (mr->ply_lngbits <= 9)
                    mr->ply_lngbits+=2;
                if (mr->ply_latbits <= 9)
                    mr->ply_latbits+=2;
                if (! get_bits(mr,1)) {
                    mr->ply_lngbits+=1;
                    mr->ply_lngsign=0;
                } else if (get_bits(mr, 1))
                    mr->ply_lngsign=-1;
                else
                    mr->ply_lngsign=1;
                if (! get_bits(mr,1)) {
                    mr->ply_latbits+=1;
                    mr->ply_latsign=0;
                } else if (get_bits(mr, 1))
                    mr->ply_latsign=-1;
                else
                    mr->ply_latsign=1;
                mr->ply_lnglimit=1 << (mr->ply_lngbits-1);
                mr->ply_latlimit=1 << (mr->ply_latbits-1);
                mr->ply_lng=mr->ply->lng_delta;
                mr->ply_lat=mr->ply->lat_delta;
                if (debug)
                    printf("lngbits %d latbits %d bitcount %d\n", mr->ply_lngbits, mr->ply_latbits, mr->ply_bitcount);
                c->x=0;
                c->y=0;
            } else {
                if (mr->ply_bitpos + mr->ply_lngbits + mr->ply_latbits > mr->ply_bitcount) {
                    if (debug)
                        printf("out of bits %d + %d + %d >= %d\n", mr->ply_bitpos, mr->ply_lngbits, mr->ply_latbits, mr->ply_bitcount);
                    return ret;
                }
                c->x=0;
                c->y=0;
                int x,y;
                for (;;) {
                    x=get_bits(mr,mr->ply_lngbits);
                    if (debug)
                        printf("x %d ", x);
                    if (mr->ply_lngsign || x != mr->ply_lnglimit)
                        break;
                    c->x += x-1;
                }
                if (mr->ply_lngsign) {
                    c->x=x*mr->ply_lngsign;
                } else {
                    if (x >= mr->ply_lnglimit)
                        c->x = x - (mr->ply_lnglimit << 1) - c->x;
                    else
                        c->x +=x;
                }
                for (;;) {
                    y=get_bits(mr,mr->ply_latbits);
                    if (debug)
                        printf("y %d ", y);
                    if (mr->ply_latsign || y != mr->ply_latlimit)
                        break;
                    c->y += y-1;
                }
                if (mr->ply_latsign) {
                    c->y=y*mr->ply_latsign;
                } else {
                    if (y >= mr->ply_latlimit)
                        c->y = y - (mr->ply_latlimit << 1) - c->y;
                    else
                        c->y +=y;
                }
                mr->ply_lng += c->x;
                mr->ply_lat += c->y;
            }
            if (debug)
                printf(": x %d y %d\n", c->x, c->y);

            c->x=triple(&sub->center.lng)+(mr->ply_lng << shift);
            c->y=triple(&sub->center.lat)+(mr->ply_lat << shift);
        }
#if 0
        c->x-=0x6f160;
        c->y-=0x181f59;
        c->x+=0x168ca1;
        c->y+=0x68d815;
#endif
        c++;
        ret++;
        if (mr->rgn_type < 2)
            return ret;
    }
    return ret;
}

static char *get_label_offset(struct map_rect_priv *mr, int offset) {
    g_assert(offset < mr->lbl_hdr->label.offset_len.length);
    return file_read(&mr->lbl, mr->lbl_hdr->label.offset_len.offset+offset, 128);
}

static void garmin_img_attr_rewind(void *priv_data) {
}

static int garmin_img_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr) {
    struct map_rect_priv *mr=priv_data;
    int debug=0;

    if (debug)
        printf("garmin_img_attr_get\n");
    if (attr_type == attr_label) {
        if (debug)
            printf("garmin_img_attr_get label\n");
        attr->type=attr_type;
        if (mr->rgn_type < 2) {
            if (mr->label)
                file_free(mr->label);
            mr->label=get_label_offset(mr, triple_u(&mr->pnt->lbl_offset) & 0x3fffff);
            attr->u.str=mr->label;
        } else {
            attr->u.str="";
        }
        return 1;
    }
    return 0;
}

static struct item_methods methods_garmin_img = {
    garmin_img_coord_rewind,
    garmin_img_coord_get,
    garmin_img_attr_rewind,
    garmin_img_attr_get,
};

static int rgn_next_type(struct map_rect_priv *mr) {
    while (mr->rgn_type < 3) {
        mr->rgn_type++;
        if (mr->rgn_items[mr->rgn_type].offset && mr->rgn_items[mr->rgn_type].length != 0) {
            mr->rgn_offset=mr->rgn_items[mr->rgn_type].offset;
            mr->rgn_end=mr->rgn_offset+mr->rgn_items[mr->rgn_type].length;
            return 0;
        }
    }
    return 1;
}

static int sub_next(struct map_rect_priv *mr, int next) {
    int i,offset,first=-1,last=-1,count=-1;
    int end;
    unsigned short *offsets;
    int debug=0;

    if (mr->subdiv_level_count <= 0)
        return 1;
    if (debug)
        printf("%d left\n", mr->subdiv_level_count);
    mr->subdiv_level_count--;

#if 0
    if (next && mr->subdiv[mr->subdiv_current].width & 0x8000)
        return 1;
#endif
    if (debug)
        dump_hex_r(mr->subdiv+mr->subdiv_pos, 64, 14);
    mr->subdiv_pos+=next;
    if (debug)
        printf("subdiv_pos 0x%x\n", mr->subdiv_pos);
    if (mr->subdiv_pos > mr->tre_hdr->subdivision.length)
        return 1;
    struct subdivision *sub=(struct subdivision *)(mr->subdiv+mr->subdiv_pos);
    offset=triple_u(&sub->rgn_offset)+mr->rgn_hdr->offset_len.offset;
    if (debug) {
        printf("offset=0x%x\n", offset);
        dump_subdivision(sub);
    }
    offsets=file_read(&mr->rgn, offset, 3*sizeof(unsigned short));

    if (! next)
        next=subdiv_next;
    if (mr->subdiv_pos+next < mr->tre_hdr->subdivision.length)
        end=triple_u(&((struct subdivision *)(mr->subdiv+mr->subdiv_pos+next))->rgn_offset)+mr->rgn_hdr->offset_len.offset;
    else
        end=mr->rgn_hdr->offset_len.offset+mr->rgn_hdr->offset_len.length;
    if (debug) {
        dump_subdivision(sub);
        dump_hex(offsets, 6);
    }
    for (i=0 ; i < 4 ; i++) {
        if (debug)
            printf("i=%d ", i);
        if (sub->types & (0x10 << i)) {
            if (debug)
                printf("+ ");
            if (first == -1) {
                first=i;
                mr->rgn_items[i].offset=offset;
                if (debug)
                    printf("\n");
            } else {
                mr->rgn_items[i].offset=offset+offsets[count];
                if (debug)
                    printf("0x%x\n", offsets[count]);
                mr->rgn_items[last].length=mr->rgn_items[i].offset-mr->rgn_items[last].offset;
            }
            last=i;
            count++;
        } else {
            if (debug)
                printf("-\n");
            mr->rgn_items[i].offset=0;
            mr->rgn_items[i].length=0;
        }

    }
    if (first != -1) {
        mr->rgn_items[first].offset+=count*2;
        mr->rgn_items[first].length-=count*2;
        mr->rgn_items[last].length=end-mr->rgn_items[last].offset;
    }
    if (debug) {
        for (i=0 ; i < 4 ; i++) {
            printf("%d 0x%x 0x%x\n", i, mr->rgn_items[i].offset, mr->rgn_items[i].length);
        }
    }
    mr->rgn_type=-1;
    rgn_next_type(mr);
    if (debug)
        printf("*** offset 0x%x\n", mr->rgn_offset);
    file_free(offsets);
    return 0;
}

int item_count;

static struct map_rect_priv *map_rect_new_garmin_img(struct map_priv *map, struct coord_rect *r, struct layer *layers,
        int limit) {
    struct map_rect_priv *mr;
    struct img_header img;

    if (debug)
        printf("map_rect_new_garmin_img\n");
    mr=g_new0(struct map_rect_priv, 1);
    mr->m=map;
    if (r)
        mr->r=*r;
    mr->limit=limit;
    mr->item.id_hi=0;
    mr->item.id_lo=0;
    mr->item.meth=&methods_garmin_img;
    mr->item.priv_data=mr;
    mr->f=fopen(map->filename, "r");

    fread(&img, sizeof(img), 1, mr->f);
#if 0
    dump_img(&img);
    for (i = 0 ; i < (img.file_offset-sizeof(img))/sizeof(fat_blk) ; i++) {
        fread(&fat_blk, sizeof(fat_blk), 1, mr->f);
        if (!fat_blk.flag)
            break;
        dump_fat_block(&fat_blk);
    }
#endif
    mr->rgn.offset=0xa*2048;
    mr->rgn.f=mr->f;
    mr->rgn_hdr=file_read(&mr->rgn, 0, sizeof(*mr->rgn_hdr));

    mr->tre.offset=0x62b*2048;
    mr->tre.f=mr->f;
    mr->tre_hdr=file_read(&mr->tre, 0, sizeof(*mr->tre_hdr));

    mr->lbl.offset=0x64a*2048;
    mr->lbl.f=mr->f;
    mr->lbl_hdr=file_read(&mr->lbl, 0, sizeof(*mr->lbl_hdr));

    mr->subdiv=file_read(&mr->tre, mr->tre_hdr->subdivision.offset, mr->tre_hdr->subdivision.length);
#if 0
    dump_hex_r(mr->subdiv, mr->tre_hdr->subdivision.length, 16);
#endif
    dump_tree_header(mr->tre_hdr);

    dump_levels(mr);


    printf("limit=%d\n", limit);
    if (limit < 3) {
        mr->subdiv_pos=0;
        mr->subdiv_level_count=1;
        shift=11;
    } else if (limit < 6) {
        mr->subdiv_pos=1*sizeof(struct subdivision);
        mr->subdiv_level_count=5;
        shift=9;
    } else if (limit < 8) {
        mr->subdiv_pos=6*sizeof(struct subdivision);
        mr->subdiv_level_count=9;
        shift=7;
    } else if (limit < 10) {
        mr->subdiv_pos=15*sizeof(struct subdivision);
        mr->subdiv_level_count=143;
        shift=5;
    } else {
        mr->subdiv_pos=158*sizeof(struct subdivision);
        mr->subdiv_level_count=4190;
        shift=2;
        subdiv_next=14;
    }

#if 0
    mr->rgn_offset=triple_u(&mr->subdiv[mr->subdiv_current].rgn_offset)+mr->rgn_hdr->offset_len.offset+4;
    mr->rgn_type=1;
    mr->rgn_end=mr->rgn_offset+20*8;
#endif
    mr->count=0;
    item_count=0;

#if 0
    printf("*** offset 0x%x\n", 0x656c-mr->rgn.offset);
    printf("*** offset 0x%x\n", mr->rgn_offset);
#endif
#if 1
    sub_next(mr, 0);
#endif
#if 0
    {
        struct rgn_point *pnt;
        int i;
        int offset=0x65cc;
        for (i = 0 ; i < 26 ; i++) {
            pnt=file_read(&mr->rgn, 0x656c+8*i-mr->rgn.offset, sizeof(*pnt));
            // dump_hex(pnt, sizeof(*pnt));
            dump_point(pnt);
            dump_label_offset(mr, triple_u(&pnt->lbl_offset));
        }
    }
    exit(0);
#endif
#if 0
    dump_tree(&mr->tre,&mr->rgn,mr);
#endif

#if 0
    f.offset=0x64a*2048;
    f.f=mr->f;
    dump_labels(&f);
#endif
#if 0
    fseek(mr->f, img.file_offset, SEEK_SET);
    fread(&fil, sizeof(fil), 1, mr->f);
    dump_file(&fil);
    fread(&rgn, sizeof(rgn), 1, mr->f);
    dump_region(&rgn);
    fseek(mr->f, rgn.data_length, SEEK_CUR);
    fread(&fil, sizeof(fil), 1, mr->f);
    dump_file(&fil);
#endif
    return mr;
}


static void map_rect_destroy_garmin_img(struct map_rect_priv *mr) {
    fclose(mr->f);
    g_free(mr);
}


static struct item *map_rect_get_item_garmin_img(struct map_rect_priv *mr) {
    char *p,type[256];
    int ptype;
    int debug=0;

    item_count++;

    if (debug)
        printf("map_rect_get_item_garmin_img\n");
    for (;;) {
        if (mr->rgn_offset < mr->rgn_end) {
            if (debug)
                printf("data available\n");
            if (mr->rgn_type >= 2) {
                int len;
                if (debug)
                    printf("polyline %d\n", mr->count);
                if (mr->ply)
                    file_free(mr->ply);
                mr->ply=file_read(&mr->rgn, mr->rgn_offset, sizeof(*mr->ply)*3);
                if(triple_u(&mr->ply->lbl_offset) >= mr->lbl_hdr->label.offset_len.length) {
                    printf("item_count %d\n", item_count);
                    dump_poly(mr->ply);
                    dump_hex(mr->ply, 32);
                    printf("%d vs %d\n", triple_u(&mr->ply->lbl_offset), mr->lbl_hdr->label.offset_len.length);
                }
                g_assert(triple_u(&mr->ply->lbl_offset) < mr->lbl_hdr->label.offset_len.length);
                if (debug) {
                    dump_hex(mr->ply, 16);
                    dump_poly(mr->ply);
                }
                if (mr->ply_data)
                    file_free(mr->ply_data);
                mr->rgn_offset+=10;
                if (mr->ply->info & 0x80) {
                    mr->rgn_offset++;
                    len=mr->ply->u.p2.bitstream_len;
                } else
                    len=mr->ply->u.p1.bitstream_len;

                mr->ply_data=file_read(&mr->rgn, mr->rgn_offset, len);
                mr->rgn_offset += len;
                mr->ply_bitpos=0;
                // dump_hex(mr->ply_data, 32);
                if (mr->rgn_type == 3) {
                    switch(mr->ply->info & 0x7f) {
                    case 0x1:	/* large urban area (>200k) */
                        mr->item.type=type_town_poly;
                        break;
                    case 0xd:	/* reservation */
                        mr->item.type=type_park_poly;
                        break;
                    case 0xe: 	/* airport runway */
                        mr->item.type=type_airport_poly;
                        break;
                    case 0x14:	/* national park */
                        mr->item.type=type_park_poly;
                        break;
                    case 0x32:	/* sea */
                    case 0x3d: 	/* large lake (77-250km2) */
                    case 0x4c: 	/* intermittend water */
                        mr->item.type=type_water_poly;
                        break;
                    case 0x4b: 	/* background */
                        continue;
                    default:
                        printf("unknown polygon: 0x%x\n", mr->ply->info);
                        mr->item.type=type_street_3_city;
                    }
                } else {
                    switch(mr->ply->info & 0x3f) {
                    case 0x1:	/* major highway */
                        mr->item.type=type_highway_land;
                        break;
                    case 0x2:	/* principal highway */
                        mr->item.type=type_street_3_land;
                        break;
                    case 0x6:	/* residental street */
                        mr->item.type=type_street_2_land;
                        break;
                    case 0x16:	/* walkway/trail */
                        mr->item.type=type_street_1_land;
                        break;
                    case 0x1e:	/* international boundary */
                        mr->item.type=type_border_country;
                        break;
                    case 0x20:	/* minor land contour 1/10 */
                        mr->item.type=type_height_line_1;
                        break;
                    case 0x21:	/* major land contour 1/2 */
                        mr->item.type=type_height_line_2;
                        break;
                    default:
                        printf("unknown polyline: 0x%x\n", mr->ply->info);
                        mr->item.type=type_street_3_city;
                    }
                }
                return &mr->item;
            }
            if (mr->pnt)
                file_free(mr->pnt);
            mr->pnt=file_read(&mr->rgn, mr->rgn_offset, sizeof(*mr->pnt));
            mr->item.type=type_none;
            int subtype=mr->pnt->subtype;
            if (mr->pnt->lbl_offset.data[2] & 0x80)
                mr->rgn_offset+=9;
            else {
                mr->rgn_offset+=8;
                subtype=0;
            }
            switch(mr->pnt->info) {
            case 0x3:	/* large city 2-5M */
                mr->item.type=type_town_label_2e6;
                break;
            case 0xa:	/* small city/town 10-20k */
                mr->item.type=type_town_label_1e4;
                break;
            case 0xd:	/* settlement 1-2K  */
                mr->item.type=type_town_label_1e3;
                break;
            case 0x11:	/* settlement less 100 */
                mr->item.type=type_town_label_5e1;
                break;
            case 0x1c:
                switch(subtype) {
                case 0x01:
                    mr->item.type=type_poi_wreck;
                    break;
                }
                break;
            case 0x20:
                mr->item.type=type_highway_exit;
                break;
            case 0x25:
                mr->item.type=type_poi_toll_booth;
                break;
            case 0x2b:
                switch(subtype) {
                case 0x01:
                    mr->item.type=type_poi_hotel;
                    break;
                case 0x03:
                    mr->item.type=type_poi_camp_rv;
                    break;
                }
                break;
            case 0x2c:
                switch(subtype) {
                case 0x00:
                    mr->item.type=type_poi_attraction;
                    break;
                case 0x02:
                    mr->item.type=type_poi_museum_history;
                    break;
                }
                break;
            case 0x2e:
                mr->item.type=type_poi_shopping;
                break;
            case 0x2f:
                switch(subtype) {
                case 0x01:
                    mr->item.type=type_poi_fuel;
                    break;
                case 0x07:
                    mr->item.type=type_poi_car_dealer_parts;
                    break;
                case 0x0b:
                    mr->item.type=type_poi_car_parking;
                    break;
                case 0x15:
                    mr->item.type=type_poi_public_utilities;
                    break;
                }
                break;
            case 0x30:
                switch(subtype) {
                case 0x02:
                    mr->item.type=type_poi_hospital;
                    break;
                }
                break;
            case 0x43:
                mr->item.type=type_poi_marina;
                break;
            case 0x46:
                mr->item.type=type_poi_bar;
                break;
            case 0x48:
                mr->item.type=type_poi_camping;
                break;
            case 0x49:
                mr->item.type=type_poi_park;
                break;
            case 0x4a:
                mr->item.type=type_poi_picnic;
                break;
            case 0x59:	/* airport */
                mr->item.type=type_poi_airport;
                break;
            case 0x64:
                switch(subtype) {
                case 0x1:
                    mr->item.type=type_poi_bridge;
                    break;
                case 0x2:
                    mr->item.type=type_poi_building;
                    break;
                case 0x15:
                    mr->item.type=type_town_ghost;
                    break;
                }
                break;
            case 0x65:
                switch(subtype) {
                case 0x0:
                    mr->item.type=type_poi_water_feature;
                    break;
                case 0xc:
                    mr->item.type=type_poi_island;
                    break;
                case 0xd:
                    mr->item.type=type_poi_lake;
                    break;
                }
                break;
            case 0x66:
                switch(subtype) {
                case 0x0:
                    mr->item.type=type_poi_land_feature;
                    break;
                case 0x6:
                    mr->item.type=type_poi_cape;
                    break;
                case 0x14:
                    mr->item.type=type_poi_rock;
                    break;
                }
                break;
            }
            if (mr->item.type == type_none) {
                printf("unknown point: 0x%x 0x%x\n", mr->pnt->info, mr->pnt->subtype);
                dump_point(mr->pnt);
                printf("label: %s\n", get_label_offset(mr, triple_u(&mr->pnt->lbl_offset) & 0x3fffff));
                mr->item.type=type_town_label;
            }
            return &mr->item;
        }
        if (debug)
            printf("out of data for type\n");
        if (rgn_next_type(mr)) {
            if (debug)
                printf("out of data for region\n");
            if (sub_next(mr, subdiv_next)) {
                if (debug)
                    printf("out of data for subdivision\n");
                return NULL;
            }
        }
    }
}

static struct item *map_rect_get_item_byid_garmin_img(struct map_rect_priv *mr, int id_hi, int id_lo) {
    fseek(mr->f, id_lo, SEEK_SET);
    get_line(mr);
    mr->item.id_hi=id_hi;
    return map_rect_get_item_garmin_img(mr);
}

static struct map_methods map_methods_garmin_img = {
    projection_garmin,
    "iso8859-1",
    map_destroy_garmin_img,
    map_charset_garmin_img,
    map_projection_garmin_img,
    map_rect_new_garmin_img,
    map_rect_destroy_garmin_img,
    map_rect_get_item_garmin_img,
    map_rect_get_item_byid_garmin_img,
};

static struct map_priv *map_new_garmin_img(struct map_methods *meth, struct attr **attrs) {
    struct map_priv *m;
    struct attr *data=attr_search(attrs, attr_data);
    if (! data)
        return NULL;

    *meth=map_methods_garmin_img;
    m=g_new(struct map_priv, 1);
    m->id=++map_id;
    m->filename=g_strdup(data->u.str);
    return m;
}

void plugin_init(void) {
    plugin_register_category_map("garmin_img", map_new_garmin_img);
}

