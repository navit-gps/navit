include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=map_shapefile
if PLUGINS
  modulemap_LTLIBRARIES = libmap_shapefile.la
else
  noinst_LTLIBRARIES = libmap_shapefile.la
endif
if HAVE_SYSTEM_SHAPEFILELIB
libmap_shapefile_la_SOURCES = shapefile.c
libmap_shapefile_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@ @SHAPEFILE_LIBS@
else
libmap_shapefile_la_SOURCES = shapefile.c dbfopen.c shpopen.c shptree.c shapefil.h
libmap_shapefile_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
endif
