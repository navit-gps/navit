Using the Navit D-Bus Interface for External Dashboards, Instrument Clusters and Head Units
===========================================================================================

.. contents::
   :local:
   :depth: 2


1. Overview and Motivation
==========================

1.1 What This Document Provides
-------------------------------

This document explains how to build an **external display** (instrument cluster,
head unit, data logger, or any other consumer) that receives live navigation
state from a running Navit instance **over D-Bus**, with **no changes** to Navit’s
source code or plugin code.

After reading this, an embedded Linux developer should be able to:

- Enable Navit’s D-Bus binding
- Connect to Navit on the D-Bus session bus
- Subscribe to navigation state attributes
- Listen for updates and drive their own UI (cluster, head unit, web dashboard, etc.)

You do **not** need to be a D-Bus expert. The examples here are practical and
focused on the common tasks needed for a cluster or head unit.


1.2 Why Use D-Bus for External Displays?
----------------------------------------

Navit exposes a D-Bus interface via its ``libbinding_dbus.so`` plugin. This
interface lets external applications:

- **Subscribe to navigation state** (next maneuver, distance to turn, ETA, etc.)
- **Receive updates automatically** via signals when data changes
- **Avoid polling** Navit for state

The external display:

- Runs as a separate process (often on the same device)
- Connects to Navit’s D-Bus service
- Registers which attributes it wants to watch
- Then passively **listens for signals** when those attributes change


1.3 Goals and Safety
--------------------

The goals of this interface are:

- **Open and stable protocol**: Any cluster or head unit that follows this
  specification should work with any Navit instance that has the D-Bus binding
  enabled.
- **Safety**: Navigation instructions and critical turn information can be shown
  in the driver’s primary field of view (instrument cluster), so the driver does
  not need to look away at a secondary screen.
- **Hardware independence**: This works on any Linux-based platform:

  - Raspberry Pi
  - x86-based PCs
  - Embedded ARM boards

Your choice of CPU, display, and UI framework is up to you. As long as you can
run Navit and D-Bus, you can implement this interface.


2. Prerequisites
================

2.1 Software Requirements
-------------------------

You need:

- **Linux** with a working **D-Bus session bus**
- **Navit** installed and running
- **Navit D-Bus binding enabled**

  In your ``navit.xml``, ensure this plugin is active:

  .. code-block:: xml

     <plugin path="$NAVIT_LIBDIR/*/${NAVIT_LIBPREFIX}libbinding_dbus.so" active="yes"/>

- For the Python examples:

  - ``python3``
  - ``python3-dbus`` (D-Bus bindings for Python)
  - ``python3-gi`` and the ``gi.repository.GLib`` module (for the main loop)


2.2 Basic D-Bus Concepts (Minimal Overview)
-------------------------------------------

You only need a few core ideas:

- **Bus**: A message bus where processes talk. Here we use the **session bus**.
- **Service name**: A unique name used by a process on the bus.
- **Object path**: A path-like identifier for an object exposed by a service.
- **Interface**: A named collection of methods and signals.
- **Signal**: An asynchronous message sent by a service (like an event).

In this setup:

- Navit runs as a process.
- It exposes a service on the D-Bus session bus.
- Your external display connects to that service and listens for signals.


3. D-Bus Service Details
========================

3.1 Service and Bus
-------------------

- **Service name:** ``org.navit_project.navit``
- **Bus type:** Session bus (per-user bus, typically used for GUI and user apps)


3.2 Object Paths
----------------

Navit exposes several important objects:

- **Main Navit object:**

  - Path: ``/org/navit_project/navit/default_navit``
  - Role: Core object that lets you create callbacks and register attributes.

- **Route object:**

  - Path: ``/org/navit_project/navit/default_navit/default_route``
  - Role: Provides route-related information (next turn, distance to turn, ETA,
    remaining distance, route status).

- **Vehicle object:**

  - Path: ``/org/navit_project/navit/default_navit/default_vehicle``
  - Role: Provides current GPS position, speed, and heading.


