Plugins
=======

Navit includes several plugins that extend its functionality:

Driver Break Plugin
------------------

The Driver Break plugin provides configurable rest period management for multiple travel modes. It tracks activity time, suggests rest stops using OpenStreetMap data, discovers nearby Points of Interest (POIs), and supports route validation and energy-based routing where applicable.

**Travel modes**

- **Car** – Configurable soft and maximum driving hours, break interval (e.g. every 4–4.5 hours), and break duration (e.g. 15–45 minutes).
- **Truck** – Mandatory rest and driving time rules aligned with EU Regulation EC 561/2006 and related rules (e.g. break after 4.5 hours, 45-minute break, max daily driving, daily and weekly rest, and other driving/rest time limits).
- **Hiking** – Daily segments: 40 km suggested maximum per day. Rest stops at 11.295 km intervals (main) or 2.275 km (alternative). Optional SRTM elevation and POI support (water, cabins).
- **Bicycle (cycling)** – Daily segments: 100 km suggested maximum per day. Rest stops at 28.24 km intervals (main) or 5.69 km (alternative). Suggested defaults use the same rast/vei concept as hiking, scaled up for cycling (see "Rast and vei" below).

**POIs searched**

The plugin discovers nearby Points of Interest depending on travel mode. Search radii are configurable (e.g. water 2 km, cabins 5 km, general POI 15 km, network huts 25 km). Searched POI types include:

- **Water** – Drinking water, fountain, spring (for hiking/cycling rest stops).
- **Cabins and huts** – Wilderness hut, alpine hut, hostel, camping; with optional DNT/network detection for prioritization.
- **Car** – Cafe, restaurant, museum, viewpoint, zoo, picnic, attraction; convenience stores, general stores, farm shops, malls; places that offer bike repairs and bike parts (bicycle shop, repair service). See "All searched POIs" for exact tags.

**All searched POIs (reference)**

Below are the exact POI tags and types the plugin uses (OpenStreetMap/Overpass API or map item types). Data may come from map tiles or, when configured, from Overpass.

- **Rest stop candidate locations** (where to suggest a stop along a route): ``highway=unclassified``, ``highway=service``, ``highway=track``, ``highway=driver_break_area``, ``highway=tertiary``.

- **Water** (hiking/cycling): ``amenity=drinking_water``, ``amenity=fountain``, ``natural=spring``. Map types: potable water, fountain.

- **Cabins and huts** (hiking/cycling, network detection): ``tourism=wilderness_hut``, ``tourism=alpine_hut``, ``tourism=hostel``, ``tourism=camp_site``. Map types: hostel, camping.

- **Car** (along driving routes): map types cafe, restaurant, museum (history), viewpoint, zoo, picnic, attraction; shop_grocery (convenience store), shopping (supermarket/general store), mall; shop_bicycle (bike shop, parts, repairs), repair_service. OSM: ``amenity=cafe``, ``amenity=restaurant``, ``tourism=museum``, ``tourism=viewpoint``, ``tourism=zoo``, picnic, attraction; ``shop=convenience`` (map: shop_grocery), ``shop=supermarket`` (map: shopping), ``shop=mall``, ``shop=bicycle``, ``amenity=bicycle_repair_station``, general repair (map: repair_service).

- **General POI fallback** (when enriching a rest stop with no specific categories): ``amenity=cafe``, ``amenity=restaurant``, ``tourism=museum``, ``tourism=viewpoint``; ``shop=convenience``, ``shop=farm``, ``shop=supermarket``, ``shop=mall``, ``shop=bicycle``, ``amenity=bicycle_repair_station``.

**Distance from buildings (camping, allemannsretten)**

For overnight or camping stops (e.g. when using right-to-roam rules such as Norwegian allemannsretten), the plugin enforces a configurable minimum distance from buildings or dwellings. Rest stop candidates too close to buildings are filtered out so suggested sites respect local camping rules (default e.g. 150 m; configurable).

**Distance from glaciers (overnight)**

Nightly camping positions too close to glaciers are rejected. A configurable minimum distance (e.g. 300 m) applies; the check can be relaxed when the location has a camping building.

**Configurable rest stop periods**

Rest parameters are configurable per mode, including:

