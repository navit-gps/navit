include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @DBUS_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=binding_dbus
modulebinding_LTLIBRARIES = libbinding_dbus.la
libbinding_dbus_la_SOURCES = binding_dbus.c
libbinding_dbus_la_LIBADD = @DBUS_LIBS@
libbinding_dbus_la_LDFLAGS = -module -avoid-version

servicedir = @DBUS_SERVICE_DIR@
service_in_files = org.navit_project.navit.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	sed -e "s|\@bindir\@|$(bindir)|" $< > $@

EXTRA_DIST = $(service_in_files)
