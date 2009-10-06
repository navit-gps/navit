include $(top_srcdir)/Makefile.inc
DIST_SUBDIRS=binding map fib-1.1 font fonts gui graphics osd speech support vehicle xpm maps
SUBDIRS=binding map fib-1.1 font gui graphics osd speech support vehicle xpm
if BUILD_SAMPLEMAP
  SUBDIRS += maps
endif

if FONTS
  SUBDIRS += fonts
endif

AM_CPPFLAGS = -I$(top_srcdir)/navit/fib-1.1 @NAVIT_CFLAGS@ @ZLIB_CFLAGS@ -DPREFIX=\"@prefix@\" -DLIBDIR=\"@libdir@\" -DMODULE=navit
BUILT_SOURCES = version.h navit_config.h
if BUILD_SAMPLEMAP
  BUILT_SOURCES += osm2navit$(EXEEXT)
endif

if SUPPORT_ANDROID
  lib_LTLIBRARIES        = libnavit.la
  libnavit_la_LDFLAGS = -module -avoid-version @MODULE_LDFLAGS@ -Wl,--no-undefined
  libnavit_la_LIBADD = @NAVIT_LIBS@ @WORDEXP_LIBS@ @ZLIB_LIBS@ @INTLLIBS@ -Lfib-1.1 -lfib -llog

else
  bin_PROGRAMS = navit osm2navit
  noinst_LTLIBRARIES        = libnavit.la
endif

pkgdata_DATA = navit.xml

EXTRA_DIST = navit_shipped.xml navit.dtd

libnavit_la_SOURCES = announcement.c atom.c attr.c cache.c callback.c command.c compass.c coord.c country.c cursor.c data_window.c debug.c \
	event.c event_glib.h file.c graphics.c gui.c item.c layout.c log.c main.c map.c \
	linguistics.c mapset.c maptype.c menu.c messages.c navit.c navigation.c osd.c param.c phrase.c plugin.c popup.c \
	profile.c projection.c roadprofile.c route.c search.c speech.c start_real.c transform.c track.c \
	util.c vehicle.c vehicleprofile.c xmlconfig.c announcement.h atom.h attr.h attr_def.h cache.h callback.h color.h command.h compass.h coord.h country.h \
	cursor.h data.h data_window.h data_window_int.h debug.h destination.h draw_info.h endianess.h event.h \
	file.h graphics.h gtkext.h gui.h item.h item_def.h keys.h log.h layer.h layout.h linguistics.h main.h map-share.h map.h\
	map_data.h mapset.h maptype.h menu.h messages.h navigation.h navit.h osd.h \
	param.h phrase.h plugin.h point.h plugin_def.h projection.h popup.h route.h profile.h roadprofile.h search.h speech.h start_real.h \
	transform.h track.h util.h vehicle.h vehicleprofile.h window.h xmlconfig.h zipfile.h \
	navit_nls.h

comma=,
XSLTS=@XSLTS@
navit.xml: navit_shipped.xml  $(foreach xslt, $(subst $(comma), ,$(XSLTS)), $(addsuffix .xslt,$(addprefix xslt/,$(xslt))))
	cp $< navit.xml.new
	if [ ! -f "$(builddir)/navit.dtd" ]; then cp -f $(srcdir)/navit.dtd $(builddir)/navit.dtd; fi
	for i in $^; do if [ "$${i%.xslt}" != "$$i" ]; then echo "Applying $$i" ; saxon -snone navit.xml.new $$i >navit.xml.tmp || exit ; mv navit.xml.tmp navit.xml.new || exit ; fi ; done
	mv navit.xml.new navit.xml
	rm -f navit.xml.tmp
	

if SUPPORT_ANDROID
  libnavit_la_SOURCES += android.c
  navit_SOURCES =
  navit_LDADD =
  osm2navit_SOURCES =
  osm2navit_LDADD =

android/build.xml: android/AndroidManifest.xml
	if [ "$(builddir)" != "$(srcdir)" ]; then cp -rp $(srcdir)/android $(builddir)/android; fi
	android update project -t 3 -p android

