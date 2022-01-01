OpenStreetMap
=============

`OpenStreetMap <http://www.openstreetmap.org>`__ is a free editable map
of the whole world. It is made by people like you. OpenStreetMap allows
you to view, edit and use geographical data in a collaborative way from
anywhere on Earth. Maps from OpenStreetMap can be used in Navit.

.. _quick_start:

Quick Start
-----------

#. Go to `Navit Planet Extractor <http://maps.navit-project.org/>`__
   (http://maps.navit-project.org/)
#. Zoom in and select the area that you are interested in. Use the 'Map
   Controls' on the right to switch between 'Navigation' and 'Select'
   modes. or use preselected areas:

-  `Germany <http://maps.navit-project.org/api/map/?bbox=5.185546875,46.845703125,15.46875,55.634765625>`__
-  `France <http://maps.navit-project.org/api/map/?bbox=-5.44921875,42.01171875,8.437500000000002,51.6796875>`__
-  `BeNeLux <http://maps.navit-project.org/api/map/?bbox=2.08740234375,48.8671875,7.778320312500001,54.51416015625>`__
-  `Spain/Portugal <http://maps.navit-project.org/api/map/?bbox=-11.0302734375,34.87060546875,4.614257812500003,44.40673828125>`__
-  `Italy <http://maps.navit-project.org/api/map/?bbox=6.52587890625,36.38671875,18.96240234375,47.197265625>`__
-  `Entire planet <http://maps.navit-project.org/planet.bin>`__ (curr.
   10GB!)

#. Hit **Get Map!**
#. Move the downloaded map to the directory of your choice, and add it
   to the active the mapset (see `Configuration <Configuration>`__) in
   navit.xml with a line similar to the following:

| 
| `` ``\ 
| 

.. _add_osm_map_to_your_mapset:

Add OSM map to your mapset
~~~~~~~~~~~~~~~~~~~~~~~~~~

Move the downloaded map to the directory of your choice, and add it to
the active mapset (see `Configuration <Configuration>`__) in navit.xml
with a line similar to the following:

| 
| `` ``\ 
| 

.. _topographic_maps:

Topographic Maps
----------------

Navit will display elevation/height lines but the required data is not
included in most OSM derived maps.

Navit compatible maps with height lines can be created by feeding the
output of Phyghtmap (http://wiki.openstreetmap.org/wiki/Phyghtmap,
http://katze.tfiu.de/projects/phyghtmap/) to Navit's maptool.
Alternatively the SRTM data can be downloaded in osm.xml format
http://geoweb.hft-stuttgart.de/SRTM/srtm_as_osm/, avoiding the Phygtmap
step. The information can be either merged with OSM derived maps or used
in a separate layer.

Many Garmin type maps such as
http://www.wanderreitkarte.de/garmin_de.php also have the height lines
information but routing will not work with them.

.. _processing_osm_maps_yourself:

Processing OSM Maps Yourself
----------------------------

You can create your own Navit binfiles from OSM data very easily using
`maptool <maptool>`__, the conversion program which installs alongside
Navit. *maptool* can process both OpenStreetMap XML Data files (*.osm
files) and OpenStreetMap Protobuf Data files (*.pbf files) Follow these
steps to process your own maps.

.. _download_your_own_osm_data:

Download your own OSM data
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. raw:: mediawiki

   {{note
   |Navit 0.2.0 cannot process OpenStreetMap Protobuf Data files - for this functionality, please upgrade to one of the [[Download_Navit| latest SVN builds or build from SVN source]]. Functionality was added in {{revision|3871}}.
   }}

OSM data can be downloaded from a variety of sources. OpenStreetMap XML
Data files are regular textfiles, easily editable in any text editor.
OpenStreetMap Protobuf Data files are binary files, which take up less
space (so are quicker to download and process) but are not editable.

-  **OpenStreetMap XML Data**

   -  `Geofabrik <http://download.geofabrik.de/osm/>`__ provides
      pre-processed OpenStreetMap XML Data files of almost all
      countries, and all continents. This method is probably the easiest
      way of downloading OpenStreetMap XML Data for an entire country or
      continent. Note that the OSM files are bzipped
   -  `planet.openstreetmap.org <http://planet.openstreetmap.org>`__
      hosts the complete data set (the whole world). You can use
      `Osmosis <http://wiki.openstreetmap.org/index.php/Osmosis>`__ to
      cut it into smaller chunks.
   -  `OpenStreetMap ReadOnly
      (XAPI) <http://wiki.openstreetmap.org/wiki/Xapi>`__ The API allows
      to get the data of a specific bounding box, so that download
      managers can be used. For example:

``wget -O map.osm "``\ ```http://xapi.openstreetmap.org/api/0.6/map?bbox=11.4,48.7,11.6,48.9`` <http://xapi.openstreetmap.org/api/0.6/map?bbox=11.4,48.7,11.6,48.9>`__\ ``"``

-  

   -  `OpenStreetMap (visual) <http://www.openstreetmap.org/export>`__
      allows you to select a small rectangular area and download the
      selection as OpenStreetMap XML Data.

-  **OpenStreetMap Protobuf Data**

   -  `Geofabrik <http://download.geofabrik.de/osm/>`__ provides
      pre-processed OpenStreetMap Protobuf Data files of almost all
      countries, and all continents.

.. _convert_osm_data_to_navit_binfile:

Convert OSM data to Navit binfile
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following examples assume that you have installed Navit system-wide.
If this is not the case, you will need to provide an absolute path to
the *maptool* executable, which is in the navit/maptool folder.

Please also note, that maptool uses country multipolygon relations. So
it's a good idea to include the whole country boundary to your dataset.
You can use the josm editor to download the country boundary relation
and save it as osm file. Then this file can be concatenated with your
sub-country level excerpt.

.. _from_.osm:

From .osm
^^^^^^^^^

``cat my_OSM_map.osm | maptool my_Navit_map.bin``

Or

``maptool -i my_OSM_map.osm my_Navit_map.bin``

Or for multiple OSM data files use the ``--dedupe-ways`` option to avoid
duplication of way data if a way occurs multiple times in the OSM maps.

``cat my_OSM_map1.osm my_OSM_map2.osm my_OSM_map3.osm | maptool --dedupe-ways my_Navit_map.bin``

.. _from_.bz2:

From .bz2
^^^^^^^^^

``bzcat my_OSM_map.osm.bz2 | maptool my_Navit_map.bin``

.. _from_.pbf:

From .pbf
^^^^^^^^^

*Functionality only available in SVN versions of Navit later than ,
which does not include the official Navit release 0.2.0*

``maptool --protobuf -i my_OSM_map.osm.pbf my_Navit_map.bin``

.. _processing_the_whole_planet:

Processing the whole Planet
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The OpenStreetMap wiki
`Planet.osm <http://wiki.openstreetmap.org/index.php/Planet.osm>`__ page
lists mirrors where Planet.osm can be downloaded. There are also
downloads of smaller areas such as the UK and parts of Europe. These
smaller excerpts are a lot quicker to download and process.

In case you want the whole planet.osm (24GB in December 2012), it is
even possible to process planet.osm. It will take about 7 hours ,
requires > 1GB of main memory and about 30 GB disk space for result and
temp files - planet.bin is currently (as of December 2012) 9.6GB:

``bzcat planet.osm.bz2 | maptool -6 my_Navit_map.bin``

Please note -6 option (long name --64bit) used above. It should be used
always if output bin file grows above 4GB, or generated file will not
work at all. Using that option on smaller files slightly increases their
size and makes them unreadable by some unzip versions.

Tips
----

.. raw:: mediawiki

   {{note
   |To increase performance of Navit use maps for only the country or state you are interested in.
   }}

-  To enable a map you have downloaded refer `adding OSM map to
   navit.xml <OpenStreetMap#Adding_an_OSM_map_to_your_mapset>`__
-  If you don't see any map data in Navit (assuming your map is properly
   specified in navit.xml) using the Internal GUI click anywhere on the
   screen to bring up the menu. Click on "Actions" and then "Town". Type
   in the name of a town that should be within your map data. Select
   your town from the list that appears. This will bring up a sub-menu
   where you can click "View On Map". Note that if you have a GPS
   receiver you can also just wait till you get a satellite lock.
-  To avoid changing navit.xml if you update your maps and the maps have
   different file names use the wildcard (*.bin) in your navit.xml file.
   For example:

.. _see_also:

See also
--------

-  `binfile <binfile>`__

.. _problems_with_osm:

Problems with OSM
-----------------

.. _search_doesnt_work:

Search doesn't work
~~~~~~~~~~~~~~~~~~~

Now maptool uses country boundary multipolygon relations for detecting
town membership. Sometimes this isn't an option, because the boundaries
are not closed or wrong.

When there's no multipolygon around the town, maptool will attempt to
use is_in tags. It only knows about a few OSM `is_in
tags <http://wiki.openstreetmap.org/wiki/Key:is_in>`__. Please help the
osm community to fix the country boundary.

.. _search_is_totally_b0rkz0r3d_or_behaves_very_strangely_vs_other_navigation_software.:

Search is totally b0rkz0r3d! Or, behaves very strangely vs other navigation software.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

OK, so the country you're searching **is** `listed in the
table <OpenStreetMap/countries>`__. Search probably will work. But be
aware there are some quirks.

First, there is no "Search" button--searches will **begin
automatically** as you fill in the gtk dialogue. Note there are minimum
string length requirements. For instance the Town search only
"activates" after 3 characters are typed, to prevent a huge number of
results (which would take a lot of CPU time to find and display). If you
really need to search for two character towns, you'll have to modify the
navit source code.

Also, Navit's search process **is hierarchical**. Meaning you must
search for and select a valid Country, then select a Town, then select a
Street. Right now, zipcode and district searches are unimplemented; only
Country, Town, Street and Housenumber actually work. Furthermore, there
is no US State or County information; when searching for a US town, you
may get hits from all over the US with planet.bin, and unfortunately,
the states that each hit is in are not displayed. You basically have to
use the Map button to examine them one-by-one, until you find the one
you want (or, only travel to towns with unique names).

The dialogue sometimes moves the cursor between text boxes
automatically. This can be pretty annoying.

Finally, there are a couple ways to crash (segfault) navit inside the
search dialogue.

The developers are aware of these issues and plan to improve behaviour
(this will happen faster if you send a patch!)

.. _search_for_my_town_works_but_i_cant_find_my_street:

Search for my town works, but I can't find my street
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Maptool since April, 2013 uses polygones and multipolygon relations
tagged with place=town/city/village etc to find place border.

Please note, there anyway should be a node tagged with the same name as
surrounding [multi]polygon for town to be findable. If town boundary is
found, it will be used as a limit for the street search.

Otherwise, city limit will be guessed by the place rank
(city/town/village). This will often lead to streets not being found or
found in neighboring places.

With the code improvements
`1 <http://navit.svn.sourceforge.net/viewvc/navit?view=revision&revision=5424>`__
of April 2013, maptool attempts to use admin_level=8 boundary relations
in Germany to find town borders, as these are not tagged with place=\*
tags in Germany. If your country doesn't use place=\* tags on place
boundaries, please report to trac which admin_level relations can be
used to get town boundaries.

For some countries, we have established a correspondence of navit
internal "State, county, district" terms to osm admin_level values. It
makes it easier to find your town if there are many duplicate names
without zip code. Report to trac if you have ideas how to extend this
mapping to your country.

The following countries have a mapping to OSM admin_level for boundaries
: Belgium, France, Germany, Poland, Russian federation, Switzerland, the
Netherlands

.. _routes_go_through_a_city_instead_around_of_it:

Routes go through a city instead around of it
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is because OSM doesn't provide any information whether a street is
rural (can be driven with higher speeds) or not. In that case all roads
are considered as in a city, which leads to using shorter routes through
a city. The **issue is in the OSM data**, not Navit routing. You may
want to fix the OSM data, in particular the speed limits (tag e.g.
maxspeed=50) for inner-city roads, which are e.g. 50 km/h even when they
are roads of higher order.

.. _there_are_strange_purple_objects_on_the_navit_map:

There are strange purple objects on the navit map
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These are objects in OSM which have no representation in navit. This
includes ways which have no tag and therefore are not visible in
osmarender/mapnik maps. If you want to have tagged objects included in
navit add an entry to the discussion what tags are used for this object
and what item from `Item_def.h <Item_def.h>`__ should be used (suggest a
new one together with how it should be rendered if no appropriate one
exists).
