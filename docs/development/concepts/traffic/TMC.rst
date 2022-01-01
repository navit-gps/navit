TMC
===

.. _technical_documentation:

Technical documentation
-----------------------

http://read.pudn.com/downloads53/doc/comm/185140/14819_1.pdf

.. _how_to_gain_tmc_data:

How to gain TMC Data
--------------------

The TMC Data is hidden inside the `RDS
Signal <https://en.wikipedia.org/wiki/Radio_Data_System>`__. It contains
a location and a event. The event could be something like "traffic jam
in the following 5km"

.. _sample_devices:

Sample Devices
~~~~~~~~~~~~~~

-  SI4703
   `Documents <https://www.silabs.com/support/resources.p-audio-and-radio_fm-radios_si4702-03>`__
   `ready to use <https://www.sparkfun.com/products/12938>`__

.. _problems_with_tmc:

Problems with TMC
-----------------

In the order of the biggest to the lowest problem (one means biggest
problem)

#. TMC location code table is not good to match with the maps

   #. coverage of TMC is zero in all countries except Germany with 2%
   #. maptool currently discards all TMC information
   #. [STRIKEOUT:It could not (really) automatically merged] *The
      traffic module can match the location of an event from the map,
      based only on coordinates and road attributes*

#. [STRIKEOUT:Navit currently has no traffic providers] *Generic traffic
   functionality is available as a module, a functional traffic provider
   is available for Android*
#. Every location code table and event code table must be obtained
   separately

.. _idea_how_to_get_tmc_data_inside_navit:

Idea how to get TMC Data inside Navit
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Extend the NMEA Data to have a custom tag called "PTMCA" witch must
defined later. PTMCA stands for "Proprietary TMC announcement". The P in
front is because its common and stands for proprietary. The TMC is
because its TMC data. And the A is the type, in this case
"announcement".

Something similar is already commercially available from RoyalTek. Some
information about the protocol is available here:

-  http://www.gpspassion.com/FORUMSEN/topic.asp?TOPIC_ID=17275&whichpage=2
   (several posts in the bottom half of the page)
-  (archive)
   https://web.archive.org/web/20160513151613/http://gpspassion.com/forumsen/topic.asp?TOPIC_ID=17275&whichpage=2

.. _other_ideas:

Other Ideas
~~~~~~~~~~~

Traffic is currently being implemented, see `Traffic in
Navit <Traffic_in_Navit>`__. The traffic branch works with a universal
format called TraFF. TMC is received with an external app, converted to
TraFF and fed into the plugin (Android only at the moment). The external
app handles all location decoding; the TraFF data contains coordinates
along with road numbers and road names, where available.

This approach is not limited to TMC: the external app could, in theory,
pull traffic events from any source, convert them into TraFF and feed
them into Navit.

Matching is currently done by coordinates, thus lack of TMC coverage in
OSM is not a deal-breaker. However, TMC codes could be used to improve
the quality of location matching—this would require some extensions to
the code.

.. _related_software:

Related Software
----------------

-  https://github.com/windytan/redsea (MIT license, GPLv2-compatible)
-  https://github.com/xenos1984/RdsDecoder
-  https://github.com/xenos1984/TmcViewer
-  https://github.com/ChristopheJacquet/RdsSurveyor (GPLv2, some
   backends GPLv3)
-  https://sourceforge.net/projects/radiodatasystem/?source=directory
   (LGPLv3)
-  https://bitbucket.org/tobylorenz/iso_14819 (GPLv3)

.. _see_also:

See also
--------

-  https://de.wikipedia.org/wiki/Transport_Protocol_Experts_Group

Sources
-------

-  Areas with TMC: http://tisa.org/technologies/coverage/
-  Another source for the Areas:
   http://de.support.tomtom.com/app/answers/detail/a_id/5828/~/rds-tmc-abdeckung
