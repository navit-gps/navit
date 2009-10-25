include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=map_filter
if PLUGINS
  modulemap_LTLIBRARIES = libmap_filter.la
else
  noinst_LTLIBRARIES = libmap_filter.la
endif
libmap_filter_la_SOURCES = filter.c
libmap_filter_la_LDFLAGS = -module -avoid-version @MODULE_LDFLAGS@
