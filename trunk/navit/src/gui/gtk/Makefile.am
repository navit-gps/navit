AUTOMAKE_OPTIONS = no-exeext
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I../..

objdir=$(prefix)/dummy
obj_PROGRAMS=gtk.o
INSTALL_PROGRAM=$(INSTALL_DATA)

gtk_o_SOURCES=gtkeyboard.c gui_gtk_menu.c gui_gtk_statusbar.c gui_gtk_toolbar.c gui_gtk_window.c

gtk.o: $(gtk_o_OBJECTS)
	ld -r -o gtk.o $(gtk_o_OBJECTS)
