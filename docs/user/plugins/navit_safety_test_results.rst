Navit Safety Plugin Test Results
================================

Test Execution Date
-------------------

**Date**: 2026-03-10
**Last Updated**: 2026-03-10
**Build Configuration**: Plugin enabled with SQLite3 present; tests built from `navit/plugin/navit_safety/tests`
**Test Framework**: Custom unit test executable (GLib + stdio)

Test Summary
------------

The Navit Safety plugin tests currently cover configuration defaults for remote mode, POI density threshold,
buffer distances, and safety-related toggles. The goal is to guarantee that the runtime configuration starts
from conservative, spec-compliant defaults for remote and arid terrain.

Available Tests
---------------

The following test executable was successfully built:

- ``test_navit_safety_config`` – Verifies default values in ``struct navit_safety_config`` as defined in
  ``navit/plugin/navit_safety/navit_safety.c`` and ``navit/plugin/navit_safety/navit_safety.h``.

Test Results
------------

Configuration Defaults (test_navit_safety_config)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Status**: PASSED

**Command**::

   cd build/navit/plugin/navit_safety/tests
   ./test_navit_safety_config

**Output**::

   Running Navit Safety plugin configuration tests...
   All Navit Safety configuration tests passed!

**Test Cases**:

- Remote mode default is ``NAVIT_SAFETY_REMOTE_AUTO``
- POI density threshold default is 80 km
- Köppen trigger enabled by default
- Fuel buffers (standard/remote/arid) are 25/85/140 km
- Water buffers (standard/remote/arid) are 5/20/30 km
- Desert consumption warning enabled by default
- Census depopulation layer enabled by default
- Chain API queries enabled by default
- Unconfirmed POI display enabled by default
- Foot travel mode disabled by default
- Default body weight is 70 kg
- Heat stress warnings enabled by default

**Result**: All configuration default checks passed successfully.

Build Status
------------

**Successfully Built**:

- ``test_navit_safety_config``

**Notes**:

- The test links only against the configuration portion of ``navit_safety.c`` (OSD and plugin registration
  are compiled out for this target using ``NAVIT_SAFETY_WITH_OSD=0``).
- No Navit core symbols are required; this keeps the test fast and self-contained.

Running Tests
-------------

To build and run the tests from a clean build directory::

   mkdir -p build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make test_navit_safety_config
   cd navit/plugin/navit_safety/tests
   ./test_navit_safety_config

Future Improvements
-------------------

- Add tests for remote-mode activation logic once implemented (POI density, Köppen zones, sparse-region polygons).
- Add tests for buffer selection logic (standard vs remote vs arid) given mocked route segments.
- Add tests for foot travel water planning and heat-stress threshold handling.
