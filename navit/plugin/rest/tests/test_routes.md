# Test Routes Documentation

## Overview

This document describes the test routes used for validation of the Rest plugin routing functionality with different vehicle profiles.

**Note**: These routes are defined for integration testing. Unit tests validate the routing logic and coordinate handling, while full route calculation requires actual map data and Navit's routing engine.

## Hiking Route: Bjøberg → Bjordalsbu → Aurlandsdalen Turisthytte

### Route Information

- **Route Type**: Hiking
- **Vehicle Profile**: Pedestrian/Hiking
- **Total Distance**: ~22 km (estimated)
- **Elevation Gain**: ~800 meters
- **Expected Duration**: 6-8 hours

### Waypoints

1. **Start: Bjøberg**
   - OSM Node ID: 1356457581
   - Approximate Coordinates: 60.8°N, 7.2°E
   - Elevation: ~800 m
   - Location: Norway, Vestland county

2. **Via: Bjordalsbu**
   - OSM Node ID: 754402416
   - Approximate Coordinates: 60.9°N, 7.3°E
   - Elevation: ~1200 m
   - Location: Mountain hut/cabin

3. **End: Aurlandsdalen Turisthytte**
   - OSM Node ID: 1356459524
   - Approximate Coordinates: 61.0°N, 7.4°E
   - Elevation: ~900 m
   - Location: Tourist hut in Aurlandsdalen valley

### Expected Route Characteristics

- **Road Types**: Should use footways, paths, tracks, hiking trails
- **Forbidden Roads**: No motorways, trunk, or primary roads
- **Priority Paths**: High percentage of footways, paths, tracks
- **Route Validation**: 
  - Forbidden highways: 0%
  - Priority paths: >80%
  - Valid for hiking: Yes

### Rest Stop Suggestions

Based on hiking profile (11.295 km main rest, 2.275 km alternative):
- Rest stop 1: ~11.3 km (near Bjordalsbu)
- Rest stop 2: ~22.6 km (near destination)

### POI Discovery

Expected POIs along route:
- Water points: Natural springs, streams
- Cabins: Bjordalsbu, Aurlandsdalen Turisthytte
- DNT/Network huts: Possible DNT huts in area

## Car Route: St1 Moelv → Spidsbergseterkrysset → Enden → Aukrustsenteret

### Route Information

- **Route Type**: Car
- **Vehicle Profile**: Car
- **Total Distance**: ~44 km (estimated)
- **Expected Duration**: 45-60 minutes

### Waypoints

1. **Start: St1 Moelv**
   - OSM Node ID: 8233820034
   - Approximate Coordinates: 61.0°N, 10.7°E
   - Location: Fuel station in Moelv, Norway

2. **Via 1: Spidsbergseterkrysset**
   - OSM Node ID: 10677676828
   - Approximate Coordinates: 61.1°N, 10.8°E
   - Location: Road junction

3. **Via 2: Enden**
   - OSM Node ID: 1289150990
   - Approximate Coordinates: 61.2°N, 10.9°E
   - Location: Village/area

4. **End: Aukrustsenteret**
   - Approximate Coordinates: 61.3°N, 11.0°E
   - Location: Cultural center/museum

### Expected Route Characteristics

- **Road Types**: May include motorways, trunk, primary, secondary roads
- **Forbidden Roads**: None (all road types allowed for cars)
- **Route Validation**:
  - Forbidden highways: 0% (motorways allowed for cars)
  - Valid for car: Yes

### Rest Stop Suggestions

Based on car profile (4 hour break interval):
- Rest stop: Not required for 44 km route (< 1 hour driving)

### POI Discovery

Expected POIs along route:
- Fuel stations: St1 Moelv (start)
- Cafes/Restaurants: Possible along route
- Attractions: Aukrustsenteret (destination)

## Test Execution

### Prerequisites

1. **Map Data**: OSM-based map covering Norway (Vestland and Innlandet counties)
2. **SRTM Data**: HGT tiles for Norway (N60-N62, E007-E011)
3. **Vehicle Profiles**: Pedestrian and Car profiles configured

### Running Route Tests

```bash
# With actual Navit instance and map data
navit --config=test_routes.xml

# Or via Navit API
route_calculate(start_coord, via_coords[], end_coord, vehicle_profile)
route_validate(route, vehicle_type)
```

### Expected Test Results

#### Hiking Route
- Route calculated successfully
- Route uses pedestrian profile
- Route validation: 0% forbidden highways
- Route validation: >80% priority paths
- Rest stops calculated at ~11.3 km intervals
- POI discovery finds water points and cabins

#### Car Route
- Route calculated successfully
- Route uses car profile
- Route validation: 0% forbidden highways (motorways allowed)
- Rest stops: Not required (short route)
- POI discovery finds fuel stations and attractions

## Integration with Rest Plugin

### Hiking Route Integration

The Rest plugin should:
1. Detect vehicle type as `REST_VEHICLE_HIKING`
2. Calculate rest stops every 11.295 km (or 2.275 km alternative)
3. Validate route against forbidden highways
4. Discover POIs (water points, cabins) near rest stops
5. Check for DNT/network huts if DNT priority enabled
6. Use SRTM elevation data for elevation profile

### Car Route Integration

The Rest plugin should:
1. Detect vehicle type as `REST_VEHICLE_CAR`
2. Monitor driving time (not needed for 44 km route)
3. Validate route (all roads allowed for cars)
4. Discover POIs (fuel stations, cafes) if needed
5. Use SRTM elevation data for elevation profile

## Notes

- OSM Node IDs are provided for reference
- Actual coordinates should be resolved from OSM data
- Route calculation requires full Navit routing engine
- Elevation data requires SRTM HGT tiles for Norway region
- POI discovery requires map data with POI items or Overpass API access
