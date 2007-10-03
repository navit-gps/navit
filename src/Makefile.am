include $(top_srcdir)/Makefile.inc
SUBDIRS=binding data fib-1.1 gui graphics osd speech vehicle xpm

SAMPLE_MAP=osm_bbox_11.3,47.9,11.7,48.2

AM_CPPFLAGS = -I$(top_srcdir)/src/fib-1.1 @NAVIT_CFLAGS@ -DPREFIX=\"@prefix@\" -DMODULE=\"navit\"


bin_PROGRAMS = navit

pkgdata_DATA = navit.xml
maps_DATA=maps/$(SAMPLE_MAP).txt

EXTRA_DIST = navit.xml maps/$(SAMPLE_MAP).txt

navit_SOURCES = attr.c callback.c compass.c coord.c country.c cursor.c data_window.c debug.c \
	file.c graphics.c gui.c item.c layout.c log.c main.c map.c \
	mapset.c maptype.c menu.c navit.c navigation.c osd.c param.c phrase.c plugin.c popup.c \
	profile.c route.c search.c speech.c transform.c track.c \
	util.c vehicle.c xmlconfig.c attr.h attr_def.h callback.h color.h compass.h coord.h country.h \
	cursor.h data.h data_window.h data_window_int.h debug.h destination.h draw_info.h \
	file.h graphics.h gtkext.h gui.h item.h item_def.h log.h layer.h layout.h main.h map-share.h map.h\
	map_data.h mapset.h maptype.h menu.h navigation.h navit.h osd.h \
	param.h phrase.h plugin.h point.h plugin_def.h projection.h popup.h route.h profile.h search.h speech.h statusbar.h \
	transform.h track.h util.h vehicle.h xmlconfig.h

navit_LDADD = @NAVIT_LIBS@ @GPSD_LIBS@ -Lfib-1.1 -lfib

maps/$(SAMPLE_MAP).osm:
	echo "Downloading osm sample map"
	mkdir -p maps
	wget -O maps/$(SAMPLE_MAP).osm.tmp http://navit.sourceforge.net/maps/$(SAMPLE_MAP).osm
	mv maps/$(SAMPLE_MAP).osm.tmp maps/$(SAMPLE_MAP).osm

maps/$(SAMPLE_MAP).txt: maps/$(SAMPLE_MAP).osm $(top_srcdir)/src/script/osm2navit
	echo "Converting osm sample map"
	$(top_srcdir)/src/script/osm2navit <maps/$(SAMPLE_MAP).osm >maps/$(SAMPLE_MAP).txt.tmp
	mv maps/$(SAMPLE_MAP).txt.tmp maps/$(SAMPLE_MAP).txt

distclean-local:
	rm -f maps/$(SAMPLE_MAP).osm
