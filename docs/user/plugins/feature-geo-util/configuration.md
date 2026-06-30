# Geo utility configuration

`driver_break_geo` exposes no configuration structures and no XML or database
keys. Both functions are pure helpers:

- Input coordinates are WGS84 degrees (`struct coord_geo`).
- `driver_break_coord_distance_geo` returns meters.
- `driver_break_route_bbox` writes min/max longitude and latitude in degrees and
  returns 1 when at least one route polyline point was found.

There are no plugin toggles or `ecu_config`-style fields for this module.
