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


int block_lin_count,block_idx_count,block_active_count,block_mem,block_active_mem;

struct block_index_item {
    /*unsigned int blocknum;
    unsigned int blocks;*/
    unsigned char p[8];
};
static inline unsigned int block_index_item_get_blocknum(struct block_index_item * blk) {
    unsigned char *p = blk->p;
    return get_u32(&p);
}
static inline unsigned int block_index_item_get_blocks(struct block_index_item * blk) {
    unsigned char *p = blk->p+4;
    return get_u32(&p);
}

struct block_index {
    /*	unsigned int blocks;
            unsigned int size;
            unsigned int next;
    	struct block_index_item list[0];*/
    unsigned char p[12];
};
static inline unsigned int block_index_get_blocks(struct block_index * blk) {
    unsigned char *p = blk->p;
    return get_u32(&p);
}
static inline unsigned int block_index_get_size(struct block_index * blk) {
    unsigned char *p = blk->p+4;
    return get_u32(&p);
}
static inline unsigned int block_index_get_next(struct block_index * blk) {
    unsigned char *p = blk->p+8;
    return get_u32(&p);
}
static inline struct block_index_item * block_index_get_list(struct block_index * blk) {
    return (struct block_index_item *)(blk->p+12);
}

static struct block *block_get(unsigned char **p) {
    struct block *ret=(struct block *)(*p);
    *p += sizeof(*ret);
    return ret;
}


static struct block *block_get_byid(struct file *file, int id, unsigned char **p_ret) {
    struct block_index *blk_idx;
    int blk_num,max;


    blk_idx=(struct block_index *)(file->begin+0x1000);
    max=(block_index_get_size(blk_idx)-sizeof(struct block_index))/sizeof(struct block_index_item);
    block_mem+=24;
    while (id >= max) {
        blk_idx=(struct block_index *)(file->begin+block_index_get_next(blk_idx)*512);
        id-=max;
    }
    blk_num=block_index_item_get_blocknum(&block_index_get_list(blk_idx)[id]);

    *p_ret=file->begin+blk_num*512;
    return block_get(p_ret);
}

int block_get_byindex(struct file *file, int idx, struct block_priv *blk) {
    dbg(lvl_debug,"idx=%d", idx);
    blk->b=block_get_byid(file, idx, &blk->p);
    blk->block_start=(unsigned char *)(blk->b);
    blk->p_start=blk->p;
    blk->end=blk->block_start+block_get_size(blk->b);

    return 1;
}

static void block_setup_tags(struct map_rect_priv *mr) {
    int len;
    unsigned char *p,*t;
    char *str;

    mr->b.binarytree=0;

    p=mr->file->begin+0x0c;
    while (*p) {
        str=get_string(&p);
        len=get_u32_unal(&p);
        t=p;
        /* printf("String '%s' len %d\n", str, len); */
        if (! strcmp(str,"FirstBatBlock")) {
            /* printf("%ld\n", get_u32_unal(&t)); */
        } else if (! strcmp(str,"MaxBlockSize")) {
            /* printf("%ld\n", get_u32_unal(&t)); */
        } else if (! strcmp(str,"FREE_BLOCK_LIST")) {
            /* printf("%ld\n", get_u32_unal(&t)); */
        } else if (! strcmp(str,"TotalRect")) {
            mr->b.b_rect.lu.x=get_u32_unal(&t);
            mr->b.b_rect.lu.y=get_u32_unal(&t);
            mr->b.b_rect.rl.x=get_u32_unal(&t);
            mr->b.b_rect.rl.y=get_u32_unal(&t);
            /* printf("0x%x,0x%x-0x%x,0x%x\n", mr->b.b_rect.lu.x, mr->b.b_rect.lu.y, mr->b.b_rect.rl.x, mr->b.b_rect.rl.y); */
        } else if (! strcmp(str,"Version")) {
            /* printf("0x%lx\n", get_u32_unal(&t)); */
        } else if (! strcmp(str,"Categories")) {
            /* printf("0x%x\n", get_u16(&t)); */
        } else if (! strcmp(str,"binaryTree")) {
            mr->b.binarytree=get_u32_unal(&t);
            /* printf("%d\n", mr->b.binarytree); */
        } else if (! strcmp(str,"CategorySets")) {
            /* printf("0x%x\n", get_u16(&t)); */
        } else if (! strcmp(str,"Kommentar")) {
            /* printf("%s\n", get_string(&t)); */
        }
        p+=len;
    }
}

#if 0
static void block_rect_print(struct coord_rect *r) {
    printf ("0x%x,0x%x-0x%x,0x%x (0x%x,0x%x)", r->lu.x, r->lu.y, r->rl.x, r->rl.y, r->lu.x/2+r->rl.x/2,r->lu.y/2+r->rl.y/2);
}
#endif

static void block_rect_same(struct coord_rect *r1, struct coord_rect *r2) {
    dbg_assert(r1->lu.x==r2->lu.x);
    dbg_assert(r1->lu.y==r2->lu.y);
    dbg_assert(r1->rl.x==r2->rl.x);
    dbg_assert(r1->rl.y==r2->rl.y);
}

