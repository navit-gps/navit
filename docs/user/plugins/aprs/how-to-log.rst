How to Log for APRS/APRS SDR Debugging
======================================

This document summarizes how to add and use logging in the APRS and APRS SDR
plugins to make debugging simpler and more consistent.


Logging primitives
------------------

All APRS-related C files already include ``debug.h`` and use the standard Navit
logging macro:

.. code-block:: c

   dbg(level, "message with %d parameters", value);

Common log levels used in the APRS stack:

* ``lvl_error`` – fatal conditions (cannot open database, device, etc.).
* ``lvl_warning`` – non-fatal problems (missing APRS map, lost device, etc.).
* ``lvl_info`` – high-level state changes and events (plugin start/stop,
  successful initialization, decoded frame counts).
* ``lvl_debug`` – detailed flow, arguments, intermediate values.

When adding new logging, prefer **``lvl_info`` for user-visible events** and
**``lvl_debug`` for verbose per-packet or per-sample details**.


Where to log
------------

To keep logs readable, prefer logging at **natural choke points** instead of
every low-level helper:

* **Initialization / teardown**

  * ``aprs_map_new()`` and ``aprs_map_destroy()`` (APRS core).
  * ``aprs_sdr_map_new()`` and ``aprs_sdr_map_destroy()`` (APRS SDR plugin).
  * ``aprs_sdr_hw_new()``, ``aprs_sdr_hw_start()``, ``aprs_sdr_hw_stop()``.
  * ``aprs_sdr_dsp_new()`` (Bell 202 DSP configuration).
  * ``aprs_nmea_start()`` / ``aprs_nmea_stop()`` (NMEA device).

* **Configuration changes**

  * ``aprs_map_set_attr()`` and its helpers:

    * ``aprs_handle_frequency_attr()``
    * ``aprs_handle_distance_attr()``
    * ``aprs_handle_timeout_attr()``
    * ``aprs_handle_position_attr()``
    * ``aprs_handle_device_attr()``
    * ``aprs_handle_data_attr()``
    * ``aprs_handle_baudrate_attr()``

  * OSD helpers in ``aprs_osd.c`` that map menu actions to attributes:

    * ``aprs_osd_set_frequency()``
    * ``aprs_osd_set_timeout()``
    * ``aprs_osd_set_device()``
    * ``aprs_osd_set_nmea_baud()``
    * ``aprs_osd_set_nmea_parity()``

* **Packet and station processing**

  * ``aprs_sdr_frame_delivery_callback()`` – frame length, success/failure
    of forwarding to the APRS core.
  * ``aprs_decode_ax25()`` – basic source/destination, length, and decode
    success.
  * ``aprs_parse_position()`` – whether a position was parsed and key fields
    (lat, lon, symbol table/code) at ``lvl_debug``.
  * ``aprs_process_packet()`` – station created/updated, timestamp selection.
  * ``aprs_db_update_station()`` – callsign, lat/lon and SQLite return code.
  * ``aprs_update_items()`` – number of stations rendered and filtered.

* **Input backends**

  * NMEA (``aprs_nmea.c``):

    * ``aprs_nmea_parse_sentence()`` – accepted/rejected sentences and type
      (``GPWPL``, ``PGRMW``, etc.).
    * ``read_nmea_data()`` – I/O errors at ``lvl_error``.

  * SDR (``aprs_sdr_hw.c``, ``aprs_sdr_dsp.c``):

    * Device count and selected index.
    * RF configuration (frequency, sample rate, gain, PPM).
    * DSP initialization (mark/space frequencies, audio rate, nominal samples per bit).
    * When debugging demodulation: discriminator/DC/PLL state (phase, lock) at ``lvl_debug``.
    * Number of frames decoded per call (use ``lvl_debug`` to avoid noise).


Example patterns
----------------

**High-level event at ``lvl_info``**

.. code-block:: c

   dbg(lvl_info, "APRS SDR plugin started: %.3f MHz", hw_config.frequency_mhz);

Use this style for:

* Plugin start/stop.
* Device opened/closed.
* NMEA input started/stopped.
* Major configuration changes (frequency, timeout, device type, baud rate).


**Detailed flow at ``lvl_debug``**

.. code-block:: c

   dbg(lvl_debug, "aprs_db_update_station: callsign='%s', pos=(%f, %f)",
       station ? station->callsign : NULL,
       station ? station->position.lat : 0.0,
       station ? station->position.lng : 0.0);

Use this for:

* Function entry/exit when diagnosing a specific bug.
* Intermediate values that are only needed during deep debugging.
* Per-row or per-frame information that might be too noisy otherwise.


How to enable logging at runtime
--------------------------------

Navit logging levels are controlled via the configuration (``navit.xml``) and
command line. For APRS debugging you typically want at least:

* ``lvl_info`` for regular operation.
* ``lvl_debug`` temporarily when investigating issues.

Typical workflow:

1. Start with ``lvl_info`` for APRS categories to see high-level events:

   * APRS map creation and destruction.
   * APRS SDR plugin initialization and frame delivery.
   * NMEA device start/stop and configuration.

2. If more detail is needed, temporarily enable ``lvl_debug`` for APRS-related
   modules (``aprs``, ``aprs_sdr``, ``aprs_nmea``, ``aprs_db``) and reproduce
   the issue.

3. Once the problem is understood, **remove or downgrade** any overly verbose
   logs to keep the default log output readable.


Guidelines
----------

* Keep messages **short, precise and structured** – include callsign, frequency
  or device names when relevant.
* Avoid logging inside tight inner loops unless strictly necessary; prefer
  logging aggregated information at higher levels.
* Use ``lvl_error`` only for conditions that require user or developer
  attention; reserve ``lvl_warning`` for recoverable issues.
* For new code, follow the existing style of the APRS and APRS SDR modules to
  keep the log output consistent across the project.

