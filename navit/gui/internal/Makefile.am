include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/navit -I$(top_srcdir)/navit/fib-1.1 @NAVIT_CFLAGS@ -DMODULE=gui_internal
if PLUGINS
modulegui_LTLIBRARIES = libgui_internal.la
else
noinst_LTLIBRARIES = libgui_internal.la
endif
libgui_internal_la_SOURCES = gui_internal_bookmark.c gui_internal.c gui_internal_command.c gui_internal_gesture.c gui_internal_html.c gui_internal_keyboard.c gui_internal_menu.c gui_internal_poi.c gui_internal_search.c gui_internal_widget.c gui_internal_bookmark.h gui_internal_command.h gui_internal_gesture.h gui_internal.h gui_internal_html.h gui_internal_keyboard.h gui_internal_menu.h gui_internal_poi.h gui_internal_priv.h gui_internal_search.h gui_internal_widget.h
libgui_internal_la_LIBADD = 
libgui_internal_la_LDFLAGS = -module -avoid-version @NAVIT_MODULE_LDFLAGS@
