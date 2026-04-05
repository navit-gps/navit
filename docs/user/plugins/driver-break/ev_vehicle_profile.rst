.. _ev-vehicle-profile:

EV OBD vehicle profile table (SQLite)
=======================================

This page defines the **SQLite schema** for per-model electric vehicle OBD-II
profiles used by the planned EV backend in the Driver Break plugin (see
:doc:`todo-electric`). **Navit does not create or use this table yet**; the DDL
is specification-only so it can be copied into the plugin when implementation
starts.

**Files**

- Machine-readable DDL: ``ev_vehicle_profile.sql`` (same directory as this page).
- On GitHub: `ev_vehicle_profile.sql <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/ev_vehicle_profile.sql>`__

Purpose
-------

Rows describe **how to read** state of charge, traction power (or motor
current), battery temperature, and pack voltage over an **ELM327-class**
adapter for a given **make, model, and model-year range**. The plugin (when
implemented) can match the user’s vehicle to a row and issue the documented
requests.

Table name
----------

``driver_break_ev_vehicle_profile``

Follows the existing Driver Break database naming style
(``driver_break_*``).

Columns
-------

.. list-table::
   :header-rows: 1
   :widths: 22 12 66

   * - Column
     - Type
     - Meaning
   * - ``id``
     - INTEGER
     - Primary key, ``AUTOINCREMENT``.
   * - ``profile_code``
     - TEXT
     - Short stable identifier (e.g. ``nissan_leaf_ze1``). **UNIQUE.** For
       configuration and logging.
   * - ``make``
     - TEXT
     - Manufacturer name; recommend normalized lowercase for matching (e.g.
       ``nissan``).
   * - ``model``
     - TEXT
     - Model name (e.g. ``leaf``).
   * - ``year_from``
     - INTEGER
     - First model year (**inclusive**). ``NULL`` means “no lower bound”.
   * - ``year_to``
     - INTEGER
     - Last model year (**inclusive**). ``NULL`` means “no upper bound”.
   * - ``soc_request_hex``
     - TEXT
     - **Required.** OBD request payload as **contiguous hexadecimal without
       spaces**: first byte is OBD mode, then PID / manufacturer data bytes.
       Example: mode ``0x22``, PID ``0x015B`` → ``22015B``. Encoding matches
       what the adapter sends after ISO-TP framing (length nibble is **not**
       stored in this column; the implementation will prepend PCI as needed).
   * - ``soc_formula``
     - TEXT
     - Expression to decode SoC (0–100 %) from response bytes **after** the
       service ID byte, Torque / ``ev-obd-pids`` style (e.g. ``A``,
       ``A*100/255``, ``Int16(A,B)``, ``Signed(A)``). Empty or NULL if the
       plugin supplies a default for that PID layout.
   * - ``power_kw_request_hex``
     - TEXT
     - Optional. Same hex format for **traction power** (signed kW, negative =
       regen) or a surrogate signal documented in ``source_note``.
   * - ``power_kw_formula``
     - TEXT
     - Optional decode expression for ``power_kw_request_hex``.
   * - ``battery_temp_request_hex``
     - TEXT
     - Optional. Battery (or coolant) temperature request.
   * - ``battery_temp_formula``
     - TEXT
     - Optional decode (typical unit °C).
   * - ``battery_voltage_request_hex``
     - TEXT
     - Optional. Pack or bus voltage request.
   * - ``battery_voltage_formula``
     - TEXT
     - Optional decode (typical unit V).
   * - ``protocol_hint``
     - INTEGER
     - ELM327 **ATSP** hint: ``0`` = automatic (``ATSP0``), ``6`` = ISO
       15765-4 CAN 11 bit / 500 kbit, ``7`` = 29 bit / 500 kbit, etc. Range
       0–15; see ELM327 documentation for other values.
   * - ``elm_preamble``
     - TEXT
     - Optional extra init commands, **semicolon-separated** (e.g.
       ``AT CAF0;ATSH 7E4``). Applied before periodic polling **only** when
       documented for that vehicle.
   * - ``enabled``
     - INTEGER
     - ``1`` = row may be used, ``0`` = disabled (seed / unverified data).
   * - ``source_note``
     - TEXT
     - Provenance, caveats, forum link, or PID document revision.

Constraints and indexes
-----------------------

- ``CHECK`` rules: ``enabled`` in ``(0,1)``; ``protocol_hint`` in ``0..15``;
  years in ``1900..2100`` when not NULL; ``year_from <= year_to`` when both set.
- Index ``driver_break_ev_vehicle_profile_lookup`` on ``(make, model,
  year_from, year_to)`` for selection by vehicle.
- Index ``driver_break_ev_vehicle_profile_code`` on ``profile_code`` for
  configuration by code.

Lookup semantics (for implementers)
-----------------------------------

#. Normalize **make** / **model** the same way as stored (document convention in
   the plugin).
#. Choose rows where ``enabled = 1`` and calendar year ``Y`` satisfies
   ``(year_from IS NULL OR Y >= year_from)`` and ``(year_to IS NULL OR Y <= year_to)``.
#. If multiple rows match, prefer the **narrowest** year span; then highest
   ``id`` or explicit ``profile_code`` from user config (policy TBD in code).

DDL copy
--------

The authoritative ``CREATE TABLE`` and ``CREATE INDEX`` statements are in
``ev_vehicle_profile.sql``. Do not duplicate divergent DDL in the plugin;
import or paste from that file when adding ``sqlite3_exec`` in
``driver_break_db.c``.

Related documentation
-----------------------

- :doc:`ecu_ports` — current OBD-II / ELM327 behaviour (combustion).
- :doc:`todo-electric` — EV backend checklist and research notes.
