Plugins
=======

Navit includes several plugins that extend its functionality:

Driver Break Plugin
------------------

**Contents**

- :ref:`Travel modes <driver-break-travel-modes>`
- :ref:`POIs searched <driver-break-pois-searched>`
- :ref:`All searched POIs (reference) <driver-break-all-searched-pois>`
- :ref:`Distance from buildings (camping, allemannsretten) <driver-break-distance-from-buildings>`
- :ref:`Distance from glaciers (overnight) <driver-break-distance-from-glaciers>`
- :ref:`Configurable rest stop periods <driver-break-configurable-rest-stop-periods>`
- :ref:`Rast and vei (historical basis for hiking and cycling defaults) <driver-break-rast-and-vei>`
- :ref:`Networks and priorities <driver-break-networks-and-priorities>`
- :ref:`Hiking route validation <driver-break-hiking-route-validation>`
- :ref:`Energy-based routing (cycling,walking,cars,trucks) <driver-break-energy-based-routing>`
- :ref:`Fuel tracking and gas stations <driver-break-fuel-tracking-and-gas-stations>`
- :ref:`Supported vehicle fuel types <driver-break-supported-vehicle-fuel-types>`
- :ref:`Live fuel data <driver-break-live-fuel-data>`
- :ref:`SRTM elevation <driver-break-srtm-elevation>`
- :ref:`User interface and configuration <driver-break-user-interface-and-configuration>`
- :ref:`Testing <driver-break-testing>`

The Driver Break plugin provides configurable rest period management for multiple travel modes. It tracks activity time, suggests rest stops using OpenStreetMap data, discovers nearby Points of Interest (POIs), and supports route validation and energy-based routing where applicable.

.. _driver-break-travel-modes:

**Travel modes**

- **Car** – Configurable soft and maximum driving hours, break interval (e.g. every 4–4.5 hours), and break duration (e.g. 15–45 minutes).
- **Truck** – Mandatory rest and driving time rules aligned with EU Regulation EC 561/2006 and related rules (e.g. break after 4.5 hours, 45-minute break, max daily driving, daily and weekly rest, and other driving/rest time limits).
- **Hiking** – Daily segments: 40 km suggested maximum per day. Rest stops at 11.295 km intervals (main) or 2.275 km (alternative). Optional SRTM elevation and POI support (water, cabins).
- **Bicycle (cycling)** – Daily segments: 100 km suggested maximum per day. Rest stops at 28.24 km intervals (main) or 5.69 km (alternative). Suggested defaults use the same rast/vei concept as hiking, scaled up for cycling (see "Rast and vei" below).

.. _driver-break-pois-searched:

**POIs searched**

The plugin discovers nearby Points of Interest depending on travel mode. Search radii are configurable (e.g. water 2 km, cabins 5 km, general POI 15 km, network huts 25 km). Searched POI types include:

- **Water** – Drinking water, fountain, spring (for hiking/cycling rest stops).
- **Cabins and huts** – Wilderness hut, alpine hut, hostel, camping; with optional DNT/network detection for prioritization.
- **Car** – Cafe, restaurant, museum, viewpoint, zoo, picnic, attraction; convenience stores, general stores, farm shops, malls; places that offer bike repairs and bike parts (bicycle shop, repair service). See "All searched POIs" for exact tags.

.. _driver-break-all-searched-pois:

**All searched POIs (reference)**

Below are the exact POI tags and types the plugin uses (OpenStreetMap/Overpass API or map item types). Data may come from map tiles or, when configured, from Overpass.

- **Rest stop candidate locations** (where to suggest a stop along a route): ``highway=unclassified``, ``highway=service``, ``highway=track``, ``highway=driver_break_area``, ``highway=tertiary``.

- **Water** (hiking/cycling): ``amenity=drinking_water``, ``amenity=fountain``, ``natural=spring``. Map types: potable water, fountain.

- **Cabins and huts** (hiking/cycling, network detection): ``tourism=wilderness_hut``, ``tourism=alpine_hut``, ``tourism=hostel``, ``tourism=camp_site``. Map types: hostel, camping.

