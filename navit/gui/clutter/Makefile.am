include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = -I$(top_srcdir)/navit @NAVIT_CFLAGS@ @GLC_CFLAGS@ @CLUTTER_CFLAGS@ -DMODULE=gui_clutter
modulegui_LTLIBRARIES = libgui_clutter.la
libgui_clutter_la_SOURCES = gui_clutter_main.c gui_clutter_animators.c gui_clutter_animators.h gui_clutter_menuhack.c gui_clutter_menuhack.h navit_actor.h navit_actor.c
libgui_clutter_la_LIBADD = @CLUTTER_LIBS@
