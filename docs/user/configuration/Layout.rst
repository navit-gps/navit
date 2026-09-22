Layout
======

The way a map and the cursor (the thing that shows your current
location) is rendered in Navit is controlled by the particular Layout
which has been selected. As with almost everything else in Navit,
Layouts are highly configurable. For a description of the available
layout configuration options, see :doc:`Layout Options
</user/configuration/basic/layout>`. Below are user-submitted examples of
Navit Layouts.

Adding a Layout
---------------

If you want to share a layout with other Navit users, please do so using
this page. Leave this page as an introduction to your layout, use the
other layout descriptions below as a template, and add the full XML code
of your layout. For any questions or feedback, you can reach the Navit
team via the channels listed on the :doc:`/user/community/contacts` page.

Alternate Layouts
-----------------

Mapnik
~~~~~~

.. figure:: layout_osm_2.png
   :alt: Mapnik-style map layout
   :width: 300px

   Mapnik-style map layout

This layout tries to closely mimic the Mapnik rendering style used by
default over at `OpenStreetMap <http://www.openstreetmap.org>`__. It even
uses the same icon styles where available (this means you will have to
download the relevant icons - a link is provided). See
:doc:`Layout_mapnik` for the full description and XML.

Features
^^^^^^^^

-  Cycle ways are more prominently displayed in purple
-  Routing is shown with a bright green line which is overlaid onto the
   road, rather than a fat blue line underneath it. Road names appear
   above the routing line.
-  Zoom settings for various POIs have been changed. For example, fuel
   station POIs are shown out to quite a far zoom level.

   -  I have tried to make sure that POIs which will be most important
      to *navigating drivers* are prominently displayed, whilst those
      which are perhaps interesting but not very useful when navigating
      are less noticeable and/or only show up when zooming in closer.
      This is so that unhelpful POIs do not clutter up the map view.

-  Bus stops are shown with a blue ring, until zoomed in quite close
   when a proper icon is used.

   -  There are a lot of bus stops everywhere, and the POI icon was
      cluttering up the map. The unobtrusive blue ring is still
      noticeable, but less annoying!

-  Mini-roundabout icons have been removed, and are now shown by black
   rings.
-  A few POI types which do not appear in maptool's osm.c (i.e. don't
   actually get converted from OSM and won't currently appear in the
   Navit data) have been removed.

Mapnik for small screens
~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: 0606_Screenshot_2012-02-27-22-07-03.png
   :alt: Mapnik-style map layout for small screens
   :width: 300px

   Mapnik-style map layout for small screens

Based upon the original Mapnik style, these map layouts are optimised
for devices with smaller screens. There are two layouts available:

-  HDPI - for high definition small screens. See
   :doc:`Layout_mapnik-for-hdpi`.
-  MDPI - for medium definition small screens. See
   :doc:`Layout_mapnik-for-mdpi`.

Features
^^^^^^^^

-  Only for navigation important POIs are shown
-  Reduced the number of visible elements at higher zoom levels
-  Increased font sizes for town and street names

See also
^^^^^^^^

This map layout was developed together with the 0606.at OSD layout for
Android (see :ref:`at_android_gui` on the :doc:`Internal GUI
</user/configuration/Internal_GUI>` page). Check out the OSD layout for a
simple download package to install both this map layout and the OSD
layout, and all associated icons and POI image files.

Snow
~~~~

.. figure:: Snow.png
   :alt: Snow-style map layout
   :width: 300px

   Snow-style map layout

Snow style theme, optimised for the iPod/iPhone. See
:doc:`Layout_snow` for the full description and XML.

High-Visibility
~~~~~~~~~~~~~~~

.. figure:: Hi_vis2.jpg
   :alt: Hi-Vis style layout
   :width: 300px

   Hi-Vis style layout

Trying to read a computer screen in full sun with aging eyes and
sunglasses on while driving is difficult - this high-visibility layout
attempts to address those problems. See :doc:`Layout_hi_vis` for the
full description and XML.

Features
^^^^^^^^