- **Car** (along driving routes): map types cafe, restaurant, museum (history), viewpoint, zoo, picnic, attraction; shop_grocery (convenience store), shopping (supermarket/general store), mall; shop_bicycle (bike shop, parts, repairs), repair_service. OSM: ``amenity=cafe``, ``amenity=restaurant``, ``tourism=museum``, ``tourism=viewpoint``, ``tourism=zoo``, picnic, attraction; ``shop=convenience`` (map: shop_grocery), ``shop=supermarket`` (map: shopping), ``shop=mall``, ``shop=bicycle``, ``amenity=bicycle_repair_station``, general repair (map: repair_service).

- **General POI fallback** (when enriching a rest stop with no specific categories): ``amenity=cafe``, ``amenity=restaurant``, ``tourism=museum``, ``tourism=viewpoint``; ``shop=convenience``, ``shop=farm``, ``shop=supermarket``, ``shop=mall``, ``shop=bicycle``, ``amenity=bicycle_repair_station``.

.. _driver-break-distance-from-buildings:

**Distance from buildings (camping, allemannsretten)**

For overnight or camping stops (e.g. when using right-to-roam rules such as Norwegian allemannsretten), the plugin enforces a configurable minimum distance from buildings or dwellings. Rest stop candidates too close to buildings are filtered out so suggested sites respect local camping rules (default e.g. 150 m; configurable).

.. _driver-break-distance-from-glaciers:

**Distance from glaciers (overnight)**

Nightly camping positions too close to glaciers are rejected. A configurable minimum distance (e.g. 300 m) applies; the check can be relaxed when the location has a camping building.

.. _driver-break-configurable-rest-stop-periods:

**Configurable rest stop periods**

Rest parameters are configurable per mode, including:

- Car: soft limit hours, max hours, break interval (hours), break duration (minutes).
- Truck: mandatory break after (hours), break duration (minutes), max daily driving hours.
- Hiking: main and alternative break distances (km), max daily distance (km).
- Cycling: main and alternative break distances (km), max daily distance (km).
- General: rest interval range (min/max hours), POI search radii, minimum distance from buildings (camping / allemannsretten), minimum distance from glaciers for overnight stops.

.. _driver-break-rast-and-vei:

**Rast and vei (historical basis for hiking and cycling defaults)**

The suggested default rest intervals for hiking (11.295 km main, 2.275 km alternative; 40 km daily max) and for cycling (28.24 km main, 5.69 km alternative; 100 km daily max) are inspired by the old Scandinavian units of length **rast** and **vei**.

