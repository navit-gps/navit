include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -I$(top_srcdir)/navit/support -DMODULE=support_glib
noinst_LTLIBRARIES = libsupport_glib.la
libsupport_glib_la_SOURCES = fake.c galiasdef.c gatomic.c gerror.c \
  ghash.c glib_init.c glist.c gmem.c gmessages.c gprimes.c gprintf.c gslice.c \
  gstrfuncs.c gstring.c gutils.c \
  fake.h galias.h gerror.h ghash.h glib.h glibconfig.h glibintl.h glist.h \
  gmacros.h gmem.h gmessages.h gprintf.h gprintfint.h gquark.h gslice.h \
  gstrfuncs.h gthreadprivate.h gtypes.h
