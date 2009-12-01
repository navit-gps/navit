include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @DBUS_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_gpsd_dbus
modulevehicle_LTLIBRARIES = libvehicle_gpsd_dbus.la
libvehicle_gpsd_dbus_la_SOURCES = vehicle_gpsd_dbus.c
libvehicle_gpsd_dbus_la_LIBADD = @DBUS_LIBS@
libvehicle_gpsd_dbus_la_LDFLAGS = -module -avoid-version
