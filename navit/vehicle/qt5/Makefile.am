include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_qt5
if PLUGINS
  modulevehicle_LTLIBRARIES = libvehicle_qt5.la
else
  noinst_LTLIBRARIES = libvehicle_qt5.la
endif
libvehicle_null_la_SOURCES = vehicle_qt5.cpp
libvehicle_null_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