For cycling, the same rast/vei concept is used with distances scaled up. A "rast" was the distance one traveled on foot before needing a rest ("rast," "pause," or the like); it corresponded to a **mil** and was often tied to the length of the ell. The distance varied by region and over time. In the 900s a rast was about 192 stone throws, divided into four quarters ("fjerdingvei"), and corresponded to roughly 9,100.8 meters; in the 12th century it was expressed as 16,000 ells (four quarters of 8,000 feet) but remained in the same order of magnitude.
A "dagsvei" (day's way/journey) was a traditional Scandinavian unit meaning roughly how far you could walk in a day, commonly reckoned at about 40 km.

It is humbling to note that what they considered a normal day's travel — 40 km —
is what many people today would consider a serious athletic achievement, even for
a fit, well-equipped modern person. Yet they were pragmatic about it: rest was
institutionalized or planned, built into the unit of distance itself.

The plugin's hiking and cycling interval defaults follow from these historical rast-based distances.

The scaling factor is 2.5×, reflecting that a cyclist travels roughly two to three times faster than a walker over a full day. A cyclist's dagsvei is thus 100 km (40 km × 2.5), with rest stops at 28.24 km (one rast × 2.5) and 5.69 km (one fjerdingvei × 2.5).

.. _driver-break-networks-and-priorities:

**Networks and priorities**

- **DNT/network priority** – Optional priority for network huts (e.g. Norwegian Trekking Association, DNT) with configurable hut search radius. Set the network hut search radius according to typical spacing (see below); in remote areas consider raising it toward the upper range to include the next cabin.

  **Networked cabin spacing (nearest-neighbor distances), for setting search radius:**

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

- **Hiking/pilgrimage priority** – Optional preference for official hiking and pilgrimage routes when validating or suggesting stops. The plugin treats segments tagged ``pilgrimage=yes`` or ``route=hiking`` as priority (e.g. Camino de Santiago and similar routes in OpenStreetMap). Facilities along pilgrimage routes (huts, hostels, water) are discovered with the same POI search (cabins, water); set the POI and cabin search radii in the same range as for national networks (e.g. 10–25 km) so stops along the route are found.

.. _driver-break-hiking-route-validation:

**Hiking route validation**

For hiking routes, the plugin can validate that the route avoids forbidden road types (e.g. motorway, trunk, primary) and reports how much of the route uses priority paths (footway, path, track, steps, bridleway). With pilgrimage/hiking priority enabled, segments tagged as pilgrimage or hiking routes are counted as priority. Warnings are produced for high forbidden percentage or low priority path percentage.

.. _driver-break-energy-based-routing:

**Energy-based routing (cycling,walking,cars,trucks)**

Optional energy-based routing uses a physical model (total weight, rolling and air resistance, recuperation on downhill) to compute segment cost, inspired by BRouter's kinematic model. Configurable via total weight and use_energy_routing.

.. _driver-break-fuel-tracking-and-gas-stations:

**Fuel tracking and gas stations**

The plugin stores a per-vehicle fuel profile in its SQLite database (fuel type, tank capacity,
average consumption, OBD-II/J1939 availability flags, manual ethanol % for flex-fuel). Users can set
the current fuel level and log fuel stops; from this and the configured average consumption, the plugin
computes an estimated remaining range. When an active route is present (car/truck) and remaining range
falls below the distance to destination plus a configurable buffer (and below a low-range threshold),
the plugin searches for nearby fuel stations (``amenity=fuel``) and surfaces suggestions alongside
rest stop information. Fuel stops are recorded in a dedicated history table, and additional tables
exist for adaptive fuel consumption learning (``driver_break_fuel_samples``) and trip summaries
(``driver_break_trip_summaries``). These tables are populated from manual fuel data and from the
built-in live backends (OBD-II, J1939). Additional backends (e.g. for aftermarket ECUs) can feed
the same fields to refine range estimates without schema changes.

**Supported vehicle fuel types**

The Driver Break plugin currently supports the following fuel types in its vehicle profile and POI
matching logic. These values are stored in ``driver_break_config.fuel_type`` and are used to filter
and rank ``amenity=fuel`` POIs based on their ``fuel:*`` tags:

- **Petrol / gasoline** – Regular and premium petrol grades, including:

  - EU-style octane tags: ``fuel:octane_95``, ``fuel:octane_98``, ``fuel:octane_100``.
  - North American tags: ``fuel:octane_87``, ``fuel:octane_89``, ``fuel:octane_91``, ``fuel:octane_93``.
  - Generic petrol tags: ``fuel:regular``, ``fuel:premium``, ``fuel:ethanol_free`` / ``fuel:e0``.

- **Diesel (cars and light commercial)** – Standard on-road diesel and common blends:

  - ``fuel:diesel``, ``fuel:diesel_b7``, ``fuel:diesel_b10``, ``fuel:biodiesel``,
    ``fuel:diesel:class2`` (cold-weather/Arctic diesel), ``fuel:taxfree_diesel`` and the overloaded
    ``fuel:b7`` tag which is treated as ``fuel:diesel_b7``.

- **Diesel (trucks / HGV)** – Heavy-vehicle diesel stations suitable for trucks:

  - All of the above diesel tags, plus ``fuel:HGV_diesel`` and (optionally) ``fuel:adblue`` for
    urea/DEF availability. These are preferred when the vehicle profile is set to truck.

- **Flex-fuel (E0–E100)** – Vehicles that can run on petrol/ethanol blends:

  - ``fuel:e5``, ``fuel:e10``, ``fuel:e15``, ``fuel:e20``, ``fuel:e85``, ``fuel:e100`` /
    ``fuel:ethanol``.
  - Flex-fuel profiles also accept petrol-only stations matching the petrol tags above, with a
    preference for stations that explicitly advertise E85 or E100.

- **Ethanol-only** – Vehicles configured for pure ethanol (E100):

  - ``fuel:e100``, ``fuel:ethanol`` and related tags.

- **Natural gas** – Compressed and liquefied natural gas:

  - **CNG**: ``fuel:cng``, ``fuel:GNV``, ``fuel:biogas``.
  - **LNG**: ``fuel:lng``.

- **LPG / autogas** – Liquefied petroleum gas:

  - ``fuel:lpg`` and the overloaded ``fuel:GPL`` tag.

- **Hydrogen** – Gaseous and liquid hydrogen for fuel cell vehicles:

  - ``fuel:h35`` (350 bar), ``fuel:h70`` (700 bar), ``fuel:LH2`` (liquid hydrogen).

Stations whose tags do not explicitly match the selected fuel type are generally filtered out; only
when no specific tags are present does the plugin fall back to treating generic ``amenity=fuel`` POIs
as candidates. This ensures that suggested fuel stops are appropriate for the configured vehicle.

.. _driver-break-live-fuel-data:

**Live fuel data**

The plugin supports three built-in live fuel backends. All write into the same internal state
(``driver_break_priv.fuel_rate_l_h``, ``fuel_current``, and optionally ethanol %); adaptive
consumption, range estimation, and trip summaries then work identically.

**Supported live fuel backends (implemented)**

- **OBD-II (cars, light vehicles)** – Implemented in the plugin. Uses an ELM327-compatible adapter
  on a serial port (default ``/dev/ttyUSB0``). Enable via config ``fuel_obd_available``.

  - Adapter init: ``ATZ``, ``ATE0``, ``ATL0``, ``ATH0``, ``ATS0``, ``ATSP0``.
  - PID discovery via ``0100``, ``0120``, ``0140`` (bitmasks).
  - Periodic polling: ``012F`` (fuel tank level %, PID 0x2F), ``015E`` (engine fuel rate L/h, PID
    0x5E), ``0110`` (MAF g/s, PID 0x10), ``0152`` (ethanol %, PID 0x52, optional).

  - If PID 0x5E is not supported, fuel rate is derived from MAF using:

    .. math::

       \text{Fuel rate (L/h)} = \frac{\text{MAF (g/s)} \times 3600}{\text{AFR} \times \text{density} \times 1000}

    (AFR and density depend on fuel type and ethanol %; e.g. petrol 14.7/0.745, diesel 14.5/0.832.)
  - Outputs: ``fuel_rate_l_h``, ``fuel_current`` (from tank % and configured capacity),
    ``config.fuel_ethanol_manual_pct`` when PID 0x52 is available.

- **J1939 (trucks, heavy vehicles)** – Implemented in the plugin. Uses SocketCAN (default interface
  ``can0``). Enable via config ``fuel_j1939_available``. Listens for PGN **65266** (FEEA) Engine
  fuel rate (SPN 183, 0.05 L/h per bit) and PGN **65276** (FEF4) Fuel level (SPN 96, 0.4 % per bit,
  0–250 %). Outputs: ``fuel_rate_l_h``, ``fuel_current`` (from fuel level % and tank capacity).

- **MegaSquirt (aftermarket ECU, experimental)** – Implemented in the plugin. Uses a direct serial
  connection to MegaSquirt family ECUs (MS1, MS2, MS3, MS3-Pro, MicroSquirt) on ``/dev/ttyUSB0`` at
  115200 8N1. Enable via config ``fuel_megasquirt_available`` and set
  ``fuel_injector_flow_cc_min`` (injector flow at rated pressure, cc/min). The backend sends the
  MegaSquirt realtime command (``A``) and converts injector pulse width and RPM into a fuel rate
  in L/h using the configured injector flow and a default cylinder count. When both OBD-II and
  MegaSquirt are configured, OBD-II takes precedence to avoid sharing the same serial adapter.

.. _driver-break-adaptive-range-and-high-load-detection:

**Adaptive range and high-load detection**

The live fuel rate from either backend is combined with GPS distance between vehicle callbacks to:
log per-segment samples into ``driver_break_fuel_samples``; maintain short-term and long-term
rolling consumption averages; use the short-term average for range estimation; detect high-load
when short-term exceeds baseline by ``fuel_high_load_threshold`` (user notified via OSD); and write
a trip summary to ``driver_break_trip_summaries`` at plugin shutdown.

.. _driver-break-adding-support-aftermarket-ecus:

**Adding support for other aftermarket ECUs**

Apart from the built-in MegaSquirt backend, the plugin does not include native drivers for other
aftermarket ECUs. You can add support by implementing a small backend that talks to the ECU
(serial, CAN, or via an OBD-II bridge) and writes the same fields the built-in backends use:

- **Required:** ``driver_break_priv.fuel_rate_l_h`` (L/h) and, if available,
  ``fuel_current`` (litres, from level % and tank capacity).
- **Optional:** ``config.fuel_ethanol_manual_pct`` (0–100) for flex-fuel.

No schema changes are needed; the existing adaptive logging, range logic, and trip summaries will
apply as soon as these fields are updated. Backends are started from the plugin init path
(see ``driver_break_obd_start`` / ``driver_break_j1939_start``) and must be mutually exclusive
with any use of the same hardware (e.g. one serial port for OBD-II or one CAN interface).

Candidates for such an integration (protocol documentation only; no code in this repo for these
brands):

- **Haltech** (Elite, Nexus): CAN broadcast – `Haltech <https://www.haltech.com/>`_ (protocol docs
  for product owners).
- **Link ECU** (G4+, G4X, G5): CAN stream – `Link ECU manuals
  <https://linkecu.com/software-support/manuals/>`_.
- **AEM** (Infinity, Series 2): AEMnet CAN – `AEM Electronics <https://www.aemelectronics.com/>`_.
- **ECUMaster** (EMU, EMU Black, EMU Pro): CAN output – `ECUMaster EMU
  <https://www.ecumaster.com/products/emu/>`_.

.. _driver-break-srtm-elevation:

**SRTM elevation**

The plugin uses **Copernicus DEM GLO-30** as the primary elevation source for download: GeoTIFF tiles from the public AWS S3 bucket (no authentication). URL pattern: ``https://copernicus-dem-30m.s3.amazonaws.com/Copernicus_DSM_COG_10_{LAT}_{LON}_DEM/Copernicus_DSM_COG_10_{LAT}_{LON}_DEM.tif``. When built with libtiff, the plugin downloads these tiles and reads elevation from them. If Copernicus download fails or libtiff is not available, the plugin falls back to **Viewfinder Panoramas** dem3: HGT 1 arc-second tiles in zone folders, ``http://www.viewfinderpanoramas.org/dem3/{ZONE}/{TILENAME}.zip`` (e.g. ``dem3/M32/N61E009.zip`` for Norway). The exact path for each tile is listed in ``viewfinderpanoramas.org/dem3list.txt``. Viewfinder blocks scripted downloads without a browser User-Agent; the plugin sends a browser-like User-Agent when downloading. A second fallback is NASA SRTMGL1. The plugin supports listing available regions, downloading a region by name (Copernicus GeoTIFF, then Viewfinder HGT zip, then NASA zip), and querying elevation at a coordinate. Elevation is used where applicable for routing and display. Tile borders are handled by per-point lookup: each coordinate is mapped to one 1x1 degree tile via the tile index (floor of longitude and latitude).

.. _driver-break-user-interface-and-configuration:

**User interface and configuration**

The plugin registers as an OSD (type ``rest``). With the internal GUI, menu actions are available: suggest rest stop (along current route), rest stop history, start break, end break, configure intervals (per profile: car, truck, hiking, cycling), and configure overnight (min distance from buildings/glaciers, POI radii). Session state (driving time, break in progress, mandatory break required) is tracked. Configuration and rest stop history are stored in a SQLite database (path configurable via OSD ``data`` attribute; default in user data directory). Vehicle type can be set via OSD attribute ``type`` (e.g. car, truck).

.. _driver-break-testing:

**Testing**

The Driver Break plugin has a dedicated test suite (configuration, database, finder, routing, SRTM/HGT and GeoTIFF download-and-read, integration). The route integration tests use **Rondanestien** (OpenStreetMap relation 1572954, Målia–Hjerkinn) as the hiking route so that POI discovery can find DNT huts, water, and other POIs along the trail. Additional hiking test routes (Mogen–Myrdal, Gjendesheim–Vetledalseter) are available as GPX in ``tests/route_gpx/``; waypoints are DNT huts with name and elevation for use with the height profile and energy-based routing. For test executables, how to run them, and expected results, see :doc:`tests`.
