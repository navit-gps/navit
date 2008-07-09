include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_gypsy
modulevehicle_LTLIBRARIES = libvehicle_gypsy.la
libvehicle_gypsy_la_SOURCES = vehicle_gypsy.c
libvehicle_gypsy_la_LIBADD = @GYPSY_LIBS@
