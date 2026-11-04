Dbus
====

The Navit dbus interface allows you to control certain aspects of Navit
from outside the program. This document displays the requirements and
implementation details of the Navit dbus interface.

Enabling
========

To be able to send and receive signals from Navit, you must enable it in
navit.xml. Change line 13 to the following:

.. code:: xml

    <plugin path="$NAVIT_LIBDIR/*/${NAVIT_LIBPREFIX}libbinding_dbus.so" active="yes"/>

.. _common_actions:

Common actions
==============

.. _zoom_inout:

Zoom in/out
-----------

Zoom to level 500:

.. code:: bash

   dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit org.navit_project.navit.navit.set_attr string:"zoom" variant:int32:500

Zoom in by factor of 2:

.. code:: bash

   dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit org.navit_project.navit.navit.zoom int32:2

Zoom out by factor of 2:

.. code:: bash

   dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit org.navit_project.navit.navit.zoom int32:-2

.. _set_positiondestination:

Set Position/Destination
------------------------

Set position to longitude 24.0, latitude 65.0:

.. code:: bash

   dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit org.navit_project.navit.navit.set_position string:"geo: 24.0 65.0"

Set destination to longitude 24.0, latitude 65.0:

.. code:: bash

   dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit org.navit_project.navit.navit.set_destination string:"geo: 24.0 65.0" string:"comment"

.. _set_map_view_centre:

Set map-view centre
-------------------

Centre the map to longitude 24.0, latitude 65.0:

.. code:: bash

   dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit org.navit_project.navit.navit.set_center_by_string string:"geo: 24.0 65.0"

.. _change_map_layout:

Change map layout
-----------------

Change layout to "Car-dark":

.. code:: bash

   dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit org.navit_project.navit.navit.set_layout string:"Car-dark"

.. _change_vehicle_profile:

Change vehicle profile
----------------------

Change profile to "bike":

.. code:: bash

   dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit/default_vehicle org.navit_project.navit.vehicle.set_attr string:"profilename" variant:string:"bike"

.. _navit_dbus_details:

Navit DBus details
==================

| DBUS Service: "org.navit_project.navit"
| DBUS Bus: Session bus

org.navit_project.navit.navit
=============================

DBUS Object Path: /org/navit_project/navit/navit/[]

The way navit uses DBus is not the usual way. To control your navit
instance (set_destination, zoom, …), you must first obtain the navit
object path. The following complete Python example demonstrates the
process:

.. code:: python

   import dbus

   # 1) Connect to the session bus
   bus = dbus.SessionBus()

   # 2) Get the navit general object
   obj = bus.get_object("org.navit_project.navit", "/org/navit_project/navit")

   # 3) Get the interface
   iface = dbus.Interface(obj, dbus_interface="org.navit_project.navit")

   # 4) Get an iterator through navit instances
   iter = iface.attr_iter()

   # 5) Get the object path of the navit instance
   path = obj.get_attr_wi("navit", iter)
   # path is a tuple: ("navit", "/org/navit_project/navit/default_navit")

   # 6) Get the navit instance you are interested in
   navit = bus.get_object('org.navit_project.navit', path[1])

   # 7) Destroy the iterator when done
   iface.attr_iter_destroy(iter)

   # Now you can call methods on the navit object
   navit_iface = dbus.Interface(navit, dbus_interface="org.navit_project.navit.navit")
   navit_iface.zoom(2)  # Zoom in by factor of 2

The following methods have to be applied on the navit object.

Methods
-------

Examples are shown for each command, and when using on the command line
must be prefixed by:

``dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit``

When a coordinate is specified, the coordinate can be formatted like:

``[Proj:]-0xX [-]0xX``

where ``Proj`` can be ``mg`` or ``garmin``. Default: ``mg``. For
example:

``mg: 0x138a4a 0x5d773f``

Or

| ``[Proj:][D][D]Dmm.ss[S][S] N/S [D][D]DMM.ss[S][S]... E/W``
| ``[Proj:][-][D]D.d[d]... [-][D][D]D.d[d]``

