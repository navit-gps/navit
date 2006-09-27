struct poly_hdr {
	unsigned char *addr;
	struct coord *c;
	char *name;
	unsigned char order;
	unsigned char type;
	u32 polys;
	u32 *count;
	u32 count_sum;
};

void poly_draw_block(struct block_info *blk_inf, unsigned char *p, unsigned char *end, void *data);
int poly_get_param(struct segment *seg, struct param_list *param, int count);
