# RTL-SDR Setup Guide for APRS Plugin

This guide covers setup and configuration for RTL-SDR dongles with the Navit APRS plugin.

## Supported RTL-SDR Devices

The plugin is compatible with all RTL-SDR devices, including:

- **RTL-SDR Blog V3** (RTL2832U + R820T2)
- **V4 R828D** (RTL2832U + R828D)
- **Nooelec dongles** (various models: NESDR, NESDR Mini, NESDR SMArt, NESDR XTR, etc.)
- **Generic RTL2832U-based dongles**

All these devices use the same driver stack and are fully compatible.

### Quick Driver Installation Reference

| Platform | Driver Method | Notes |
|----------|---------------|-------|
| **Linux** | `apt-get install rtl-sdr` or build from source | Requires DVB-T module blacklisting |
| **Windows** | Zadig (WinUSB) or SDR# driver package | Easiest: Use Zadig tool |
| **macOS** | `brew install rtl-sdr` or build from source | No kernel module issues |
| **Android** | SDR Driver app from Play Store | Requires USB OTG support |

**All devices (RTL-SDR Blog V3, V4 R828D, Nooelec, Generic) use the same drivers on each platform.**

## Hardware Requirements

- RTL-SDR USB dongle (one of the models above)
- USB 2.0+ port
- Appropriate antenna for 2-meter band (144-146 MHz)
- USB extension cable (recommended to reduce RFI)

## Software Stack

The RTL-SDR → APRS Plugin pipeline:

```
RTL-SDR Dongle
    |
    v
rtl_fm (or SoapySDR)
    |
    v
Audio Stream (48 kHz, 16-bit, mono)
    |
    v
multimon-ng / direwolf (Bell 202 demodulator)
    |
    v
Decoded AX.25 Packets
    |
    v
Navit APRS Plugin
```

## Installation

### Platform-Specific Driver Installation

#### Linux (Debian/Ubuntu, Fedora, Arch, etc.)

**For RTL-SDR Blog V3, V4 R828D, Nooelec, and Generic RTL2832U devices:**

**Method 1: Package Manager (Easiest)**
```bash
# Debian/Ubuntu
sudo apt-get update
sudo apt-get install rtl-sdr librtlsdr-dev rtl-sdr-dbg

# Fedora
sudo dnf install rtl-sdr rtl-sdr-devel

# Arch Linux
sudo pacman -S rtl-sdr

# Verify installation
rtl_test -t
```

**Method 2: Build from Source (Recommended for latest features)**
```bash
# Install dependencies
sudo apt-get install build-essential cmake libusb-1.0-0-dev

# Clone and build RTL-SDR library
git clone https://github.com/osmocom/rtl-sdr.git
cd rtl-sdr
mkdir build && cd build
cmake ../ -DINSTALL_UDEV_RULES=ON
make
sudo make install
sudo ldconfig

# Verify installation
rtl_test -t
```

**Blacklist DVB-T Kernel Modules (Required for all Linux systems):**

RTL-SDR dongles are often detected as DVB-T tuners. Blacklist these modules:

```bash
sudo nano /etc/modprobe.d/blacklist-rtl.conf
```

Add:
```
blacklist dvb_usb_rtl28xxu
blacklist rtl2832
blacklist rtl2830
```

Reboot or unload modules:
```bash
sudo rmmod dvb_usb_rtl28xxu rtl2832 rtl2830
```

**Create udev rules (for non-root access):**
```bash
sudo nano /etc/udev/rules.d/20-rtlsdr.rules
```

Add:
```
SUBSYSTEM=="usb", ATTRS{idVendor}=="0bda", ATTRS{idProduct}=="2838", GROUP="plugdev", MODE="0666", SYMLINK+="rtl_sdr"
```

Reload udev rules:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Add your user to the plugdev group:
```bash
sudo usermod -a -G plugdev $USER
# Log out and log back in for changes to take effect
```

#### Windows

**For RTL-SDR Blog V3, V4 R828D, Nooelec, and Generic RTL2832U devices:**

**Method 1: Zadig Driver Installation (Recommended)**

1. **Download Zadig:**
   - Visit: https://zadig.akeo.ie/
   - Download the latest version

