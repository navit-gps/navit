# POI module code map

Branch `feature-poi`. Cyclomatic complexity from `lizard -C 10` on
`navit/plugin/driver_poi/`.

## Public API summary

### `driver_break_poi.h`

- `driver_break_poi_discover` — map search first, Overpass fallback
- `driver_break_poi_rank` — sort by distance from rest stop (returns sorted list head)
- `driver_break_poi_free_list` / `driver_break_poi_free`

### `driver_break_poi_map.h`

- `driver_break_poi_map_search` — generic item-type search
- `driver_break_poi_map_search_water` / `_cabins` / `_car_pois` / `_fuel`
- `driver_break_poi_is_network_cabin` — DNT/STF/etc. tag detection
- `driver_break_route_is_pilgrimage_hiking` — route tag helper

### `driver_break_poi_hiking.h`

- `poi_search_water_points` / `_map`
- `poi_search_cabins` / `_map`
- `struct water_point`, `struct cabin_hut` and free helpers

### `driver_break_glacier.h`

- `glacier_find_nearby`, `glacier_is_too_close_for_camping`
- `glacier_free_list`, `glacier_get_min_camping_distance`

## Highest-complexity functions (CCN at limit 10)

| Function | File | CCN |
|----------|------|-----|
| `driver_break_poi_map_search_cabins` | driver_break_poi_map.c | 10 |
| `fuel_station_matches_profile` | driver_break_poi_map.c | 10 |
| `driver_break_poi_is_network_cabin` | driver_break_poi_map.c | 9 |
| `parse_overpass_response` | driver_break_poi.c | 9 |
| `glacier_is_too_close_for_camping` | driver_break_glacier.c | 8 |
| `process_cabin_item` | driver_break_poi_map.c | 8 |

## Dependency graph

```
driver_break.c ------------\
driver_break_finder.c -----+--> driver_poi (static lib)
driver_break_route_validator.c
driver_break_osd.c (driver_break_poi.h only)

driver_poi/*
    +--> driver_break_geo.h (Phase 2)
    +--> driver_break.h (shared structs)
    +--> Navit core: mapset, item, coord, route
    +--> libcurl (optional, Overpass)
```

## Refactors in this branch

- Overpass tag parsing uses a dispatch table (`poi_category_parsers`) instead
  of nested amenity/tourism/leisure/shop conditionals.
- `parse_overpass_response` split into `overpass_find_node_object`,
  `overpass_next_element`, `overpass_try_append_node`.
- Glacier, hiking, and fuel map search split into smaller static helpers to
  keep cyclomatic complexity at or below 10.
