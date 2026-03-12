How the Driver Break plugin works
=================================

This document describes how the Driver Break plugin implements its main
behaviours: how it uses the current route to suggest rest stops, how it
calculates energy-based (eco) routing costs, how it uses SRTM elevation data,
and how it discovers Points of Interest (POIs). It is intended for users and
developers who want to understand the plugin's internals without reading the
source code.


Overview
--------

The plugin registers with Navit as an OSD and subscribes to the current route
and vehicle position. Depending on the selected vehicle type (car, truck,
hiking, cycling), it:

- **Suggests rest stops** along the route at configurable intervals or when a
  mandatory break is required.
- **Enriches each stop** with nearby POIs (water, cabins, cafes, fuel stations,
  etc.) and optionally with elevation from SRTM.
- **Optionally influences routing** via an energy-based cost model (eco-mode)
  when enabled.

All of this uses Navit's existing route and map infrastructure; the plugin
does not replace Navit's router but adds rest-stop logic and optional segment
costs on top of it.


How rest stops are calculated
-----------------------------

**Hiking and cycling (distance-based)**

- The plugin gets the route's total length from Navit (``attr_destination_length``).
- For **hiking**, it divides the route into segments using the configured main
  and alternative break distances (defaults based on rast/vei, e.g. 11.295 km
  main, 2.275 km alternative) and a maximum daily distance (e.g. 40 km). When
  DNT priority is enabled, the max daily distance can be increased (e.g. 46.4 km)
  to match typical hut spacing.
- For **cycling**, the same idea applies with cycling defaults (e.g. 28.24 km
  main, 5.69 km alternative, 100 km daily max).
- Rest stop positions are placed at these distance intervals along the route.
  For each such position, the plugin finds the nearest point on the route and
  discovers POIs around that point (see "How POIs are found" below).

**Car and truck (along-route search)**

- When a mandatory break is required (or when suggesting stops), the plugin
  does not precompute fixed intervals. Instead it walks the **route map**
  (``route_get_map(route)``) segment by segment.
- It iterates over items of type ``type_street_route``, sums segment lengths,
  and after a minimum accumulated distance (e.g. 5 km) looks for segments that
  are on suitable highway types (e.g. unclassified, service, track,
  driver_break_area, tertiary).
- For each candidate segment end point, it converts the coordinate to
  latitude/longitude and runs **location validation** (distance from buildings,
  distance from glaciers if applicable). If valid, it creates a rest stop there
  and discovers POIs around it.
- This yields rest stops that lie on the driven route at sensible road types,
  rather than at arbitrary map points.

So: **hiking/cycling** use distance-based intervals along the route; **car/truck**
use a walk over the route geometry with highway and location checks.


How energy-based (eco) routing works
------------------------------------

When ``use_energy_routing`` is enabled, the plugin uses a **kinematic
energy model** (inspired by BRouter) to assign a cost to each route segment.
Navit's router can then prefer routes with lower total cost (i.e. lower
predicted energy use).

- The model is parameterised by **total weight** (vehicle + cargo + person),
  **rolling resistance**, **air resistance**, **standby power**, and
  **recuperation efficiency** for downhill.
- For each segment the plugin (or the routing path) has: **distance**,
  **height difference** (delta_h), **elevation** (for air density correction),
  and **speed limit**.
- The cost is computed from:
  - Rolling force (constant per metre).
  - Air resistance (proportional to speed squared; adjusted for temperature
    and elevation).
  - Gravitational work (delta_h * weight * g / distance).
  - Recuperation on downhill (negative gradient reduces effective energy).
- The result is a single **cost** value per segment. The router uses the sum
  of these costs over the path. So flatter or slightly longer routes can be
  preferred when they reduce total energy.

Elevation for this model comes from SRTM (or similar) when available; see
"How SRTM is used" below. The mathematical details are in :doc:`formulas`.


How SRTM files are used
-----------------------

The plugin uses elevation data only when the SRTM subsystem is initialised
(with a data directory, e.g. the default ``~/.navit/srtm/`` or a user-chosen
path).

**Tile layout**

- Elevation is stored in **1 degree x 1 degree tiles** (e.g. one tile per
  latitude/longitude cell).
- Each coordinate (lat, lon) is mapped to a tile index: ``lon_idx = floor(lon)``,
  ``lat_idx = floor(lat)``. So there is no overlap; each point belongs to
  exactly one tile.

**Data sources (in order of use)**

1. **Copernicus DEM GLO-30** – GeoTIFF tiles from the public AWS S3 bucket.
   The plugin looks for a file named according to the tile (e.g. for the
   cell containing (61.5, 9.7) it will look for the Copernicus tile for that
   cell). When built with libtiff, it reads elevation from the GeoTIFF (Int16
   or Float32 samples).
