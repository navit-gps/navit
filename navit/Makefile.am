include $(top_srcdir)/Makefile.inc
comma=,
SUBDIRS=fib-1.1 support
if PLUGINS
  SUBDIRS += .
endif
DIST_SUBDIRS=autoload binding map maptool fib-1.1 font fonts gui graphics osd plugin speech support vehicle xpm maps
SUBDIRS+=autoload binding map font gui graphics osd plugin speech vehicle xpm

MODULES = $(wildcard $(top_builddir)/navit/binding/*/*.la $(top_builddir)/navit/font/*/*.la $(top_builddir)/navit/graphics/*/*.la $(top_builddir)/navit/gui/*/*.la $(top_builddir)/navit/map/*/*.la $(top_builddir)/navit/osd/*/*.la $(top_builddir)/navit/speech/*/*.la $(top_builddir)/navit/vehicle/*/*.la)

if FONTS
  SUBDIRS += fonts
endif
if !PLUGINS
  SUBDIRS += .
endif

if MAPTOOL
  SUBDIRS += maptool
endif

if BUILD_SAMPLEMAP
  SUBDIRS += maps
endif


AM_CPPFLAGS = -I$(top_srcdir)/navit/fib-1.1 @NAVIT_CFLAGS@ @ZLIB_CFLAGS@ -DPREFIX=\"@prefix@\" -DLIBDIR=\"@libdir@\" -DMODULE=navit
BUILT_SOURCES = version.h navit_config.h

if BIN_NAVIT
  bin_PROGRAMS = navit
endif

if SHARED_LIBNAVIT
  lib_LTLIBRARIES        = lib@LIBNAVIT@.la
  lib@LIBNAVIT@_la_LDFLAGS = -avoid-version @MODULE_LDFLAGS@ -no-undefined -Wl,--no-undefined
else
  noinst_LTLIBRARIES        = lib@LIBNAVIT@.la
endif
lib@LIBNAVIT@_la_LIBADD = @NAVIT_LIBS@ @WORDEXP_LIBS@ @ZLIB_LIBS@ @INTLLIBS@ -Lfib-1.1 -lfib @MODULE_LIBADD@ @CRYPTO_LIBS@

if SUPPORT_WIN32CE
  lib@LIBNAVIT@_la_LIBADD += -Lsupport/libc -lsupport_libc
endif

pkgdata_DATA = navit.xml

EXTRA_DIST = navit_shipped.xml navit.dtd

lib@LIBNAVIT@_la_SOURCES = announcement.c atom.c attr.c cache.c callback.c command.c compass.c config_.c coord.c country.c data_window.c debug.c \
	event.c event_glib.h file.c graphics.c gui.c item.c layout.c log.c main.c map.c \
	linguistics.c mapset.c maptype.c menu.c messages.c bookmarks.c bookmarks.h navit.c navigation.c osd.c param.c phrase.c plugin.c popup.c \
	profile.c projection.c roadprofile.c route.c routech.c search.c speech.c start_real.c transform.c track.c \
	util.c vehicle.c vehicleprofile.c xmlconfig.c announcement.h atom.h attr.h attr_def.h cache.h callback.h color.h command.h compass.h config_.h coord.h country.h \
	data.h data_window.h data_window_int.h debug.h destination.h draw_info.h endianess.h event.h \
	file.h graphics.h gtkext.h gui.h item.h item_def.h keys.h log.h layer.h layout.h linguistics.h main.h map-share.h map.h\
	map_data.h mapset.h maptype.h menu.h messages.h navigation.h navit.h osd.h \
	param.h phrase.h plugin.h point.h plugin_def.h projection.h popup.h route.h profile.h roadprofile.h search.h speech.h start_real.h \
	transform.h track.h util.h vehicle.h vehicleprofile.h window.h xmlconfig.h zipfile.h \
	navit_nls.h sunriset.c sunriset.h

XSLTS=@XSLTS@
navit.xml: navit_shipped.xml  $(foreach xslt, $(subst $(comma), ,$(XSLTS)), $(addsuffix .xslt,$(addprefix xslt/,$(xslt))))
	cp $< navit.xml.new
	if [ ! -f navit.dtd ]; then cp -f $(top_srcdir)/navit/navit.dtd .; fi
	for i in $^; do if [ "$${i%.xslt}" != "$$i" ]; then echo "Applying $$i" ; @SAXON@ -snone navit.xml.new $$i >navit.xml.tmp || exit ; mv navit.xml.tmp navit.xml.new || exit ; fi ; done
	mv navit.xml.new navit.xml
	rm -f navit.xml.tmp
	

if SUPPORT_ANDROID
  lib@LIBNAVIT@_la_SOURCES += android.c
  navit_SOURCES =
  navit_LDADD =

