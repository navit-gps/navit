include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = -I$(top_srcdir)/navit -I$(top_srcdir)/navit/binding/win32 @NAVIT_CFLAGS@ -DMODULE=binding_win32
if PLUGINS
modulebinding_LTLIBRARIES = libbinding_win32.la
else
noinst_LTLIBRARIES = libbinding_win32.la
endif
libbinding_win32_la_SOURCES = binding_win32.c binding_win32.h
libbinding_win32_la_LIBADD = 
libbinding_win32_la_LDFLAGS = -module -avoid-version
bin_PROGRAMS=tell_navit
tell_navit_LDADD = @NAVIT_LIBS@
