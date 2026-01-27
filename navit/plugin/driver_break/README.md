# Navit Vehicle Pause and Rest Stop Plugin


## Overview

The Rest plugin provides dynamic rest period management for vehicles, with support for both cars and trucks. It tracks driving time, suggests rest stops based on OpenStreetMap data, discovers nearby Points of Interest (POIs), and ensures compliance with EU Regulation EC 561/2006 for commercial vehicles.

## Route Safety for Hikers and Cyclists

**Current Status**: The plugin provides **post-route validation** to check if calculated routes use unsafe roads (motorways, trunk, primary roads), but does **not actively prevent** routing on these roads during route calculation.

**How It Works**:
- **Route Validation**: After a route is calculated, the plugin can validate it against forbidden highways
- **Warnings**: If unsafe roads are detected, warnings are generated
- **Rest Stop Filtering**: Rest stops are only suggested at safe locations (excludes motorways, trunk, primary)

**For Full Safety**: Use Navit's built-in vehicle profiles:
- Select "pedestrian" profile for hiking (excludes motorways)
- Select "bike" profile for cycling (excludes motorways)

**Forbidden Highways for Hikers**:
- `motorway`, `trunk`, `primary` and their links
- These are checked during route validation

**Priority Paths (Safe)**:
- `footway`, `path`, `track`, `steps`, `bridleway`

See [ROUTE_SAFETY.md](ROUTE_SAFETY.md) for detailed information.

## Features

### DNT/Network and Pilgrimage Route Support

The plugin fully supports DNT (Den Norske Turistforening) and network huts, as well as pilgrimage routes:

- **Network Detection**: Automatically detects DNT, STF, and other network-operated cabins via OSM tags
- **DNT Priority Mode**: When enabled, uses 25 km search radius for network huts and adjusts daily distance to 46.4 km
- **Pilgrimage Routes**: Detects and prioritizes `route=hiking` and `pilgrimage=yes` paths
- **Map-Based POI Discovery**: Uses direct map queries (preferred) instead of unreliable Overpass API

See [DNT_NETWORK_PILGRIMAGE.md](DNT_NETWORK_PILGRIMAGE.md) for details.

## Features

### Core Functionality

- **Driving Time Tracking**: Monitors continuous driving time and total daily driving hours
- **Rest Stop Discovery**: Finds suitable rest locations along routes based on OSM criteria
- **POI Discovery**: Discovers nearby amenities, restaurants, attractions (15 km radius for general POIs, 2 km for water points, 5 km for cabins/huts)
- **EU Regulation Compliance**: Enforces mandatory break rules for trucks (EC 561/2006)
- **History Tracking**: SQLite database stores rest stop history
- **Configurable Settings**: Customizable rest intervals, break durations, and vehicle-specific limits

### Vehicle Types

#### Cars
- Soft limit: 5-9 hours per day (configurable, default 7 hours)
- Maximum: 10-12 hours per day (configurable, default 10 hours)
- Recommended break: Every 4.5 hours (configurable, default 4 hours)
- Break duration: 15-45 minutes (configurable, default 30 minutes)

#### Trucks (EU Regulation EC 561/2006)
- Mandatory break: 45 minutes after 4.5 hours of driving
- Maximum daily driving: 9 hours
- Predictive break scheduling
- Cross-border regulation support

## Rest Stop Location Criteria

The plugin finds rest stops at intersections between specific road types:
- `highway=unclassified`
- `highway=service`
- `highway=track`
- `highway=rest_area`
- `highway=tertiary`

### Exclusion Rules

Rest stops are automatically excluded from:
- `landuse=farmland`
- `landuse=residential`
- `landuse=commercial`
- `landuse=industrial`
- `landuse=military`
- `landuse=recreation_ground`

Minimum distance from buildings: 150 meters (configurable)

## POI Categories

The plugin discovers POIs with different search radii depending on type:
- **General POIs** (cafes, restaurants, museums, etc.): 15 km radius (configurable)
- **Water points**: 2 km radius (configurable)
- **Cabins/huts**: 5 km radius (configurable, 25 km when DNT priority enabled)

### Food & Drink
- Cafes (`amenity=cafe`)
- Restaurants (`amenity=restaurant`)
- Bars/Pubs (`amenity=bar`, `amenity=pub`)
- Breweries (`craft_brewery=yes`)

### Cultural & Educational
- Museums (`tourism=museum`)
- Art Galleries (`tourism=gallery`)
- Attractions (`tourism=attraction`)
- Zoos (`tourism=zoo`)
- Information Centers (`tourism=information`)

### Natural & Scenic
- Viewpoints (`tourism=viewpoint`)
- Picnic Sites (`leisure=picnic_table`)
- Scenic Overlooks

## Installation

### Dependencies

