include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=data_binfile
if PLUGINS
  moduledata_LTLIBRARIES = libdata_binfile.la
else
  noinst_LTLIBRARIES = libdata_binfile.la
endif
libdata_binfile_la_SOURCES = binfile.c
