# APRS Plugin Architecture

## Overview

The APRS functionality is implemented as two separate, modular plugins that communicate through a well-defined interface. This separation provides flexibility, maintainability, and allows users to deploy only what they need.

## Plugin 1: aprs (Core Plugin)

**Location**: `navit/plugin/aprs/`

**Responsibilities:**
- APRS packet parsing (AX.25 frame decoding)
- Position extraction and validation
- SQLite database management
- Station tracking and expiration
- Map rendering (POI items)
- Menu/OSD interface
- NMEA serial input parsing
- Packet ingestion API for external sources

**Key Files:**
- `aprs.c` - Main plugin implementation
- `aprs.h` - Exports `aprs_process_packet()` and `aprs_update_items()`
- `aprs_db.c/h` - SQLite database operations
- `aprs_decode.c/h` - AX.25 and APRS position parsing
- `aprs_nmea.c/h` - NMEA sentence parsing
- `aprs_osd.c` - Menu interface
- `aprs_plugin_interface.h` - Inter-plugin communication API

**Dependencies:**
- SQLite3 (required)
- libnmea (optional, for NMEA parsing)
- Navit core

**Exports:**
- `aprs_process_packet()` - Process decoded AX.25 frames
- `aprs_register_packet_source()` - Register external packet sources
- `aprs_unregister_packet_source()` - Unregister packet sources

## Plugin 2: aprs_sdr (SDR Plugin)

**Location**: `navit/plugin/aprs_sdr/`

**Responsibilities:**
- RTL-SDR hardware initialization and control
- Device detection and enumeration
- Frequency tuning and gain control
- I/Q sample acquisition
- Bell 202 demodulation (2FSK at 1200 bps)
- AX.25 frame extraction
- Android USB Host API integration
- Delivering decoded frames to APRS plugin

**Key Files:**
- `aprs_sdr.c` - Main plugin implementation and inter-plugin communication
- `aprs_sdr_hw.c/h` - RTL-SDR hardware interface
- `aprs_sdr_dsp.c/h` - Bell 202 demodulation
- `aprs_sdr_android.c/h` - Android USB Host API integration

**Dependencies:**
- librtlsdr (required)
- pthread (for threading)
- Android USB Host API (Android only)

**Exports:**
- Map plugin category "aprs_sdr" (for configuration)
- No direct API exports (uses APRS plugin's registration API)

## Inter-Plugin Communication

### Discovery Mechanism

The SDR plugin discovers the APRS plugin using Navit's plugin system:

1. **Method 1: Plugin Category Lookup**
   ```c
   void *map_priv = plugin_get_category_map("aprs");
   ```
   This is the primary method - fast and reliable.

2. **Method 2: Symbol Lookup (Fallback)**
   ```c
   g_module_symbol(module, "aprs_register_packet_source", &func);
   ```
   Used if category lookup fails (e.g., plugin not yet initialized).

### Registration Process

1. SDR plugin calls `aprs_register_packet_source()` with its frame delivery callback
2. APRS plugin stores the callback and associated map instance
3. SDR plugin stores the map instance pointer for direct frame delivery

### Frame Delivery

When the SDR plugin decodes an AX.25 frame:

1. Bell 202 demodulator extracts frame from bitstream
2. Frame is validated (checksum, flags, etc.)
3. SDR plugin calls `aprs_process_packet()` directly on the APRS map instance
4. APRS plugin processes the frame through its normal pipeline:
   - AX.25 decoding
   - Position extraction
   - Database update
   - Map item creation/update

### Graceful Fallback

If the APRS plugin is not available:
- SDR plugin logs a warning
- Continues running (hardware initialized)
- Frames are discarded until APRS plugin loads
- No crashes or errors

If the SDR plugin is not available:
- APRS plugin continues normally
- Works with NMEA or other packet sources
- Menu shows SDR options as unavailable

## Configuration

### XML Configuration

Both plugins can be configured in `navit.xml`:

```xml
<!-- APRS Core Plugin -->
<map type="aprs" data="/path/to/aprs.db">
    <attribute name="timeout" value="5400"/>
    <attribute name="distance" value="150000"/>
</map>

<!-- SDR Plugin (optional) -->
<map type="aprs_sdr">
    <attribute name="frequency" value="144.390"/>
    <attribute name="gain" value="49"/>
    <attribute name="device" value="rtlsdr_blog_v3"/>
</map>
```

### Runtime Configuration

Both plugins support runtime configuration via GUI menu:
- **APRS Plugin Menu**: Frequency selection, timeout settings, device selection
- **SDR Plugin**: Automatically configured when APRS plugin settings change

## Initialization Order

Navit loads plugins in the order specified in the configuration. For proper operation:

1. **APRS plugin should load first** (or both can load simultaneously)
2. **SDR plugin discovers APRS plugin** during its initialization
3. If APRS plugin loads after SDR plugin, SDR plugin retries discovery

The discovery mechanism is robust and handles initialization order gracefully.

## Android Considerations

### USB Permissions

The SDR plugin handles Android USB permissions:
- Requests permission via Android USB Host API
- Handles permission denial gracefully
- Shows user-friendly error messages

### NDK Compilation

- SDR plugin's DSP code compiled via NDK
- Native threads for sample acquisition
- JNI bridge for Android USB API
- Bell 202 demodulation runs in native code for performance

### USB OTG Support

- Device must support USB OTG
- RTL-SDR dongles draw power from USB
- May need powered USB hub for stable operation
- See Android USB Serial section in README for chip recommendations

## Benefits of Separation

1. **Modularity**: Each plugin has single responsibility
2. **Flexibility**: Users can use APRS without SDR
3. **Maintainability**: Easier to update independently
4. **Deployment**: Smaller plugin sizes
5. **Testing**: Can test plugins independently
6. **Dependencies**: APRS plugin doesn't require librtlsdr

## Testing

Unit tests are provided for core components:

- Database operations: CRUD, expiration, range filtering
- Packet decoding: AX.25, position parsing, timestamp extraction
- DSP operations: Bell 202 demodulation, sample processing

See `tests/TEST_RESULTS.md` for complete test documentation.

## Future Enhancements

Potential additions to the architecture:
- Multiple SDR plugins (different hardware types)
- Network packet sources plugin
- File input plugin
- TNC plugin
- All communicating through the same packet ingestion API

