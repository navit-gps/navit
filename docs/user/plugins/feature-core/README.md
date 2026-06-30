# Core module split (`feature-core`)

This branch refactors the Driver Break plugin core on `origin/trunk`. The
monolithic `driver_break_db.c` and large sections of `driver_break.c` are split
into focused translation units under `navit/plugin/driver_break_core/`.

On this branch the slice builds standalone: ECU, OSD commands, SRTM, and POI
implementations are stubbed in `driver_break_slice_stubs.c` until the other
feature branches merge on trunk.

## Files

| File | Role |
|------|------|
| `driver_break_db_priv.h` | Shared DB struct, config key map, validation constants |
| `driver_break_db_schema.c` | Open/close database, table creation, corruption cleanup hook |
| `driver_break_db_history.c` | Stop history CRUD |
| `driver_break_db_config.c` | Config load/save and key validation |
| `driver_break_db_fuel.c` | Fuel stops, samples, trip summaries |
| `driver_break_config.c` | `driver_break_config_default()` |
| `driver_break_fuel_core.c` | Adaptive fuel learning and low-range station search |
| `driver_break_osd_lifecycle.c` | OSD constructor/destructor, DB path, callbacks, backend hooks |
| `driver_break_route_stops.c` | Hiking/cycling stop conversion and POI attachment |
| `driver_break_finder.c` | Rest stop finder along routes |
| `driver_break_glacier.c` | Glacier proximity checks |
| `driver_break_hiking.c` / `driver_break_cycling.c` | Profile stop calculators |
| `driver_break_route_validator.c` | Route validation helpers |
| `driver_break_energy.c` | Energy-based routing model |
| `driver_break_slice_stubs.c` | No-op ECU/OSD/SRTM/POI symbols for standalone build |
| `driver_break.c` | Route/vehicle callbacks, driving-time checks, `plugin_init()` |

Removed from this branch (live on other feature branches): `driver_break_osd.c`,
POI sources, SRTM sources, and `navit/plugin/driver_ecu/`.

## OSD lifecycle ownership

`driver_break_osd_lifecycle.c` owns plugin construction and teardown only
(`driver_break_osd_new`, destroy path, DB open, Navit callbacks, ECU backend
start/stop hooks, and `driver_break_ecu_sync_to_priv`). Menu commands and
profile dialogs remain on `feature-osd` after merge.

## Build

```bash
cmake -S . -B build -DUSE_PLUGINS=y
cmake --build build --target plugin_driver_break test_driver_break_db test_driver_break_config -j$(nproc)
ctest -R driver_break --output-on-failure
```

The plugin target is gated behind `USE_PLUGINS` (Sailfish harbour builds use
`-DUSE_PLUGINS=n` and skip the slice).

## Related documentation

- `configuration.md` — persisted config keys and defaults
- `code-map.md` — module dependency overview
- `complexity-report.txt` — `lizard -C 10` output for Phase 6 files
- `tests.md` — phase test plan and commands
- `test-results.txt` — captured gate run output
