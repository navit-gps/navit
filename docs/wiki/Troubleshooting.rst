Troubleshooting
===============

There is a huge possibility that your problem is solved by someone else.
First thing to check is `FAQ <FAQ>`__.

.. _segfaults_and_coredumps:

Segfaults and Coredumps
-----------------------

While already working pretty well, navit is still picky about certain
things, which may cause it to segfault. You will find here the case we
know of, and how to solve it until we handle it in the code. If none of
theses work, please see how to report your segfault at the bottom of the
page.

.. _error_coord_rect_overlap_assertion_failed:

ERROR coord_rect_overlap: assertion failed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is caused by NavIt being unable to match a road for your current
position. The first thing to check is your data source. If you don't
have a GPS, try to modify the starting position in navit.xml (see
Configuring navit for more details). Basically, you need to edit the
line in navit.xml, and change it to a position near you. Most probably,
you don't have maps datas for Germany, where is the default starting
point.

If you have a GPS, or if you're using an nmea log file, you've got three
possibilities:

-  your position isn't reported accurately yet
-  you have a problem with the LC_NUMERIC environment variable, making
   NavIt to truncate the coordinates ( 5.1351N is read as 5,1351N and so
   it's truncated to 5N since it doesn't find the decimal part)
-  you have no maps for the area your current position.

.. _segfault_in_search_dialog:

Segfault in search dialog
~~~~~~~~~~~~~~~~~~~~~~~~~

If Navit segfaults after opening the search dialog (Route->Destination),
make sure you have the LANG environment variable set. E.g., try running

``export LANG=sk_SK.UTF8; navit``

.. _reporting_a_segfault:

Reporting a segfault
~~~~~~~~~~~~~~~~~~~~

So, none of the previous examples worked for you. We're really sorry
about that, but with your help it can be solved quickly. Please contact
us. The best way is probably to ask for help in #navit on
irc.freenode.org. Otherwise, you can also try our forums. Before
contacting us, please make sure you have gdb (the GNU Project debugger)
installed. The 'strace' program can be useful too. If you are using the
sdl gui, please also join the CEGUI.log file you will find in your
current directory.

To use gdb, simply run NavIt this way :

``gdb navit``

At the gdb prompt, just type "run". NavIt will start. When it is hanged,
switch back to the gdb window, type "where" and report us the whole gdb
session.

No doubt we will quicky fix your issue :)

Thanks for you help

.. _when_starting_navit_in_a_console_you_see_output_like_navitvehicle_trackoverflow_navitvehicle_parse_gpsno_leading:

When starting Navit in a Console, you see output like navit:vehicle_track:overflow navit:vehicle_parse_gps:no leading $
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

(Appeared on a Navilock NL-302U; SirfIII-Chipset)

This output started to appear at my Installation, when i run navit
(listening directly on file://dev/ttyUSB0) and simultanously run gpsd
with cgps. The reason is, that the device has been set into SIRF-Mode
(the green flashing/shining light on the device stops working). What you
have to do is simply run:

``sirfmon /dev/ttyUSB0 (assuming you have a USB-device and USB0 is the serial port for it) and then press "n" and ``\ \ ``.``

This will set the device back in NMEA-Mode (light flashs again) and
navit will work again.

.. _map_problems:

Map Problems
------------

.. _incomplete_coastlines_within_osm_data:

Incomplete Coastlines within OSM Data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you encounter that your whole map or parts are under water, check for
incomplete coastlines.

You must fix the osm data and convert your map again with maptool.

To get osm id of conflicting item, click on the drowned area, in the
main menu go to actions, click on the big globe, select
"poly_water_tiled" item, then go to attributes and navit will show you
the osm way id number for further inspection.

Another useful tool seems to be
http://tools.geofabrik.de/osmi/?view=coastline

And it's possible to analyze your area of interest with following
`overpass-turbo snippet <http://overpass-turbo.eu/s/81T>`__:

| ````
| ``[out:xml] [timeout:25];``
| ````
| ``(``
| ``  way["natural"="coastline"]({{bbox}});``
| ``);``
| ``// print results``
| ``out meta;``
| ``>;``
| ``out meta qt;``

We believe that coastline shouldn't be used to tag river banks and river
islands. To get general information on water objects, islands, and
coastline tagging, see OSM wiki info for
`waterway=riverbank <http://wiki.openstreetmap.org/wiki/Tag:waterway%3Driverbank>`__,
`natural=coastline <http://wiki.openstreetmap.org/wiki/Tag:natural%3Dcoastline>`__.

.. _my_trouble_wasnt_solved_where_to_next:

My trouble wasn't solved, where to next
---------------------------------------

The easiest way to get help is via IRC. Check `Contact
us <http://www.navit-project.org/?page=contact>`__.