3.3 Discovering Interfaces (Optional, but Helpful)
--------------------------------------------------

If you have ``gdbus`` installed, you can introspect the Navit service from a
terminal:

.. code-block:: bash

   # Introspect the main Navit object
   gdbus introspect \
       --session \
       --dest org.navit_project.navit \
       --object-path /org/navit_project/navit/default_navit

This shows the exact interfaces, methods, and signals exported by your Navit
build. Names like ``callback_attr_new`` and ``add_attr`` may appear there.
You should confirm them against your installation if you plan to ship a product.


4. How the Callback/Subscription Model Works
===========================================

4.1 High-Level Pattern
----------------------

Navit’s D-Bus binding uses a **callback registration** model:

1. The external display **connects** to Navit’s D-Bus service.
2. It **creates a callback object** in Navit (a "sink" for attribute updates).
3. It **registers attributes** (by name) that it wants to watch.
4. Navit then **emits signals** whenever those attributes change.
5. The external display **listens for signals** and updates its UI.

This is a **one-time setup at startup** (or after a reconnection). After that,
everything is event-driven.


4.2 Typical Methods and Signals
-------------------------------

The exact names may vary slightly by Navit version, but the common pattern is:

- Method on the main object to create a callback:

  - Example: ``callback_attr_new()`` → returns a new callback object path

- Method to register an attribute on that callback:

  - Example: ``add_attr(callback_path, attribute_name)``

- Signal emitted when a watched attribute changes:

  - Example (signature style): ``callback_attr_update(string attr_name, variant value)``

In the Python example later, we will use these names as **concrete working
placeholders**. If your build uses different names, you can adjust them by
checking via ``gdbus introspect`` or a D-Bus GUI tool such as ``d-feet``.


4.3 One-Time Setup vs. Continuous Operation
-------------------------------------------

- The **setup** (connecting, creating a callback, adding attributes) runs once on
  startup, or after a reconnection.
- After that, your code:

  - Waits in an event loop.
  - Receives D-Bus signals.
  - Updates its own internal state and UI.

There is **no ongoing command stream** from the display to Navit, and no need to
constantly poll for data.


5. Navigation State Attributes to Subscribe To
==============================================

Below are the key attributes to subscribe to for a functional navigation cluster.

For each attribute we list:

- **Attribute name** (string used with ``add_attr``)
- **Object source** (conceptual owner in Navit)
- **Meaning**
- **Example values**

5.1 Next Turn Type
------------------

- **Attribute:** ``navigation_next_turn``
- **Object:** Route (``default_route``)
- **Meaning:** Type of upcoming maneuver.
- **Examples:**

  - ``straight``
  - ``turn_slight_left``
  - ``turn_slight_right``
  - ``turn_left``
  - ``turn_right``
  - ``turn_sharp_left``
  - ``turn_sharp_right``
  - ``roundabout_r2`` (take exit 2)
  - ``merge``
  - ``keep_left``
  - ``keep_right``
  - ``u_turn``
  - ``destination``


5.2 Distance to Next Turn
--------------------------

- **Attribute:** ``navigation_distance_turn``
- **Object:** Route
- **Meaning:** Distance in metres to the next maneuver.
- **Examples:**

  - ``350`` (350 m)
  - ``1200`` (1.2 km)


5.3 Next Street Name (Human Readable)
-------------------------------------

- **Attribute:** ``navigation_street_name``
- **Object:** Route
- **Meaning:** Human-readable name of the next street/road.
- **Examples:**

  - ``"Main Street"``
  - ``"Airport Road"``


5.4 Next Street Name (Systematic / Route Number)
-------------------------------------------------

- **Attribute:** ``navigation_street_name_systematic``
- **Object:** Route
- **Meaning:** Systematic name or route number of the next road.
- **Examples:**

  - ``"A7"``
  - ``"E45"``
  - ``"US-101"``


5.5 Navigation Status
---------------------

