include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @GD_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=graphics_gd
libgraphics_gd_la_LDFLAGS = @GD_LIBS@
if PLUGINS
modulegraphics_LTLIBRARIES = libgraphics_gd.la
else
noinst_LTLIBRARIES = libgraphics_gd.la
endif
libgraphics_gd_la_SOURCES = graphics_gd.c
