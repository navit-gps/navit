include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@   -I$(top_srcdir)/navit -DMODULE=data_garmin
AM_CPPFLAGS+= @LIBGARMIN_CFLAGS@
INCLUDES=-I$(top_srcdir)/navit/data/garmin/libgarmin
AM_CPPFLAGS+=-I$(top_srcdir)/navit/data/garmin/libgarmin
AM_CPPFLAGS+=-I$(srcdir)

moduledata_LTLIBRARIES = libdata_garmin.la
libdata_garmin_la_SOURCES = garmin.c garmin.h gar2navit.c gar2navit.h
libdata_garmin_la_LIBADD = @LIBGARMIN_LIBS@
nodist_libdata_garmin_la_SOURCES = g2nbuiltin.h
builddir = @builddir@

gar2navit.l$(OBJEXT): g2nbuiltin.h

CLEANFILES = g2nbuiltin.h

noinst_PROGRAMS=gentypes
gentypes_SOURCES=gentypes.c

gentypes.$(OBJEXT): gentypes.c
	if [ "$(cross_compiling)" = "yes" ]; then \
		$(CC_FOR_BUILD) -I$(top_srcdir)/navit -c -o $@ $<; \
	else \
		$(COMPILE) -c -o $@ $<; \
	fi

gentypes$(EXEEXT): $(gentypes_OBJECTS) $(gentypes_DEPENDENCIES) 
	@rm -f gentypes$(EXEEXT)
	if [ "$(cross_compiling)" = "yes" ]; then \
		$(CCLD_FOR_BUILD) -o $@ $(gentypes_OBJECTS); \
	else \
		$(LINK) $(gentypes_OBJECTS) $(LDADD) $(LIBS); \
	fi

g2nbuiltin.h: gentypes garmintypes.txt
	./gentypes $(top_srcdir)/navit/data/garmin/garmintypes.txt $@

EXTRA_DIST = garmintypes.txt

