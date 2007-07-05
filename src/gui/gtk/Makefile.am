include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @GTK2_CFLAGS@ -I$(top_srcdir)/src -DMODULE=\"gui_gtk\"
modulegui_LTLIBRARIES = libgui_gtk.la
libgui_gtk_la_SOURCES = destination.c gui_gtk_statusbar.c gui_gtk_action.c gui_gtk_window.c gui_gtk.h
libgui_gtk_la_LIBADD = @GTK2_LIBS@
