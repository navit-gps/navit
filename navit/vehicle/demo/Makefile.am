include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_demo
if PLUGINS
modulevehicle_LTLIBRARIES = libvehicle_demo.la
else
noinst_LTLIBRARIES = libvehicle_demo.la
endif
libvehicle_demo_la_SOURCES = vehicle_demo.c
libvehicle_demo_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
