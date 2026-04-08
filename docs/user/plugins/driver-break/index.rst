Plugins
=======

Navit includes several plugins that extend its functionality:

.. toctree::
   :maxdepth: 2
   :caption: Contents

   Driver Break Plugin <self>
   How the plugin works <how_it_works>
   Driver Break ECU ports <ecu_ports>
   Driver Break EV backend (to-do) <todo-electric>
   Driver Break EV vehicle profile (SQLite) <ev_vehicle_profile>
   Driver Break Tests <tests>
   Driver Break test results (captured run) <test-results>
   Driver Break aftermarket ECUs <aftermarket_ecus>
   Driver Break formulas <formulas>
   Driver Break Navit-daemon integration <navit_daemon_integration>
   Driver Break DBus API (eco_mode_fuel_enabled) <dbus>
   Driver Break OSD example menu tree <osd_example_menu_tree>
   Driver Break OSD commands <osd_commands>

.. contents:: On this page
   :local:
   :depth: 2

.. _driver-break-plugin:

Driver Break Plugin
-------------------

The Driver Break plugin provides configurable rest period management for multiple travel modes. It tracks activity time, suggests rest stops using OpenStreetMap data, discovers nearby Points of Interest (POIs), and supports route validation and energy-based routing where applicable. When a compatible vehicle ECU is present, live fuel consumption data is read directly from the vehicle and used to improve route calculations.

.. _travel-modes:

Travel modes
~~~~~~~~~~~~

- **Car** – Configurable soft and maximum driving hours, break interval (e.g. every 4–4.5 hours), and break duration (e.g. 15–45 minutes). Optional **remote/arid/hot climate** mode adds water POIs (drinking water, fountain, spring) at rest stops (see :ref:`water-sources-and-filtration`).
- **Truck** – Mandatory rest and driving time rules aligned with EU Regulation EC 561/2006 and related rules (e.g. break after 4.5 hours, 45-minute break, max daily driving, daily and weekly rest, and other driving/rest time limits). Optional **remote/arid/hot climate** mode adds water POIs at rest stops.
- **Hiking** – Daily segments: 40 km suggested maximum per day. Rest stops at 11.295 km intervals (main) or 2.275 km (alternative). Optional SRTM elevation and POI support (water, cabins).
- **Bicycle (cycling)** – Daily segments: 100 km suggested maximum per day. Rest stops at 28.24 km intervals (main) or 5.69 km (alternative). Suggested defaults use the same rast/vei concept as hiking, scaled up for cycling (see :ref:`rast-and-vei` below).
- **Motorcycle** – Soft limit 2 hours riding, mandatory break after 3.5 hours, break duration 15–30 minutes (all configurable). Terrain sub-type: **Road** (paved only: surface=asphalt, paved) or **Adventure/dual-sport** (additionally gravel, unpaved, track with tracktype grade1–3 and configurable smoothness; see :ref:`motorcycle-adventure-legal`). Routing prefers motorcycle=yes/designated/permissive and filters motorcycle=no and motor_vehicle=no. Fuel: OBD-II for Euro 4+ bikes, adaptive estimation and manual tank/consumption for others. Energy-based routing uses configurable rider+bike weight (default 250 kg). Optional **remote/arid/hot climate** mode adds water POIs at rest stops.

.. _motorcycle-adventure-legal:

Motorcycle adventure mode – legal restrictions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Adventure/dual-sport mode is strictly limited to legally accessible ways. The plugin never routes across uncultivated land (utmark), unmapped terrain, or ways tagged access=private or access=no. Only highway=track and similar ways with explicit access=yes, access=permissive, or motorcycle=yes/designated/permissive are considered. Off-road motor traffic on uncultivated land is prohibited without a permit in many countries. When adventure mode is active, a country-aware warning is shown reminding the user of these rules. Relevant legislation includes: Norway – Motorferdselloven (1977); Sweden – Terrängkörningslagen (1975:1313); Finland – Maastoliikennelaki (1710/1995); and similar laws in most other European countries.

.. _pois-searched:

POIs searched
~~~~~~~~~~~~~

