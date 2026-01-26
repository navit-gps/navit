# Route Safety Handling for Hikers and Cyclists

## Current Implementation Status

### Overview

The Rest plugin provides **post-route validation** for hiking routes, but does **not actively prevent** routing on unsafe roads during route calculation. The safety handling is implemented through:

1. **Route Validation Module** (`rest_route_validator.c/h`)
2. **Vehicle Profile Integration** (framework ready, not yet implemented)
3. **Rest Stop Location Filtering** (excludes unsafe areas)

## How It Currently Works

### 1. Route Validation (Post-Route Analysis)

**Location**: `rest_route_validator.c`

**Functionality**:
- **Validates** a calculated route after it's been generated
- **Checks** if the route uses forbidden highways
- **Calculates** statistics about route composition
- **Generates warnings** if unsafe roads are detected

**Forbidden Highways for Hikers**:
```c
static const char *forbidden_highways[] = {
    "motorway",      // Motorways (highway=motorway)
    "trunk",         // Trunk roads (highway=trunk)
    "primary",       // Primary roads (highway=primary)
    "primary_link",  // Primary road links
    "motorway_link", // Motorway links/ramps
    "trunk_link",    // Trunk road links
    NULL
};
```

**Priority Paths (Safe for Hikers)**:
```c
static const char *priority_paths[] = {
    "footway",       // Dedicated footpaths
    "path",          // General paths
    "track",         // Tracks (unpaved)
    "steps",         // Stairs/steps
    "bridleway",     // Bridleways (horses/bikes)
    NULL
};
```

**Usage**:
```c
struct route_validation_result *result = route_validator_validate_hiking(route);
if (!result->is_valid) {
    // Route uses forbidden highways
    // Display warnings to user
}
```

**Current Limitation**: The validation function is a **placeholder** - it doesn't actually extract highway types from route segments yet. Full implementation requires:
- Access to way/item attributes from route segments
- Highway type extraction from OSM tags
- Segment length calculation

### 2. Vehicle Profile Integration (Not Yet Implemented)

**How Navit Controls Route Safety**:

Navit uses **vehicle profiles** to determine which road types are accessible:

```xml
<vehicleprofile name="hiking" flags="0x80000000" ...>
    <roadprofile item_types="footway,path,track" speed="5" route_weight="10"/>
    <!-- Excludes motorway, trunk, primary by not listing them -->
</vehicleprofile>
```

**Current Status**: The plugin does **not** create or modify vehicle profiles for hiking/cycling. To properly prevent unsafe roads, the plugin would need to:

1. **Create hiking vehicle profile** that excludes:
   - `highway_land` (motorway)
   - `highway_city` (motorway in cities)
   - `street_n_lanes` (trunk/primary roads)

2. **Create cycling vehicle profile** that:
   - Allows bicycle-accessible roads
   - Excludes motorways
   - Prefers cycleways and paths

3. **Set vehicle profile** when hiking/cycling mode is selected

**Integration Point**: `route_value_seg()` in `navit/route.c` checks vehicle profile flags:
```c
if ((over->data.flags & profile->flags_forward_mask) != profile->flags)
    return INT_MAX;  // Segment excluded from routing
```

### 3. Rest Stop Location Filtering

**Location**: `rest_finder.c`

**Functionality**:
- Rest stops are **only suggested** at safe locations
- Filters out locations in forbidden landuse areas
- Ensures minimum distance from buildings/glaciers

**Safe Rest Stop Criteria**:
- Intersections between safe highway types:
  - `highway=unclassified`
  - `highway=service`
  - `highway=track`
  - `highway=rest_area`
  - `highway=tertiary`

**Excluded Areas**:
- `landuse=farmland`
- `landuse=residential`
- `landuse=commercial`
- `landuse=industrial`
- `landuse=military`
- `landuse=recreation_ground`

## What's Missing for Full Safety Implementation

### 1. Active Route Prevention

**Problem**: Currently, the plugin can only **warn** about unsafe routes after they're calculated. It cannot **prevent** routing on unsafe roads.

**Solution**: Integrate with Navit's vehicle profile system:

```c
/* When hiking mode is selected */
if (config->vehicle_type == REST_VEHICLE_HIKING) {
    /* Set hiking vehicle profile that excludes motorways/trunk/primary */
    struct attr profile_attr;
    profile_attr.type = attr_vehicleprofile_name;
    profile_attr.u.str = "hiking_safe";
    navit_set_attr(nav, &profile_attr);
}
```

### 2. Complete Route Validation

**Problem**: Route validator doesn't actually extract highway types from route segments.

