SUBDIRS = src man 

EXTRA_DIST = gps2shp pinatest2.gpx debian develop

debian-dist:
	dpkg-buildpackage -rfakeroot -d -tc -us -uc

CLEANFILES = \
	pinatest2_wpt.shx \
	pinatest2_wpt.shp \
	pinatest2_wpt.dbf \
	pinatest2_trk.shx \
	pinatest2_trk.shp \
	pinatest2_trk.dbf \
	pinatest2_rte.shx \
	pinatest2_rte.shp \
	pinatest2_rte.dbf \
	pinatest2_meta.txt 

GPX2SHP=src/gpx2shp
check: $(GPX2SHP)
	$(GPX2SHP) || true
	$(GPX2SHP) --version
	$(GPX2SHP) -s pinatest2.gpx
	$(GPX2SHP) -e pinatest2.gpx
	$(GPX2SHP) -p pinatest2.gpx
	$(GPX2SHP) -v pinatest2.gpx > /dev/null
