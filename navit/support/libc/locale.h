#ifndef _LOCALE_H
#define _LOCALE_H       1

#ifndef SUBLANG_BENGALI_BANGLADESH
#define SUBLANG_BENGALI_BANGLADESH 0x02
#endif
#ifndef SUBLANG_PUNJABI_PAKISTAN
#define SUBLANG_PUNJABI_PAKISTAN 0x02
#endif
#ifndef SUBLANG_ROMANIAN_MOLDOVA
#define SUBLANG_ROMANIAN_MOLDOVA 0x02
#endif
struct lconv {
	char *decimal_point;
};

#define LC_ALL 1
#define LC_NUMERIC 2
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
#endif
