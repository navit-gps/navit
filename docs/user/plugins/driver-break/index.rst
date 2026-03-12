Plugins
=======

Navit includes several plugins that extend its functionality:

.. toctree::
   :maxdepth: 2
   :caption: Contents

   Driver Break Plugin <self>
   Driver Break ECU ports <ecu_ports>
   Driver Break Tests <tests>
   Driver Break aftermarket ECUs <aftermarket_ecus>
   Driver Break formulas <formulas>
   Driver Break Navit-daemon integration <navit_daemon_integration>

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

- **Car** – Configurable soft and maximum driving hours, break interval (e.g. every 4–4.5 hours), and break duration (e.g. 15–45 minutes).
- **Truck** – Mandatory rest and driving time rules aligned with EU Regulation EC 561/2006 and related rules (e.g. break after 4.5 hours, 45-minute break, max daily driving, daily and weekly rest, and other driving/rest time limits).
- **Hiking** – Daily segments: 40 km suggested maximum per day. Rest stops at 11.295 km intervals (main) or 2.275 km (alternative). Optional SRTM elevation and POI support (water, cabins).
- **Bicycle (cycling)** – Daily segments: 100 km suggested maximum per day. Rest stops at 28.24 km intervals (main) or 5.69 km (alternative). Suggested defaults use the same rast/vei concept as hiking, scaled up for cycling (see :ref:`rast-and-vei` below).

.. _pois-searched:

POIs searched
~~~~~~~~~~~~~

The plugin discovers nearby Points of Interest depending on travel mode. Search radii are configurable (e.g. water 2 km, cabins 5 km, general POI 15 km, network huts 25 km). Searched POI types include:

- **Water** – Drinking water, fountain, spring (for hiking/cycling rest stops).
- **Cabins and huts** – Wilderness hut, alpine hut, hostel, camping; with optional DNT/network detection for prioritization.
- **Car** – Cafe, restaurant, museum, viewpoint, picnic, attraction (and similar amenities along driving routes).

.. _distance-from-buildings:

Distance from buildings (camping, allemannsretten)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For overnight or camping stops (e.g. when using right-to-roam rules such as Norwegian allemannsretten), the plugin enforces a configurable minimum distance from buildings or dwellings. Rest stop candidates too close to buildings are filtered out so suggested sites respect local camping rules (default e.g. 150 m; configurable).

.. _distance-from-glaciers:

Distance from glaciers (overnight)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Nightly camping positions too close to glaciers are rejected. A configurable minimum distance (e.g. 300 m) applies; the check can be relaxed when the location has a camping building.

.. _configurable-rest-stop-periods:

Configurable rest stop periods
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Rest parameters are configurable per mode, including:

- Car: soft limit hours, max hours, break interval (hours), break duration (minutes).
- Truck: mandatory break after (hours), break duration (minutes), max daily driving hours.
- Hiking: main and alternative break distances (km), max daily distance (km).
- Cycling: main and alternative break distances (km), max daily distance (km).
- General: rest interval range (min/max hours), POI search radii, minimum distance from buildings (camping / allemannsretten), minimum distance from glaciers for overnight stops.

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

- **Hiking/pilgrimage priority** – Optional preference for official hiking and pilgrimage routes when validating or suggesting stops.

.. _hiking-route-validation:

Hiking route validation
~~~~~~~~~~~~~~~~~~~~~~~

For hiking routes, the plugin can validate that the route avoids forbidden road types (e.g. motorway, trunk, primary) and reports how much of the route uses priority paths (footway, path, track, steps, bridleway). With pilgrimage/hiking priority enabled, segments tagged as pilgrimage or hiking routes are counted as priority. Warnings are produced for high forbidden percentage or low priority path percentage.

.. _energy-based-routing:

Energy-based routing (cycling,car,trucks,walking)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Optional energy-based routing is also an **eco-mode**: it models the physical effort and energy use required for each segment of a route, taking into account total weight, rolling and air resistance, elevation changes, and recuperation on downhill sections. When a compatible ECU is connected (see :ref:`fuel-monitoring`), live fuel consumption data from the vehicle is incorporated into the route cost calculations, giving a more accurate picture of the energy demands of the planned route. This allows Navit to prefer flatter or more energy-efficient (fuel-saving) paths where alternatives exist.

.. _srtm-elevation:

SRTM elevation
~~~~~~~~~~~~~~

The plugin can use elevation data to improve rest stop suggestions and energy-based routing. Elevation data is downloaded on demand by region and queried automatically as needed. Three elevation sources are supported and tried in order of preference: Copernicus DEM GLO-30, Viewfinder Panoramas, and NASA SRTMGL1. Downloads can be paused, resumed, or cancelled, and progress is tracked per region. Elevation is used where applicable for routing and display.

.. _fuel-monitoring:

Fuel monitoring (OBD-II, J1939, MegaSquirt)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The plugin can read live fuel consumption directly from the vehicle's ECU. This information is used to track fuel use over time and, when :ref:`energy-based-routing` is enabled, to improve the accuracy of route calculations. Three ECU backends are supported:

- **OBD-II (ELM327)** – Works with any petrol, diesel, or flex-fuel car or light vehicle equipped with a standard OBD-II port and an ELM327-compatible adapter. The plugin reads fuel level, fuel rate, and where available ethanol content. For vehicles that do not report a direct fuel rate, consumption is estimated from the mass air flow sensor reading. If the adapter is not present or does not respond correctly, the plugin falls back to manual and adaptive estimation without any loss of other functionality.

- **J1939 (SocketCAN)** – Intended primarily for trucks and heavy vehicles. The plugin listens for engine fuel rate and fuel level signals broadcast over the vehicle's CAN bus. This backend is used automatically in truck mode when a CAN interface is available.

- **MegaSquirt** – Supports aftermarket and performance ECUs from the MegaSquirt family, including MS1, MS2, MS3, MS3-Pro, and MicroSquirt. These are commonly used in kit cars, race vehicles, and custom engine installations. The plugin connects to the ECU over a serial connection and reads engine data to calculate fuel consumption. As with the other backends, if the ECU is not available or not configured the plugin continues to work normally using adaptive estimation.

.. _adaptive-fuel-learning:

Adaptive fuel learning
~~~~~~~~~~~~~~~~~~~~~~

Even without a live ECU connection, the plugin builds up a fuel consumption model over time by recording consumption samples and trip summaries as you travel. This learned data is stored persistently and used to progressively refine fuel estimates for future trips. Fuel stops are also logged alongside rest stop history, giving a complete picture of stops made during a journey.

.. _history-and-persistence:

History and persistence
~~~~~~~~~~~~~~~~~~~~~~~

The plugin stores rest stop history, fuel stop history, fuel consumption samples, trip summaries, and all configuration persistently across sessions. History can be browsed in the GUI and older entries can be cleared. Configuration changes made through the menus are saved automatically and restored on the next start.

.. _ui-and-configuration:

User interface and configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The plugin registers as an OSD element in Navit's display. With the internal GUI, menu actions are available: suggest rest stop (along current route), rest stop history, start break, end break, configure intervals (per profile: car, truck, hiking, cycling), and configure overnight settings (minimum distance from buildings and glaciers, POI search radii). Session state — including driving time, whether a break is in progress, and whether a mandatory break is required — is tracked continuously. Vehicle type is selected via the OSD configuration.
