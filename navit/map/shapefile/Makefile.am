include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=map_shapefile
if PLUGINS
  modulemap_LTLIBRARIES = libmap_shapefile.la
else
  noinst_LTLIBRARIES = libmap_shapefile.la
endif
libmap_shapefile_la_SOURCES = shapefile.c
libmap_shapefile_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
if HAVE_SYSTEM_SHAPEFILELIB
libmap_shapefile_la_LIBADD = @SHAPEFILE_LIBS@
else
AM_CPPFLAGS += -I$(top_srcdir)/navit/support/shapefile
libmap_shapefile_la_LIBADD = $(top_builddir)/navit/support/shapefile/libsupport_shapefile.la
endif
