include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @DIRECTFB_CFLAGS@ @FREETYPE2_CFLAGS@ @FONTCONFIG_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=graphics_directfb
modulegraphics_LTLIBRARIES = libgraphics_directfb.la
libgraphics_directfb_la_SOURCES = graphics_directfb.c
libgraphics_directfb_la_LDFLAGS = @DIRECTFB_LIBS@ @FREETYPE2_LIBS@ @FONTCONFIG_LIBS@ @IMLIB2_LIBS@
