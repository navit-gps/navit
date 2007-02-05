SUBDIRS=fib-1.0 gui graphics

fib-1.0 fib-1.0/Makefile.in: fib-1.0/Makefile.am
	cd fib-1.0
	./configure

AM_CPPFLAGS = @PACKAGE_CFLAGS@


bin_PROGRAMS = navit

navit_SOURCES = block.c command.c compass.c coord.c country.c cursor.c data_window.c debug.c destination.c \
	display.c file.c graphics.c log.c main.c map-common.c map-skelimpl.c map-skels.c map-srv.c \
	map-stubs.c map_data.c menu.c navigation.c osd.c param.c phrase.c plugin.c poly.c popup.c \
	profile.c route.c search.c speech.c street.c street_name.c town.c transform.c tree.c track.c \
	util.c vehicle.c block.h command.h compass.h config.h container.h coord.h country.h \
	cursor.h data.h data_window.h data_window_int.h debug.h destination.h display.h draw_info.h \
	file.h graphics.h gtkext.h log.h main.h map-share.h map.h map_data.h menu.h navigation.h \
	osd.h param.h phrase.h plugin.h point.h poly.h popup.h route.h search.h speech.h statusbar.h \
	street.h street_data.h street_name.h toolbar.h town.h transform.h tree.h track.h util.h vehicle.h

navit_LDADD = @PACKAGE_LIBS@ gui/gtk/gtk.o graphics/gtk_drawing_area/gtk_drawing_area.o -Lfib-1.0 -lfib

