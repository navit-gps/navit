# APRS Plugin Range Filtering

## Overview

Range filtering is a feature that limits which APRS stations are displayed on the map based on their distance from a specified center point. This helps reduce visual clutter and improve performance when tracking many stations.

## How It Works

### Distance Calculation

The plugin uses the **Haversine formula** to calculate the great-circle distance between two points on Earth:

```c
distance = 2 * R * atan2(√a, √(1-a))

where:
  a = sin²(Δlat/2) + cos(lat1) * cos(lat2) * sin²(Δlon/2)
  R = Earth's radius (6371 km)
```

This provides accurate distance calculations accounting for Earth's curvature.

### Filtering Process

1. **Center Point**: A geographic coordinate (latitude/longitude) is specified as the center
2. **Range**: A maximum distance in kilometers is set
3. **Query**: The database is queried for all stations
4. **Distance Check**: For each station, the distance from the center is calculated
5. **Filtering**: Only stations within the range are added to the map

## Configuration

### Basic Configuration

```xml
<map type="aprs" data="/path/to/aprs.db">
    <attribute name="distance" value="50000"/>
    <attribute name="position_coord_geo" lat="37.7749" lng="-122.4194"/>
</map>
```

### Parameters

- **`distance`** (double, in meters):
  - Maximum distance from center point
  - `0` = disabled (show all stations)
  - Example: `50000` = 50 km range
  - Default: `0` (no filtering)

- **`position_coord_geo`** (coord_geo):
  - Center point for range calculation
  - Format: `lat="latitude" lng="longitude"`
  - Required when distance > 0
  - Typically set to current position or area of interest

## Use Cases

### 1. Local Area Monitoring

Show only stations within a city or region:

```xml
<map type="aprs" data="/path/to/aprs.db">
    <attribute name="distance" value="25000"/>
    <attribute name="position_coord_geo" lat="40.7128" lng="-74.0060"/>
</map>
```

This shows stations within 25 km of New York City.

### 2. Current Position Tracking

Filter stations relative to your current location:

```xml
<map type="aprs" data="/path/to/aprs.db">
    <attribute name="distance" value="100000"/>
    <attribute name="position_coord_geo" lat="${current_lat}" lng="${current_lng}"/>
</map>
```

Shows stations within 100 km of your current position.

### 3. Performance Optimization

When tracking many stations globally, limit display to reduce processing:

```xml
<map type="aprs" data="/path/to/aprs.db">
    <attribute name="distance" value="200000"/>
    <attribute name="position_coord_geo" lat="51.5074" lng="-0.1278"/>
</map>
```

Shows only stations within 200 km of London, improving map rendering performance.

### 4. Disable Filtering

Show all stations regardless of distance:

```xml
<map type="aprs" data="/path/to/aprs.db">
    <attribute name="distance" value="0"/>
</map>
```

Or simply omit the distance attribute.

## Implementation Details

### Code Location

Range filtering is implemented in:

- **`aprs_db.c`**: `aprs_db_get_stations_in_range()` function
- **`aprs.c`**: `aprs_update_items()` function

### Algorithm

```c
// For each station in database:
1. Calculate distance using Haversine formula
2. Compare distance to range_km
3. If distance <= range_km:
   - Add station to display list
4. Else:
   - Discard station (not displayed)
```

### Performance Impact

**With Range Filtering Enabled:**
- Reduces number of items processed
- Faster map rendering
- Lower memory usage
- Better user experience with many stations

**Without Range Filtering:**
- All stations processed
- Higher CPU/memory usage
- Slower rendering with many stations
- Complete global view

## Examples

### Example 1: 50 km Range Around San Francisco

```xml
<map type="aprs" data="/var/lib/navit/aprs.db">
    <attribute name="distance" value="50000"/>
    <attribute name="position_coord_geo" lat="37.7749" lng="-122.4194"/>
</map>
```

**Result**: Only stations within 50 km of San Francisco are displayed.

### Example 2: 100 km Range Around Berlin

```xml
<map type="aprs" data="/var/lib/navit/aprs.db">
    <attribute name="distance" value="100000"/>
    <attribute name="position_coord_geo" lat="52.5200" lng="13.4050"/>
</map>
```

**Result**: Only stations within 100 km of Berlin are displayed.

### Example 3: No Filtering (Global View)

```xml
<map type="aprs" data="/var/lib/navit/aprs.db">
    <attribute name="distance" value="0"/>
</map>
```

**Result**: All stations in the database are displayed, regardless of location.

## Distance Reference

Common range values:

| Distance (meters) | Distance (km) | Typical Use Case |
|-------------------|---------------|------------------|
| 10,000 | 10 km | City center |
| 25,000 | 25 km | Metropolitan area |
| 50,000 | 50 km | Regional area |
| 100,000 | 100 km | Large region |
| 200,000 | 200 km | State/province |
| 500,000 | 500 km | Country/continent |
| 0 | Unlimited | Global view |

## Dynamic Updates

The range filtering is applied when:
- Map items are updated (every 1 second by default)
- New packets are received
- Map is refreshed

**Note**: The center point is static once set. To change it, update the configuration and reload the map.

## Technical Notes

1. **Distance Calculation**: Uses Haversine formula for accuracy on Earth's surface
2. **Database Query**: All stations are loaded, then filtered in memory (for simplicity)
3. **Performance**: For very large databases, consider adding SQL-based filtering
4. **Coordinate System**: Uses WGS84 (standard GPS coordinates)

## Troubleshooting

### No stations showing

1. Check if range is too small
2. Verify center point coordinates are correct
3. Ensure stations exist in database
4. Check expiration timeout (stations may have expired)

### Too many stations

1. Reduce the `distance` value
2. Enable range filtering if disabled
3. Adjust center point to area of interest

### Performance issues

1. Enable range filtering
2. Reduce range distance
3. Increase expiration timeout to reduce database size

