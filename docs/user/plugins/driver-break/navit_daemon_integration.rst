Integrating Navit-daemon with Driver Break eco-mode
===================================================

`Navit-daemon <https://github.com/Supermagnum/Navit-daemon>`_ is a separate
daemon that fuses:

- Accelerometer (3-axis)
- Gyroscope (3-axis)
- Magnetometer / digital compass (3-axis, optional)
- GPS (position, speed, course-over-ground)

into a unified NMEA output with AHRS-derived heading. It can also expose raw
IMU samples and calibration via a TCP JSON API.

The Driver Break plugin can benefit from this data in two ways:

1. As an improved **vehicle source** (better heading and position for Navit).
2. As an additional **telemetry source** for eco-mode and fuel learning,
   combining IMU, RPM, gear, throttle and injector information.


Using Navit-daemon as the vehicle source
----------------------------------------

The simplest integration is to run Navit-daemon and configure Navit to consume
its NMEA output:

1. Start Navit-daemon so that it listens on a TCP port and outputs NMEA
   sentences (GGA, RMC) with fused heading.
2. Configure Navit’s vehicle plugin (for example, ``vehicle_gpsd`` or a TCP
   NMEA source) to connect to the Navit-daemon output.

From the Driver Break plugin’s perspective, nothing changes in its code:

- Position, speed and heading still come from Navit’s vehicle interface.
- Eco-mode and rest-stop logic automatically benefit from:

  - More stable heading (from IMU fusion rather than coarse GPS bearing).
  - Continuous heading even when GPS is weak (tunnels, urban canyons).

This alone improves:

- Tunnel extrapolation (dead reckoning).
- Vehicle icon orientation and map rotation.
- Route validation and POI search along the actual driven path.


Feeding accelerometer and RPM/gear into eco-mode
------------------------------------------------

For deeper integration, Navit-daemon can be extended (or complemented by a
small helper) to expose additional telemetry that Driver Break can use:

- **Accelerometer:** longitudinal and vertical acceleration for gradient and
  braking/coasting detection.
- **RPM:** from the ECU (OBD-II, J1939 or a native aftermarket ECU backend).
- **Gear position:** derived from RPM + vehicle speed or read from the ECU.
- **Throttle position (TPS):** when available from OBD-II or an aftermarket ECU.
- **Injector pulse width / fuel rate:** when available from the ECU.

The recommended architecture is:

1. **Navit-daemon (or a companion process)** collects:

   - IMU samples (accel/gyro/magnetometer) and GPS (already supported).
   - ECU data via existing Driver Break backends (OBD-II, J1939, MegaSquirt)
     or a dedicated ECU reader.

2. A small **telemetry backend** inside the Driver Break plugin subscribes to
   this combined stream (for example, a TCP JSON feed) and, for each segment:

   - Reads:

     - Longitudinal acceleration (for load and braking detection).
     - Vertical acceleration (for short-range gradient detection).
     - RPM and gear (operating point on the engine map).
     - Throttle position and injector pulse width / fuel rate.

   - Updates:

     - ``fuel_rate_l_h`` (if the ECU provides direct or easily derived fuel).
     - High-load / eco indicators for the current segment.
     - Telemetry fields that refine the adaptive fuel learning.

3. Eco-mode uses these enriched samples to better correlate:

   - Road gradient (from DEM + vertical accel).
   - Driver input (TPS, gear selection).
   - Actual fuel usage (injector-based or MAF-based).


Correcting DEM (HGT) limitations with IMU data
----------------------------------------------

The Driver Break plugin uses external DEM sources (for example, Copernicus
DEM GLO-30 and SRTM HGT) with a resolution on the order of 30 m. This means:

- Short hills or dips smaller than a DEM grid cell may be smoothed out.
- Rapid elevation changes over a few tens of metres can be underestimated.

By combining DEM data with vertical acceleration from the IMU:

- Short up/down segments that fall within a single DEM cell can still be
  detected from the **pattern of vertical acceleration**.
- The plugin (or its telemetry backend) can:

  - Detect sustained positive or negative vertical acceleration that indicates
    a climb or descent.
  - Adjust the effective ``Δh`` used when computing energy-based cost for the
    segment, instead of relying solely on DEM grid samples.

This improves:

- The accuracy of the **energy-based routing** model at small scales.
- The match between predicted energy use and measured fuel consumption.


Using RPM, gear and TPS for eco feedback
----------------------------------------

Once RPM, gear and throttle position are available alongside fuel usage,
Driver Break’s learning and eco-mode can offer more targeted feedback:

- Identify **inefficient operating regions**, for example:

  - High RPM and low gear with modest throttle on flat ground.
  - Repeated heavy acceleration followed by hard braking.

- Detect **coasting** and **engine braking**:

  - Low or zero injector pulse width at certain RPM ranges with negative
    longitudinal acceleration indicates engine braking.
  - TPS near zero and low fuel rate with slight deceleration indicates coasting.

- Build **per-gear efficiency hints**:

  - For a given vehicle speed and road grade, compare fuel usage in different
    gears.
  - Offer simple suggestions (for example, “shifting up earlier on gentle
    climbs reduces fuel use”).

These insights can be surfaced in the Driver Break OSD as optional eco-mode
indicators, without changing the underlying range or rest-stop logic.


Summary
-------

- Navit-daemon already improves heading and position for Navit, which benefits
  Driver Break immediately when used as the vehicle source.
- For deeper integration, treat Navit-daemon (and/or a helper) as a **sensor
  and ECU fusion layer** that provides a single stream of:

  - IMU (accel/gyro/mag) + GPS,
  - ECU data (RPM, gear, TPS, injector/fuel rate).

- A small backend in Driver Break can subscribe to that stream, update
  ``fuel_rate_l_h`` and related telemetry, and feed eco-mode and adaptive fuel
  learning with much richer data than GPS + DEM alone.

