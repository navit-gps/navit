# APRS SDR Plugin

## Overview

The APRS SDR plugin provides direct RTL-SDR hardware access with built-in Bell 202 demodulation for APRS packet reception. It works as a companion to the core APRS plugin, automatically discovering and delivering decoded AX.25 frames.

## Features

- Direct RTL-SDR hardware interface (no external tools required)
- Bell 202 demodulation (2FSK at 1200 bps) based on Direwolf patterns
- Automatic APRS plugin discovery and registration
- Support for multiple RTL-SDR device types
- Android USB Host API integration
- Configurable frequency, gain, and PPM correction
- Real-time I/Q sample processing

## Architecture

The SDR plugin is a separate, optional component that handles all RF-related operations:

```
RTL-SDR Hardware
    |
    v
aprs_sdr_hw.c (Hardware Interface)
    |
    v
aprs_sdr_dsp.c (Bell 202 Demodulation)
    |
    v
AX.25 Frames
    |
    v
aprs_sdr.c (Plugin Integration)
    |
    v
APRS Plugin (via aprs_process_packet)
```

## Device Support

The plugin supports the following RTL-SDR devices:

- **RTL-SDR Blog V3** (RTL2832U + R820T2) - Recommended for best performance
- **V4 R828D** (RTL2832U + R828D) - Latest generation, excellent sensitivity
- **Nooelec dongles** (NESDR series) - Budget-friendly option
- **Generic RTL2832U** - Fallback for other compatible dongles

Device type can be auto-detected from USB device strings or manually selected via the APRS plugin menu.

## Bell 202 Demodulation

The plugin implements Bell 202 demodulation internally:

- **Modulation**: 2FSK (Binary Frequency Shift Keying)
- **Mark frequency**: 1200 Hz (binary 1)
- **Space frequency**: 2200 Hz (binary 0)
- **Data rate**: 1200 baud
- **Encoding**: NRZI (Non-Return-to-Zero Inverted)
- **Frame format**: AX.25 UI-frames

The demodulation uses Goertzel filters for frequency detection, bit synchronization, and AX.25 frame extraction with bit stuffing handling.

## Inter-Plugin Communication

The SDR plugin automatically discovers the APRS plugin during initialization:

1. Uses `plugin_get_category_map("aprs")` to find APRS map instance
2. Attempts to register via `aprs_register_packet_source()` if available
3. Falls back to direct function call via `aprs_process_packet()`
4. Logs warnings if APRS plugin not found but continues running

This allows the SDR plugin to work independently - if the APRS plugin loads later, frames will start being delivered automatically.

## Configuration

### XML Configuration

```xml
<map type="aprs_sdr">
    <attribute name="frequency" value="144.390"/>
    <attribute name="gain" value="49"/>
    <attribute name="device" value="rtlsdr_blog_v3"/>
</map>
```

### Runtime Configuration

Configuration can be changed via the APRS plugin menu:
- Frequency selection (144.390, 144.800, 145.175 MHz, etc.)
- Device type selection
- Gain adjustment
- PPM correction

## Android Support

The plugin includes native Android USB Host API integration:

- Automatic USB device discovery
- Permission request handling
- Bulk transfer operations for I/Q samples
- JNI bridge for Android USB API

**Requirements:**
- Android device with USB OTG support
- RTL-SDR dongle connected via USB OTG cable
- USB permissions granted at runtime

See the main APRS README for Android setup instructions.

## Building

### Dependencies

- librtlsdr (librtlsdr-dev on Debian/Ubuntu)
- pthread (for threading)
- GLib 2.0
- Navit core headers

### Build Instructions

The plugin is built automatically when librtlsdr is found:

```bash
# Install dependencies
sudo apt-get install librtlsdr-dev

# Build (plugin auto-detected)
cd build
cmake ..
make plugin_aprs_sdr
```

If librtlsdr is not found, the plugin build is skipped (APRS plugin continues to work with NMEA/other sources).

## Usage

1. Connect RTL-SDR dongle to USB port
2. Ensure both `aprs` and `aprs_sdr` plugins are loaded in Navit configuration
3. Select device type in APRS plugin menu
4. Set frequency for your region
5. Plugin automatically starts receiving and demodulating