- **SQLite3**: For rest stop history storage
  - Debian/Ubuntu: `apt-get install libsqlite3-dev`
  - Fedora: `dnf install sqlite-devel`

- **libcurl**: For POI discovery via Overpass API fallback (optional, only needed if map data is unavailable)
  - Debian/Ubuntu: `apt-get install libcurl4-openssl-dev`
  - Fedora: `dnf install libcurl-devel`

### Build

The plugin is built by default when Navit is compiled. To disable it:

```bash
cmake -Dplugin/rest=OFF ..
```

## Configuration

### XML Configuration

Add the plugin to your `navit.xml`:

```xml
<osd type="rest" enabled="yes" data="/path/to/rest_plugin.db">
    <attr name="type" value="car"/>
</osd>
```

For trucks:

```xml
<osd type="rest" enabled="yes" data="/path/to/rest_plugin.db">
    <attr name="type" value="truck"/>
</osd>
```

### Configuration Attributes

- `data`: Path to SQLite database file (optional, defaults to user data directory)
- `type`: Vehicle type - "car", "truck", "hiking", or "cycling" (default: "car")

### Customizing Settings

Settings can be customized through the plugin's configuration menu or by modifying the database directly. The plugin saves configuration automatically.

**Note**: Currently, changing configuration values through the UI is not yet implemented. The configuration dialogs display current values but do not allow editing. To modify settings, edit the database directly using SQLite tools.

## Usage

### Menu Commands

The plugin provides the following menu commands:

- `rest_suggest_stop()`: Show rest stop suggestions (placeholder - not yet implemented)
- `rest_show_history()`: Display rest stop history (placeholder - not yet implemented)
- `rest_configure_intervals()`: Configure rest stop intervals (based on selected vehicle profile)
- `rest_configure_overnight()`: Configure overnight stop settings (based on selected vehicle profile)
- `rest_start_break()`: Record break start (placeholder - not yet implemented)
- `rest_end_break()`: Record break end and save to history (placeholder - not yet implemented)

**Note**: The configuration commands (`rest_configure_intervals` and `rest_configure_overnight`) are fully implemented and show dialogs with current settings. Other commands are placeholders for future implementation.

### Automatic Operation

The plugin automatically:
- Tracks driving time when vehicle is moving
- Checks for mandatory breaks based on vehicle type
- Suggests rest stops when mandatory break is required
- Discovers POIs near suggested rest stops

## Database Schema

The plugin uses SQLite with the following tables:

### rest_stop_history
- `id`: Primary key
- `timestamp`: Unix timestamp
- `latitude`: Rest stop latitude
- `longitude`: Rest stop longitude
- `name`: Rest stop name (optional)
- `duration_minutes`: Break duration
- `was_mandatory`: 1 if mandatory break, 0 otherwise

### rest_config
- `key`: Configuration key
- `value`: Configuration value (stored as text)

## API Integration

### POI Discovery Methods

The plugin uses **map-based POI discovery** as the primary method (preferred), with Overpass API as a fallback:

**Primary Method: Map-Based Discovery**
- Uses direct queries to Navit's loaded map data
- More reliable and faster than API calls
- No network dependency
- Works with any OSM-based map format

**Fallback Method: Overpass API**
- Used only when map data is not available
- Endpoint: `https://overpass-api.de/api/interpreter`
- Timeout: 30 seconds
- Query format: Overpass QL
- Requires libcurl

### Graceful Degradation

If map data is not available and Overpass API is unavailable or libcurl is not installed:
- Rest stop discovery still works
- POI discovery is disabled
- User is notified via debug messages

## Performance

- Minimal computational overhead
- Asynchronous POI discovery (planned)
- Caching of frequently accessed data (planned)
- Database queries optimized with indexes

## Logging

The plugin logs events at various levels:
- `lvl_info`: Important events (break required, rest stops found)
- `lvl_debug`: Detailed operation (POI discovery, route analysis)
- `lvl_error`: Errors (database issues, API failures)

## Future Enhancements

- User preference system for POI ranking
- Operating hours integration
- User reviews integration
- Accessibility features filtering
- Multiple EU member state regulation support
- Visual indicators for recommended/mandatory breaks
- Route integration for automatic rest stop suggestions

## Unit Tests

Comprehensive unit tests are available in the `tests/` directory:

- `test_rest_db.c` - Database operation tests
- `test_rest_config.c` - Configuration and driving time calculation tests
- `test_rest_finder.c` - Rest stop finder tests

See `tests/TEST_RESULTS.md` for detailed test documentation and results.

Build and run tests:
```bash
cmake .. -DBUILD_REST_TESTS=ON
make test_rest_db test_rest_config test_rest_finder
./build/navit/plugin/rest/tests/test_rest_db
```

## API Documentation

### Core Structures

