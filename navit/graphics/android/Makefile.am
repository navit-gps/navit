include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=graphics_android
if PLUGINS
modulegraphics_LTLIBRARIES = libgraphics_android.la
else
noinst_LTLIBRARIES = libgraphics_android.la
endif
libgraphics_android_la_SOURCES = graphics_android.c
libgraphics_android_la_LDFLAGS = -module -avoid-version @MODULE_LDFLAGS@
