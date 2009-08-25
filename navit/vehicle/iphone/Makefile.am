include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_iphone
modulevehicle_LTLIBRARIES = libvehicle_iphone.la
libvehicle_iphone_la_SOURCES = vehicle_iphone.c corelocation.m
libvehicle_iphone_la_LDFLAGS = -module -Wl,-framework,CoreLocation
libvehicle_iphone_la_LIBADD = @IPHONE_LIBS@
