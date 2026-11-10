OpenStreetMap
-------------

`OpenStreetMap <http://www.openstreetmap.org/>` is a free editable map of the whole world. It is made by people like you. OpenStreetMap allows you to view, edit and use geographical data in a collaborative way from anywhere on Earth. Maps from OpenStreetMap can be used in Navit.


Quick Start
~~~~~~~~~~~

* Go to `Github Actions Mapserver <https://github.com/navit-gps/gh-actions-mapserver/releases/latest>`_
* Download the map you are interested in. Github does not show all artefacts on
  first page load, so be sure to use the dropdown to show all >200 maps
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

Complete coastlines for regional maps
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Regional extracts can omit coastline ways needed to distinguish land from
ocean. The historical coastline inference can therefore flood land, including
land in another loaded map. For complete ocean coverage, prepare water polygons
from a full-resolution global land snapshot and supply them to maptool::

    python3 -m pip install pyshp 'shapely>=2.0'
    python3 contrib/coastline-polygons.py \
        --land-shapefile /data/land-polygons-split-3857/land_polygons.shp \
        --bounds=-5.883925,47.07722,-1.0145379,49.414549 /data/bretagne-water.items
    maptool --protobuf -i /data/bretagne.osm.pbf \
        --ocean-polygons /data/bretagne-water.items /data/bretagne.bin

Download and extract ``land-polygons-split-3857.zip`` from
https://osmdata.openstreetmap.de/download/land-polygons-split-3857.zip.
Keep its README, projection and shapefile components together. Use the full
resolution dataset; the simplified zoom 0–9 dataset can put water on small
islands at navigation zooms. The data is copyright OpenStreetMap contributors,
available under the ODbL: https://www.openstreetmap.org/copyright.

Replace the example bounds with the desired ocean coverage rectangle, in
west,south,east,north order. The PBF header bounds, visible with
``osmium fileinfo --json``, are suitable for regional maps. The generator
supports bounds between 85 degrees south and north; split coverage at the
antimeridian. It computes ocean as coverage minus complete land, preserves
island holes, and validates the polygons after rounding to Navit's metre grid.
It records the source hash, date and output hash alongside the item file.

Use one pinned global snapshot for adjoining maps. Invalid source geometry or
an invalid item stream stops conversion; missing coverage never falls back to
inferred ocean. The prepared items replace the regional coastline inference.
Roads, inland water and other features still follow the ordinary conversion
path, and the result is one normal searchable, routable binfile. Python and the
geometry libraries are needed only on the conversion host.

Existing binfiles must be rebuilt. Omitting ``--ocean-polygons`` retains the
historical coastline inference. Ocean outside the requested coverage rectangle
is not supplied by this map. Keep the option when resuming conversion phases.

Incomplete inland water boundaries
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Open water, reservoir and basin ways are retained for multipolygon assembly,
but are not filled independently. Connected rings are classified by containment,
preserving islands and nested lakes. An inner ring without its enclosing shore
remains land.

When a water relation has a missing or unclosed inner boundary, maptool skips
that relation and reports its OSM URL. This deliberately omits its water too:
closing the gap or dropping the hole could flood an island. Check the full OSM
object and whether the extract includes every member before fixing source data.
No OSM object IDs are hardcoded, and boundaries are never borrowed from unrelated
ways. Closed rings which collapse at metre precision do not invalidate a relation.

Geometry near the date line and poles can extend beyond the legacy tile grid.
Maptool retains it and expands its index bounds while preserving tile names.
Rebuild existing binfiles to apply these inland water and map-edge fixes.

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