int block_init(struct map_rect_priv *mr) {
    mr->b.block_num=-1;
    mr->b.bt.b=NULL;
    mr->b.bt.next=0;
    block_setup_tags(mr);
    if (mr->b.binarytree) {
        mr->b.bt.next=mr->b.binarytree;
        mr->b.bt.p=NULL;
        mr->b.bt.block_count=0;
    }
    if (mr->cur_sel && !coord_rect_overlap(&mr->cur_sel->u.c_rect, &mr->b.b_rect))
        return 0;
    return block_next(mr);
}


int block_next_lin(struct map_rect_priv *mr) {
    struct coord_rect r;
    for (;;) {
        block_lin_count++;
        block_mem+=sizeof(struct block *);
        mr->b.block_num++;
        if (! mr->b.block_num)
            mr->b.p=mr->file->begin+0x2000;
        else
            mr->b.p=mr->b.block_start+block_get_blocks(mr->b.b)*512;
        if (mr->b.p >= mr->file->end) {
            dbg(lvl_debug,"end of blocks %p vs %p", mr->b.p, mr->file->end);
            return 0;
        }
        mr->b.block_start=mr->b.p;
        mr->b.b=block_get(&mr->b.p);
        mr->b.p_start=mr->b.p;
        mr->b.end=mr->b.block_start+block_get_size(mr->b.b);
        if (block_get_count(mr->b.b) == -1) {
            dbg(lvl_warning,"empty blocks");
            return 0;
        }
        block_get_r(mr->b.b, &r);
        if (!mr->cur_sel || coord_rect_overlap(&mr->cur_sel->u.c_rect, &r)) {
            block_active_count++;
            block_active_mem+=block_get_blocks(mr->b.b)*512-sizeof(struct block *);
            dbg(lvl_debug,"block ok");
            return 1;
        }
        dbg(lvl_info,"block not in cur_sel");
    }
}

int block_next(struct map_rect_priv *mr) {
    int blk_num,coord,r_h,r_w;
    struct block_bt_priv *bt=&mr->b.bt;
    struct coord_rect r;

    if (!mr->b.binarytree || ! mr->cur_sel)
        return block_next_lin(mr);
    for (;;) {
        if (! bt->p) {
            dbg(lvl_debug,"block 0x%x", bt->next);
            if (bt->next == -1)
                return 0;
            bt->b=block_get_byid(mr->file, bt->next, &bt->p);
            bt->end=(unsigned char *)mr->b.bt.b+block_get_size(mr->b.bt.b);
            bt->next=block_get_next(bt->b);
            bt->order=0;
            dbg(lvl_debug,"size 0x%x next 0x%x", block_get_size(bt->b), block_get_next(bt->b));
            if (! mr->b.bt.block_count) {
                block_get_r(bt->b, &bt->r);
                bt->r_curr=bt->r;
                coord=get_u32(&mr->b.bt.p);
            } else {
                bt->p=(unsigned char *)bt->b+0xc;
            }
            bt->block_count++;
        }
        while (mr->b.bt.p < mr->b.bt.end) {
            block_idx_count++;
            blk_num=get_u32(&mr->b.bt.p);
            coord=get_u32(&mr->b.bt.p);
            block_mem+=8;
            dbg(lvl_debug,"%p vs %p coord 0x%x ", mr->b.bt.end, mr->b.bt.p, coord);
            dbg(lvl_debug,"block 0x%x", blk_num);

            r_w=bt->r_curr.rl.x-bt->r_curr.lu.x;
            r_h=bt->r_curr.lu.y-bt->r_curr.rl.y;
            mr->b.b=NULL;
            if (blk_num != -1) {
                block_mem+=8;
                if (coord_rect_overlap(&mr->cur_sel->u.c_rect, &bt->r_curr)) {
                    mr->b.b=block_get_byid(mr->file, blk_num, &mr->b.p);
                    mr->b.block_num=blk_num;
                    dbg_assert(mr->b.b != NULL);
                    mr->b.block_start=(unsigned char *)(mr->b.b);
                    mr->b.p_start=mr->b.p;
                    mr->b.end=mr->b.block_start+block_get_size(mr->b.b);
                    block_get_r(mr->b.b, &r);
                    block_rect_same(&r, &bt->r_curr);
                }
            }
            if (coord != -1) {
                bt->stack[bt->stackp]=bt->r_curr;
                if (r_w > r_h) {
                    bt->r_curr.rl.x=coord;
                    bt->stack[bt->stackp].lu.x=coord+1;
                } else {
                    bt->r_curr.lu.y=coord;
                    bt->stack[bt->stackp].rl.y=coord+1;
                }
                bt->stackp++;
                dbg_assert(bt->stackp < BT_STACK_SIZE);
            } else {
                if (bt->stackp) {
                    bt->stackp--;
                    bt->r_curr=bt->stack[bt->stackp];
                } else {
                    bt->r_curr=bt->r;
                    bt->order++;
                    if (bt->order > 100)
                        return 0;
                }
            }
            if (mr->b.b) {
                block_active_count++;
                block_active_mem+=block_get_blocks(mr->b.b)*512;
                return 1;
            }
        }
        bt->p=NULL;
    }
    return 0;
}
