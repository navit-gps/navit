Driver Break Plugin Tests
=========================

This document describes the test suite for the Driver Break plugin. The tests live in
``navit/plugin/driver_break/tests/`` and are built when the plugin is enabled (SQLite3
found). See :doc:`index` for plugin feature documentation.

Test executables
----------------

All executables are built under the Navit build directory as
``navit/plugin/driver_break/tests/<name>``.

+-----------------------------------+------------------------------------------------------------------+
| Executable                        | Purpose                                                          |
+===================================+==================================================================+
| test_driver_break_config          | Configuration parsing and defaults                               |
+-----------------------------------+------------------------------------------------------------------+
| test_driver_break_db              | SQLite database operations (history, config storage)             |
+-----------------------------------+------------------------------------------------------------------+
| test_driver_break_finder          | Rest stop finder and location validation                         |
+-----------------------------------+------------------------------------------------------------------+
| test_driver_break_routing         | Routing profiles and route validator                             |
+-----------------------------------+------------------------------------------------------------------+
| test_driver_break_srtm            | SRTM HGT and GeoTIFF elevation (see below)                       |
+-----------------------------------+------------------------------------------------------------------+
| test_driver_break_integration     | End-to-end plugin functions (POI, SRTM, no map required)         |
+-----------------------------------+------------------------------------------------------------------+
| test_driver_break_route_integration| Rest intervals, POI discovery along routes, SRTM along route     |
+-----------------------------------+------------------------------------------------------------------+

SRTM and elevation tests (test_driver_break_srtm)
------------------------------------------------

The SRTM test executable covers HGT file handling, tile borders, and optional
download-and-read tests for both SRTM HGT and Copernicus GLO-30 GeoTIFF.

**Always run (no network):**

- Init and data directory
- Valid HGT read, corrupt/missing file handling
- Tile filename generation (N/S E/W)
- Tile borders: points on either side of a tile boundary use the correct tile
- Cache / multiple-tile read

**When built with CURL** (optional dependency):

- **SRTM HGT download and read**  
  Downloads HGT tiles (Viewfinder Panoramas primary, NASA SRTMGL1 fallback) for
  two 1x1 degree tiles (N62E007, N61E009), extracts the ``.hgt`` files, then
  verifies elevation read at three coordinates (Norway, from OpenStreetMap map
  locations). If download or unzip fails (e.g. network unavailable), the test is
  skipped with a message; no failure.

**When built with libtiff and CURL** (optional):

- **Copernicus GLO-30 download and read**  
  Downloads Copernicus DEM GLO-30 GeoTIFF tiles from AWS S3 for the same two
  tiles, then verifies elevation read at the same three coordinates. If
  download fails, the test is skipped; no failure.

**Test coordinates (same for HGT and GeoTIFF):**

- 62.0937 N, 7.1433 E (tile N62E007)
- 61.5919 N, 9.7018 E (tile N61E009)
- 61.36012 N, 9.66941 E (tile N61E009)

Elevations are asserted to be valid (not void) and in the range 0–3000 m for
these Norwegian locations.

Running the tests
-----------------

From the Navit build directory:

.. code-block:: bash

   cd build
   ./navit/plugin/driver_break/tests/test_driver_break_config
   ./navit/plugin/driver_break/tests/test_driver_break_db
   ./navit/plugin/driver_break/tests/test_driver_break_finder
   ./navit/plugin/driver_break/tests/test_driver_break_routing
   ./navit/plugin/driver_break/tests/test_driver_break_srtm
   ./navit/plugin/driver_break/tests/test_driver_break_integration
   ./navit/plugin/driver_break/tests/test_driver_break_route_integration

Or run all driver_break tests in one go:

.. code-block:: bash

   for t in config db finder routing srtm integration route_integration; do
     ./navit/plugin/driver_break/tests/test_driver_break_$t || exit 1
   done

The CTest names (if registered from the plugin subdirectory) are:
``driver_break_config_test``, ``driver_break_db_test``, ``driver_break_finder_test``,
``driver_break_routing_test``, ``driver_break_srtm_test``, ``driver_break_integration_test``,
``driver_break_route_integration_test``.