- **Attribute:** ``navigation_status``
- **Object:** Route or main navigation state
- **Meaning:** High-level state of routing.
- **Typical values:**

  - ``no_route`` (no active route)
  - ``no_destination`` (no destination set)
  - ``position_wait`` (waiting for GPS position)
  - ``calculating`` (initial route calculation)
  - ``recalculating`` (route recalculation)
  - ``routing`` (route active and guiding)


5.6 Current Position (Latitude/Longitude)
-----------------------------------------

- **Attribute:** ``position_coord_geo``
- **Object:** Vehicle (``default_vehicle``)
- **Meaning:** Current GPS coordinates (latitude, longitude).
- **Examples:**

  - ``(52.5200, 13.4050)``  (Berlin)
  - ``(37.7749, -122.4194)`` (San Francisco)


5.7 Current Speed
-----------------

- **Attribute:** ``position_speed``
- **Object:** Vehicle
- **Meaning:** Current speed of the vehicle.
- **Units:** Usually km/h or m/s (check your Navit build; test with a known speed).
- **Examples:**

  - ``0.0``
  - ``50.0``  (50 km/h)
  - ``13.9``  (approx. 50 km/h in m/s)


5.8 Current Direction (Heading)
-------------------------------

- **Attribute:** ``position_direction``
- **Object:** Vehicle
- **Meaning:** Heading in degrees (0 or 360 = north).
- **Examples:**

  - ``0`` (north)
  - ``90`` (east)
  - ``180`` (south)
  - ``270`` (west)


5.9 Estimated Time of Arrival (ETA)
-----------------------------------

- **Attribute:** ``eta``
- **Object:** Route
- **Meaning:** Estimated time of arrival at the final destination.
- **Representation:** Depends on your build; common options:

  - Unix timestamp (seconds since epoch)
  - Seconds since local midnight

You can test with a running route and print the raw values to decide how to
render them (for example, format into a human-readable clock time).


5.10 Remaining Route Distance
-----------------------------

- **Attribute:** ``destination_length``
- **Object:** Route
- **Meaning:** Remaining distance from current position to destination.
- **Units:** Typically metres.
- **Examples:**

  - ``5000`` (5 km)
  - ``120000`` (120 km)


6. Maneuver Type Code Reference Table
=====================================

This table maps Navit maneuver codes to human-friendly descriptions and suggested
icon filenames. You can adapt the icon names to your own asset naming scheme.

.. list-table::
   :header-rows: 1
   :widths: 25 45 30

   * - Maneuver code
     - Description
     - Recommended icon
   * - ``straight``
     - Continue straight
     - ``straight.svg``
   * - ``turn_slight_left``
     - Slight left turn
     - ``turn_slight_left.svg``
   * - ``turn_slight_right``
     - Slight right turn
     - ``turn_slight_right.svg``
   * - ``turn_left``
     - Standard left turn
     - ``turn_left.svg``
   * - ``turn_right``
     - Standard right turn
     - ``turn_right.svg``
   * - ``turn_sharp_left``
     - Sharp left turn
     - ``turn_sharp_left.svg``
   * - ``turn_sharp_right``
     - Sharp right turn
     - ``turn_sharp_right.svg``
   * - ``roundabout_r1``
     - Roundabout, take exit 1
     - ``roundabout_exit_1.svg``
   * - ``roundabout_r2``
     - Roundabout, take exit 2
     - ``roundabout_exit_2.svg``
   * - ``roundabout_r3``
     - Roundabout, take exit 3
     - ``roundabout_exit_3.svg``
   * - ``roundabout_r4``
     - Roundabout, take exit 4
     - ``roundabout_exit_4.svg``
   * - ``roundabout_r5``
     - Roundabout, take exit 5
     - ``roundabout_exit_5.svg``
   * - ``roundabout_r6``
     - Roundabout, take exit 6
     - ``roundabout_exit_6.svg``
   * - ``u_turn``
     - U-turn
     - ``u_turn.svg``
   * - ``merge_left``
     - Merge to the left
     - ``merge_left.svg``
   * - ``merge_right``
     - Merge to the right
     - ``merge_right.svg``
   * - ``keep_left``
     - Keep left
     - ``keep_left.svg``
   * - ``keep_right``
     - Keep right
     - ``keep_right.svg``
   * - ``motorway_enter``
     - Enter motorway / highway
     - ``motorway_enter.svg``
   * - ``motorway_exit``
     - Leave motorway / highway
     - ``motorway_exit.svg``
   * - ``destination``
     - Destination reached
     - ``destination.svg``
   * - ``recalculating``
     - Route is being recalculated
     - ``recalculating.svg``