The plugin discovers nearby Points of Interest depending on travel mode. Search radii are configurable (e.g. water 2 km, cabins 5 km, general POI 15 km, network huts 25 km). Searched POI types include:

- **Water** – Drinking water, fountain, spring (for hiking/cycling rest stops; and for car, truck, motorcycle when **remote/arid/hot climate** is enabled in Configure overnight).
- **Cabins and huts** – Wilderness hut, alpine hut, hostel, camping; with optional DNT/network detection for prioritization.
- **Car** – Cafe, restaurant, museum, viewpoint, picnic, attraction (and similar amenities along driving routes).
- **Motorcycle** – Same as car (cafe, restaurant, viewpoint, petrol, picnic), plus amenity=motorcycle_repair and shop=motorcycle when present in map data.
- **Cycling** – Same water and cabin/hut categories as hiking at rest stops. For **e-bikes and utility cycling**, charging is searched explicitly: ``amenity=charging_station`` (Overpass when libcurl is available, and in the general Overpass fallback without a map). Also at rest stops: from map data, bike shop, generic repair (often bicycle repair), bicycle rental, and bicycle parking; plus Overpass for ``shop=bicycle`` and ``amenity=bicycle_repair_station`` (merged with map hits when libcurl is available). Radii for water and cabins are set in Configure overnight; the **general POI search radius** there also limits how far the plugin looks for cycling service POIs (map + Overpass) around each cycling rest stop. With **hiking/pilgrimage priority** enabled, **hiking and cycling** rest stops also include **places of worship** (``amenity=place_of_worship``) for churches and pilgrim services along the stage (same general POI radius).

.. _distance-from-buildings:

Distance from buildings (camping, allemannsretten)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For overnight or camping stops (e.g. when using right-to-roam rules such as Norwegian allemannsretten), the plugin enforces a configurable minimum distance from buildings or dwellings. Rest stop candidates too close to buildings are filtered out so suggested sites respect local camping rules (default e.g. 150 m; configurable).

.. _distance-from-glaciers:

Distance from glaciers (overnight)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Nightly camping positions too close to glaciers are rejected. A configurable minimum distance (e.g. 300 m) applies; the check can be relaxed when the location has a camping building.

.. _water-sources-and-filtration:

Water sources and filtration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When using water from natural sources such as streams, lakes, or springs suggested as rest stop POIs, always treat or filter the water before drinking. A good portable filter should include:

- A hollow fiber or ceramic membrane rated at 0.1 micron or smaller to remove bacteria, protozoa, and parasites such as Giardia and Cryptosporidium
- An activated carbon stage (preferably carbon block) to remove pesticides, herbicides, heavy metals, chlorine, and improve taste and odour
- Look for NSF/ANSI 42 and 53 certified filters

Note that standard filters do not remove viruses. In remote wilderness areas (Norway, Sweden, Finland) viral contamination is generally low risk, but in areas near settlements, farms, or grazing land, consider using a filter rated to 0.02 micron or adding chemical treatment (iodine tablets, chlorine dioxide) as a second stage.

Water sources inside Norwegian national parks and nature reserves are generally considered safe to drink directly, as these areas are protected from agricultural runoff, grazing, and industrial activity. However, even in protected areas it is advisable to collect water from fast-moving streams or high-altitude sources rather than slow or stagnant water, and to avoid collecting immediately downstream of trails, campsites, or huts.

Even water tagged as ``amenity=drinking_water`` on OpenStreetMap should be treated with caution in remote areas, as tags may be outdated or conditions may have changed.

.. _configurable-rest-stop-periods:

Configurable rest stop periods
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Rest parameters are configurable per mode, including:

- Car: soft limit hours, max hours, break interval (hours), break duration (minutes).
- Truck: mandatory break after (hours), break duration (minutes), max daily driving hours.
- Hiking: main and alternative break distances (km), max daily distance (km).
- Cycling: main and alternative break distances (km), max daily distance (km).
- Motorcycle: soft limit (minutes), mandatory break after (minutes), break duration (minutes), terrain sub-type (road/adventure), adventure max smoothness and tracktype, default weight for energy routing.
- General: rest interval range (min/max hours), POI search radii, minimum distance from buildings (camping / allemannsretten), minimum distance from glaciers for overnight stops, and **water POIs at rest stops** for car/truck/motorcycle (remote/arid/hot climate).

