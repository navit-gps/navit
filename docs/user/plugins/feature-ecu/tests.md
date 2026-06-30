# Tests (`feature-ecu`)

ECU fuel backends are tested independently of `driver_break.c` in `tests/ecu/`.

## Executables

- `test_ecu_obd_j1939` — OBD-II MAF fuel rate and J1939 PGN scaling
- `test_ecu_megasquirt` — MegaSquirt parsing, formula, and malformed buffers

## Build and run

From a Navit build directory with the Driver Break plugin enabled:

```bash
cmake .. -Dplugin/driver_break=ON -DBUILD_DRIVER_BREAK_TESTS=ON
cmake --build . --target <executable> -j$(nproc)
./navit/plugin/driver_break/tests/<executable>   # or ./tests/ecu/<executable>
```

Or use CTest with the phase filter shown in `test-results.txt`.

## Gate

Phase completion requires these tests to exit 0 and \`lizard -C 10\` to report no CCN violations
on phase sources (see \`complexity-report.txt\`).

## Related

- \`test-results.txt\` — captured output from the gate run documented below
- \`../driver-break/tests.rst\` — full plugin test suite reference
