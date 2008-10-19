include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=graphics_null
if PLUGINS
modulegraphics_LTLIBRARIES = libgraphics_null.la
else
noinst_LTLIBRARIES = libgraphics_null.la
endif
libgraphics_null_la_SOURCES = graphics_null.c
