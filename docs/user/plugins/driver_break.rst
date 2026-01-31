Driver Break Plugin
===========

Overview
--------

The Driver Break plugin provides dynamic rest period management for vehicles, with support for both cars and trucks. It tracks driving time, suggests rest stops based on OpenStreetMap data, discovers nearby Points of Interest (POIs), and ensures compliance with EU Regulation EC 561/2006 for commercial vehicles.

Route Safety for Hikers and Cyclists
-------------------------------------

**Current Status**: The plugin provides **post-route validation** to check if calculated routes use unsafe roads (motorways, trunk, primary roads), but does **not actively prevent** routing on these roads during route calculation.

**How It Works**:
- **Route Validation**: After a route is calculated, the plugin can validate it against forbidden highways
- **Warnings**: If unsafe roads are detected, warnings are generated
- **Rest Stop Filtering**: Rest stops are only suggested at safe locations (excludes motorways, trunk, primary)

**For Full Safety**: Use Navit's built-in vehicle profiles:
- Select "pedestrian" profile for hiking (excludes motorways)
- Select "bike" profile for cycling (excludes motorways)

**Forbidden Highways for Hikers**:
- ``motorway``, ``trunk``, ``primary`` and their links
- These are checked during route validation

**Priority Paths (Safe)**:
- ``footway``, ``path``, ``track``, ``steps``, ``bridleway``

Features
--------

DNT/Network and Pilgrimage Route Support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The plugin fully supports DNT (Den Norske Turistforening) and network huts, as well as pilgrimage routes:

- **Network Detection**: Automatically detects DNT, STF, and other network-operated cabins via OSM tags
- **DNT Priority Mode**: When enabled, uses 25 km search radius for network huts and adjusts daily distance to 46.4 km
- **Pilgrimage Routes**: Detects and prioritizes ``route=hiking`` and ``pilgrimage=yes`` paths
- **Map-Based POI Discovery**: Uses direct map queries (preferred) instead of unreliable Overpass API

Core Functionality
~~~~~~~~~~~~~~~~~~

- **Driving Time Tracking**: Monitors continuous driving time and total daily driving hours
- **Rest Stop Discovery**: Finds suitable rest locations along routes based on OSM criteria
- **POI Discovery**: Discovers nearby amenities, restaurants, attractions (15 km radius for general POIs, 2 km for water points, 5 km for cabins/huts)
- **EU Regulation Compliance**: Enforces mandatory break rules for trucks (EC 561/2006)
- **History Tracking**: SQLite database stores rest stop history
- **Configurable Settings**: Customizable rest intervals, break durations, and vehicle-specific limits

Vehicle Types
~~~~~~~~~~~~~

Cars
^^^^

- Soft limit: 5-9 hours per day (configurable, default 7 hours)
- Maximum: 10-12 hours per day (configurable, default 10 hours)
- Recommended break: Every 4.5 hours (configurable, default 4 hours)
- Break duration: 15-45 minutes (configurable, default 30 minutes)

Trucks (EU Regulation EC 561/2006)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Mandatory break: 45 minutes after 4.5 hours of driving
- Maximum daily driving: 9 hours
- Predictive break scheduling
- Cross-border regulation support

Hiking
^^^^^^

The default break distances for hikers are based on the historical Norwegian mile system:

- **1 Norwegian mile** = 11 km (traditional unit)
- **1/4 Norwegian mile** = 2.8 km (short rest interval)
- **Day's walk** = approximately 40 km

Defaults (configurable):

- Main rest interval: 11.295 km (1 Norwegian mile)
- Short rest interval: 2.275 km (approximately 1/4 Norwegian mile)
- Maximum daily distance: 40 km

Rest Stop Location Criteria
---------------------------

The plugin finds rest stops at intersections between specific road types:
- ``highway=unclassified``
- ``highway=service``
- ``highway=track``
- ``highway=rest_area``
- ``highway=tertiary``

Exclusion Rules
~~~~~~~~~~~~~~~

Rest stops are automatically excluded from:
- ``landuse=farmland``
- ``landuse=residential``
- ``landuse=commercial``
- ``landuse=industrial``
- ``landuse=military``
- ``landuse=recreation_ground``

