# SRTM module (`feature-srtm`)

This branch splits the monolithic `driver_break_srtm.c` into focused translation
units and extracts the shared `gui_internal` runtime shim used by OSD dialogs.

This branch contains **only** the SRTM slice: `navit/plugin/driver_break_srtm/`
and `tests/srtm/`. There is no `navit/plugin/driver_break/` or
`navit/plugin/driver_ecu/` tree here; integration happens after merge to trunk.

## Source layout

| File | Responsibility |
|------|----------------|
| `driver_break_srtm.h` | Public SRTM API (unchanged signatures) |
| `driver_break_srtm_priv.h` | Shared constants and cross-module declarations |
| `driver_break_srtm_tiles.c` | Data directory, tile indexing, region catalog, free helpers |
| `driver_break_srtm_io.c` | HGT and GeoTIFF elevation sampling |
| `driver_break_srtm_download.c` | libcurl tile download with Copernicus, Viewfinder, NASA fallbacks |
| `driver_break_srtm_osd.c` | Navit command registration for SRTM menu/download/elevation |
| `driver_break_gui_shim.{c,h}` | Runtime `dlsym` resolution of `gui_internal` widgets (shared with Phase 5) |

## Public API

Elevation and tile management use the existing `srtm_*` symbols declared in
`driver_break_srtm.h`:

- `srtm_init`, `srtm_get_data_directory`
- `srtm_get_elevation`, `srtm_tile_exists`, `srtm_get_tile_filename`
- `srtm_calculate_tiles`, `srtm_get_regions`, `srtm_get_region`
- `srtm_download_region`, `srtm_download_pause`, `srtm_download_resume`, `srtm_download_cancel`
- `srtm_free_regions`, `srtm_free_tiles`, `srtm_free_download_context`

OSD integration:

- `driver_break_srtm_register_commands` (`driver_break_srtm_osd.h`)

## Data sources

1. **Primary (GeoTIFF):** Copernicus DEM GLO-30 on AWS S3
2. **Fallback:** Viewfinder Panoramas dem3 zip tiles (browser User-Agent required)
3. **Second fallback:** NASA SRTMGL1 zip archives

Tiles are stored under `~/.navit/srtm/` by default.

## Build and tests

The SRTM module builds as static library `driver_break_srtm` (links `navit_core`).
Unit tests live in `tests/srtm/` and do not link any other Driver Break sources.

```bash
cmake -S . -B build -DUSE_PLUGINS=y -DBUILD_DRIVER_BREAK_SRTM_TESTS=ON
cmake --build build --target driver_break_srtm test_driver_break_srtm -j$(nproc)
cd build && ctest -R driver_break_srtm_test --output-on-failure
```

The slice is gated behind `USE_PLUGINS` (Sailfish harbour builds use
`-DUSE_PLUGINS=n` and skip it).

Optional features:

- `HAVE_GEOTIFF` -- Copernicus GeoTIFF reads (requires libtiff)
- `HAVE_CURL` -- async tile download

## Related documentation

- `configuration.md` -- runtime paths and download behaviour
- `code-map.md` -- function inventory and dependency graph
- `complexity-report.txt` -- `lizard` output for touched files
- `tests.md` -- phase test plan and commands
- `test-results.txt` -- captured gate run output
