include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=graphics_win32
if PLUGINS
modulegraphics_LTLIBRARIES = libgraphics_win32.la
else
noinst_LTLIBRARIES = libgraphics_win32.la
endif
libgraphics_win32_la_SOURCES = graphics_win32.c xpm2bmp.c
