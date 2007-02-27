AUTOMAKE_OPTIONS = no-exeext
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -Iinclude

objdir=$(prefix)
obj_PROGRAMS=libmdb.a
INSTALL_PROGRAM=/bin/true

libmdb_a_SOURCES=backend.c catalog.c data.c dump.c file.c iconv.c index.c kkd.c like.c map.c mem.c money.c options.c props.c sargs.c stats.c table.c worktable.c write.c

libmdb.a: $(libmdb_a_OBJECTS)
	ar r $@ $(libmdb_a_OBJECTS)


