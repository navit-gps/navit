EV backend — to-do
==================

**Repository (same document):** `todo-electric.rst on GitHub`__

.. __: https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/todo-electric.rst

This page tracks planned work for an electric-vehicle (EV) energy backend in the
Driver Break plugin. It is a design checklist, not end-user documentation.

Research findings (survey)
--------------------------

Sources and roles
~~~~~~~~~~~~~~~~~

- **commaai/opendbc** (`OpenDBC <https://github.com/commaai/opendbc>`__) —
  **Raw CAN** DBC files and car ports (`opendbc/dbc
  <https://github.com/commaai/opendbc/tree/master/opendbc/dbc>`__,
  `docs/CARS.md <https://github.com/commaai/opendbc/blob/master/docs/CARS.md>`__).
  Coverage is **signal-oriented** (CAN IDs, factors, signed fields), not a catalog
  of OBD-II PIDs. Deepest for vehicles with an openpilot-style port. Use for
  **SocketCAN / panda**-class access, not ELM327 request/response alone.

- **evDash** (`repository <https://github.com/nickn17/evDash>`__) — Firmware for
  dev boards using **OBD2 (BLE) and/or CAN**. Per-model behavior is defined in
  the project sources; use it to see what that stack reads, not as a global PID
  database.

- **EVNotify** (`app <https://github.com/EVNotify/EVNotify>`__,
  `evnotify.com <https://evnotify.com>`__) — Android app with **BLE OBD dongle**
  support. **Full** vs **basic** vehicle lists reflect app validation (SoC,
  charging), not a public DBC.

- **iternio/ev-obd-pids** (`ev-obd-pids <https://github.com/iternio/ev-obd-pids>`__) —
  Consolidates community **OBD PID** lists (init commands, protocol 6/7,
  equations similar to Torque) for **A Better Routeplanner**-style telemetry.
  Best single place for **per-model ELM dongle** PID coverage across brands.

Makes/models with stronger community evidence
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- **Nissan Leaf** — Long history of reverse-engineered **OBD** manufacturer PIDs
  and CAN docs: `Leaf OBD-II manual <https://leaf-obd.readthedocs.io/>`__,
  `dalathegreat/leaf_can_bus_messages <https://github.com/dalathegreat/leaf_can_bus_messages>`__,
  and forum threads. **Generation matters** (ZE0 vs AZE0 vs ZE1).

- **Hyundai / Kia (e.g. Ioniq EV, Kona EV, Niro EV, Soul EV)** — HKMC-focused PID
  collections (Torque/OBD spreadsheets), **EVNotify** full support for several
  models, and **OpenDBC** HKMC CAN work for ADAS (distinguish OBD path vs raw bus).

**Safest starting points for implementation and testing:** Leaf plus one HKMC EV
(e.g. Ioniq Electric, Kona Electric, or Niro EV), matching community docs and
EVNotify’s better-supported list.

ELM327 vs raw CAN (SocketCAN / panda)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- **ELM327-class dongles:** ISO 15765-4 OBD-II on CAN (functional addressing),
  **Mode 01** standard PIDs, **Mode 09**, often **Mode 22** (manufacturer) if the
  firmware handles multi-frame responses. Suited to **request/response** SoC,
  power, and temps **when the ECU exposes them on that path**. Throughput and
  timing are limited.

- **Raw SocketCAN / comma panda:** Full bus access, arbitrary IDs, rates suitable
  for **OpenDBC** DBC decoding. **Required** when signals exist only on powertrain
  CAN frames **not** reachable via standard OBD requests.

**Rule of thumb:** If a profile appears in **ev-obd-pids** / Torque as explicit
OBD requests, assume **ELM-capable** (adapter quality permitting). If a signal
exists only as a **DBC signal** on a non-OBD CAN ID, assume **raw CAN** hardware.

Motor current: scaling and regen sign
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- **DBC** is authoritative for raw CAN: ``factor``, ``offset``, signedness per signal.

- **OBD PID spreadsheets** use per-equation decoding (e.g. ``Signed()``,
  ``Int16(A,B)`` in `ev-obd-pids <https://github.com/iternio/ev-obd-pids>`__).
  **Regen sign is not standardized across OEMs** (negative = regen vs offset
  zero vs magnitude-only). Document **per make/model** (bytes, endianness,
  unit).

