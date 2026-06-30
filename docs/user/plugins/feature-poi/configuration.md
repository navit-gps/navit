# POI module configuration

POI behaviour is controlled through `struct driver_break_config` in
`driver_break.h`. There are no separate XML keys inside `driver_poi` itself.

## Search radii

| Field | Default (typical) | Used by |
|-------|-------------------|---------|
| `poi_search_radius_km` | 15 km | `driver_break_poi_discover`, finder, car/truck stops |
| `poi_water_search_radius_km` | 2 km | `poi_search_water_points*` |
| `poi_cabin_search_radius_km` | 5 km | `poi_search_cabins*` |
| `network_hut_search_radius_km` | 25 km | Cabin search when `enable_dnt_priority` is set |

Header constants `POI_WATER_SEARCH_RADIUS` and `POI_CABIN_SEARCH_RADIUS` in
`driver_break_poi_hiking.h` mirror the water/cabin defaults in meters.

## Priority toggles

| Field | Effect |
|-------|--------|
| `enable_dnt_priority` | Prefer DNT and other network huts; widens cabin search radius |
| `enable_hiking_pilgrimage_priority` | Route validator and map search favour pilgrimage/hiking routes |

## Glacier camping

| Field | Effect |
|-------|--------|
| `min_distance_from_glaciers` | Minimum distance (meters) from glaciers for nightly camping; `glacier_get_min_camping_distance()` returns 300 m |

## Fuel POI matching

Fuel station map search uses `vehicle_type` and `fuel_type` from config
(`enum driver_break_fuel_type`) to filter OSM fuel tags (petrol, diesel, LPG,
CNG, hydrogen, etc.).

## Overpass fallback

When map-based discovery returns no results and `HAVE_CURL` is defined,
`driver_break_poi_discover` queries the Overpass API with category strings such
as `amenity=cafe`. No separate Overpass URL or timeout is exposed in config;
network behaviour follows libcurl defaults in `driver_break_poi.c`.
