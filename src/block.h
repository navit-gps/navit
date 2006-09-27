#include "coord.h"
#include "transform.h"

struct block {
	u32 blocks;
	u32 size;
	u32 next;
	struct coord c[2];
	int count;
};

struct block_info {
	struct map_data *mdata;	
	struct file *file;
	struct block *block;
	int block_number;
};

struct segment {
	struct block_info blk_inf;
	void *data[4];
};

struct param_list;

struct block * block_get(unsigned char **p);
void block_foreach_visible(struct block_info *blk_inf, struct transformation *t, int limit, void *data, 
	void(*func)(struct block_info *blk_inf, unsigned char *, unsigned char *, void *));
int block_get_param(struct block_info *blk_inf, struct param_list *param, int count);
struct block *block_get_byindex(struct file *file, int idx, unsigned char **p_ret);
