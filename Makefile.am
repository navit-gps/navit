include $(top_srcdir)/Makefile.inc
SUBDIRS=src po intl
pkgdoc_DATA = README
EXTRA_DIST = README

all-local:
	@echo ------------------------
	@echo Navit build complete
	@echo To build the sample map:
	@echo make samplemap
	@echo ------------------------

samplemap:
	make -C src samplemap
