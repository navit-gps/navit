include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=map_binfile
if PLUGINS
  modulemap_LTLIBRARIES = libmap_binfile.la
else
  noinst_LTLIBRARIES = libmap_binfile.la
endif
libmap_binfile_la_SOURCES = binfile.c
libmap_binfile_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
