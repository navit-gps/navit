include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=speech_android
if PLUGINS
modulespeech_LTLIBRARIES = libspeech_android.la
else
noinst_LTLIBRARIES = libspeech_android.la
endif
libspeech_android_la_SOURCES = speech_android.c
libspeech_android_la_LDFLAGS = -module -avoid-version  -Xcompiler -nostdlib

android: libspeech_android.la
	mkdir -p ../../android/libs/armeabi/navit/speech
	cp .libs/libspeech_android.so ../../android/libs/armeabi

push: libspeech_android.la
	adb push .libs/libspeech_android.so /data/data/org.navitproject.navit/lib
