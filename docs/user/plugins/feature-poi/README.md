# POI module (`feature-poi`)

This branch extracts point-of-interest (POI) discovery into static libraries under
`navit/plugin/driver_poi/` and `navit/plugin/driver_break_geo/` (geo dependency
from Phase 2).

This branch contains **only** the POI slice plus geo helpers and `tests/poi/`.
There is no `navit/plugin/driver_break/` or `navit/plugin/driver_ecu/` tree here;
integration happens after merge to trunk.

## Modules

| File pair | Role |
|-----------|------|
| `driver_break_poi.{c,h}` | High-level discover/rank/free API; Overpass JSON fallback when map data is unavailable |
| `driver_break_poi_map.{c,h}` | Mapset queries for water, cabins, car POIs, fuel stations, and generic item types |
| `driver_break_poi_hiking.{c,h}` | Hiking/cycling water and cabin search wrappers with typed result structs |
| `driver_break_glacier.{c,h}` | Glacier proximity checks for nightly camping safety |
| `driver_break_types.h` | Shared POI/config structs (full `driver_break.h` lives on trunk after merge) |

## Build and tests

`driver_poi` links `driver_break_geo` and `glib-2.0`. The slice is built when
`USE_PLUGINS` is on (Sailfish harbour builds pass `-DUSE_PLUGINS=n` and skip it).

```bash
cmake -S . -B build -DBUILD_DRIVER_BREAK_POI_TESTS=ON
cmake --build build --target driver_poi test_driver_break_poi -j$(nproc)
cd build && ctest -R driver_break_poi_test --output-on-failure
```

When `libcurl` is found, Overpass HTTP queries are enabled via `HAVE_CURL`.

## Dependencies

- `driver_break_geo` for Haversine distance
- Navit core: `mapset`, `item`, `coord`, `route` (via headers at compile time)
- Optional: libcurl for Overpass API

## Related documentation

- `configuration.md` -- plugin config fields that affect POI search
- `code-map.md` -- function inventory and dependency graph
- `complexity-report.txt` -- `lizard -C 10` output for `driver_poi/`
- `tests.md` -- phase test plan and commands
- `test-results.txt` -- captured gate run output
