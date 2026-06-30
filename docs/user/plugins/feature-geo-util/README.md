# Geo utility module (`feature-geo-util`)

This branch introduces shared geodesy helpers in `navit/plugin/driver_break_geo/`.
The module consolidates Haversine distance and route bounding-box logic that
previously lived in multiple Driver Break translation units.

This branch contains **only** the geo slice: `navit/plugin/driver_break_geo/` and
`tests/geo/`. There is no `navit/plugin/driver_break/` or `navit/plugin/driver_ecu/`
tree here; callers on other branches include `driver_break_geo.h` after merge.

## Functions

| Symbol | Purpose |
|--------|---------|
| `driver_break_coord_distance_geo` | Great-circle distance in meters between two WGS84 points |
| `driver_break_route_bbox` | Axis-aligned bounding box of all route polyline coordinates |

## Post-merge callers

After phase branches merge to trunk, these files are expected to include
`driver_break_geo.h` instead of local static helpers:

- `driver_break_finder.c`
- `driver_break_glacier.c`
- `driver_break_poi.c`
- `driver_break_poi_hiking.c`
- `driver_break_poi_map.c`

## Build and tests

The geo module builds as static library `driver_break_geo` (links `glib-2.0`; Navit
core symbols resolve when the library is linked into the plugin on trunk). Unit
tests live in `tests/geo/` and are enabled when `USE_PLUGINS` is on (Sailfish
harbour builds pass `-DUSE_PLUGINS=n` and skip this slice).

```bash
cmake -S . -B build -DBUILD_DRIVER_BREAK_GEO_TESTS=ON
cmake --build build --target driver_break_geo test_driver_break_geo -j$(nproc)
cd build && ctest -R driver_break_geo_test --output-on-failure
```

## Related documentation

- `configuration.md` -- no runtime configuration (pure library)
- `code-map.md` -- function inventory and dependency graph
- `complexity-report.txt` -- `lizard` output for `driver_break_geo.c`
- `tests.md` -- phase test plan and commands
- `test-results.txt` -- captured gate run output
