# Tests (`feature-ecu`)

ECU fuel backends are tested independently of `driver_break.c` in `tests/ecu/`.

## Executables

- `test_ecu_obd_j1939` -- OBD-II MAF fuel rate and J1939 PGN scaling
- `test_ecu_megasquirt` -- MegaSquirt parsing, formula, and malformed buffers

## Build and run

From the repository root:

```bash
cmake -S . -B build -DBUILD_DRIVER_ECU_TESTS=ON
cmake --build build --target test_ecu_obd_j1939 test_ecu_megasquirt -j$(nproc)
cd build && ctest -R ecu_ --output-on-failure
```

Or run binaries directly:

```bash
./build/tests/ecu/test_ecu_obd_j1939
./build/tests/ecu/test_ecu_megasquirt
```

## Gate

Phase completion requires these tests to exit 0 and `lizard -C 10` to report no CCN violations
on phase sources (see `complexity-report.txt`).

## Related

- `test-results.txt` -- captured output from the gate run documented below