-  Cursor changes:

   -  Enlarged
   -  Different colour

-  Altered route colour
-  Street changes

   -  street_1_city is brown with a beige border
   -  street_2_city is brown with a black border
   -  street_3_city is a orange with a grey border

-  Changed activation zoom levels for airport POI icons

Detailed Camping Biker
~~~~~~~~~~~~~~~~~~~~~~

.. figure:: Layout_Detailed_Camping_Biker.png
   :alt: Detailed Camping Biker layout
   :width: 300px

   Detailed Camping Biker layout

Many POIs are visible like housenumbers, Camping, Restaurants, Bakerys,
Shops for food, Peaks (with names), Unknown (points with names). Tracks,
Paths, hiking paths and so on have different colors to distinguish them.
See :doc:`Layout_detailedcampingbike` for the full description and XML.

bike
~~~~

Because all other layouts are not displaying bike paths properly on
winCE devices, this layout was rolled on its own. It is simple and needs
fixing and refining. Most POIs are hidden - this is one of the things
which should be changed. It is also not independent from the car layout
- another problem. See :doc:`Layout_bike` for the full XML.

Features
^^^^^^^^

-  bike paths visible green
-  less comfortable but still fine roads are visible in brown
-  everything else is much like car layout
-  no dashed lines - suitable for winCE

Alternate Cursors
-----------------

The cursor is also defined in the layout, and alternate cursors are
shown below. If you would like to share your alternate cursor design,
but haven't really changed the rest of the layout, add it below!

2D Car
~~~~~~

.. figure:: NavitScreenshot.jpg
   :alt: 2D car cursor
   :width: 300px

   2D car cursor

+----------------------------------------------------------+
| Layout XML                                               |
+==========================================================+
| .. code:: xml                                            |
|                                                          |
|    <cursor w="50" h="50">                                |
|        <itemgra>                                         |
|            <!-- Car outline -->                          |
|            <polyline color="#ffffff" width="12">         |
|                <coord x="-16" y="0"/>                    |
|                <coord x="15" y="0"/>                     |
|            </polyline>                                   |
|            <polyline color="#ffffff" width="5">          |
|                <coord x="-16" y="0"/>                    |
|                <coord x="-13" y="13"/>                   |
|                <coord x="13" y="13"/>                    |
|                <coord x="16" y="0"/>                     |
|            </polyline>                                   |
|            <polyline color="#ffffff" width="7">          |
|                <coord x="-10" y="-7"/>                   |
|                <coord x="-10" y="-12"/>                  |
|            </polyline>                                   |
|            <polyline color="#ffffff" width="7">          |
|                <coord x="10" y="-7"/>                    |
|                <coord x="10" y="-12"/>                   |
|            </polyline>                                   |
|            <!-- Car -->                                  |
|            <polyline color="#0000ff" width="10">         |
|                <coord x="-15" y="0"/>                    |
|                <coord x="15" y="0"/>                     |
|            </polyline>                                   |
|            <polyline color="#0000ff" width="3">          |
|                <coord x="-16" y="0"/>                    |
|                <coord x="-13" y="13"/>                   |
|                <coord x="13" y="13"/>                    |
|                <coord x="16" y="0"/>                     |
|            </polyline>                                   |
|            <polyline color="#0000ff" width="5">          |
|                <coord x="-10" y="-7"/>                   |
|                <coord x="-10" y="-12"/>                  |
|            </polyline>                                   |
|            <polyline color="#0000ff" width="5">          |
|                <coord x="10" y="-7"/>                    |
|                <coord x="10" y="-12"/>                   |
|            </polyline>                                   |
|        </itemgra>                                        |
|        <itemgra speed_range="-2">                        |
|            <!--Back lights off -->                       |
|            <circle color="#000000" radius="5" width="2"> |
|                <coord x="-12" y="0"/>                    |
|            </circle>                                     |
|            <circle color="#000000" radius="5" width="2"> |
|                <coord x="12" y="0"/>                     |
|            </circle>                                     |
|        </itemgra>                                        |
|        <itemgra speed_range="3-">                        |
|            <!--Back lights on -->                        |
|            <circle color="#ff0000" radius="5" width="2"> |
|                <coord x="-12" y="0"/>                    |
|            </circle>                                     |
|            <circle color="#ff0000" radius="5" width="2"> |
|                <coord x="12" y="0"/>                     |
|            </circle>                                     |
|            <!-- Speed lines -->                          |
|            <polyline color="#ffffff" width="3">          |
|                <coord x="-17" y="-12"/>                  |
|                <coord x="-20" y="-20"/>                  |
|            </polyline>                                   |
|            <polyline color="#000000" width="1">          |
|                <coord x="-17" y="-12"/>                  |
|                <coord x="-20" y="-20"/>                  |
|            </polyline>                                   |
|            <polyline color="#ffffff" width="3">          |
|                <coord x="-10" y="-16"/>                  |
|                <coord x="-13" y="-24"/>                  |
|            </polyline>                                   |
|            <polyline color="#000000" width="1">          |
|                <coord x="-10" y="-16"/>                  |
|                <coord x="-13" y="-24"/>                  |
|            </polyline>                                   |
|            <polyline color="#ffffff" width="3">          |
|                <coord x="17" y="-12"/>                   |
|                <coord x="20" y="-20"/>                   |
|            </polyline>                                   |
|            <polyline color="#000000" width="1">          |
|                <coord x="17" y="-12"/>                   |
|                <coord x="20" y="-20"/>                   |
|            </polyline>                                   |
|            <polyline color="#ffffff" width="3">          |
|                <coord x="10" y="-16"/>                   |
|                <coord x="13" y="-24"/>                   |
|            </polyline>                                   |
|            <polyline color="#000000" width="1">          |
|                <coord x="10" y="-16"/>                   |
|                <coord x="13" y="-24"/>                   |
|            </polyline>                                   |
|        </itemgra>                                        |
|    </cursor>                                             |
+----------------------------------------------------------+


