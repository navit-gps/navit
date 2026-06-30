# Configuration (`feature-core`)

## Defaults

`driver_break_config_default()` in `driver_break_config.c` initializes all
fields of `struct driver_break_config`. Values match the previous monolithic
implementation (car/truck EU limits, hiking/cycling distances, POI radii,
fuel profile defaults).

## Persistence

Configuration is stored in SQLite table `driver_break_config` (key/value text
pairs). Loading is handled by `driver_break_db_load_config()` in
`driver_break_db_config.c`:

- Corrupted rows (negative, empty, or out of range) are deleted before load.
- Unknown keys are logged and skipped.
- Invalid numeric values for known keys keep the current default.

Saving uses `driver_break_db_save_config()` with a fixed key list covering
vehicle type, car/truck limits, POI radii, rest intervals, and fuel settings.

## Database path

The OSD constructor resolves the database path from the `data` attribute on the
plugin XML tag, expanding environment variables via `file_wordexp`. If unset,
the default is `$NAVIT_USER_DATA/driver_break_plugin.db`.

## Fuel learning runtime state

Adaptive fuel learning state (rolling windows, trip totals, high-load flag) lives
in `struct driver_break_priv` and is not persisted except via fuel sample rows
and optional trip summary on plugin shutdown.
