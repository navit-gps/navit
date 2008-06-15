include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @SDL_CFLAGS@ @FREETYPE2_CFLAGS@ @FONTCONFIG_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=graphics_sdl
modulegraphics_LTLIBRARIES = libgraphics_sdl.la
libgraphics_sdl_la_SOURCES = graphics_sdl.c
libgraphics_sdl_la_LDFLAGS = @SDL_LIBS@ @SDL_IMAGE_LIBS@ @SDL_GFX_LIBS@ @FREETYPE2_LIBS@ @FONTCONFIG_LIBS@ @IMLIB2_LIBS@
