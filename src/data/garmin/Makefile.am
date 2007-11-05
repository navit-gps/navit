include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@   -I$(top_srcdir)/src -DMODULE=\"data_garmin\"
AM_CPPFLAGS+= @LIBGARMIN_CFLAGS@
INCLUDES=-I$(top_srcdir)/src/data/garmin/libgarmin
AM_CPPFLAGS+=-I$(top_srcdir)/src/data/garmin/libgarmin
AM_CPPFLAGS+=-I$(srcdir)

moduledata_LTLIBRARIES = libdata_garmin.la
libdata_garmin_la_SOURCES = garmin.c garmin.h gar2navit.c gar2navit.h
libdata_garmin_la_LIBADD = @LIBGARMIN_LIBS@
nodist_libdata_garmin_la_SOURCES = g2nbuiltin.h
builddir = @builddir@

gar2navit.l$(OBJEXT): g2nbuiltin.h

CLEANFILES = g2nbuiltin.h

noinst_PROGRAMS=gentypes
gentypes_SOURCES=gentypes.c

g2nbuiltin.h: gentypes garmintypes.txt
	./gentypes $(top_srcdir)/src/data/garmin/garmintypes.txt $@

EXTRA_DIST = garmintypes.txt

