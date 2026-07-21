Plugins
=======

Navit includes several plugins that extend its functionality:

.. toctree::
   :maxdepth: 1

   aprs
   aprs_test_results
   aprs_sdr
   aprs_sdr_expected_decode
   aprs_protocol_layers

APRS Plugin
-----------

The APRS (Automatic Packet Reporting System) plugin provides real-time tracking of APRS stations on the map. It decodes APRS packets (Bell 202 / AX.25), stores station information in a SQLite database, and displays moving objects on the Navit map.

See:

- https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/aprs.rst
- https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/aprs_test_results.rst

APRS SDR Plugin
---------------

The APRS SDR plugin provides direct RTL-SDR hardware access with built-in Bell
202 demodulation for APRS packet reception. It works as a companion to the core
APRS plugin and supports common RTL-SDR devices, including:

* RTL-SDR Blog V3 (RTL2832U + R820T2)
* RTL-SDR Blog V4 R828D (RTL2832U + R828D)
* Nooelec NESDR series
* Generic RTL2832U-based dongles

See:

- https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/aprs_sdr.rst


Additional APRS documentation
-----------------------------

For details on specific aspects of the APRS and APRS SDR plugins, including
where to find functions and values relevant for debugging, see:

* APRS architecture and main modules:
  https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/ARCHITECTURE.rst
  (high-level structure of ``aprs.c``, database, decoder and SDR plugin)

* APRS symbol mapping:
  https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/APRS_SYMBOLS.rst
  (lookup logic for ``aprs_symbol_get_icon()`` and icon filenames)

* APRS frequencies:
  https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/APRS_FREQUENCIES.rst
  (regional APRS channel list to cross-check frequency-related bugs)

* APRS range filtering:
  https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/RANGE_FILTERING.rst
  (explains how ``aprs_db_get_stations_in_range()`` and ``aprs_update_items()`` filter stations)

* APRS internals and key functions:
  https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/aprs_internals.rst
  (lists the primary functions to inspect when debugging, such as
  ``aprs_process_packet()``, ``aprs_decode_ax25()``, ``aprs_db_update_station()``,
  ``aprs_sdr_map_new()``, ``aprs_sdr_hw_data_callback()`` and
  ``aprs_sdr_dsp_process_samples()``)

* APRS SDR RTL-SDR setup guide:
  https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/RTL_SDR_SETUP.rst
  (hardware requirements, RF sample rates and how the built-in SDR pipeline is
  expected to be configured)

* APRS/APRS SDR unit test results:
  https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/TEST_RESULTS.rst
  (which unit tests exist for database, decoder and DSP, and what they verify)

* APRS SDR protocol/layer overview and synthetic IQ generation:
  https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/aprs_protocol_layers.rst

* APRS SDR expected decode pipeline output (reference log lines):
  https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/aprs_sdr_expected_decode.rst

* SDR basics and RTL-SDR hardware artifacts (DC spike, IQ imbalance, IF offsets):
  https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/sdr_basics_and_rtlsdr_artifacts.rst

* Logging guidelines for APRS/APRS SDR:
  https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/how-to-log.rst
  (recommended ``dbg(lvl_info/lvl_debug, ...)`` locations and messages to add
  when diagnosing APRS and APRS SDR problems)

* Hamlib and radio control integration:
  https://github.com/Supermagnum/navit/blob/feature/aprs-clean/docs/user/plugins/aprs/aprs.rst
  (explains how APRS comments are exposed via the database and map items so
  that future integrations using `hamlib`_ can tune radios based on QRV messages.
  QRV is an amateur radio Q-code meaning "I am ready to receive" or more
  casually "I am listening / active on this frequency." Weather information
  from APRS comments can be consumed by other plugins.)

.. _hamlib: https://hamlib.github.io/