2. **Install Drivers:**
   - Connect your RTL-SDR dongle to USB port
   - Open Zadig
   - Go to Options → List All Devices
   - Select "Bulk-In, Interface (Interface 0)" or "RTL2832UHIDIR"
   - Select "WinUSB" as the target driver
   - Click "Install Driver" or "Replace Driver"
   - Wait for installation to complete

3. **Verify Installation:**
   - Download SDR# or HDSDR
   - Test device detection

**Method 2: SDR# Driver Package (Alternative)**

1. **Download SDR#:**
   - Visit: https://airspy.com/download/
   - Download SDR# installer

2. **Install:**
   - Run installer
   - Follow prompts to install drivers
   - Drivers will be installed automatically

**Method 3: Manual Driver Installation**

1. **Download libusb-win32:**
   - Visit: https://sourceforge.net/projects/libusb-win32/
   - Download latest version

2. **Install:**
   - Extract and run `inf-wizard.exe`
   - Select your RTL-SDR device
   - Generate and install INF file

**For all Windows methods:**
- Device should appear in Device Manager as "RTL-SDR" or "WinUSB Device"
- No reboot required for Zadig method
- May require reboot for other methods

#### macOS

**For RTL-SDR Blog V3, V4 R828D, Nooelec, and Generic RTL2832U devices:**

**Method 1: Homebrew (Recommended)**

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install RTL-SDR
brew install rtl-sdr

# Verify installation
rtl_test -t
```

**Method 2: Build from Source**

```bash
# Install dependencies
brew install cmake libusb

# Clone and build
git clone https://github.com/osmocom/rtl-sdr.git
cd rtl-sdr
mkdir build && cd build
cmake ../
make
sudo make install

# Verify installation
rtl_test -t
```

**macOS-specific notes:**
- No kernel module blacklisting needed (macOS doesn't use DVB-T drivers)
- May need to allow USB access in System Preferences → Security & Privacy
- Some macOS versions may require disabling System Integrity Protection (SIP) - not recommended

#### Android

**For RTL-SDR Blog V3, V4 R828D, Nooelec, and Generic RTL2832U devices:**

**Method 1: SDR Driver App (Recommended for all devices)**

1. **Install SDR Driver:**
   - Open Google Play Store
   - Search for "SDR driver" by Martin Marinov
   - Install the app (this is an Android port of rtl-sdr's rtl_tcp)

2. **Connect RTL-SDR Dongle:**
   - Use USB OTG (On-The-Go) cable to connect dongle to Android device
   - Android device must support USB OTG (most modern devices do)

3. **Launch SDR Driver:**
   - Open the SDR Driver app
   - Grant USB permissions when prompted
   - App will detect and configure the RTL-SDR device automatically

4. **Configure:**
   - Set frequency, gain, and other parameters in the app
   - Connect to rtl_tcp server running on device (localhost:1234)

**Method 2: USB OTG Helper Apps**

Some devices may require additional USB OTG helper apps:

1. **USB OTG Checker:**
   - Install from Play Store to verify OTG support

2. **USB Host Controller:**
   - Some devices need this for proper USB device recognition

**Android-specific requirements:**
- **USB OTG Support:** Device must support USB On-The-Go
- **Power:** RTL-SDR dongles draw power from USB - ensure device can supply adequate power
- **USB Hub:** May need powered USB hub for stable operation
- **Permissions:** Grant USB permissions when prompted
- **Root:** Not required for SDR Driver app, but may be needed for some advanced features

**Android Device Compatibility:**
- Most Android 5.0+ devices support USB OTG
- Check device specifications for USB OTG support
- Some devices may require specific USB OTG cables

**Troubleshooting Android:**
- If device not detected, try different USB OTG cable
- Ensure USB debugging is enabled (Settings → Developer Options)
- Some devices may need "USB Host Controller" app
- Check device power management settings (prevent USB sleep)

### 2. Install APRS Decoding Tools

**Linux:**
```bash
# Debian/Ubuntu
sudo apt-get install multimon-ng

