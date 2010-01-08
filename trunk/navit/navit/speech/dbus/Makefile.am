include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=speech_dbus
modulespeech_LTLIBRARIES = libspeech_dbus.la
libspeech_dbus_la_SOURCES = speech_dbus.c
libspeech_dbus_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