State of charge vs standard PID 0x2F
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- **SAE J1979:** **0x2F** is **Fuel Tank Level Input** (percentage of fuel tank),
  typically scaled as ``A * 100 / 255``.

- Battery **SoC** is **not** globally mapped to **0x2F**. Hybrids/EVs often use
  other PIDs (e.g. **0x5B** “Hybrid Battery Pack Remaining Life” in general OBD
  references), plus **Mode 22** manufacturer-specific PIDs.

- **Do not assume** SoC **overlaps** 0x2F on BEVs; **verify per model** with logs
  or community profiles (EVNotify, Torque, ev-obd-pids).

Implementation to-do (unchanged scope)
--------------------------------------

Research and PID mapping
~~~~~~~~~~~~~~~~~~~~~~~~

- [ ] Survey OpenDBC, evDash, and EVNotify for the most complete per-model PID coverage
- [ ] Identify which makes/models have reliable community-verified PIDs (Nissan Leaf and Hyundai Ioniq / Kia e-Niro class are the safest starting points)
- [ ] Determine which PIDs are realistically readable via ELM327 vs requiring raw SocketCAN access
- [ ] Document scaling factors and sign conventions for motor current (regen negative vs positive varies by manufacturer)
- [ ] Investigate whether any SoC PIDs overlap with the existing standard PID 0x2F across common EV models

Vehicle profile table
~~~~~~~~~~~~~~~~~~~~~

- [x] Design the schema: make, model, year range, SoC PID, power/current PID, battery temp PID, battery voltage PID, protocol hint — see :doc:`ev_vehicle_profile` and ``ev_vehicle_profile.sql`` (documentation only; not wired in C yet).
- [ ] Decide storage format — database table alongside existing ``driver_break_config``, or a separate config file
- [ ] Define fallback behavior when a vehicle is not in the table (adaptive estimation, same as existing backends)

New internal fields
~~~~~~~~~~~~~~~~~~~

- ``battery_soc_pct`` — replaces ``fuel_current`` role for range anchoring
- ``power_kw`` — signed, replaces ``fuel_rate_l_h``; negative value = regenerating
- ``battery_temp_c`` — optional but important for cold-weather range warnings
- ``battery_voltage`` — optional, sanity checking
- ``battery_capacity_kwh`` — user-configured, needed for range math. Optionally look for a battery health PID if one exists

ELM327 / OBD-II backend extension
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- [ ] Add ``ev_obd_available`` flag to ``driver_break_config``
- [ ] Implement mutual exclusivity with combustion OBD-II backend (same adapter, different PIDs)
- [ ] Handle 29-bit CAN addressing where required (some EVs need ``AT CAF1`` / ``ATSH`` header commands beyond standard ELM327 init sequence)
- [ ] Add protocol hints to vehicle profile so the correct ELM327 init variant is selected per model

Energy-based routing integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- [ ] Feed real ``power_kw`` readings into the existing recuperation model instead of relying solely on elevation estimates
- [ ] Build per-segment regen yield learning into the adaptive system — elevation drop vs actual regen recovered, per vehicle
- [ ] Use ``battery_temp_c`` to apply a capacity derating factor in range estimates when battery is cold

User-facing additions
~~~~~~~~~~~~~~~~~~~~~

- [ ] Cold battery warning when ``battery_temp_c`` is below a configurable threshold
- [ ] Regen indicator in OSD — show when ``power_kw`` is negative
- [ ] Range estimate based on ``battery_soc_pct`` × ``battery_capacity_kwh`` ÷ rolling average ``power_kw``

Documentation
~~~~~~~~~~~~~

- [ ] New RST page following ``ecu_ports.rst`` style covering the vehicle profile table, EV-specific config flags, and supported models
- [ ] Update ``index.rst`` fuel monitoring section to include the EV backend
- [ ] Note limitations clearly — unlisted models fall back to adaptive estimation, no guarantees on PID accuracy across model years

Testing
~~~~~~~

- [ ] Verify against at least two real vehicles before merging (Nissan Leaf and one of Ioniq / e-Niro recommended as starting points given community PID documentation quality)
- [ ] Test cold battery derating with logged data if live testing is not possible
- [ ] Test fallback behavior with an unlisted vehicle profile