In your implementation, always handle **unknown codes** gracefully, for example by
showing a generic arrow or question mark icon.


6.1 Icon Sources for Dashboards and Clusters
--------------------------------------------

You can use either your own icon set (keyed to the maneuver codes above) or reuse
Navit's built-in assets.

**Navit's built-in XPM/SVG set**

Navit ships with an existing set of XPM and SVG icons that are already mapped to
its internal POI type names and to navigation maneuver types. Layout XML (e.g.
``navit.xml`` or platform-specific layouts) references these by filename; the
same names can be used by an external cluster to keep visuals consistent with
Navit's own UI.

**Official SVG icons in the Navit source tree**

The **official** SVG (and XPM) icons for POIs and navigation are located in the
Navit source tree under:

- **Directory:** ``navit/icons/``

There you will find both POI icons (e.g. ``airport.svg``, ``fuel.svg``, ``cafe.svg``,
``hospital.svg``, ``parking.svg``) and navigation maneuver icons (e.g.
``nav_left_1_bk.svg``, ``nav_roundabout_r2_bk.svg``, ``nav_straight_wh.svg``,
``nav_destination_wh.svg``). POI types in Navit use internal names like
``poi_airport``, ``poi_fuel``, etc.; the corresponding icon files typically drop
the ``poi_`` prefix (e.g. ``airport.svg``). You can browse this directory in any
Navit checkout or on the official repository (e.g. under ``navit-gps/navit`` or
your fork) to see the full set.

**Example SVG icons for external dashboards**

Example SVG icons suitable for use in external dashboards and instrument clusters
(including maneuver types referenced in this document) are available in the
``feature/navit-dash`` branch of the Navit repository:

- **URL:** https://github.com/Supermagnum/navit/tree/feature/navit-dash/docs/development/svg-examples

That folder contains sample SVGs (e.g. ``turn-right_1.svg``, ``roundabout-right_1.svg``,
``eco-mode.svg``) that you can copy, adapt, or use as a reference for naming and
style when building your own cluster assets.

**Eco routing symbol (eco-mode.svg)**

The ``eco-mode.svg`` icon is intended for use with Navit's energy-based or eco
routing features. It is compatible with the **Driver Break** plugin, which
provides configurable rest periods, fuel tracking, and optional energy-based
routing (e.g. for cycling and cars). To understand eco routing mode, rest-stop
suggestions, and how the plugin uses routing and POI data, see the Driver Break
plugin documentation:

- **Compatible branch:** https://github.com/Supermagnum/navit/tree/feature/driver-break
- **Plugin docs (eco routing, travel modes, POIs):** https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/index.rst


7. Startup Subscription Example (Python)
========================================

This section shows a **complete, self-contained Python 3 example** that:

- Connects to the D-Bus session bus
- Contacts the Navit service
- Creates a callback object
- Registers key navigation attributes
- Listens for updates and prints them

You can run this script on an embedded Linux device (e.g., Raspberry Pi) with
Navit running and the D-Bus binding enabled.

.. note::

   The exact method and signal names used by the D-Bus binding may differ between
   Navit versions. This example uses **typical names** that you can adapt by
   checking with ``gdbus introspect`` as shown earlier.

7.1 Full Python Example
-----------------------

