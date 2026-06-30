# Tests (`feature-osd`)

OSD modules compile into static library `driver_break_osd`. Config and database
paths used at OSD init are smoke-tested independently.

## Executables

- `test_driver_break_config` -- Configuration defaults used by OSD constructor
- `test_driver_break_db` -- SQLite persistence used when OSD loads saved config

## Build and run

From the repository root:

```bash
cmake -S . -B build -DBUILD_DRIVER_BREAK_OSD_TESTS=ON
cmake --build build --target test_driver_break_config test_driver_break_db -j$(nproc)
cd build && ctest -R 'driver_break_(config|db)_test' --output-on-failure
```

## Gate

Phase completion requires these tests to exit 0 and `lizard -C 10` to report no CCN
violations on phase sources (see `complexity-report.txt`).

## Related

- `test-results.txt` -- captured output from the gate run documented below
- `../driver-break/tests.rst` -- full plugin test suite reference (post-merge)
