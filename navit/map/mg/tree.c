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

struct tree_hdr {
    /*unsigned int addr;
    unsigned int size;
    unsigned int low;*/
    unsigned char p[12];
};
static inline unsigned int tree_hdr_get_addr(struct tree_hdr * tree) {
    unsigned char *p = tree->p;
    return get_u32(&p);
}
static inline unsigned int tree_hdr_get_size(struct tree_hdr * tree) {
    unsigned char *p = tree->p+4;
    return get_u32(&p);
}
static inline unsigned int tree_hdr_get_low(struct tree_hdr * tree) {
    unsigned char *p = tree->p+8;
    return get_u32(&p);
}

struct tree_hdr_h {
    /*	unsigned int addr;
    	unsigned int size;*/
    unsigned char p[8];
};
static inline unsigned int tree_hdr_h_get_addr(struct tree_hdr_h * tree) {
    unsigned char *p = tree->p;
    return get_u32(&p);
}
static inline unsigned int tree_hdr_h_get_size(struct tree_hdr_h * tree) {
    unsigned char *p = tree->p+4;
    return get_u32(&p);
}

struct tree_leaf_h {
    /*	unsigned int lower;
    	unsigned int higher;
    	unsigned int match;
    	unsigned int value;*/
    unsigned char p[16];
};
static inline unsigned int tree_leaf_h_get_lower(struct tree_leaf_h * tree) {
    unsigned char *p = tree->p;
    return get_u32(&p);
}
static inline unsigned int tree_leaf_h_get_higher(struct tree_leaf_h * tree) {
    unsigned char *p = tree->p+4;
    return get_u32(&p);
}
static inline unsigned int tree_leaf_h_get_match(struct tree_leaf_h * tree) {
    unsigned char *p = tree->p+8;
    return get_u32(&p);
}
static inline unsigned int tree_leaf_h_get_value(struct tree_leaf_h * tree) {
    unsigned char *p = tree->p+12;
    return get_u32(&p);
}


struct tree_hdr_v {
    /*unsigned int count;
    unsigned int next;
    unsigned int unknown;*/
    unsigned char p[12];
};
static inline unsigned int tree_hdr_v_get_count(struct tree_hdr_v * tree) {
    unsigned char *p = tree->p;
    return get_u32_unal(&p);
}
static inline unsigned int tree_hdr_v_get_next(struct tree_hdr_v * tree) {
    unsigned char *p = tree->p+4;
    return get_u32_unal(&p);
}
static inline unsigned int tree_hdr_v_get_unknown(struct tree_hdr_v * tree) {
    unsigned char *p = tree->p+8;
    return get_u32_unal(&p);
}

struct tree_leaf_v {
    unsigned char key;
    /*int value;*/
    unsigned char p[4];
} __attribute__((packed));
static inline int tree_leaf_v_get_value(struct tree_leaf_v * tree) {
    unsigned char *p = tree->p;
    return get_u32_unal(&p);
}

static int tree_search_h(struct file *file, unsigned int search) {
    unsigned char *p=file->begin,*end;
    int last,i=0,value,lower;
    struct tree_hdr_h *thdr;
    struct tree_leaf_h *tleaf;

    dbg(lvl_debug,"enter");
    while (i++ < 1000) {
        thdr=(struct tree_hdr_h *)p;
        p+=sizeof(*thdr);
        end=p+tree_hdr_h_get_size(thdr);
        dbg(lvl_debug,"@%td", p-file->begin);
        last=0;
        while (p < end) {
            tleaf=(struct tree_leaf_h *)p;
            p+=sizeof(*tleaf);
            dbg(lvl_debug,"low:0x%x high:0x%x match:0x%x val:0x%x search:0x%x", tree_leaf_h_get_lower(tleaf),
                tree_leaf_h_get_higher(tleaf), tree_leaf_h_get_match(tleaf), tree_leaf_h_get_value(tleaf), search);
            value=tree_leaf_h_get_value(tleaf);
            if (value == search)
                return tree_leaf_h_get_match(tleaf);
            if (value > search) {
                dbg(lvl_debug,"lower");
                lower=tree_leaf_h_get_lower(tleaf);
                if (lower)
                    last=lower;
                break;
            }
            last=tree_leaf_h_get_higher(tleaf);
        }
        if (! last || last == -1)
            return 0;
        p=file->begin+last;
    }
    return 0;
}

static int tree_search_v(struct file *file, int offset, int search) {
    unsigned char *p=file->begin+offset;
    int i=0,count,next;
    struct tree_hdr_v *thdr;
    struct tree_leaf_v *tleaf;
    while (i++ < 1000) {
        thdr=(struct tree_hdr_v *)p;
        p+=sizeof(*thdr);
        count=tree_hdr_v_get_count(thdr);
        dbg(lvl_debug,"offset=%td count=0x%x", p-file->begin, count);
        while (count--) {
            tleaf=(struct tree_leaf_v *)p;
            p+=sizeof(*tleaf);
            dbg(lvl_debug,"0x%x 0x%x", tleaf->key, search);
            if (tleaf->key == search)
                return tree_leaf_v_get_value(tleaf);
        }
        next=tree_hdr_v_get_next(thdr);
        if (! next)
            break;
        p=file->begin+next;
    }
    return 0;
}

