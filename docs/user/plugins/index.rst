Plugins
=======

Navit includes several plugins that extend its functionality:

Driver Break Plugin
------------------

The Driver Break plugin provides configurable rest period management for multiple travel modes. It tracks activity time, suggests rest stops using OpenStreetMap data, discovers nearby Points of Interest (POIs), and supports route validation and energy-based routing where applicable.

**Travel modes**

- **Car** – Configurable soft and maximum driving hours, break interval (e.g. every 4–4.5 hours), and break duration (e.g. 15–45 minutes).
- **Truck** – Mandatory rest rules aligned with EU Regulation EC 561/2006 (e.g. break after 4.5 hours, 45 minutes, max daily driving).
- **Bicycle (cycling)** – Configurable rest intervals by distance (e.g. main/alternative break distances and max daily distance).
- **Hiking** – Distance-based rest intervals (main and alternative), max daily distance, and optional SRTM elevation and POI support (water, cabins).

**POIs searched**

The plugin discovers nearby Points of Interest depending on travel mode. Search radii are configurable (e.g. water 2 km, cabins 5 km, general POI 15 km, network huts 25 km). Searched POI types include:

- **Water** – Drinking water, fountain, spring (for hiking/cycling rest stops).
- **Cabins and huts** – Wilderness hut, alpine hut, hostel, camping; with optional DNT/network detection for prioritization.
- **Car** – Cafe, restaurant, museum, viewpoint, picnic, attraction (and similar amenities along driving routes).

**Distance from buildings (camping, allemannsretten)**

For overnight or camping stops (e.g. when using right-to-roam rules such as Norwegian allemannsretten), the plugin enforces a configurable minimum distance from buildings or dwellings. Rest stop candidates too close to buildings are filtered out so suggested sites respect local camping rules (default e.g. 150 m; configurable). A minimum distance from glaciers for nightly stops can also be set.

**Configurable rest stop periods**

Rest parameters are configurable per mode, including:

- Car: soft limit hours, max hours, break interval (hours), break duration (minutes).
- Truck: mandatory break after (hours), break duration (minutes), max daily driving hours.
- Hiking: main and alternative break distances (km), max daily distance (km).
- Cycling: main and alternative break distances (km), max daily distance (km).
- General: rest interval range (min/max hours), POI search radii, minimum distance from buildings (camping / allemannsretten) or glaciers for overnight stops.

**Networks and priorities**

- **DNT/network priority** – Optional priority for network huts (e.g. Norwegian Trekking Association, DNT) with configurable hut search radius.
- **Hiking/pilgrimage priority** – Optional preference for official hiking and pilgrimage routes when validating or suggesting stops.

The plugin can use OpenStreetMap POIs, optional SRTM elevation data for hiking, and stores rest stop history and configuration in a local database.