2D Car black for 10,2"
~~~~~~~~~~~~~~~~~~~~~~

.. figure:: Green.png
   :alt: 2D car black cursor for 10,2" screens
   :width: 300px

   2D car black cursor for 10,2" screens

+------------------------------------------------------------------+
| Layout XML                                                       |
+==================================================================+
| .. code:: xml                                                    |
|                                                                  |
|    <cursor w="70" h="70">                                        |
|            <itemgra>                                             |
|                <!-- Car outline -->                              |
|                <polyline color="#ffffff" width="24">             |
|                    <coord x="-28" y="0"/>                        |
|                    <coord x="28" y="0"/>                         |
|                </polyline>                                       |
|                <polyline color="#ffffff" width="8">              |
|                    <coord x="-30" y="0"/>                        |
|                    <coord x="-15" y="26"/>                       |
|                    <coord x="15" y="26"/>                        |
|                    <coord x="30" y="0"/>                         |
|                </polyline>                                       |
|                <polyline color="#ffffff" width="12">             |
|                    <coord x="-25" y="-7"/>                       |
|                    <coord x="-25" y="-15"/>                      |
|                </polyline>                                       |
|                <polyline color="#ffffff" width="12">             |
|                    <coord x="25" y="-7"/>                        |
|                    <coord x="25" y="-15"/>                       |
|                </polyline>                                       |
|                                                                  |
|                                                                  |
|                <!-- Car -->                                      |
|                <polyline color="#00000f" width="22">             |
|                    <coord x="-28" y="0"/>                        |
|                    <coord x="28" y="0"/>                         |
|                </polyline>                                       |
|                <polyline color="#00000f" width="6">              |
|                                                                  |
|                                                                  |
|                    <coord x="-30" y="0"/>                        |
|                    <coord x="-15" y="26"/>                       |
|                    <coord x="15" y="26"/>                        |
|                    <coord x="30" y="0"/>                         |
|                                                                  |
|                </polyline>                                       |
|                           <!-- left tire -->                     |
|                <polyline color="#00000f" width="10">             |
|                    <coord x="-25" y="-7"/>                       |
|                    <coord x="-25" y="-15"/>                      |
|                          <!-- right tire -->                     |
|                </polyline>                                       |
|                <polyline color="#00000f" width="10">             |
|                    <coord x="25" y="-7"/>                        |
|                    <coord x="25" y="-15"/>                       |
|                </polyline>                                       |
|                                                                  |
|                                                                  |
|            </itemgra>                                            |
|            <itemgra speed_range="-2">                            |
|                                                                  |
|                <!--Back lights:brake -->                         |
|                <polyline color="#ff0000" width="8">              |
|                    <coord x="-20" y="0"/>                        |
|                    <coord x="-28" y="0"/></polyline>             |
|                             <polyline color="#ff0000" width="8"> |
|                    <coord x="20" y="0"/>                         |
|                    <coord x="28" y="0"/></polyline>              |
|                                                                  |
|                                                                  |
|                                                                  |
|                <polyline color="#ff0000" width="6">              |
|                    <coord x="-7" y="24"/>                        |
|                    <coord x="7" y="24"/>                         |
|                </polyline>                                       |
|                                                                  |
|                                                                  |
|            </itemgra>                                            |
|            <itemgra speed_range="3-">                            |
|                                                                  |
|                <!--Back lights: drive -->                        |
|                                                                  |
|    <polyline color="#ff0000" width="6">                          |
|                    <coord x="-20" y="0"/>                        |
|                    <coord x="-28" y="0"/></polyline>             |
|    <polyline color="#ff0000" width="6">                          |
|                    <coord x="20" y="0"/>                         |
|                    <coord x="28" y="0"/></polyline>              |
|                                                                  |
|                                                                  |
|                                                                  |
|                <!-- Speed lines -->                              |
|                            <polyline color="#ffffff" width="4">  |
|                    <coord x="-17" y="-12"/>                      |
|                    <coord x="-23" y="-28"/>                      |
|                </polyline>                                       |
|                <polyline color="#000000" width="3">              |
|                    <coord x="-17" y="-12"/>                      |
|                    <coord x="-23" y="-28"/>                      |
|                </polyline>                                       |
|                <polyline color="#ffffff" width="4">              |
|                    <coord x="-10" y="-16"/>                      |
|                    <coord x="-16" y="-32"/>                      |
|                </polyline>                                       |
|                <polyline color="#000000" width="3">              |
|                    <coord x="-10" y="-16"/>                      |
|                    <coord x="-16" y="-32"/>                      |
|                </polyline>                                       |
|                <polyline color="#ffffff" width="4">              |
|                    <coord x="17" y="-12"/>                       |
|                    <coord x="23" y="-28"/>                       |
|                </polyline>                                       |
|                <polyline color="#000000" width="3">              |
|                    <coord x="17" y="-12"/>                       |
|                    <coord x="23" y="-28"/>                       |
|                </polyline>                                       |
|                <polyline color="#ffffff" width="4">              |
|                    <coord x="10" y="-16"/>                       |
|                    <coord x="16" y="-32"/>                       |
|                </polyline>                                       |
|                <polyline color="#000000" width="3">              |
|                    <coord x="10" y="-16"/>                       |
|                    <coord x="16" y="-32"/>                       |
|                </polyline>                                       |
|            </itemgra>                                            |
|        </cursor>                                                 |
+------------------------------------------------------------------+