apk: all android/build.xml navit.xml
	mkdir -p android/libs/armeabi
	cp .libs/*.so */*/.libs/*.so android/libs/armeabi
	for i in $(shell cd $(srcdir)/xpm && echo *.xpm); do convert $(srcdir)/xpm/$$i android/res/drawable/$${i%.xpm}.png; done 
	for i in $(shell cd $(srcdir)/xpm && echo *.png); do cp $(srcdir)/xpm/$$i android/res/drawable/$$( echo $$i | tr "[A-Z]" "[a-z]") ; done
	if [ "$(builddir)" != "$(srcdir)" ]; then for i in $(shell cd $(builddir)/xpm && echo *.png); do cp $(builddir)/xpm/$$i android/res/drawable/$$( echo $$i | tr "[A-Z]" "[a-z]") ; done ; fi
	mkdir -p android/res/raw
	for i in $(shell cd ../po && echo *.mo); do cp ../po/$$i android/res/raw/$$( echo $$i | tr "[A-Z]" "[a-z]") ; done
	cp navit.xml android/res/raw
	cd android && ant debug

else
navit_SOURCES = start.c
navit_LDADD = libnavit.la @NAVIT_LIBS@ @WORDEXP_LIBS@ @ZLIB_LIBS@ @INTLLIBS@ -Lfib-1.1 -lfib

osm2navit_CPPFLAGS = $(AM_CPPFLAGS) @ZLIB_CFLAGS@ @POSTGRESQL_CFLAGS@
osm2navit_SOURCES = osm2navit.c
osm2navit_LDADD = libnavit.la @NAVIT_LIBS@ @ZLIB_LIBS@ @POSTGRESQL_LIBS@ @INTLLIBS@
endif

if EVENT_GLIB
  libnavit_la_SOURCES += event_glib.c
endif

if !PLUGINS
  navit_SOURCES += builtin.c
  navit_LDADD += $(wildcard $(top_builddir)/navit/*/*/*.la)
  osm2navit_SOURCES += support-builtin.c
  osm2navit_LDADD += $(wildcard $(top_builddir)/navit/support/*/*.la)
endif

if SUPPORT_WIN32
  navit_LDADD += resource.rsc
if SUPPORT_WIN32CE
  navit_LDADD += -lcommctrl
else
  navit_LDADD += -lgdi32 -lcomctl32
endif

resource.rsc: $(top_srcdir)/navit/gui/win32/resources/resource.rc
	$(WINDRES) -I $(top_srcdir)/navit/gui/win32/resources $(top_srcdir)/navit/gui/win32/resources/resource.rc resource.rsc
endif

.PHONY: version.h.tmp

version.h.tmp:
	echo "#include \"config.h\"" >version.h.tmp
if SOURCE_MODE_SVN
	echo "#define SVN_VERSION \"$$(LANG=C svnversion 2>/dev/null)\"" >>version.h.tmp
else
	echo "#define SVN_VERSION \"@SOURCE_MODE@\"" >>version.h.tmp
endif
	echo "#define NAVIT_VARIANT \"@NAVIT_VARIANT@\"" >>version.h.tmp
	if ! diff version.h.tmp version.h >/dev/null 2>/dev/null; \
	then \
		mv version.h.tmp version.h; \
	fi

version.h: version.h.tmp

navit_config.h: ../config.h
	cp ../config.h navit_config.h.tmp
	if ! diff navit_config.h.tmp navit_config.h >/dev/null 2>/dev/null; \
	then \
		mv navit_config.h.tmp navit_config.h; \
	fi


distclean-local:
	rm -f version.h version.h.tmp navit_config.h navit_config.h.tmp builtin.c support-builtin.c navit.xml
	if [ "$(builddir)" != "$(srcdir)" ]; then rm -f navit.dtd; fi

builtin.c: $(top_builddir)/config.h
	ls $(top_builddir)/navit/*/*/*.la | sed -e "s/.la/_init(void);/" -e "s/.*lib/extern void module_/" >builtin.c
	echo "extern void builtin_init(void);" >>builtin.c
	echo "void builtin_init(void) {" >>builtin.c
	ls $(top_builddir)/navit/*/*/*.la | sed -e "s/.la/_init();/" -e "s/.*\\/lib/	module_/" >>builtin.c
	echo "}" >>builtin.c
support-builtin.c: $(top_builddir)/config.h
	ls $(top_builddir)/navit/support/*/*.la | sed -e "s/.la/_init(void);/" -e "s/.*lib/extern void module_/" >support-builtin.c
	echo "extern void builtin_init(void);" >>support-builtin.c
	echo "void builtin_init(void) {" >>support-builtin.c
	ls $(top_builddir)/navit/support/*/*.la | sed -e "s/.la/_init();/" -e "s/.*\\/lib/	module_/" >>support-builtin.c
	echo "}" >>support-builtin.c
