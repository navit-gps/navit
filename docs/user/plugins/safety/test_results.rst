Test results
============

This page records the unit test status of the Navit Safety plugin. The tests
cover the pure planning engines and the route-state helpers in isolation, with
no dependency on the Navit core or a display.

How to reproduce
----------------

.. code-block:: sh

   cmake -S . -B build -DBUILD_NAVIT_SAFETY_TESTS=ON
   cmake --build build -j"$(nproc)"
   ctest --test-dir build/navit/plugin/navit_safety/tests --output-on-failure

The confirmation cache suite is built and run only when SQLite3 is available.

Summary
-------

All suites pass: 8 of 8 test executables, 28 individual test cases, 0 failures.

.. list-table::
   :header-rows: 1
   :widths: 34 12 54

   * - Suite
     - Cases
     - Coverage
   * - ``test_navit_safety_config``
     - 2
     - Configuration defaults and NULL-argument safety.
   * - ``test_navit_safety_confidence``
     - 5
     - Source ranking, age dependence, OSM operators, downgrades, resupply predicate.
   * - ``test_navit_safety_koppen``
     - 4
     - Zone predicates, arid and non-arid lookup, code strings.
   * - ``test_navit_safety_lookahead``
     - 4
     - No-stop warning, gap closing, low-confidence stops ignored, final-leg gap.
   * - ``test_navit_safety_plan``
     - 6
     - Populated route, density trigger, arid buffer, desert-warning toggle, Koppen toggle, Always mode.
   * - ``test_navit_safety_heat``
     - 3
     - WBGT thresholds, water formula, config-gated heat plan.
   * - ``test_navit_safety_route``
     - 2
     - Stop-list builder (denied stops), idle status formatting.
   * - ``test_navit_safety_cache``
     - 2
     - Confirm/query round-trips (in-memory DB), NULL safety. SQLite3 only.

Latest run
----------

::

   Test project build/navit/plugin/navit_safety/tests
       Start 1: test_navit_safety_config
   1/8 Test #1: test_navit_safety_config .........   Passed    0.00 sec
       Start 2: test_navit_safety_confidence
   2/8 Test #2: test_navit_safety_confidence .....   Passed    0.00 sec
       Start 3: test_navit_safety_koppen
   3/8 Test #3: test_navit_safety_koppen .........   Passed    0.00 sec
       Start 4: test_navit_safety_lookahead
   4/8 Test #4: test_navit_safety_lookahead ......   Passed    0.00 sec
       Start 5: test_navit_safety_plan
   5/8 Test #5: test_navit_safety_plan ...........   Passed    0.00 sec
       Start 6: test_navit_safety_heat
   6/8 Test #6: test_navit_safety_heat ...........   Passed    0.00 sec
       Start 7: test_navit_safety_route
   7/8 Test #7: test_navit_safety_route ..........   Passed    0.00 sec
       Start 8: test_navit_safety_cache
   8/8 Test #8: test_navit_safety_cache ..........   Passed    0.00 sec

   100% tests passed, 0 tests failed out of 8

Plugin compile check
--------------------

Beyond the unit tests, the shared object is built against the Navit core to
confirm the OSD, command table, and route-corridor integration link correctly::

   cmake --build build --target plugin_navit_safety
   ... Linking C shared module libosd_navit_safety.so
   ... Built target plugin_navit_safety

What the tests do not cover
---------------------------

The unit tests exercise logic that runs without a display or map. The following
require a running Navit instance with graphics and a loaded mapset and are
verified by the compile check plus manual testing, not by ctest:

- OSD drawing and redraw callbacks.
- The five registered commands (``navit_safety_toggle_remote`` and friends).
- Live mapset corridor scanning in ``navit_safety_route_refresh``.

See :doc:`codemap` for the source layout and :doc:`how_it_works` for the
behaviour under test.