#### `struct rest_config`
Configuration structure containing:
- Vehicle type (car/truck)
- Car settings (soft limit, max hours, break intervals)
- Truck settings (EU Regulation EC 561/2006 compliance)
- Rest stop criteria (min distance, POI search radius)
- Rest interval range

#### `struct driving_session`
Tracks current driving session:
- Start time and last break time
- Total and continuous driving minutes
- Mandatory break flag
- Break count

#### `struct rest_stop`
Represents a rest stop location:
- Geographic coordinates
- Name and highway type
- Distance from route
- POI list
- Ranking score

#### `struct rest_poi`
Point of Interest near rest stop:
- Coordinates and name
- Type and category
- Distance from rest stop
- Opening hours and accessibility
- Rating (if available)

### Database API

#### `rest_db_new(const char *db_path)`
Creates a new database connection. Returns NULL on failure.

#### `rest_db_destroy(struct rest_db *db)`
Closes database connection and frees resources.

#### `rest_db_add_history(struct rest_db *db, struct rest_stop_history *entry)`
Adds a rest stop entry to history. Returns 1 on success, 0 on failure.

#### `rest_db_get_history(struct rest_db *db, time_t since)`
Retrieves history entries since specified timestamp. Returns GList of `struct rest_stop_history *`.

#### `rest_db_clear_history(struct rest_db *db, time_t before)`
Removes history entries older than specified timestamp. Returns 1 on success.

#### `rest_db_save_config(struct rest_db *db, struct rest_config *config)`
Saves configuration to database. Returns 1 on success.

#### `rest_db_load_config(struct rest_db *db, struct rest_config *config)`
Loads configuration from database. Returns 1 on success, 0 if no saved config.

### Rest Stop Finder API

#### `rest_finder_find_along_route(struct route *route, struct rest_config *config, int mandatory_break_required)`
Finds rest stops along a route. Returns GList of `struct rest_stop *`.

#### `rest_finder_find_near(struct coord_geo *center, double distance_km, struct rest_config *config)`
Finds rest stops near a coordinate. Returns GList of `struct rest_stop *`.

#### `rest_finder_is_valid_location(struct coord_geo *coord, struct rest_config *config)`
Validates if a location meets rest stop criteria. Returns 1 if valid, 0 otherwise.

#### `rest_finder_free_list(GList *stops)`
Frees a list of rest stops.

#### `rest_finder_free(struct rest_stop *stop)`
Frees a single rest stop structure.

### POI Discovery API

#### `rest_poi_discover(struct coord_geo *center, int radius_km, const char **poi_categories, int num_categories)`
Discovers POIs using map-based queries (preferred) or Overpass API (fallback). Returns GList of `struct rest_poi *`. Overpass API fallback requires libcurl.

#### `rest_poi_rank(GList *pois, struct coord_geo *rest_stop, struct rest_config *config)`
Ranks POIs by distance and preferences. Modifies list in-place.

#### `rest_poi_free_list(GList *pois)`
Frees a list of POIs.

#### `rest_poi_free(struct rest_poi *poi)`
Frees a single POI structure.

### Menu Commands

The plugin registers the following menu commands:

- `rest_suggest_stop()` - Show rest stop suggestions dialog (placeholder)
- `rest_show_history()` - Display rest stop history (placeholder)
- `rest_configure_intervals()` - Configure rest stop intervals (implemented)
- `rest_configure_overnight()` - Configure overnight stop settings (implemented)
- `rest_start_break()` - Record break start time (placeholder)
- `rest_end_break()` - Record break end and save to history (placeholder)

## Architecture

The plugin is structured as an OSD (On-Screen Display) plugin that:

1. **Tracks Vehicle Position**: Registers callbacks for vehicle position updates
2. **Monitors Driving Time**: Calculates continuous and total driving time
3. **Detects Mandatory Breaks**: Checks for mandatory breaks based on vehicle type
4. **Discovers Rest Stops**: Finds suitable locations when breaks are required
5. **Discovers POIs**: Finds nearby amenities using map-based queries (preferred) or Overpass API (fallback)
6. **Stores History**: Saves rest stop visits to SQLite database
7. **Provides UI**: Menu commands for user interaction

### Component Interaction

```
Vehicle Callback → Driving Time Check → Mandatory Break Detection
                                                      ↓
Route Callback → Rest Stop Finder → POI Discovery → User Dialog
                                                      ↓
Database ← History Storage ← Break End Command
```

## License

This plugin is part of Navit and is licensed under the GNU Library General Public License version 2.

## Contributing

See the main Navit CONTRIBUTING.md for guidelines on contributing to Navit plugins.

## Related Documentation

- [IMPLEMENTATION.md](IMPLEMENTATION.md) - Implementation details
- [tests/TEST_RESULTS.md](tests/TEST_RESULTS.md) - Unit test documentation
- [example_config.xml](example_config.xml) - Example configuration
