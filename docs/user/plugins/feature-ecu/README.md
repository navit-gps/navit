# ECU fuel monitoring plugin (`feature-ecu`)

The ECU plugin provides live fuel level and consumption data for the Driver Break
navigation plugin. It replaces the former in-plugin OBD-II, J1939, and MegaSquirt
backends with a decoupled module under `navit/plugin/driver_ecu/`.

## Supported hardware

| Backend | Transport | Default device | Data published |
|---------|-----------|----------------|----------------|
| OBD-II / ELM327 | USB serial (8N1, 115200) | `/dev/ttyUSB0` | Tank level (PID 0x2F), fuel rate (PID 0x5E or MAF fallback), ethanol % (PID 0x52) |
| J1939 | SocketCAN | `can0` | Engine fuel rate (PGN 65266 / FEEA), fuel level (PGN 65276 / FEF4) |
| MegaSquirt | USB serial (8N1, 115200) | `/dev/ttyUSB0` | Fuel rate from RPM and injector pulse width |

OBD-II and MegaSquirt share the same serial port; Driver Break starts OBD-II first
and falls back to MegaSquirt when OBD-II is disabled or fails to initialize. J1939
runs independently on CAN.

## Integration with Driver Break core

Backends write into `struct ecu_fuel_state` (`fuel_current`, `fuel_rate_l_h`).
Driver Break maps configuration through `struct ecu_config` and copies fuel state
back into `driver_break_priv` on vehicle position updates and the periodic
check timeout.

When no backend is active, manual fuel values and adaptive learning continue
unchanged.

## Build and tests

The ECU sources build as static library `driver_ecu`, linked into
`plugin_driver_break`. Standalone unit tests live in `tests/ecu/` and do not
link `driver_break.c`.

```bash
cmake --build <builddir> --target test_ecu_obd_j1939 test_ecu_megasquirt
ctest -R ecu_ --output-on-failure
```

## Related documentation

- `configuration.md` -- `ecu_config` fields and XML/database mapping
- `code-map.md` -- function inventory and dependency graph
- `complexity-report.txt` -- full `lizard` output (CCN gate: all functions <= 10)
- `tests.md` — phase test plan and commands
- `test-results.txt` — captured gate run output
