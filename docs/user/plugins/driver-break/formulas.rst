Driver Break formulas
=====================

This document summarizes the main formulas used by the Driver Break plugin for:

- Fuel-rate estimation from OBD-II (MAF-based fallback).
- J1939 fuel-rate and fuel-level scaling.
- MegaSquirt injector-based fuel-rate estimation.
- Range estimation from fuel and average consumption.
- Energy-based routing (eco-mode) cost model.

The exact implementation details live in the C source files; this document is
intended as a readable reference for how the numbers are derived.


OBD-II (MAF-based fuel rate)
----------------------------

When a vehicle does not support PID 0x5E (engine fuel rate) directly, the
plugin estimates fuel rate from the mass air flow (MAF) sensor reading. The
general formula is:

.. math::

   \text{Fuel rate (L/h)} =
      \frac{\text{MAF (g/s)} \times 3600}{\text{AFR} \times \rho \times 1000}

where:

- ``MAF`` is the air mass flow in g/s from PID 0x10.
- ``AFR`` is the air–fuel ratio (mass of air per mass of fuel), which depends
  on fuel type and ethanol content (for example, about 14.7 for petrol).
- ``ρ`` (rho) is the fuel density in kg/L (for example, about 0.745 for petrol).

The plugin selects reasonable AFR and density values based on the configured
fuel type and, when available, ethanol percentage from PID 0x52.


J1939 fuel-rate and fuel-level scaling
--------------------------------------

For trucks and heavy vehicles, the J1939 backend listens to:

- PGN 65266 (FEEA) – Engine fuel rate (SPN 183).
- PGN 65276 (FEF4) – Fuel level (SPN 96).

The protocol defines:

- **SPN 183**: 0.05 L/h per bit, offset 0.

  .. math::

     \text{fuel\_rate\_L\_h} = \text{raw\_spn183} \times 0.05

- **SPN 96**: 0.4 % per bit, range 0–250 %.

  .. math::

     \text{fuel\_level\_%} = \text{raw\_spn96} \times 0.4

The plugin then converts fuel level to litres using the configured tank
capacity:

.. math::

   \text{fuel\_current\_L} =
      \frac{\text{fuel\_level\_%}}{100} \times \text{tank\_capacity\_L}


MegaSquirt injector-based fuel-rate
-----------------------------------

The MegaSquirt backend reads injector pulse width and engine RPM from the ECU
and uses an injector flow rate configured in ``fuel_injector_flow_cc_min``.

Let:

- ``pw_ms`` be injector pulse width in ms.
- ``rpm`` be engine speed.
- ``n_cyl`` be the number of cylinders.
- ``flow_cc_min`` be injector flow per injector in cc/min.

The approximate fuel rate is:

.. math::

   \text{fuel\_rate\_L\_h} =
      \frac{\text{pw\_ms} \times \text{rpm} \times n_\text{cyl} \times \text{flow\_cc\_min}}{2\,000\,000}

This comes from:

- Pulses per minute per cylinder are proportional to RPM.
- Each pulse delivers ``flow_cc_min / 60`` cc per second at 100 % duty cycle.
- The duty cycle is ``pw_ms / 1000`` of each engine cycle.

The backend applies sanity limits to RPM, pulse width and the resulting fuel
rate; if values fall outside acceptable ranges, the update is skipped.


Range estimation
----------------

Range estimation combines the current fuel amount with an average
consumption figure. The configuration stores:

- ``fuel_tank_capacity_l`` – tank capacity (L).
- ``fuel_avg_consumption_x10`` – average consumption in 0.1 L/100 km units.

Given:

- ``fuel_current_l`` – current fuel amount (L).
- ``consumption_L_per_100km`` – derived from ``fuel_avg_consumption_x10``.

The estimated range is:

.. math::

   \text{range\_km} =
      \frac{\text{fuel\_current\_L} \times 100}{\text{consumption\_L\_per\_100km}}

The plugin may also use a short-term adaptive average (from recent samples)
when live fuel data is available, to refine this estimate.


Energy-based routing (eco-mode)
-------------------------------

Energy-based routing treats each route segment as requiring a certain amount
of mechanical energy, based on:

- Total weight (vehicle + occupants + cargo).
- Rolling resistance.
- Aerodynamic drag.
- Gravitational potential changes (climbs and descents).

The cost per segment is proportional to:

.. math::

   E \approx
      (F_\text{rolling} + F_\text{drag}) \times d
      + m g \Delta h

where:

- ``F_rolling`` is rolling resistance force.
- ``F_drag`` is aerodynamic drag force.
- ``d`` is segment length.
- ``m`` is total mass.
- ``g`` is gravitational acceleration.
- ``Δh`` is height difference.

When live fuel data is available, the plugin can relate this energy cost to
actual fuel usage by comparing:

- Energy-based predictions.
- Measured fuel rate from the active ECU backend.

Routes with lower predicted energy cost (and, therefore, lower fuel use) are
preferred when energy-based routing is enabled.