The SDR plugin runs independently - it processes RF signals and delivers decoded frames to the APRS plugin, which handles database storage and map rendering.

## Troubleshooting

### Device Not Found

- Verify RTL-SDR is connected and recognized by system
- Check USB permissions (Linux: add user to plugdev group)
- Try different USB port
- Verify device is not being used by another application

### No Frames Decoded

- Check frequency is correct for your region
- Verify signal strength (may need antenna or better location)
- Check gain settings (try auto-gain or manual adjustment)
- Verify PPM correction if frequency drift is suspected

### APRS Plugin Not Found

- Ensure APRS plugin loads before or simultaneously with SDR plugin
- Check plugin loading order in Navit configuration
- Verify both plugins are enabled

### Android Issues

- Verify USB OTG support (use USB OTG Checker app)
- Grant USB permissions when prompted
- Check device power management (prevent USB sleep)
- Try different USB OTG cable

## Performance

Typical performance characteristics:

- Sample rate: 48 kHz (configurable)
- Processing latency: < 100 ms from RF to decoded frame
- CPU usage: Moderate (DSP processing in dedicated thread)
- Memory: ~10-20 MB for buffers and state

For optimal performance:
- Use RTL-SDR Blog V3 or V4 R828D (better sensitivity)
- Set appropriate gain (auto-gain recommended)
- Ensure adequate USB power (use powered hub if needed)

## API Reference

### Hardware Interface (`aprs_sdr_hw.h`)

- `aprs_sdr_hw_new()` - Initialize hardware
- `aprs_sdr_hw_start()` - Begin sample acquisition
- `aprs_sdr_hw_stop()` - Stop acquisition
- `aprs_sdr_hw_set_frequency()` - Change frequency
- `aprs_sdr_hw_set_gain()` - Adjust gain
- `aprs_sdr_hw_set_ppm()` - Set frequency correction

### DSP Interface (`aprs_sdr_dsp.h`)

- `aprs_sdr_dsp_new()` - Initialize demodulator
- `aprs_sdr_dsp_process_samples()` - Process I/Q samples
- `aprs_sdr_dsp_set_frame_callback()` - Register frame delivery callback

### Android Interface (`aprs_sdr_android.h`)

- `aprs_sdr_android_new()` - Initialize Android USB manager
- `aprs_sdr_android_open_device()` - Open USB device
- `aprs_sdr_android_bulk_read()` - Read I/Q samples
- `aprs_sdr_android_control_transfer()` - Send control requests

## Testing

### Unit Tests

DSP (Digital Signal Processing) tests are available in `navit/plugin/aprs/tests/test_aprs_dsp.c`. These test the Bell 202 demodulation without requiring hardware:

**Test Coverage:**
- DSP instance creation and destruction
- Default configuration (1200 Hz mark, 2200 Hz space, 1200 baud)
- Sample processing with synthetic I/Q data
- Callback registration and invocation

**Running DSP Tests:**
```bash
cd build
./navit/plugin/aprs/tests/test_aprs_dsp
```

**Status:** All DSP tests pass with synthetic data. No hardware required.

### Hardware Testing

Hardware tests for `aprs_sdr_hw.c` are not included as unit tests because they require actual RTL-SDR devices. The hardware interface directly calls `librtlsdr` functions that need real hardware.

**Hardware Functionality (Requires Integration Testing):**
- Device detection and enumeration
- Device type identification (Blog V3, V4 R828D, Nooelec, Generic)
- Frequency tuning and PPM correction
- Gain control (manual and auto)
- Sample acquisition thread operation
- Start/stop lifecycle management

**Manual Hardware Testing:**
1. Connect RTL-SDR dongle to USB port
2. Verify device detection via APRS plugin menu
3. Test frequency changes and verify tuning
4. Monitor sample acquisition and frame decoding
5. Test device type auto-detection

Hardware functionality is validated through manual integration testing with actual RTL-SDR devices. The code includes comprehensive error handling and device type detection, but these require hardware to validate.

## License

This plugin is licensed under the GNU Library General Public License version 2, same as Navit.

## References

- Direwolf APRS software: https://github.com/wb2osz/direwolf
- RTL-SDR library: https://github.com/librtlsdr/librtlsdr
- APRS Protocol Specification: http://www.aprs.org
- Bell 202 specification: ITU-T V.23

