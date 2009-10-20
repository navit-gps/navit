include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_null
modulevehicle_LTLIBRARIES = libvehicle_null.la
libvehicle_null_la_SOURCES = vehicle_null.c
libvehicle_null_la_LDFLAGS = -module -avoid-version @MODULE_LDFLAGS@
