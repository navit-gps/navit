OpenStreetMap
-------------

`OpenStreetMap <http://www.openstreetmap.org/>` is a free editable map of the whole world. It is made by people like you. OpenStreetMap allows you to view, edit and use geographical data in a collaborative way from anywhere on Earth. Maps from OpenStreetMap can be used in Navit.


Quick Start
~~~~~~~~~~~

* Go to `Navit Planet Extractor <http://maps.navit-project.org/>`_
* Zoom in and select the area that you are interested in. Use the 'Map Controls' on the right to switch between 'Navigation' and 'Select' modes. or use
* preselected areas:

  * `Germany <http://maps.navit-project.org/api/map/?bbox=5.185546875,46.845703125,15.46875,55.634765625>`_
  * `France <http://maps.navit-project.org/api/map/?bbox=-5.44921875,42.01171875,8.437500000000002,51.6796875>`_
  * `BeNeLux <http://maps.navit-project.org/api/map/?bbox=2.08740234375,48.8671875,7.778320312500001,54.51416015625>`_
  * `Spain/Portugal <http://maps.navit-project.org/api/map/?bbox=-11.0302734375,34.87060546875,4.614257812500003,44.40673828125>`_
  * `Italy <http://maps.navit-project.org/api/map/?bbox=6.52587890625,36.38671875,18.96240234375,47.197265625>`_
  * `Entire planet <http://maps.navit-project.org/planet.bin>`_

* Hit "Get Map!"
* Move the downloaded map to the directory of your choice, and add it to the active the mapset (see [[Configuration]]) in navit.xml with a line similar to the following:

.. code-block:: xml

 <mapset>
  <map type="binfile" enabled="yes" data="/path/to/your/map/osm_bbox.bin" />
 </mapset>

Add OSM map to your mapset
~~~~~~~~~~~~~~~~~~~~~~~~~~
Move the downloaded map to the directory of your choice, and add it to the active mapset (see [[Configuration]]) in navit.xml with a line similar to the following:

.. code-block:: xml

 <mapset>
  <map type="binfile" enabled="yes" data="/path/to/your/map/my_downloaded_map.bin" />
 </mapset>

Topographic Maps
~~~~~~~~~~~~~~~~
Navit will display elevation/height lines but the required data is not included in most OSM derived maps.

Navit compatible maps with height lines can be created by feeding the output of `Phyghtmap <http://wiki.openstreetmap.org/wiki/Phyghtmap>` to Navit's maptool. Alternatively the SRTM data can be downloaded in osm.xml format http://geoweb.hft-stuttgart.de/SRTM/srtm_as_osm/, avoiding the Phygtmap step. The information can be either merged with OSM derived maps or used in a separate layer.

Many Garmin type maps such as http://www.wanderreitkarte.de/garmin_de.php also have the height lines information but routing will not work with them.

Processing OSM Maps yourself
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
You can create your own Navit binfiles from OSM data very easily using [[maptool]], the conversion program which installs alongside Navit. ''maptool'' can process both OpenStreetMap XML Data files (*.osm files) and OpenStreetMap Protobuf Data files (*.pbf files) Follow these steps to process your own maps.

Download your own OSM data
^^^^^^^^^^^^^^^^^^^^^^^^^^
OSM data can be downloaded from a variety of sources. OpenStreetMap XML Data files are regular textfiles, easily editable in any text editor. OpenStreetMap Protobuf Data files are binary files, which take up less space (so are quicker to download and process) but are not editable.

* OpenStreetMap XML Data

  * `Geofabrik <http://download.geofabrik.de/osm/>`_ provides pre-processed OpenStreetMap XML Data files of almost all countries, and all continents. This method is probably the easiest way of downloading OpenStreetMap XML Data for an entire country or continent. Note that the OSM files are bzipped
  * `planet.openstreetmap.org <http://planet.openstreetmap.org>`_ hosts the complete data set (the whole world). You can use `Osmosis <http://wiki.openstreetmap.org/index.php/Osmosis>`_ to cut it into smaller chunks.
  * `OpenStreetMap ReadOnly (XAPI) <http://wiki.openstreetmap.org/wiki/Xapi>`_ The API allows to get the data of a specific bounding box, so that download managers can be used. For example:
    wget -O map.osm "http://xapi.openstreetmap.org/api/0.6/map?bbox=11.4,48.7,11.6,48.9"
  * `OpenStreetMap (visual) <http://www.openstreetmap.org/export>`_ allows you to select a small rectangular area and download the selection as OpenStreetMap XML Data.

* '''OpenStreetMap Protobuf Data'''
  * [http://download.geofabrik.de/osm/ Geofabrik] provides pre-processed OpenStreetMap Protobuf Data files of almost all countries, and all continents.

Convert OSM data to Navit binfile
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The following examples assume that you have installed Navit system-wide. If this is not the case, you will need to provide an absolute path to the ''maptool'' executable, which is in the navit/maptool folder.

Please also note, that maptool uses country multipolygon relations. So it's a good idea to include the whole country boundary to your dataset. You can use the josm editor to download the country boundary relation and save it as osm file. Then this file can be concatenated with your sub-country level excerpt.

From .osm
"""""""""
.. code-block:: bash

  cat my_OSM_map.osm | maptool my_Navit_map.bin

Or

.. code-block:: bash

  maptool -i my_OSM_map.osm my_Navit_map.bin

Or for multiple OSM data files use the <tt>--dedupe-ways</tt> option to avoid duplication of way data if a way occurs multiple times in the OSM maps.

.. code-block:: bash

 cat my_OSM_map1.osm my_OSM_map2.osm my_OSM_map3.osm | maptool --dedupe-ways my_Navit_map.bin

From .bz2
"""""""""
.. code-block:: bash

  bzcat my_OSM_map.osm.bz2 | maptool my_Navit_map.bin

From .pbf
"""""""""
.. code-block:: bash

 maptool --protobuf -i my_OSM_map.osm.pbf my_Navit_map.bin

Processing the whole Planet
~~~~~~~~~~~~~~~~~~~~~~~~~~~
The OpenStreetMap wiki [http://wiki.openstreetmap.org/index.php/Planet.osm Planet.osm] page lists mirrors where Planet.osm can be downloaded. There are also downloads of smaller areas such as the UK and parts of Europe. These smaller excerpts are a lot quicker to download and process.

In case you want the whole planet.osm (24GB in December 2012), it is even possible to process planet.osm. It will take about 7 hours , requires > 1GB of main memory and about 30 GB disk space for result and temp files - planet.bin is currently (as of December 2012) 9.6GB:

.. code-block:: bash

 bzcat planet.osm.bz2 | maptool -6 my_Navit_map.bin

Please note -6 option (long name --64bit) used above. It should be used always if output bin file grows above 4GB, or generated file will not work at all. Using that option on smaller files slightly increases their size and makes them unreadable by some unzip versions.

Tips
~~~~
* To enable a map you have downloaded refer [[OpenStreetMap#Adding_an_OSM_map_to_your_mapset| adding OSM map to navit.xml]]
* If you don't see any map data in Navit (assuming your map is properly specified in navit.xml) using the Internal GUI click anywhere on the screen to bring up the menu. Click on "Actions" and then "Town". Type in the name of a town that should be within your map data. Select your town from the list that appears. This will bring up a sub-menu where you can click "View On Map". Note that if you have a GPS receiver you can also just wait till you get a satellite lock.
* To avoid changing navit.xml if you update your maps and the maps have different file names use the wildcard (\*.bin) in your navit.xml file. For example:

.. code-block:: xml

 <map type="binfile" enabled="yes" data="/media/mmc2/maps/*.bin"/>
