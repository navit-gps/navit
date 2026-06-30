# SRTM configuration (`feature-srtm`)

The SRTM module has no XML-specific keys beyond the Driver Break plugin itself.
Behaviour is controlled at compile time and through filesystem layout.

## Data directory

| Setting | Default | Description |
|---------|---------|-------------|
| `srtm_init(path)` argument | `~/.navit/srtm/` | Root directory for cached `.hgt` and Copernicus `.tif` tiles |
| Lazy init | `srtm_get_data_directory()` calls `srtm_init(NULL)` | Creates the default directory on first use |

Pass an explicit path to `srtm_init` before elevation lookup or download if a
non-default cache location is required.

## Compile-time features

| Macro | Dependency | Effect |
|-------|------------|--------|
| `HAVE_GEOTIFF` | libtiff | Read Copernicus GeoTIFF tiles; prefer GeoTIFF over HGT |
| `HAVE_CURL` | libcurl | Enable `srtm_download_region` and async idle-driven downloads |

Without libcurl, region download commands log an error and return NULL.

## Download behaviour

- User-Agent string mimics a desktop browser (Viewfinder blocks bare scripts).
- Copernicus `.tif` downloads are validated by TIFF magic bytes.
- Zip fallbacks extract HGT files with size checks (~25 MB uncompressed).
- NASA archives use a distinct internal filename (`*.SRTMGL1.hgt`) renamed to the standard `NxxExxx.hgt` form.

## Regions

Predefined regions in `driver_break_srtm_tiles.c` mirror common Navit map areas
(Europe, Germany, Norway, United States, etc.). Each region stores a bounding
box used by `srtm_calculate_tiles` to enumerate 1°×1° tiles.

## GUI shim

`driver_break_gui_shim` resolves `gui_internal` symbols at runtime via `dlopen`.
No configuration is required; the shim searches standard Navit install paths
then falls back to `RTLD_DEFAULT`.
