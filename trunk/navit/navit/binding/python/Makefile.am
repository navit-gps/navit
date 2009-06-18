include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @PYTHON_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=binding_python
modulebinding_LTLIBRARIES = libbinding_python.la
libbinding_python_la_SOURCES = binding_python.c main.c navit.c pcoord.c route.c navigation.c attr.c common.h
libbinding_python_la_LIBADD = @PYTHON_LIBS@
libbinding_python_la_LDFLAGS = -module -avoid-version
