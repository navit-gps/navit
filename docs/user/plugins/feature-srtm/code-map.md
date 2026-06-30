# SRTM code map (`feature-srtm`)

## Function inventory

| Function | File | Lines (approx.) | CCN |
|----------|------|-----------------|-----|
| `srtm_init` | `driver_break_srtm_tiles.c` | 32–70 | 8 |
| `srtm_get_data_directory` | tiles | 72–77 | 2 |
| `srtm_get_tile_filename` | tiles | 79–86 | 3 |
| `srtm_viewfinder_zone` | tiles | 88–92 | 5 |
| `srtm_get_geotiff_tile_filename` | tiles | 95–102 | 3 |
| `srtm_tile_exists` | tiles | 105–123 | 3 |
| `srtm_calculate_tiles` | tiles | 172–189 | 4 |
| `srtm_get_regions` | tiles | 228–244 | 2 |
| `srtm_get_region` | tiles | 246–264 | 3 |
| `srtm_free_regions` | tiles | 266–277 | 3 |
| `srtm_free_tiles` | tiles | 279–295 | 3 |
| `srtm_free_download_context` | tiles | 297–303 | 2 |
| `read_hgt_elevation` | `driver_break_srtm_io.c` | 53–84 | 6 |
| `srtm_get_elevation` | io | 185–222 | 6 |
| `srtm_download_region` | `driver_break_srtm_download.c` | 265–298 | 3 |
| `srtm_download_pause/resume/cancel` | download | 300–319 | 2–3 |
| `driver_break_srtm_register_commands` | `driver_break_srtm_osd.c` | 135–139 | 1 |
| `driver_break_gui_shim_resolve` | `driver_break_gui_shim.c` | 33–102 | 8 |

See `complexity-report.txt` for the full `lizard` listing.

## Dependency graph

```
driver_break_srtm_osd.c
    --> driver_break_srtm.h
    --> navit command table

driver_break_srtm_tiles.c
    --> driver_break_srtm_priv.h
    --> navit (user data dir), file.c

driver_break_srtm_io.c
    --> driver_break_srtm_priv.h
    --> libtiff (optional)

driver_break_srtm_download.c
    --> driver_break_srtm_priv.h
    --> libcurl (optional), event/callback idle loop

driver_break_gui_shim.c
    --> dlopen/dlsym gui_internal.so
```

## Inbound edges (post-merge consumers of SRTM API)

- `driver_break_srtm_osd.c` -- commands and registration
- `driver_break_energy.c` / route modules on trunk -- `srtm_get_elevation`
- Unit tests -- `tests/srtm/test_driver_break_srtm.c`

## Outbound edges

- Navit core: `navit_get_user_data_directory`, `event_add_idle`, `callback`
- Optional: libcurl, libtiff
- GUI shim: `gui_internal` shared library at runtime