where ``Proj`` must be ``geo``. For example:

``geo: 24.0 65.0``

.. list-table:: org.navit_project.navit.navit methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_draw:

       **draw**
     - ``.navit``
     - *none*
     - *none*
     - Forces a redraw of the current view of the map.
     - .. code:: bash

          org.navit_project.navit.navit.draw
   * - .. _dbus_method_add_message:

       **add_message**
     - ``.navit``
     - string: "message"
     - *none*
     - FIXME. Adds a message to the messages label.
     - .. code:: bash

          org.navit_project.navit.navit.add_message string:"My Message"
   * - .. _dbus_method_set_center_by_string:

       **set_center_by_string**
     - ``.navit``
     - string: ``coordinates``
     - *none*
     - Centres the map over the specified position.
     - .. code:: bash

          org.navit_project.navit.navit.set_center_by_string string:"geo: 24.0 65.0"
   * - .. _dbus_method_set_center:

       **set_center**
     - ``.navit``
     - | int: projection, string: coordinates
       | int: projection, int: longitude, int: latitude
     - *none*
     - Centres the map over the specified position.
     - .. code:: bash

          org.navit_project.navit.navit.set_center int32:1 string:"geo: 24.0 65.0"
          org.navit_project.navit.navit.set_center int32:1 int32:0x138a4a int32:0x5d773f
   * - .. _dbus_method_set_center_screen:

       **set_center_screen**
     - ``.navit``
     - | integer: ``pixel_x``, integer: ``pixel_y``
     - *none*
     - Centers the map to a specific position on the screen.
     - .. code:: bash

          org.navit_project.navit.navit.set_center_screen int32:200 int32:400
   * - .. _dbus_method_set_layout:

       **set_layout**
     - ``.navit``
     - string: ``layoutname``
     - *none*
     - Changes the active map layout.
     - .. code:: bash

          org.navit_project.navit.navit.set_layout string:"car-dark"
   * - .. _dbus_method_zoom:

       **zoom**
     - ``.navit``
     - integer: ``factor`` (integer: ``pixel_x``, integer: ``pixel_y``)
     - *none*
     - Zooms into the map (positive ``factor``) or out of the
       map (negative ``factor``) by the factor specified. The factor must be
       integers greater than 1 (zoom in) or less than -1 (zoom out).
       Optionally, a screen position can be specified in ``pixel_x`` and
       ``pixel_y`` and the function will zoom into that particular position.
       In order to zoom to a specific zoom-level, use the ``set_attr`` method.
     - .. code:: bash

          org.navit_project.navit.navit.zoom int32:2

       .. code:: bash

          org.navit_project.navit.navit.zoom int32:6 int32:200 int32:400
   * - .. _dbus_method_block:

       **block**
     - ``.navit``
     - integer: ``mode``
     - *none*
     - FIXME.
     - .. code:: bash

          org.navit_project.navit.navit.block int32:1
   * - .. _dbus_method_set_position:

       **set_position**
     - ``.navit``
     - | string: ``coordinates``
       | integer: ``projection``, string: ``coordinates``
       | integer: ``projection``, integer: ``longitude``, integer: ``latitude``
     - *none*
     - Sets the current position (useful if no gps position
       is available).
     - .. code:: bash

          org.navit_project.navit.navit.set_position string:"geo: 24.0 65.0"

       .. code:: bash

          org.navit_project.navit.navit.set_position int32:1 string:"geo: 24.0 65.0"

       .. code:: bash

          org.navit_project.navit.navit.set_position int32:1 int32:0x138a4a int32:0x5d773f
   * - .. _dbus_method_set_destination:

       **set_destination**
     - ``.navit``
     - | string: ``coordinates``, string: ``"comment"``
       | integer: ``projection``, string: ``coordinates``, string: ``"comment"``
       | integer: ``projection``, integer: ``longitude``, integer: ``latitude``, string: ``"comment"``
     - *none*
     - Sets the destination for routing.
     - .. code:: bash

          org.navit_project.navit.navit.set_destination string:"geo: 24.0 65.0"

       .. code:: bash

          org.navit_project.navit.navit.set_destination int32:1 string:"geo: 24.0 65.0"

       .. code:: bash

          org.navit_project.navit.navit.set_destination int32:1 int32:0x138a4a int32:0x5d773f
   * - .. _dbus_method_clear_destination:

       **clear_destination**
     - ``.navit``
     - *none*
     - *none*
     - Removes the destination and stops routing.
     - .. code:: bash

          org.navit_project.navit.navit.clear_destination
   * - .. _dbus_method_get_attr:

       **get_attr**
     - ``.navit``
     - string: ``attribute``
     - string: ``attribute`` variant: ``value``
     - Gets the specified attribute value. The attribute can
       be anything from attr_def.h.
     - .. code:: bash

          org.navit_project.navit.navit.get_attr string:"zoom"

       returns:

       .. code:: bash

          string "zoom"
          variant    int32 10

       .. code:: bash

          org.navit_project.navit.navit.get_attr string:"orientation"

       returns:

       .. code:: bash

          string "orientation"
          variant    int32 1
   * - .. _dbus_method_set_attr:

       **set_attr**
     - ``.navit``
     - string: ``attribute``, variant: ``value``
     - *none*
     - Sets the specified attribute value. The attribute can
       be anything from attr_def.h. As shown in the example below, this can be
       useful to zoom to a specific zoom level, instead of just zooming by a
       factor as with the ``zoom`` method.
     - .. code:: bash

          org.navit_project.navit.navit.set_attr string:"zoom" variant:int32:500
   * - .. _dbus_method_export_as_gpx:

       **export_as_gpx**
     - ``.navit``
     - string: ``filename``
     - *none*
     - Dump the current route, if any, into the file specified
       in the argument in the GPX format.
     - .. code:: bash

          org.navit_project.navit.navit.export_as_gpx string:"/tmp/route.gpx"

