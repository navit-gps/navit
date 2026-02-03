APRS Plugin
===========

Overview
--------

The APRS (Automatic Packet Reporting System) plugin for Navit provides real-time tracking of APRS stations on the map. It decodes APRS packets (Bell 202 / AX.25), stores station information in a SQLite database, and displays moving objects on the Navit map.

Plugin Architecture
-------------------

The APRS functionality is split into two modular plugins for clean separation of concerns:

Plugin 1: ``aprs`` (Core Plugin)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- **APRS Packet Decoding**: Decodes Bell 202 / AX.25 frames
- **Position Tracking**: Extracts and displays station positions
- **Station Database**: SQLite-based persistent storage
- **Real-time Updates**: Dynamic map updates as packets arrive
- **Range Filtering**: Hardcoded 150 km maximum range (centered on device location)
- **Station Expiration**: Automatic cleanup of stale stations (configurable via GUI)
- **NMEA Serial Input**: Direct NMEA sentence parsing ($GPWPL, $PGRMW, $PMGNWPL, $PKWDWPL)
- **Inter-Plugin Interface**: Exports packet ingestion API for external sources

Plugin 2: ``aprs_sdr`` (SDR Plugin - Optional)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- **RTL-SDR Hardware Interface**: Direct access to RTL-SDR dongles
- **Bell 202 Demodulation**: 2FSK demodulation at 1200 bps (based on Direwolf patterns)
- **Device Support**: RTL-SDR Blog V3, V4 R828D, Nooelec, Generic RTL2832U
- **Android USB Support**: Native Android USB Host API integration
- **Frequency Control**: Configurable RX frequency, gain, and PPM correction
- **Automatic Discovery**: Discovers and connects to APRS plugin automatically

**Benefits of Separation:**

- Users without SDR hardware don't need SDR dependencies
- Plugins can be updated independently
- Cleaner code organization
- Better testing and maintenance
- Flexible deployment (use APRS with NMEA only, or add SDR support)

Features
--------

- **APRS Packet Decoding**: Decodes Bell 202 / AX.25 frames
- **Position Tracking**: Extracts and displays station positions
- **Station Database**: SQLite-based persistent storage
- **Real-time Updates**: Dynamic map updates as packets arrive
- **Range Filtering**: 150 km maximum range (hardcoded, centered on device)
- **Station Expiration**: Automatic cleanup of stale stations (30-180 minutes, configurable)
- **International Frequency Support**: Configurable for different regions
- **Multiple Input Sources**: NMEA serial, SDR plugin, or direct packet injection

Requirements
------------

- Navit navigation system
- SQLite3 development libraries
- APRS packet source:
  - **NMEA Serial**: USB serial adapter (works on all platforms)
  - **SDR Plugin**: RTL-SDR dongle + librtlsdr (optional, for direct RF reception)
  - **External**: TNC, network feed, or file input

Building
--------

The plugins are built as part of Navit's build system.

Core APRS Plugin (Required)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Ensure SQLite3 is available:

.. code-block:: bash

   sudo apt-get install libsqlite3-dev  # Debian/Ubuntu

The core ``aprs`` plugin will be automatically built when Navit is compiled with plugin support enabled.

SDR Plugin (Optional)
~~~~~~~~~~~~~~~~~~~~~

For RTL-SDR support, install librtlsdr:

.. code-block:: bash

   # Debian/Ubuntu
   sudo apt-get install librtlsdr-dev

   # Or build from source
   git clone https://github.com/librtlsdr/librtlsdr.git
   cd librtlsdr
   mkdir build && cd build
   cmake ..
   make
   sudo make install

The ``aprs_sdr`` plugin will be built automatically if librtlsdr is found. If not found, the plugin is skipped (APRS plugin continues to work with NMEA/other sources).

Android Build
~~~~~~~~~~~~~

For Android builds with SDR support:
- NDK is required for native code compilation
- Android USB Host API is automatically integrated
- USB permissions must be requested at runtime (handled by plugin)

Configuration
-------------

Add the APRS map to your Navit configuration:

.. code-block:: xml

   <map type="aprs" data="/path/to/aprs.db">
       <attribute name="timeout" value="5400"/>
       <attribute name="distance" value="50000"/>
       <attribute name="position_coord_geo" lat="37.7749" lng="-122.4194"/>
   </map>

**Note**: Settings can be changed at runtime via the GUI menu - no restart required!

Configuration Attributes
~~~~~~~~~~~~~~~~~~~~~~~~

- **data** (string): Path to SQLite database file (optional, defaults to in-memory)
- **timeout** (integer): Station expiration time in seconds (default: 5400 = 90 minutes)
- **distance** (double): Maximum range in meters (0 = no limit, default: 0)
- **position_coord_geo** (coord_geo): Center point for range filtering

