include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ @CEGUI_CFLAGS@ -I../.. 
plugin_LTLIBRARIES = libgui_sdl.la
libgui_sdl_la_SOURCES = gui_sdl_window.cpp sdl_events.cpp
libgui_sdl_la_LIBADD = @SDL_LIBS@ @CEGUI_LIBS@ @OPENGL_LIBS@
