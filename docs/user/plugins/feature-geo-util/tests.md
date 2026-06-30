# Tests (`feature-geo-util`)

Shared geodesy helpers in `driver_break_geo.c` are covered by a dedicated unit test.

## Executables

- `test_driver_break_geo` -- Haversine distance and route bbox null-route handling

## Build and run

From the repository root:

```bash
cmake -S . -B build -DBUILD_DRIVER_BREAK_GEO_TESTS=ON
cmake --build build --target test_driver_break_geo -j$(nproc)
cd build && ctest -R driver_break_geo_test --output-on-failure
```

Or run the binary directly:

```bash
./build/tests/geo/test_driver_break_geo
```

## Gate

Phase completion requires these tests to exit 0 and `lizard -C 10` to report no CCN violations
on phase sources (see `complexity-report.txt`).

## Related

- `test-results.txt` -- captured output from the gate run documented below
