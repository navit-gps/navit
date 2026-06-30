# ECU plugin code map

Generated for branch `feature-ecu`. Cyclomatic complexity from `lizard -C 10`.

## Function inventory

### `driver_ecu_obd.c`

| Function | Lines | CCN |
|----------|-------|-----|
| `obd_open_serial` | 42--81 | 4 |
| `obd_write_line` | 83--91 | 3 |
| `obd_read_line` | 93--110 | 7 |
| `obd_send_query` | 112--125 | 4 |
| `obd_parse_hex_bytes` | 127--143 | 6 |
| `obd_query_pid_mask` | 145--159 | 5 |
| `obd_detect_supported_pids` | 161--170 | 1 |
| `obd_pid_supported` | 172--179 | 3 |
| `obd_maf_to_fuel_rate` | 181--199 | 7 |
| `obd_poll_timeout` | 203--207 | 1 |
| `obd_read_fuel_rate_pid` | 209--229 | 6 |
| `obd_read_maf` | 231--250 | 6 |
| `obd_read_ethanol` | 252--273 | 8 |
| `obd_read_maf_and_ethanol` | 275--279 | 1 |
| `obd_update_tank_level` | 281--301 | 9 |
| `obd_poll` | 303--329 | 9 |
| `driver_ecu_obd_start` | 331--371 | 5 |
| `driver_ecu_obd_stop` | 373--383 | 4 |

### `driver_ecu_j1939.c`

| Function | Lines | CCN |
|----------|-------|-----|
| `j1939_get_pgn` | 34--38 | 1 |
| `j1939_handle_fuel_rate` | 40--52 | 5 |
| `j1939_handle_fuel_level` | 54--66 | 4 |
| `j1939_idle` | 68--89 | 8 |
| `driver_ecu_j1939_start` | 91--133 | 7 |
| `driver_ecu_j1939_stop` | 135--145 | 4 |

### `driver_ecu_megasquirt.c`

| Function | Lines | CCN |
|----------|-------|-----|
| `ms_open_serial` | 37--65 | 4 |
| `ms_write_line` | 67--75 | 2 |
| `ms_read_response` | 77--84 | 2 |
| `ms_read_realtime_block` | 86--92 | 2 |
| `ms_detect_version` | 94--109 | 4 |
| `ms_injector_to_fuel_rate` | 111--122 | 7 |
| `ms_poll_timeout` | 126--130 | 1 |
| `ms_apply_realtime_block` | 132--161 | 8 |
| `ms_poll` | 163--181 | 5 |
| `driver_ecu_megasquirt_start` | 183--212 | 6 |
| `driver_ecu_megasquirt_stop` | 214--229 | 4 |

## Dependency graph (standalone branch)

```
   driver_ecu_obd    driver_ecu_j1939    driver_ecu_megasquirt
          |               |               |
          +-------+-------+-------+-------+
                  v
            driver_ecu.h
         (ecu_config, ecu_fuel_state,
          start/stop API)

Outbound (ECU -> Navit core):
  callback.h, debug.h, event.h, config.h
  Linux: termios, fcntl, unistd (OBD, MegaSquirt)
  Linux: linux/can.h, socket, net/if (J1939)

Inbound (consumers of ECU on this branch):
  tests/ecu/  -- formula/logic tests (no driver_break.c)

Post-merge inbound (not on feature-ecu):
  driver_break.c  -- maps priv <-> ecu_config/ecu_fuel_state
```

## Public API (`driver_ecu.h`)

```c
struct ecu_config { ... };
struct ecu_fuel_state { ... };

struct driver_ecu_obd *driver_ecu_obd_start(const struct ecu_config *config,
                                            struct ecu_fuel_state *fuel);
void driver_ecu_obd_stop(struct driver_ecu_obd *obd);

struct driver_ecu_j1939 *driver_ecu_j1939_start(const struct ecu_config *config,
                                                 struct ecu_fuel_state *fuel);
void driver_ecu_j1939_stop(struct driver_ecu_j1939 *ctx);

struct driver_ecu_megasquirt *driver_ecu_megasquirt_start(const struct ecu_config *config,
                                                           struct ecu_fuel_state *fuel);
void driver_ecu_megasquirt_stop(struct driver_ecu_megasquirt *ctx);
```
