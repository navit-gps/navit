Navit Moving Objects Capability Verification
============================================

Executive Summary
-----------------

**VERIFIED**: Navit fully supports moving object tracking and real-time dynamic updates through its map plugin architecture.

Verification Results
--------------------

1. Native Moving Object Support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Status**: CONFIRMED

**Evidence:**

- The ``traffic.c`` plugin demonstrates dynamic item creation, update, and removal.
- Map plugins can maintain in-memory item lists that are queried dynamically.
- Items are returned through ``map_rect_get_item()`` which queries current state.

**Implementation Pattern (simplified):**

.. code-block:: c

   struct map_priv {
       GList *items;  /* Dynamic list of items */
       /* ... other data ... */
   };

   static struct item *map_rect_get_item(struct map_rect_priv *mr) {
       /* Returns next item from current state */
   }

2. Dynamic Point-of-Interest Updates
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Status**: CONFIRMED

**Evidence:**

- Items can be added to map dynamically via helper functions (see traffic plugin).
- Items can be removed by updating their type or dropping them from internal lists.
- Items can be updated by modifying underlying data structures.

**Update Mechanism:**

- Map callbacks notify Navit when map data changes.
- ``map_add_callback()`` / ``map_remove_callback()`` used for change notifications.

3. Real-time Coordinate Rendering
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Status**: CONFIRMED

**Evidence:**

- Map rectangles query items on-demand during rendering.
- Each ``map_rect_get_item()`` call returns current item state.
- No blocking cache prevents real-time updates.

**Rendering Flow (simplified):**

1. Navit requests items via ``map_rect_get_item()``.
2. Plugin returns items from current in-memory state.
3. Items include current coordinates and attributes.
4. Map redraws when callbacks are triggered.

4. Plugin/Extension Mechanism
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Status**: CONFIRMED

**Evidence:**

- Map plugins register via ``plugin_register_category_map()``.
- Plugins implement ``struct map_methods`` interface.
- Example implementations: ``traffic.c``, ``track.c``, ``route.c``.

**Registration Pattern:**

.. code-block:: c

   void plugin_init(void) {
       plugin_register_category_map("aprs", aprs_map_new);
   }

5. D-Bus and Socket-Based Communication
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Status**: CONFIRMED

**Evidence:**

- Full D-Bus support exists in Navit (for example, ``binding_dbus.c``).
- D-Bus service ``org.navit_project.navit`` can receive commands and send signals.
- Socket-based vehicle plugins (such as ``vehicle_file.c``) show external data feeding patterns.

**Communication Options:**

1. D-Bus – for external control and monitoring.
2. Sockets – for file/socket-based input feeds.
3. Internal callbacks – for tight in-process integration.

Recommended Implementation Approach for APRS Plugin
---------------------------------------------------

Architecture
~~~~~~~~~~~~

1. **Map Plugin**:

   - Implement APRS as a map plugin (similar to the traffic plugin).
   - Provides items representing APRS stations.
   - Items updated dynamically as packets arrive.

2. **Data Storage**:

   - SQLite database for station persistence.
   - Station metadata (callsign, position, timestamp).
   - Station expiration tracking.
   - Range filtering support.

3. **Packet Decoder**:

   - Separate module for APRS/AX.25 decoding.
   - Bell 202 / 2FSK demodulation can be done via an SDR plugin or external tools.
   - AX.25 frame parsing and APRS position extraction handled within APRS core.

4. **Update Mechanism**:

   - Event-driven updates when packets arrive.
   - Map callbacks to trigger redraws.
   - Periodic expiration cleanup.

Item Type Selection
~~~~~~~~~~~~~~~~~~~

- Use ``type_poi_custom0`` or similar POI type for APRS stations.
- Item attributes:

  - Coordinates (lat/lng).
  - Station callsign (``attr_label``).
  - Timestamp (``attr_data``).
  - Symbol/icon (``attr_icon_src`` via APRS symbol mapping).

Performance Considerations
~~~~~~~~~~~~~~~~~~~~~~~~~~

- Efficient item lookup (hash table by callsign).
- Batch updates where appropriate to minimize redraws.
- Configurable update intervals.
- Range-based filtering before item creation.

Conclusion
----------

Navit's architecture fully supports the APRS tracking plugin requirements:

- Dynamic object creation and updates.
- Real-time coordinate rendering.
- Plugin-based extensibility.
- Multiple communication mechanisms (D-Bus, sockets, callbacks).

The traffic plugin serves as an excellent reference implementation for dynamic map items, and the APRS plugin follows the same general approach, extended with dedicated decoding and database components.

