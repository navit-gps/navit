include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_android
modulevehicle_LTLIBRARIES = libvehicle_android.la
libvehicle_android_la_SOURCES = vehicle_android.c
libvehicle_android_la_LDFLAGS = -module -avoid-version @MODULE_LDFLAGS@
