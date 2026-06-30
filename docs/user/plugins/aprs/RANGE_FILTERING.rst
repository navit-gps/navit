APRS Plugin Range Filtering
===========================

Overview
--------

Range filtering limits which APRS stations are displayed on the map based on their distance from a specified center point. This helps reduce visual clutter and improve performance when tracking many stations.

How It Works
------------

Distance Calculation
~~~~~~~~~~~~~~~~~~~~

The plugin uses the Haversine formula to calculate the great-circle distance between two points on Earth::

   distance = 2 * R * atan2(√a, √(1-a))

where::

   a = sin²(Δlat/2) + cos(lat1) * cos(lat2) * sin²(Δlon/2)
   R = Earth's radius (6371 km)

This provides accurate distance calculations accounting for Earth's curvature.

Filtering Process
~~~~~~~~~~~~~~~~~

1. **Center Point** – A geographic coordinate (latitude/longitude) is specified as the center.
2. **Range** – A maximum distance in kilometers is set.
3. **Query** – The database is queried for all stations.
4. **Distance Check** – For each station, the distance from the center is calculated.
5. **Filtering** – Only stations within the range are added to the map.

Configuration
-------------

Basic Configuration
~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

   <map type="aprs" data="/path/to/aprs.db">
       <attribute name="distance" value="50000"/>
       <attribute name="position_coord_geo" lat="37.7749" lng="-122.4194"/>
   </map>

Parameters
~~~~~~~~~~

- ``distance`` (double, in meters):

  - Maximum distance from center point.
  - ``0`` = disabled (show all stations).
  - Example: ``50000`` = 50 km range.

- ``position_coord_geo`` (coord_geo):

  - Center point for range calculation.
  - Format: ``lat="latitude" lng="longitude"``.
  - Required when distance > 0.
  - Typically set to current position or area of interest.

Use Cases
---------

1. Local Area Monitoring
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

   <map type="aprs" data="/path/to/aprs.db">
       <attribute name="distance" value="25000"/>
       <attribute name="position_coord_geo" lat="40.7128" lng="-74.0060"/>
   </map>

Shows stations within 25 km of New York City.

2. Current Position Tracking
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

   <map type="aprs" data="/path/to/aprs.db">
       <attribute name="distance" value="100000"/>
       <attribute name="position_coord_geo" lat="${current_lat}" lng="${current_lng}"/>
   </map>

Shows stations within 100 km of your current position.

3. Performance Optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

   <map type="aprs" data="/path/to/aprs.db">
       <attribute name="distance" value="200000"/>
       <attribute name="position_coord_geo" lat="51.5074" lng="-0.1278"/>
   </map>

Shows only stations within 200 km of London, improving map rendering performance.

4. Disable Filtering
~~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

   <map type="aprs" data="/path/to/aprs.db">
       <attribute name="distance" value="0"/>
   </map>

Or simply omit the ``distance`` attribute to show all stations.

Implementation Details
----------------------

Code Location
~~~~~~~~~~~~~

Range filtering is implemented in:

- ``aprs_db.c`` – ``aprs_db_get_stations_in_range()``.
- ``aprs.c`` – ``aprs_update_items()``.

Algorithm (simplified)
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // For each station in database:
   1. Calculate distance using Haversine formula.
   2. Compare distance to range_km.
   3. If distance <= range_km:
        add station to display list.
      else:
        discard station.

Performance Impact
~~~~~~~~~~~~~~~~~~

With range filtering enabled:

- Reduces number of items processed.
- Faster map rendering.
- Lower memory usage.

Without range filtering:

- All stations processed.
- Higher CPU/memory usage when many stations are present.

Examples
--------

Example 1: 50 km Range Around San Francisco
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

   <map type="aprs" data="/var/lib/navit/aprs.db">
       <attribute name="distance" value="50000"/>
       <attribute name="position_coord_geo" lat="37.7749" lng="-122.4194"/>
   </map>

Example 2: 100 km Range Around Berlin
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

   <map type="aprs" data="/var/lib/navit/aprs.db">
       <attribute name="distance" value="100000"/>
       <attribute name="position_coord_geo" lat="52.5200" lng="13.4050"/>
   </map>

Example 3: No Filtering (Global View)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

   <map type="aprs" data="/var/lib/navit/aprs.db">
       <attribute name="distance" value="0"/>
   </map>

Distance Reference
------------------

Common range values:

================== =========== ====================
Distance (meters)  Distance km Typical Use Case
================== =========== ====================
10,000             10          City center
25,000             25          Metropolitan area
50,000             50          Regional area
100,000            100         Large region
200,000            200         State/province
500,000            500         Country/continent
0                  Unlimited   Global view
================== =========== ====================

Dynamic Updates
---------------

Range filtering is applied when:

- Map items are updated (periodic refresh).
- New packets are received and stored.
- The map is refreshed or configuration is changed.

Note: The center point is static once set. To change it, update the configuration and reload the map or adjust attributes at runtime.

Technical Notes
---------------

1. Distance calculation uses the Haversine formula (WGS84 coordinates).
2. Current implementation filters in memory after fetching from SQLite.
3. For very large databases, SQL-based pre-filtering could be added.
4. Coordinates use standard GPS (latitude/longitude).

Troubleshooting
---------------

No Stations Showing
~~~~~~~~~~~~~~~~~~~

1. Check if range is too small.
2. Verify center point coordinates are correct.
3. Ensure stations exist in the database.
4. Check expiration timeout (stations may have expired).

Too Many Stations
~~~~~~~~~~~~~~~~~

1. Reduce the ``distance`` value.
2. Enable range filtering if disabled.
3. Adjust center point to area of interest.

Performance Issues
~~~~~~~~~~~~~~~~~~

1. Enable range filtering.
2. Reduce range distance.
3. Ensure database expiration is configured to limit old data.

