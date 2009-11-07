include $(top_srcdir)/Makefile.inc

.SECONDARY:

SAMPLE_MAP=osm_bbox_11.3,47.9,11.7,48.2

maptool_args=--attr-debug-level=5
maptool=$(top_builddir)/navit/maptool/maptool

maps_DATA = $(SAMPLE_MAP).bin $(SAMPLE_MAP).xml

SUFFIXES=.osm.bz2

samplemap: $(SAMPLE_MAP).bin

.osm.bin:
	echo "Converting osm map"
	$(maptool) $(maptool_args) $@.tmp <$<
	mv $@.tmp $@

.txt.bin:
	$(maptool) $(maptool_args) $@.tmp -p ../map/textfile/libmap_textfile.la -m $<
	mv $@.tmp $@

.osm.bz2.bin:
	echo "Converting osm map"
	bzcat $< | $(maptool) $(maptool_args) $@.tmp
	mv $@.tmp $@

.bin.xml:
	echo '<map type="binfile" enabled="yes" data="$$NAVIT_SHAREDIR/maps/$<"/>' >$@

	
$(SAMPLE_MAP).osm.bz2:
	echo "Downloading osm sample map"
	wget -O $(SAMPLE_MAP).osm.bz2.tmp http://www.navit-project.org/maps/$(SAMPLE_MAP).osm.bz2
	mv $(SAMPLE_MAP).osm.bz2.tmp $(SAMPLE_MAP).osm.bz2

distclean-local: 
	rm -f $(SAMPLE_MAP).osm.bz2 $(SAMPLE_MAP).bin $(SAMPLE_MAP).xml

all:
