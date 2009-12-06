include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=map_textfile
if PLUGINS
  modulemap_LTLIBRARIES = libmap_textfile.la
else
  noinst_LTLIBRARIES = libmap_textfile.la
endif
libmap_textfile_la_SOURCES = textfile.c textfile.h
libmap_textfile_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
