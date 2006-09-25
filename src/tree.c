#include <stdio.h>
#include <string.h>
#include "file.h"
#include "map_data.h"
#include "data.h"
#include "tree.h"

struct tree_hdr {
	unsigned int addr;
	unsigned int size;
	unsigned int low;
};

struct tree_hdr_h {
	unsigned int addr;
	unsigned int size;
};

struct tree_leaf_h {
	unsigned int lower;
	unsigned int higher;
	unsigned int match;
	unsigned int value;
};

struct tree_hdr_v {
	unsigned int count;
	unsigned int next;
	unsigned int unknown;
};

struct tree_leaf_v {
	unsigned char key;
	int value;
} __attribute__((packed));

int
tree_compare_string(unsigned char *s1, unsigned char **s2_ptr)
{
	unsigned char *s2=*s2_ptr;
	char s1_exp, s2_exp;
	*s2_ptr+=strlen((char *)s2)+1;
	for (;;) {
		s1_exp=*s1++;
		s2_exp=*s2++;
		if (! s1_exp && ! s2_exp)
			return 0;
		if (s1_exp == 'A' && *s1 == 'E') { s1_exp='Ä'; s1++; }
		if (s1_exp == 'O' && *s1 == 'E') { s1_exp='Ö'; s1++; }
		if (s1_exp == 'U' && *s1 == 'E') { s1_exp='Ü'; s1++; }
		if (s2_exp == 'A' && *s2 == 'E') { s2_exp='Ä'; s2++; }
		if (s2_exp == 'O' && *s2 == 'E') { s2_exp='Ö'; s2++; }
		if (s2_exp == 'U' && *s2 == 'E') { s2_exp='Ü'; s2++; }
		if (s1_exp > s2_exp)
			return 2;
		if (s1_exp < s2_exp)
			return -2;
	}
}

int
tree_compare_string_partial(unsigned char *s1, unsigned char **s2_ptr)
{
	unsigned char *s2=*s2_ptr;
	char s1_exp, s2_exp;
	*s2_ptr+=strlen((char *)s2)+1;
	for (;;) {
		s1_exp=*s1++;
		s2_exp=*s2++;
		if (! s1_exp && ! s2_exp)
			return 0;
		if (s1_exp == 'A' && *s1 == 'E') { s1_exp='Ä'; s1++; }
		if (s1_exp == 'O' && *s1 == 'E') { s1_exp='Ö'; s1++; }
		if (s1_exp == 'U' && *s1 == 'E') { s1_exp='Ü'; s1++; }
		if (s2_exp == 'A' && *s2 == 'E') { s2_exp='Ä'; s2++; }
		if (s2_exp == 'O' && *s2 == 'E') { s2_exp='Ö'; s2++; }
		if (s2_exp == 'U' && *s2 == 'E') { s2_exp='Ü'; s2++; }
		if (! s1_exp)
			return -1;
		if (s1_exp > s2_exp)
			return 2;
		if (s1_exp < s2_exp)
			return -2;
	}
}


static int
tree_search_h(struct file *file, unsigned int search)
{
	unsigned char *p=file->begin,*end;
	int last,i=0,debug=0;
	struct tree_hdr_h *thdr;
	struct tree_leaf_h *tleaf;

	if (debug) {
		printf("tree_search_h\n");
	}
	while (i++ < 1000) {
		thdr=(struct tree_hdr_h *)p;
		p+=sizeof(*thdr);
		end=p+thdr->size;
		if (debug) {
			printf("@0x%x\n", p-file->begin);
		}
		last=0;
		while (p < end) {
			tleaf=(struct tree_leaf_h *)p;
			p+=sizeof(*tleaf);
			if (debug) {
				printf("low:0x%x high:0x%x match:0x%x val:0x%x search:0x%x\n", tleaf->lower, tleaf->higher, tleaf->match, tleaf->value, search);
			}
			if (tleaf->value == search)
				return tleaf->match;
			if (tleaf->value > search) {
				if (debug)
					printf("lower\n");
				if (tleaf->lower)
					last=tleaf->lower;
				break;
			}
			last=tleaf->higher; 
		}
		if (! last || last == -1)
			return 0;
		p=file->begin+last;
	}
	return 0;
}

static int
tree_search_v(struct file *file, int offset, int search)
{
	unsigned char *p=file->begin+offset;
	int i=0,count,debug=0;
	struct tree_hdr_v *thdr;
	struct tree_leaf_v *tleaf;
	while (i++ < 1000) {
		thdr=(struct tree_hdr_v *)p;
		p+=sizeof(*thdr);
		count=thdr->count;
		while (count--) {
			tleaf=(struct tree_leaf_v *)p;
			p+=sizeof(*tleaf);
			if (debug)
				printf("tree_search_v: 0x%x 0x%x\n", tleaf->key, search);
			if (tleaf->key == search)
				return tleaf->value;
		}
		if (! thdr->next)
			break;
		p=file->begin+thdr->next;
	}
	return 0;
		
}

