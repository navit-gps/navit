include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @LIBOSSO_CFLAGS@ -I$(top_srcdir) -I$(top_srcdir)/navit -DMODULE=plugin_osso
moduleautoload_LTLIBRARIES = libplugin_osso.la
libplugin_osso_la_LDFLAGS=-module -avoid-version @NAVIT_MODULE_LDFLAGS@ @LIBOSSO_LIBS@ #-Wl,--no-undefined

libplugin_osso_la_SOURCES = osso.c
