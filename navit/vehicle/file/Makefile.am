include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit
modulevehicle_LTLIBRARIES = libvehicle_file.la libvehicle_pipe.la
libvehicle_file_la_CFLAGS=-DMODULE=vehicle_file
libvehicle_file_la_SOURCES = vehicle_file.c
libvehicle_file_la_LDFLAGS = -module
libvehicle_pipe_la_CFLAGS=-DMODULE=vehicle_pipe
libvehicle_pipe_la_SOURCES = vehicle_pipe.c
libvehicle_pipe_la_LDFLAGS = -module