/* return values */
/* 0=Next */
/* -2=Too low */
/* 1=Abort */
/* 2=Too high */

static int
tree_search(int version, struct file *file, unsigned char *p, int (*tree_func)(int version, int leaf, unsigned char **, struct map_data *mdat, void *), struct map_data *mdat, void *data, int higher)
{
	unsigned char *end,*psav;
	struct tree_hdr *thdr;
	int res=1,low;
	int high;
	int debug=0;
	int leaf=0;
	int retry=0;

	if (debug)
		printf("version %d\n", version);

	thdr=(struct tree_hdr *)p;	
	p+=sizeof(*thdr);
	low=thdr->low;
	end=p+thdr->size;
	if (debug)
		printf("Header: offset 0x%x size %d low 0x%x\n", thdr->addr, thdr->size, thdr->low);
	if (higher == -1)
		leaf=-1;
	if (higher == 1 && low != -1) {
		if (debug)
			printf("calling first higher tree 0x%x\n",low);
		res=tree_search(version,file,low+file->begin,tree_func,mdat,data,1);
		if (debug)
			printf("returning from first higher tree res=%d\n", res);
	}
	while (p < end) {
		psav=p;
		high=get_u32(&p);
		if (high == -1)
			leaf=1;
		res=tree_func(version, leaf, &p, mdat, data);
		leaf=0;
		if (debug) 
			printf("result: %d high 0x%x\n", res, high);
		if (res == 1) 
			return res;
		if (res < 0) {
			if (debug)
				printf("calling lower tree 0x%x\n",low);
			if (low != -1) {
				res=tree_search(version,file,low+file->begin,tree_func,mdat,data,0);
				if (debug)
					printf("returning from lower tree res=%d\n", res);
				if (! res && !retry++ ) {
					if (debug) 
						printf("retrying current leaf\n");
					p=psav;
					continue;
				}
				low=-1;
				if (res < 0)
					break;
			}
			else
				break;
		}
		retry=0;
		if (! res && high != -1) {
			if (debug)
				printf("high=0x%x\n", high);	
			if (debug)
				printf("calling higher tree 0x%x\n",high);
			res=tree_search(version,file,high+file->begin,tree_func,mdat,data,1);
			if (debug)
				printf("returning from higer tree res=%d\n", res);
			low=-1;
		} else {
			low=high;
		}
	}
	if (low != -1)
		res=tree_search(version,file,low+file->begin,tree_func,mdat,data,0);

	return res;
}

int
tree_search_map(struct map_data *mdat, int map, char *ext,
		int (*tree_func)(int, int, unsigned char **, struct map_data *mdat, void *), void *data)
		
{
	struct file *f_dat,*f_idx;
	char filename[4096];
	int len, version, ret;
	unsigned char *p;
	int debug=0;

	while (mdat) {
		f_dat=mdat->file[map];
		strcpy(filename, f_dat->name);
		len=strlen(filename);
		strcpy(filename+len-3,ext);	
		if (debug)
			printf("tree_search_map: filename='%s'\n", filename);
		f_idx=file_create(filename);
		version=1;
		p=f_idx->begin;
		if (!strncmp((char *)(p+4),"RootBlock",9)) {
			p+=0x1000;
			version=2;
		}
		ret=tree_search(version, f_idx, p, tree_func, mdat, data, -1);
		if (debug)
			printf("tree_search_map: ret=%d\n", ret);
		file_destroy(f_idx);
		if (ret == 1)
			return 1;
		mdat=mdat->next;
	}
	return 0;
}

struct tree_search_hv {
	struct map_data *mdat;
	struct file *f_idx_h;
	struct file *f_idx_v;
	struct tree_search_hv *next;
};

int
tree_search_hv_map(struct map_data *mdat, int map, unsigned int search1, unsigned int search2, int *result, struct map_data **mdat_result)
		
{
	struct file *f_dat,*f_idx_h, *f_idx_v;
	char filename[4096];
	int h,len,ret=0;
	int debug=0;

	if (debug)
		printf("tree_search_hv_map(0x%x 0x%x)\n",search1, search2);
	while (mdat && !ret) {
		f_dat=mdat->file[map];
		strcpy(filename, f_dat->name);
		len=strlen(filename);
		strcpy(filename+len-3,"h1");		
		f_idx_h=file_create(filename);
		strcpy(filename+len-3,"v1");
		f_idx_v=file_create(filename);
		h=tree_search_h(f_idx_h, search1);
		if (debug)
			printf("h=0x%x\n", h);
		if (h) {
			ret=tree_search_v(f_idx_v, h, search2);
			if (ret) {
				if (debug)
					printf("result 0x%x\n", ret);
				*result=ret;
				*mdat_result=mdat;
				file_destroy(f_idx_v);
				file_destroy(f_idx_h);
				return 0;
			}
		}
		file_destroy(f_idx_v);
		file_destroy(f_idx_h);
		mdat=mdat->next;
	}
	return 1;
}
