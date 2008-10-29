#ifndef _LOCALE_H
#define _LOCALE_H       1
#define SUBLANG_BENGALI_BANGLADESH 0x02
#define SUBLANG_PUNJABI_PAKISTAN 0x02
#define SUBLANG_ROMANIAN_MOLDOVA 0x02
struct lconv {
	char *decimal_point;
};

#define LC_ALL 1
#define LC_NUMERIC 2
#endif
