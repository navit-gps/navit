include $(top_srcdir)/Makefile.inc
if !SUPPORT_ANDROID
  bin_PROGRAMS = maptool
  noinst_LTLIBRARIES        = libmaptool.la
endif

AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit @ZLIB_CFLAGS@ @POSTGRESQL_CFLAGS@ -DMODULE=maptool
libmaptool_la_SOURCES = buffer.c ch.c coastline.c geom.c itembin.c itembin_buffer.c misc.c osm.c sourcesink.c tempfile.c tile.c zip.c maptool.h
maptool_SOURCES = maptool.c
maptool_LDADD = libmaptool.la ../libnavit.la @NAVIT_LIBS@ @WORDEXP_LIBS@ @ZLIB_LIBS@ @POSTGRESQL_LIBS@ @CRYPTO_LIBS@ @INTLLIBS@ @LIBC_LIBS@