.. _example-osd-navit-xml:

Example layered OSD (navit.xml)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A full **text-only** on-screen menu example using ``osd_configuration`` bitmasks (main menu, travel mode, per-mode break settings, routing toggles, **elevation data** at bitmask ``1024``) and the commands ``driver_break_open_settings()``, ``driver_break_set_mode()``, ``driver_break_toggle()``, plus SRTM helpers ``srtm_download_menu()``, ``srtm_download_region()``, and ``srtm_get_elevation()`` is available as :download:`navit_driver_break_osd_example.xml` and on GitHub as `navit_driver_break_osd_example.xml <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/navit_driver_break_osd_example.xml>`__. It includes a minimal valid ``<config>`` skeleton; merge the Driver Break OSD block into your own ``navit.xml`` or adapt paths and layout as needed.

An ASCII tree of the same menu (flags, navigation, and dialog commands) is in :doc:`osd_example_menu_tree` and on GitHub as `osd_example_menu_tree.rst <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/osd_example_menu_tree.rst>`__.

A concise reference for every registered command name, arguments, and SRTM helpers is in :doc:`osd_commands`.

Hiking break settings use a **second page** (bitmask ``256``) so POI, DNT/pilgrimage, and overnight rows stay on screen on typical portrait resolutions; page 1 (bit ``8``) holds interval distances and a "More hiking settings..." control. Motorcycle uses the same pattern: page 1 (bit ``128``) for time-interval rows, page 2 (bitmask ``512``) for POI, overnight, and scenic/tour lines, with "More motorcycle settings..." on page 1. The **Elevation data** submenu (bitmask ``1024``) opens the SRTM download manager, region picker, example named-region downloads, and elevation-at-position; tile coverage follows **fixed region bounding boxes** in the plugin, not the loaded map or route (see :doc:`osd_commands`). The **Routing mode** submenu uses **fixed labels** for Kinetic and Eco: they do not reflect on or off state in the UI, but each line still toggles the corresponding option and persists it; the example file comments describe this limitation and a possible future approach (expression-bound labels if attributes become available). **Kinetic** (energy-based routing) only applies where :ref:`energy-based-routing` can run: **terrain profile (elevation) data must be present** along routes (see :ref:`srtm-elevation`); toggling it without DEM coverage does not yield useful energy-based discrimination between paths.

.. _driver-break-example-icons:

Example OSD icons (Driver Break)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Example SVG assets for kinetic (energy-based) routing and eco / ECU-related routing when you use ``type="button"`` with ``src=`` instead of text-only OSD items. Source files in the repository:

- `kinetic-routing.svg <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/Icons/kinetic-routing.svg>`__
- `nav_eco-mode.svg <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/Icons/nav_eco-mode.svg>`__

.. image:: Icons/kinetic-routing.svg
   :width: 96px
   :alt: Kinetic routing example icon for Driver Break OSD

.. image:: Icons/nav_eco-mode.svg
   :width: 96px
   :alt: Eco mode example icon for Driver Break OSD

.. _rast-and-vei:

