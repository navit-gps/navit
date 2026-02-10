Plugins
=======

Navit includes several plugins that extend its functionality:

.. toctree::
   :maxdepth: 1

   rest
   aprs
   aprs_sdr

Rest Plugin
-----------

The Rest plugin provides dynamic rest period management for vehicles, with support for both cars and trucks. It tracks driving time, suggests rest stops based on OpenStreetMap data, discovers nearby Points of Interest (POIs), and ensures compliance with EU Regulation EC 561/2006 for commercial vehicles.

See :doc:`rest` for complete documentation.

APRS Plugin
-----------

The APRS (Automatic Packet Reporting System) plugin provides real-time tracking of APRS stations on the map. It decodes APRS packets (Bell 202 / AX.25), stores station information in a SQLite database, and displays moving objects on the Navit map.

See :doc:`aprs` for complete documentation.

APRS SDR Plugin
---------------

The APRS SDR plugin provides direct RTL-SDR hardware access with built-in Bell 202 demodulation for APRS packet reception. It works as a companion to the core APRS plugin.

See :doc:`aprs_sdr` for complete documentation.