org.navit_project.navit.vehicleprofile
======================================

.. _methods_1:

Methods
-------

Examples are shown for each command, and when using on the command line
must be prefixed by:

``dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit/default_vehicleprofile``

.. list-table:: org.navit_project.navit.vehicleprofile methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_vehicleprofile_get_attr:

       **get_attr**
     - ``.vehicleprofile``
     - string: ``attribute``
     - string: ``attribute`` variant: ``value``
     - Gets the specified attribute value for the ``vehicleprofile``.
     - .. code:: bash

          org.navit_project.navit.vehicleprofile.get_attr string:"name"

       returns:

       .. code:: bash

          string "name"
          variant    string "car"

       .. code:: bash

          org.navit_project.navit.vehicleprofile.get_attr string:"static_speed"

       returns:

       .. code:: bash

          string "static_speed"
          variant    int32 5
   * - .. _dbus_method_vehicleprofile_set_attr:

       **set_attr**
     - ``.vehicleprofile``
     - string: ``attribute``, variant: ``value``
     - *none*
     - Sets the specified attribute value. The example below
       *renames* the current ``vehicleprofile``. To actually *change* the
       ``vehicleprofile``, use the ``set_attr`` method in the ``.vehicle`` path
       (see below).
     - .. code:: bash

          org.navit_project.navit.vehicleprofile.set_attr string:"name" variant:string:"bike"

org.navit_project.navit.vehicle
===============================

.. _methods_2:

Methods
-------

Examples are shown for each command, and when using on the command line
must be prefixed by:

``dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit/default_vehicle``

.. list-table:: org.navit_project.navit.vehicle methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_vehicle_set_attr:

       **set_attr**
     - ``.vehicle``
     - string: ``attribute``, variant: ``value``
     - *none*
     - Sets the specified attribute value. The example below
       changes the current ``vehicleprofile`` to ``"bike"``.
     - .. code:: bash

          org.navit_project.navit.vehicle.set_attr string:"profilename" variant:string:"bike"

org.navit_project.navit.route
=============================

.. _methods_3:

Methods
-------

Examples are shown for each command, and when using on the command line
must be prefixed by:

``dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit/default_route``

.. list-table:: org.navit_project.navit.route methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_route_get_attr:

       **get_attr**
     - ``.route``
     - string: ``attribute``
     - string: ``attribute`` variant: ``value``
     - Gets the specified attribute value. The attribute can
       be anything from route_get_attr() in navit/route.c
     - .. code:: bash

          org.navit_project.navit.route.get_attr string:"destination_length"

       returns:

       .. code:: bash

          string "destination_length"
          variant    int32 338111

Signals
=======

.. _add_bookmark_signal:

add bookmark signal
-------------------

Setup the callback:

.. code:: bash

   dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit org.navit_project.navit.callback_attr_new string:my_signal string:bookmark_map object path "/org/navit_project/navit/callback/0"
   dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit org.navit_project.navit.navit.add_attr string:callback variant:objpath:/org/navit_project/navit/callback/0

Replace "my_signal" with a signal name of your choice.

Now add a bookmark and you should see this in dbus-monitor --session:

.. code:: bash

   signal sender=:1.927 -> dest=(null destination) path=/org/navit_project/navit; interface=org.navit_project.navit; member=my_signal

.. _undocumented_methods:

Additional interfaces
=====================

The following interfaces are available but were not previously documented.

org.navit_project.navit.config
------------------------------

These methods are available on the root config object (path: ``/org/navit_project/navit``).

.. list-table:: org.navit_project.navit.config methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_config_attr_iter:

       **attr_iter**
     - 
     - *none*
     - ``o`` (object path)
     - Returns an iterator for config attributes.
     - .. code:: bash

          org.navit_project.navit.config.attr_iter
   * - .. _dbus_method_config_attr_iter_destroy:

       **attr_iter_destroy**
     - 
     - ``o`` (attr_iter)
     - *none*
     - Destroys a config attribute iterator.
     - .. code:: bash

          org.navit_project.navit.config.attr_iter_destroy object_path:"/org/navit_project/navit/attr_iter/0"
   * - .. _dbus_method_config_get_attr:

       **get_attr**
     - 
     - string: ``attrname``
     - string: ``attrname`` variant: ``value``
     - Gets a config attribute value.
     - .. code:: bash

          org.navit_project.navit.config.get_attr string:"zoom"
   * - .. _dbus_method_config_get_attr_wi:

       **get_attr_wi**
     - 
     - string: ``attrname``, ``o`` (attr_iter)
     - string: ``attrname`` variant: ``value``
     - Gets a config attribute value using an iterator.
     - .. code:: bash

          org.navit_project.navit.config.get_attr_wi string:"zoom" object_path:"/org/navit_project/navit/attr_iter/0"
   * - .. _dbus_method_config_callback_new:

       **callback_new**
     - 
     - string: ``signalname``
     - ``o`` (object path)
     - Creates a new callback for a signal.
     - .. code:: bash

          org.navit_project.navit.config.callback_new string:"my_signal"
   * - .. _dbus_method_config_callback_attr_new:

       **callback_attr_new**
     - 
     - string: ``signalname``, string: ``attribute``
     - ``o`` (object path)
     - Creates a new callback for a signal with attribute filter.
     - .. code:: bash

          org.navit_project.navit.config.callback_attr_new string:"my_signal" string:"zoom"
   * - .. _dbus_method_config_search_list_new:

       **search_list_new**
     - 
     - ``o`` (mapset)
     - ``o`` (object path)
     - Creates a new search list for a mapset.
     - .. code:: bash

          org.navit_project.navit.config.search_list_new object_path:"/org/navit_project/navit/default_mapset"

org.navit_project.navit.callback
--------------------------------

.. list-table:: org.navit_project.navit.callback methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_callback_destroy:

       **destroy**
     - ``.callback``
     - *none*
     - *none*
     - Destroys a callback object.
     - .. code:: bash

          org.navit_project.navit.callback.destroy

org.navit_project.navit.graphics
--------------------------------

