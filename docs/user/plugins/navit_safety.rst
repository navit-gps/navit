Navit Safety Plugin
===================

Overview
--------

The Navit Safety plugin addresses POI reliability and resource planning in remote, arid, or sparsely serviced terrain. In populated areas with good map coverage, finding fuel, water, or shelter is straightforward. In remote or depopulating regions, POI data cannot be trusted at face value: a fuel station on the map may have closed years ago; a spring may be seasonal or dry. This plugin treats POI reliability as a first-class input to resource planning and provides location-aware remote mode with conservative buffer thresholds.

The plugin handles:

- **POI confidence scoring** — Sources are ranked by reliability; only confirmed or reasonably recent POIs count as reliable resupply points.
- **Location-aware remote mode** — Automatic detection via POI density, Köppen climate classification, and known sparse-region polygons; manual override available.
- **Resource planning** — Fuel and water buffers, lookahead planning, carry capacity, and handling of unconfirmed stops for desert driving and foot travel.

Core problem
------------

Map data (including OSM) reflects the world at last edit time, not necessarily today. In remote areas a POI may be years or decades stale. A closed fuel station 120 km into a desert crossing or a dry spring on a waterless stretch is a life-safety failure. The plugin does not count unverifiable POIs as reliable resupply in minimum-range calculations.

POI confidence hierarchy
-------------------------

Confidence levels
~~~~~~~~~~~~~~~~~

- **High** — Confirmed operational from a live or recently verified authoritative source. Counts fully toward resupply; no extra buffer.
- **Medium** — Likely operational from reasonably recent data but not independently verified. Counts toward resupply with a modest buffer increase.
- **Low** — Exists in map data but operational status unverifiable or data is old. Does not count as reliable resupply; plugin plans as if the stop does not exist and warns.
- **Unknown** — No usable metadata; treated as Low until confirmed.

Source ranking (summary)
~~~~~~~~~~~~~~~~~~~~~~~~

- Live chain operator APIs (e.g. Shell, Circle K, NREL): High.
- OSM cross-referenced with NREL (match within 30 days): Medium–High.
- iOverlander CSV import, verified within 60 days: Medium–High (offline sync only).
- OSM with known chain operator tag: Medium.
- OSM independent operator, edited within 12 months: Medium–Low.
- OSM independent operator, 1–3 years old: Low in remote areas.
- OSM any operator, more than 3 years old: Low (high closure risk in depopulating areas).
- OSM in known depopulating county/region: Very Low.
- User-confirmed on current trip: High (overrides others until trip end).

Chain operator APIs (with public or partner endpoints)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- **Shell** — https://developer.shell.com/api-catalog (OAuth 2.0, free developer account).
- **Circle K** — https://www.developer.circlek.com/ (partner access).
- **NREL Alternative Fuel Stations** (US/Canada) — https://developer.nrel.gov/docs/transportation/alt-fuel-stations-v1/ (free API key, conventional fuel, LPG, CNG, hydrogen, EV).

Operators without public API (use OSM + operator name, and NREL where applicable): Esso/ExxonMobil, BP, Costco Fuel, Pilot/Flying J, Love's, TA/Petro, Uno-X, St1, Ampol, Caltex, 7-Eleven Fuel.

Location-aware remote mode
--------------------------

Automatic detection
~~~~~~~~~~~~~~~~~~~

Remote mode activates when any of the following hold along the planned route:

- **POI density** — Average spacing between confirmed Medium-or-higher POIs exceeds a threshold (default 80 km fuel, 40 km water on foot) in the route corridor (default 50 km width).
- **Köppen climate** — Route segment midpoint in BWh, BWk, BSh, or BSk (hot/cold desert, hot/cold semi-arid). Uses a bundled lightweight Köppen–Geiger–derived dataset.
- **Known sparse regions** — Route enters maintained bounding polygons for depopulation, remoteness, or difficult access (e.g. US Great Plains counties, Alaska highway, Australian Cape York/Kimberley, Nordic Finnmark/Hardangervidda/Svalbard, Central Asia corridors, Atacama/altiplano/Patagonia, Canadian northern highways). Custom regions configurable.
- **Census/population layer** (optional) — US Census county-level population change; counties with >20% loss since 2000 trigger confidence downgrade. Similar datasets for Australia, Canada, Norway can be integrated.
- **OSM terrain tags** — natural=desert, natural=scrub, etc. used as a supporting signal only; not primary trigger.

