AUTOMAKE_OPTIONS = no-exeext
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I../..

objdir=$(prefix)
obj_PROGRAMS=gtk_drawing_area.o
INSTALL_PROGRAM=/bin/true

gtk_drawing_area_o_SOURCES=graphics_gtk_drawing_area.c

gtk_drawing_area.o: $(gtk_drawing_area_o_OBJECTS)
	ld -r -o gtk_drawing_area.o $(gtk_drawing_area_o_OBJECTS)
