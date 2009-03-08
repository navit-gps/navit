include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=map_shapefile
if PLUGINS
  modulemap_LTLIBRARIES = libmap_shapefile.la
else
  noinst_LTLIBRARIES = libmap_shapefile.la
endif
libmap_shapefile_la_SOURCES = shapefile.c dbfopen.c shpopen.c shptree.c shapefil.h
libmap_shapefile_la_LDFLAGS = -module
