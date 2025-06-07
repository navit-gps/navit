Csv
===

The CSV map driver allows reading map items from a CSV file. It is
particularly useful for displaying custom points of interes.

.. _including_in_navit:

Including in Navit
==================

Includingn order for the points in the csv file to show up in Navit, the
map must be added to navit.xml. This can be accomplished by adding the
following line into the enabled (see `Configuration <Configuration>`__).

.. code:: xml

    <map type="csv" enabled="yes" data="poi.csv" item_type="poi_hotel" attr_types="id,position_longitude,position_latitude,label" />

Features to note from the previous example:

-  ``item_type``

   -  The type of item in each csv file is defined in the ``item_type``
      attribute of the above line, and can be anything from
   -  This of course means that each csv file (or map) can hold only
      **one** type of item. For example, a list of hotels
      (``item_type="poi_hotel"``) or fuel stations
      (``item_type="poi_fuel"``). You cannot mix different items within
      the same csv file.

-  ``attr_types``

   -  The corresponding attribute types for the columns of the csv file
      should be specified by the ``attr_types`` attribute inside the map
      tag. The value of that attribute should be a comma separated list
      of navit attributes in the order they appear in the csv file.
   -  You can define any attribute from
   -  The csv map definition should have ``position_longitude`` and
      ``position_latitude`` attributes in the ``attr_types`` list
      definition otherwise the csv file will not be loaded.

.. _adding_map_items_by_osd_commands:

Adding map items by osd commands
================================

One can add new items which are saved on exit to a csv map runtime by
OSD commands. Please see *map_add_curr_pos* command in the `OSD commands
description <On_Screen_Display#Commands>`__ section for an example.

.. _special_case_igo8_speed_camera_files:

Special Case: IGO8 (speed camera) files
---------------------------------------

The `speed_cam <OSD#speed_cam>`__ OSD item can use speed camera data
from IGO8 files, which are a type of csv file.

If your IGO8 file comes from http://speedcamerapoi.com/download.php, the
following line should be used:

.. code:: xml

   <map type="csv" enabled="yes" data="/path/to/igo8/file/speedcam.txt" item_type="tec_common" attr_types="position_longitude,position_latitude,tec_type,maxspeed,tec_dirtype,tec_direction" />

Sometimes, an IGO8 file has an ID column as its first column. Then the
following should be used:

.. code:: xml

   <map type="csv" enabled="yes" data="/path/to/igo8/file/speedcam.txt" item_type="tec_common" attr_types="id,position_longitude,position_latitude,tec_type,maxspeed,tec_dirtype,tec_direction" />

.. _the_csv_file:

The csv File
============

Contents of an example csv file is shown below, and corresponds to the
example line for navit.xml shown above.

.. code:: sql

   01,23.657,-46.7678,The Sunny Hotel
   02,23.457,-46.7878,Spa Hotel
   03,23.437,-46.2378,The Railway Hotel

Note:

-  No trailing comma
-  Each item is defined on a new line
-  Each column corresponds to the ``attr_types`` attribute in the
   ``<map ...>`` line in the example above.

.. _garmin_gpx_poi:

Garmin GPX POI
==============

Since the native GPX files (note: only fixed point like POI, not tracks
or waypoints) have the same layout as a csv file (without "id" column)
and it can be directly loaded using the csv method.

.. code:: xml

    <map type="csv" enabled="yes" data="PATH/TO/poi.gpx" item_type="poi_custom0" attr_types="position_longitude,position_latitude,label" />

Choose any of the poi_customX item_type and define a icon image using
the poi_custom0 poi-layer. Keep in mind that you can't have mixed POI's
in one file, or you have to add a extra column in the GPX file defining
the icon-image. Instead use different GPX files for each poi_type and
define the corresponding images with the poi_layer.

.. _android_issues:

Android Issues
==============

Currently, the Android builds do not include the required plugin line in
navit.xml, although the csv map driver itself is built. To enable csv
maps on Android, add the following line into the plugins section of your
navit.xml:

.. code:: xml

    <plugin path="$NAVIT_PREFIX/lib/libmap_csv.so" ondemand="no"/>

For more information, see .

.. _wince_issues:

WinCE Issues
============

You will need to use the full path to the CSV file, and not
$NAVIT_SHAREDIR

.. code:: xml

     <map type="csv" enabled="yes" data="\SDMMC\navit/maps/speedcam.txt" item_type="tec_common" attr_types="position_longitude,position_latitude,tec_type,maxspeed,tec_dirtype,tec_direction" />
