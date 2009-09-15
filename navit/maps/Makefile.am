include $(top_srcdir)/Makefile.inc

SAMPLE_MAP=osm_bbox_11.3,47.9,11.7,48.2

osm2navit_args=--attr-debug-level=5

maps_DATA = $(SAMPLE_MAP).bin $(SAMPLE_MAP).xml

samplemap: $(SAMPLE_MAP).bin

.osm.bin:
	echo "Converting osm map"
	$(top_builddir)/navit/osm2navit $(osm2navit_args) $@.tmp <$<
	mv $@.tmp $@

.osm_bz2.bin:
	echo "Converting osm map"
	bzcat $< | $(top_builddir)/navit/osm2navit $(osm2navit_args) $@.tmp
	mv $@.tmp $@

.bin.xml:
	echo '<map type="binfile" enabled="yes" data="$$NAVIT_SHAREDIR/maps/$<"/>' >$@

	
$(SAMPLE_MAP).osm_bz2:
	echo "Downloading osm sample map"
	wget -O $(SAMPLE_MAP).osm_bz2.tmp http://www.navit-project.org/maps/$(SAMPLE_MAP).osm.bz2
	mv $(SAMPLE_MAP).osm_bz2.tmp $(SAMPLE_MAP).osm_bz2

distclean-local: 
	rm -f $(SAMPLE_MAP).osm.bz2 $(SAMPLE_MAP).bin $(SAMPLE_MAP).xml

all:
