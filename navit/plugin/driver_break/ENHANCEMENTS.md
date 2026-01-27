# Rest Plugin Enhancements - Energy-Based Routing and Hiking/Cycling Support

## Overview

This document describes the enhancements made to the Rest plugin to support:
- Energy-based routing with kinematic calculations (inspired by BRouter)
- Hiking-specific rest stop calculations
- Cycling-specific rest stop calculations
- Enhanced POI search for water points and cabins
- Route validation for hiking (forbidden highways)
- Glacier proximity checking for nightly rest stops

## Energy-Based Routing

### Implementation

The energy-based routing module (`rest_energy.c`) implements a simplified kinematic model based on BRouter's approach:

**Key Components:**
- **Energy Model**: Physical parameters (weight, rolling resistance, air resistance, recuperation)
- **Segment Energy Calculation**: Considers distance, elevation change, speed limits, temperature, and air pressure
- **Cost Calculation**: Energy consumption converted to routing cost

**Physical Parameters:**
- `totalweight`: Total weight (vehicle + cargo + person) in kg
- `f_roll`: Rolling resistance coefficient
- `f_air`: Air resistance coefficient
- `f_recup`: Recuperation coefficient for downhill sections
- `p_standby`: Standby power consumption (W)
- `recup_efficiency`: Recuperation efficiency (0.0-1.0)
- `outside_temp`: Outside temperature (Celsius)

**Energy Calculation:**
```
Energy = (Rolling Force + Air Force + Elevation Force) × Distance + Standby Energy
Cost = (Power × Time + Energy) / Minimum Cost
```

**Elevation Consideration:**
- Uphill: Additional energy required (gravity component)
- Downhill: Energy recuperation possible (with efficiency factor)
- Temperature correction: Air resistance adjusted for temperature
- Air pressure correction: Air resistance adjusted for elevation

### Integration with Navit Routing

Currently, Navit's routing system uses time-based costs. To integrate energy-based routing:

1. **Option 1**: Modify route cost calculation to use energy costs when enabled
2. **Option 2**: Use energy costs as additional penalty/adjustment to time costs
3. **Option 3**: Create separate routing profile for energy-based routing

**Current Status**: Energy model is implemented but not yet integrated into Navit's route cost calculation. This requires modification of `route_time_seg()` or creation of a custom routing profile.

## Hiking Support

### Rest Stop Calculations

**Main Rest Distance**: 11.295 km (default)
**Alternative Rest Distance**: 2.275 km
**Maximum Daily Distance**: 40 km

**Implementation** (`rest_hiking.c`):
- `hiking_calculate_rest_stops()`: Calculates rest stops along route
- `hiking_calculate_daily_segments()`: Splits route into daily segments (max 40 km/day)

**Usage:**
```c
GList *rest_stops = hiking_calculate_rest_stops(total_distance_meters, use_alternative);
GList *daily_segments = hiking_calculate_daily_segments(total_distance_meters, max_daily_km);
```

### Route Validation

**Forbidden Highways for Hikers:**
- `motorway`
- `trunk`
- `primary`
- `primary_link`
- `motorway_link`
- `trunk_link`

**Priority Paths:**
- `footway`
- `path`
- `track`
- `steps`
- `bridleway`

**Implementation** (`rest_route_validator.c`):
- Validates route against forbidden highways
- Calculates percentage of route on priority paths
- Generates warnings for routes using forbidden highways

## Cycling Support

### Rest Stop Calculations

**Main Rest Distance**: 28.24 km (scaled from hiking by 2.5x)
**Alternative Rest Distance**: 5.69 km
**Maximum Daily Distance**: 100 km

**Implementation** (`rest_cycling.c`):
- `cycling_calculate_rest_stops()`: Calculates main and alternative rest stops
- Ensures alternative rests are >1 km from main rests
- `cycling_calculate_daily_segments()`: Splits route into daily segments (max 100 km/day)

**Scaling Factor**: Cycling distances are 2.5x hiking distances, reflecting typical speed differences:
- Hiker: ~40 km/day
- Trekking Cyclist (loaded): ~100 km/day

## Enhanced POI Search

### Water Points (2 km radius)

**POI Types:**
- `amenity=drinking_water`
- `amenity=fountain`
- `natural=spring`

**Implementation** (`rest_poi_hiking.c`):
- `poi_search_water_points()`: Searches for water points using Overpass API
- Sorts by distance from rest stop
- Marks natural springs with warning ("Drink at your own risk")

### Cabins/Huts (5 km radius)

**POI Types:**
- `tourism=wilderness_hut`
- `tourism=alpine_hut`
- `tourism=hostel`
- `tourism=camp_site`

**Special Features:**
- DNT (Norwegian Trekking Association) cabin detection
- Network cabin detection (DNT, STF, etc.)
- Locked/unlocked status
- Water availability

**Implementation** (`rest_poi_hiking.c`):
- `poi_search_cabins()`: Searches for cabins/huts using Overpass API
- Sorts by distance
- Identifies network cabins for membership-based access

## Enhanced Land Use Restrictions

### Standard Restrictions

