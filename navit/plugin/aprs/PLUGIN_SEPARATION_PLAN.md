# APRS Plugin Separation Plan

## Overview

This document outlines the separation of the APRS plugin into two independent plugins:
1. **aprs** - Core APRS functionality (packet parsing, database, map rendering)
2. **aprs_sdr** - RTL-SDR hardware interface and DSP pipeline

## Architecture

### Plugin 1: aprs (Core Plugin)

**Responsibilities:**
- APRS packet parsing (AX.25, position decoding)
- SQLite database management
- Station tracking and expiration
- Map rendering (POI items)
- Menu/OSD interface
- NMEA serial input support
- Packet ingestion interface (for external sources)

**Exports:**
- `aprs_process_packet()` - Function pointer for packet ingestion
- Map plugin registration
- OSD plugin registration

**Dependencies:**
- SQLite3
- Navit core
- (Optional) libnmea for NMEA parsing

### Plugin 2: aprs_sdr (SDR Plugin)

**Responsibilities:**
- RTL-SDR hardware initialization
- Device detection and enumeration
- Frequency tuning and gain control
- I/Q sample acquisition
- Bell 202 demodulation (2FSK at 1200 bps)
- AX.25 frame extraction
- Delivering decoded frames to APRS plugin

**Exports:**
- SDR device enumeration
- Configuration interface
- Status reporting

**Dependencies:**
- librtlsdr
- Navit core
- (Optional) DSP libraries for demodulation

## Inter-Plugin Communication

### Method 1: Direct Function Call (Preferred)

The APRS plugin registers a callback function that the SDR plugin can discover and call directly.

**APRS Plugin Side:**
```c
// In aprs.c
static int (*aprs_sdr_callback_registered)(void (*callback)(const unsigned char *data, int length, void *user_data), void *user_data) = NULL;

void aprs_register_sdr_callback(void (*callback)(const unsigned char *data, int length, void *user_data), void *user_data) {
    if (aprs_sdr_callback_registered) {
        aprs_sdr_callback_registered(callback, user_data);
    }
}
```

**SDR Plugin Side:**
```c
// In aprs_sdr.c
typedef int (*aprs_register_callback_t)(void (*callback)(const unsigned char *data, int length, void *user_data), void *user_data);
static aprs_register_callback_t aprs_register_callback = NULL;

// During plugin_init, discover APRS plugin
void plugin_init_aprs_sdr(void) {
    // Look up APRS plugin's exported function
    aprs_register_callback = plugin_get_function("aprs", "aprs_register_sdr_callback");
    if (aprs_register_callback) {
        aprs_register_callback(aprs_sdr_deliver_frame, NULL);
    }
}
```

### Method 2: Navit Attribute System

Use Navit's attribute system to pass configuration and data:
- APRS plugin exposes `attr_aprs_packet_callback` attribute
- SDR plugin sets this attribute with its callback
- APRS plugin invokes callback when packets arrive

### Method 3: Event System

Use Navit's event system for asynchronous communication:
- SDR plugin emits events with decoded frames
- APRS plugin subscribes to these events

**Recommendation:** Use Method 1 (direct function calls) for lowest latency and simplicity.

## Initialization Order

### Dependency Management

1. **APRS plugin initializes first:**
   - Registers map plugin
   - Registers OSD plugin
   - Exports packet ingestion function
   - Sets up database and callbacks

2. **SDR plugin initializes second:**
   - Discovers APRS plugin
   - Registers callback with APRS plugin
   - Initializes RTL-SDR hardware
   - Starts demodulation thread

### Discovery Mechanism

The SDR plugin will:
1. Query Navit's plugin registry for "aprs" plugin
2. Look up exported symbols using `g_module_symbol()` or Navit's plugin API
3. Retry discovery if APRS plugin not yet loaded (with timeout)
4. Fail gracefully if APRS plugin unavailable

## Configuration Coordination

### Shared Configuration

Both plugins read from Navit's unified XML config:
```xml
<map type="aprs" ...>
    <attr name="frequency" value="144.390"/>
    <attr name="gain" value="49"/>
    <attr name="device" value="rtlsdr_blog_v3"/>
</map>
```

### Menu Integration

The APRS plugin's OSD menu can:
- Detect if SDR plugin is loaded
- Show SDR-specific menu options when available
- Hide SDR options when SDR plugin unavailable
- Display status messages about SDR availability

## Android Considerations

### USB Permissions

The SDR plugin handles Android USB permissions:
- Request USB device access via Android USB Host API
- Handle permission denial gracefully
- Notify user through Navit's message system

### NDK Compilation

- SDR plugin's DSP code compiled via NDK
- Native threads for sample acquisition
- JNI bridge for Android USB API

## Graceful Fallback

### SDR Plugin Failure Scenarios

1. **No hardware present:**
   - Plugin loads but reports "no devices"
   - APRS plugin continues with NMEA/network sources

2. **USB permission denied:**
   - Plugin reports permission error
   - APRS plugin shows user-friendly message

3. **APRS plugin not loaded:**
   - SDR plugin fails to discover APRS plugin
   - Logs warning, unloads gracefully

4. **Hardware initialization failure:**
   - Plugin reports error to APRS plugin
   - APRS plugin continues with other sources

## File Structure

```
navit/plugin/
├── aprs/
│   ├── aprs.c              (core plugin, no RTL-SDR code)
│   ├── aprs.h              (exports aprs_process_packet)
│   ├── aprs_db.c
│   ├── aprs_decode.c
│   ├── aprs_nmea.c
│   ├── aprs_osd.c
│   └── CMakeLists.txt
└── aprs_sdr/
    ├── aprs_sdr.c          (main plugin file)
    ├── aprs_sdr.h
    ├── aprs_sdr_hw.c       (RTL-SDR hardware interface)
    ├── aprs_sdr_dsp.c      (Bell 202 demodulation)
    ├── aprs_sdr_android.c  (Android USB interface)
    └── CMakeLists.txt
```

## Migration Steps

1. Create `aprs_sdr` plugin directory structure
2. Move `aprs_rtlsdr.c` → `aprs_sdr_hw.c`
3. Extract Bell 202 demodulation into `aprs_sdr_dsp.c`
4. Create plugin initialization in `aprs_sdr.c`
5. Add inter-plugin communication code
6. Remove RTL-SDR code from `aprs.c`
7. Update CMakeLists.txt files
8. Test on Linux and Android

## Benefits

1. **Modularity:** Each plugin has single responsibility
2. **Flexibility:** Users can use APRS without SDR
3. **Maintainability:** Easier to update independently
4. **Deployment:** Smaller plugin sizes
5. **Testing:** Can test plugins independently

