SUBDIRS=libmdb
AUTOMAKE_OPTIONS = no-exeext
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I../.. -Ilibmdb/include -shared

objdir=$(prefix)
obj_PROGRAMS=plugin_poi_geodownload.so
INSTALL_PROGRAM=/bin/true

plugin_poi_geodownload_so_SOURCES=poi.c

plugin_poi_geodownload.so: $(plugin_poi_geodownload_so_OBJECTS)
	$(CC) $(CFLAGS) -shared -o $@ $(plugin_poi_geodownload_so_OBJECTS) -Llibmdb -lmdb


