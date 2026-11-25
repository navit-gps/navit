Source code map
===============

A map of the plugin's source tree to make navigation and debugging easier. All
paths are under ``navit/plugin/navit_safety/``. For the planning pipeline these
files implement, see :doc:`how_it_works`.

Source files
------------

``navit_safety.h``
    Public configuration: ``enum navit_safety_remote_mode``
    (Off/Auto/Always), ``struct navit_safety_config`` (16 fields), and the
    ``navit_safety_config_default`` prototype. Documents the deferred items as
    TODOs.

``navit_safety_route.{c,h}``
    Live route integration. ``navit_safety_route_refresh`` samples the active
    route, searches the mapset corridor for fuel or water POIs, scores them,
    and runs the planning engines. ``navit_safety_format_status`` composes the
    OSD text. Confirm/deny helpers update the cache and recalculate.

``navit_safety.c`` (OSD section)
    OSD widget, command table (``navit_safety_toggle_remote``,
    ``navit_safety_confirm_poi``, ``navit_safety_deny_poi``,
    ``navit_safety_show_plan``, ``navit_safety_toggle_foot``), attribute
    parsing, and graphics lifecycle.

``navit_safety_confidence.{c,h}``
    POI confidence scoring. ``navit_safety_score_poi`` maps source and age to a
    confidence level; ``navit_safety_confidence_counts_as_resupply`` is the
    Medium-or-higher predicate. Age thresholds are named constants at the top
    of the ``.c``.

``navit_safety_koppen.{c,h}``
    Climate classification. ``navit_safety_koppen_lookup`` resolves a
    coordinate via the static ``koppen_boxes`` table; the predicates
    ``navit_safety_koppen_is_arid`` / ``_is_semiarid`` / ``_triggers_remote``
    and ``navit_safety_koppen_code`` round it out.

``navit_safety_lookahead.{c,h}``
    Gap detection. ``navit_safety_lookahead_plan`` produces a
    ``struct navit_safety_gap_result`` (max gap, gap start, warning,
    shortfall). Depends only on the confidence enum.

``navit_safety_plan.{c,h}``
    Resource-planning orchestrator. ``navit_safety_plan`` ties the config,
    Koppen lookup, POI-density check (``density_is_sparse``), buffer selection
    (``select_buffer``), and lookahead together into a
    ``struct navit_safety_plan_result``. This is the entry point a route hook
    will call.

``navit_safety_heat.{c,h}``
    Foot-travel heat model. ``navit_safety_heat_assess`` (WBGT to risk level),
    ``navit_safety_heat_water_ml_per_hour`` (water requirement), and
    ``navit_safety_heat_plan`` (config-driven combination). Depends only on the
    config struct.

``navit_safety_cache.{c,h}``
    SQLite confirmation cache (only built when SQLite3 is available).
    ``navit_safety_cache_open`` / ``_close`` manage the handle;
    ``navit_safety_cache_confirm`` / ``_is_confirmed`` read and write the
    ``poi_confirmations`` table (keyed by ``poi_id`` and ``trip_id``).

Build wiring
------------

``CMakeLists.txt`` (plugin)
    Builds the engines into ``plugin_navit_safety`` (output
    ``libosd_navit_safety.so``). ``navit_safety_cache.c`` is added and
    ``NAVIT_SAFETY_WITH_SQLITE=1`` is defined only when SQLite3 is found.
    ``NAVIT_SAFETY_WITH_OSD=1`` is scoped to the plugin target.

``CMakeLists.txt`` (root, excerpt)
    Detects SQLite3 and registers ``add_module(plugin/navit_safety ...)``,
    enabling it when SQLite3 is present.

Tests
-----

Under ``tests/`` (built when ``BUILD_NAVIT_SAFETY_TESTS=ON``):

- ``test_navit_safety_config.c`` — defaults, via the ``navit_safety_config_obj``
  OBJECT library compiled with ``NAVIT_SAFETY_WITH_OSD=0`` to isolate it from
  the OSD/core symbols.
- ``test_navit_safety_confidence.c`` — the source-ranking table and downgrades.
- ``test_navit_safety_koppen.c`` — zone predicates and the coarse lookup.
- ``test_navit_safety_lookahead.c`` — gaps, ignored low-confidence stops, final
  leg.
- ``test_navit_safety_plan.c`` — buffer selection, POI-density trigger, and the
  desert warning across terrain and modes.
- ``test_navit_safety_heat.c`` — WBGT thresholds, the water formula, and the
  config-gated heat plan.
- ``test_navit_safety_route.c`` — stop-list builder and idle status formatting.

- ``test_navit_safety_cache.c`` — confirm/query round-trips (in-memory DB);
  only built with SQLite3.

Run them with::

   cmake -S . -B build -DBUILD_NAVIT_SAFETY_TESTS=ON
   cmake --build build -j"$(nproc)"
   ctest --test-dir build/navit/plugin/navit_safety/tests --output-on-failure

Debugging tips
--------------

- The OSD logs through ``dbg(...)``; raise the Navit debug level to see
  ``Navit Safety`` messages. Useful breakpoints: ``navit_safety_osd_new`` (OSD
  lifecycle) and ``navit_safety_plan`` (planning entry point).
- The pure engines have no Navit dependencies, so the unit tests are the
  fastest way to reproduce and step through scoring, classification, lookahead,
  and planning logic in isolation.
- For cache issues, open ``~/.navit/navit_safety.db`` with the ``sqlite3`` CLI
  and inspect the ``poi_confirmations`` table.
- Remember the two compile guards: a symbol missing from a build is usually
  explained by ``NAVIT_SAFETY_WITH_OSD`` (OSD path) or
  ``NAVIT_SAFETY_WITH_SQLITE`` (cache).
