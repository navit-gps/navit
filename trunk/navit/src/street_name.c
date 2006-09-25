#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "map_data.h"
#include "street_name.h"
#include "file.h"
#include "block.h"
#include "data.h"
#include "tree.h"

void
street_name_get_by_id(struct street_name *name, struct map_data *mdat, unsigned long id)
{
	unsigned char *p;
        if (id) {
		p=mdat->file[file_strname_stn]->begin+id+0x2000;
		street_name_get(name, &p);
	}
}            

void
street_name_get(struct street_name *name, unsigned char **p)
{
	unsigned char *start=*p;
	name->len=get_u16(p);
	name->country=get_u16(p);
	name->townassoc=get_u32(p);
	name->name1=get_string(p);
	name->name2=get_string(p);
	name->segment_count=get_u32(p);
	name->segments=(struct street_name_segment *)(*p);
	(*p)+=(sizeof (struct street_name_segment))*name->segment_count;
	name->aux_len=name->len-(*p-start);
	name->aux_data=*p;
	name->tmp_len=name->aux_len;
	name->tmp_data=name->aux_data;
	(*p)+=name->aux_len;
}


struct street_name_index {
	unsigned short country;
	long town_assoc;
	char name[0];
} __attribute__((packed));

struct street_search_priv {
	struct street_name_index *search;
	int partial;
	struct block_offset last_leaf;
	struct map_data *last_mdat;
	int last_res;
};

static int street_name_compare(struct street_name_index *i1, struct street_name_index *i2, int partial)
{
	unsigned char c1_u,c2_u;
	int ret=0;
	int debug=0;

	if (i1->country > i2->country)
		ret=2;
	if (i1->country < i2->country)
		ret=-2;
	if (! ret) {
		if (debug)
			printf("town: %ld vs %ld\n", i1->town_assoc,i2->town_assoc);
		if (i1->town_assoc > i2->town_assoc)
			ret=2;
		if (i1->town_assoc < i2->town_assoc)
			ret=-2;
		if (! ret) {
			char *c1=i1->name;
			char *c2=i2->name;
			if (debug)
				printf("name: '%s' vs '%s'\n", c1, c2);
			for (;;) {
				c1_u=toupper(*c1);
				c2_u=toupper(*c2);	
				if (c1_u == c2_u && c1) {
					c1++;
					c2++;	
				} else {
					if (! c1_u && ! c2_u) {
						if (debug)
							printf("return 0: end of strings\n");
						ret=0;
						break;
					}
					if (c1_u == '\0' && c2_u == '\t') {
						if (debug)
							printf("return 0: delimiter found\n");
						ret=0;
						break;
					}
					if (c1_u == '\0' && partial) {
						ret=-1;
						break;
					}
					if (c1_u > c2_u) {
						ret=2;
						break;
					}
					if (c1_u < c2_u) {
						ret=-2;
						break;
					}
				}
			}
		}
	}
	return ret;
}

static int
street_name_tree_process(int version, int leaf, unsigned char **s2, struct map_data *mdat, void *data)
{
	int ret;
        struct street_search_priv *priv_data=data;
	struct block_offset *blk_off;
	int debug=0;

	blk_off=(struct block_offset *)(*s2);
	if (debug)
		printf("0x%x\n", get_u32(s2));
	else
		get_u32(s2);
	struct street_name_index *i1=priv_data->search;
	struct street_name_index *i2=(struct street_name_index *)(*s2);

	if (debug) {
		printf("Country %d %d\n",i1->country, i2->country);
		printf("Town_Assoc 0x%lx 0x%lx\n",i1->town_assoc, i2->town_assoc);
		printf("Name '%s' '%s'\n",i1->name, i2->name);
		printf("Leaf Data 0x%x 0x%x %d\n", blk_off->offset, blk_off->block, sizeof(*blk_off));
	}	
	*s2+=sizeof(*i2)+strlen(i2->name)+1;
	ret=street_name_compare(i1, i2, priv_data->partial);
	if (ret <= 0 && leaf == 1 && i1->country == i2->country && priv_data->last_res > 0) {
		if (debug)
			printf("street_tree_process: file='%s'\n", mdat->file[file_strname_stn]->name);
		priv_data->last_leaf=*blk_off;
		priv_data->last_mdat=mdat;
		priv_data->last_res=ret;
	}
	if (debug)
		printf("ret=%d\n", ret);
	return ret;
}

