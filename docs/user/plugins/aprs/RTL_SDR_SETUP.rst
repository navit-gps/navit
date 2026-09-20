RTL-SDR Setup Guide for APRS Plugin
===================================

This guide covers setup and configuration for RTL-SDR dongles with the Navit APRS plugin.

Supported RTL-SDR Devices
-------------------------

The plugin is compatible with all RTL-SDR devices, including:

- RTL-SDR Blog V3 (RTL2832U + R820T2)
- V4 R828D (RTL2832U + R828D)
- Nooelec dongles (various models: NESDR, NESDR Mini, NESDR SMArt, NESDR XTR, etc.)
- Generic RTL2832U-based dongles

All these devices use the same driver stack and are fully compatible.

Quick Driver Installation Reference
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

================= =============================== =======================================
Platform          Driver Method                    Notes
================= =============================== =======================================
Linux             ``apt-get install rtl-sdr``     Requires DVB-T module blacklisting
Windows           Zadig (WinUSB) or SDR# package  Easiest: use Zadig tool
macOS             ``brew install rtl-sdr``        No kernel module issues
Android           SDR Driver app from Play Store  Requires USB OTG support
================= =============================== =======================================

All devices (RTL-SDR Blog V3, V4 R828D, Nooelec, generic) use the same drivers on each platform.

Hardware Requirements
---------------------

- RTL-SDR USB dongle (one of the models above)
- USB 2.0+ port
- Appropriate antenna for 2-meter band (144–146 MHz)
- USB extension cable (recommended to reduce RFI)

Software Stack
--------------

There are two main ways to get APRS packets into Navit:

1. **Using the built-in APRS SDR plugin (recommended when librtlsdr is available)**:

   ::

      RTL-SDR Dongle (center tuned above APRS channel)
          |
          v
      aprs_sdr_hw.c  (RTL-SDR RF I/Q at RF sample rate, e.g. 192 kHz)
          |
          v
      aprs_sdr_dsp.c (IF downconversion, DC block, FM discriminator, bit PLL, Bell 202)
          |
          v
      AX.25 Frames
          |
          v
      APRS Core Plugin (aprs.c, database + map items)

2. **Using external tools (legacy / alternative pipeline)**:

   ::

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
      Navit APRS Plugin (via custom bridge or network feed)

Installation
------------

Linux (Debian/Ubuntu, Fedora, Arch, etc.)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Method 1: Package Manager (easiest)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # Debian/Ubuntu
   sudo apt-get update
   sudo apt-get install rtl-sdr librtlsdr-dev rtl-sdr-dbg

   # Fedora
   sudo dnf install rtl-sdr rtl-sdr-devel

   # Arch Linux
   sudo pacman -S rtl-sdr

   # Verify installation
   rtl_test -t

Method 2: Build from Source
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

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

Blacklist DVB-T Kernel Modules
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

RTL-SDR dongles are often detected as DVB-T tuners. Blacklist these modules:

.. code-block:: bash

   sudo nano /etc/modprobe.d/blacklist-rtl.conf

Add::

   blacklist dvb_usb_rtl28xxu
   blacklist rtl2832
   blacklist rtl2830

Then reload or unload:

.. code-block:: bash

   sudo rmmod dvb_usb_rtl28xxu rtl2832 rtl2830

Create udev rules (for non-root access):

.. code-block:: bash

   sudo nano /etc/udev/rules.d/20-rtlsdr.rules

Add::

   SUBSYSTEM=="usb", ATTRS{idVendor}=="0bda", ATTRS{idProduct}=="2838", GROUP="plugdev", MODE="0666", SYMLINK+="rtl_sdr"

Reload rules and add user:

.. code-block:: bash

   sudo udevadm control --reload-rules
   sudo udevadm trigger
   sudo usermod -a -G plugdev $USER
   # Log out and log back in

Windows
~~~~~~~

Method 1: Zadig (recommended)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Download Zadig from ``https://zadig.akeo.ie/``.
2. Connect RTL-SDR dongle.
3. Open Zadig → Options → List All Devices.
4. Select "Bulk-In, Interface (Interface 0)" or "RTL2832UHIDIR".
5. Select "WinUSB" as driver, click "Install Driver".
6. Verify with SDR# or similar.

