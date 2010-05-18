include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=speech_espeak
if PLUGINS
modulespeech_LTLIBRARIES = libspeech_espeak.la
else
noinst_LTLIBRARIES = libspeech_espeak.la
endif
libspeech_espeak_la_SOURCES = speak.c 
libspeech_espeak_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@ -L$(top_builddir)/navit/support/espeak -lsupport_espeak -Wl,-lwinmm