- Car: soft limit hours, max hours, break interval (hours), break duration (minutes).
- Truck: mandatory break after (hours), break duration (minutes), max daily driving hours.
- Hiking: main and alternative break distances (km), max daily distance (km).
- Cycling: main and alternative break distances (km), max daily distance (km).
- General: rest interval range (min/max hours), POI search radii, minimum distance from buildings (camping / allemannsretten), minimum distance from glaciers for overnight stops.

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

**Hiking route validation**

For hiking routes, the plugin can validate that the route avoids forbidden road types (e.g. motorway, trunk, primary) and reports how much of the route uses priority paths (footway, path, track, steps, bridleway). With pilgrimage/hiking priority enabled, segments tagged as pilgrimage or hiking routes are counted as priority. Warnings are produced for high forbidden percentage or low priority path percentage.

**Energy-based routing (cycling)**

Optional energy-based routing uses a physical model (total weight, rolling and air resistance, recuperation on downhill) to compute segment cost, inspired by BRouter's kinematic model. Configurable via total weight and use_energy_routing.

**Fuel tracking and gas stations**

The plugin stores a basic per-vehicle fuel profile in its SQLite database (fuel type, tank capacity,
average consumption, OBD-II/J1939 availability flags, manual ethanol % for flex-fuel). Users can set
the current fuel level and log fuel stops; from this and the configured average consumption, the plugin
computes an estimated remaining range. When an active route is present (car/truck) and remaining range
falls below the distance to destination plus a configurable buffer (and below a low-range threshold),
the plugin searches for nearby fuel stations (``amenity=fuel``) and surfaces suggestions alongside
rest stop information. Future versions can use OBD-II (ELM327) and J1939 to fill in live fuel rate and
level, and adaptive logging will refine consumption estimates over time.

**SRTM elevation**

The plugin uses **Copernicus DEM GLO-30** as the primary elevation source for download: GeoTIFF tiles from the public AWS S3 bucket (no authentication). URL pattern: ``https://copernicus-dem-30m.s3.amazonaws.com/Copernicus_DSM_COG_10_{LAT}_{LON}_DEM/Copernicus_DSM_COG_10_{LAT}_{LON}_DEM.tif``. When built with libtiff, the plugin downloads these tiles and reads elevation from them. If Copernicus download fails or libtiff is not available, the plugin falls back to **Viewfinder Panoramas** dem3: HGT 1 arc-second tiles in zone folders, ``http://www.viewfinderpanoramas.org/dem3/{ZONE}/{TILENAME}.zip`` (e.g. ``dem3/M32/N61E009.zip`` for Norway). The exact path for each tile is listed in ``viewfinderpanoramas.org/dem3list.txt``. Viewfinder blocks scripted downloads without a browser User-Agent; the plugin sends a browser-like User-Agent when downloading. A second fallback is NASA SRTMGL1. The plugin supports listing available regions, downloading a region by name (Copernicus GeoTIFF, then Viewfinder HGT zip, then NASA zip), and querying elevation at a coordinate. Elevation is used where applicable for routing and display. Tile borders are handled by per-point lookup: each coordinate is mapped to one 1x1 degree tile via the tile index (floor of longitude and latitude).

**User interface and configuration**

The plugin registers as an OSD (type ``rest``). With the internal GUI, menu actions are available: suggest rest stop (along current route), rest stop history, start break, end break, configure intervals (per profile: car, truck, hiking, cycling), and configure overnight (min distance from buildings/glaciers, POI radii). Session state (driving time, break in progress, mandatory break required) is tracked. Configuration and rest stop history are stored in a SQLite database (path configurable via OSD ``data`` attribute; default in user data directory). Vehicle type can be set via OSD attribute ``type`` (e.g. car, truck).

**Testing**

The Driver Break plugin has a dedicated test suite (configuration, database, finder, routing, SRTM/HGT and GeoTIFF download-and-read, integration). The route integration tests use **Rondanestien** (OpenStreetMap relation 1572954, Målia–Hjerkinn) as the hiking route so that POI discovery can find DNT huts, water, and other POIs along the trail. Additional hiking test routes (Mogen–Myrdal, Gjendesheim–Vetledalseter) are available as GPX in ``tests/route_gpx/``; waypoints are DNT huts with name and elevation for use with the height profile and energy-based routing. For test executables, how to run them, and expected results, see :doc:`tests`.
