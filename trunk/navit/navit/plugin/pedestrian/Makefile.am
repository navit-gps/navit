include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=plugin_pedestrian
moduleplugin_LTLIBRARIES = libplugin_pedestrian.la
libplugin_pedestrian_la_LDFLAGS=-module -avoid-version @NAVIT_MODULE_LDFLAGS@ #-Wl,--no-undefined

libplugin_pedestrian_la_SOURCES = pedestrian.c

android: libplugin_pedestrian.la
	cp .libs/libplugin_pedestrian.so ../../android/libs/armeabi
xslt:
	cp *.xslt ../../xslt
