include $(top_srcdir)/Makefile.inc
if !SUPPORT_ANDROID
  bin_PROGRAMS = maptool
  noinst_LTLIBRARIES        = libmaptool.la
endif

AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit @ZLIB_CFLAGS@ @POSTGRESQL_CFLAGS@ -DMODULE=maptool
libmaptool_la_SOURCES = boundaries.c buffer.c ch.c coastline.c geom.c itembin.c itembin_buffer.c misc.c osm.c osm_o5m.c osm_psql.c osm_protobuf.c osm_protobufdb.c osm_xml.c sourcesink.c tempfile.c tile.c zip.c maptool.h generated-code/fileformat.pb-c.c generated-code/fileformat.pb-c.h generated-code/osmformat.pb-c.c generated-code/osmformat.pb-c.h google/protobuf-c/protobuf-c.c google/protobuf-c/protobuf-c.h google/protobuf-c/protobuf-c-private.h
maptool_SOURCES = maptool.c
maptool_LDADD = libmaptool.la ../libnavit.la @NAVIT_LIBS@ @WORDEXP_LIBS@ @ZLIB_LIBS@ @POSTGRESQL_LIBS@ @CRYPTO_LIBS@ @INTLLIBS@ @LIBC_LIBS@
