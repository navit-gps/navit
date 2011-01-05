include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=map_csv
if PLUGINS
  modulemap_LTLIBRARIES = libmap_csv.la
else
  noinst_LTLIBRARIES = libmap_csv.la
endif
libmap_csv_la_SOURCES = csv.c csv.h quadtree.c quadtree.h
libmap_csv_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
