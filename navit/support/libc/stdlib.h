/**
 * stdlib.h wrapper for WinCE — see navit-gps/navit#1499
 *
 * The WinCE cross-compiler (mingw32ce) provides broken static inline
 * stubs for getenv/setenv/unsetenv in its <stdlib.h> that always
 * return failure. These stubs are inlined into every translation unit,
 * making them impossible to override at link time.
 *
 * This wrapper renames the broken stubs before including the real
 * <stdlib.h>, then declares navit's working implementations from
 * support/libc/libc.c.
 */
#ifndef NAVIT_STDLIB_WRAPPER_H
#define NAVIT_STDLIB_WRAPPER_H

/* Rename broken inline stubs so they compile harmlessly */
#define getenv   _navit_broken_getenv
#define setenv   _navit_broken_setenv
#define unsetenv _navit_broken_unsetenv

#include_next <stdlib.h>

#undef getenv
#undef setenv
#undef unsetenv

/* Declare navit's working implementations (defined in libc.c) */
extern char *getenv(const char *name);
extern int setenv(const char *name, const char *value, int overwrite);
extern int unsetenv(const char *name);

#endif /* NAVIT_STDLIB_WRAPPER_H */