**Note**: All these settings can also be configured via the GUI menu (see Menu Configuration below).

APRS Packet Processing
-----------------------

The APRS plugin processes packets through the ``aprs_process_packet()`` function. Packets can be fed from multiple sources:

1. SDR Plugin (aprs_sdr) - Recommended for Direct RF Reception
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``aprs_sdr`` plugin provides direct RTL-SDR hardware access with built-in Bell 202 demodulation:

- **Automatic Discovery**: SDR plugin automatically finds and connects to APRS plugin
- **Hardware Support**: RTL-SDR Blog V3, V4 R828D, Nooelec, Generic RTL2832U
- **Bell 202 Demodulation**: Built-in 2FSK demodulation (1200 bps) - no external tools needed
- **Android Support**: Native Android USB Host API integration
- **Configuration**: Frequency, gain, and PPM correction via GUI menu

**Setup:**
1. Connect RTL-SDR dongle to USB port (or USB OTG on Android)
2. Load both ``aprs`` and ``aprs_sdr`` plugins in Navit configuration
3. Select SDR device type in APRS plugin menu
4. Set frequency for your region (see International Frequencies below)

2. NMEA Serial Input
~~~~~~~~~~~~~~~~~~~~

Direct NMEA sentence input via USB serial adapters:
- Supports $GPWPL, $PGRMW, $PMGNWPL, $PKWDWPL sentence formats
- Configurable baud rate (4800, 9600, 19200, 38400) and parity (None, Even, Odd) via GUI menu
- Works on all platforms (Linux, Windows, macOS, Android)
- See Android USB Serial section below for Android compatibility

3. External Packet Sources
~~~~~~~~~~~~~~~~~~~~~~~~~~

You can also feed packets directly to the plugin:
- **TNC (Terminal Node Controller)**: Direct serial/USB connection
- **Network Feed**: APRS-IS or other network sources
- **File Input**: Pre-recorded packet files
- **Custom Sources**: Use ``aprs_register_packet_source()`` API

Inter-Plugin Communication
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The SDR plugin automatically discovers the APRS plugin and registers a packet delivery callback. This happens transparently - no manual configuration needed. If the SDR plugin loads before the APRS plugin, it will retry discovery.

SDR Plugin Device Selection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The SDR plugin supports device selection via the APRS plugin menu:
- **RTL-SDR Blog V3** (RTL2832U + R820T2) - Recommended for best performance
- **V4 R828D** (RTL2832U + R828D) - Latest generation, excellent sensitivity
- **Nooelec dongles** (NESDR series) - Budget-friendly option
- **Generic RTL2832U** - Fallback for other compatible dongles

Device type can be selected in the plugin menu, or the plugin will auto-detect based on USB device strings.

Android USB Serial Compatibility (NMEA Input)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For NMEA serial input on Android devices, you need USB serial adapters with proper chip support:

What You Need for Android Compatibility
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**1. USB OTG Support**
- Your Android device must support USB OTG (On-The-Go)
- You can use Prolific PL2303 or FTDI RS232 to USB adapters via an OTG adapter that provides a USB port for connection
- Most modern Android phones/tablets support OTG

**2. The Right Chip - FTDI is Best**
- **FTDI chips** support Android along with Windows, Mac, and Linux - **Recommended**
- **Avoid Prolific chips** - they have driver and counterfeit issues
- **CH-370 chips** also work OK (cheaper alternative)
- **Opto-isolation** for protection (in marine/professional models)

**3. Driver Support**
- The ``usb-serial-for-android`` library provides driver support for:
  - CDC (Communication Device Class)
  - FTDI
  - Arduino
  - Other USB serial devices using Android USB Host Mode (OTG)

**Android Setup Steps:**
1. Verify USB OTG support on your device (use "USB OTG Checker" app from Play Store)
2. Connect USB serial adapter via USB OTG cable
3. Install USB serial driver app if needed (many modern Android versions have built-in support)
4. Configure the APRS plugin's NMEA settings:
   - Select "NMEA Serial" as input device in the plugin menu
   - Set baud rate (4800, 9600, 19200, or 38400)
   - Set parity (None, Even, or Odd)
   - Set device path (typically ``/dev/ttyUSB0`` or similar)

**Recommended USB Serial Adapters for Android:**
- FTDI FT232RL-based adapters (best compatibility)
- CH340/CH341-based adapters (budget option, works well)
- Avoid: Prolific PL2303 (driver issues, counterfeit problems)

**Troubleshooting Android USB Serial:**
- If device not detected, try different USB OTG cable
- Ensure USB debugging is enabled (Settings → Developer Options)
- Some devices may need "USB Host Controller" app
- Check device power management settings (prevent USB sleep)
- Verify chip type matches Android driver support

