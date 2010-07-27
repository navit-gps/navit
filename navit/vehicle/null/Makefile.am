include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_null
if PLUGINS
  modulevehicle_LTLIBRARIES = libvehicle_null.la
else
  noinst_LTLIBRARIES = libvehicle_null.la
endif
libvehicle_null_la_SOURCES = vehicle_null.c
libvehicle_null_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
