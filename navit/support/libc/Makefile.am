include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=support_libc
noinst_LTLIBRARIES = libsupport_libc.la
libsupport_libc_la_SOURCES = libc.c libc_init.c
if SUPPORT_WIN32CE
noinst_LTLIBRARIES += libc.la
libc_la_SOURCES =
endif
