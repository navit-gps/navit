include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit
modulevehicle_LTLIBRARIES = libvehicle_file.la libvehicle_pipe.la libvehicle_socket.la libvehicle_serial.la
libvehicle_file_la_CFLAGS=-DMODULE=vehicle_file
libvehicle_file_la_SOURCES = vehicle_file.c
libvehicle_file_la_LDFLAGS = -module -avoid-version @NAVIT_SOCKET_LDFLAGS@ @NAVIT_MODULE_LDFLAGS@
libvehicle_pipe_la_CFLAGS=-DMODULE=vehicle_pipe
libvehicle_pipe_la_SOURCES = vehicle_pipe.c
libvehicle_pipe_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
libvehicle_socket_la_CFLAGS=-DMODULE=vehicle_socket
libvehicle_socket_la_SOURCES = vehicle_socket.c
libvehicle_socket_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
libvehicle_serial_la_CFLAGS=-DMODULE=vehicle_serial
libvehicle_serial_la_SOURCES = vehicle_serial.c
libvehicle_serial_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
