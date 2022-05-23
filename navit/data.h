/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef NAVIT_DATA_H
#define NAVIT_DATA_H

#include "config.h"

#ifdef WORDS_BIGENDIAN
#include <byteswap.h>
#endif

static inline unsigned char
get_u8(unsigned char **p)
{
	return *((*p)++);
}

static inline unsigned short
get_u16(unsigned char **p) {
	unsigned short ret;
	ret=*((unsigned short *)*p);
	*p+=sizeof(unsigned short);
#ifdef WORDS_BIGENDIAN
	return __bswap_16(ret);
#else
	return ret;
#endif
}

static inline unsigned short
get_u16_unal(unsigned char **p) {
	unsigned short ret;
	ret=*(*p)++;
	ret|=(*(*p)++) << 8;
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
get_u24_unal(unsigned char **p) {
	unsigned long ret;
	ret=get_u16_unal(p);
	ret|=*((*p)++) << 16;
	return ret;
}


static inline unsigned int
get_u32(unsigned char **p) {
	unsigned long ret;
	ret=*((unsigned int *)*p);
	*p+=sizeof(unsigned int);
#ifdef WORDS_BIGENDIAN
	return __bswap_32(ret);
#else
	return ret;
#endif
}

static inline unsigned int
get_u32_unal(unsigned char **p) {
	unsigned long ret;
	ret=*(*p)++;
	ret|=(*(*p)++) << 8;
	ret|=(*(*p)++) << 16;
	ret|=(*(*p)++) << 24;
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

#define L(x) ({ unsigned char *t=(unsigned char *)&(x); t[0] | (t[1] << 8) | (t[2] << 16) | (t[3] << 24); })

#endif

