AUTOMAKE_OPTIONS = no-exeext
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I../..

objdir=$(prefix)
obj_PROGRAMS=gtk.o
INSTALL_PROGRAM=/bin/true

gtk_o_SOURCES=gtkeyboard.c gui_gtk_statusbar.c gui_gtk_action.c gui_gtk_window.c

gtk.o: $(gtk_o_OBJECTS)
	ld -r -o gtk.o $(gtk_o_OBJECTS)