International Frequencies
--------------------------

APRS uses different frequencies in different regions. Configure your receiver/TNC for the appropriate frequency for your region.

Primary 2-Meter (VHF) Frequencies
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-------------+------------------------------------------------------------------+
| Frequency   | Region/Countries                                                 |
+=============+==================================================================+
| 144.390 MHz | North America (USA, Canada), Colombia, Chile, Indonesia,        |
|             | Malaysia, Singapore, Thailand                                    |
+-------------+------------------------------------------------------------------+
| 144.575 MHz | New Zealand                                                      |
+-------------+------------------------------------------------------------------+
| 144.640 MHz | China, Taiwan, Japan (main island)                               |
+-------------+------------------------------------------------------------------+
| 144.660 MHz | Japan (Kyushu area)                                              |
+-------------+------------------------------------------------------------------+
| 144.800 MHz | Europe (unified), South Africa, Russia                          |
+-------------+------------------------------------------------------------------+
| 144.930 MHz | Argentina, Uruguay, Panama                                        |
+-------------+------------------------------------------------------------------+
| 144.990 MHz | Vancouver, Canada (secondary - for handhelds)                    |
+-------------+------------------------------------------------------------------+
| 145.175 MHz | Australia (nationwide)                                           |
+-------------+------------------------------------------------------------------+
| 145.570 MHz | Brazil                                                           |
+-------------+------------------------------------------------------------------+

European Countries on 144.800 MHz
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following European countries all use **144.800 MHz** as standardized by IARU Region 1:

Austria, Belgium, Denmark, Finland, France, Greece, Hungary, Italy, The Netherlands, Norway, Portugal, Slovenia, Spain, Sweden, Switzerland, Turkey, and the United Kingdom.

Configuration Notes
~~~~~~~~~~~~~~~~~~~

- The plugin itself does not handle frequency selection - this is configured in your SDR plugin or hardware.
- Ensure your demodulator (multimon-ng, direwolf, etc.) is tuned to the correct frequency for your region
- Some regions may have secondary frequencies for specific use cases (e.g., handheld operations)

Frequency Reference
~~~~~~~~~~~~~~~~~~~

For complete worldwide APRS frequency information by region, consult these external resources:

- **APRS.fi Frequency Guide**: https://aprs.fi/info/frequencies.html - Comprehensive worldwide frequency list
- **IARU Region 1 VHF Manager's Handbook**: Official frequency allocations for Europe, Africa, and Middle East
- **APRS Protocol Specification**: http://www.aprs.org - Official APRS protocol documentation
- **Local Amateur Radio Regulations**: Check with your national amateur radio organization for region-specific frequencies

**Note**: Always ensure you have proper licensing and authorization before transmitting on amateur radio frequencies. This plugin is for receive-only operation unless you have appropriate licenses and permissions.

Database Schema
---------------

The plugin creates the following SQLite schema:

.. code-block:: sql

   CREATE TABLE stations (
       callsign TEXT PRIMARY KEY,
       latitude REAL NOT NULL,
       longitude REAL NOT NULL,
       altitude REAL,
       course INTEGER,
       speed INTEGER,
       timestamp INTEGER NOT NULL,
       comment TEXT,
       symbol_table TEXT,
       symbol_code TEXT,
       path TEXT
   );

Menu Configuration
------------------

The APRS plugin provides a comprehensive menu interface accessible from Navit's settings menu:

Accessing the Menu
~~~~~~~~~~~~~~~~~~

1. Open Navit's settings menu
2. Select "APRS"
3. Choose from submenus:
   - **Input Device**: Select input source (RTL-SDR Blog V3, V4 R828D, Nooelec, Generic RTL-SDR, or NMEA Serial)
   - **Frequency**: Select RX frequency (144.390, 144.800, 145.175 MHz, etc.) - for RTL-SDR only
   - **NMEA Settings**: Configure NMEA serial input (Baud Rate, Parity) - for NMEA input only
   - **Station Timeout**: Set expiration time (30, 60, 90, 120, 180 minutes)
   - **Clear Expired Now**: Manually remove expired stations from database

Menu Options
~~~~~~~~~~~~

Frequency Selection
^^^^^^^^^^^^^^^^^^^

- 144.390 MHz (North America)
- 144.800 MHz (Europe)
- 145.175 MHz (Australia)
- 144.575 MHz (New Zealand)
- 144.640 MHz (China/Japan)
- 144.930 MHz (Argentina/Uruguay)
- 145.570 MHz (Brazil)

Input Device Selection
^^^^^^^^^^^^^^^^^^^^^^

