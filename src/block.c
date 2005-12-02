#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include "file.h"
#include "block.h"
#include "data.h"


struct file_private {
	int binarytree;
};

struct block_index_item {
	unsigned long blocknum;
	unsigned long blocks;
};

struct block_index {
	unsigned long blocks;
        unsigned long size;
        unsigned long next;      
	struct block_index_item list[0];
};


struct block *
block_get(unsigned char **p)
{
	struct block *ret=(struct block *)(*p);
	*p += sizeof(*ret);
	return ret;
}

struct block *
block_get_byindex(struct file *file, int idx, unsigned char **p_ret)
{
	struct block_index *blk_idx;
	int blk_num,max;

	blk_idx=(struct block_index *)(file->begin+0x1000);
	max=(blk_idx->size-sizeof(struct block_index))/sizeof(struct block_index_item);
	while (idx >= max) {
		blk_idx=(struct block_index *)(file->begin+blk_idx->next*512);
		idx-=max;
	}
	blk_num=blk_idx->list[idx].blocknum;

	*p_ret=file->begin+blk_num*512;
	return block_get(p_ret);
}

int
block_binarytree_walk(struct block **block, unsigned char **p, struct coord *c, int ign, struct block_info *blk_inf, struct transformation *t, 
			void *data, void(*func)(struct block_info *, unsigned char *, unsigned char *, void *))
{
	struct coord ca[2],cb[2];
	struct block *blk;
	int blk_num,val,dx,dy;
	int ret=0;

	ca[0].x=c[0].x;
	ca[0].y=c[0].y;
	ca[1].x=c[1].x;
	ca[1].y=c[1].y;
	cb[0].x=c[0].x;
	cb[0].y=c[0].y;
	cb[1].x=c[1].x;
	cb[1].y=c[1].y;

	if (*p >= (unsigned char *)(*block)+(*block)->size) {
		*block=block_get_byindex(blk_inf->file, (*block)->next, p);
		*p-=20;
	}
	blk_num=get_long(p);
        val=get_long(p); 

	if (blk_num != -1)
		ret++;

	if (blk_num != -1 && (t == NULL || is_visible(t, c))) {
		unsigned char *t,*end;
		blk=block_get_byindex(blk_inf->file, blk_num, &t);
		if (c[0].x != blk->c[0].x || c[0].y != blk->c[0].y || c[1].x != blk->c[1].x || c[1].y != blk->c[1].y) {
			printf("ERROR3\n");
			printf("!= 0x%lx,0x%lx-0x%lx,0x%lx\n", blk->c[0].x,blk->c[0].y,blk->c[1].x,blk->c[1].y);
		}
		end=(unsigned char *)blk;
		end+=blk->size;
		blk_inf->block=blk;
		blk_inf->block_number=blk_num;
		(*func)(blk_inf, t, end, data);
	}

	if (val != -1) {
		dx=c[1].x-c[0].x;
		dy=c[0].y-c[1].y;
		if (dy > dx) {
			ca[0].y=val;
			cb[1].y=val+1;
		} else {
			ca[1].x=val;
			cb[0].x=val+1;		
		}
		ret+=block_binarytree_walk(block, p, ca, ign, blk_inf, t, data, func);
		ret+=block_binarytree_walk(block, p, cb, ign, blk_inf, t, data, func);
	}
	return ret;
}

void
block_file_private_setup(struct file *file)
{
	int len;
	unsigned char *p,*str,*t;
	struct file_private *file_priv;

	file_priv=malloc(sizeof(*file_priv));
	file->private=file_priv;
	memset(file_priv, 0, sizeof(*file_priv));

	p=file->begin+0x0c;
	while (*p) {
		str=get_string(&p);
		len=get_long(&p);
		t=p;
		if (! strcmp(str,"binaryTree")) {
			file_priv->binarytree=get_long(&t);
		}
		p+=len;
	}
}

void
block_foreach_visible_linear(struct block_info *blk_inf, struct transformation *t, void *data, 
			void(*func)(struct block_info *blk_inf, unsigned char *, unsigned char *, void *))
{
	unsigned char *p,*start,*end;
	struct block *blk;

	blk_inf->block_number=0;
	p=blk_inf->file->begin+0x2000;
	while (p < blk_inf->file->end) {
		blk_inf->block_number++;
		start=p;
		end=p;
		blk=block_get(&p);
		end+=blk->size;
		if (blk->count == -1)
			break;
		if (t == NULL || is_visible(t, blk->c)) {
			blk_inf->block=blk;
			(*func)(blk_inf, p, end, data);
		}
		p=start+blk->blocks*512;
	}
}

void
block_foreach_visible(struct block_info *blk_inf, struct transformation *t, int limit, void *data,
			void(*func)(struct block_info *, unsigned char *, unsigned char *, void *))
{
	struct file_private *file_priv=blk_inf->file->private;

	if (! file_priv) {
		block_file_private_setup(blk_inf->file);
		file_priv=blk_inf->file->private;
	}
	if (! file_priv->binarytree) {
		block_foreach_visible_linear(blk_inf, t, data, func);
	} else {
		unsigned char *p,*p2;
		int dummy1,dummy2,xy,i,count;
		struct block *block=block_get_byindex(blk_inf->file, file_priv->binarytree, &p);
		p2=p;
		dummy1=get_long(&p2);
		if (block->count != -1 || dummy1 != -1) {
			printf("ERROR2 0x%x\n", block->count);
		}
		xy=1;
		p=p2;
		for (i = 0 ; i < limit ; i++) {
			p2=p;
			dummy1=get_long(&p2);
        		dummy2=get_long(&p2); 
			assert((dummy1 == -1 && dummy2 == -1) || i < 32) ;
			count=block_binarytree_walk(&block, &p, block->c, 0, blk_inf, t, data, func);
		}
	}
}

int
block_get_param(struct block_info *blk_inf, struct param_list *param, int count)
{
	int i=count;
	param_add_hex("Number", blk_inf->block_number, &param, &count);
	param_add_hex("Addr", (unsigned char *)blk_inf->block-blk_inf->file->begin, &param, &count);
	param_add_hex("Blocks", blk_inf->block->blocks, &param, &count);
	param_add_hex("Size", blk_inf->block->size, &param, &count);
	param_add_hex("Next", blk_inf->block->next, &param, &count);
	param_add_hex_sig("L", blk_inf->block->c[0].x, &param, &count);
	param_add_hex_sig("T", blk_inf->block->c[0].y, &param, &count);
	param_add_hex_sig("R", blk_inf->block->c[1].x, &param, &count);
	param_add_hex_sig("B", blk_inf->block->c[1].y, &param, &count);
	param_add_hex("W", blk_inf->block->c[1].x-blk_inf->block->c[0].x, &param, &count);
	param_add_hex("H", blk_inf->block->c[0].y-blk_inf->block->c[1].y, &param, &count);
	param_add_hex("Count", blk_inf->block->count, &param, &count);
	return i-count;
}
