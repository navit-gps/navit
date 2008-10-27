include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @FREETYPE2_CFLAGS@ @FONTCONFIG_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=font_freetype
if PLUGINS
  modulefont_LTLIBRARIES = libfont_freetype.la
else
  noinst_LTLIBRARIES = libfont_freetype.la
endif
libfont_freetype_la_SOURCES = font_freetype.c
libfont_freetype_la_LDFLAGS = @FREETYPE2_LIBS@ @FONTCONFIG_LIBS@
