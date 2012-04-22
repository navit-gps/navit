include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @SDL_CFLAGS@ @LIBPDL_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_webos
modulevehicle_LTLIBRARIES = libvehicle_webos.la
libvehicle_webos_la_SOURCES = vehicle_webos.c bluetooth.c cJSON.c
libvehicle_webos_la_LDFLAGS = @SDL_LIBS@ @LIBPDL_LIBS@ -module -avoid-version
