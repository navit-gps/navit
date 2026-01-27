# SRTM HGT Elevation Data Downloader

## Overview

The SRTM (Shuttle Radar Topography Mission) downloader module provides regional/country-based elevation data downloading for the Navit Rest plugin, mirroring how Navit handles map downloads.

## Features

- **Regional Downloads**: Download elevation data for predefined regions matching Navit's map regions
- **Tile Management**: Automatic calculation of required 1°×1° HGT tiles based on bounding boxes
- **Download Sources**: 
  - Primary: Viewfinder Panoramas (viewfinderpanoramas.org) - void-filled 1 arc-second data
  - Fallback: NASA SRTMGL1
- **Storage**: HGT files stored in `~/.navit/srtm/` (or platform equivalent)
- **Elevation Query API**: Query elevation by coordinate for routing integration
- **Download Manager**: Support for pause/resume (framework in place)

## Storage Location

HGT files are stored in:
- Linux: `~/.navit/srtm/`
- Windows: `%USERPROFILE%\navit\srtm\`
- Android: `/data/data/org.navitproject.navit/srtm/`

## Tile Format

- **Format**: SRTM HGT (1 arc-second resolution)
- **File naming**: `N45E006.hgt` (latitude direction, latitude index, longitude direction, longitude index)
- **File size**: ~25.9 MB uncompressed, ~2.8 MB compressed
- **Coverage**: Each tile covers 1°×1° (approximately 111 km × 111 km at equator)

## Available Regions

The downloader includes predefined regions matching Navit's map download regions:

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

Each region includes:
- Bounding box (min_lon, min_lat, max_lon, max_lat)
- Tile count calculation
- Download size estimate
- Progress tracking

## Usage

### Initialization

```c
/* Initialize SRTM system (uses default directory ~/.navit/srtm/) */
srtm_init(NULL);

/* Or specify custom directory */
srtm_init("/custom/path/to/srtm");
```

### Query Elevation

```c
struct coord_geo coord;
coord.lat = 52.5;
coord.lng = 13.4;

int elevation = srtm_get_elevation(&coord);
if (elevation != -32768) {
    printf("Elevation: %d meters\n", elevation);
} else {
    printf("Elevation data not available\n");
}
```

### Download Region

```c
/* Get region */
struct srtm_region *region = srtm_get_region("Germany");

/* Start download */
struct srtm_download_context *ctx = srtm_download_region(
    region,
    progress_callback,  /* Optional progress callback */
    user_data
);

/* Pause/resume/cancel */
srtm_download_pause(ctx);
srtm_download_resume(ctx);
srtm_download_cancel(ctx);
```

### List Available Regions

```c
GList *regions = srtm_get_regions();
GList *l = regions;

while (l) {
    struct srtm_region *region = (struct srtm_region *)l->data;
    printf("Region: %s, Tiles: %d, Size: %lld bytes, Progress: %d%%\n",
           region->name, region->tile_count, region->size_bytes, 
           region->progress_percent);
    l = g_list_next(l);
}

srtm_free_regions(regions);
```

## Integration with Routing

The elevation data can be queried during routing to:

1. **Calculate gradients**: Determine uphill/downhill segments
2. **Energy-based routing**: Use elevation in energy calculations
3. **Route optimization**: Prefer routes with moderate gradients

### Example: Get elevation for route segment

```c
/* For each route segment */
struct coord_geo start, end;
/* ... get segment coordinates ... */

int elev_start = srtm_get_elevation(&start);
int elev_end = srtm_get_elevation(&end);

if (elev_start != -32768 && elev_end != -32768) {
    double delta_h = (double)(elev_end - elev_start);
    double distance = coord_distance(&start, &end);
    double gradient = (delta_h / distance) * 100.0;  /* Percentage */
    
    /* Use gradient in routing cost calculation */
}
```

## Download Process

1. **Calculate Tiles**: Determine which 1°×1° tiles are needed for the region's bounding box
2. **Check Existing**: Skip tiles that are already downloaded
3. **Download**: Fetch ZIP files from Viewfinder Panoramas or NASA
4. **Extract**: Extract HGT files from ZIP archives
5. **Verify**: Check file integrity (checksum verification - TODO)
6. **Cache**: Keep loaded tiles in memory during routing session

## Tile Calculation

For a bounding box (min_lon, min_lat, max_lon, max_lat):

```c
int min_lon_idx = (int)floor(min_lon);
int max_lon_idx = (int)floor(max_lon);
int min_lat_idx = (int)floor(min_lat);
int max_lat_idx = (int)floor(max_lat);

/* Tiles needed: (max_lon_idx - min_lon_idx + 1) × (max_lat_idx - min_lat_idx + 1) */
```

Example: Germany (5.18°E to 15.47°E, 46.84°N to 55.64°N)
- Longitude tiles: 5 to 15 = 11 tiles
- Latitude tiles: 46 to 55 = 10 tiles
- Total: 11 × 10 = 110 tiles
- Size: 110 × 2.8 MB ≈ 308 MB

## HGT File Format

- **Resolution**: 1 arc-second (approximately 30 meters)
- **Grid size**: 3601 × 3601 pixels (covers 1°×1°)
- **Data type**: 16-bit signed integers (big-endian)
- **Void value**: -32768 (indicates no data)
- **Elevation range**: -500 to 8848 meters (approximately)

### Reading HGT File

```c
/* Calculate pixel position within tile */
int x = (int)((lon - tile_min_lon) * 3600.0);
int y = (int)((tile_max_lat - lat) * 3600.0);  /* Y inverted */

/* Calculate byte offset */
long offset = (long)y * 3601L * 2L + (long)x * 2L;

/* Read 16-bit big-endian value */
fseek(file, offset, SEEK_SET);
unsigned char bytes[2];
fread(bytes, 1, 2, file);
int16_t elevation = (int16_t)((bytes[0] << 8) | bytes[1]);
```

## Commands

The module registers Navit commands for GUI integration:

- `srtm_download_menu`: Show download menu with available regions
- `srtm_download_region <name>`: Download elevation data for a region
- `srtm_get_elevation <coord>`: Get elevation at coordinate

## Dependencies

- **libcurl**: Required for downloading HGT files
- **libzip** (optional): For extracting ZIP archives (currently placeholder)

## Status

### Implemented
- [x] Tile calculation from bounding boxes
- [x] Region definitions matching Navit map regions
- [x] HGT file reading and elevation query
- [x] File existence checking
- [x] Download context management
- [x] Command registration

### Pending
- [ ] Async download implementation (background thread)
- [ ] ZIP extraction (requires libzip integration)
- [ ] Checksum verification (MD5)
- [ ] Pause/resume implementation
- [ ] Progress callback integration
- [ ] GUI integration (download menu UI)
- [ ] Tile caching in memory
- [ ] Automatic tile loading on-demand

## Future Enhancements

1. **3 arc-second support**: Add support for lower-resolution SRTM data (smaller files)
2. **Custom regions**: Allow users to define custom bounding boxes
3. **Automatic updates**: Check for updated SRTM data
4. **Compression**: Store tiles in compressed format to save space
5. **Elevation interpolation**: Bilinear interpolation for sub-pixel accuracy
6. **Multi-threaded downloads**: Download multiple tiles in parallel

## References

- Viewfinder Panoramas: https://viewfinderpanoramas.org/
- NASA SRTM: https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL1.003/
- SRTM File Format: https://dds.cr.usgs.gov/srtm/version2_1/Documentation/SRTM_Topo.pdf
