Textfile
========

Apart from the various map formats it can read, Navit also supports its
own textual map format. This is useful for converting some datas sources
(gpx, OSM) into something Navit can read without too much work. An
example is given in `Other maps <Other_maps>`__

Overview
========

You can define polygons/polylines and points.

Polygons/Polylines are specified as follows:

| ``type=<any type from item_def.h> attr1=xxx attr2="ab cd"``
| ``coordinate1``
| ``coordinate2``
| ``coordinate3``

"coordinate" can use various formats. The simplest is longitude /
latitude (note order) as decimal fractions, for example:

``-33.3553 6.334``

For the full list of formats see `Coordinate
format <Coordinate_format>`__.

If you want to have points after polylines/polygons, you have to add an
empty line after the polys Then:

``coordinate type=<any type from item_def.h> attr1=xxx attr2="ab cd"``

and so on, every line a point

You can also mix polys and points:

| ``type=<any type from item_def.h> attr1=xxx attr2="ab cd"``
| ``coordinate1 type=<any type from item_def.h> attr1=xxx attr2="ab cd"``
| ``coordinate2 type=<any type from item_def.h> attr1=xxx attr2="ab cd"``
| ``coordinate3 type=<any type from item_def.h> attr1=xxx attr2="ab cd"``

Confusing? Here's a sample :)

| ``type=rail debug="way_id=2953991 railway='rail' "``
| ``4821.199248 N 1056.437366 E``
| ``4821.194591 N 1056.596736 E``
| ``type=rail debug="way_id=2953991 railway='rail' "``
| ``4821.194591 N 1056.596736 E``
| ``4821.173691 N 1056.880243 E``

The above sample defines two segments used for a railway. You will
notice that the second coordinates of the first item and the first
coordinate of the second items are the same, to ensure a good
overlapping. This is not necessary, but ensures a consistent drawing.

Or here an example with decimal coordinates :

| ``type=image label=/image/raster001.jpg debug="raster001"``
| ``0.02527076695 47.22659264``
| ``0.09127025553 47.22659264 ``
| ``0.02527076695 47.27285604  ``

And here is one to display a point of interest:

| ``type=poi_custom1 icon_src=car_sharing label="Some car sharing place"``
| ``0.02527076695 47.22659264``

For a list of usable types see `Item_def.h <Item_def.h>`__.

.. _creating_it_out_of_an_osm_map:

Creating it out of an OSM map
=============================

You can convert OSM xml files to Navit textfiles using maptool. This is
only recommended for small OSM areas, as rendering a large textfile will
slow Navit down. To convert, execute something similar to the following:

``bzcat my_downloaded_map.osm.bz2 | maptool -D > navit_textfile_map.txt``

.. _creating_a_binfile_map_out_of_it:

Creating a binfile Map out of it
================================

**TODO:** *osm2navit* could do this (using -p
/path/to/libmap_textfile.so); find out if there's a way using *maptool*
as well.
