SUBDIRS=fib-1.1 data gui graphics xpm

AM_CPPFLAGS = -Ifib-1.1 @NAVIT_CFLAGS@ @SPEECHD_CFLAGS@ -DMODULE=\"navit\"


bin_PROGRAMS = navit

pkglib_DATA = navit.xml
EXTRA_DIST = navit.xml

navit_SOURCES = attr.c callback.c compass.c coord.c country.c cursor.c data_window.c debug.c \
	file.c graphics.c gui.c item.c layout.c log.c main.c map.c \
	mapset.c maptype.c menu.c navit.c navigation.c param.c phrase.c plugin.c popup.c \
	profile.c python.c route.c search.c speech.c transform.c track.c \
	util.c vehicle.c xmlconfig.c attr.h attr_def.h callback.h color.h compass.h coord.h country.h \
	cursor.h data.h data_window.h data_window_int.h debug.h destination.h draw_info.h \
	file.h graphics.h gtkext.h gui.h item.h item_def.h log.h layer.h layout.h main.h map-share.h map.h\
	map_data.h mapset.h maptype.h menu.h navigation.h navit.h \
	param.h phrase.h plugin.h point.h plugin_def.h projection.h popup.h route.h profile.h search.h speech.h statusbar.h \
	toolbar.h transform.h track.h util.h vehicle.h xmlconfig.h

navit_LDADD = @NAVIT_LIBS@ @SPEECHD_LIBS@ -Lfib-1.1 -lfib

