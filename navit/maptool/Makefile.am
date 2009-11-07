include $(top_srcdir)/Makefile.inc
if !SUPPORT_ANDROID
  bin_PROGRAMS = maptool
endif

AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit @ZLIB_CFLAGS@ @POSTGRESQL_CFLAGS@
maptool_SOURCES = buffer.c coastline.c geom.c itembin.c maptool.c misc.c osm.c sourcesink.c tempfile.c tile.c zip.c maptool.h
maptool_LDADD = ../libnavit.la @NAVIT_LIBS@ @WORDEXP_LIBS@ @ZLIB_LIBS@ @POSTGRESQL_LIBS@ @INTLLIBS@ @LIBC_LIBS@
