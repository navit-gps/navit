include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=map_mg
if PLUGINS
  modulemap_LTLIBRARIES = libmap_mg.la
else
  noinst_LTLIBRARIES = libmap_mg.la
endif
libmap_mg_la_SOURCES = map.c block.c town.c tree.c poly.c street.c mg.h
libmap_mg_la_LDFLAGS = -module