Network and dependencies
------------------------

- **Download tests** (SRTM HGT and Copernicus GLO-30) need network access and,
  for Copernicus, libtiff. They are skipped if the build has no CURL or if
  download/unzip fails.
- **Route integration test** may report 0 POIs and elevation -32768 if no map
  or HGT data is present; that is expected. The test still verifies that the
  plugin code runs correctly.

Expected results
----------------

All seven test executables exit with code 0 when all tests pass. Typical output:

- **test_driver_break_config:** ``All configuration tests passed!``
- **test_driver_break_db:** ``All database tests passed!``
- **test_driver_break_finder:** ``All finder tests passed!``
- **test_driver_break_routing:** ``All routing tests passed!``
- **test_driver_break_srtm:** ``All SRTM HGT file handling tests passed!``  
  If HGT download ran: ``SRTM HGT: tiles downloaded and read correctly at 3 OSM locations ...``  
  If Copernicus download ran: ``Copernicus GLO-30: tiles read correctly at 3 OSM locations ...``
- **test_driver_break_integration:** ``All integration tests passed!``
- **test_driver_break_route_integration:** ``All integration tests passed!`` with
  rest interval and POI counts (may be 0 without map data).

Any failure is reported to stderr with file and line; the executable then exits
with a non-zero code.

Test results
------------

Sample output from a full run of all seven test executables (build without map
or HGT data; SRTM/Copernicus download tests may be skipped if CURL or network
is unavailable). All tests passed (exit code 0).

.. code-block:: text

   === test_driver_break_config ===
   Running Driver Break plugin configuration tests...
   All configuration tests passed!

   === test_driver_break_db ===
   Running Driver Break plugin database tests...
   All database tests passed!

   === test_driver_break_finder ===
   Running Driver Break plugin finder tests...
   All finder tests passed!

   === test_driver_break_routing ===
   Running Driver Break plugin routing tests...
   All routing tests passed!

   === test_driver_break_srtm ===
   Running SRTM HGT file handling tests...
   All SRTM HGT file handling tests passed!

   === test_driver_break_integration ===
   Running Driver Break plugin integration tests (using actual Navit/plugin functions)...
   All integration tests passed!
   Note: Some tests may return empty results if map/HGT data not available.
   This is expected - tests verify functions execute correctly.

   === test_driver_break_route_integration ===
   Running Driver Break plugin integration tests (using actual Navit/plugin functions)...
   Testing rest interval creation and POI discovery along routes...

   Test 1: Hiking rest intervals creation
     Created 1 rest intervals for 25 km hiking route

   Test 2: POI discovery along hiking route
     Loading binfile map driver from: .../libmap_binfile.so
     binfile map driver loaded and initialized
     Map data not found, skipping map-based POI search
     Note: To test map-based POI discovery, run download_test_map_data.sh first
     Total POIs found along hiking route: 0

   Test 3: POI discovery along car route
     Map data not found, skipping map-based POI search
     Note: To test map-based POI discovery, run download_test_map_data.sh first
     Total POIs found along car route: 0

   Test 4: SRTM elevation along hiking route
     Elevation at Bjøberg: -32768 m
     Elevation at Bjordalsbu: -32768 m
     Elevation at Aurlandsdalen: -32768 m

   Test 5: Cycling rest intervals creation
     Created 3 rest intervals for 100 km cycling route

   Test 6: Route validation functions

   All integration tests passed!

   Note: POI discovery may return 0 results if map data not available.
   This is expected - tests verify functions execute correctly.
   For full testing, ensure map data and HGT files are available.

With map data and HGT/GeoTIFF tiles present (or with network for the download
tests), the SRTM test may additionally print ``SRTM HGT: tiles downloaded and
read correctly at 3 OSM locations ...`` and/or ``Copernicus GLO-30: tiles read
correctly at 3 OSM locations ...``, and route integration may report non-zero
POI counts and valid elevations.
