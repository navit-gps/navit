#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(gpx2shp, 0.69, kogame at gmail.com)
AM_INIT_AUTOMAKE(gpx2shp, 0.69)
AC_CONFIG_SRCDIR(src/main.c)
AM_CONFIG_HEADER(src/config.h)

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET

# Ask GCC to give us heaps of warnings
if eval "test x$GCC = xyes"; then
	CFLAGS="$CFLAGS -W -Wall -Wcast-align -Wcast-qual"
	CFLAGS="$CFLAGS -Wmissing-declarations -Wmissing-prototypes"
	CFLAGS="$CFLAGS -Wstrict-prototypes -Wpointer-arith -Wreturn-type"
	AC_SUBST(CFLAGS)
fi

AC_ARG_ENABLE(coverage,
[  --enable-coverage       compile with coverage testing enabled],
              CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage")

# Checks for libraries.
AC_CHECK_LIB(proj, pj_param)
AC_CHECK_LIB(proj, pj_init, [], [ echo "Error you need lib proj4" && exit -1])
AC_CHECK_LIB(expat, XML_SetUserData)
AC_CHECK_LIB(m, cos)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(limits.h stdlib.h string.h unistd.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_CHECK_FUNCS(memset sqrt)

AC_CONFIG_FILES(Makefile src/Makefile man/Makefile)
AC_OUTPUT
