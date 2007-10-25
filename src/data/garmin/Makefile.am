include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@   -I$(top_srcdir)/src -DMODULE=\"data_garmin\"
AM_CPPFLAGS+= @LIBGARMIN_CFLAGS@
INCLUDES=-I$(top_srcdir)/src/data/garmin/libgarmin
moduledata_LTLIBRARIES = libdata_garmin.la
libdata_garmin_la_SOURCES = garmin.c garmin.h gar2navit.c gar2navit.h
libdata_garmin_la_LIBADD = @LIBGARMIN_LIBS@
