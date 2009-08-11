include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=speech_cmdline
modulespeech_LTLIBRARIES = libspeech_cmdline.la
libspeech_cmdline_la_SOURCES = speech_cmdline.c
libspeech_cmdline_la_LDFLAGS = -module -avoid-version @MODULE_LDFLAGS@