int
street_name_search(struct map_data *mdat, int country, int town_assoc, const char *name, int partial, int (*func)(struct street_name *name, void *data), void *data)
{
	struct street_search_priv priv;
	unsigned char idx_buffer[strlen(name)+1+sizeof(struct street_name_index)];
	struct street_name_index *idx=(struct street_name_index *)idx_buffer;
	struct block *blk;
	unsigned char *p,*end;
	int blk_num,res;
	struct street_name str_name;
	char buffer2[4096];
	struct street_name_index *idx2=(struct street_name_index *)buffer2;
	int ret,debug=0;

	priv.partial=partial;
	idx->country=country;
	idx->town_assoc=town_assoc;
	strcpy(idx->name, name);
	priv.search=idx;
	priv.last_res=1;
	priv.last_mdat=NULL;

	tree_search_map(mdat, file_strname_stn, "b1", street_name_tree_process, &priv);

#if 0
	if (debug)	
		printf("street_search_by_name: name='%s' leaf_data=0x%x priv=%p\n",name,priv.last_leaf.data,&priv);
#endif
	if (!priv.last_mdat)
		return 0;
	blk_num=priv.last_leaf.offset;
	if (debug) {
		printf("block_num 0x%x\n", blk_num);
		printf("file %p\n", priv.last_mdat->file[file_strname_stn]);
	}
	blk=block_get_byindex(priv.last_mdat->file[file_strname_stn], blk_num, &p);
	end=(unsigned char *)blk;
	end+=blk->size;

	p=(unsigned char *)blk;
	p+=12;
	if (debug)
		printf("p=%p\n",p);
	while (p < end) {
		street_name_get(&str_name, &p);
		idx2->country=str_name.country;
		idx2->town_assoc=str_name.townassoc;
		strcpy(idx2->name, str_name.name2);
		res=street_name_compare(idx,idx2,partial);
		if (res == 0 || res == -1) {
			if (debug)
				printf("Add res=%d '%s' '%s'\n",res,str_name.name1,str_name.name2);
			ret=(*func)(&str_name, data);
			if (ret)
				return 1;
		}
		if (res < -1)
			break;		
	}
	if (debug)
		printf("street_search_by_name: %p vs %p\n", p, end);
	return 0;
}


int
street_name_get_info(struct street_name_info *inf, struct street_name *name)
{
	unsigned char *p=name->tmp_data;
	
	if (name->tmp_len <= 0)
		return 0;
	inf->len=get_u16(&p);
	inf->tag=*p++;	
	inf->dist=get_u32(&p);
	inf->country=get_u32(&p);
	inf->c=coord_get(&p);
	inf->first=get_u24(&p);
	inf->last=get_u24(&p);
	inf->segment_count=get_u32(&p);
	inf->segments=(struct street_segment *)p;
	p+=sizeof(struct street_name_segment)*inf->segment_count;
	inf->aux_len=name->tmp_data+name->tmp_len-p;
	inf->aux_data=p;
	inf->tmp_len=inf->aux_len;
	inf->tmp_data=inf->aux_data;
	name->tmp_data+=inf->len;
	name->tmp_len-=inf->len;

	return 1;
}

int
street_name_get_number_info(struct street_name_number_info *num, struct street_name_info *inf)
{
	unsigned char *p=inf->tmp_data;

	if (inf->tmp_len <= 0)
		return 0;
	num->len=get_u16(&p);
	num->tag=*p++;	
	num->c=coord_get(&p);
	num->first=get_u24(&p);
	num->last=get_u24(&p);
	num->segment=(struct street_name_segment *)p;
	inf->tmp_data+=num->len;
	inf->tmp_len-=num->len;

	return 1;
}
