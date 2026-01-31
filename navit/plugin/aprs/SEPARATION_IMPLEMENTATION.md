# APRS Plugin Separation - Implementation Guide

## Current State

The APRS plugin currently integrates RTL-SDR functionality directly:
- `aprs_rtlsdr.c` - RTL-SDR hardware interface
- `aprs_rtlsdr.h` - RTL-SDR API definitions
- RTL-SDR code compiled conditionally with `#ifdef HAVE_RTLSDR`
- Direct integration in `aprs.c` via `aprs_rtlsdr_data_callback()`

## Target Architecture

### Plugin 1: aprs (Core)
- **Files to keep:**
  - `aprs.c` - Core plugin (remove RTL-SDR code)
  - `aprs.h` - Export packet ingestion interface
  - `aprs_db.c/h` - Database management
  - `aprs_decode.c/h` - Packet parsing
  - `aprs_nmea.c/h` - NMEA serial input
  - `aprs_osd.c` - Menu interface
  - `aprs_plugin_interface.h` - Inter-plugin communication

- **Files to remove:**
  - `aprs_rtlsdr.c` - Move to SDR plugin
  - `aprs_rtlsdr.h` - Move to SDR plugin

### Plugin 2: aprs_sdr (New)
- **New files:**
  - `aprs_sdr.c` - Main plugin initialization
  - `aprs_sdr_hw.c` - RTL-SDR hardware (from `aprs_rtlsdr.c`)
  - `aprs_sdr_dsp.c` - Bell 202 demodulation (new or extracted)
  - `aprs_sdr.h` - Internal API

## Inter-Plugin Communication Implementation

### Step 1: Export Function from APRS Plugin

In `aprs.c`, add:

```c
#include "aprs_plugin_interface.h"

// Global callback list for packet sources
static GList *packet_sources = NULL;

int aprs_register_packet_source(aprs_packet_callback_t callback, void *user_data) {
    if (!callback) return 0;

    struct packet_source *src = g_new0(struct packet_source, 1);
    src->callback = callback;
    src->user_data = user_data;

    packet_sources = g_list_append(packet_sources, src);
    dbg(lvl_info, "APRS packet source registered");
    return 1;
}

int aprs_unregister_packet_source(aprs_packet_callback_t callback) {
    GList *l;
    for (l = packet_sources; l; l = g_list_next(l)) {
        struct packet_source *src = (struct packet_source *)l->data;
        if (src->callback == callback) {
            packet_sources = g_list_remove(packet_sources, src);
            g_free(src);
            dbg(lvl_info, "APRS packet source unregistered");
            return 1;
        }
    }
    return 0;
}

// Internal function to deliver packets from any source
static void aprs_deliver_packet(const unsigned char *data, int length, void *user_data) {
    struct map_priv *priv = (struct map_priv *)user_data;
    aprs_process_packet(priv, data, length);
}

// Make function visible to other plugins
__attribute__((visibility("default")))
int aprs_register_packet_source(aprs_packet_callback_t callback, void *user_data);
```

### Step 2: SDR Plugin Discovers APRS Plugin

In `aprs_sdr.c`:

```c
#include <gmodule.h>
#include "aprs_plugin_interface.h"

static aprs_packet_callback_t aprs_deliver_callback = NULL;
static struct map_priv *aprs_map_priv = NULL;

// Function to deliver decoded frames to APRS plugin
static void aprs_sdr_deliver_frame(const unsigned char *frame_data, int frame_length) {
    if (aprs_deliver_callback && aprs_map_priv) {
        aprs_deliver_callback(frame_data, frame_length, aprs_map_priv);
    }
}

// Discover APRS plugin and register callback
static int aprs_sdr_discover_aprs_plugin(void) {
    GModule *aprs_module = NULL;
    int (*register_func)(aprs_packet_callback_t, void *) = NULL;

    // Try to open APRS plugin module
    aprs_module = g_module_open("libaprs.so", G_MODULE_BIND_LAZY);
    if (!aprs_module) {
        // Try alternative names
        aprs_module = g_module_open("aprs", G_MODULE_BIND_LAZY);
    }

    if (!aprs_module) {
        dbg(lvl_warning, "APRS plugin not found - SDR plugin will not deliver packets");
        return 0;
    }

    // Look up the registration function
    if (!g_module_symbol(aprs_module, "aprs_register_packet_source", (gpointer *)&register_func)) {
        dbg(lvl_error, "Could not find aprs_register_packet_source in APRS plugin");
        g_module_close(aprs_module);
        return 0;
    }

    // Get APRS map instance (need to find it via Navit's map system)
    struct mapset *ms = navit_get_mapset(navit_get());
    // ... find APRS map ...

    // Register our callback
    if (register_func(aprs_sdr_deliver_frame, aprs_map_priv)) {
        dbg(lvl_info, "Successfully registered with APRS plugin");
        return 1;
    }

    return 0;
}
```

### Step 3: Alternative - Use Navit's Plugin Discovery

Better approach using Navit's plugin system:

```c
// In aprs_sdr.c
#include "plugin.h"
#include "mapset.h"
#include "map.h"

static int aprs_sdr_discover_aprs_plugin(void) {
    // Use Navit's plugin_get_category to find APRS map
    struct map_priv *aprs_map = (struct map_priv *)plugin_get_category_map("aprs");

    if (!aprs_map) {
        dbg(lvl_warning, "APRS map not found");
        return 0;
    }

    // Get the packet processing function
    // This requires exporting it from aprs.c
    extern int aprs_process_packet(struct map_priv *priv, const unsigned char *data, int length);

    // Store reference for later use
    aprs_map_priv = aprs_map;

    return 1;
}
```

## Bell 202 Demodulation

The SDR plugin needs to implement Bell 202 demodulation:

1. **I/Q Samples** → **Audio Signal** (downconvert, filter)
2. **Audio Signal** → **Digital Bits** (2FSK demodulation at 1200 bps)
3. **Digital Bits** → **AX.25 Frames** (bit sync, frame detection)
4. **AX.25 Frames** → **APRS Plugin** (via callback)

This can be implemented in `aprs_sdr_dsp.c` using:
- Goertzel algorithm for tone detection
- Bit synchronization and NRZI decoding
- AX.25 frame extraction

## Migration Checklist

- [ ] Create `aprs_sdr` plugin directory
- [ ] Move `aprs_rtlsdr.c` → `aprs_sdr_hw.c`
- [ ] Move `aprs_rtlsdr.h` → `aprs_sdr_hw.h`
- [ ] Create `aprs_sdr_dsp.c` for Bell 202 demodulation
- [ ] Create `aprs_sdr.c` for plugin initialization
- [ ] Implement inter-plugin communication
- [ ] Remove RTL-SDR code from `aprs.c`
- [ ] Update `CMakeLists.txt` files
- [ ] Remove RTL-SDR dependencies from APRS plugin
- [ ] Add SDR plugin to main CMakeLists.txt
- [ ] Test on Linux
- [ ] Test on Android
- [ ] Update documentation

## Testing Strategy

1. **Unit Tests:**
   - Test SDR plugin hardware interface independently
   - Test Bell 202 demodulation with sample data
   - Test inter-plugin communication

2. **Integration Tests:**
   - Load both plugins
   - Verify packet delivery
   - Test graceful fallback when APRS plugin unavailable
   - Test graceful fallback when SDR hardware unavailable

3. **Platform Tests:**
   - Linux desktop with RTL-SDR
   - Android with USB OTG
   - Android without USB OTG (should fail gracefully)

## Benefits of This Approach

1. **Clean Separation:** Each plugin has single responsibility
2. **Independent Updates:** Can update SDR or APRS independently
3. **Smaller Binaries:** Users without SDR don't need SDR code
4. **Better Testing:** Can test plugins independently
5. **Flexible Deployment:** Mix and match plugins as needed

