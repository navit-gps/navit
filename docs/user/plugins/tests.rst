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
| test_driver_break_db              | SQLite database operations (history, config, fuel stops)         |
+-----------------------------------+------------------------------------------------------------------+
| test_driver_break_finder          | Rest stop finder and location validation                         |
+-----------------------------------+------------------------------------------------------------------+
| test_driver_break_routing         | Routing profiles and route validator                             |
+-----------------------------------+------------------------------------------------------------------+
| test_driver_break_srtm            | SRTM HGT and GeoTIFF elevation (see below)                       |
+-----------------------------------+------------------------------------------------------------------+
| test_driver_break_integration     | End-to-end plugin functions (POI, SRTM, no map required)         |
+-----------------------------------+------------------------------------------------------------------+
| test_driver_break_route_integration| Rest intervals, POI discovery, SRTM. Route: **Rondanestien** (rel. 1572954). |
+-----------------------------------+------------------------------------------------------------------+
| test_driver_break_obd_j1939       | OBD-II/J1939 backend logic: MAF-based fuel rate, J1939 scaling.  |

SRTM and elevation tests (test_driver_break_srtm)
------------------------------------------------

The SRTM test executable covers HGT and GeoTIFF file handling, tile borders, and
optional download-and-read tests. The plugin uses **Copernicus DEM GLO-30** (AWS S3)
as the primary elevation source and **Viewfinder Panoramas dem3** (zone folders, e.g.
``dem3/M32/N61E009.zip``) as fallback, then NASA SRTMGL1.

**Always run (no network):**

- Init and data directory
- Valid HGT read, corrupt/missing file handling
- Tile filename generation (N/S E/W)
- Tile borders: points on either side of a tile boundary use the correct tile
- Cache / multiple-tile read

**When built with CURL** (optional dependency):

- **SRTM HGT download and read**
  Downloads HGT tiles from Viewfinder Panoramas dem3 (fallback; zone M32 for Norway;
  tile index at ``dem3list.txt``; a browser User-Agent is sent to avoid block) or
  NASA SRTMGL1 for tiles N62E007, N61E009, extracts the ``.hgt`` files, then verifies
  elevation read. If download or unzip fails, the test is skipped; no failure.

**When built with libtiff and CURL** (optional):

- **Copernicus GLO-30 download and read**
  Downloads Copernicus DEM GLO-30 GeoTIFF tiles from the public AWS S3 bucket (no auth).
  URL pattern: ``{base}/{tilename}/{tilename}.tif``. Verifies elevation read. If
  download fails or returns non-TIFF (e.g. error page), the test is skipped; no failure.

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

Or use CTest from the build directory:

.. code-block:: bash

   ctest -R driver_break --output-on-failure

The CTest names (if registered from the plugin subdirectory) are:
``driver_break_config_test``, ``driver_break_db_test``, ``driver_break_finder_test``,
``driver_break_routing_test``, ``driver_break_srtm_test``, ``driver_break_integration_test``,
``driver_break_route_integration_test``.

**Downloading map data for tests**

To get map data so that Test 2 and Test 3 (POI discovery along Rondanestien
and car route) find POIs, run (requires network and optionally ``osmium`` or
``osmosis`` for PBF extract)::

   ./navit/plugin/driver_break/tests/download_test_map_data.sh

This downloads OSM data for bbox 9.5-11.35 E, 60.95-62.25 N (Rondanestien and
Moelv-Aukrust car route), converts to Navit binary format, and writes
``map_data/osm_bbox_9.5,60.95,11.35,62.25.bin``. The route integration test
loads this file when present.

**Downloading elevation data for tests**

To get elevation data so that Test 4 (route integration) and the SRTM test can
report real elevations along Rondanestien, run the helper script before the
tests (requires network and ``curl``)::

   ./navit/plugin/driver_break/tests/download_test_srtm_data.sh

This tries **Copernicus DEM GLO-30** (AWS S3) first for each tile, then
**Viewfinder Panoramas dem3** (e.g. M32/N61E009.zip), then NASA SRTMGL1, and
writes into ``/tmp/test_srtm_hgt_download``. Then run ``test_driver_break_srtm``
and ``test_driver_break_route_integration``; they will use that directory when
tiles are present.

**Hiking test routes (GPX output)**

