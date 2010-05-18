include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_wince
if PLUGINS
modulevehicle_LTLIBRARIES = libvehicle_wince.la
else
noinst_LTLIBRARIES = libvehicle_wince.la
endif
libvehicle_wince_la_SOURCES = vehicle_wince.c
libvehicle_wince_la_LDFLAGS = -module -avoid-version
