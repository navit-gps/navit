include $(top_srcdir)/Makefile.inc
SUBDIRS=datafiles
AM_CPPFLAGS = @NAVIT_CFLAGS@ @CEGUI_CFLAGS@ @GLC_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=gui_cegui
modulegui_LTLIBRARIES = libgui_cegui.la
libgui_cegui_la_SOURCES = gui_sdl_window.cpp sdl_events.cpp gui_sdl.h sdl_events.h wmcontrol.c wmcontrol.h cegui_keyboard.cpp cegui_keyboard.h
libgui_cegui_la_LIBADD = @SDL_LIBS@ @CEGUI_LIBS@ @OPENGL_LIBS@ @GLC_LIBS@
