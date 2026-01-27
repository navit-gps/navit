# SRTM Elevation Data Implementation Summary

## Status: Implemented

The SRTM HGT elevation data downloader has been implemented as part of the Navit Rest plugin.

## What Was Implemented

### Core Module (`rest_srtm.c/h`)

1. **SRTM System Initialization**
   - Automatic directory creation (`~/.navit/srtm/`)
   - Platform-specific path handling

2. **Tile Management**
   - Automatic calculation of required 1°×1° HGT tiles from bounding boxes
   - Tile filename generation (`N45E006.hgt` format)
   - Tile existence checking

3. **Elevation Query API**
   - `srtm_get_elevation()`: Query elevation by coordinate
   - HGT file reading with proper big-endian handling
   - Returns -32768 for void/missing data

4. **Regional Support**
   - Predefined regions matching Navit's map download regions
   - Region list with bounding boxes, tile counts, and size estimates
   - Progress tracking per region

5. **Download Framework**
   - Download context management
   - Primary source: Viewfinder Panoramas
   - Fallback source: NASA SRTMGL1
   - Pause/resume/cancel support (framework ready)

### Command Integration (`rest_srtm_osd.c/h`)

- `srtm_download_menu`: Show download menu
- `srtm_download_region <name>`: Download region
- `srtm_get_elevation <coord>`: Get elevation at coordinate

### Integration Points

- **Rest Plugin**: SRTM system initialized when rest plugin loads
- **Energy Routing**: Framework ready for elevation integration
- **Navit Commands**: Commands registered for GUI access

## File Structure

```
navit/plugin/rest/
├── rest_srtm.h              (SRTM API definitions)
├── rest_srtm.c              (SRTM implementation)
├── rest_srtm_osd.h          (Command definitions)
├── rest_srtm_osd.c          (Command implementation)
├── SRTM_DOWNLOADER.md       (User documentation)
└── SRTM_IMPLEMENTATION.md   (This file)
```

## Storage

- **Location**: `~/.navit/srtm/` (or platform equivalent)
- **Format**: Raw HGT files (1 arc-second, 3601×3601 pixels)
- **Naming**: `N45E006.hgt` (latitude direction, lat index, lon direction, lon index)
- **Size**: ~25.9 MB per tile uncompressed, ~2.8 MB compressed

## Available Regions

- Europe
- Germany
- France
- United Kingdom
- Italy
- Spain
- Norway
- Sweden
- Poland
- United States
- China
- Japan

## Usage Example

```c
/* Initialize */
srtm_init(NULL);

/* Query elevation */
struct coord_geo coord = {52.5, 13.4};
int elevation = srtm_get_elevation(&coord);
if (elevation != -32768) {
    printf("Elevation: %d meters\n", elevation);
}

/* Download region */
struct srtm_region *region = srtm_get_region("Germany");
struct srtm_download_context *ctx = srtm_download_region(region, NULL, NULL);
```

## What's Pending

1. **Async Download**: Background thread implementation for downloads
2. **ZIP Extraction**: libzip integration for extracting HGT from ZIP archives
3. **Checksum Verification**: MD5 checksum validation
4. **GUI Integration**: Full UI integration with Navit's map download interface
5. **Tile Caching**: In-memory caching of loaded tiles during routing
6. **On-Demand Loading**: Automatic tile loading when elevation is queried

## Dependencies

- **libcurl**: Required for downloading (checked with `HAVE_CURL`)
- **libzip** (optional): For ZIP extraction (not yet integrated)

## Integration with Energy Routing

The elevation data can be used in energy-based routing:

```c
/* Get elevation for route segment endpoints */
int elev_start = srtm_get_elevation(&segment_start);
int elev_end = srtm_get_elevation(&segment_end);

if (elev_start != -32768 && elev_end != -32768) {
    double delta_h = (double)(elev_end - elev_start);
    /* Use in energy calculation */
    struct energy_result result = energy_calculate_segment(
        model, distance, delta_h, elev_start, speed_limit
    );
}
```

## Testing

To test the implementation:

1. **Initialize SRTM**:
   ```c
   srtm_init(NULL);
   ```

2. **List regions**:
   ```c
   GList *regions = srtm_get_regions();
   /* ... iterate and display ... */
   ```

3. **Query elevation** (requires downloaded tile):
   ```c
   struct coord_geo coord = {52.5, 13.4};  /* Berlin */
   int elevation = srtm_get_elevation(&coord);
   ```

4. **Download region** (requires libcurl):
   ```c
   struct srtm_region *region = srtm_get_region("Germany");
   struct srtm_download_context *ctx = srtm_download_region(region, NULL, NULL);
   ```

## Notes

- The download implementation currently requires manual ZIP extraction
- Checksum verification is planned but not yet implemented
- The GUI integration is a placeholder - full UI would need to be implemented in Navit's GUI system
- Tile sharing between overlapping regions is automatic (tiles are stored once)

## References

- Viewfinder Panoramas: https://viewfinderpanoramas.org/
- NASA SRTM: https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL1.003/
- SRTM File Format: Standard 1 arc-second HGT format
