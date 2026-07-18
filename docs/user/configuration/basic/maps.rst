.. _maps_options:

Map Options
===========

Navit can read various map formats, and can even show multiple maps at a time. This is done by defining a mapset. Each mapset can have one or more maps.


Mapset Configuration
--------------------

To configure maps in Navit, you use the ``<mapset>`` tag containing one or more ``<map>`` tags:

.. code-block:: xml

   <mapset>
     <map type="binfile" enabled="yes" data="/var/navit/maps/uk.bin" />
     <map type="binfile" enabled="yes" data="/var/navit/maps/france.bin" />
     <map type="binfile" enabled="yes" data="/var/navit/maps/*.bin" />
   </mapset>


Using Wildcards
---------------

You can use wildcards to include multiple map files from a directory:

.. code-block:: xml

   <maps type="binfile" enabled="yes" data="$NAVIT_SHAREDIR/maps/*.bin"/>


Map Attributes
--------------

The ``<map>`` tag supports the following attributes:

- **type**: Map format (e.g., "binfile" for binary files)
- **enabled**: Whether the map is enabled ("yes" or "no")
- **data**: Path to the map file (supports wildcards)


Multiple Maps
-------------

Navit can display multiple maps simultaneously. You can enable or disable specific maps at runtime using the GTK GUI.


Known Issues
------------

.. warning::

   Navit may cause problems when using multiple binfiles at once. See `Ticket #1046 <http://trac.navit-project.org/ticket/1046>`_ for details.


Getting Maps
------------

To get free maps, see `OpenStreetMap <https://www.openstreetmap.org/>`_.

For different map providers, see the `Maps <https://wiki.navit-project.org/index.php/Maps>`_ wiki page.


See Also
--------

- :doc:`coordinate_formats` for coordinate format options
- :doc:`../advanced/full_options_reference` for complete configuration reference