|


2D cursor tangoGPS-like
~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: TangoGPS_cursor.png
   :alt: 2D cursor tangoGPS-like
   :width: 200px

   2D cursor tangoGPS-like

+------------------------------------------------------------+
| Layout XML                                                 |
+============================================================+
| .. code:: xml                                              |
|                                                            |
|    <cursor w="50" h="50">                                  |
|        <itemgra>                                           |
|            <circle color="#ffffff" radius="20" width="20"> |
|                <coord x="0" y="0"/>                        |
|            </circle>                                       |
|            <circle color="#0000ff" radius="30" width="5">  |
|                <coord x="0" y="0"/>                        |
|            </circle>                                       |
|        </itemgra>                                          |
|        <itemgra speed_range="-2">                          |
|            <circle color="#0000ff" radius="4" width="5">   |
|                <coord x="0" y="0"/>                        |
|            </circle>                                       |
|        </itemgra>                                          |
|        <itemgra speed_range="3-">                          |
|            <polyline color="#ffffff" width="11">           |
|                <coord x="0" y="0"/>                        |
|                <coord x="0" y="20"/>                       |
|            </polyline>                                     |
|            <polyline color="#0000ff" width="8">            |
|                <coord x="0" y="0"/>                        |
|                <coord x="0" y="20"/>                       |
|            </polyline>                                     |
|        </itemgra>                                          |
|    </cursor>                                               |
+------------------------------------------------------------+