Manual override
~~~~~~~~~~~~~~~

Remote mode can be forced on or off in the configuration UI (per-trip, not global): to force conservative planning, or to disable when auto-detection incorrectly flags a region.

Resource planning in remote mode
--------------------------------

Buffer thresholds (defaults)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Standard: fuel 25 km, water (foot) 5 km.
- Remote (auto): fuel 85 km, water 20 km.
- Remote (arid/desert): fuel 140 km, water 30 km.
- User-configured overrides available.

Buffers apply to confirmed resupply points only. Low-confidence POIs do not reduce the buffer requirement.

Lookahead planning
~~~~~~~~~~~~~~~~~

In remote mode the plugin scans the full route at calculation time, assigns confidence, and builds a resupply plan. If any gap between confirmed stops exceeds usable range (with buffer), it warns before departure. This allows carrying extra fuel or choosing an alternative route instead of discovering a long unserviced gap mid-journey.

Carry capacity and desert warnings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When a gap exceeds standard tank range, the plugin can indicate extra fuel (litres), need for jerry can/auxiliary tank, and weight impact on consumption. For desert conditions it warns that actual consumption may be higher than the kinematic model (AC load, high temperature, unpaved/sand Crr).

Unconfirmed stops
~~~~~~~~~~~~~~~~~

Low-confidence POIs are shown with a distinct indicator (e.g. question mark or warning). The planning engine does not rely on them. If the user confirms a stop as operational during the trip, confidence is upgraded to High for the remainder and the plan is recalculated; the confirmation can be stored (e.g. SQLite) for future reference.

Foot travel and water sources
-----------------------------

The same resource model applies with water in place of fuel. Water consumption can be estimated from body weight, temperature, exertion (grade/pace), and humidity. The plugin can apply a heat stress check (e.g. WBGT approximation): warn above 28°C WBGT for strenuous activity, strong warning above 32°C, and avoid planning sustained exertion above 35°C without explicit user override.

OSM tags for water: amenity=drinking_water, natural=spring, man_made=water_well, amenity=water_point, amenity=shelter. Natural sources in arid regions may get a seasonal/drought downgrade.

Configuration UI (Remote Planning section)
------------------------------------------

- **Remote mode** — Off / Auto-detect / Always on. Default: Auto-detect.
- **POI density threshold** — km; default 80.
- **Köppen classification trigger** — On/Off; default On.
- **Fuel buffer (standard / remote / arid)** — km; defaults 25, 85, 140.
- **Water buffer (standard / remote / arid)** — km; defaults 5, 20, 30.
- **Desert consumption warning** — On/Off; default On.
- **Census depopulation layer** — On/Off; default On where data available.
- **Chain API queries** — On/Off; default On (disable for offline).
- **Unconfirmed POI display** — Show/Hide; default Show with warning.
- **Custom sparse regions** — User-defined bounding areas that always trigger remote mode.
- **Foot travel mode** — Enables water source planning and water consumption model.
- **Body weight** (foot) — kg for water calculation.
- **Temperature source** (foot) — Weather feed or manual entry.
- **Heat stress warnings** (foot) — On/Off; default On.

Enabling the plugin
-------------------

Build with SQLite3 available; the plugin is built when the driver_break plugin is enabled (same dependency). Add the plugin to your Navit config and enable the OSD:

- Load the plugin (e.g. in navit.xml or your config): ``<plugin path="$NAVIT_LIBDIR/plugin/${NAVIT_LIBPREFIX}libosd_navit_safety.so" .../>``
- Add an OSD instance: ``<osd type="navit_safety" enabled="yes"/>``

Data sources and licensing
--------------------------

- **NREL Alternative Fuel Stations** — US Government open data.
- **Köppen–Geiger** (e.g. Beck et al., gloh2o.org/koppen) — Check dataset licence for bundling.
- **OSM** — ODbL; credit OpenStreetMap contributors.
- **iOverlander CSV** — CC BY-SA; offline import only.

Notes
-----

The conservative defaults are intentional. In remote areas, erring on the side of caution is justified; users can reduce buffers in the UI if needed. Automatic detection is designed so that most users in populated areas never need to change settings; remote mode activates only when the route enters sparse or arid terrain.
