#include "types.h"

static inline u8
get_u8(unsigned char **p)
{
	return *((*p)++);
}

static inline u16
get_u16(unsigned char **p) {
	u16 ret;
	ret=*((u16 *)*p);
	*p+=sizeof(u16);
	return ret;
}

static inline u32
get_u24(unsigned char **p) {
	u32 ret;
	ret=get_u16(p);
	ret|=*((*p)++) << 16;
	return ret;
}


static inline u32
get_u32(unsigned char **p) {
	u32 ret;
	ret=*((u32 *)*p);
	*p+=sizeof(u32);
	return ret;
}

static inline char *
get_string(unsigned char **p)
{
        char *ret=(char *)(*p);
        while (**p) (*p)++;
        (*p)++;
        return ret;
}

