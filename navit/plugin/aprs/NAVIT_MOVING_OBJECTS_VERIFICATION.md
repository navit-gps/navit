# Navit Moving Objects Capability Verification

## Executive Summary

**VERIFIED**: Navit fully supports moving object tracking and real-time dynamic updates through its map plugin architecture.

## Verification Results

### 1. Native Moving Object Support

**Status**: CONFIRMED

**Evidence**:
- The `traffic.c` plugin demonstrates dynamic item creation, update, and removal
- Map plugins can maintain in-memory item lists that are queried dynamically
- Items are returned through `map_rect_get_item()` which queries current state
- Reference: `navit/traffic.c` lines 145-189, 887-1066

**Implementation Pattern**:
```c
struct map_priv {
    GList * items;  // Dynamic list of items
    // ... other data
};

static struct item * map_rect_get_item(struct map_rect_priv *mr) {
    // Returns next item from current state
    // Items can be added/removed/updated between calls
}
```

### 2. Dynamic Point-of-Interest Updates

**Status**: CONFIRMED

**Evidence**:
- Items can be added to map dynamically via `tm_add_item()` in traffic plugin
- Items can be removed by setting type to `type_none`
- Items can be updated by modifying underlying data structures
- Reference: `navit/traffic.c` lines 887-1066

**Update Mechanism**:
- Map callbacks notify Navit when map data changes
- `map_add_callback()` / `map_remove_callback()` for change notifications
- Reference: `navit/map.c` lines 161-176

### 3. Real-time Coordinate Rendering

**Status**: CONFIRMED

**Evidence**:
- Map rectangles query items on-demand during rendering
- Each `map_rect_get_item()` call returns current item state
- No caching prevents real-time updates
- Reference: `navit/track.c` lines 1077-1121

**Rendering Flow**:
1. Navit requests items via `map_rect_get_item()`
2. Plugin returns items from current in-memory state
3. Items include current coordinates and attributes
4. Map redraws when callbacks are triggered

### 4. Plugin/Extension Mechanism

**Status**: CONFIRMED

**Evidence**:
- Map plugins register via `plugin_register_category_map()`
- Plugins implement `struct map_methods` interface
- Reference: `navit/plugin.h` lines 136-140
- Example implementations: `traffic.c`, `track.c`, `route.c`

**Registration Pattern**:
```c
void plugin_init(void) {
    plugin_register_category_map("aprs", aprs_map_new);
}
```

### 5. D-Bus and Socket-Based Communication

**Status**: CONFIRMED

**Evidence**:
- Full D-Bus support in `navit/binding/dbus/binding_dbus.c`
- D-Bus service: `org.navit_project.navit`
- Can receive commands and send signals
- Socket-based vehicle plugins exist (e.g., `vehicle_file.c`)
- Reference: `navit/binding/dbus/binding_dbus.c` lines 1-2265

**Communication Options**:
1. **D-Bus**: Full integration for external control
2. **Sockets**: File/socket vehicle plugins demonstrate pattern
3. **Internal Callbacks**: Direct function calls for performance

## Recommended Implementation Approach for APRS Plugin

### Architecture

1. **Map Plugin**: Implement as map plugin (like traffic plugin)
   - Provides items representing APRS stations
   - Items updated dynamically as packets arrive

2. **Data Storage**: SQLite database for station persistence
   - Station metadata (callsign, position, timestamp)
   - Station expiration tracking
   - Range filtering support

3. **Packet Decoder**: Separate module for APRS/AX.25 decoding
   - Bell 202 / 2FSK demodulation (can use multimon-ng or custom)
   - AX.25 frame parsing
   - APRS position extraction

4. **Update Mechanism**:
   - Event-driven updates when packets arrive
   - Map callbacks to trigger redraws
   - Periodic expiration cleanup

### Item Type Selection

- Use `type_poi` or custom POI type for APRS stations
- Items include:
  - Coordinates (lat/lng)
  - Station callsign (attr_label)
  - Timestamp (attr_data)
  - Symbol/icon (attr_icon)

### Performance Considerations

- Efficient item lookup (hash table by callsign)
- Batch updates to minimize redraws
- Configurable update intervals
- Range-based filtering before item creation

## Conclusion

Navit's architecture fully supports the APRS tracking plugin requirements:
- Dynamic object creation and updates
- Real-time coordinate rendering
- Plugin-based extensibility
- Multiple communication mechanisms (D-Bus, sockets, callbacks)

The traffic plugin (`navit/traffic.c`) serves as an excellent reference implementation for dynamic map items.

