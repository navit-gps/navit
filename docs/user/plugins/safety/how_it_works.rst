How the Navit Safety plugin works
=================================

This page explains the plugin's planning pipeline and how the pieces fit
together. For the full feature specification see :doc:`navit_safety`; for a
file-by-file map see :doc:`codemap`.

Pipeline overview
-----------------

The plugin turns POI reliability and terrain into a conservative resupply plan:

1. **Configuration** provides the thresholds and buffers (conservative defaults).
2. **POI confidence scoring** decides which stops may count as resupply.
3. **Remote-mode detection** decides whether conservative planning applies.
4. **Buffer selection** picks the standard, remote, or arid buffer.
5. **Lookahead** finds the worst gap between reliable stops and warns if it
   exceeds the usable range.
6. **Confirmation cache** persists user confirmations across sessions.

The orchestrator ``navit_safety_plan`` (in ``navit_safety_plan.c``) runs steps
3 to 5 for a single resource (fuel or water).

Configuration
-------------

``struct navit_safety_config`` (in ``navit_safety.h``) holds 16 fields;
``navit_safety_config_default`` fills them with the spec defaults, for example
remote mode Auto, fuel buffers 25 / 85 / 140 km and water buffers 5 / 20 /
30 km. The defaults are intentionally cautious so users in populated areas
rarely need to change anything.

POI confidence scoring
----------------------

``navit_safety_score_poi`` (in ``navit_safety_confidence.c``) maps a POI's
source and age to ``High``, ``Medium``, ``Low``, or ``Unknown``:

- live chain operator APIs and NREL rank High;
- an OSM/NREL match within 30 days is High, otherwise Medium;
- iOverlander within 60 days is Medium, otherwise Low;
- an OSM chain operator is Medium; an independent operator is Medium within 12
  months and Low beyond that;
- any source is capped at Low when the data is older than three years or the
  POI is in a known depopulating region;
- a user confirmation on the current trip overrides everything to High.

Only ``Medium`` or ``High`` stops count as resupply
(``navit_safety_confidence_counts_as_resupply``); ``Low`` and ``Unknown`` stops
are planned around as if they did not exist.

Remote-mode detection
---------------------

``navit_safety_koppen_lookup`` (in ``navit_safety_koppen.c``) classifies a
coordinate into a Koppen zone using a compact, built-in table of the major arid
belts. It is a coarse approximation, not a gridded Koppen-Geiger dataset.
``navit_safety_koppen_triggers_remote`` returns true for any group-B
(desert/semi-arid) zone.

A second automatic signal is POI density: the orchestrator measures the average
spacing between confirmed (Medium-or-higher) stops over the route, including the
legs from the start to the first stop and from the last stop to the
destination. When that spacing exceeds ``poi_density_threshold_km`` (halved for
water on foot, matching the 80 km fuel / 40 km water defaults) the route is
treated as sparse.

The orchestrator then decides remote mode from the configuration:

- **Always** — remote planning is always on;
- **Auto** — remote planning is on when the Koppen trigger is enabled and the
  sampled midpoint is a group-B zone, or when the POI density is sparse;
- **Off** — remote planning is never auto-enabled.

In arid remote planning the result also carries a desert consumption warning
(gated by ``desert_consumption_warning``) noting that real consumption may
exceed the kinematic model.

Buffer selection and usable range
---------------------------------

With remote mode decided, the orchestrator selects the buffer: the arid buffer
in desert zones (when remote is active), the remote buffer for other remote
terrain, and the standard buffer in populated terrain. The usable range is the
full tank or water load minus the selected buffer (never negative).

Lookahead gap detection
-----------------------

``navit_safety_lookahead_plan`` (in ``navit_safety_lookahead.c``) walks the
reliable stops in order and measures each leg, including the leg from the start
to the first reliable stop and from the last reliable stop to the destination.
It reports the largest gap, where it begins, whether it exceeds the usable
range, and by how much (the shortfall). This is what allows a warning before
departure rather than a discovery mid-journey.

Heat stress and water (foot travel)
-----------------------------------

``navit_safety_heat.c`` provides the foot-travel model.
``navit_safety_heat_assess`` maps a WBGT value to a risk level (caution at
28 °C, warning at 32 °C, danger at 35 °C).
``navit_safety_heat_water_ml_per_hour`` estimates water use from a resting term
scaled by ``body_weight_kg``, an exertion term for strenuous activity, and a
heat term above 25 °C. ``navit_safety_heat_plan`` combines these from the
configuration: the water requirement always uses the configured body weight,
while the risk level and the avoid-exertion flag are only reported when
``heat_stress_warnings`` is enabled.

Confirmation cache
------------------

``navit_safety_cache.c`` stores confirmations in an SQLite table keyed by POI
and trip, so a stop the user verifies stays High for the rest of the trip and
across sessions. The cache is optional and only compiled when SQLite3 is
available (guarded by ``NAVIT_SAFETY_WITH_SQLITE``).

Build-time guards
-----------------

- ``NAVIT_SAFETY_WITH_OSD`` — set to 1 for the plugin build and 0 for the
  configuration unit test, so the OSD/core dependencies are compiled out of the
  isolated test.
- ``NAVIT_SAFETY_WITH_SQLITE`` — set when SQLite3 is found; gates the cache
  source and its use in the OSD.

Not yet wired
-------------

Chain operator API queries remain deferred (network client and API keys). The census depopulation dataset feed is not bundled (the scoring engine consumes a per-POI flag when callers supply it). The foot-travel temperature source is limited to manual WBGT via ``data="wbgt=..."`` on the OSD until a weather feed is integrated.

Live integration (implemented)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the OSD is active, ``navit_safety_route.c`` scans the calculated route corridor (25 km sample spacing, 50 km total width), collects fuel or water POIs from the mapset, scores them, and calls ``navit_safety_plan``. The OSD redraws on route and position updates; the five ``navit_safety_*`` commands adjust mode, confirm or deny POIs, and speak the plan.
