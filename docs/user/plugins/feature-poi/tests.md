# Tests (`feature-poi`)

POI discovery via `driver_poi` is exercised by the standalone unit test in
`tests/poi/`.

## Executables

- `test_driver_break_poi` -- POI ranking, null-input guards, glacier constants

## Build and run

From the repository root:

```bash
cmake -S . -B build -DBUILD_DRIVER_BREAK_POI_TESTS=ON
cmake --build build --target test_driver_break_poi -j$(nproc)
./build/tests/poi/test_driver_break_poi
```

Or use CTest:

```bash
cd build && ctest -R driver_break_poi_test --output-on-failure
```

## Gate

Phase completion requires these tests to exit 0 and `lizard -C 10` to report no CCN
violations on phase sources (see `complexity-report.txt`).

## Related

- `test-results.txt` -- captured output from the gate run documented below
- `../driver-break/tests.rst` -- full plugin test suite reference (post-merge)