Rest stops are excluded from:
- `landuse=farmland`
- `landuse=residential`
- `landuse=commercial`
- `landuse=industrial`
- `landuse=military`
- `landuse=recreation_ground`

### Nightly Rest Stop Requirements

**Minimum Distances:**
- **Buildings**: 150 meters (standard)
- **Glaciers**: 300 meters (for nightly camping)

**Glacier Proximity Checking** (`rest_glacier.c`):
- `glacier_is_too_close_for_camping()`: Checks if position is too close to glaciers
- Exempts camping-suitable buildings (alpine_hut, wilderness_hut, shelter, etc.)
- Searches for glaciers within 1 km radius

**Camping-Suitable Buildings** (exempt from glacier distance rule):
- `alpine_hut`
- `wilderness_hut`
- `basic_hut`
- `shelter`
- `hut`
- `cabin`
- `chalet`

## Configuration

### Vehicle Type Selection

```xml
<osd type="rest" enabled="yes">
    <attr name="type" value="hiking"/>
    <!-- or: "car", "truck", "cycling" -->
</osd>
```

### Energy Routing Configuration

```xml
<osd type="rest" enabled="yes">
    <attr name="type" value="cycling"/>
    <attr name="use_energy_routing" value="1"/>
    <attr name="total_weight" value="95.0"/>
</osd>
```

### Hiking Configuration

```xml
<osd type="rest" enabled="yes">
    <attr name="type" value="hiking"/>
    <attr name="hiking_rest_distance_main" value="11295.0"/>
    <attr name="hiking_max_daily_distance" value="40000.0"/>
</osd>
```

### Cycling Configuration

```xml
<osd type="rest" enabled="yes">
    <attr name="type" value="cycling"/>
    <attr name="cycling_rest_distance_main" value="28240.0"/>
    <attr name="cycling_max_daily_distance" value="100000.0"/>
</osd>
```

## API Reference

### Energy Routing

```c
/* Initialize energy model */
void energy_model_init(struct energy_model *model, double totalweight, 
                       double f_roll, double f_air, double p_standby);

/* Calculate energy cost for segment */
struct energy_result energy_calculate_segment(struct energy_model *model,
                                               double distance,
                                               double delta_h,
                                               double elevation,
                                               double speed_limit);
```

### Hiking

```c
/* Calculate hiking rest stops */
GList *hiking_calculate_rest_stops(double total_distance, int use_alternative);

/* Calculate daily segments */
GList *hiking_calculate_daily_segments(double total_distance, double max_daily_km);
```

### Cycling

```c
/* Calculate cycling rest stops */
GList *cycling_calculate_rest_stops(double total_distance, int use_alternative);

/* Calculate daily segments */
GList *cycling_calculate_daily_segments(double total_distance);
```

### POI Search

```c
/* Search for water points */
GList *poi_search_water_points(struct coord_geo *rest_stop, double radius_km);

/* Search for cabins/huts */
GList *poi_search_cabins(struct coord_geo *rest_stop, double radius_km);
```

### Route Validation

```c
/* Validate hiking route */
struct route_validation_result *route_validator_validate_hiking(struct route *route);

/* Check if highway is forbidden */
int route_validator_is_forbidden_highway(const char *highway_type);
```

### Glacier Checking

```c
/* Check if too close to glacier for camping */
int glacier_is_too_close_for_camping(struct coord_geo *position, 
                                      struct mapset *ms,
                                      int has_camping_building);
```

## Integration Status

### Completed
- [x] Energy model implementation
- [x] Hiking rest calculator
- [x] Cycling rest calculator
- [x] POI search for water and cabins
- [x] Route validation framework
- [x] Glacier proximity checking
- [x] Enhanced land use restrictions
- [x] Configuration system updates

### Pending
- [ ] Energy-based routing integration with Navit's route cost calculation
- [ ] Full route validation implementation (requires way tag access)
- [ ] Complete POI tag parsing (currently simplified)
- [ ] Glacier detection from map data (currently placeholder)
- [ ] UI integration for hiking/cycling modes
- [ ] Distance-based rest stop calculation along actual route

## Future Enhancements

1. **Full Energy Routing Integration**: Modify Navit's routing to use energy costs
2. **Elevation Data Integration**: Use heightlines map for accurate elevation calculations
3. **Advanced POI Tag Parsing**: Full OSM tag parsing for cabins (operator, locked, etc.)
4. **Route Distance Calculation**: Calculate actual route distances for rest stop placement
5. **Visual Indicators**: Show energy consumption, elevation profiles, POI locations on map
6. **User Preferences**: Customizable rest distances, POI preferences, energy model parameters

## References

- BRouter KinematicModel: `/mnt/.../brouter/brouter-core/src/main/java/btools/router/KinematicModel.java`
- BRouter HikingRestCalculator: `/mnt/.../brouter/brouter-core/src/main/java/btools/router/HikingRestCalculator.java`
- BRouter CyclingRestCalculator: `/mnt/.../brouter/brouter-core/src/main/java/btools/router/CyclingRestCalculator.java`
- BRouter RestStopPOISearcher: `/mnt/.../brouter/brouter-core/src/main/java/btools/router/RestStopPOISearcher.java`
