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

## Dependency graph (standalone branch)

```
tests/geo/test_driver_break_geo.c --> driver_break_geo.h --> driver_break_geo.c
                                                              |
                                                              +--> Navit core:
                                                                   coord, route, map,
                                                                   item, projection, transform
```

## Post-merge consumers (not on this branch)

```
driver_break_finder.c ----\
driver_break_glacier.c ----+--> driver_break_geo.h
driver_break_poi.c --------+
driver_break_poi_hiking.c -+
driver_break_poi_map.c ----/
```

Each updated caller drops a file-local static Haversine helper in favour of the
shared implementation. Behaviour is unchanged (same Earth radius and formula).
