APRS and APRS SDR Plugin Internals
==================================

Overview
--------

The APRS plugin stack in Navit is split into three main parts:

* The core APRS map plugin (``navit/plugin/aprs/aprs.c`` and helpers) which
  manages stations, the SQLite cache and map items.
* The APRS input backends:

  * NMEA input (``navit/plugin/aprs/aprs_nmea.c``) for serial/TTY based APRS feeds.
  * SDR based input (``navit/plugin/aprs_sdr/*.c``) which demodulates Bell 202
    and hands decoded AX.25 frames to the APRS core.

* Supporting modules:

  * AX.25 / APRS decoder (``navit/plugin/aprs/aprs_decode.c``)
  * Station database (``navit/plugin/aprs/aprs_db.c``)
  * Symbol lookup (``navit/plugin/aprs/aprs_symbols.c``)
  * OSD/menu integration (``navit/plugin/aprs/aprs_osd.c``)


High level data flow
--------------------

1. **Signal / packet source**

   * For NMEA: a serial device configured via the ``device`` attribute
     (``nmea,...``) feeds NMEA sentences into ``aprs_nmea_start()`` /
     ``aprs_nmea_thread()``.
   * For SDR: an RTL-SDR device is opened in ``aprs_sdr_hw_new()`` and samples
     are streamed via ``aprs_sdr_hw_read_thread()`` into the DSP code.

2. **Demodulation / parsing**

   * SDR:

     * ``aprs_sdr_hw_data_callback()`` receives I/Q samples from the RTL-SDR
       hardware (center tuned slightly above the APRS channel) and passes them
       to ``aprs_sdr_dsp_process_samples()``.
     * ``aprs_sdr_dsp_process_samples()``:

       - Applies complex DC blocking and digitally mixes the APRS channel from
         the RF offset down to baseband.
       - Runs an FM discriminator to recover a Bell 202 audio stream at an
         audio-rate sample frequency.
       - Uses Goertzel filters, bit timing and NRZI decoding to recover AX.25
         bits and frames.
       - Calls ``aprs_sdr_frame_delivery_callback()`` with decoded AX.25
         frames.

   * NMEA:

     * ``aprs_nmea_thread()`` reads from the configured TTY and builds complete
       lines, passing each sentence to ``process_nmea_line()``.
     * ``aprs_nmea_parse_sentence()`` and its helpers
       (``parse_gpwpl()``, ``parse_pgrmw()``, ``parse_pmgnwpl()``,
       ``parse_pkwdwpl()``) fill an ``aprs_station`` which is then reported via
       the NMEA callback into the APRS core.

3. **APRS core processing**

   * For AX.25 frames (SDR path):

     * ``aprs_decode_ax25()`` (``aprs_decode.c``) decodes AX.25 headers
       (destination, source, path) and extracts the information field.
     * ``aprs_parse_position()`` and helpers interpret the APRS position payload
       into an ``aprs_position`` (lat/lon, symbol, comment, course/speed).
     * ``aprs_process_packet()`` (``aprs.c``) combines this data into an
       ``aprs_station`` and updates the database via
       ``aprs_db_update_station()``.

   * For NMEA stations:

     * The NMEA backend directly constructs an ``aprs_station`` and calls
       back into the APRS core via ``aprs_nmea_callback()`` where
       ``aprs_db_update_station()`` is also used.

4. **Database and map items**

   * ``aprs_db_update_station()`` stores or updates stations in SQLite.
   * ``aprs_update_items()`` (``aprs.c``) reads all relevant stations
     (respecting center, range and expiration) and creates Navit ``item``s with
     attributes such as label, timestamp, speed, direction and icon.
   * ``aprs_symbol_get_icon()`` (``aprs_symbols.c``) maps APRS symbol table and
     code to an icon filename used for the map marker.

5. **User interface and configuration**

   * ``aprs_osd_new()`` and the command table in ``aprs_osd.c`` register menu
     commands (frequency selection, timeouts, device type, baud rate, parity).
   * These commands call helpers such as ``aprs_osd_set_frequency()``,
     ``aprs_osd_set_timeout()``, ``aprs_osd_set_device()``,
     ``aprs_osd_set_nmea_baud()`` and ``aprs_osd_set_nmea_parity()`` which in
     turn use ``map_set_attr()`` on the APRS map. The map plugin then processes
     those attributes in ``aprs_map_set_attr()``.


Key functions to look at when debugging
---------------------------------------

This section lists the most important entry points when debugging the APRS and
APRS SDR plugins, similar in spirit to the driver_break* helpers.

APRS map plugin (core)
~~~~~~~~~~~~~~~~~~~~~~

* ``aprs_map_new()`` (``aprs.c``)

  * Creates the APRS map instance, opens the SQLite database, initializes
    symbol lookup and schedules periodic expire/update callbacks.

* ``aprs_map_set_attr()`` (``aprs.c``)

  * Central dispatcher for runtime configuration via Navit attributes
    (frequency, distance, timeout, position, device, data, baudrate).
  * Calls:

    * ``aprs_handle_frequency_attr()``
    * ``aprs_handle_distance_attr()``
    * ``aprs_handle_timeout_attr()``
    * ``aprs_handle_position_attr()``
    * ``aprs_handle_device_attr()``
    * ``aprs_handle_data_attr()``
    * ``aprs_handle_baudrate_attr()``

* ``aprs_process_packet()`` (``aprs.c``)

  * Main AX.25/APRS packet handler.
  * Calls ``aprs_decode_ax25()``, ``aprs_parse_position()``,
    ``aprs_db_update_station()`` and then forces a map refresh via
    ``aprs_update_items()``.

* ``aprs_update_items()`` (``aprs.c``)

  * Rebuilds all map items from database contents (range, expiry and center
    filtering), assigns attributes and icons and notifies the GUI via the
    callback list.

* ``aprs_expire_callback()`` / ``aprs_update_callback()`` (``aprs.c``)

  * Periodic timers that delete expired stations and refresh the visible map
    items.


APRS decoder and database
~~~~~~~~~~~~~~~~~~~~~~~~~

* ``aprs_decode_ax25()`` (``aprs_decode.c``)

  * Low level AX.25 decoder: parses destination/source/path and validates
    control/PID; calls:

    * ``decode_ax25_dest_src()``
    * ``decode_ax25_path()``
    * ``decode_ax25_control_pid()``
    * ``aprs_parse_position()``

* ``aprs_parse_position()`` (``aprs_decode.c``)

  * Decodes APRS position reports from the information field using helpers
    like ``parse_ddmm_mm()`` and ``aprs_parse_position_parse_lat_lon()`` and
    fills ``aprs_position`` including symbol, course/speed and comment.

* ``aprs_db_update_station()`` (``aprs_db.c``)

  * Writes/updates a single station row in SQLite, including path and symbol
    information; important when you need to verify that stations are actually
    persisted.

* ``aprs_db_get_stations_in_range()`` / ``aprs_db_get_all_stations()`` (``aprs_db.c``)

  * Read side of the database, including distance filtering and station
    reconstruction, used by ``aprs_update_items()``.


NMEA input backend
~~~~~~~~~~~~~~~~~~

* ``aprs_nmea_start()`` / ``aprs_nmea_thread()`` (``aprs_nmea.c``)

  * Opens the serial device, configures termios and runs the read loop which
    accumulates lines and passes them to ``process_buffer_lines()``.

* ``aprs_nmea_parse_sentence()`` (``aprs_nmea.c``)

  * Entry point for parsing a single NMEA sentence; dispatches to:

    * ``parse_gpwpl()``
    * ``parse_pgrmw()``
    * ``parse_pmgnwpl()``
    * ``parse_pkwdwpl()``

* ``aprs_nmea_callback()`` (``aprs.c``)

  * Bridge from NMEA backend to APRS core: receives a filled ``aprs_station``,
    calls ``aprs_db_update_station()`` and then ``aprs_update_items()``.


APRS SDR plugin (RTL-SDR and DSP)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ``aprs_sdr_map_new()`` (``aprs_sdr.c``)

  * Entry point when the ``aprs_sdr`` map is created.
  * Builds ``aprs_sdr_hw_config`` and ``aprs_sdr_dsp_config`` from attributes,
    creates ``aprs_sdr_hw`` and ``aprs_sdr_dsp`` instances, registers callbacks
    and starts the hardware stream.

* ``aprs_sdr_discover_aprs_plugin()`` (``aprs_sdr.c``)

  * Responsible for finding an APRS map instance and (if available) registering
    a packet callback via ``aprs_register_packet_source``.
  * If this fails, decoded frames cannot reach the APRS core.

* ``aprs_sdr_hw_data_callback()`` (``aprs_sdr.c``)

  * Hardware-to-DSP bridge: receives I/Q samples from the RTL-SDR backend and
    forwards them into ``aprs_sdr_dsp_process_samples()``.

* ``aprs_sdr_frame_delivery_callback()`` (``aprs_sdr.c``)

  * Final step in the SDR pipeline: called by the DSP with a complete AX.25
    frame and forwards it to the APRS core via ``aprs_process_packet()``.

* ``aprs_sdr_hw_new()`` / ``aprs_sdr_hw_start()`` / ``aprs_sdr_hw_read_thread()`` (``aprs_sdr_hw.c``)

  * Low level RTL-SDR setup and I/Q streaming; useful when debugging device
    enumeration or RF configuration issues.

* ``aprs_sdr_dsp_new()`` / ``aprs_sdr_dsp_process_samples()`` (``aprs_sdr_dsp.c``)

  * Bell 202 demodulator; implements Goertzel filters, NRZI decoding and
    AX.25 flag/bit-stuffing handling, then calls the frame callback.


OSD and commands
~~~~~~~~~~~~~~~~

* ``aprs_osd_new()`` (``aprs_osd.c``)

  * Registers all APRS related commands in the command table
    (frequency, timeout, device, baud, parity, settings menu).

* ``aprs_osd_set_frequency()`` / ``aprs_osd_set_timeout()`` /
  ``aprs_osd_set_device()`` / ``aprs_osd_set_nmea_baud()`` /
  ``aprs_osd_set_nmea_parity()`` (``aprs_osd.c``)

  * Translate UI menu actions into ``map_set_attr()`` calls on the APRS map,
    which are then handled by ``aprs_map_set_attr()`` in the core.

