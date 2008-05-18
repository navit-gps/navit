SUBDIRS=libmdb
include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -I$(srcdir)/libmdb/include -DMODULE=data_poi_geodownload
moduledata_LTLIBRARIES = libdata_poi_geodownload.la
libdata_poi_geodownload_la_SOURCES = poi_geodownload.c
libdata_poi_geodownload_la_LIBADD = -Llibmdb -lmdb