2. **Viewfinder Panoramas dem3** – HGT format: 1 arc-second resolution,
   3601x3601 pixels per tile, 16-bit big-endian. Filenames follow the pattern
   N/S and E/W (e.g. N61E009.hgt). If the Copernicus tile is missing or
   unreadable, the plugin falls back to HGT in the same directory (or after
   downloading via the SRTM download commands).
3. **NASA SRTMGL1** – Used as a second fallback for download; same 1x1 degree
   concept.

**Reading elevation for a point**

- ``srtm_get_elevation(coord)`` computes ``lon_idx`` and ``lat_idx`` from
  the coordinate, checks whether that tile exists (file present in the data
  directory), then:
  - If a GeoTIFF for that tile exists, it reads the pixel at the coordinate
    (interpolated to tile bounds) and returns the elevation value.
  - Otherwise it opens the HGT file, computes the pixel offset (from the
    coordinate within the 1x1 degree cell), reads the 16-bit value, and
    returns it.
- **Tile borders**: The route or path may cross from one tile to another. The
  plugin does not merge tiles; each call to ``srtm_get_elevation`` uses only
  one tile (the one containing the point). So at the boundary, the next
  segment will use the adjacent tile. There is no special smoothing at the
  border; the elevation is that of the tile at that point.

**Void / missing data**

- SRTM uses -32768 as the "void" value (no data). The plugin returns this
  value when the tile is missing, the file cannot be read, or the sample is
  void. Callers (e.g. energy model or display) should treat -32768 as
  "elevation unknown".


How POIs are found
------------------

The plugin discovers POIs in two ways: **map-based search** (preferred) and
**Overpass API** (fallback when no suitable map is available).

**Map-based search**

- When Navit has a **mapset** (e.g. loaded OSM or binary map), the plugin
  queries the map layers for items near a given coordinate and within a
  **search radius** (configurable, e.g. 2 km for water, 5 km for cabins,
  15 km for general POIs).
- It builds a **map selection** (rectangle or radius around the center),
  then iterates over maps in the mapset and over items in that area.
- POI **types** are matched to Navit item types (e.g. cafe, restaurant,
  museum, shop types, amenity types). For **water**, it looks for drinking
  water and fountains; for **cabins/huts**, it looks for wilderness hut,
  alpine hut, hostel, camping; for **car** it looks for cafes, restaurants,
  museums, viewpoints, shops, bicycle repair; for **fuel** it looks for
  fuel stations and filters by the vehicle's fuel type (petrol, diesel, etc.)
  using OSM ``fuel:*`` tags when available.
- **DNT/network huts**: When network priority is enabled, the plugin checks
  for operator/network tags (e.g. DNT, STF) and can rank or prioritise
  network cabins within the cabin list.
- **Distance from buildings / glaciers**: Rest stop *candidates* are
  validated so that the chosen location is at least a configured distance
  from buildings (e.g. 150 m for camping/allemannsretten) and from glaciers
  (e.g. 300 m for overnight). This is done by querying the map for landuse
  and building items around the candidate point; if too close, the candidate
  is rejected.

**Overpass API fallback**

- When map data is not available (or for "find near" without a route), the
  plugin can call the **Overpass API** with a bounding box and tags (e.g.
  ``amenity=cafe``, ``highway=service``). It then parses the response and
  builds a list of POIs with coordinates and names. This is used for
  rest-stop candidate search (highway types) and for general POI discovery
  when the mapset is empty or not used.

**Along route vs near a point**

- **Along route**: For car/truck, rest stops are found by walking the route
  map and, at each valid segment end, calling POI discovery with that
  coordinate and the configured radius. So POIs are "around" each suggested
  stop.
- **Near a point**: For "find rest stops near me" or for enriching a
  precomputed hiking/cycling stop, the plugin calls POI discovery once per
  stop with the stop's coordinate and the appropriate radius (water, cabin,
  or general POI radius).

**Ranking**

- POIs can be ranked by distance from the rest stop and by profile (e.g.
  fuel type match, network cabin priority). The exact ranking is implemented
  in the POI and finder modules; the result is a list of POIs attached to
  each suggested rest stop for display in the UI.


Summary
-------

- **Rest stops**: Hiking/cycling use distance-based intervals; car/truck walk
  the route map and pick valid highway segments with location checks.
- **Energy routing**: A kinematic model (weight, rolling/air resistance,
  gradient, recuperation) yields a cost per segment so the router can prefer
  energy-efficient paths.
- **SRTM**: 1x1 degree tiles (Copernicus GeoTIFF first, then HGT, then NASA);
  one elevation value per coordinate from the tile containing that point;
  -32768 means no data.
- **POIs**: Prefer map-based search (mapset + item types + radius); fallback
  to Overpass; validate rest stop locations for distance from buildings and
  glaciers; attach POI lists to each suggested stop.

For formulas and configuration details, see :doc:`formulas`, :doc:`ecu_ports`,
and the main :doc:`index`.
