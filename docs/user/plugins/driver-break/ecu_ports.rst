ECU ports and live fuel configuration
=====================================

This document describes how to configure the ports and options used by the
Driver Break plugin for live fuel monitoring from vehicle ECUs:

- OBD-II / ELM327 adapters (cars and light vehicles)
- J1939 over SocketCAN (trucks and heavy vehicles)
- MegaSquirt family ECUs (MS1, MS2, MS3, MS3-Pro, MicroSquirt)

It assumes that the Driver Break plugin itself is already enabled and that the
``driver_break_config`` database has been created (see the main Driver Break
documentation for details).


OBD-II (ELM327) serial adapters
-------------------------------

**Typical hardware and port names**

- USB dongles and USB-serial adapters usually appear as ``/dev/ttyUSB0``,
  ``/dev/ttyUSB1`` and so on.
- On-board serial ports (or some PCIe cards) use ``/dev/ttyS0``, ``/dev/ttyS1``, etc.
- Bluetooth serial profiles (RFCOMM) can appear as ``/dev/rfcomm0``.

The built-in OBD-II backend in the Driver Break plugin expects an
ELM327-compatible adapter. The plugin will, by default, try a suitable
serial port at 38400 or 115200 baud with 8 data bits, no parity and 1 stop bit
(8N1), then initialize the adapter using the standard sequence::

   ATZ        (reset)
   ATE0       (echo off)
   ATL0       (linefeeds off)
   ATH0       (headers off)
   ATS0       (spaces off)
   ATSP0      (automatic protocol)

**Enabling the OBD-II backend**

The OBD-II backend is controlled by the ``fuel_obd_available`` flag in
``driver_break_config``. To use it:

1. Connect the ELM327 adapter to the vehicle OBD-II port and to your Navit host.
2. Determine the serial port name (for example, ``/dev/ttyUSB0``).
3. Ensure that the user running Navit has permission to open that device
   (for example, by being in the ``dialout`` group on most Linux systems).
4. Set ``fuel_obd_available = 1`` in the Driver Break configuration (via GUI
   or directly in the database).

If the adapter is not present or cannot be initialized, the plugin will log
debug messages and fall back to manual/adaptive fuel estimation; other Driver
Break functionality will continue to work.

**Port selection and troubleshooting**

If you have more than one serial device:

- Temporarily unplug the adapter and check the output of ``dmesg`` while
  plugging it back in to see which ``/dev/ttyUSB*`` node it creates.
- Verify that only one application is trying to use the port at a time.
  Sharing a single adapter between multiple processes is not supported.

In case of connection problems:

- Enable Navit debug output for the ``driver_break`` plugin and look for
  messages about ``ATZ`` / ``0100`` failing.
- Confirm that the baud rate and serial settings match your adapter’s
  documentation (most ELM327 clones use 38400 or 115200 8N1).


J1939 over SocketCAN
--------------------

The J1939 backend is intended for trucks and other heavy vehicles which expose
fuel information on their CAN bus. The Driver Break plugin listens on a
SocketCAN interface (for example, ``can0``) for:

- PGN 65266 (FEEA) – Engine fuel rate (SPN 183, 0.05 L/h per bit)
- PGN 65276 (FEF4) – Fuel level (SPN 96, 0.4 % per bit, 0–250 %)

**Preparing the CAN interface**

1. Configure a SocketCAN interface (for example, using a USB-CAN adapter or
   a native CAN controller) and bring it up at the correct bitrate::

      sudo ip link set can0 type can bitrate 250000
      sudo ip link set up can0

2. Verify that frames are received, for example with::

      candump can0

**Enabling the J1939 backend**

- Set ``fuel_j1939_available = 1`` in ``driver_break_config`` when using a
  truck profile.
- Ensure that the interface name used by the backend (typically ``can0``) is
  present and up when Navit starts.

When valid J1939 messages are received, the backend will update
``fuel_rate_l_h`` and ``fuel_current`` based on the configured tank capacity.


MegaSquirt serial backend
-------------------------

The MegaSquirt backend provides native support for the MegaSquirt family of
aftermarket ECUs (MS1, MS2, MS3, MS3-Pro, MicroSquirt). It connects directly
to the ECU over a serial port and periodically requests realtime data.

**Typical hardware and port names**

- USB-to-serial adapters: ``/dev/ttyUSB0``, ``/dev/ttyUSB1``, …
- Built-in serial ports: ``/dev/ttyS0``, ``/dev/ttyS1``, …

By default, the backend uses:

- Device: ``/dev/ttyUSB0``
- Baud rate: 115200
- Mode: 8N1, raw mode (no echo, no line processing)

**Enabling the MegaSquirt backend**

The MegaSquirt backend is controlled by two fields in ``driver_break_config``:

- ``fuel_megasquirt_available`` – set to ``1`` to enable the backend.
- ``fuel_injector_flow_cc_min`` – injector flow rate in cc/min at rated pressure
  (for one injector).

Steps:

1. Connect the MegaSquirt ECU to the Navit host using a supported
   USB-to-serial interface.
2. Determine the device node (for example, ``/dev/ttyUSB0``) and ensure that
   the Navit user can open it.
3. In the Driver Break configuration:

   - Set ``fuel_megasquirt_available = 1``.
   - Set ``fuel_injector_flow_cc_min`` to the injector flow rating from your
     engine/ECU configuration (for example, 250, 300, 440 cc/min).

When enabled, the backend:

- Opens the serial port at 115200 8N1.
- Sends the MegaSquirt realtime command (``A`` followed by carriage return).
- Parses the returned binary block to extract RPM and injector pulse width.
- Converts these values into an approximate fuel rate in L/h using the injector
  flow and a default cylinder count.

If the data are out of range or invalid (for example, impossible RPM or pulse
width values), the update is skipped to avoid polluting the adaptive fuel
learning with bad samples.

**Mutual exclusivity with OBD-II**

OBD-II and MegaSquirt both use serial ports. To prevent conflicts:

- If ``fuel_obd_available = 1``, the plugin will prefer the OBD-II backend and
  will not start the MegaSquirt backend on the same device.
- If you are using MegaSquirt as the primary ECU source, set
  ``fuel_obd_available = 0`` and ``fuel_megasquirt_available = 1``.

This ensures that only one backend attempts to control the serial adapter at a
time.


Summary of configuration flags
------------------------------

The key configuration fields for live fuel data are:

- ``fuel_obd_available`` – enable OBD-II (ELM327) backend when set to ``1``.
- ``fuel_j1939_available`` – enable J1939/SocketCAN backend when set to ``1``.
- ``fuel_megasquirt_available`` – enable MegaSquirt serial backend when set to ``1``.
- ``fuel_injector_flow_cc_min`` – injector flow (cc/min) for MegaSquirt; must
  be greater than 0 for the backend to produce fuel rate values.

All three backends update the same internal fields:

- ``fuel_rate_l_h`` – instantaneous fuel rate in L/h.
- ``fuel_current`` – current fuel in the tank (litres), when a level signal is available.

Energy-based routing and range estimation use these fields in the same way,
regardless of which backend is active.