.. code-block:: python

   #!/usr/bin/env python3
   """
   Simple Navit D-Bus listener for external dashboards / clusters.

   This script:
     - Connects to the D-Bus session bus
     - Talks to org.navit_project.navit
     - Creates a callback object
     - Registers key navigation attributes
     - Prints updates when Navit emits signals

   Requirements:
     - python3
     - python3-dbus
     - python3-gi
     - Navit running with libbinding_dbus.so enabled
   """

   import sys
   import traceback

   import dbus
   import dbus.mainloop.glib
   from gi.repository import GLib


   # Attributes we want to subscribe to.
   # You can add/remove attributes here as needed for your UI.
   WATCHED_ATTRIBUTES = [
       "navigation_next_turn",
       "navigation_distance_turn",
       "navigation_street_name",
       "navigation_street_name_systematic",
       "navigation_status",
       "position_coord_geo",
       "position_speed",
       "position_direction",
       "eta",
       "destination_length",
   ]


   class NavitListener:
       """
       Encapsulates D-Bus communication with Navit.

       - Creates a callback object (sink) on the Navit side
       - Registers attributes we want to watch
       - Receives D-Bus signals when values change
       """

       def __init__(self):
           # Make dbus-python use the GLib main loop.
           # This is necessary so that D-Bus signals are dispatched in the GLib loop.
           dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

           # Connect to the session bus (per-user message bus).
           self.bus = dbus.SessionBus()

           # Get a proxy for the main Navit object.
           # If this fails, Navit is probably not running or the D-Bus plugin is disabled.
           self.navit_obj = self.bus.get_object(
               "org.navit_project.navit",              # D-Bus service name
               "/org/navit_project/navit/default_navit"  # Object path
           )

           # Create an interface wrapper. The interface name must match what
           # the Navit D-Bus plugin exports. If in doubt, confirm via introspection.
           self.navit_iface = dbus.Interface(
               self.navit_obj,
               dbus_interface="org.navit_project.navit"  # Commonly used interface name
           )

           # Create a new callback object on the Navit side.
           # The method name and return type may differ slightly between builds.
           # In typical setups this returns an object path string.
           self.callback_path = self.navit_iface.callback_attr_new()

           print(f"[INFO] Created Navit callback object at: {self.callback_path}")

           # Register all attributes we want to monitor.
           for attr in WATCHED_ATTRIBUTES:
               print(f"[INFO] Registering attribute: {attr}")
               # We assume add_attr(callback_path, attr_name) style.
               # Check your D-Bus introspection data if this differs.
               self.navit_iface.add_attr(self.callback_path, attr)

           # Register a Python method as a D-Bus signal handler.
           # - signal_name: name of the signal method on the Navit side
           # - dbus_interface: interface that defines the signal
           # - path: object path for our callback object
           self.bus.add_signal_receiver(
               handler_function=self.on_attr_update,          # Python method to call
               signal_name="callback_attr_update",            # Signal name in Navit
               dbus_interface="org.navit_project.navit",      # Interface that owns the signal
               path=self.callback_path,                       # Specific callback object
           )

       def on_attr_update(self, attr_name, value):
           """
           D-Bus signal handler for attribute updates.

           Expected signature (example):
             - attr_name: string (name of the updated attribute)
             - value:     variant-like object (new value)

           Check your D-Bus introspection if the signature is different.
           """

           # For development, just print everything we get.
           print(f"[UPDATE] {attr_name} = {value}")

           # In a real cluster implementation, you would:
           #   - Store these values in an internal state object
           #   - Trigger UI redraws (e.g., update speed, ETA, next maneuver icon)
           #   - Possibly perform unit conversions (e.g., metres to km)

       def run(self):
           """
           Start the GLib main loop and listen for D-Bus signals.
           """
           print("[INFO] NavitListener is running. Press Ctrl+C to exit.")
           loop = GLib.MainLoop()

           try:
               loop.run()
           except KeyboardInterrupt:
               print("\n[INFO] Exiting on user request.")
               loop.quit()
           except Exception:
               print("[ERROR] Unexpected exception in main loop:")
               traceback.print_exc()
               loop.quit()


   def main():
       try:
           listener = NavitListener()
       except dbus.DBusException as e:
           # If we cannot connect to Navit, explain the likely cause clearly.
           print("[ERROR] Failed to connect to Navit over D-Bus.")
           print(f"        Details: {e}")
           print("        Is Navit running and is libbinding_dbus.so enabled in navit.xml?")
           sys.exit(1)
       except Exception:
           # Catch any other initialization errors for easier diagnostics.
           print("[ERROR] Unexpected error while setting up NavitListener:")
           traceback.print_exc()
           sys.exit(1)

       # If initialization succeeded, enter the main loop.
       listener.run()


   if __name__ == "__main__":
       main()

