static inline int street_get_bytes(struct block *blk)
{
	int bytes,dx,dy;
        bytes=2;
        dx=blk->c[1].x-blk->c[0].x;
        dy=blk->c[0].y-blk->c[1].y;
 
        if (dx > 32767 || dy > 32767)
                bytes=3;
        if (dx > 8388608 || dy > 8388608)
                bytes=4;                  
	
	return bytes;
}

static inline int street_get_coord(unsigned char **pos, int bytes, struct coord *ref, struct coord *f)
{
	unsigned char *p;
	int x,y,flags=0;
		
	p=*pos;
        x=*p++;
        x|=(*p++) << 8;
        if (bytes == 2) {
		if (   x > 0x7fff) {
			x=0x10000-x;
			flags=1;
		}
	}
	else if (bytes == 3) {
		x|=(*p++) << 16;
		if (   x > 0x7fffff) {
			x=0x1000000-x;
			flags=1;
		}
	} else {
		x|=(*p++) << 16;
		x|=(*p++) << 24;
		if (x < 0) {
			x=-x;
			flags=1;
		}
	}
	y=*p++;
	y|=(*p++) << 8;
	if (bytes == 3) {
		y|=(*p++) << 16;
	} else if (bytes == 4) {
		y|=(*p++) << 16;
		y|=(*p++) << 24;
	}
	f->x=ref[0].x+x;
	f->y=ref[1].y+y;         
#if 0
	printf("0x%x,0x%x + 0x%x,0x%x = 0x%x,0x%x", x, y, ref[0].x, ref[1].y, f->x, f->y);
#endif
	*pos=p;
	return flags;
}

