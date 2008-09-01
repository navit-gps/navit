include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = -I$(top_srcdir)/navit @NAVIT_CFLAGS@ @DIRECTFB_CFLAGS@ -DMODULE=gui_directfb
modulegui_LTLIBRARIES = libgui_directfb.la
libgui_directfb_la_SOURCES = gui_directfb.c gui_directfb.h
libgui_directfb_la_LIBADD = @DIRECTFB_LIBS@
