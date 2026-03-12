Plugins
=======

Navit includes several plugins that extend its functionality:

.. toctree::
   :maxdepth: 2
   :caption: Contents

   Driver Break Plugin <self>
   tests

.. contents:: On this page
   :local:
   :depth: 2

.. _driver-break-plugin:

Driver Break Plugin
-------------------

The Driver Break plugin provides configurable rest period management for multiple travel modes. It tracks activity time, suggests rest stops using OpenStreetMap data, discovers nearby Points of Interest (POIs), and supports route validation and energy-based routing where applicable. It also supports live fuel monitoring via OBD-II (ELM327), J1939 (SocketCAN), and MegaSquirt ECU backends.

.. _travel-modes:

Travel modes
~~~~~~~~~~~~

- **Car** – Configurable soft and maximum driving hours, break interval (e.g. every 4–4.5 hours), and break duration (e.g. 15–45 minutes).
- **Truck** – Mandatory rest and driving time rules aligned with EU Regulation EC 561/2006 and related rules (e.g. break after 4.5 hours, 45-minute break, max daily driving, daily and weekly rest, and other driving/rest time limits).
- **Hiking** – Daily segments: 40 km suggested maximum per day. Rest stops at 11.295 km intervals (main) or 2.275 km (alternative). Optional SRTM elevation and POI support (water, cabins).
- **Bicycle (cycling)** – Daily segments: 100 km suggested maximum per day. Rest stops at 28.24 km intervals (main) or 5.69 km (alternative). Suggested defaults use the same rast/vei concept as hiking, scaled up for cycling (see :ref:`rast-and-vei` below).

.. _pois-searched:

POIs searched
~~~~~~~~~~~~~

The plugin discovers nearby Points of Interest depending on travel mode. Search radii are configurable (e.g. water 2 km, cabins 5 km, general POI 15 km, network huts 25 km). Searched POI types include:

- **Water** – Drinking water, fountain, spring (for hiking/cycling rest stops).
- **Cabins and huts** – Wilderness hut, alpine hut, hostel, camping; with optional DNT/network detection for prioritization.
- **Car** – Cafe, restaurant, museum, viewpoint, picnic, attraction (and similar amenities along driving routes).

.. _distance-from-buildings:

Distance from buildings (camping, allemannsretten)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For overnight or camping stops (e.g. when using right-to-roam rules such as Norwegian allemannsretten), the plugin enforces a configurable minimum distance from buildings or dwellings. Rest stop candidates too close to buildings are filtered out so suggested sites respect local camping rules (default e.g. 150 m; configurable).

.. _distance-from-glaciers:

Distance from glaciers (overnight)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Nightly camping positions too close to glaciers are rejected. A configurable minimum distance (e.g. 300 m) applies; the check can be relaxed when the location has a camping building.

.. _configurable-rest-stop-periods:

Configurable rest stop periods
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Rest parameters are configurable per mode, including:

- Car: soft limit hours, max hours, break interval (hours), break duration (minutes).
- Truck: mandatory break after (hours), break duration (minutes), max daily driving hours.
- Hiking: main and alternative break distances (km), max daily distance (km).
- Cycling: main and alternative break distances (km), max daily distance (km).
- General: rest interval range (min/max hours), POI search radii, minimum distance from buildings (camping / allemannsretten), minimum distance from glaciers for overnight stops.

.. _rast-and-vei:

