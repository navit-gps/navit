# Navit Routing and Elevation Handling

## Routing Algorithm

### Overview

Navit uses the **Lifelong Planning A* (LPA*)** algorithm for route calculation. This is an evolution of the A* algorithm, which itself extends Dijkstra's algorithm.

### Key Characteristics

1. **Backward Routing**: Routes are calculated from destination to start position
   - Makes it easier to react to position changes
   - Graph is reusable when vehicle position updates
   - Destination remains fixed while start position moves

2. **Route Graph Structure**:
   - **Points**: Nodes in the graph, typically at intersections or segment endpoints
   - **Segments**: Edges representing driveable paths (streets, highways, etc.)
   - Segments can represent entire map items or parts of segmented items

3. **Graph Building Process**:
   ```
   1. Calculate map selection rectangles around waypoints
   2. Query map for road segments in these rectangles
   3. Build route graph from segments
   4. Initialize destination point candidates
   5. Flood graph (assign costs to all points)
   6. Extract route path from position to destination
   ```

### Map Selection Strategy

Navit calculates multiple rectangles for route graph building:

- **Main rectangle**: Includes position and destination with 25% padding (order 4)
- **Large squares**: ~80 km around each waypoint (order 8)
- **Small squares**: ~20 km around each waypoint (order 18)

The "order" determines which tile level roads are selected from. Higher order = more detailed roads.

### Cost Calculation

Route costs are calculated in **tenths of seconds** (time-based, not distance-based):

```c
time = (segment_length * conversion_factor * 10) / speed + traffic_delay
```

Where:
- `segment_length`: Length of the road segment in meters
- `speed`: Maximum speed for the segment (from road profile or traffic)
- `traffic_delay`: Additional delay from traffic distortions (in tenths of seconds)

### Route Value Calculation

The `route_value_seg()` function determines if a segment can be traversed and its cost:

**Factors considered:**
- **Direction restrictions**: One-way streets, turn restrictions
- **Vehicle profile flags**: Which road types are allowed
- **Traffic distortions**: Dynamic delays from traffic module
- **Through traffic penalties**: Penalties for certain road types
- **Segment length and speed**: Time to traverse

**NOT considered:**
- Elevation/gradient
- Road surface quality
- Weather conditions
- Real-time speed limits (only static maxspeed from map data)

### LPA* Advantages

1. **Incremental Updates**: When segment costs change (e.g., traffic updates), only affected parts of the graph are recalculated
2. **Dynamic Traffic**: Traffic module can update segment costs without full recalculation
3. **Efficiency**: Uses Fibonacci heaps for efficient minimum extraction

### Vehicle Profiles

Routing behavior is controlled by **vehicle profiles** which define:
- Which road types are accessible
- Speed limits for different road types
- Turn penalties
- Through traffic preferences
- Size/weight restrictions (for trucks)

## Elevation Handling

### Current Implementation

**Elevation is NOT used in routing calculations.** Navit treats routing as a 2D problem based on:
- Road network topology
- Distance
- Speed limits
- Traffic conditions

### Elevation Display

Elevation data is used **only for visualization**, not routing:

1. **Heightlines Map**: Special map file (`*.heightlines.bin`) containing contour lines
   - Three types: `height_line_1` (major), `height_line_2` (medium), `height_line_3` (minor)
   - Extracted from OSM data with `contour_ext=elevation_*` tags

2. **Elevation Profile**: Can display elevation profile along a route
   - Requires heightlines map to be installed
   - Calculates intersections between route path and contour lines
   - Shows elevation vs. distance diagram

3. **Visualization**: Heightlines are rendered on the map as:
   - Polylines with different widths based on zoom level
   - Text labels showing elevation values
   - Separate layer that can be toggled

### Elevation Data Sources

- **OSM Contour Data**: From OpenStreetMap with elevation tags
- **Heightlines Map**: Processed into binary format by maptool
- **Map Format**: Stored as `height_line_1`, `height_line_2`, `height_line_3` item types

### Elevation Profile Generation

When displaying route elevation profile:

1. Route path coordinates are extracted
2. Heightlines map is queried for contour lines intersecting the route
3. Intersection points are calculated
4. Elevation values are extracted from contour line attributes
5. Profile diagram is generated showing elevation vs. distance

**Code location**: `navit/gui/internal/gui_internal_command.c` (elevation profile command)

### Limitations

1. **No Gradient Consideration**: Steep hills don't affect route selection
2. **No Elevation-Based Routing**: Routes don't avoid steep climbs
3. **Display Only**: Elevation is purely visual
4. **Optional Feature**: Requires separate heightlines map file

## Implications for Rest Stop Plugin

### Routing Integration

The rest stop plugin can:
- Use `route_get_map()` to access route segments
- Iterate through route path to find suitable rest stop locations
- Calculate distances along route using route segment lengths
- Access route graph for more detailed analysis

### Elevation Considerations

If you want to consider elevation for rest stops:

1. **Would need to**:
   - Query heightlines map for elevation data
   - Calculate gradients along route segments
   - Prefer locations with moderate gradients
   - Avoid very steep sections

2. **Current limitation**: Elevation is not factored into routing, so:
   - Routes may include steep sections
   - Rest stops might be suggested on difficult terrain
   - No automatic avoidance of elevation changes

3. **Potential enhancement**: Could add elevation-based filtering:
   ```c
   // Pseudo-code for elevation-aware rest stop selection
   double gradient = calculate_gradient(segment_start, segment_end, heightlines);
   if (gradient > MAX_GRADIENT_THRESHOLD) {
       // Prefer alternative locations
   }
   ```

## Technical Details

### Route Graph Structure

```c
struct route_graph {
    struct route_graph_point *hash[HASH_SIZE];  // Points by hash
    struct fibheap *heap;                        // For LPA* algorithm
    // ...
};

struct route_graph_segment {
    struct route_graph_point *start, *end;      // Endpoints
    struct route_segment_data data;             // Road data
    int len;                                     // Length in meters
    // ...
};
```

### Cost Calculation Flow

```
route_value_seg()
  ↓
route_time_seg()
  ↓
route_seg_speed()  (from vehicle profile)
  ↓
time = (len * conversion) / speed + traffic_delay
```

### Elevation Data Structure

```c
struct heightline {
    struct coord *c;        // Coordinates of contour line
    int count;              // Number of coordinates
    int height;             // Elevation value
    struct coord_rect bbox; // Bounding box
    // ...
};
```

## Summary

- **Routing**: Uses LPA* algorithm, time-based costs, backward calculation, supports dynamic traffic updates
- **Elevation**: Display-only feature, not used in routing decisions, requires separate heightlines map
- **Integration**: Rest stop plugin can access route data but elevation would need custom implementation

For the rest stop plugin, this means:
- Route segments are available for analysis
- Distance calculations work well
- Elevation-aware features would require additional implementation
- Current routing doesn't consider terrain difficulty
