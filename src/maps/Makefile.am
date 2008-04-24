include $(top_srcdir)/Makefile.inc

SAMPLE_MAP=osm_bbox_11.3,47.9,11.7,48.2

maps_DATA = $(SAMPLE_MAP).bin $(SAMPLE_MAP).xml

samplemap: $(SAMPLE_MAP).bin

$(SAMPLE_MAP).osm.bz2:
	echo "Downloading osm sample map"
	wget -O $(SAMPLE_MAP).osm.bz2.tmp http://navit.sourceforge.net/maps/$(SAMPLE_MAP).osm.bz2
	mv $(SAMPLE_MAP).osm.bz2.tmp $(SAMPLE_MAP).osm.bz2

$(SAMPLE_MAP).bin $(SAMPLE_MAP).xml: $(SAMPLE_MAP).osm.bz2 $(top_builddir)/src/osm2navit
	echo "Converting osm sample map"
	bzcat $(SAMPLE_MAP).osm.bz2 | $(top_builddir)/src/osm2navit --attr-debug-level=5 $(SAMPLE_MAP).bin.tmp
	mv $(SAMPLE_MAP).bin.tmp $(SAMPLE_MAP).bin
	echo '<map type="binfile" enabled="yes" data="$$NAVIT_SHAREDIR/maps/osm_bbox_11.3,47.9,11.7,48.2.bin"/>' >$(SAMPLE_MAP).xml

distclean-local:
	rm -f $(SAMPLE_MAP).osm.bz2 $(SAMPLE_MAP).bin

all:
