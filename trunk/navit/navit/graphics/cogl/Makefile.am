include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @CLUTTER_CFLAGS@ @GLC_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=graphics_cogl
modulegraphics_LTLIBRARIES = libgraphics_cogl.la
libgraphics_cogl_la_SOURCES = graphics_cogl.c navit_actor.c navit_actor.h
libgraphics_cogl_la_LIBADD = @CLUTTER_LIBS@ @GLC_LIBS@
