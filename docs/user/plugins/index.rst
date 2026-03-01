Plugins
=======

Navit includes several plugins that extend its functionality:

Driver Break Plugin
------------------

The Driver Break plugin provides configurable rest period management for multiple travel modes. It tracks activity time, suggests rest stops using OpenStreetMap data, discovers nearby Points of Interest (POIs), and supports route validation and energy-based routing where applicable.

**Travel modes**

- **Car** – Configurable soft and maximum driving hours, break interval (e.g. every 4–4.5 hours), and break duration (e.g. 15–45 minutes).
- **Truck** – Mandatory rest and driving time rules aligned with EU Regulation EC 561/2006 and related rules (e.g. break after 4.5 hours, 45-minute break, max daily driving, daily and weekly rest, and other driving/rest time limits).
- **Bicycle (cycling)** – Configurable rest intervals by distance (e.g. main/alternative break distances and max daily distance).
- **Hiking** – Distance-based rest intervals (main and alternative), max daily distance, and optional SRTM elevation and POI support (water, cabins).

**POIs searched**

The plugin discovers nearby Points of Interest depending on travel mode. Search radii are configurable (e.g. water 2 km, cabins 5 km, general POI 15 km, network huts 25 km). Searched POI types include:

- **Water** – Drinking water, fountain, spring (for hiking/cycling rest stops).
- **Cabins and huts** – Wilderness hut, alpine hut, hostel, camping; with optional DNT/network detection for prioritization.
- **Car** – Cafe, restaurant, museum, viewpoint, picnic, attraction (and similar amenities along driving routes).

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

**Networks and priorities**

- **DNT/network priority** – Optional priority for network huts (e.g. Norwegian Trekking Association, DNT) with configurable hut search radius.
- **Hiking/pilgrimage priority** – Optional preference for official hiking and pilgrimage routes when validating or suggesting stops.

**Hiking route validation**

For hiking routes, the plugin can validate that the route avoids forbidden road types (e.g. motorway, trunk, primary) and reports how much of the route uses priority paths (footway, path, track, steps, bridleway). With pilgrimage/hiking priority enabled, segments tagged as pilgrimage or hiking routes are counted as priority. Warnings are produced for high forbidden percentage or low priority path percentage.

**Energy-based routing (cycling)**

Optional energy-based routing uses a physical model (total weight, rolling and air resistance, recuperation on downhill) to compute segment cost, inspired by BRouter's kinematic model. Configurable via total weight and use_energy_routing.

**SRTM elevation**

For hiking, elevation data from SRTM HGT files can be used. The plugin supports listing available SRTM regions, downloading a region by name, and querying elevation at a coordinate. Elevation is used where applicable for routing and display.

**User interface and configuration**

The plugin registers as an OSD (type ``rest``). With the internal GUI, menu actions are available: suggest rest stop (along current route), rest stop history, start break, end break, configure intervals (per profile: car, truck, hiking, cycling), and configure overnight (min distance from buildings/glaciers, POI radii). Session state (driving time, break in progress, mandatory break required) is tracked. Configuration and rest stop history are stored in a SQLite database (path configurable via OSD ``data`` attribute; default in user data directory). Vehicle type can be set via OSD attribute ``type`` (e.g. car, truck).
