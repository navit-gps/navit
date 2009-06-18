include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @GPSBT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_gpsd
modulevehicle_LTLIBRARIES = libvehicle_gpsd.la
libvehicle_gpsd_la_SOURCES = vehicle_gpsd.c
libvehicle_gpsd_la_LIBADD = @GPSD_LIBS@ @GPSBT_LIBS@
libvehicle_gpsd_la_LDFLAGS = -module -avoid-version
