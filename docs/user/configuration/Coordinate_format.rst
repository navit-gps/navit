.. _coordinate_format:

Coordinate format
=================

.. _coordinates_in_navit:

Coordinates in Navit
--------------------

Various parts of Navit will read geographical coordinates provided as
text:

-  the :doc:`textfile </user/configuration/maps/Textfile>` map format
-  the "center=" attribute in the configuration file
-  some Navit commands (e.g. set_position), which can be invoked via the
   :doc:`internal GUI </user/configuration/Internal_GUI>` or the
   :doc:`Dbus </user/configuration/integrations/dbus>` bindings
-  the files for bookmarks and last map position (bookmarks.txt and
   center.txt)

This page documents the coordinate systems and formats that Navit will
accept.

.. _supported_coordinate_systems_and_formats:

Supported coordinate systems and formats
----------------------------------------

.. _longitude_latitude_in_decimal_degrees:

Longitude / Latitude in decimal degrees
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Longitude / latitude in degrees can be specified as signed decimal
fractions:

``-33.3553 6.334``

That would be about 33° West, 6° North. Note that in this format
longitude comes first. The coordinates are assumed to be based on WGS84
(the coordinate system used by the GPS system, and by practically all
common navigation systems).

.. _latitude_comma_longitude:

Latitude, Longitude (comma-separated)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If the two values are separated by a comma and the text contains no
whitespace, Navit interprets them as latitude first, longitude second
(the order used by Google Maps and similar tools):

``48.1333,11.5666``

That is 48°8' N, 11°34' E (Munich). Note the inverted order compared
to the space-separated formats on this page.

.. _latitude_longitude_in_degrees_and_minutes:

Latitude / Longitude in degrees and minutes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Latitude / Longitude can also be specified in degrees and minutes with
compass directions (N/S, E/W):

``4808 N 1134 E``

Here the degrees are multiplied by 100 and the arcminutes are added:
``4808`` means 48 degrees and 8 arcminutes (48°8'), ``1134`` means 11
degrees and 34 arcminutes (11°34'), so the position above corresponds
to 48°8' N, 11°34' E (Munich).

For greater precision you can write the minutes as decimal fractions:

``4808.2356 N 1134.5252 E``

That is 48°8.2356' N 11°34.5252' E, the center of the Marienplatz in
Munich.

Notes:

-  This format is rather unusual (because it uses arcminutes, but not
   arcseconds). It is probably easier to just use decimal fractions of
   degrees.
-  Separating whitespace is optional: ``4808 N 1134 E`` and
   ``4808N 1134E`` are parsed the same way. The only case in which the
   exact presence of spaces matters is the comma-separated format below,
   which requires the text to contain no spaces at all.

.. _cartesian_coordinates:

Cartesian coordinates
~~~~~~~~~~~~~~~~~~~~~

Internally, Navit uses a cartesian coordinate system induced by a
Mercator projection. Coordinates are written as hexadecimal integers:

``0x13a3d7 0x5d6d6d``

or specifying a projection:

``mg: 0x13a3d7 0x5d6d6d``

That is again 48°8.2356' N 11°34.5252' E. The part up to and including
the colon is optional, it names the projection to use. Possible values:

-  mg - the projection used by Map&Guide (the default)
-  garmin - "Garmin" projection (TODO: When would it be useful?)

This format is used internally by Navit, but is probably not very useful
for other purposes.

.. _utm_coordinates:

UTM coordinates
~~~~~~~~~~~~~~~

Navit can read coordinates in the `Universal Transverse Mercator
coordinate
system <http://en.wikipedia.org/wiki/Universal_Transverse_Mercator_coordinate_system>`__
(UTM).

The expected format is::

    utm<zone><n|s>: <easting> <northing>

The ``utm`` specifier is mandatory: it is followed by the UTM zone
number (1-60) and the hemisphere letter ``n`` or ``s``, e.g.
``utm32n``. The coordinates are the UTM easting and northing in
metres, as given on maps. Example (Frankfurt am Main, zone 32N):

``utm32n: 477119 5550910``

For the southern hemisphere use ``s``; the northing is then entered in
the usual way and Navit applies the required sign internally.

.. _development_notes:

Development notes
-----------------

The coordinates are parsed in function coord_parse() in coord.c. This
code is used everywhere where Navit parses coordinates, except for the
manual coordinate input in the Internal GUI (which uses its own format
and parsing function).
