.. _configurationmaps_options:

Configuration/Maps Options
==========================

.. _maps_options:

Maps Options
------------

Navit can read various map formats, and can even show multiple maps at a
time. This is done by defining a mapset. Each mapset can have one or
more maps. Using the GTK GUI, you can enable or disable specific maps at
runtime.

.. code:: xml

    <mapset>
      <map type="binfile" enabled="yes" data="/var/navit/maps/uk.bin" />
      <map type="binfile" enabled="yes" data="/var/navit/maps/france.bin" />
      <map type="binfile" enabled="yes" data="/var/navit/maps/*.bin" />
    </mapset>

If you want to include whole dirctories, use the maps tag:

.. code:: xml

    <maps type="binfile" enabled="yes" data="$NAVIT_SHAREDIR/maps/*.bin"/>

Attention: currently navit causes problems when you use multiple
binfiles at once: `Ticket
#1046 <http://trac.navit-project.org/ticket/1046>`__

To get free maps, see `OpenStreetMap <OpenStreetMap>`__

For different providers see `Maps <Maps>`__