7.2 How to Test This Example
----------------------------

1. Ensure Navit is installed and the D-Bus binding is enabled in ``navit.xml``.
2. Start Navit (e.g., with a GUI or via a wrapper script).
3. Save the Python script above as ``navit_listener.py`` and make it executable:

   .. code-block:: bash

      chmod +x navit_listener.py

4. Install Python dependencies (on Debian/Ubuntu-like systems):

   .. code-block:: bash

      sudo apt-get install python3-dbus python3-gi

5. Run the listener:

   .. code-block:: bash

      ./navit_listener.py

6. Plan or start a route in Navit and move the vehicle position (real GPS or
   simulation). You should see printed updates for the attributes you registered.


8. Integrating with a UI Framework
==================================

Once you can receive updates, the next step is to **connect them to your UI**.

8.1 Qt / PyQt5
--------------

You have two main options.

**Option 1: Use Qt’s native D-Bus support (recommended for C++ / PyQt)**

- Use ``QDBusConnection::sessionBus()`` to connect to the D-Bus session bus.
- Use ``QDBusInterface`` to call Navit methods.
- Subclass ``QObject`` and define slots that are connected to D-Bus signals.
- In PyQt5, the equivalents are in ``PyQt5.QtDBus``.

This keeps everything in the Qt event loop and integrates well with widgets.

**Option 2: Use Python dbus + GLib in a worker thread**

- Run a GLib main loop (as in the example) in a separate thread.
- When updates arrive, push them to the Qt main thread:

  - Use ``QMetaObject.invokeMethod``
  - Or Qt signals/slots with ``Qt.QueuedConnection``

This approach is useful if you already have a working Python/dbus-based
listener and want to add a Qt UI around it.


8.2 GTK
-------

GTK already uses the GLib main loop, which is **compatible with our example**:

- Initialize D-Bus as in the Python example
  (``dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)``).
- Start your GTK application normally (``Gtk.main()``).
- Because both use GLib, D-Bus signals and GTK events are handled in the same loop.

In this design:

- Your D-Bus signal handlers can directly update GTK widgets.
- Or they can emit GObject signals that your UI layer connects to.


8.3 Web-Based Dashboards
------------------------

For a browser-based dashboard (e.g., HTML/JavaScript over HTTP):

