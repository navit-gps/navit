Adding support for other aftermarket ECUs
=========================================

The Driver Break plugin includes native backends for three live fuel sources:

- OBD-II / ELM327 (cars and light vehicles)
- J1939 over SocketCAN (trucks and heavy vehicles)
- MegaSquirt (MS1, MS2, MS3, MS3-Pro, MicroSquirt)

For other aftermarket ECUs, the recommended approach is to implement a small
backend that reads the ECU's own protocol and writes into the same internal
fields that the built-in backends use:

- ``driver_break_priv.fuel_rate_l_h`` – instantaneous fuel rate in L/h.
- ``driver_break_priv.fuel_current`` – current fuel in the tank (litres), if available.
- ``config.fuel_ethanol_manual_pct`` – ethanol percentage (0–100), for flex-fuel profiles.

No database schema changes are required; the existing adaptive fuel logging,
range estimation and trip summaries will automatically use the new backend.


Design principles for new backends
----------------------------------

When adding support for an aftermarket ECU (for example Haltech, Link ECU, AEM,
ECUMaster), follow these principles:

- **Use the native protocol where possible.** Many ECUs stream data over CAN
  or serial with documented message layouts.
- **Do not re-implement full tuning features.** The backend should be read-only
  from the plugin's point of view: subscribe to or request engine data, never
  change ECU configuration.
- **Produce physically plausible values only.** If the ECU reports clearly
  invalid data (for example, impossible RPM or fuel rate), skip the update
  instead of writing it to the adaptive logs.
- **Avoid hardware conflicts.** Re-use the same mutual-exclusion idea as
  OBD-II vs MegaSquirt: if two backends would need the same serial port or
  CAN interface, only one should be active at a time.


Choosing the transport
----------------------

Common options for connecting to aftermarket ECUs are:

- **Direct CAN** – Many ECUs broadcast a fixed-rate CAN stream with engine
  speed, throttle position, injector pulse width, fuel pressure and so on.
  Example vendors:

  - Haltech (Elite, Nexus) – CAN broadcast, documented for product owners.
  - Link ECU (G4+, G4X, G5) – CAN streams documented in Link manuals.
  - AEM (Infinity, Series 2) – AEMnet over CAN.
  - ECUMaster (EMU, EMU Black, EMU Pro) – CAN output with documented PIDs.

- **Serial / USB-serial** – Some ECUs expose a serial stream with a text or
  binary protocol (similar to MegaSquirt).

- **OBD-II bridge** – Some setups route ECU data through an OBD-II gateway
  so standard OBD-II PIDs can be used; in that case, the existing OBD-II
  backend is usually sufficient.

In all cases, the backend's job is to:

1. Open the appropriate device (SocketCAN interface, serial port, etc.).
2. Read or decode the ECU's messages using the vendor's public documentation.
3. Convert the data into a fuel rate in L/h and, where possible, current fuel.


Mapping ECU channels to Driver Break fields
-------------------------------------------

For most aftermarket ECUs, you will have access to:

- Engine speed (RPM).
- Injector pulse width (ms) and injector size (cc/min).
- Optionally: fuel pressure, fuel density, lambda / AFR, fuel tank level.

From these, you can compute fuel rate in L/h in a similar way to the MegaSquirt
backend, for example:

.. math::

   \text{fuel\_rate\_L\_h} =
      \frac{\text{pw\_ms} \times \text{rpm} \times n_\text{cyl} \times \text{flow\_cc\_min}}{2\,000\,000}

where:

- ``pw_ms`` is injector pulse width in ms.
- ``rpm`` is engine speed.
- ``n_cyl`` is the number of cylinders.
- ``flow_cc_min`` is injector flow per injector in cc/min.

If the ECU provides a direct fuel-rate channel (for example, L/h or g/s), you
can convert it directly:

- For ECU fuel-rate in L/h, simply assign it to ``fuel_rate_l_h``.
- For ECU fuel-rate in g/s, divide by fuel density and convert seconds to
  hours (similar to the OBD-II MAF-based formula).


Configuration and flags
-----------------------

It is recommended to add one configuration flag per new backend in
``driver_break_config``, similar to the existing:

- ``fuel_obd_available``
- ``fuel_j1939_available``
- ``fuel_megasquirt_available``

For example:

- ``fuel_haltech_available``
- ``fuel_linkecu_available``

and so on, depending on which brands you support. Each backend should:

- Check its own flag before starting.
- Respect mutual exclusivity with other backends that use the same hardware.


Examples of vendor protocol documentation
-----------------------------------------

The following vendors provide protocol documentation to product owners:

- **Haltech** (Elite, Nexus): CAN broadcast – see `Haltech <https://www.haltech.com/>`_.
- **Link ECU** (G4+, G4X, G5): CAN stream – see `Link ECU manuals
  <https://linkecu.com/software-support/manuals/>`_.
- **AEM** (Infinity, Series 2): AEMnet CAN – see `AEM Electronics
  <https://www.aemelectronics.com/>`_.
- **ECUMaster** (EMU, EMU Black, EMU Pro): CAN output – see `ECUMaster EMU
  <https://www.ecumaster.com/products/emu/>`_.

Use these documents to identify which CAN IDs or serial messages carry fuel
information, then implement a minimal backend that:

- Subscribes to the necessary channels.
- Computes ``fuel_rate_l_h`` (and optionally ``fuel_current``).
- Skips updates when values are missing or clearly invalid.

