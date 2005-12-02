AUTOMAKE_OPTIONS = no-exeext
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I../..

objdir=$(prefix)/dummy
obj_PROGRAMS=gtk_drawing_area.o
INSTALL_PROGRAM=$(INSTALL_DATA)

gtk_drawing_area_o_SOURCES=graphics_gtk_drawing_area.c

gtk_drawing_area.o: $(gtk_drawing_area_o_OBJECTS)
	ld -r -o gtk_drawing_area.o $(gtk_drawing_area_o_OBJECTS)
