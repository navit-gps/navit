.. _navit_and_osm:

Navit and OSM
=============

.. raw:: mediawiki

   {{merge|OpenStreetMap}}

Navit has community project which is quite important for further osm
processing

The binary format driver uses the numeric values of attributes and items
and codes them into the map. So if you insert a attribute or item
somewhere in the middle, the ids of the following elements will change
and the map will be incompatible. So here are the tasks:

-  Find out which objects from osm we want to integrate into navit and
   compile a list of mapping osm -> navit, maybe defining new items and
   attributes
-  Bring a structure into these lists so new elements (we probably
   missed something or osm will extend the data) can be added without
   changing the ids of existing elements
-  attributes should be first sorted by type and then by function. So
   for example attributes with values between 0x10000 and 0x1ffff are of
   type "integer"

So you can easily add an integer attribute at the end of the list
without changing the values of the following attributes, because they
start with 0x20000

Similar for items, but here the structure should be multi-level I guess

``The raw structure is already hard-coded``

-  0-0x7ffffffff are points
-  0x80000000-0xbfffffff are lines
-  0xc0000000-0xffffffff are polygons