android/AndroidManifest.xml: android/AndroidManifest.xml.in
	mkdir -p $(builddir)/android
	sed '0,/<uses-sdk/!d' <$< >$@
	for i in $(subst $(comma), ,@ANDROID_PERMISSIONS@); do echo "    <uses-permission android:name=\"android.permission.$$i\" />" >>$@; done
	sed '0,/<uses-sdk/d' <$< >>$@
	

android/build.xml: android/AndroidManifest.xml
	mkdir -p android
	if [ "$(builddir)" != "$(srcdir)" ]; then cp -rp $(srcdir)/android/* $(builddir)/android; fi
	android update project -t 3 -p android

apkg-all: all android/build.xml navit.xml
	mkdir -p android/libs/armeabi
	cp .libs/*.so android/libs/armeabi/libnavit.so
	for i in */*/.libs/*.so ; do sed 's&lib_data_data_org\.navitproject\.navit_lib_&/data/data/org.navitproject.navit/lib/lib&' <$$i >android/libs/armeabi/$$(basename $$i); done
	for i in $(shell cd $(srcdir)/xpm && echo *.xpm); do convert $(srcdir)/xpm/$$i android/res/drawable/$${i%.xpm}.png; done 
	for i in $(shell cd $(srcdir)/xpm && echo *.png); do cp $(srcdir)/xpm/$$i android/res/drawable/$$( echo $$i | tr "[A-Z]" "[a-z]") ; done
	if [ "$(builddir)" != "$(srcdir)" ]; then for i in $(shell cd $(builddir)/xpm && echo *.png); do cp $(builddir)/xpm/$$i android/res/drawable/$$( echo $$i | tr "[A-Z]" "[a-z]") ; done ; fi
	mkdir -p android/res/raw
	for i in $(shell cd ../po && echo *.mo); do cp ../po/$$i android/res/raw/$$( echo $$i | tr "[A-Z]" "[a-z]") ; done
	cp navit.xml android/res/raw

apkg: apkg-all
	cd android && ant debug

apkg-release: apkg-all
	cd android && ant release
	jarsigner $$(cat ~/.keystore_args) android/bin/Navit-unsigned.apk mykey
#	zipalign -f 4  android/bin/Navit-unsigned.apk android/bin/Navit-signed.apk
else
navit_SOURCES = start.c
navit_LDADD = lib@LIBNAVIT@.la

endif

if EVENT_GLIB
  lib@LIBNAVIT@_la_SOURCES += event_glib.c
endif

if !PLUGINS
  lib@LIBNAVIT@_la_SOURCES += builtin.c
  lib@LIBNAVIT@_la_LIBADD += $(MODULES)
endif

navit_LDFLAGS = -export-dynamic

navit_LDADD += @NAVIT_LIBS@ @WORDEXP_LIBS@ @ZLIB_LIBS@ @CRYPTO_LIBS@ @INTLLIBS@

if SUPPORT_WIN32
  navit_LDADD += resource.rsc
if SUPPORT_WIN32CE
  navit_LDADD += -lcommctrl -Lsupport/libc
else
  navit_LDADD += -lgdi32 -lcomctl32 -lwinmm
endif


resource.rsc: $(top_srcdir)/navit/gui/win32/resources/resource.rc
	$(WINDRES) -I $(top_srcdir)/navit/gui/win32/resources $(top_srcdir)/navit/gui/win32/resources/resource.rc resource.rsc
endif

.PHONY: version.h.tmp

version.h.tmp:
	echo "#include \"config.h\"" >version.h.tmp
if SOURCE_MODE_SVN
	echo "#define SVN_VERSION \"$$(LANG=C svnversion $(top_srcdir) 2>/dev/null)\"" >>version.h.tmp
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
	if [ "$(top_builddir)" != "$(top_srcdir)" ]; then rm -f navit.dtd; fi

builtin.c: $(top_builddir)/config.h
	ls $(top_builddir)/navit/*/*/*.la | sed -e "s/\.la/_init(void);/" -e "s/.*lib/extern void module_/" >builtin.c
	echo "extern void builtin_init(void);" >>builtin.c
	echo "void builtin_init(void) {" >>builtin.c
	ls $(top_builddir)/navit/*/*/*.la | sed -e "s/\.la/_init();/" -e "s/.*\\/lib/	module_/" >>builtin.c
	echo "}" >>builtin.c
support-builtin.c: $(top_builddir)/config.h
	ls $(top_builddir)/navit/support/*/*.la | sed -e "s/\.la/_init(void);/" -e "s/.*lib/extern void module_/" >support-builtin.c
	echo "extern void builtin_init(void);" >>support-builtin.c
	echo "void builtin_init(void) {" >>support-builtin.c
	ls $(top_builddir)/navit/support/*/*.la | sed -e "s/\.la/_init();/" -e "s/.*\\/lib/	module_/" >>support-builtin.c
	echo "}" >>support-builtin.c