.. list-table:: org.navit_project.navit.graphics methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_graphics_get_data:

       **get_data**
     - ``.graphics``
     - string: ``type``
     - ``ay`` (byte array)
     - Gets graphics data of the specified type.
     - .. code:: bash

          org.navit_project.navit.graphics.get_data string:"screenshot"
   * - .. _dbus_method_graphics_set_attr:

       **set_attr**
     - ``.graphics``
     - string: ``attribute`` variant: ``value``
     - *none*
     - Sets a graphics attribute.
     - .. code:: bash

          org.navit_project.navit.graphics.set_attr string:"width" variant:int32:800

org.navit_project.navit.gui
----------------------------

.. list-table:: org.navit_project.navit.gui methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_gui_get_attr:

       **get_attr**
     - ``.gui``
     - string: ``attribute``
     - string: ``attribute`` variant: ``value``
     - Gets a GUI attribute value.
     - .. code:: bash

          org.navit_project.navit.gui.get_attr string:"active"
   * - .. _dbus_method_gui_command_parameter:

       **command_parameter**
     - ``.gui``
     - string: ``command``, dict: ``parameter``
     - dict: ``return``
     - Executes a GUI command with parameters.
     - .. code:: bash

          org.navit_project.navit.gui.command_parameter string:"menu" dict:{}
   * - .. _dbus_method_gui_command:

       **command**
     - ``.gui``
     - string: ``command``
     - dict: ``return``
     - Executes a GUI command.
     - .. code:: bash

          org.navit_project.navit.gui.command string:"menu"

org.navit_project.navit.map
----------------------------

.. list-table:: org.navit_project.navit.map methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_map_get_attr:

       **get_attr**
     - ``.map``
     - string: ``attribute``
     - string: ``attribute`` variant: ``value``
     - Gets a map attribute value.
     - .. code:: bash

          org.navit_project.navit.map.get_attr string:"name"
   * - .. _dbus_method_map_set_attr:

       **set_attr**
     - ``.map``
     - string: ``attribute`` variant: ``value``
     - *none*
     - Sets a map attribute value.
     - .. code:: bash

          org.navit_project.navit.map.set_attr string:"enabled" variant:boolean:true

org.navit_project.navit.mapset
-------------------------------

.. list-table:: org.navit_project.navit.mapset methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_mapset_attr_iter:

       **attr_iter**
     - ``.mapset``
     - *none*
     - ``o`` (object path)
     - Returns an iterator for mapset attributes.
     - .. code:: bash

          org.navit_project.navit.mapset.attr_iter
   * - .. _dbus_method_mapset_attr_iter_destroy:

       **attr_iter_destroy**
     - ``.mapset``
     - ``o`` (attr_iter)
     - *none*
     - Destroys a mapset attribute iterator.
     - .. code:: bash

          org.navit_project.navit.mapset.attr_iter_destroy object_path:"/org/navit_project/navit/mapset/attr_iter/0"
   * - .. _dbus_method_mapset_get_attr:

       **get_attr**
     - ``.mapset``
     - string: ``attribute``
     - string: ``attribute`` variant: ``value``
     - Gets a mapset attribute value.
     - .. code:: bash

          org.navit_project.navit.mapset.get_attr string:"name"
   * - .. _dbus_method_mapset_get_attr_wi:

       **get_attr_wi**
     - ``.mapset``
     - string: ``attribute``, ``o`` (attr_iter)
     - string: ``attribute`` variant: ``value``
     - Gets a mapset attribute value using an iterator.
     - .. code:: bash

          org.navit_project.navit.mapset.get_attr_wi string:"name" object_path:"/org/navit_project/navit/mapset/attr_iter/0"

org.navit_project.navit.navigation
----------------------------------

.. list-table:: org.navit_project.navit.navigation methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_navigation_get_attr:

       **get_attr**
     - ``.navigation``
     - string: ``attribute``
     - *none* (see description)
     - Gets a navigation attribute value. Return type varies by attribute.
     - .. code:: bash

          org.navit_project.navit.navigation.get_attr string:"destination_length"

