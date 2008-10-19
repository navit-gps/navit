include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=osd_core
if PLUGINS
  moduleosd_LTLIBRARIES = libosd_core.la
else
  noinst_LTLIBRARIES = libosd_core.la
endif
libosd_core_la_SOURCES = osd_core.c