Rast and vei (historical basis for hiking and cycling defaults)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The suggested default rest intervals for hiking (11.295 km main, 2.275 km alternative; 40 km daily max) and for cycling (28.24 km main, 5.69 km alternative; 100 km daily max) are inspired by the old Scandinavian units of length **rast** and **vei**. For cycling, the same rast/vei concept is used with distances scaled up. A "rast" was the distance one traveled on foot before needing a rest ("rast," "pause," or the like); it corresponded to a **mil** and was often tied to the length of the ell. The distance varied by region and over time. In the 900s a rast was about 192 stone throws, divided into four quarters ("fjerdingvei"), and corresponded to roughly 9,100.8 meters; in the 12th century it was expressed as 16,000 ells (four quarters of 8,000 feet) but remained in the same order of magnitude.
A "dagsvei" (day's way/journey) was a traditional Scandinavian unit meaning roughly how far you could walk in a day, commonly reckoned at about 40 km.

The plugin's hiking and cycling interval defaults follow from these historical rast-based distances.

.. _networks-and-priorities:

Networks and priorities
~~~~~~~~~~~~~~~~~~~~~~~

- **DNT/network priority** – Optional priority for network huts (e.g. Norwegian Trekking Association, DNT) with configurable hut search radius.
- **Hiking/pilgrimage priority** – Optional preference for official hiking and pilgrimage routes when validating or suggesting stops.

.. _hiking-route-validation:

Hiking route validation
~~~~~~~~~~~~~~~~~~~~~~~

For hiking routes, the plugin can validate that the route avoids forbidden road types (e.g. motorway, trunk, primary) and reports how much of the route uses priority paths (footway, path, track, steps, bridleway). With pilgrimage/hiking priority enabled, segments tagged as pilgrimage or hiking routes are counted as priority. Warnings are produced for high forbidden percentage or low priority path percentage.

.. _energy-based-routing:

Energy-based routing (cycling)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Optional energy-based routing uses a physical model (total weight, rolling and air resistance, recuperation on downhill) to compute segment cost, inspired by BRouter's kinematic model. The model parameters are:

- **totalweight** – Total weight in kg (vehicle + cargo + person).
- **f_roll** – Rolling resistance coefficient.
- **f_air** – Air resistance coefficient.
- **f_recup** – Recuperation coefficient for downhill segments.
- **p_standby** – Standby power in watts.
- **recup_efficiency** – Recuperation efficiency (0.0–1.0).
- **outside_temp** – Outside temperature in Celsius (affects resistance).
- **vmax** – Maximum speed in m/s.

For each route segment, the model calculates energy consumed (Joules), time taken (seconds), and a normalized routing cost based on distance, elevation change, and speed limit. Gradient is derived from elevation delta and segment distance. Configurable via OSD attributes ``totalweight`` and ``use_energy_routing``.

.. _srtm-elevation:

SRTM elevation
~~~~~~~~~~~~~~

For hiking, elevation data from SRTM HGT files can be used. The plugin supports three elevation data sources, tried in order:

1. **Copernicus DEM GLO-30** (primary) – GeoTIFF tiles downloaded from the public AWS S3 bucket. Requires libcurl and libtiff.
2. **Viewfinder Panoramas dem3** (fallback) – HGT zip files organised in zone folders (e.g. ``dem3/M32/N61E009.zip``); a browser User-Agent is sent to avoid blocking.
3. **NASA SRTMGL1** (second fallback) – Standard 1-arcsecond HGT zip files.

The SRTM module provides the following capabilities:

- Query elevation at any coordinate (returns -32768 if data is unavailable).
- Calculate the set of tiles required to cover a bounding box.
- List, download, pause, resume, and cancel downloads per named region (e.g. "Norway", "Europe").
- Track per-tile and per-region download progress (0–100 %).
- Cache multiple tiles; handle tile borders transparently.
- Each tile carries primary, fallback, and second-fallback download URLs, plus an optional MD5 checksum.

.. _obd2-support:

OBD-II (ELM327) fuel monitoring
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The OBD-II backend connects to an ELM327-compatible adapter over a serial port (default ``/dev/ttyUSB0``) and feeds live fuel data into the plugin. It is intentionally conservative: if the adapter or ECU does not respond as expected the backend disables itself and the plugin falls back to manual and adaptive estimation.

**PID discovery and polling**

On startup the backend discovers which PIDs the ECU supports by requesting bitmask responses for PID ranges 0x00, 0x20, and 0x40. It then polls only the supported subset at reasonable intervals:

+--------+---------------------------------------------+
| PID    | Data                                        |
+========+=============================================+
| 0x2F   | Fuel tank level (%)                         |
+--------+---------------------------------------------+
| 0x5E   | Engine fuel rate (L/h)                      |
+--------+---------------------------------------------+
| 0x10   | Mass Air Flow (MAF) rate – used to estimate |
|        | fuel rate when 0x5E is unsupported          |
+--------+---------------------------------------------+
| 0x0D   | Vehicle speed (km/h)                        |
+--------+---------------------------------------------+
| 0x51   | Fuel type                                   |
+--------+---------------------------------------------+
| 0x52   | Ethanol fuel percentage                     |
+--------+---------------------------------------------+

**MAF-based fuel rate estimation**

When PID 0x5E (direct fuel rate) is not supported, the backend estimates fuel consumption from the MAF sensor reading using the stoichiometric ratio for the detected fuel type. Petrol engines at cruise typically report 10–20 g/s MAF (2–4 L/h); diesel engines under load report 30+ g/s with higher volumetric rates. Flex-fuel vehicles report ethanol content via PID 0x52: E85 requires a higher volume fuel rate than E10 for the same MAF value.

**Fields updated in the plugin**

- ``fuel_rate_l_h`` – Current fuel consumption in litres per hour.
- ``fuel_current`` – Current fuel level derived from tank level % and configured tank capacity.
- Ethanol percentage (for flex-fuel vehicles).

.. _j1939-support:

J1939 (SocketCAN) fuel monitoring
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The J1939 backend listens on a SocketCAN interface (default ``can0``) for two Parameter Group Numbers (PGNs) broadcast by heavy-vehicle ECUs:

+------------------+------------------+------------------------------------------+
| PGN              | Acronym          | Signal                                   |
+==================+==================+==========================================+
| 65266 (0xFEEA)   | Engine Fuel Rate | Fuel rate, 0.05 L/h per bit              |
+------------------+------------------+------------------------------------------+
| 65276 (0xFEF4)   | Fuel Level       | Tank level %, 0.4 % per bit              |
+------------------+------------------+------------------------------------------+

The backend translates these CAN signals into the same ``fuel_rate_l_h`` and ``fuel_current`` fields used by the OBD-II backend, so the rest of the plugin is source-agnostic. Example scaling: a raw PGN 65266 value of 1000 yields 50 L/h; a raw PGN 65276 value of 50 yields 20 % of a 400 L tank. This backend is primarily intended for truck mode.

.. _megasquirt-support:

MegaSquirt ECU fuel monitoring
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The MegaSquirt backend supports the following ECUs from the MegaSquirt family:

- MS1 (MegaSquirt-I)
- MS2 (MegaSquirt-II)
- MS3 / MS3-Pro (MegaSquirt-III)
- MicroSquirt

**Connection**

The backend communicates over a serial RS-232 or USB-serial connection at **115200 baud, 8N1** (default device ``/dev/ttyUSB0``). On startup it optionally sends a version command to detect the firmware variant (MS2/MS3/MicroSquirt), then begins periodic polling.

**Protocol**

The legacy ``A`` (realtime data) command from the MSExtra documentation is used to request the realtime data block. RPM and injector pulse-width fields are parsed from this block to compute fuel rate.

**Fuel rate calculation**

Fuel rate is derived from injector pulse width, engine RPM, and the configured injector flow rate:

.. code-block:: text

   fuel_rate_l_h = (pulse_width_ms / 1000) * (RPM / 60)
                   * injector_flow_cc_per_min * num_cylinders / 1000

The backend only starts if the configuration flag ``fuel_megasquirt_available`` is set and a valid injector flow rate has been configured via ``fuel_injector_flow_cc_min``. It applies strict bounds checks on RPM and pulse-width values and handles malformed, short, and overflow input buffers without corrupting fuel statistics. Like the OBD-II and J1939 backends, results are written into ``fuel_rate_l_h`` and ``fuel_current``. If the serial port cannot be opened or no plausible realtime data is received, the backend fails quietly and the plugin falls back to manual/adaptive estimation.

.. _ui-and-configuration:

User interface and configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The plugin registers as an OSD (type ``rest``). With the internal GUI, menu actions are available: suggest rest stop (along current route), rest stop history, start break, end break, configure intervals (per profile: car, truck, hiking, cycling), and configure overnight (min distance from buildings/glaciers, POI radii). Session state (driving time, break in progress, mandatory break required) is tracked. Configuration and rest stop history are stored in a SQLite database (path configurable via OSD ``data`` attribute; default in user data directory). Vehicle type can be set via OSD attribute ``type`` (e.g. car, truck).

Fuel backend selection is automatic: J1939 is tried first for truck mode (requires SocketCAN), then OBD-II (requires a serial ELM327 adapter), then MegaSquirt (requires serial connection to the ECU). If none are available, the plugin falls back to manual and adaptive fuel estimation.