Method 2: SDR# Driver Package
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Download SDR# from ``https://airspy.com/download/`` and follow its installer, which includes drivers.

macOS
~~~~~

Method 1: Homebrew
^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   brew install rtl-sdr
   rtl_test -t

Method 2: Build from Source
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   brew install cmake libusb
   git clone https://github.com/osmocom/rtl-sdr.git
   cd rtl-sdr
   mkdir build && cd build
   cmake ../
   make
   sudo make install
   rtl_test -t

Android
~~~~~~~

Method: SDR Driver App
^^^^^^^^^^^^^^^^^^^^^^

1. Install "SDR driver" by Martin Marinov from Google Play.
2. Connect RTL-SDR via USB OTG cable.
3. Grant USB permissions.
4. Configure frequency, gain, etc. in the app or connect to the rtl_tcp server.

Install APRS Decoding Tools
---------------------------

Linux:

.. code-block:: bash

   sudo apt-get install multimon-ng

   # Or build from source
   git clone https://github.com/EliasOenal/multimon-ng.git
   cd multimon-ng
   mkdir build && cd build
   cmake ../
   make
   sudo make install

macOS:

.. code-block:: bash

   brew install multimon-ng

Verify Device Detection
-----------------------

Linux/macOS:

.. code-block:: bash

   rtl_test -t

You should see something like::

   Found 1 device(s):
     0:  Realtek, RTL2838UHIDIR, SN: 00000001

   Using device 0: Generic RTL2832U OEM

Configuration by Device Type (Built-in SDR)
-------------------------------------------

When using the built-in APRS SDR plugin, you typically configure it only through
Navit, for example:

.. code-block:: xml

   <map type="aprs_sdr">
       <attribute name="frequency" value="144.390"/>  <!-- APRS channel -->
       <attribute name="gain" value="49"/>
       <attribute name="device" value="rtlsdr_blog_v3"/>
   </map>

Internally the plugin:

* Tunes the RTL-SDR center frequency slightly above the APRS channel (for
  example 144.490 MHz for a 144.390 MHz APRS channel) to avoid the DC spike.
* Runs the RTL-SDR at an RF sample rate around 192 kHz by default.
* Downconverts, DC-blocks, FM-demodulates and decimates to an audio stream
  (typically 48 kHz) before Bell 202 demodulation.

The device-specific RF recommendations from librtlsdr and RTL-SDR Blog still
apply (antenna, gain, PPM calibration), but you generally do **not** run
``rtl_fm`` or ``multimon-ng`` yourself when using the built-in plugin.

Integration with Navit APRS Plugin
----------------------------------

The APRS core plugin consumes decoded AX.25 packets. Typical integration approaches:

- Use multimon-ng or direwolf to decode RF into AX.25/APRS text.
- Feed the resulting packets into a small bridge process that calls into the APRS plugin (via C, D-Bus, or another IPC).

Troubleshooting
---------------

Device Not Found (Linux)
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   lsusb | grep RTL
   # Check permissions
   lsmod | grep rtl28
   # Ensure DVB-T modules are unloaded/blacklisted

Poor Reception
~~~~~~~~~~~~~~

- Check antenna.
- Adjust gain.
- Use USB extension cable to move dongle away from RF noise sources.
- Verify correct frequency for your region.

Performance Tuning
------------------

- Lower sample rate (for example, 24000 Hz) if CPU is constrained.
- Pin rtl_fm and multimon-ng to a dedicated CPU core if needed:

  .. code-block:: bash

     taskset -c 0 rtl_fm ...

References
----------

- RTL-SDR Blog: https://www.rtl-sdr.com
- multimon-ng: https://github.com/EliasOenal/multimon-ng
- direwolf: https://github.com/wb2osz/direwolf
- RTL-SDR Library: https://github.com/osmocom/rtl-sdr

Legal Notice
------------

Ensure compliance with local regulations. RTL-SDR dongles are receive-only devices. Always verify you have proper authorization before receiving or processing radio signals.

