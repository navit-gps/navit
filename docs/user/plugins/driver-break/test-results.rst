Driver Break test results (captured run)
========================================

This page records **one full run** of all nine Driver Break plugin test executables plus a **CTest** summary. Output varies with your machine, optional dependencies (CURL, libtiff, map tiles), and whether ``download_test_map_data.sh`` / ``download_test_srtm_data.sh`` have been run. For how to run the suite yourself, see :doc:`tests`.

Capture metadata
----------------

- **When:** 2026-04-05 (ISO local time 04:51:25 +02:00).
- **Build:** CMake ``Debug``, ``DISABLE_QT=1``, plugin enabled with SQLite3 and **libcurl** (Copernicus download in the SRTM test). Executables from a temporary build tree (``/tmp/navit-trdoc``).
- **Map data:** ``navit/plugin/driver_break/tests/map_data/osm_bbox_9.5,60.95,11.35,62.25.bin`` present. Route integration uses CMake-injected ``map_data`` and the **binfile** plugin path.
- **Elevation:** Copernicus GeoTIFF tiles under ``/tmp/test_srtm_hgt_download`` (shared with ``test_driver_break_srtm``, including optional **N61E010** and **N62E009** for Rondanestien Test 4). Override with ``SRTM_TEST_DIR`` if needed.
- **Outcome:** All nine tests passed (exit code 0). CTest reported 100% pass, 0 failures.

Sequential executable output
----------------------------

Stdout from each binary, in the usual order (config, db, finder, routing, SRTM, integration, route integration, OBD/J1939, MegaSquirt). ``map_new`` pointers are omitted (they differ each run).

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
     Downloading HGT tiles (Viewfinder Panoramas / NASA SRTM)...
   SRTM HGT: tile download failed or unzip failed (network/unavailable). Skip read test.
     Downloading Copernicus GLO-30 GeoTIFF from AWS S3...
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
     Loading binfile map driver from: .../navit/map/binfile/libmap_binfile.so
     binfile map driver loaded and initialized
     Loading map data from: .../navit/plugin/driver_break/tests/map_data/osm_bbox_9.5,60.95,11.35,62.25.bin
     binfile map driver is registered
     Calling map_new with type=binfile
     map_new succeeded, map=(opaque pointer)
     Map loaded successfully (Rondanestien bbox)
     Using map-based POI search (Rondanestien, map data loaded)
     Found 11 cabins near Rondanestien mid (map data)
     Found 65 water points near Rondanestien mid (map data)
     Found 8 cabins near Rondanestien north / Hjerkinn (map data)
     Total POIs found along hiking route: 84

   Test 3: POI discovery along car route
     Loading map data from: .../navit/plugin/driver_break/tests/map_data/osm_bbox_9.5,60.95,11.35,62.25.bin
     binfile map driver is registered
     Calling map_new with type=binfile
     map_new succeeded, map=(opaque pointer)
     Map loaded successfully (Rondanestien bbox)
     Using map-based POI search (map data loaded)
     Found 376 POIs near Moelv (map data)
     Found 402 POIs near Aukrustsenteret (map data)
     Total POIs found along car route: 778

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

   Running Driver Break OBD-II and J1939 backend tests...
   All OBD-II/J1939 backend tests passed!

   Running Driver Break MegaSquirt backend tests...
   All MegaSquirt backend tests passed.

Without ``map_data`` or elevation tiles, Tests 2–4 may show zero POIs or void elevation while still exiting 0. CTest runs **SRTM before route integration**, so Copernicus tiles fetched by ``test_driver_break_srtm`` are available for Test 4 when using the shared directory; see :doc:`tests`.

CTest summary
-------------

.. code-block:: text

   1/9 Test #1: driver_break_db_test ..................   Passed    0.80 sec
   2/9 Test #2: driver_break_config_test ..............   Passed    0.00 sec
   3/9 Test #3: driver_break_finder_test ..............   Passed    0.00 sec
   4/9 Test #4: driver_break_srtm_test ................   Passed    7.98 sec
   5/9 Test #5: driver_break_routing_test .............   Passed    0.00 sec
   6/9 Test #6: driver_break_integration_test .........   Passed    0.00 sec
   7/9 Test #7: driver_break_route_integration_test ...   Passed    1.48 sec
   8/9 Test #8: driver_break_obd_j1939_test ...........   Passed    0.00 sec
   9/9 Test #9: driver_break_megasquirt_test ..........   Passed    0.00 sec

   100% tests passed, 0 tests failed out of 9

   Total Test time (real) =  10.27 sec

Per-test timings depend on host I/O and network (SRTM and route integration are usually the slowest).
