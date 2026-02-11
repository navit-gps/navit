Plugins
=======

Navit includes several plugins that extend its functionality:

.. toctree::
   :maxdepth: 1

   aprs
   aprs_test_results
   aprs_sdr

APRS Plugin
-----------

The APRS (Automatic Packet Reporting System) plugin provides real-time tracking of APRS stations on the map. It decodes APRS packets (Bell 202 / AX.25), stores station information in a SQLite database, and displays moving objects on the Navit map.

See :doc:`aprs` for complete documentation.

Test results: :doc:`aprs_test_results`

APRS SDR Plugin
---------------

The APRS SDR plugin provides direct RTL-SDR hardware access with built-in Bell 202 demodulation for APRS packet reception. It works as a companion to the core APRS plugin.

See :doc:`aprs_sdr` for complete documentation.
