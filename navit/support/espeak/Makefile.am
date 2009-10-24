include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -DMODULE=support_espeak
noinst_LTLIBRARIES = libsupport_espeak.la
libsupport_espeak_la_SOURCES = compiledict.c dictionary.c intonation.c \
	readclause.c setlengths.c numbers.c synth_mbrola.c \
	synthdata.c synthesize.c translate.c \
	tr_languages.c voices.c wavegen.c phonemelist.c klatt.c speak_init.c
libsupport_espeak_la_LDFLAGS = -module -avoid-version @MODULE_LDFLAGS@

#bin_PROGRAMS = speak

#speak_SOURCES = speak.c $(libsupport_espeak_la_SOURCES)