The plugin provides GPX files for two hiking routes (DNT huts) so that hut names
and elevations are visible in height profile data. Use with energy-based routing
in Navit (e.g. cycling or hiking profile with ``use_energy_routing``). Generate
the GPX files::

   ./navit/plugin/driver_break/tests/generate_hiking_route_gpx.sh

Output is written to ``navit/plugin/driver_break/tests/route_gpx/``:

- **Main route** (``hiking_route_mogen_myrdal.gpx``): Mogen (start, node 1356402304)
  -> Tråastølen (via, 872147024) -> Finsehytta (via, 444092409) -> Myrdal Fjellstove
  (end, way 1012805346). Waypoints have name and elevation (950 m, 1240 m, 1223 m, 867 m).

- **Alternative route** (``hiking_route_gjendesheim_vetledalseter.gpx``): Gjendesheim
  (start, node 791106132) -> Vetledalseter (end, node 1356403887). Elevations 994 m, 515 m.

Each GPX contains ``<rte>``, ``<trk>``, and ``<wpt>`` with ``<ele>`` and ``<name>`` so
the height profile and POI names are available when the route is loaded in Navit.

Network and dependencies
------------------------

- **Download tests**: SRTM HGT (Viewfinder/NASA) needs network and CURL; it is
  the primary elevation source used by the plugin. Copernicus GLO-30 download
  test is optional (libtiff + CURL) and skipped if download fails or returns
  non-TIFF.
- **Route integration test** may report 0 POIs and elevation -32768 if no map
  or HGT data is present; that is expected. The test still verifies that the
  plugin code runs correctly.

Expected results
----------------

All seven test executables exit with code 0 when all tests pass. Typical output:

- **test_driver_break_config:** ``All configuration tests passed!``
- **test_driver_break_db:** ``All database tests passed!``
  Verifies creation of the database, rest stop history insert/retrieve/clear, configuration save/load
  (including fuel profile fields such as tank capacity and average consumption), and fuel stop logging
  into the ``driver_break_fuel_stops`` table.
- **test_driver_break_finder:** ``All finder tests passed!``
- **test_driver_break_routing:** ``All routing tests passed!``
- **test_driver_break_srtm:** ``All SRTM HGT file handling tests passed!``
  If Copernicus download ran: ``Copernicus GLO-30: tiles read correctly at 3 OSM locations (62.09,7.14 / 61.59,9.70 / 61.36,9.67).``
  If HGT download ran: ``SRTM HGT: tiles downloaded and read correctly at 3 OSM locations ...``
- **test_driver_break_integration:** ``All integration tests passed!``
- **test_driver_break_route_integration:** ``All integration tests passed!`` with
  rest interval and POI counts (may be 0 without map data).
- **test_driver_break_obd_j1939:** ``All OBD-II/J1939 backend tests passed!`` exercising:

  - MAF-based fuel rate estimation for:

    - Petrol engines at cruise (typical 10–20 g/s MAF, 2–4 L/h).
    - Diesel engines under load (30+ g/s MAF, higher L/h than petrol).
    - Flex-fuel vehicles at different ethanol contents (E10 vs E85), verifying
      that E85 requires a higher volume fuel rate than E10 for the same MAF.

  - J1939 PGN scaling:

    - PGN 65266 (FEEA) fuel rate scaling (0.05 L/h per bit) tested with realistic
      raw values (e.g. raw=1000 -> 50 L/h).
    - PGN 65276 (FEF4) fuel level scaling (0.4 % per bit) tested with representative
      levels for heavy trucks (e.g. raw=50 -> 20 % of a 400 L tank).

Any failure is reported to stderr with file and line; the executable then exits
with a non-zero code.

Test results
------------

Sample output from a full run of all seven test executables with map data and
elevation tiles present (after running ``download_test_map_data.sh`` and
``download_test_srtm_data.sh``). All tests passed (exit code 0).