**Solution**: Implement full route segment analysis:
```c
/* Iterate through route path */
struct route_path *path = route_get_path(route);
for (each segment in path) {
    /* Get highway type from segment */
    char *highway_type = get_highway_type_from_segment(segment);
    
    if (route_validator_is_forbidden_highway(highway_type)) {
        forbidden_length += segment_length;
        result->forbidden_segments++;
    } else if (route_validator_is_priority_path(highway_type)) {
        path_length += segment_length;
    }
}
```

### 3. Cycling-Specific Safety Rules

**Problem**: No specific validation for cyclists.

**Solution**: Add cycling route validator:
```c
/* Forbidden for cyclists (similar but may allow some primary roads) */
static const char *cycling_forbidden_highways[] = {
    "motorway",
    "motorway_link",
    NULL
};

/* Cycling priority paths */
static const char *cycling_priority_paths[] = {
    "cycleway",
    "path",
    "track",
    "bridleway",
    NULL
};
```

### 4. Visual Indicators

**Problem**: No visual feedback about route safety on the map.

**Solution**: Use map markers to indicate:
- Safe segments (green)
- Unsafe segments (red)
- Warning segments (yellow)

## Recommended Implementation Approach

### Phase 1: Complete Route Validation

1. Implement full highway type extraction from route segments
2. Calculate accurate statistics (percentages, segment counts)
3. Generate detailed warnings

### Phase 2: Vehicle Profile Integration

1. Create hiking vehicle profile XML definition
2. Create cycling vehicle profile XML definition
3. Add plugin command to switch vehicle profiles
4. Auto-switch profile when hiking/cycling mode selected

### Phase 3: Active Route Filtering

1. Modify route calculation to exclude unsafe segments
2. Add penalties for less-safe roads (instead of complete exclusion)
3. Provide route alternatives that avoid unsafe roads

### Phase 4: Visual Feedback

1. Add map overlays for route safety
2. Color-code route segments by safety level
3. Show warnings in navigation UI

## Current Workaround

Until full implementation is complete, users can:

1. **Use Navit's built-in vehicle profiles**:
   - Select "pedestrian" profile for hiking
   - Select "bike" profile for cycling
   - These profiles already exclude motorways

2. **Check route validation warnings**:
   - After route calculation, check validation results
   - Recalculate route if warnings indicate unsafe roads

3. **Manual route adjustment**:
   - Use waypoints to force route through safe areas
   - Avoid areas known to have motorways/trunk roads

## API Reference

### Route Validation Functions

```c
/* Validate hiking route */
struct route_validation_result *route_validator_validate_hiking(struct route *route);

/* Check if highway type is forbidden */
int route_validator_is_forbidden_highway(const char *highway_type);

/* Check if highway type is priority path */
int route_validator_is_priority_path(const char *highway_type);

/* Free validation result */
void route_validator_free_result(struct route_validation_result *result);
```

### Validation Result Structure

```c
struct route_validation_result {
    double path_percentage;      /* % on priority paths (footway, path, track) */
    double forbidden_percentage; /* % on forbidden highways (motorway, trunk, primary) */
    double secondary_percentage; /* % on secondary roads */
    int forbidden_segments;      /* Number of forbidden segments */
    GList *warnings;             /* Warning messages */
    int is_valid;                /* 1 if route is safe, 0 if uses forbidden highways */
};
```

## Integration with Navit Vehicle Profiles

Navit's vehicle profiles control routing through:

1. **Flags**: Which vehicle types can use a road (car, bike, pedestrian, etc.)
2. **Road Profiles**: Which item types are allowed and their speeds
3. **Route Value**: Segments that don't match the profile get `INT_MAX` cost (excluded)

**Example Hiking Profile** (would need to be added to navit.xml):
```xml
<vehicleprofile name="hiking_safe" flags="0x80000000" 
                flags_forward_mask="0x80000000" 
                flags_reverse_mask="0x80000000"
                maxspeed_handling="1" route_mode="0" 
                static_speed="3" static_distance="10">
    <roadprofile item_types="footway,path,track,steps,bridleway" 
                 speed="5" route_weight="10"/>
    <roadprofile item_types="street_0,street_1_city,living_street" 
                 speed="5" route_weight="20"/>
    <!-- Note: No motorway, trunk, or primary roads listed -->
</vehicleprofile>
```

## Summary

**Current State**:
- ✅ Route validation framework exists
- ✅ Forbidden highway definitions in place
- ✅ Priority path definitions in place
- ❌ Route validation not fully implemented (placeholder)
- ❌ No active prevention of unsafe routes
- ❌ No vehicle profile integration
- ❌ No cycling-specific validation

**To Fully Implement**:
1. Complete route segment analysis in validator
2. Create hiking/cycling vehicle profiles
3. Integrate profile switching with plugin
4. Add visual feedback for route safety

The plugin currently provides **post-route analysis** but does **not actively prevent** routing on unsafe roads. Users should use Navit's built-in "pedestrian" or "bike" vehicle profiles for safer routing.
