include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -I$(top_srcdir)/navit/support -DMODULE=support_glib
noinst_LTLIBRARIES = libsupport_glib.la
libsupport_glib_la_SOURCES = fake.c galiasdef.c gatomic.c gerror.c \
  ghash.c glib_init.c glist.c gmem.c gmessages.c gprimes.c gprintf.c gslice.c \
  gslist.c gstrfuncs.c gstring.c gutils.c gutf8.c \
  fake.h galias.h gerror.h ghash.h glib.h glibconfig.h glibintl.h glist.h \
  gmacros.h gmem.h gmessages.h gprintf.h gprintfint.h gquark.h gslice.h \
  gslist.h gstrfuncs.h gthreadprivate.h gtypes.h
if GLIB_USE_OWN_PRINTF
  libsupport_glib_la_SOURCES += gnulib/asnprintf.c gnulib/printf-args.c gnulib/printf-args.h \
    gnulib/printf-parse.c gnulib/printf-parse.h gnulib/vasnprintf.c gnulib/vasnprintf.h gnulib/printf.c \
    gnulib/printf.h gnulib/g-gnulib.h
endif