Minimum distance from buildings: 150 meters (configurable)

POI Categories
--------------

The plugin discovers POIs with different search radii depending on type:
- **General POIs** (cafes, restaurants, museums, etc.): 15 km radius (configurable)
- **Water points**: 2 km radius (configurable)
- **Cabins/huts**: 5 km radius (configurable, 25 km when DNT priority enabled)

Food & Drink
~~~~~~~~~~~~

- Cafes (``amenity=cafe``)
- Restaurants (``amenity=restaurant``)
- Bars/Pubs (``amenity=bar``, ``amenity=pub``)
- Breweries (``craft_brewery=yes``)

Cultural & Educational
~~~~~~~~~~~~~~~~~~~~~~

- Museums (``tourism=museum``)
- Art Galleries (``tourism=gallery``)
- Attractions (``tourism=attraction``)
- Zoos (``tourism=zoo``)
- Information Centers (``tourism=information``)

Natural & Scenic
~~~~~~~~~~~~~~~~

- Viewpoints (``tourism=viewpoint``)
- Picnic Sites (``leisure=picnic_table``)
- Scenic Overlooks

Installation
------------

Dependencies
~~~~~~~~~~~~

- **SQLite3**: For rest stop history storage
  - Debian/Ubuntu: ``apt-get install libsqlite3-dev``
  - Fedora: ``dnf install sqlite-devel``

- **libcurl**: Optional, enables POI discovery via Overpass API when map data is not available
  - Debian/Ubuntu: ``apt-get install libcurl4-openssl-dev``
  - Fedora: ``dnf install libcurl-devel``

  **Note**: The plugin works without libcurl, but POI discovery will be disabled if map data is unavailable.

Build
~~~~~

The plugin is built by default when Navit is compiled. To disable it:

.. code-block:: bash

   cmake -Dplugin/driver_break=OFF ..

Configuration
-------------

XML Configuration
~~~~~~~~~~~~~~~~~

Add the plugin to your ``navit.xml``:

.. code-block:: xml

   <osd type="driver_break" enabled="yes" data="/path/to/driver_break_plugin.db">
       <attr name="type" value="car"/>
   </osd>

For trucks:

.. code-block:: xml

   <osd type="driver_break" enabled="yes" data="/path/to/driver_break_plugin.db">
       <attr name="type" value="truck"/>
   </osd>

Configuration Attributes
~~~~~~~~~~~~~~~~~~~~~~~~

- ``data``: Path to SQLite database file (optional, defaults to user data directory)
- ``type``: Vehicle type - "car", "truck", "hiking", or "cycling" (default: "car")

Customizing Settings
~~~~~~~~~~~~~~~~~~~~

Settings can be customized through the plugin's configuration menu or by modifying the database directly. The plugin saves configuration automatically.

**Note**: Currently, changing configuration values through the UI is not yet implemented. The configuration dialogs display current values but do not allow editing. To modify settings, edit the database directly using SQLite tools.

**Rest period intervals**: There is currently no way to adjust rest period intervals (main rest distance, short rest distance, maximum daily distance) from the user interface. These values are stored in the database and can be changed with SQLite tools. Adding UI controls for them (for example, drop-down choices or sliders in the configuration dialogs) would be a relatively straightforward improvement for anyone who wants to implement it.

Usage
-----

Menu Commands
~~~~~~~~~~~~~

The plugin provides the following menu commands, all of which are fully implemented:

- ``rest_suggest_stop()`` or ``driver_break_suggest_stop()``: Show rest stop suggestions along the current route. Finds suitable rest locations based on route analysis and displays them in a dialog with POI information.

- ``rest_show_history()`` or ``driver_break_show_history()``: Display rest stop history from the database. Shows a list of all recorded breaks with timestamps, locations, durations, and whether they were mandatory breaks.

- ``rest_configure_intervals()`` or ``driver_break_configure_intervals()``: Configure rest stop intervals based on the selected vehicle profile. Shows a profile-specific dialog (car, truck, hiking, cycling) with current settings that can be modified and saved.

