# OSD module split (`feature-osd`)

This branch reorganises the Driver Break on-screen display (OSD) layer into
`navit/plugin/driver_break_osd/`. The monolithic `driver_break_osd.c` is split
into focused translation units plus the shared GUI shim.

This branch contains **only** the OSD slice: split OSD sources, `driver_break_db`
(for config/history persistence used by dialogs), finder stubs for standalone
build, and `tests/osd/`. There is no full `navit/plugin/driver_break/` tree;
integration happens after merge to trunk.

## Files

| File | Role |
|------|------|
| `driver_break_osd_common.c` | Plugin instance, GUI priv lookup, shared dialog helpers |
| `driver_break_osd_core.c` | Command registration, suggestions/history, break start/end |
| `driver_break_osd_fuel.c` | Fuel configuration dialog and fuel commands |
| `driver_break_osd_car.c` | Car rest interval summary dialog |
| `driver_break_osd_truck.c` | Truck rest interval summary dialog |
| `driver_break_osd_motorcycle.c` | Motorcycle rest interval summary and terrain toggle |
| `driver_break_osd_hiking.c` | Hiking rest interval summary dialog |
| `driver_break_osd_cycling.c` | Cycling rest interval summary dialog |
| `driver_break_osd_overnight.c` | Overnight stop settings dialog |
| `driver_break_osd_priv.h` | Internal declarations shared between OSD units |
| `driver_break_gui_shim.{c,h}` | Runtime `dlsym` resolution of `gui_internal` |
| `driver_break_db.{c,h}` | SQLite persistence for config/history/fuel |
| `driver_break_finder_stub.c` | Standalone stubs for finder APIs referenced by OSD core |

## Build and tests

Static library `driver_break_osd` is built when `USE_PLUGINS` is on. Phase gate
tests in `tests/osd/` cover configuration defaults and database persistence.

```bash
cmake -S . -B build -DBUILD_DRIVER_BREAK_OSD_TESTS=ON
cmake --build build --target driver_break_osd test_driver_break_config test_driver_break_db -j$(nproc)
cd build && ctest -R 'driver_break_(config|db)_test' --output-on-failure
```

Sailfish harbour builds pass `-DUSE_PLUGINS=n` and skip this slice.

## Related documentation

- `configuration.md` -- menu commands and config fields surfaced in dialogs
- `code-map.md` -- function inventory and dependency graph
- `complexity-report.txt` -- `lizard -C 10` output
- `tests.md` -- phase test plan and commands
- `test-results.txt` -- captured gate run output
