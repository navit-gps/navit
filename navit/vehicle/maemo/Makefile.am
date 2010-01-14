include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @LIBLOCATION_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_maemo
modulevehicle_LTLIBRARIES = libvehicle_maemo.la
libvehicle_maemo_la_SOURCES = vehicle_maemo.c
libvehicle_maemo_la_LIBADD = @LIBLOCATION_LIBS@
libvehicle_maemo_la_LDFLAGS = -module -avoid-version
