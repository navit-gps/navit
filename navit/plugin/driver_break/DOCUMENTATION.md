# Rest Plugin Complete Documentation

## Table of Contents

1. [Overview](#overview)
2. [Features](#features)
3. [Route Safety for Hikers and Cyclists](#route-safety-for-hikers-and-cyclists)
4. [Installation](#installation)
5. [Configuration](#configuration)
6. [Usage](#usage)
7. [API Reference](#api-reference)
8. [Architecture](#architecture)
9. [Testing](#testing)
10. [Troubleshooting](#troubleshooting)

## Overview

The Navit Vehicle Pause and Rest Stop Plugin is a comprehensive solution for managing driver rest periods and finding suitable rest locations. It supports both personal vehicles (cars) and commercial vehicles (trucks), with full compliance with EU Regulation EC 561/2006 for commercial transport.

### Key Capabilities

- **Automatic Driving Time Tracking**: Monitors continuous and total daily driving time
- **Mandatory Break Detection**: Alerts when breaks are legally required
- **Intelligent Rest Stop Discovery**: Finds suitable locations based on OSM criteria
- **POI Integration**: Discovers nearby amenities and attractions
- **History Management**: Tracks rest stop visits for analysis
- **Configurable Limits**: Customizable for different vehicle types and regulations

## Route Safety for Hikers and Cyclists

The plugin handles safe and unsafe roads for hikers and cyclists through:

### Current Implementation

1. **Post-Route Validation**: Validates calculated routes to detect unsafe road usage
2. **Rest Stop Filtering**: Only suggests rest stops at safe locations
3. **Warning System**: Generates warnings when routes use forbidden highways

### Forbidden Highways

**For Hikers**:
- `motorway` (highways)
- `trunk` (major roads)
- `primary` (primary roads)
- `primary_link`, `motorway_link`, `trunk_link` (road links)

**For Cyclists**:
- Similar restrictions (motorways excluded)
- Some primary roads may be acceptable with bike lanes

### Safe Paths (Priority)

**For Hikers**:
- `footway` (dedicated footpaths)
- `path` (general paths)
- `track` (unpaved tracks)
- `steps` (stairs)
- `bridleway` (shared with horses)

**For Cyclists**:
- `cycleway` (dedicated bike paths)
- `path` (shared paths)
- `track` (unpaved tracks suitable for bikes)

### Limitations

**Current Status**:
- ✅ Route validation framework exists
- ✅ Forbidden highway definitions in place
- ❌ Route validation not fully implemented (requires way tag access)
- ❌ No active prevention during route calculation
- ❌ No vehicle profile integration

**Workaround**: Use Navit's built-in vehicle profiles:
- "pedestrian" profile for hiking (excludes motorways)
- "bike" profile for cycling (excludes motorways)

**Future Implementation**:
- Complete route segment analysis
- Vehicle profile integration for active filtering
- Visual route safety indicators
- Cycling-specific validation rules

See [ROUTE_SAFETY.md](ROUTE_SAFETY.md) for detailed technical information.

## Features

### Driving Time Management

The plugin continuously tracks:
- **Continuous Driving Time**: Time since last break
- **Total Daily Driving**: Cumulative driving time for the day
- **Break History**: Record of all breaks taken

### Vehicle Type Support

#### Cars
- Soft limit: 5-9 hours/day (default: 7 hours)
- Maximum: 10-12 hours/day (default: 10 hours)
- Recommended breaks: Every 4.5 hours (default: 4 hours)
- Break duration: 15-45 minutes (default: 30 minutes)

#### Trucks (EU Regulation EC 561/2006)
- **Mandatory Break**: 45 minutes after 4.5 hours of continuous driving
- **Maximum Daily Driving**: 9 hours
- **Break Splitting**: Can split 45-minute break into 15 + 30 minutes
- **Daily Rest**: 11 hours (can be reduced to 9 hours twice per week)

### Rest Stop Discovery

The plugin uses sophisticated algorithms to find suitable rest locations:

**Location Criteria:**
- Intersections between specific highway types
- Minimum 150 meters from buildings
- Excluded from restricted landuse areas
- Accessible from current route

**POI Discovery:**
- Searches within 15 km radius
- Categories: Food & Drink, Cultural, Natural & Scenic
- Ranked by distance, preferences, and accessibility

## Installation

### Dependencies

**Required:**
- SQLite3 (libsqlite3-dev on Debian/Ubuntu)
- GLib 2.0

**Optional:**
- libcurl (for POI discovery via Overpass API)

### Build

The plugin is built by default. To disable:

```bash
cmake -Dplugin/rest=OFF ..
```

To enable tests:

```bash
cmake -DBUILD_REST_TESTS=ON ..
make test_rest_db test_rest_config test_rest_finder
```

## Configuration

### XML Configuration

Add to `navit.xml`:

```xml
<osd type="rest" enabled="yes" data="/path/to/rest.db">
    <attr name="type" value="car"/>
</osd>
```

**Attributes:**
- `data`: Database file path (optional, defaults to user data directory)
- `type`: Vehicle type - "car" or "truck" (default: "car")

### Programmatic Configuration

Configuration can be modified at runtime through the database or menu commands. Settings persist automatically.

## Usage

### Automatic Operation

The plugin operates automatically:
1. Tracks vehicle position via callbacks
2. Calculates driving time continuously
3. Detects when mandatory breaks are required
4. Suggests rest stops when needed
5. Discovers POIs near suggested locations

### Menu Commands

Access via Navit menu:

- **Suggest Rest Stop**: Shows dialog with rest stop suggestions
- **Rest Stop History**: Displays past rest stop visits
- **Configure**: Opens configuration menu
- **Start Break**: Records break start time
- **End Break**: Records break end and saves to history

### Manual Break Recording

Users can manually record breaks:

```xml
<!-- In menu XML -->
<a onclick='rest_start_break()'><text>Start Break</text></a>
<a onclick='rest_end_break()'><text>End Break</text></a>
```

## API Reference

### Core Data Structures

See [README.md](README.md#api-documentation) for complete API reference.

### Database Schema

**rest_stop_history:**
- `id`: Primary key (INTEGER)
- `timestamp`: Unix timestamp (INTEGER)
- `latitude`: Rest stop latitude (REAL)
- `longitude`: Rest stop longitude (REAL)
- `name`: Rest stop name (TEXT)
- `duration_minutes`: Break duration (INTEGER)
- `was_mandatory`: Mandatory break flag (INTEGER)

**rest_config:**
- `key`: Configuration key (TEXT, PRIMARY KEY)
- `value`: Configuration value (TEXT)

## Architecture

### Component Structure

```
rest.c (Main Plugin)
├── rest_db.c (Database Operations)
├── rest_finder.c (Rest Stop Discovery)
├── rest_poi.c (POI Discovery)
└── rest_osd.c (UI Integration)
```

### Data Flow

```
Vehicle Position → Driving Time Calculation
                        ↓
              Mandatory Break Check
                        ↓
              Rest Stop Discovery
                        ↓
              POI Discovery
                        ↓
              User Dialog
                        ↓
              History Storage
```

### Callback System

The plugin registers:
- **Vehicle Callback**: Position updates trigger driving time checks
- **Route Callback**: Route changes trigger rest stop discovery
- **Timeout Event**: Periodic checks for break requirements

## Testing

### Unit Tests

Three test suites are available:

1. **Database Tests** (`test_rest_db.c`)
   - Database creation/destruction
   - History operations
   - Configuration persistence

2. **Configuration Tests** (`test_rest_config.c`)
   - Default values
   - Break calculations
   - EU Regulation compliance

3. **Finder Tests** (`test_rest_finder.c`)
   - Coordinate validation
   - Distance calculations
   - Memory management

### Running Tests

```bash
cd build
make test_rest_db test_rest_config test_rest_finder
./navit/plugin/rest/tests/test_rest_db
./navit/plugin/rest/tests/test_rest_config
./navit/plugin/rest/tests/test_rest_finder
```

### Test Results

All tests pass successfully:
- Database tests: 5/5 passed
- Configuration tests: 6/6 passed
- Finder tests: 5/5 passed

See [tests/TEST_RESULTS.md](tests/TEST_RESULTS.md) for detailed results.

## Troubleshooting

### Plugin Not Loading

**Symptoms:** Plugin doesn't appear in Navit

**Solutions:**
1. Check plugin is enabled in CMakeLists.txt
2. Verify SQLite3 is installed
3. Check Navit logs for error messages
4. Ensure plugin is listed in navit.xml

### Database Errors

**Symptoms:** Database operations fail

**Solutions:**
1. Check database file permissions
2. Verify SQLite3 library is available
3. Check disk space
4. Review database path in configuration

### POI Discovery Not Working

**Symptoms:** No POIs found near rest stops

**Solutions:**
1. Verify libcurl is installed
2. Check network connectivity
3. Review Overpass API status
4. Check POI search radius in configuration

### Mandatory Breaks Not Detected

**Symptoms:** No alerts for required breaks

**Solutions:**
1. Verify vehicle type is set correctly (truck vs car)
2. Check driving time calculation is working
3. Review break interval configuration
4. Ensure vehicle callbacks are registered

### Performance Issues

**Symptoms:** Plugin slows down Navit

**Solutions:**
1. Reduce POI search radius
2. Limit history entries (clear old data)
3. Disable POI discovery if not needed
4. Check database file size

## Best Practices

1. **Regular Database Maintenance**: Clear old history entries periodically
2. **Appropriate Configuration**: Set vehicle type correctly for accurate break detection
3. **Network Usage**: POI discovery requires internet - consider caching
4. **Break Recording**: Use manual break commands for accurate history
5. **Testing**: Run unit tests after configuration changes

## Future Enhancements

Planned improvements:
- Asynchronous POI discovery
- User preference system for POI ranking
- Operating hours integration
- User reviews integration
- Visual map indicators
- Route integration for automatic suggestions
- Multiple EU member state regulation support

## Support

For issues, questions, or contributions:
- See main Navit documentation
- Check plugin README.md
- Review test documentation
- Consult Navit community forums

## License

This plugin is part of Navit and is licensed under the GNU Library General Public License version 2.
