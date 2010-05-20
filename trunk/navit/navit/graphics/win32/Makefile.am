include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @ZLIB_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=graphics_win32
libgraphics_win32_la_SOURCES = graphics_win32.c xpm2bmp.c
libgraphics_win32_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
if !SUPPORT_WIN32CE
libgraphics_win32_la_LDFLAGS += -Wl,-lgdi32
endif
if PLUGINS
modulegraphics_LTLIBRARIES = libgraphics_win32.la
else
noinst_LTLIBRARIES = libgraphics_win32.la
endif
