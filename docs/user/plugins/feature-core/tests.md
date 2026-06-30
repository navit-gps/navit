# Tests (`feature-core`)

Core split (database modules, config, fuel learning helpers) is validated by config
and database unit tests plus a full plugin build.

## Executables

- `test_driver_break_db` — Split `driver_break_db_*` modules (history, config, fuel, schema)
- `test_driver_break_config` — `driver_break_config_default()` defaults and session structs
- `plugin_driver_break` — Full core slice including lifecycle and route modules

## Build and run

From a Navit build directory with plugins enabled:

```bash
cmake .. -DUSE_PLUGINS=y -DBUILD_DRIVER_BREAK_CORE_TESTS=ON
cmake --build . --target test_driver_break_db test_driver_break_config plugin_driver_break -j$(nproc)
./tests/core/test_driver_break_db
./tests/core/test_driver_break_config
ctest -R driver_break --output-on-failure
```

## Gate

Phase completion requires these tests to exit 0 and `lizard -C 10` to report no CCN
violations on phase sources (see `complexity-report.txt`).

## Related

- `test-results.txt` — captured output from the gate run
- `../driver-break/tests.rst` — full plugin test suite reference (post-merge)