# Or build from source
git clone https://github.com/EliasOenal/multimon-ng.git
cd multimon-ng
mkdir build && cd build
cmake ../
make
sudo make install
```

**Windows:**
- Download pre-built binaries from: https://github.com/EliasOenal/multimon-ng/releases
- Or use SDR# with APRS plugins

**macOS:**
```bash
brew install multimon-ng
```

**Android:**
- Use SDR Driver app with built-in decoding
- Or connect to rtl_tcp server and use external decoder

### 3. Verify Device Detection

RTL-SDR dongles are often detected as DVB-T tuners. Blacklist these modules:

```bash
sudo nano /etc/modprobe.d/blacklist-rtl.conf
```

Add:
```
blacklist dvb_usb_rtl28xxu
blacklist rtl2832
blacklist rtl2830
```

Reboot or unload modules:
```bash
sudo rmmod dvb_usb_rtl28xxu rtl2832 rtl2830
```

### 3. Verify Device Detection

**Linux/macOS:**
```bash
rtl_test -t
```

You should see output like:
```
Found 1 device(s):
  0:  Realtek, RTL2838UHIDIR, SN: 00000001

Using device 0: Generic RTL2832U OEM
```

**Windows:**
- Use SDR# or HDSDR to verify device detection
- Device should appear in Device Manager as "RTL-SDR" or "WinUSB Device"

**Android:**
- Open SDR Driver app
- Device should be detected automatically
- Check app status for connection confirmation

## Configuration by Device Type

### RTL-SDR Blog V3

**Specifications:**
- Chipset: RTL2832U + R820T2
- Frequency range: 24-1766 MHz
- Sample rate: Up to 3.2 MS/s
- Bias-T: Available on some models

**Recommended settings:**
```bash
rtl_fm -f 144.39M -s 48000 -g 49 -l 0 -M fm -E dc -p 0 - | \
multimon-ng -t raw -a AFSK1200 -
```

### V4 R828D

**Specifications:**
- Chipset: RTL2832U + R828D
- Frequency range: 24-1766 MHz
- Sample rate: Up to 3.2 MS/s
- Improved sensitivity over R820T2

**Recommended settings:**
```bash
rtl_fm -f 144.39M -s 48000 -g 50 -l 0 -M fm -E dc -p 0 - | \
multimon-ng -t raw -a AFSK1200 -
```

**Note:** R828D may require slightly different gain settings. Start with `-g 50` and adjust.

### Nooelec Dongles

**Common models:**
- NESDR SMArt (R820T2)
- NESDR Mini (R820T2)
- NESDR Nano (R820T2)
- NESDR XTR (R828D)

**Recommended settings:**
```bash
rtl_fm -f 144.39M -s 48000 -g 49 -l 0 -M fm -E dc -p 0 - | \
multimon-ng -t raw -a AFSK1200 -
```

**Nooelec-specific notes:**
- Some models have built-in bias-T (check model specifications)
- May require USB power supply for stable operation
- Use provided USB extension cable to reduce RFI

## Complete Setup Scripts

### North America (144.390 MHz)

```bash
#!/bin/bash
# APRS receiver for North America

FREQ=144.39M
GAIN=49
SAMPLE_RATE=48000

rtl_fm -f $FREQ \
       -s $SAMPLE_RATE \
       -g $GAIN \
       -l 0 \
       -M fm \
       -E dc \
       -p 0 \
       - | \
multimon-ng -t raw \
            -a AFSK1200 \
            -q \
            - | \
while read line; do
    # Process decoded packets here
    echo "$line"
done
```

### Europe (144.800 MHz)

```bash
#!/bin/bash
# APRS receiver for Europe

FREQ=144.8M
GAIN=49
SAMPLE_RATE=48000

rtl_fm -f $FREQ \
       -s $SAMPLE_RATE \
       -g $GAIN \
       -l 0 \
       -M fm \
       -E dc \
       -p 0 \
       - | \
multimon-ng -t raw \
            -a AFSK1200 \
            -q \
            -
```

### Australia (145.175 MHz)

```bash
#!/bin/bash
# APRS receiver for Australia

FREQ=145.175M
GAIN=49
SAMPLE_RATE=48000

rtl_fm -f $FREQ \
       -s $SAMPLE_RATE \
       -g $GAIN \
       -l 0 \
       -M fm \
       -E dc \
       -p 0 \
       - | \
multimon-ng -t raw \
            -a AFSK1200 \
            -q \
            -
