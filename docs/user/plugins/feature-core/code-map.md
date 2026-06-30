# Code map (`feature-core`)

## Branch layout

```
navit/plugin/driver_break_core/   # core slice + integration stubs
tests/core/                       # config and database unit tests
```

Out of scope on this branch: `navit/plugin/driver_break/`, `navit/plugin/driver_ecu/`,
POI/SRTM/OSD monolith sources (see `feature-poi`, `feature-srtm`, `feature-osd`,
`feature-ecu`).

## Module dependencies

```
driver_break.c
  -> driver_break_osd_lifecycle.h (OSD registration via plugin_init)
  -> driver_break_fuel_core.h
  -> driver_break_route_stops.h
  -> driver_break_hiking/cycling/finder (route handlers)

driver_break_osd_lifecycle.c
  -> driver_break_config.c (defaults)
  -> driver_break_db_* (open, load config)
  -> driver_break.c callbacks (vehicle, route, timeout)
  -> driver_break_slice_stubs.c (ECU, OSD commands, SRTM init)

driver_break_fuel_core.c
  -> driver_break_db_fuel.c (sample persistence)
  -> driver_break_slice_stubs.c (fuel station map search stub)

driver_break_route_stops.c
  -> driver_break_slice_stubs.c (water/cabin search stubs)

driver_break_finder.c
  -> driver_break_glacier.c
  -> driver_break_slice_stubs.c (POI discover stub)

driver_break_db_schema.c
  -> driver_break_db_config.c (clean_corrupted on open)
```

## Public entry points

- `plugin_init()` registers `driver_break_osd_new` with Navit.
- Database API unchanged in `driver_break_db.h`.
- Stubbed symbols match the full-plugin names so trunk merge replaces stubs with
  real implementations without renaming call sites.

## Tests

| Test | Sources linked |
|------|----------------|
| `test_driver_break_db` | Four `driver_break_db_*.c` files from `driver_break_core/` |
| `test_driver_break_config` | Header-only defaults test (no plugin link) |
| `plugin_driver_break` | Full core slice module build |