int tree_search_hv(char *dirname, char *filename, unsigned int search_h, unsigned int search_v, int *result) {
    struct file *f_idx_h, *f_idx_v;
    char buffer[4096];
    int h,v;

    dbg(lvl_debug,"enter(%s, %s, 0x%x, 0x%x, %p)",dirname, filename, search_h, search_v, result);
    sprintf(buffer, "%s/%s.h1", dirname, filename);
    f_idx_h=file_create_caseinsensitive(buffer, 0);
    if ((!f_idx_h) || (!file_mmap(f_idx_h)))
        return 0;
    sprintf(buffer, "%s/%s.v1", dirname, filename);
    f_idx_v=file_create_caseinsensitive(buffer, 0);
    dbg(lvl_debug,"%p %p", f_idx_h, f_idx_v);
    if ((!f_idx_v) || (!file_mmap(f_idx_v))) {
        file_destroy(f_idx_h);
        return 0;
    }
    if ((h=tree_search_h(f_idx_h, search_h))) {
        dbg(lvl_debug,"h=0x%x", h);
        if ((v=tree_search_v(f_idx_v, h, search_v))) {
            dbg(lvl_debug,"v=0x%x", v);
            *result=v;
            file_destroy(f_idx_v);
            file_destroy(f_idx_h);
            dbg(lvl_debug,"return 1");
            return 1;
        }
    }
    file_destroy(f_idx_v);
    file_destroy(f_idx_h);
    dbg(lvl_debug,"return 0");
    return 0;
}

static struct tree_search_node *tree_search_enter(struct tree_search *ts, int offset) {
    struct tree_search_node *tsn=&ts->nodes[++ts->curr_node];
    unsigned char *p;
    p=ts->f->begin+offset;
    tsn->hdr=(struct tree_hdr *)p;
    tsn->p=p+sizeof(struct tree_hdr);
    tsn->last=tsn->p;
    tsn->end=p+tree_hdr_get_size(tsn->hdr);
    tsn->low=tree_hdr_get_low(tsn->hdr);
    tsn->high=tree_hdr_get_low(tsn->hdr);
    dbg(lvl_debug,"pos %td addr 0x%ux size 0x%ux low 0x%ux end %tu", p-ts->f->begin, tree_hdr_get_addr(tsn->hdr),
        tree_hdr_get_size(tsn->hdr), tree_hdr_get_low(tsn->hdr), tsn->end-ts->f->begin);
    return tsn;
}

int tree_search_next(struct tree_search *ts, unsigned char **p, int dir) {
    struct tree_search_node *tsn=&ts->nodes[ts->curr_node];

    if (! *p)
        *p=tsn->p;
    dbg(lvl_debug,"next *p=%p dir=%d", *p, dir);
    dbg(lvl_debug,"low1=0x%x high1=0x%x", tsn->low, tsn->high);
    if (dir <= 0) {
        dbg(lvl_debug,"down 0x%x", tsn->low);
        if (tsn->low != 0xffffffff) {
            tsn=tree_search_enter(ts, tsn->low);
            *p=tsn->p;
            tsn->high=get_u32(p);
            ts->last_node=ts->curr_node;
            dbg(lvl_debug,"saving last2 %d %td", ts->curr_node, tsn->last-ts->f->begin);
            dbg(lvl_debug,"high2=0x%x", tsn->high);
            return 0;
        }
        return -1;
    }
    tsn->low=tsn->high;
    tsn->last=*p;
    tsn->high=get_u32_unal(p);
    dbg(lvl_debug,"saving last3 %d %p", ts->curr_node, tsn->last);
    if (*p < tsn->end)
        return (tsn->low == 0xffffffff ? 1 : 0);
    dbg(lvl_debug,"end reached high=0x%x",tsn->high);
    if (tsn->low != 0xffffffff) {
        dbg(lvl_debug,"low 0x%x", tsn->low);
        tsn=tree_search_enter(ts, tsn->low);
        *p=tsn->p;
        tsn->high=get_u32_unal(p);
        ts->last_node=ts->curr_node;
        dbg(lvl_debug,"saving last4 %d %td", ts->curr_node, tsn->last-ts->f->begin);
        dbg(lvl_debug,"high4=0x%x", tsn->high);
        return 0;
    }
    return -1;
}

int tree_search_next_lin(struct tree_search *ts, unsigned char **p) {
    struct tree_search_node *tsn=&ts->nodes[ts->curr_node];
    int high;

    dbg(lvl_debug,"pos=%d %td", ts->curr_node, *p-ts->f->begin);
    if (*p)
        ts->nodes[ts->last_node].last=*p;
    *p=tsn->last;
    for (;;) {
        high=get_u32_unal(p);
        if (*p < tsn->end) {
            ts->last_node=ts->curr_node;
            while (high != 0xffffffff) {
                tsn=tree_search_enter(ts, high);
                dbg(lvl_debug,"reload %d",ts->curr_node);
                high=tsn->low;
            }
            return 1;
        }
        dbg(lvl_debug,"eon %d %td %td", ts->curr_node, *p-ts->f->begin, tsn->end-ts->f->begin);
        if (! ts->curr_node)
            break;
        ts->curr_node--;
        tsn=&ts->nodes[ts->curr_node];
        *p=tsn->last;
    }

    return 0;
}

void tree_search_init(char *dirname, char *filename, struct tree_search *ts, int offset) {
    char buffer[4096];
    sprintf(buffer, "%s/%s", dirname, filename);
    ts->f=file_create_caseinsensitive(buffer, 0);
    ts->curr_node=-1;
    if (ts->f) {
        file_mmap(ts->f);
        tree_search_enter(ts, offset);
    }
}

void tree_search_free(struct tree_search *ts) {
    if (ts->f)
        file_destroy(ts->f);
}
