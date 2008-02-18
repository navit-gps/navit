include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @DBUS_CFLAGS@ -I$(top_srcdir)/src -DMODULE=\"binding_dbus\"
modulebinding_LTLIBRARIES = libbinding_dbus.la
libbinding_dbus_la_SOURCES = binding_dbus.c
libbinding_dbus_la_LIBADD = @DBUS_LIBS@