org.navit_project.navit.route
------------------------------

.. list-table:: org.navit_project.navit.route methods (additional)
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_route_set_attr:

       **set_attr**
     - ``.route``
     - string: ``attribute`` variant: ``value``
     - *none*
     - Sets a route attribute value.
     - .. code:: bash

          org.navit_project.navit.route.set_attr string:"avoid_tolls" variant:boolean:true
   * - .. _dbus_method_route_add_attr:

       **add_attr**
     - ``.route``
     - string: ``attribute`` variant: ``value``
     - *none*
     - Adds a route attribute.
     - .. code:: bash

          org.navit_project.navit.route.add_attr string:"waypoint" variant:object_path:"/org/navit_project/navit/waypoint/0"
   * - .. _dbus_method_route_remove_attr:

       **remove_attr**
     - ``.route``
     - string: ``attribute`` variant: ``value``
     - *none*
     - Removes a route attribute.
     - .. code:: bash

          org.navit_project.navit.route.remove_attr string:"waypoint" variant:object_path:"/org/navit_project/navit/waypoint/0"

org.navit_project.navit.search_list
------------------------------------

.. list-table:: org.navit_project.navit.search_list methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_search_list_destroy:

       **destroy**
     - ``.search_list``
     - *none*
     - *none*
     - Destroys a search list object.
     - .. code:: bash

          org.navit_project.navit.search_list.destroy
   * - .. _dbus_method_search_list_get_result:

       **get_result**
     - ``.search_list``
     - *none*
     - ``i(iii)a{sa{sv}}`` (id, coord, dict)
     - Gets the next search result.
     - .. code:: bash

          org.navit_project.navit.search_list.get_result
   * - .. _dbus_method_search_list_search:

       **search**
     - ``.search_list``
     - string: ``attribute`` variant: ``value`` int32: ``partial``
     - *none*
     - Performs a search with the given attribute and value.
     - .. code:: bash

          org.navit_project.navit.search_list.search string:"name" variant:string:"Berlin" int32:1
   * - .. _dbus_method_search_list_select:

       **select**
     - ``.search_list``
     - string: ``attribute_type`` int32: ``id`` int32: ``mode``
     - *none*
     - Selects a search result.
     - .. code:: bash

          org.navit_project.navit.search_list.select string:"name" int32:0 int32:1

org.navit_project.navit.tracking
---------------------------------

.. list-table:: org.navit_project.navit.tracking methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_tracking_get_attr:

       **get_attr**
     - ``.tracking``
     - string: ``attribute``
     - *none* (see description)
     - Gets a tracking attribute value. Return type varies by attribute.
     - .. code:: bash

          org.navit_project.navit.tracking.get_attr string:"position"

org.navit_project.navit.navit (additional)
-------------------------------------------

Additional methods on the navit object.