- **RTL-SDR Blog V3**: RTL-SDR Blog V3 dongle (RTL2832U + R820T2)
- **V4 R828D**: V4 R828D dongle (RTL2832U + R828D)
- **Nooelec**: Nooelec dongles (NESDR series)
- **Generic RTL-SDR**: Generic RTL2832U-based dongles
- **NMEA Serial**: USB serial adapter for NMEA sentence input

NMEA Settings (for NMEA Serial input)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **Baud Rate**: 4800, 9600, 19200, or 38400 baud
- **Parity**: None, Even, or Odd

Station Timeout
^^^^^^^^^^^^^^^

- 30 minutes (for frequently updating moving objects)
- 60 minutes
- 90 minutes (default, recommended for mixed traffic)
- 120 minutes
- 180 minutes (for stationary objects with infrequent updates)

**Timeout Guidelines:**
- **Moving objects**: Report every few seconds to minutes → Use 30-60 minutes
- **Stationary objects**: Report every 10-30 minutes → Use 90-180 minutes
- **Mixed traffic**: Use 90 minutes (default)

Performance Considerations
---------------------------

- **Update Interval**: Default 1 second map refresh
- **Expiration Check**: Default 60 seconds
- **Range Filtering**: Applied before item creation to reduce processing
- **Database**: In-memory database for maximum performance (use file for persistence)

Troubleshooting
---------------

No stations appearing
~~~~~~~~~~~~~~~~~~~~~

1. Verify packets are being received and decoded
2. Check database path is accessible
3. Verify range filtering settings (150 km maximum, centered on device)
4. Check expiration timeout settings
5. Ensure APRS plugin is loaded and active

Stations not updating
~~~~~~~~~~~~~~~~~~~~~

1. Ensure ``aprs_process_packet()`` is being called
2. Check packet decoding is successful
3. Verify database updates are working
4. Check if SDR plugin is delivering frames (if using SDR)

Performance issues
~~~~~~~~~~~~~~~~~~

1. Reduce update frequency (default 1 second)
2. Range filtering is always enabled (150 km max)
3. Use in-memory database for speed (omit data attribute)
4. Reduce expiration timeout

SDR plugin not working
~~~~~~~~~~~~~~~~~~~~~~

1. Verify RTL-SDR device is connected and recognized
2. Check USB permissions (Linux: add user to plugdev group)
3. Verify librtlsdr is installed and detected
4. Check if APRS plugin is loaded (SDR plugin requires it)
5. Review debug logs for initialization errors

APRS Symbols
------------

The plugin integrates with the hessu/aprs-symbols symbol set for high-quality APRS station icons. Symbols are bundled with the plugin and installed automatically during the Navit build process.

**Station names (callsigns) are displayed as labels** next to each APRS station icon on the map.

APRS stations use the ``type_poi_communication`` POI type and will display with the default communication POI icon defined in your Navit layout.

Unit Tests
----------

Comprehensive unit tests are available in the ``tests/`` directory:

- ``test_aprs_db.c`` - Database operation tests
- ``test_aprs_decode.c`` - Packet decoding tests
- ``test_aprs_dsp.c`` - DSP operation tests (SDR plugin)

Build and run tests:

.. code-block:: bash

   cmake .. -DBUILD_APRS_TESTS=ON
   make test_aprs_db test_aprs_decode test_aprs_dsp
   ./build/navit/plugin/aprs/tests/test_aprs_db

References
----------

- APRS Protocol Specification: http://www.aprs.org
- Navit Documentation: http://wiki.navit-project.org
- SQLite Documentation: https://www.sqlite.org/docs.html
- IARU Region 1 VHF Manager's Handbook
- Direwolf APRS software: https://github.com/wb2osz/direwolf

Possible Future Enhancements
-----------------------------

- Compatibility with NA7Q-APRSdroid (https://github.com/9M2PJU/NA7Q-APRSdroid) for enhanced Android APRS integration and position sharing
- Display APRS messages, some APRS users sends out the message QRV 144.625 MHz as a example. That means that they are listening on that frequency. If a radio is attached to the device running NAVIT, use hamlib to tune the radios VFO to the frequency displayed in the received message.
- Route to the APRS station, clicking on the APRS stations icon creates a route to it. 
- Compatibility with https://repeatermap.de/, a database over radio amateur repeaters.
- Compatibility with POI's like members of this network: https://www.openstreetmap.org/relation/18780801#map=18/61.897568/9.283834, a repeater like this one typically has a 50 kilometers range: https://www.openstreetmap.org/node/2641537344#map=19/60.808619/9.615892
One could potentially display a pop up when entering the cover area, with the possibility to tune one of the vfo's to the correct frequency and subtone by using libham.


