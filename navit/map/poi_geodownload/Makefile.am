SUBDIRS=libmdb
include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -I$(srcdir)/libmdb/include -DMODULE=map_poi_geodownload
modulemap_LTLIBRARIES = libmap_poi_geodownload.la
libmap_poi_geodownload_la_SOURCES = poi_geodownload.c
libmap_poi_geodownload_la_LIBADD = -Llibmdb -lmdb
