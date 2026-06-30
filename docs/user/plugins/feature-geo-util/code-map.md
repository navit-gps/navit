# Geo utility code map

Branch `feature-geo-util`. Cyclomatic complexity from `lizard -C 10` on
`driver_break_geo.c`.

## `driver_break_geo.c`

| Function | Lines | CCN |
|----------|-------|-----|
| `driver_break_coord_distance_geo` | 18--38 | 3 |
| `driver_break_route_collect_coord` | 40--52 | 5 |
| `driver_break_route_foreach_coord` | 54--91 | 9 |
| `driver_break_route_bbox` | 93--109 | 6 |

## Public API (`driver_break_geo.h`)

```c
double driver_break_coord_distance_geo(const struct coord_geo *c1,
                                       const struct coord_geo *c2);
int driver_break_route_bbox(struct route *route, double *min_lon, double *min_lat,
                            double *max_lon, double *max_lat);
```

## Dependency graph

```
driver_break_finder.c ----\
driver_break_glacier.c ----+--> driver_break_geo.h --> driver_break_geo.c
driver_break_poi.c --------+         |
driver_break_poi_hiking.c -+         +--> Navit core: coord, route, map, item,
driver_break_poi_map.c ----/              projection, transform

Future:
driver_break_srtm_osd.c --> driver_break_route_bbox (feature/driver-break)
driver_break_energy_route.c --> remove duplicate defs when merged
```

## Duplicate removal

Each updated caller dropped a file-local static Haversine helper
(`coord_distance`, `coord_distance_poi`, or `coord_distance_geo`) in favour of
the shared implementation. Behaviour is unchanged (same Earth radius and formula).
