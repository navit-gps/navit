include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @PYTHON_CFLAGS@ -I$(top_srcdir)/src -DMODULE=binding_python
modulebinding_LTLIBRARIES = libbinding_python.la
libbinding_python_la_SOURCES = binding_python.c
libbinding_python_la_LIBADD = @PYTHON_LIBS@
