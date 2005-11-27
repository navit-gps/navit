CFLAGS_LOCAL=-I. -I../speechd/src/clients
CFLAGS=$(CFLAGS_LOCAL) -I/usr/include/linc-1.0 $(shell pkg-config --cflags ORBit-2.0 gtk+-2.0)  -g -Wall # -Werror # -O2 
LDLIBS=$(shell pkg-config --libs ORBit-2.0 gtk+-2.0 gtkglext-1.0) fib-1.0/fib.so ../speechd/src/clients/libspeechd.lo 

SUBDIRS=plugins fib-1.0 gui graphics

OBJECTS=file.o param.o block.o display.o town.o street.o poly.o transform.o coord.o data_window.o route.o menu.o popup.o vehicle.o log.o command.o tree.o util.o map-common.o map-skels.o map-srv.o destination.o speech.o main.o phrase.o navigation.o cursor.o plugin.o map_data.o street_name.o country.o graphics.o graphics/gtk_gl_ext/gtk_gl_ext.o profile.o compass.o search.o 

all: $(SUBDIRS) map mapclient

map: $(OBJECTS) fib-1.0/fib.so gui/gtk/gtk.o graphics/gtk_drawing_area/gtk_drawing_area.o 
	$(CC) -g -o map $(OBJECTS) gui/gtk/gtk.o graphics/gtk_drawing_area/gtk_drawing_area.o $(LDLIBS)
map-static: $(OBJECTS)
	$(CC) -g -static -o map-static $(OBJECTS) $(LDLIBS)

mapclient: mapclient.o map-stubs.o map-common.o
	$(CC) -g -o mapclient mapclient.o map-stubs.o map-common.o $(shell pkg-config --libs ORBit-2.0)

map-common.c map-skels.c map-stubs.c map.h: map.idl
	orbit-idl-2 --skeleton-impl map.idl

clean:
	rm *.o map


.PHONY: $(SUBDIRS)
$(SUBDIRS):
	cd $@ && make

depend: .depend

.depend: *.c *.h
	[ -f .depend ] || touch -r Makefile .depend
	makedepend -f .depend -- $(CFLAGS) -- *.c *.h

include .depend
