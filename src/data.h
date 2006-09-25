static inline unsigned char
get_u8(unsigned char **p)
{
	return *((*p)++);
}

static inline unsigned short
get_u16(unsigned char **p) {
	unsigned long ret;
	ret=*((unsigned short *)*p);
	*p+=sizeof(unsigned short);
	return ret;
}

static inline unsigned int
get_u24(unsigned char **p) {
	unsigned long ret;
	ret=get_u16(p);
	ret|=*((*p)++) << 16;
	return ret;
}


static inline unsigned int
get_u32(unsigned char **p) {
	unsigned long ret;
	ret=*((unsigned int *)*p);
	*p+=sizeof(unsigned int);
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