Rast and vei (historical basis for hiking and cycling defaults)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The suggested default rest intervals for hiking (11.295 km main, 2.275 km alternative; 40 km daily max) and for cycling (28.24 km main, 5.69 km alternative; 100 km daily max) are inspired by the old Scandinavian units of length **rast** and **vei**. For cycling, the same rast/vei concept is used with distances scaled up. A "rast" was the distance one traveled on foot before needing a rest ("rast," "pause," or the like); it corresponded to a **mil** and was often tied to the length of the ell. The distance varied by region and over time. In the 900s a rast was about 192 stone throws, divided into four quarters ("fjerdingvei"), and corresponded to roughly 9,100.8 meters; in the 12th century it was expressed as 16,000 ells (four quarters of 8,000 feet) but remained in the same order of magnitude.
A "dagsvei" (day's way/journey) was a traditional Scandinavian unit meaning roughly how far you could walk in a day, commonly reckoned at about 40 km.

The plugin's hiking and cycling interval defaults follow from these historical rast-based distances.

.. _networks-and-priorities:

Networks and priorities
~~~~~~~~~~~~~~~~~~~~~~~

- **DNT/network priority** – Optional priority for network huts (e.g. Norwegian Trekking Association, DNT) with configurable hut search radius. Set the network hut search radius according to typical spacing (see below); in remote areas consider raising it toward the upper range to include the next cabin.

  **Networked cabin spacing (nearest-neighbor distances, for setting search radius):**

  - **Norway (DNT)** – OpenStreetMap relation 1110420 (DNT cabins): 449 huts; average 10.56 km, median 8.83 km, max 100.45 km.
  - **Sweden** – Overpass API (``tourism=wilderness_hut``, ``tourism=alpine_hut`` in Sweden): 439 huts total; average 12.31 km, median 8.24 km, max 83.85 km. STF (Svenska Turistföreningen) network only (42 huts): average 14.47 km, median 11.50 km, max 83.85 km.
  - **Finland** – Overpass API (``tourism=wilderness_hut``, ``tourism=alpine_hut`` in Finland): 324 huts total; average 11.72 km, median 6.68 km, max 64.32 km. Metsähallitus network only (108 huts): average 16.05 km, median 5.31 km, max 247 km (remote areas).
  - **Germany** – Overpass API (``tourism=wilderness_hut``, ``tourism=alpine_hut`` in Germany): 261 huts total; average 12.98 km, median 9.72 km, max 119.76 km. DAV (Deutscher Alpenverein) network: 22 huts in sample.
  - **Switzerland** – Overpass API (``tourism=wilderness_hut``, ``tourism=alpine_hut`` in Switzerland): 328 huts total; average 4.40 km, median 3.82 km, max 23.70 km. SAC/CAS (Schweizer Alpen-Club) and similar: 66 huts in sample; denser spacing in the Alps.
  - **Austria** – Overpass API (``tourism=wilderness_hut``, ``tourism=alpine_hut`` in Austria): 330 huts total; average 5.30 km, median 3.56 km, max 102.51 km. OeAV (Österreichischer Alpenverein) and similar: 22 huts in sample; denser spacing in the Alps.

  **Open / non-networked huts** (no ``network`` tag, not operated by DNT/STF/DAV/SAC/OeAV/Metsähallitus etc.) can also show somewhat regular spacing in the same countries. Use the same Overpass tag set and exclude networked operators; nearest-neighbor distances (for setting cabin search radius) in the samples:

  - **Germany** – 235 huts; average 14.29 km, median 10.22 km, max 119.76 km.
  - **Switzerland** – 261 huts; average 4.93 km, median 4.03 km, max 23.70 km.
  - **Austria** – 287 huts; average 5.71 km, median 3.65 km, max 102.51 km.
  - **Sweden** – 395 huts; average 12.45 km, median 7.85 km, max 64.86 km.
  - **Finland** – 206 huts; average 17.00 km, median 12.33 km, max 75.75 km.

  Set cabin search radius in the typical-to-max range for the region (e.g. 5–15 km in the Alps, 10–20 km in Scandinavia/Germany/Finland for open huts).

  Use at least the typical spacing (e.g. 10–12 km) so nearby huts are found; in remote areas consider raising the radius toward the max values.

- **Hiking/pilgrimage priority** – Optional preference for official hiking and pilgrimage routes when validating routes, and for rest-stop POI discovery on **hiking and cycling**: when enabled, the plugin also searches for ``amenity=place_of_worship`` (churches and similar) within the **general POI search radius**, for stages along pilgrim paths that offer services. Map search uses worship/church item types; Overpass supplements when libcurl is available.

.. _hiking-route-validation:

Hiking route validation
~~~~~~~~~~~~~~~~~~~~~~~

For hiking routes, the plugin can validate that the route avoids forbidden road types (e.g. motorway, trunk, primary) and reports how much of the route uses priority paths (footway, path, track, steps, bridleway). With pilgrimage/hiking priority enabled, segments tagged as pilgrimage or hiking routes are counted as priority. The same toggle adds **place of worship** POIs near hiking (and cycling) rest stops, as above. Warnings are produced for high forbidden percentage or low priority path percentage.

.. _cycling-route-validation:

Cycling route validation
~~~~~~~~~~~~~~~~~~~~~~~~

The API ``route_validator_validate_cycling()`` classifies each route segment using the same **forbidden** highway set as hiking (motorway, trunk, primary, and links). **Cycle-priority** distance includes: ``highway=cycleway``; ways with ``route=bicycle`` or ``route=mtb``; ``network`` of ``ncn``, ``rcn``, ``lcn``, or ``icn`` (international / national / regional / local signed cycle networks); and ``bicycle=yes`` or ``bicycle=designated`` on path-like highways (path, footway, track, living_street, pedestrian, service). Segments that would otherwise count as secondary but are tagged ``pilgrimage=yes`` are promoted to cycle-priority (forbidden highways are never promoted). Tags are read from way strings on Navit street items when present (relation-only route membership may not appear on all extracts).

**MTB scale warnings (threshold ≥ 4):** The plugin scans each segment’s tags for ``mtb:scale`` (0–6, using the leading digit) and ``mtb:scale:uphill`` (0–5, leading digit). It records the **maximum** value seen along the route. A warning is added only when that maximum is **4 or higher**: for ``mtb:scale``, that flags **difficult MTB terrain** (OSM scale 4+ is advanced / expert); for ``mtb:scale:uphill``, that flags **steep or very steep uphill** segments. Values 0–3 do not produce these warnings. Callers can use this alongside hiking validation where appropriate.

.. _energy-based-routing:

Energy-based routing (car, truck, motorcycle, cycling, hiking)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The plugin can steer routing toward **lower predicted energy use** in two related ways:

- **Kinetic (energy) routing** – When enabled (``use_energy_routing``), Navit uses a **kinematic model** per route segment: total mass, rolling resistance, air drag (from **Cd** and **frontal area**), elevation change, and downhill recuperation contribute to a segment **cost**. The router prefers paths with lower summed cost. This applies in principle to **motorized and human-powered** profiles (car, truck, motorcycle, cycling, hiking) whenever the router consults this cost; you must supply parameters that match the **current** mode (see below).

- **Eco / ECU route cost** – A separate flag (``use_ecu_route_cost``) controls whether **live or learned fuel consumption** from the plugin (see :ref:`fuel-monitoring` and :ref:`adaptive-fuel-learning`) is blended into route costs. This refines **motor-fuel** routing when data is available; it is not a substitute for setting mass and aerodynamics for the kinetic model.

**A terrain profile (per-segment elevation along the route) must be available** for kinetic routing to be useful: without elevation data, grade and potential-energy terms are not meaningful and the model cannot reliably prefer flatter or easier grades over hilly shortcuts. Obtain coverage for your regions via SRTM / DEM tiles as described under :ref:`srtm-elevation`. Formulas and implementation details are in :doc:`formulas` and :doc:`how_it_works`.

How to use (kinetic and eco)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. **Load the plugin** – Include an enabled ``<osd type="driver_break" .../>`` in ``navit.xml`` so commands and configuration are available (:ref:`example-osd-navit-xml`).

2. **Provide elevation data** – Download tiles for the regions you route through (:ref:`srtm-elevation`, :doc:`osd_commands`). Without tiles, toggling kinetic routing on has little practical effect on route choice.

3. **Set travel mode** – Use ``navit.driver_break_set_mode(...)`` or your vehicle profile so the plugin mode matches reality (car, truck, motorcycle, hiking, cycling). Mode affects **rest stops, POIs, and fuel backends**; it does **not** automatically switch kinetic parameters (mass, Cd, area).

4. **Set kinetic parameters** – Adjust **total mass (kg)**, **drag coefficient Cd** (dimensionless), and **frontal area (m**\ :sup:`2`\ **)** so they describe the **rider or vehicle system** you are routing for. Use the Navit commands (below), the illustrative presets in :download:`navit_driver_break_osd_example.xml`, or edit the SQLite database keys if you maintain config offline. After a mode change (e.g. car to motorcycle), update these values if the defaults no longer match.

5. **Enable kinetic routing** – Run ``navit.driver_break_toggle(kinetic)`` from an OSD or script until **kinetic** is on (the example menu uses **fixed labels** that do not show on/off; each tap toggles). The state is stored as ``use_energy_routing`` in the plugin database and persists across sessions.

6. **Optional: enable ECU eco cost** – For cars, trucks, or motorcycles with fuel monitoring, run ``navit.driver_break_toggle(eco)`` to turn **ECU-weighted route cost** on (``use_ecu_route_cost``). This uses live or adaptive fuel data when :ref:`eco-mode-fuel-attribute` is satisfied. Kinetic and eco can be enabled independently; together they combine physics-based costs with observed consumption where implemented.

Full command syntax and argument types are in :doc:`osd_commands`.

Parameters the plugin expects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

These values are **validated** when set via commands (invalid numbers are rejected with a user message). They are persisted in the plugin SQLite database when the DB is available.

.. list-table::
   :header-rows: 1
   :widths: 18 28 20 34

   * - Meaning
     - SQLite key
     - Allowed range
     - Typical use
   * - Total mass
     - ``total_weight``
     - 1–50000 kg
     - Car/truck: laden vehicle. Motorcycle: rider + bike + gear. Cycling/hiking: rider + bike or pack as appropriate.
   * - Drag coefficient **Cd**
     - ``energy_drag_cd``
     - 0.01–1.5 (dimensionless)
     - Shape of the moving body; with frontal area sets air drag at speed.
   * - Frontal area
     - ``energy_frontal_area_sqm``
     - 0.05–20 m\ :sup:`2`
     - Projected area for aerodynamic drag; small for an upright cyclist, larger for a car or truck.

**Commands:** ``navit.driver_break_set_total_weight(…)``, ``navit.driver_break_set_drag_cd(…)``, ``navit.driver_break_set_frontal_area(…)`` – see :doc:`osd_commands`.

**Defaults** after a fresh install (before DB overrides) are oriented toward a **light passenger car** order of magnitude for Cd and area, with a **low** default mass (see :doc:`how_it_works`); for **hiking or cycling**, lower **frontal area** and set **mass** to your body plus equipment (and bicycle if applicable). **Motorcycle** interval settings include a separate default weight hint for the UI; the kinetic model still uses the ``total_weight`` field until you change it.

**Per-mode storage:** The plugin keeps **one** triple (mass, Cd, area) at a time. Switching travel mode does not load alternate presets automatically.

How these settings affect behaviour
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **Kinetic routing on** – Segment costs include work against **rolling resistance** (scales with **total mass**), **air drag** (scales with **Cd**, frontal area, and effective speed), and **grade** (needs elevation along the segment). Downhill segments can recover part of the energy via the model’s recuperation term. **Route choice** may shift toward longer but flatter or slower-speed paths when that lowers total predicted energy. **Rest-stop timing**, break dialogs, and POI logic are **not** driven by these numbers; they still follow :ref:`travel-modes` and your interval configuration.

- **Kinetic routing off** – Navit’s usual time/distance routing applies (subject to profile and traffic); the stored mass, Cd, and area remain in the database for the next time you enable kinetic routing.

- **Eco / ``use_ecu_route_cost`` on** – When fuel data is available, route costs can reflect **observed or learned consumption** in addition to or instead of pure time-based costs (depending on integration in your build). If no ECU is connected and adaptive learning is off, turning **eco** on may have limited effect until data exists.

- **Poor parameter choice** – Overstated mass or drag makes **every** segment look “expensive” in a similar way, which **weakens** the model’s ability to separate “easy” vs “hard” alternatives; wrong elevation is worse than wrong Cd for **hill** decisions. Prefer realistic values for the **current** trip mode.

.. _srtm-elevation:

SRTM elevation
~~~~~~~~~~~~~~

The plugin can use elevation data to improve rest stop suggestions and energy-based routing. **Energy-based (kinetic) routing depends on this terrain profile information** being available for the route corridor (see :ref:`energy-based-routing`). Elevation data is downloaded on demand by region and queried automatically as needed. Three elevation sources are supported and tried in order of preference: Copernicus DEM GLO-30, Viewfinder Panoramas, and NASA SRTMGL1. Downloads can be paused, resumed, or cancelled, and progress is tracked per region. Elevation is used where applicable for routing and display.

.. _fuel-monitoring:

Fuel monitoring (OBD-II, J1939, MegaSquirt)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The plugin can read live fuel consumption directly from the vehicle's ECU. This information is used to track fuel use over time and, when :ref:`energy-based-routing` is enabled, to improve the accuracy of route calculations. Three ECU backends are supported:

- **OBD-II (ELM327)** – Works with any petrol, diesel, or flex-fuel car or light vehicle equipped with a standard OBD-II port and an ELM327-compatible adapter. The plugin reads fuel level, fuel rate, and where available ethanol content. For vehicles that do not report a direct fuel rate, consumption is estimated from the mass air flow sensor reading. If the adapter is not present or does not respond correctly, the plugin falls back to manual and adaptive estimation without any loss of other functionality.

- **J1939 (SocketCAN)** – Intended primarily for trucks and heavy vehicles. The plugin listens for engine fuel rate and fuel level signals broadcast over the vehicle's CAN bus. This backend is used automatically in truck mode when a CAN interface is available.

- **MegaSquirt** – Supports aftermarket and performance ECUs from the MegaSquirt family, including MS1, MS2, MS3, MS3-Pro, and MicroSquirt. These are commonly used in kit cars, race vehicles, and custom engine installations. The plugin connects to the ECU over a serial connection and reads engine data to calculate fuel consumption. As with the other backends, if the ECU is not available or not configured the plugin continues to work normally using adaptive estimation.

Planned support for **battery electric vehicles** (live SoC, power, and related OBD/CAN data) is outlined in `todo-electric.rst <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/todo-electric.rst>`__.

In the fuel configuration dialog (Configure fuel), one toggle turns live ECU on or off for all three backends (OBD-II, J1939, MegaSquirt) together. Press OK to save.

.. _adaptive-fuel-learning:

Adaptive fuel learning
~~~~~~~~~~~~~~~~~~~~~~

Even without a live ECU connection, the plugin builds up a fuel consumption model over time by recording consumption samples and trip summaries as you travel. This learned data is stored persistently and used to progressively refine fuel estimates for future trips. Fuel stops are also logged alongside rest stop history, giving a complete picture of stops made during a journey. A configuration switch controls whether adaptive fuel learning is enabled; when disabled, no samples or trip summaries are recorded. In the fuel configuration dialog, one toggle turns adaptive fuel learning on or off (press OK to save). Energy-based routing remains available for cars and trucks even when no ECU data and no adaptive learning are in use.

.. _eco-mode-fuel-attribute:

Attribute ``eco_mode_fuel_enabled`` (for DBus / API)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The plugin exposes a boolean attribute **eco_mode_fuel_enabled**. It is **true** when either (1) an ECU backend is available and running (OBD-II, J1939, or MegaSquirt), or (2) adaptive fuel learning is enabled in configuration. This allows external components to detect whether the plugin is using live fuel data or learned consumption for eco/fuel-related behaviour.

**DBus:** The attribute is available on the Navit DBus interface. Call ``get_attr("eco_mode_fuel_enabled")`` on the navit object; the method returns ``(attrname, value)`` with a boolean value. No need to resolve the Driver Break OSD; Navit aggregates the value from its OSDs. For service name, object paths, and examples in Python and with ``dbus-send``, see `dbus.rst <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/dbus.rst>`__.

.. _history-and-persistence:

History and persistence
~~~~~~~~~~~~~~~~~~~~~~~

The plugin stores rest stop history, fuel stop history, fuel consumption samples, trip summaries, and all configuration persistently across sessions. History can be browsed in the GUI and older entries can be cleared. Configuration changes made through the menus are saved automatically and restored on the next start.

.. _config-file-vs-menus:

Configuration: config file vs on-screen menus
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**What you configure in the config file (e.g. navit.xml)**

- **Plugin load** – Include ``<plugin path="$NAVIT_LIBDIR/plugin/${NAVIT_LIBPREFIX}libosd_driver_break.so"/>`` so the Driver Break plugin is loaded.
- **OSD on/off** – ``<osd enabled="yes" type="driver_break"/>`` enables the plugin OSD and its menu commands; ``enabled="no"`` disables it.
- **Initial vehicle mode (optional)** – On the same ``<osd>`` element, ``<attr name="type" value="car"/>`` (or ``truck``, ``hiking``, ``cycling``, ``motorcycle``) sets the initial rest/fuel mode if present; otherwise the plugin uses the active vehicle's profile.
- **Database path (optional)** – ``<attr name="data" value="/path/to/driver_break.db"/>`` on the ``<osd>`` element sets the SQLite database file for config and history; if omitted, a default path is used.
- **Vehicle profile** – The active vehicle's ``profilename`` (e.g. ``profilename="car"``, ``"truck"``, ``"motorcycle"``) determines which rest and fuel behaviour applies and which "Configure intervals" dialog is shown. Define vehicle profiles and assign them to vehicles in the config file.

**What you can turn on/off or set in on-screen menus**

- **Configure Driver Break Intervals** – Opens a profile-specific dialog. You can set: car (soft limit, max hours, break interval, break duration); truck (mandatory break after, duration, max daily hours); hiking (main/alt break distances, max daily distance); cycling (same); motorcycle (soft limit, mandatory break after, break duration, terrain Road/Adventure). Values are saved to the database.
- **Configure Overnight Stops** – Shows min distance from buildings, glaciers (for hiking), POI search radius, water search radius, cabin search radius, and a **toggle for water POIs at rest stops** (remote/arid/hot) for car, truck, and motorcycle. Use the toggle to turn that option on or off; press OK to save.
- **Configure fuel** – Two toggles: **live ECU** (OBD-II, J1939, MegaSquirt) on/off, and **adaptive fuel learning** on/off. Press OK to save. Separate menu actions: set fuel level, log fuel stop.
- **Other menu actions** – Suggest rest stop, Rest stop history, Start break, End break (no toggles; they perform an action).

All settings changed in the dialogs are stored in the plugin database and restored on the next start.

.. _ui-and-configuration:

User interface and configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The plugin registers as an OSD element in Navit's display. With the internal GUI, menu actions are available: suggest rest stop (along current route), rest stop history, start break, end break, configure intervals (per profile: car, truck, hiking, cycling, motorcycle), configure overnight settings (minimum distance from buildings and glaciers, POI search radii, water POIs toggle), and configure fuel. In the fuel configuration dialog, two toggles are available: one for live ECU (OBD-II, J1939, MegaSquirt) and one for adaptive fuel learning; press OK to save. Session state — including driving time, whether a break is in progress, and whether a mandatory break is required — is tracked continuously. Vehicle type is selected via the active vehicle's profile (config file) or the OSD ``type`` attribute.

Further reading
~~~~~~~~~~~~~~~

For details on specific aspects of the Driver Break plugin, see:

* `How the plugin works <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/how_it_works.rst>`__
* `ECU ports <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/ecu_ports.rst>`__
* `EV backend (to-do, design checklist) <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/todo-electric.rst>`__
* `EV vehicle profile table (SQLite spec) <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/ev_vehicle_profile.rst>`__
* `EV vehicle profile DDL (SQL file) <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/ev_vehicle_profile.sql>`__
* `Tests <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/tests.rst>`__
* `Test results (captured run) <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/test-results.rst>`__
* `Aftermarket ECUs <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/aftermarket_ecus.rst>`__
* `Formulas <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/formulas.rst>`__
* `Navit-daemon integration <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/navit_daemon_integration.rst>`__
* `DBus API (eco_mode_fuel_enabled) <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/dbus.rst>`__
* `OSD example menu tree (ASCII) <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/osd_example_menu_tree.rst>`__
* `OSD commands reference <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/osd_commands.rst>`__
* `Example OSD icons (SVG) <https://github.com/Supermagnum/navit/tree/feature/driver-break/docs/user/plugins/driver-break/Icons>`__