.. list-table:: org.navit_project.navit.navit methods (additional)
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_navit_resize:

       **resize**
     - ``.navit``
     - int32: ``upperleft``, int32: ``lowerright``
     - *none*
     - Resizes the navit display area.
     - .. code:: bash

          org.navit_project.navit.navit.resize int32:0 int32:0 int32:800 int32:480
   * - .. _dbus_method_navit_attr_iter:

       **attr_iter**
     - ``.navit``
     - *none*
     - ``o`` (object path)
     - Returns an iterator for navit attributes.
     - .. code:: bash

          org.navit_project.navit.navit.attr_iter
   * - .. _dbus_method_navit_attr_iter_destroy:

       **attr_iter_destroy**
     - ``.navit``
     - ``o`` (attr_iter)
     - *none*
     - Destroys a navit attribute iterator.
     - .. code:: bash

          org.navit_project.navit.navit.attr_iter_destroy object_path:"/org/navit_project/navit/navit/attr_iter/0"
   * - .. _dbus_method_navit_get_attr_wi:

       **get_attr_wi**
     - ``.navit``
     - string: ``attribute``, ``o`` (attr_iter)
     - string: ``attribute`` variant: ``value``
     - Gets a navit attribute value using an iterator.
     - .. code:: bash

          org.navit_project.navit.navit.get_attr_wi string:"zoom" object_path:"/org/navit_project/navit/navit/attr_iter/0"
   * - .. _dbus_method_navit_add_attr:

       **add_attr**
     - ``.navit``
     - string: ``attribute`` variant: ``value``
     - *none*
     - Adds a navit attribute.
     - .. code:: bash

          org.navit_project.navit.navit.add_attr string:"callback" variant:object_path:"/org/navit_project/navit/callback/0"
   * - .. _dbus_method_navit_remove_attr:

       **remove_attr**
     - ``.navit``
     - string: ``attribute`` variant: ``value``
     - *none*
     - Removes a navit attribute.
     - .. code:: bash

          org.navit_project.navit.navit.remove_attr string:"callback" variant:object_path:"/org/navit_project/navit/callback/0"
   * - .. _dbus_method_navit_evaluate:

       **evaluate**
     - ``.navit``
     - string: ``command``
     - string: ``result``
     - Evaluates a navit command string.
     - .. code:: bash

          org.navit_project.navit.navit.evaluate string:"zoom(2)"
   * - .. _dbus_method_navit_quit:

       **quit**
     - ``.navit``
     - *none*
     - *none*
     - Quits the navit application.
     - .. code:: bash

          org.navit_project.navit.navit.quit
   * - .. _dbus_method_navit_traffic_export_gpx:

       **traffic_export_gpx**
     - ``.navit``
     - string: ``filename``
     - *none*
     - Exports traffic data to a GPX file.
     - .. code:: bash

          org.navit_project.navit.navit.traffic_export_gpx string:"/tmp/traffic.gpx"
   * - .. _dbus_method_navit_traffic_inject:

       **traffic_inject**
     - ``.navit``
     - string: ``filename``
     - *none*
     - Injects traffic data from a file.
     - .. code:: bash

          org.navit_project.navit.navit.traffic_inject string:"/tmp/traffic.gpx"
   * - .. _dbus_method_navit_zoom_to_route:

       **zoom_to_route**
     - ``.navit``
     - *none*
     - *none*
     - Zooms the map to fit the current route.
     - .. code:: bash

          org.navit_project.navit.navit.zoom_to_route
   * - .. _dbus_method_navit_export_as_geojson:

       **export_as_geojson**
     - ``.navit``
     - string: ``filename``
     - *none*
     - Exports the current route to a GeoJSON file.
     - .. code:: bash

          org.navit_project.navit.navit.export_as_geojson string:"/tmp/route.geojson"

org.navit_project.navit.layout
-------------------------------

.. list-table:: org.navit_project.navit.layout methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_layout_get_attr:

       **get_attr**
     - ``.layout``
     - string: ``attribute``
     - string: ``attribute`` variant: ``value``
     - Gets a layout attribute value.
     - .. code:: bash

          org.navit_project.navit.layout.get_attr string:"name"

org.navit_project.navit.map
----------------------------

.. list-table:: org.navit_project.navit.map methods (additional)
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_map_dump:

       **dump**
     - ``.map``
     - string: ``file``
     - *none*
     - Dumps the map data to a file.
     - .. code:: bash

          org.navit_project.navit.map.dump string:"/tmp/map.bin"

org.navit_project.navit.osd
----------------------------

.. list-table:: org.navit_project.navit.osd methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_osd_get_attr:

       **get_attr**
     - ``.osd``
     - string: ``attribute``
     - string: ``attribute`` variant: ``value``
     - Gets an OSD attribute value.
     - .. code:: bash

          org.navit_project.navit.osd.get_attr string:"enabled"
   * - .. _dbus_method_osd_set_attr:

       **set_attr**
     - ``.osd``
     - string: ``attribute`` variant: ``value``
     - *none*
     - Sets an OSD attribute value.
     - .. code:: bash

          org.navit_project.navit.osd.set_attr string:"enabled" variant:boolean:true

