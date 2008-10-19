include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = -I$(top_srcdir)/navit @NAVIT_CFLAGS@ -DMODULE=gui_win32
if PLUGINS
modulegui_LTLIBRARIES = libgui_win32.la
else
noinst_LTLIBRARIES = libgui_win32.la
endif
libgui_win32_la_SOURCES = gui_win32.c win32_gui_destination.c win32_gui_notify.c
libgui_win32_la_LIBADD = 
