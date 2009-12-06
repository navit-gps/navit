include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=speech_espeak
modulespeech_LTLIBRARIES = libspeech_espeak.la
libspeech_espeak_la_SOURCES = speak.c 
libspeech_espeak_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@