org.navit_project.navit.roadprofile
------------------------------------

.. list-table:: org.navit_project.navit.roadprofile methods
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_roadprofile_get_attr:

       **get_attr**
     - ``.roadprofile``
     - string: ``attribute``
     - string: ``attribute`` variant: ``value``
     - Gets a road profile attribute value.
     - .. code:: bash

          org.navit_project.navit.roadprofile.get_attr string:"speed"
   * - .. _dbus_method_roadprofile_get_attr_wi:

       **get_attr_wi**
     - ``.roadprofile``
     - string: ``attribute``, ``o`` (attr_iter)
     - string: ``attribute`` variant: ``value``
     - Gets a road profile attribute value using an iterator.
     - .. code:: bash

          org.navit_project.navit.roadprofile.get_attr_wi string:"speed" object_path:"/org/navit_project/navit/roadprofile/attr_iter/0"
   * - .. _dbus_method_roadprofile_set_attr:

       **set_attr**
     - ``.roadprofile``
     - string: ``attribute`` variant: ``value``
     - *none*
     - Sets a road profile attribute value.
     - .. code:: bash

          org.navit_project.navit.roadprofile.set_attr string:"speed" variant:int32:50
   * - .. _dbus_method_roadprofile_attr_iter:

       **attr_iter**
     - ``.roadprofile``
     - *none*
     - ``o`` (object path)
     - Returns an iterator for road profile attributes.
     - .. code:: bash

          org.navit_project.navit.roadprofile.attr_iter
   * - .. _dbus_method_roadprofile_attr_iter_destroy:

       **attr_iter_destroy**
     - ``.roadprofile``
     - ``o`` (attr_iter)
     - *none*
     - Destroys a road profile attribute iterator.
     - .. code:: bash

          org.navit_project.navit.roadprofile.attr_iter_destroy object_path:"/org/navit_project/navit/roadprofile/attr_iter/0"

org.navit_project.navit.route (additional)
-------------------------------------------

.. list-table:: org.navit_project.navit.route methods (additional)
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_route_destroy:

       **destroy**
     - ``.route``
     - *none*
     - *none*
     - Destroys the route object.
     - .. code:: bash

          org.navit_project.navit.route.destroy
   * - .. _dbus_method_route_dup:

       **dup**
     - ``.route``
     - *none*
     - ``o`` (object path)
     - Duplicates the route object.
     - .. code:: bash

          org.navit_project.navit.route.dup

org.navit_project.navit.vehicleprofile (additional)
----------------------------------------------------

.. list-table:: org.navit_project.navit.vehicleprofile methods (additional)
   :widths: 12 8 18 10 25 27
   :header-rows: 1

   * - Method
     - Path
     - Arguments
     - Return
     - Description
     - Example
   * - .. _dbus_method_vehicleprofile_attr_iter:

       **attr_iter**
     - ``.vehicleprofile``
     - *none*
     - ``o`` (object path)
     - Returns an iterator for vehicle profile attributes.
     - .. code:: bash

          org.navit_project.navit.vehicleprofile.attr_iter
   * - .. _dbus_method_vehicleprofile_attr_iter_destroy:

       **attr_iter_destroy**
     - ``.vehicleprofile``
     - ``o`` (attr_iter)
     - *none*
     - Destroys a vehicle profile attribute iterator.
     - .. code:: bash

          org.navit_project.navit.vehicleprofile.attr_iter_destroy object_path:"/org/navit_project/navit/vehicleprofile/attr_iter/0"
   * - .. _dbus_method_vehicleprofile_get_attr_wi:

       **get_attr_wi**
     - ``.vehicleprofile``
     - string: ``attribute``, ``o`` (attr_iter)
     - string: ``attribute`` variant: ``value``
     - Gets a vehicle profile attribute value using an iterator.
     - .. code:: bash

          org.navit_project.navit.vehicleprofile.get_attr_wi string:"name" object_path:"/org/navit_project/navit/vehicleprofile/attr_iter/0"