3D Arrow-head
~~~~~~~~~~~~~

Note that the screenshot below left was taken on an Android - not all
graphics drivers support the drop shadow underneath the arrow, as shown
by the screenshot from Navit on Windows Vista:

.. figure:: 3D-arrow-head.png
   :alt: 3D Arrow-head cursor
   :width: 300px

   3D Arrow-head cursor

.. figure:: 3D-arrow-head-windows.png
   :alt: 3D Arrow-head cursor on Windows
   :width: 500px

   3D Arrow-head cursor on Windows

+------------------------------------------------------------+
| Layout XML                                                 |
+============================================================+
| .. code:: xml                                              |
|                                                            |
|    <cursor w="30" h="32">                                  |
|        <itemgra speed_range="-2">                          |
|            <polyline color="#00BC00" radius="0" width="4"> |
|                <coord x="0" y="0"/>                        |
|            </polyline>                                     |
|            <circle color="#008500" radius="8" width="3">   |
|                <coord x="0" y="0"/>                        |
|            </circle>                                       |
|            <circle color="#00BC00" radius="14" width="3">  |
|                <coord x="0" y="0"/>                        |
|            </circle>                                       |
|            <circle color="#008500" radius="20" width="3">  |
|                <coord x="0" y="0"/>                        |
|            </circle>                                       |
|        </itemgra>                                          |
|        <itemgra speed_range="3-">                          |
|            <polygon color="#00000066">                     |
|                <coord x="-14" y="-18"/>                    |
|                <coord x="0" y="8"/>                        |
|                <coord x="14" y="-18"/>                     |
|                <coord x="0" y="-8"/>                       |
|                <coord x="-14" y="-18"/>                    |
|            </polygon>                                      |
|            <polygon color="#008500">                       |
|                <coord x="-14" y="-12"/>                    |
|                <coord x="0" y="14"/>                       |
|                <coord x="0" y="-2"/>                       |
|                <coord x="-14" y="-12"/>                    |
|            </polygon>                                      |
|            <polygon color="#00BC00">                       |
|                <coord x="14" y="-12"/>                     |
|                <coord x="0" y="14"/>                       |
|                <coord x="0" y="-2"/>                       |
|                <coord x="14" y="-12"/>                     |
|            </polygon>                                      |
|            <polyline color="#008500" width="2">            |
|                <coord x="-14" y="-12"/>                    |
|                <coord x="0" y="14"/>                       |
|                <coord x="0" y="-2"/>                       |
|                <coord x="-14" y="-12"/>                    |
|            </polyline>                                     |
|            <polyline color="#008500" width="2">            |
|                <coord x="14" y="-12"/>                     |
|                <coord x="0" y="14"/>                       |
|                <coord x="0" y="-2"/>                       |
|                <coord x="14" y="-12"/>                     |
|            </polyline>                                     |
|        </itemgra>                                          |
|    </cursor>                                               |
+------------------------------------------------------------+

Layout Gallery
--------------

The complete XML for the layouts presented above is kept in dedicated
pages:

.. toctree::
   :maxdepth: 1

   Layout_mapnik
   Layout_mapnik-for-hdpi
   Layout_mapnik-for-mdpi
   Layout_snow
   Layout_hi_vis
   Layout_detailedcampingbike
   Layout_bike