```

## rtl_fm Parameters Explained

- `-f FREQ`: Center frequency (e.g., 144.39M for 144.390 MHz)
- `-s RATE`: Sample rate in Hz (48000 recommended for APRS)
- `-g GAIN`: Tuner gain in tenths of dB (49 = 4.9 dB, auto = automatic)
- `-l SQUELCH`: Squelch level (0 = disabled)
- `-M MODE`: Modulation mode (fm for FM)
- `-E dc`: Enable DC blocking filter
- `-p PPM`: Frequency correction in parts per million (calibrate with rtl_test)
- `-`: Output to stdout (pipe to multimon-ng)

## Gain Optimization

Different RTL-SDR models may require different gain settings:

1. **Start with auto gain:**
   ```bash
   rtl_fm -f 144.39M -s 48000 -g auto ...
   ```

2. **If too noisy, reduce gain:**
   ```bash
   rtl_fm -f 144.39M -s 48000 -g 30 ...
   ```

3. **If too weak signals, increase gain:**
   ```bash
   rtl_fm -f 144.39M -s 48000 -g 50 ...
   ```

4. **Check available gain range:**
   ```bash
   rtl_test -g
   ```

## Frequency Calibration

RTL-SDR dongles often have frequency offset. Calibrate:

```bash
# Find PPM offset
rtl_test -p

# Use the reported PPM value in rtl_fm
rtl_fm -f 144.39M -p <PPM_VALUE> ...
```

Typical offsets:
- RTL-SDR Blog V3: ±1-5 ppm
- V4 R828D: ±2-10 ppm
- Nooelec: ±1-5 ppm

## Integration with Navit APRS Plugin

### Method 1: Pipe to Custom Processor

Create a packet processor that feeds the plugin:

```bash
#!/bin/bash
# aprs_to_navit.sh

rtl_fm -f 144.39M -s 48000 -g 49 -l 0 -M fm -E dc -p 0 - | \
multimon-ng -t raw -a AFSK1200 -q - | \
while IFS= read -r line; do
    if [[ $line =~ ^APRS: ]]; then
        # Extract packet data and call plugin function
        # This requires custom integration code
        echo "Packet: $line"
    fi
done
```

### Method 2: Using direwolf (Recommended)

direwolf can decode APRS and output in various formats:

```bash
# Install direwolf
sudo apt-get install direwolf

# Configure direwolf.conf
cat > direwolf.conf << EOF
ADEVICE plughw:1,0
ACHANNELS 1
CHANNEL 0
MYCALL N0CALL
MODEM 1200
AGWPORT 8000
KISSPORT 8001
EOF

# Run direwolf with rtl_fm
rtl_fm -f 144.39M -s 48000 -g 49 -l 0 -M fm -E dc -p 0 - | \
sox -t raw -r 48000 -es -b 16 -c 1 - -t wav - | \
direwolf -c direwolf.conf -
```

### Method 3: Network Integration

Use APRS-IS or local network feed:

```python
#!/usr/bin/env python3
# aprs_network_feed.py
import socket
import struct

# Connect to APRS-IS or local direwolf
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 8000))

while True:
    data = sock.recv(1024)
    # Parse and feed to Navit plugin
    # Implementation depends on your integration method
```

## Troubleshooting

### Device Not Found

**Linux:**
```bash
# Check USB connection
lsusb | grep RTL

# Should show: Bus XXX Device XXX: ID 0bda:2838 Realtek Semiconductor Corp. RTL2838 DVB-T