.. code-block:: text

   Running Driver Break plugin configuration tests...
   All configuration tests passed!

   Running Driver Break plugin database tests...
   All database tests passed!

   Running Driver Break plugin finder tests...
   All finder tests passed!

   Running Driver Break plugin routing tests...
   All routing tests passed!

   Running SRTM HGT file handling tests...
   Copernicus GLO-30: tiles read correctly at 3 OSM locations (62.09,7.14 / 61.59,9.70 / 61.36,9.67).
   All SRTM HGT file handling tests passed!

   Running Driver Break plugin integration tests (using actual Navit/plugin functions)...
   All integration tests passed!
   Note: Some tests may return empty results if map/HGT data not available.
   This is expected - tests verify functions execute correctly.

   Running Driver Break plugin integration tests (using actual Navit/plugin functions)...
   Testing rest interval creation and POI discovery along routes...

   Test 1: Hiking rest intervals creation
     Created 1 rest intervals for 25 km hiking route

   Test 2: POI discovery along hiking route
     Loading binfile map driver from: .../libmap_binfile.so
     binfile map driver loaded and initialized
     Loading map data from: .../map_data/osm_bbox_9.5,60.95,11.35,62.25.bin
     binfile map driver is registered
     Calling map_new with type=binfile
     map_new succeeded, map=...
     Map loaded successfully (Rondanestien bbox)
     Using map-based POI search (Rondanestien, map data loaded)
     Found 11 cabins near Rondanestien mid (map data)
     Found 65 water points near Rondanestien mid (map data)
     Found 8 cabins near Rondanestien north / Hjerkinn (map data)
     Total POIs found along hiking route: 84

   Test 3: POI discovery along car route
     Loading map data from: .../map_data/osm_bbox_9.5,60.95,11.35,62.25.bin
     binfile map driver is registered
     Calling map_new with type=binfile
     map_new succeeded, map=...
     Map loaded successfully (Rondanestien bbox)
     Using map-based POI search (map data loaded)
     Found 375 POIs near Moelv (map data)
     Found 401 POIs near Aukrustsenteret (map data)
     Total POIs found along car route: 776

   Test 4: SRTM elevation along hiking route
     Using elevation data from: /tmp/test_srtm_hgt_download
     Elevation at Rondanestien south (61.16,10.92): 667 m
     Elevation at Rondanestien mid (61.59,10.35): 1136 m
     Elevation at Rondanestien north / Hjerkinn (62.09,9.64): 997 m
     Elevation gain (south to mid): 469 m

   Test 5: Cycling rest intervals creation
     Created 3 rest intervals for 100 km cycling route

   Test 6: Route validation functions

   All integration tests passed!

   Note: POI discovery may return 0 results if map data not available.
   This is expected - tests verify functions execute correctly.
   For full testing, ensure map data and HGT files are available.

Without map data or elevation tiles, Test 2 and Test 3 report 0 POIs and Test 4
reports ``no data (elevation tiles not present)``; the tests still pass. The
route integration test uses **Rondanestien** (OSM relation 1572954, Målia–Hjerkinn)
as the hiking route. When Copernicus or HGT tiles N61E009, N61E010, N62E009 are
present in the elevation data directory, Test 4 reports elevations at south/mid/north
waypoints (typically in the range 200–1800 m for the Rondane/Hjerkinn area).

GeoTIFF and SRTM HGT download test results
------------------------------------------

When the SRTM test is built with CURL and has network access, it runs the
**SRTM HGT download and read** test: it downloads tiles N62E007 and N61E009
from Viewfinder (or NASA fallback), extracts the ``.hgt`` files, and verifies
elevation at three Norway coordinates. When built with libtiff and CURL, it
also runs the **Copernicus GLO-30 download and read** test: it downloads
GeoTIFF tiles from AWS S3 and verifies elevation. The route integration test
uses Rondanestien waypoints and tiles N61E009, N61E010, N62E009 when
``download_test_srtm_data.sh`` has been run.

**Example: SRTM test output when Copernicus GLO-30 download/read runs successfully**

.. code-block:: text

   Running SRTM HGT file handling tests...
   Copernicus GLO-30: tiles read correctly at 3 OSM locations (62.09,7.14 / 61.59,9.70 / 61.36,9.67).
   All SRTM HGT file handling tests passed!

**Example: SRTM test output when download is skipped (no network or CURL)**

.. code-block:: text

   Running SRTM HGT file handling tests...
   All SRTM HGT file handling tests passed!

If only one download test runs (e.g. CURL but no libtiff), you may see only
the SRTM HGT line or only the Copernicus line. If a download fails (e.g. 404 or
timeout), the test prints a skip message and still exits 0:

.. code-block:: text

   SRTM HGT: tile download failed or unzip failed (network/unavailable). Skip read test.

or

.. code-block:: text

   Copernicus GLO-30: tile download failed or skipped (network/unavailable). Skip read test.
