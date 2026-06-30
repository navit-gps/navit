# OSD configuration and commands

OSD behaviour is driven by Navit menu commands registered in
`driver_break_osd_core.c` and by `struct driver_break_config` fields shown in
the summary dialogs.

## Registered commands

| Command | Purpose |
|---------|---------|
| `driver_break_suggest_stop` | Show rest stop suggestions along the active route |
| `driver_break_show_history` | Show logged rest stop history |
| `driver_break_configure` | Placeholder for future global settings menu |
| `driver_break_start_break` / `driver_break_end_break` | Manual break timer and history logging |
| `driver_break_configure_intervals` | Profile-specific rest interval summary (car/truck/hiking/cycling) |
| `driver_break_configure_overnight` | Overnight search radii and building/glacier distances |
| `driver_break_configure_fuel` | Fuel and adapter summary dialog |
| `driver_break_set_fuel_level` | Set current fuel level (numeric argument) |
| `driver_break_log_fuel_stop` | Log a fuel stop (optional added amount argument) |

Legacy `rest_*` command aliases remain for backward compatibility.

## Dialog-exposed config fields

Vehicle interval dialogs display (read-only today):

- Car: `car_soft_limit_hours`, `car_max_hours`, `car_break_interval_hours`, `car_break_duration_min`
- Truck: `truck_mandatory_break_after_hours`, `truck_mandatory_break_duration_min`, `truck_max_daily_hours`
- Hiking: `hiking_driver_break_distance_main`, `hiking_driver_break_distance_alt`, `hiking_max_daily_distance`
- Cycling: `cycling_driver_break_distance_main`, `cycling_driver_break_distance_alt`, `cycling_max_daily_distance`

Overnight dialog: `min_distance_from_buildings`, `min_distance_from_glaciers` (hiking/pedestrian),
`poi_search_radius_km`, `poi_water_search_radius_km`, `poi_cabin_search_radius_km`.

Fuel dialog: `fuel_type`, tank capacity, consumption, warning thresholds, and ECU adapter flags.

Pressing OK in a dialog calls `driver_break_db_save_config` when a database is available.

## Requirements

Summary dialogs require Navit's internal GUI (`gui_internal`). Commands that do
not open dialogs (fuel level, break start/end) work without the internal GUI.