# Check permissions
sudo chmod 666 /dev/bus/usb/*/*

# Or add user to dialout/plugdev group
sudo usermod -a -G dialout $USER
sudo usermod -a -G plugdev $USER
# Log out and log back in

# Verify udev rules
ls -l /dev/rtl_sdr*

# Check if DVB-T modules are blacklisted
lsmod | grep rtl28
# Should return nothing if properly blacklisted
```

**Windows:**
- Check Device Manager for "RTL-SDR" or "WinUSB Device"
- If device shows as "Unknown Device" or with yellow exclamation:
  - Reinstall drivers using Zadig
  - Try different USB port
  - Check USB cable
- If using SDR#, ensure drivers are installed via SDR# installer

**macOS:**
```bash
# Check USB connection
system_profiler SPUSBDataType | grep -i rtl

# Check if device is recognized
ioreg -p IOUSB -l -w 0 | grep -i rtl
```

**Android:**
- Verify USB OTG support (use USB OTG Checker app)
- Try different USB OTG cable
- Check if device appears in SDR Driver app
- Enable USB debugging (Settings → Developer Options)
- Some devices may need "USB Host Controller" app
- Check device power settings (prevent USB sleep mode)

### Driver-Specific Issues

**Linux - Permission Denied:**
```bash
# Add user to plugdev group
sudo usermod -a -G plugdev $USER
# Log out and log back in

# Or use sudo (not recommended for regular use)
sudo rtl_test -t
```

**Linux - Device Still Detected as DVB-T:**
```bash
# Check loaded modules
lsmod | grep rtl28

# Force unload
sudo rmmod dvb_usb_rtl28xxu rtl2832 rtl2830

# Verify blacklist
cat /etc/modprobe.d/blacklist-rtl.conf

# Reboot if necessary
sudo reboot
```

**Windows - Driver Installation Failed:**
- Run Zadig as Administrator
- Disable driver signature enforcement (Windows 10/11):
  - Settings → Update & Security → Recovery → Advanced Startup → Restart Now
  - Troubleshoot → Advanced Options → Startup Settings → Restart
  - Press F7 to disable driver signature enforcement
- Try different USB port (USB 2.0 recommended)
- Check Windows Device Manager for conflicting drivers

**macOS - USB Access Denied:**
- System Preferences → Security & Privacy → Privacy → USB Access
- Grant access to terminal/application using RTL-SDR
- Some versions may require disabling System Integrity Protection (not recommended)

**Android - Device Not Detected:**
- Verify USB OTG cable is working (test with USB flash drive)
- Try different USB OTG cable
- Check if device supports USB OTG (use USB OTG Checker app)
- Some devices require specific USB OTG adapters
- Check device power management (prevent USB sleep)
- Try "USB Host Controller" app if device still not detected

### Poor Reception

1. **Check antenna connection**
2. **Try different gain settings**
3. **Use USB extension cable** (reduces RFI from computer)
4. **Check frequency calibration** (PPM offset)
5. **Verify correct frequency** for your region

### No Decoded Packets

1. **Verify signal strength:**
   ```bash
   rtl_fm -f 144.39M -s 48000 -g 49 -l 0 -M fm -E dc -p 0 - | \
   sox -t raw -r 48000 -es -b 16 -c 1 - -t wav - | \
   aplay
   ```
   You should hear the APRS signal (distinctive "chirping" sound).

2. **Test multimon-ng directly:**
   ```bash
   multimon-ng -t wav -a AFSK1200 test_aprs.wav
   ```

3. **Check sample rate:** Must be 48000 Hz for AFSK1200

### Device-Specific Issues

**RTL-SDR Blog V3:**
- Generally very stable
- May need USB power supply for long operation
- Bias-T available on some models

**V4 R828D:**
- Improved sensitivity
- May require different gain settings
- Check for firmware updates

**Nooelec:**
- Models vary - check specific model documentation
- Some models have TCXO (temperature compensated oscillator) for better stability
- USB extension cable highly recommended

## Performance Tuning

### CPU Usage

RTL-SDR processing is CPU-intensive. Optimize:

1. **Reduce sample rate** (if acceptable):
   ```bash
   rtl_fm -s 24000 ...  # Lower CPU, may affect decoding
   ```

2. **Use hardware acceleration** (if available)

3. **Dedicated CPU core** (taskset):
   ```bash
   taskset -c 0 rtl_fm ...
   ```

### Memory Usage

Monitor memory usage:
```bash
htop
# or
top
```

RTL-SDR typically uses 50-200 MB RAM.

## Advanced Configuration

### Multiple Frequencies

Use multiple RTL-SDR dongles or frequency hopping:

```bash
# Frequency 1
rtl_fm -f 144.39M -d 0 ... &
# Frequency 2  
rtl_fm -f 144.8M -d 1 ... &
```

### Recording for Analysis

```bash
rtl_fm -f 144.39M -s 48000 -g 49 -l 0 -M fm -E dc -p 0 - | \
sox -t raw -r 48000 -es -b 16 -c 1 - aprs_recording.wav
```

## References

- RTL-SDR Blog: https://www.rtl-sdr.com
- multimon-ng: https://github.com/EliasOenal/multimon-ng
- direwolf: https://github.com/wb2osz/direwolf
- RTL-SDR Library: https://github.com/osmocom/rtl-sdr

## Legal Notice

Ensure compliance with local regulations. RTL-SDR dongles are receive-only devices. Always verify you have proper authorization before receiving or processing radio signals.

