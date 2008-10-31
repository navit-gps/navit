include $(top_srcdir)/Makefile.inc
SUBDIRS=
if ENABLE_NLS
SUBDIRS+=intl po
else
SUBDIRS+=navit
endif
pkgdoc_DATA = README
EXTRA_DIST = README COPYRIGHT
DIST_SUBDIRS=intl po navit