1. **Bridge process (backend)**

   - Runs the Python (or C/C++/Rust) D-Bus listener.
   - Keeps an in-memory copy of the latest navigation state.
   - Exposes a local WebSocket (e.g., ``ws://localhost:8080``).

2. **WebSocket client (frontend)**

   - Runs in the browser.
   - Connects to the WebSocket.
   - Receives JSON messages with navigation state (e.g., next turn, distance).
   - Updates the UI (SVG icons, map, text).

This makes it easy to design dashboards using standard web technologies while
keeping all D-Bus details hidden in the backend process.


8.4 Other Event Loop Frameworks
-------------------------------

For any other event-driven framework (SDL, Kivy, custom loops):

- Run the D-Bus/GLib main loop:

  - Directly, if your framework can integrate GLib file descriptors; or
  - In a separate thread, pushing updates into a thread-safe queue.

- From your main loop:

  - Periodically read from the queue.
  - Update your display objects (gauges, text fields, icons) accordingly.


9. Handling Edge Cases
======================

A robust cluster or head unit must handle failures and unusual situations
gracefully. The following guidelines assume you have a main navigation state
model in your application.

9.1 Navit Not Running
---------------------

**Symptoms:**

- D-Bus connection to ``org.navit_project.navit`` fails.
- Your Python script prints an error like "Failed to connect".

**Recommended behavior:**

- Show a clear message such as "Navigation unavailable" or "Navit not running".
- Periodically (e.g., every 5–10 seconds) retry connection in the background.
- As soon as Navit appears, reconnect and re-register attributes.


9.2 GPS Signal Lost
-------------------

**Symptoms:**

- ``position_coord_geo`` and ``position_speed`` stop changing.
- ``navigation_status`` may continue reporting ``routing`` for some time.

**Recommended behavior:**

- If no position update has been received for a configurable timeout (e.g., 10 or
  30 seconds):

  - Show "No GPS" or "GPS signal lost".
  - Consider graying out speed/heading indicators.

- Resume normal display as soon as new valid position updates arrive.


9.3 Route Cancelled
-------------------

**Symptoms:**

- ``navigation_status`` becomes ``no_destination`` or ``no_route``.
- Maneuver-related attributes may reset or become irrelevant.

**Recommended behavior:**

- Clear next-turn icon and distance fields.
- Show a neutral state: "No active route".
- Keep GPS position, speed, and heading visible if still available.


9.4 Route Being Recalculated
----------------------------

**Symptoms:**

- ``navigation_status`` becomes ``recalculating``.
- Distance and maneuver attributes may be briefly inconsistent.

**Recommended behavior:**

- Show a "Recalculating route" indicator.
- Optionally retain the last instruction until new data arrives.
- Once status returns to ``routing``, resume normal guidance.


9.5 Navit Exits and Restarts
----------------------------

**Symptoms:**

- D-Bus connection drops.
- You no longer receive signals.
- Service name ``org.navit_project.navit`` disappears, then reappears later.

**Recommended behavior:**

1. **On disconnect:**

   - Detect the lost connection (e.g., via exception or NameOwnerChanged signal).
   - Switch UI to "Navigation unavailable".

2. **On reconnect:**

   - Wait until ``org.navit_project.navit`` reappears on the bus.
   - Recreate the NavitListener:

     - Reconnect to the bus
     - Create a new callback object
     - Re-add all attributes

   - Once updates start flowing again, resume normal display.


10. Reference Implementation Notes
==================================

10.1 No Changes Required in Navit
---------------------------------

This entire approach relies on Navit’s existing D-Bus binding:

- You **do not** need to modify Navit’s core code.
- You **do not** need to create a custom Navit plugin.
- The only configuration change is enabling ``libbinding_dbus.so`` in
  ``navit.xml``.

Your cluster or head unit is a **separate process** that just happens to speak D-Bus.


10.2 Open Protocol for External Displays
----------------------------------------

Taken together, this document and the Python example define a practical,
implementation-oriented specification:

- How to connect to Navit’s D-Bus service
- How to register a callback and attributes
- Which attributes represent navigation state
- How to respond to updates and edge cases

Any external display that follows this pattern is:

- Compatible with any standard Navit installation that exposes the same D-Bus
  interface.
- Interoperable with other implementations that understand the same attributes
  and maneuver codes.


10.3 Contributing Implementations Back
--------------------------------------

To help the ecosystem grow, consider:

- Publishing your cluster or head unit code under an open-source license.
- Sharing:

  - Reusable navigation widgets
  - Icon sets for maneuver types
  - Integration code for frameworks (Qt, GTK, web, etc.)

- Proposing improvements to this document (additional attributes, better error
  handling patterns, etc.).

By sharing reference implementations, the Navit community can build a consistent,
open standard for Navit-based external dashboards, instrument clusters, and
head units.

