# OSD code map

Branch `feature-osd`. Cyclomatic complexity from `lizard -C 10` on all
`driver_break_osd_*.c` and `driver_break_gui_shim.c`.

## Translation units

| File | Lines | Functions (approx.) |
|------|-------|---------------------|
| `driver_break_osd_common.c` | 202 | Plugin lookup, dialog shell helpers |
| `driver_break_osd_core.c` | 466 | Command table, suggestions/history/break/intervals |
| `driver_break_osd_fuel.c` | 206 | Fuel dialog and fuel commands |
| `driver_break_osd_car.c` | 38 | Car intervals dialog |
| `driver_break_osd_truck.c` | 22 | Truck intervals dialog |
| `driver_break_osd_hiking.c` | 21 | Hiking intervals dialog |
| `driver_break_osd_cycling.c` | 21 | Cycling intervals dialog |
| `driver_break_osd_overnight.c` | 113 | Overnight dialog and command |
| `driver_break_gui_shim.c` | 101 | `dlsym` resolution |

## Public API (`driver_break_osd.h`)

Command handlers and `driver_break_register_commands` — unchanged signatures for
Navit menu integration.

## Internal API (`driver_break_osd_priv.h`)

- `driver_break_osd_get_plugin`, `driver_break_osd_get_internal_gui_priv`
- `driver_break_osd_require_plugin`, `driver_break_osd_require_gui`
- `osd_dialog_begin`, `osd_dialog_end`, `osd_dialog_end_with_save`
- `osd_append_label`, `osd_append_label_printf`, `osd_append_validated_int_label`
- Vehicle-specific `driver_break_show_*_dialog` functions

## Dependency graph

```
driver_break.c (osd_new sets driver_break_plugin_instance)
        |
        v
driver_break_osd_core.c --> driver_break_osd_priv.h
        |                          |
        +--> driver_break_osd_fuel.c
        +--> driver_break_osd_*_intervals.c
        +--> driver_break_osd_overnight.c
        |
        v
driver_break_osd_common.c --> driver_break_gui_shim.c --> libgui_internal.so (runtime)
        |
        +--> driver_break_db.c (save config / history)
        +--> driver_break_finder.c (route stop suggestions)
```

## Refactors in this branch

- Removed duplicated inline `dlsym` block from OSD; reuse Phase 3 GUI shim.
- Shared dialog shell helpers reduce repeated `gui_internal_enter`/render/leave code.
- Interval profile dispatch consolidated in `driver_break_osd_show_intervals_for_profile`.
- Fuel stop logging split into parse/apply/fill helpers for CCN compliance.
- Fixed unbalanced `#ifdef HAVE_CURL` in `driver_break_poi.c` (trunk pre-existing).
