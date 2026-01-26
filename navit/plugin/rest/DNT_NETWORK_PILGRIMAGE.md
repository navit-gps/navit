# DNT/Network and Pilgrimage Route Support

## Overview

The Rest plugin now fully integrates DNT (Den Norske Turistforening) and network hut support, as well as pilgrimage route prioritization, based on BRouter's implementation patterns.

## Features

### 1. DNT/Network Hut Detection

The plugin detects network-operated cabins and huts by checking OSM tags:
- **operator=dnt** or **operator=Den Norske Turistforening** → DNT network
- **operator=stf** or **operator=Svenska Turistforeningen** → STF network
- **network=*** → Other network organizations

**Implementation:**
- `rest_poi_is_network_cabin()` checks OSM tags via `attr_osm_tag` attribute
- Network information is stored in cabin structures (`is_dnt`, `is_network`, `network` fields)
- Network detection works with both map-based POI queries and Overpass API fallback

### 2. DNT Priority Mode

When `enable_dnt_priority` is enabled:
- **Search radius**: Increases from 5 km to 25 km for network huts (matches typical 20-25 km DNT hut spacing)
- **Daily distance**: Adjusts from 40 km to 46.4 km (90th percentile of DNT hut spacing)
- **Prioritization**: Network cabins are sorted first, then by distance
- **Overnight stops**: Locks to nearest network cabin when available

**Configuration:**
```c
config->enable_dnt_priority = 1;  /* Enable DNT priority mode */
config->network_hut_search_radius_km = 25;  /* 25 km for network huts */
```

### 3. Pilgrimage Route Support

The plugin detects and prioritizes pilgrimage routes by checking OSM tags:
- **route=hiking** → Hiking route
- **pilgrimage=yes** → Pilgrimage route (e.g., Camino de Santiago networks)

**Implementation:**
- `rest_route_is_pilgrimage_hiking()` checks route segment tags
- When `enable_hiking_pilgrimage_priority` is enabled, pilgrimage/hiking routes are counted as priority paths in route validation
- Route segments with these tags are preferred over secondary roads

**Configuration:**
```c
config->enable_hiking_pilgrimage_priority = 1;  /* Enable pilgrimage route priority */
```

### 4. Map-Based POI Discovery

**Primary Method**: Direct map queries (reliable, no timeouts)
- Queries POI items directly from Navit's mapset
- Uses `map_rect_get_item()` with item type filters
- Extracts OSM tags via `attr_osm_tag` for network detection
- No network dependency, works offline

**Fallback Method**: Overpass API (when mapset not available)
- Used only if mapset cannot be accessed
- Subject to timeouts and server load issues
- JSON parsing implemented for basic POI extraction

**Implementation:**
- `rest_poi_map_search()` - General POI search in maps
- `rest_poi_map_search_water()` - Water points (2 km radius)
- `rest_poi_map_search_cabins()` - Cabins/huts with DNT detection (5-25 km radius)
- `rest_poi_map_search_car_pois()` - Car POIs (cafes, restaurants, museums)

## Integration Points

### Route Validation

Route validation now checks for pilgrimage/hiking route tags when priority mode is enabled:

```c
struct route_validation_result *result = route_validator_validate_hiking_with_priority(
    route, 
    config->enable_hiking_pilgrimage_priority
);
```

Pilgrimage/hiking routes are counted as priority paths, improving route safety scoring.

### Rest Stop Calculation

For hiking routes with DNT priority enabled:

```c
double max_daily = config->enable_dnt_priority ? 46400.0 : 40000.0;
GList *stops = hiking_calculate_rest_stops_with_max(total_distance, 0, max_daily);
```

### POI Discovery

POI discovery automatically uses map-based queries when mapset is available:

```c
struct mapset *ms = navit_get_mapset(nav);
if (ms) {
    /* Use map-based search (preferred) */
    GList *cabins = poi_search_cabins_map(rest_stop, radius_km, ms, enable_dnt_priority);
} else {
    /* Fallback to Overpass API */
    GList *cabins = poi_search_cabins(rest_stop, radius_km);
}
```

## Configuration

Add to `navit.xml`:

```xml
<plugin name="rest">
    <config>
        <!-- Enable DNT/network priority mode -->
        <attribute name="enable_dnt_priority" value="1"/>
        
        <!-- Enable hiking/pilgrimage route priority -->
        <attribute name="enable_hiking_pilgrimage_priority" value="1"/>
        
        <!-- Network hut search radius (when DNT priority enabled) -->
        <attribute name="network_hut_search_radius_km" value="25"/>
    </config>
</plugin>
```

## API Functions

### Network Detection
- `int rest_poi_is_network_cabin(struct item *item, char **network_name)` - Check if POI is a network cabin
- `int rest_route_is_pilgrimage_hiking(struct item *item)` - Check if route segment is pilgrimage/hiking

### POI Search (Map-Based)
- `GList *rest_poi_map_search(struct coord_geo *center, double radius_km, enum item_type *poi_types, int num_types, struct mapset *ms)`
- `GList *rest_poi_map_search_water(struct coord_geo *center, double radius_km, struct mapset *ms)`
- `GList *rest_poi_map_search_cabins(struct coord_geo *center, double radius_km, struct mapset *ms)`
- `GList *poi_search_water_points_map(struct coord_geo *rest_stop, double radius_km, struct mapset *ms)`
- `GList *poi_search_cabins_map(struct coord_geo *rest_stop, double radius_km, struct mapset *ms, int enable_dnt_priority)`

### Route Validation
- `struct route_validation_result *route_validator_validate_hiking_with_priority(struct route *route, int enable_hiking_pilgrimage_priority)`

## Benefits

1. **Reliability**: Map-based POI queries avoid Overpass API timeouts
2. **Offline Support**: Works without internet connection
3. **Network Support**: Full DNT/STF and other network detection
4. **Route Safety**: Pilgrimage/hiking route prioritization improves route validation
5. **BRouter Compatibility**: Matches BRouter's network priority and search radius logic

## Limitations

- OSM tag access depends on map format (works with OSM-based maps)
- Network detection from Overpass API fallback is limited (name-based only)
- Full network cabin prioritization requires mapset access

## Future Enhancements

- Vehicle profile integration for active route prevention of unsafe roads
- Visual indicators for network cabins on map
- Network membership validation (check if user has DNT/STF membership)
- Extended network support (more organizations beyond DNT/STF)
