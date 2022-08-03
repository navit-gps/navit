.. _garmin_maps:

Garmin maps
===========

.. warning

   This is an experimental driver for '''Garmin IMG maps'''.Note that many features are not yet implemented (such as navigation). See section "Limitations" for details.

It's located here: http://libgarmin.sourceforge.net/ You will need the
'subversion' (svn) tool to install this driver.

You can search a map for your area at: http://gpsmapsearch.com/
http://mapcenter2.cgpsmapper.com/

You can build your own from OpenStreetMap (OSM) data, see:
http://wiki.openstreetmap.org/index.php/Mkgmap However, if you want to
use OSM data in Navit, it makes more sense to convert to
`binfile <binfile>`__ format (Navit's own map format), or just download
a pre-generated binfile map (see `OpenStreetMap <OpenStreetMap>`__).

Please give us feedback / bug reports/feature requests/success stories
about this driver either in navit tracker or
`mailto:libgarmin@gmail.com <mailto:libgarmin@gmail.com>`__.

Limitations / known problems
============================

-  The driver supports only maps that **are not locked!**. Some of
   original Garmin maps are not locked, for example MetroGuide Europe,
   but most of them are.
-  The driver has only partial (or no?) support for address search and
   navigation.
-  The driver apparently cannot load recent (after about year 2010)
   original Garmin maps, because it does not support the format (*NT
   format*?) that recent maps use. See `explanation in QMapShack's
   bugtracker <https://bitbucket.org/maproom/qmapshack/issues/12/cannot-import-nt-format-map-file>`__
   or `Debian bug
   #601135 <https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=601135>`__
   for details.

Installation and configuration
==============================

First, get libgarmin from svn, and install it, as usual :

.. code:: bash

    svn co http://libgarmin.svn.sourceforge.net/svnroot/libgarmin/libgarmin/dev libgarmin
    cd libgarmin
    ./autosh.sh
    ./configure
    make

And, as root :

.. code:: bash

    make install

.. _how_to_build_the_driver:

How to build the driver
-----------------------

Be sure to update navit to latest SVN

reconfigure and build navit.

.. code:: bash

    cd ../..
    ./autogen.sh && ./configure && make

.. _configure_your_map_source:

Configure your map source
-------------------------

Now add your first garmin map to navit.xml (see
`Configuration <Configuration>`__)

.. code:: xml

           <mapset enabled="yes">
                   <map type="garmin" enabled="yes" data="/path/to/gmapsupp.img"/>
           </mapset>

Where you can give it either dskimg file (gmapsupp.img) or a .tdb file

Note that only one mapset may be enabled, so if your existing navit.xml
has another mapset enabled (default), you need to disable it by setting

.. code:: xml

           <mapset enabled="no">

You also need to add

.. code:: xml

           <plugin path="$NAVIT_PREFIX/lib/libmap_garmin.so" ondemand="no"/>

to the plugins section on the top of navit.xml

Install a free basemap from Garmin
----------------------------------

You can download a free base map from garmin.
http://www8.garmin.com/support/download_details.jsp?id=3645

After you get GarminMobileXTFreeBasemap_4xxxx.exe, unzip it to some
temporary directory. In GMobileCard/Garmin directory you will find a
file called gmapbmap.img. Copy that file in some location for later use,
let's say /mymaps/gmapbmap.img . Now you can remove the temporary
directory. And register the map in your navit.xml

