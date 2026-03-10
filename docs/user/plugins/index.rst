Plugins
=======

Navit includes several plugins that extend its functionality:

.. toctree::
   :maxdepth: 1

   driver_break
   driver_break_test_results
   aprs
   aprs_test_results
   aprs_sdr
   navit_safety
   navit_safety_test_results

Driver Break Plugin
-------------------

The Driver Break plugin provides dynamic rest period management for vehicles, with support for both cars and trucks. It tracks driving time, suggests rest stops based on OpenStreetMap data, discovers nearby Points of Interest (POIs), and ensures compliance with EU Regulation EC 561/2006 for commercial vehicles.

See :doc:`driver_break` for complete documentation.

Test results: :doc:`driver_break_test_results`

APRS Plugin
-----------

The APRS (Automatic Packet Reporting System) plugin provides real-time tracking of APRS stations on the map. It decodes APRS packets (Bell 202 / AX.25), stores station information in a SQLite database, and displays moving objects on the Navit map.

See :doc:`aprs` for complete documentation.

Test results: :doc:`aprs_test_results`

APRS SDR Plugin
---------------

The APRS SDR plugin provides direct RTL-SDR hardware access with built-in Bell 202 demodulation for APRS packet reception. It works as a companion to the core APRS plugin.

See :doc:`aprs_sdr` for complete documentation.

Navit Safety Plugin
-------------------

The Navit Safety plugin provides POI confidence scoring, location-aware remote mode, and resource planning for desert driving and foot travel in arid or sparsely serviced terrain. It treats POI reliability as a first-class input and uses conservative buffers when route segments are in remote or desert regions.

See :doc:`navit_safety` for complete documentation.

Test results: :doc:`navit_safety_test_results`
