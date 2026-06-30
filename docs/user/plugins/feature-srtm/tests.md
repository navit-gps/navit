# Tests (`feature-srtm`)

SRTM tile I/O, HGT parsing, and optional GeoTIFF download paths are validated by
the SRTM test executable in `tests/srtm/`.

## Executables

- `test_driver_break_srtm` -- HGT/GeoTIFF handling, tile borders, optional download tests

## Build and run

From the repository root:

```bash
cmake -S . -B build -DBUILD_DRIVER_BREAK_SRTM_TESTS=ON
cmake --build build --target test_driver_break_srtm -j$(nproc)
./build/tests/srtm/test_driver_break_srtm
```

Or use CTest:

```bash
cd build && ctest -R driver_break_srtm_test --output-on-failure
```

Optional test data download helper:

```bash
./tests/srtm/download_test_srtm_data.sh
```

## Gate

Phase completion requires these tests to exit 0 and `lizard -C 10` to report no CCN
violations on phase sources (see `complexity-report.txt`).

## Related

- `test-results.txt` -- captured output from the gate run documented below
- `../driver-break/tests.rst` -- full plugin test suite reference (post-merge)
