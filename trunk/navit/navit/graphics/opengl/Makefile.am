include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @OPENGL_CFLAGS@ @GLC_CFLAGS@ -I$(top_srcdir)/src -DMODULE=graphics_opengl
modulegraphics_LTLIBRARIES = libgraphics_opengl.la
libgraphics_opengl_la_SOURCES = graphics_opengl.c
libgraphics_opengl_la_LIBADD = @OPENGL_LIBS@ @GLC_LIBS@
