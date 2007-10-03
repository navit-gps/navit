include $(top_srcdir)/Makefile.inc
SUBDIRS=datafiles
AM_CPPFLAGS = @NAVIT_CFLAGS@ @CEGUI_CFLAGS@ @GLC_CFLAGS@ -I$(top_srcdir)/src -DMODULE=\"gui_sdl\"
modulegui_LTLIBRARIES = libgui_sdl.la
libgui_sdl_la_SOURCES = gui_sdl_window.cpp sdl_events.cpp gui_sdl.h sdl_events.h
libgui_sdl_la_LIBADD = @SDL_LIBS@ @CEGUI_LIBS@ @OPENGL_LIBS@ @GLC_LIBS@
