# ECU configuration (`ecu_config`)

All fields are populated by Driver Break from `struct driver_break_config` before
backends start. Values persist in the `driver_break_config` SQLite table unless
noted as runtime-only.

## Field reference

| Field | Type | Units / range | XML / DB key | Used by |
|-------|------|---------------|--------------|---------|
| `fuel_type` | int | `0` petrol, `1` diesel, `2` flex, `3` CNG, `4` LNG, `5` LPG, `6` hydrogen, `7` ethanol | `fuel_type` | OBD MAF fallback AFR/density |
| `fuel_tank_capacity_l` | int | Liters (or equivalent for gas fuels); must be > 0 for tank-level conversion | `fuel_tank_capacity_l` | OBD PID 0x2F, J1939 PGN FEF4 |
| `fuel_ethanol_manual_pct` | int | 0--100; runtime update when OBD PID 0x52 present | `fuel_ethanol_manual_pct` | OBD (read/write) |
| `fuel_obd_available` | int | 0 or 1 | `fuel_obd_available` | OBD backend enable |
| `fuel_j1939_available` | int | 0 or 1 | `fuel_j1939_available` | J1939 backend enable |
| `fuel_megasquirt_available` | int | 0 or 1 | `fuel_megasquirt_available` | MegaSquirt backend enable |
| `fuel_injector_flow_cc_min` | int | cc/min at rated pressure; must be > 0 for MegaSquirt | `fuel_injector_flow_cc_min` | MegaSquirt fuel rate formula |

## Runtime fuel state (`ecu_fuel_state`)

| Field | Type | Units | Direction |
|-------|------|-------|-----------|
| `fuel_current` | double | Liters (same unit as tank capacity) | ECU writes, core reads |
| `fuel_rate_l_h` | double | Liters per hour | ECU writes, core reads |

Driver Break copies both fields into `driver_break_priv` on each vehicle
callback and on the one-minute check timeout.

## Enable flags in practice

Set exactly one serial backend when sharing `/dev/ttyUSB0`:

1. `fuel_obd_available = 1` for ELM327/OBD-II adapters.
2. If OBD-II is off or fails, set `fuel_megasquirt_available = 1` and configure
   `fuel_injector_flow_cc_min`.
3. For trucks with SocketCAN, set `fuel_j1939_available = 1` independently.

Tank level from OBD or J1939 requires a valid `fuel_tank_capacity_l`.
