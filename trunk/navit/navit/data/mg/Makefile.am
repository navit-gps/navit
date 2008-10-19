include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=data_mg
if PLUGINS
  moduledata_LTLIBRARIES = libdata_mg.la
else
  noinst_LTLIBRARIES = libdata_mg.la
endif
libdata_mg_la_SOURCES = map.c block.c town.c tree.c poly.c street.c mg.h