- ``rest_configure_overnight()`` or ``driver_break_configure_overnight()``: Configure overnight stop settings based on the selected vehicle profile. Allows configuration of minimum distances from buildings and glaciers, and other overnight-specific parameters.

- ``rest_start_break()`` or ``driver_break_start_break()``: Record break start time and current position. Tracks the start of a break session and stores the GPS coordinates.

- ``rest_end_break()`` or ``driver_break_end_break()``: Record break end, calculate duration, and save to history. Calculates the break duration, creates a history entry, and saves it to the database for future reference.

**Note**: The plugin supports both the legacy ``rest_*`` command names (for backward compatibility) and the new ``driver_break_*`` command names. All commands require the internal GUI to display dialogs; if the internal GUI is not available, appropriate error messages are shown.

Automatic Operation
~~~~~~~~~~~~~~~~~~

The plugin automatically:
- Tracks driving time when vehicle is moving
- Checks for mandatory breaks based on vehicle type
- Suggests rest stops when mandatory break is required
- Discovers POIs near suggested rest stops

Database Schema
---------------

The plugin uses SQLite with the following tables:

rest_stop_history
~~~~~~~~~~~~~~~~~

- ``id``: Primary key
- ``timestamp``: Unix timestamp
- ``latitude``: Rest stop latitude
- ``longitude``: Rest stop longitude
- ``name``: Rest stop name (optional)
- ``duration_minutes``: Break duration
- ``was_mandatory``: 1 if mandatory break, 0 otherwise

rest_config
~~~~~~~~~~~

- ``key``: Configuration key
- ``value``: Configuration value (stored as text)

API Integration
--------------

POI Discovery Methods
~~~~~~~~~~~~~~~~~~~~~

The plugin uses **map-based POI discovery** as the primary method (preferred), with Overpass API as a fallback:

**Primary Method: Map-Based Discovery**
- Uses direct queries to Navit's loaded map data
- More reliable and faster than API calls
- No network dependency
- Works with any OSM-based map format

**Fallback Method: Overpass API**
- Used only when map data is not available
- Endpoint: ``https://overpass-api.de/api/interpreter``
- Timeout: 30 seconds
- Query format: Overpass QL
- Requires libcurl

Graceful Degradation
~~~~~~~~~~~~~~~~~~~~

If map data is not available and Overpass API is unavailable or libcurl is not installed:
- Rest stop discovery still works
- POI discovery is disabled
- User is notified via debug messages

Performance
-----------

- Minimal computational overhead
- Asynchronous POI discovery (planned)
- Caching of frequently accessed data (planned)
- Database queries optimized with indexes

Logging
-------

The plugin logs events at various levels:
- ``lvl_info``: Important events (break required, rest stops found)
- ``lvl_debug``: Detailed operation (POI discovery, route analysis)
- ``lvl_error``: Errors (database issues, API failures)

Possible Future Enhancements
-----------------------------

- **Rest interval UI**: Add controls (e.g. drop-downs or sliders) in the configuration dialogs to adjust rest period intervals; the backend and database already support custom values.
- User preference system for POI ranking
- Operating hours integration
- User reviews integration
- Accessibility features filtering
- Multiple EU member state regulation support
- Visual indicators for recommended/mandatory breaks
- Route integration for automatic rest stop suggestions

Unit Tests
----------

Comprehensive unit tests are available in the ``tests/`` directory:

- ``test_driver_break_db.c`` - Database operation tests
- ``test_driver_break_config.c`` - Configuration and driving time calculation tests
- ``test_driver_break_finder.c`` - Rest stop finder tests
- ``test_driver_break_srtm.c`` - SRTM elevation data tests
- ``test_driver_break_routing.c`` - Routing and route validation tests
- ``test_driver_break_integration.c`` - Integration tests using actual Navit functions
- ``test_driver_break_route_integration.c`` - Route integration tests for rest intervals and POI discovery

Build and run tests:

.. code-block:: bash

   cmake .. -DBUILD_DRIVER_BREAK_TESTS=ON
   make test_driver_break_db test_driver_break_config test_driver_break_finder
   ./build/navit/plugin/driver_break/tests/test_driver_break_db

License
-------

This plugin is part of Navit and is licensed under the GNU Library General Public License version 2.
