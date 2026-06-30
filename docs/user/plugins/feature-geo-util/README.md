# Geo utility module (`feature-geo-util`)

This branch introduces `driver_break_geo.{c,h}`, a shared geodesy helper module
for the Driver Break plugin. It consolidates duplicated Haversine distance and
route bounding-box logic that previously lived in multiple translation units (and
on other branches in `driver_break_energy_route.c`).

## Functions

| Symbol | Purpose |
|--------|---------|
| `driver_break_coord_distance_geo` | Great-circle distance in meters between two WGS84 points |
| `driver_break_route_bbox` | Axis-aligned bounding box of all route polyline coordinates |

## Callers updated

The following files now include `driver_break_geo.h` and call the shared
symbols instead of local static helpers:

- `driver_break_finder.c`
- `driver_break_glacier.c`
- `driver_break_poi.c`
- `driver_break_poi_hiking.c`
- `driver_break_poi_map.c`

`driver_break_srtm_osd.c` on trunk does not yet call `driver_break_route_bbox`
(the route-tile download command exists on `feature/driver-break`). When that
command is merged, `srtm_osd` should include `driver_break_geo.h` for bbox
computation.

## Build

`driver_break_geo.c` is compiled into `plugin_driver_break` via
`navit/plugin/driver_break/CMakeLists.txt`.

## Related documentation

- `configuration.md` — no runtime configuration (pure library)
- `code-map.md` — function inventory and dependency graph
- `complexity-report.txt` — `lizard` output for `driver_break_geo.c`
- `tests.md` — phase test plan and commands
- `test-results.txt` — captured gate run output
