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


.. _coordinate-format-reference:

Coordinate Format Reference
===========================

For a complete description of all supported coordinate formats (including
decimal degrees, degrees/minutes, UTM, and cartesian), see
:doc:`Coordinate format </user/configuration/Coordinate_format>`.

In brief, DBus accepts:

* **MG/Garmin format** (cartesian coordinates): ``[Proj:]-0xX [-]0xX`` (e.g. ``mg: 0x138a4a 0x5d773f``)
* **Geo format** (decimal degrees): ``geo: <longitude> <latitude>`` (e.g. ``geo: 24.0 65.0``)


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

.. _dbus_method_draw:

draw
~~~~

================ =================================================
**Path:**        ``.navit``
**Arguments:**   *none*
**Return:**      *none*
**Description:** Forces a redraw of the current view of the map.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.draw
================ =================================================

.. _dbus_method_add_message:

add_message
~~~~~~~~~~~

================ ==================================================================
**Path:**        ``.navit``
**Arguments:**   string: "message"
**Return:**      *none*
**Description:** FIXME. Adds a message to the messages label.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.add_message string:"My Message"
================ ==================================================================

.. _dbus_method_set_center_by_string:

set_center_by_string
~~~~~~~~~~~~~~~~~~~~

================ ============================================================================================================
**Path:**        ``.navit``
**Arguments:**   string: ``coordinates``
**Return:**      *none*
**Description:** Centres the map over the specified position. See :ref:`coordinate-format-reference` for supported formats.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.set_center_by_string string:"geo: 24.0 65.0"
================ ============================================================================================================

.. _dbus_method_set_center:

set_center
~~~~~~~~~~

================ ============================================================================================================
**Path:**        ``.navit``
**Arguments:**   | int: projection, string: coordinates
                 | int: projection, int: longitude, int: latitude
**Return:**      *none*
**Description:** Centres the map over the specified position. See :ref:`coordinate-format-reference` for supported formats.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.set_center int32:1 string:"geo: 24.0 65.0"
                    org.navit_project.navit.navit.set_center int32:1 int32:0x138a4a int32:0x5d773f
================ ============================================================================================================

.. _dbus_method_set_center_screen:

set_center_screen
~~~~~~~~~~~~~~~~~

================ ========================================================================
**Path:**        ``.navit``
**Arguments:**   | integer: ``pixel_x``, integer: ``pixel_y``
**Return:**      *none*
**Description:** Centers the map to a specific position on the screen.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.set_center_screen int32:200 int32:400
================ ========================================================================

.. _dbus_method_set_layout:

set_layout
~~~~~~~~~~

================ ===============================================================
**Path:**        ``.navit``
**Arguments:**   string: ``layoutname``
**Return:**      *none*
**Description:** Changes the active map layout.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.set_layout string:"car-dark"
================ ===============================================================

.. _dbus_method_zoom:

zoom
~~~~

================ =========================================================================
**Path:**        ``.navit``
**Arguments:**   integer: ``factor`` (integer: ``pixel_x``, integer: ``pixel_y``)
**Return:**      *none*
**Description:** Zooms into the map (positive ``factor``) or out of the
                 map (negative ``factor``) by the factor specified. The factor must be
                 integers greater than 1 (zoom in) or less than -1 (zoom out).
                 Optionally, a screen position can be specified in ``pixel_x`` and
                 ``pixel_y`` and the function will zoom into that particular position.
                 In order to zoom to a specific zoom-level, use the ``set_attr`` method.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.zoom int32:2

                 .. code:: bash

                    org.navit_project.navit.navit.zoom int32:6 int32:200 int32:400
================ =========================================================================

.. _dbus_method_block:

block
~~~~~

================ ================================================
**Path:**        ``.navit``
**Arguments:**   integer: ``mode``
**Return:**      *none*
**Description:** FIXME.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.block int32:1
================ ================================================

.. _dbus_method_set_position:

set_position
~~~~~~~~~~~~

================ =====================================================================================
**Path:**        ``.navit``
**Arguments:**   | string: ``coordinates``
                 | integer: ``projection``, string: ``coordinates``
                 | integer: ``projection``, integer: ``longitude``, integer: ``latitude``
**Return:**      *none*
**Description:** Sets the current position (useful if no gps position
                 is available). See :ref:`coordinate-format-reference` for supported formats.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.set_position string:"geo: 24.0 65.0"

                 .. code:: bash

                    org.navit_project.navit.navit.set_position int32:1 string:"geo: 24.0 65.0"

                 .. code:: bash

                    org.navit_project.navit.navit.set_position int32:1 int32:0x138a4a int32:0x5d773f
================ =====================================================================================

.. _dbus_method_set_destination:

set_destination
~~~~~~~~~~~~~~~

================ =================================================================================================
**Path:**        ``.navit``
**Arguments:**   | string: ``coordinates``, string: ``"comment"``
                 | integer: ``projection``, string: ``coordinates``, string: ``"comment"``
                 | integer: ``projection``, integer: ``longitude``, integer: ``latitude``, string: ``"comment"``
**Return:**      *none*
**Description:** Sets the destination for routing. See :ref:`coordinate-format-reference` for supported formats.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.set_destination string:"geo: 24.0 65.0"

                 .. code:: bash

                    org.navit_project.navit.navit.set_destination int32:1 string:"geo: 24.0 65.0"

                 .. code:: bash

                    org.navit_project.navit.navit.set_destination int32:1 int32:0x138a4a int32:0x5d773f
================ =================================================================================================

.. _dbus_method_clear_destination:

clear_destination
~~~~~~~~~~~~~~~~~

================ ====================================================
**Path:**        ``.navit``
**Arguments:**   *none*
**Return:**      *none*
**Description:** Removes the destination and stops routing.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.clear_destination
================ ====================================================

.. _dbus_method_get_attr:

get_attr
~~~~~~~~

================ ================================================================
**Path:**        ``.navit``
**Arguments:**   string: ``attribute``
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets the specified attribute value. The attribute can
                 be anything from attr_def.h.
**Example:**     .. code:: bash

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
================ ================================================================

.. _dbus_method_set_attr:

set_attr
~~~~~~~~

================ ===========================================================================
**Path:**        ``.navit``
**Arguments:**   string: ``attribute``, variant: ``value``
**Return:**      *none*
**Description:** Sets the specified attribute value. The attribute can
                 be anything from attr_def.h. As shown in the example below, this can be
                 useful to zoom to a specific zoom level, instead of just zooming by a
                 factor as with the ``zoom`` method.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.set_attr string:"zoom" variant:int32:500
================ ===========================================================================

.. _dbus_method_export_as_gpx:

export_as_gpx
~~~~~~~~~~~~~

================ ========================================================================
**Path:**        ``.navit``
**Arguments:**   string: ``filename``
**Return:**      *none*
**Description:** Dump the current route, if any, into the file specified
                 in the argument in the GPX format.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.export_as_gpx string:"/tmp/route.gpx"
================ ========================================================================

.. _dbus_method_navit_resize:

resize
~~~~~~

================ =============================================================================
**Path:**        ``.navit``
**Arguments:**   int32: ``upperleft``, int32: ``lowerright``
**Return:**      *none*
**Description:** Resizes the navit display area.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.resize int32:0 int32:0 int32:800 int32:480
================ =============================================================================

.. _dbus_method_navit_attr_iter:

attr_iter
~~~~~~~~~

================ ================================================
**Path:**        ``.navit``
**Arguments:**   *none*
**Return:**      ``o`` (object path)
**Description:** Returns an iterator for navit attributes.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.attr_iter
================ ================================================

.. _dbus_method_navit_attr_iter_destroy:

attr_iter_destroy
~~~~~~~~~~~~~~~~~

================ =============================================================================================================
**Path:**        ``.navit``
**Arguments:**   ``o`` (attr_iter)
**Return:**      *none*
**Description:** Destroys a navit attribute iterator.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.attr_iter_destroy object_path:"/org/navit_project/navit/navit/attr_iter/0"
================ =============================================================================================================

.. _dbus_method_navit_get_attr_wi:

get_attr_wi
~~~~~~~~~~~

================ =====================================================================================================================
**Path:**        ``.navit``
**Arguments:**   string: ``attribute``, ``o`` (attr_iter)
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets a navit attribute value using an iterator.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.get_attr_wi string:"zoom" object_path:"/org/navit_project/navit/navit/attr_iter/0"
================ =====================================================================================================================

.. _dbus_method_navit_add_attr:

add_attr
~~~~~~~~

================ =======================================================================================================================
**Path:**        ``.navit``
**Arguments:**   string: ``attribute`` variant: ``value``
**Return:**      *none*
**Description:** Adds a navit attribute.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.add_attr string:"callback" variant:object_path:"/org/navit_project/navit/callback/0"
================ =======================================================================================================================

.. _dbus_method_navit_remove_attr:

remove_attr
~~~~~~~~~~~

================ ==========================================================================================================================
**Path:**        ``.navit``
**Arguments:**   string: ``attribute`` variant: ``value``
**Return:**      *none*
**Description:** Removes a navit attribute.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.remove_attr string:"callback" variant:object_path:"/org/navit_project/navit/callback/0"
================ ==========================================================================================================================

.. _dbus_method_navit_evaluate:

evaluate
~~~~~~~~

================ ============================================================
**Path:**        ``.navit``
**Arguments:**   string: ``command``
**Return:**      string: ``result``
**Description:** Evaluates a navit command string.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.evaluate string:"zoom(2)"
================ ============================================================

.. _dbus_method_navit_quit:

quit
~~~~

================ ================================================
**Path:**        ``.navit``
**Arguments:**   *none*
**Return:**      *none*
**Description:** Quits the navit application.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.quit
================ ================================================

.. _dbus_method_navit_traffic_export_gpx:

traffic_export_gpx
~~~~~~~~~~~~~~~~~~

================ ===============================================================================
**Path:**        ``.navit``
**Arguments:**   string: ``filename``
**Return:**      *none*
**Description:** Exports traffic data to a GPX file.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.traffic_export_gpx string:"/tmp/traffic.gpx"
================ ===============================================================================

.. _dbus_method_navit_traffic_inject:

traffic_inject
~~~~~~~~~~~~~~

================ ===========================================================================
**Path:**        ``.navit``
**Arguments:**   string: ``filename``
**Return:**      *none*
**Description:** Injects traffic data from a file.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.traffic_inject string:"/tmp/traffic.gpx"
================ ===========================================================================

.. _dbus_method_navit_zoom_to_route:

zoom_to_route
~~~~~~~~~~~~~

================ ================================================
**Path:**        ``.navit``
**Arguments:**   *none*
**Return:**      *none*
**Description:** Zooms the map to fit the current route.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.zoom_to_route
================ ================================================

.. _dbus_method_navit_export_as_geojson:

export_as_geojson
~~~~~~~~~~~~~~~~~

================ ================================================================================
**Path:**        ``.navit``
**Arguments:**   string: ``filename``
**Return:**      *none*
**Description:** Exports the current route to a GeoJSON file.
**Example:**     .. code:: bash

                    org.navit_project.navit.navit.export_as_geojson string:"/tmp/route.geojson"
================ ================================================================================

org.navit_project.navit.vehicleprofile
======================================


.. _methods_1:

Methods
-------

Examples are shown for each command, and when using on the command line
must be prefixed by:

``dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit``

.. _dbus_method_vehicleprofile_get_attr:

.. _get_attr_1:

get_attr
~~~~~~~~

================ ==========================================================================
**Path:**        ``.vehicleprofile``
**Arguments:**   string: ``attribute``
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets the specified attribute value for the ``vehicleprofile``.
**Example:**     .. code:: bash

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
================ ==========================================================================

.. _dbus_method_vehicleprofile_set_attr:

.. _set_attr_1:

set_attr
~~~~~~~~

================ ========================================================================================
**Path:**        ``.vehicleprofile``
**Arguments:**   string: ``attribute``, variant: ``value``
**Return:**      *none*
**Description:** Sets the specified attribute value. The example below
                 *renames* the current ``vehicleprofile``. To actually *change* the
                 ``vehicleprofile``, use the ``set_attr`` method in the ``.vehicle`` path
                 (see below).
**Example:**     .. code:: bash

                    org.navit_project.navit.vehicleprofile.set_attr string:"name" variant:string:"bike"
================ ========================================================================================

.. _dbus_method_vehicleprofile_attr_iter:

attr_iter
~~~~~~~~~

================ =====================================================
**Path:**        ``.vehicleprofile``
**Arguments:**   *none*
**Return:**      ``o`` (object path)
**Description:** Returns an iterator for vehicle profile attributes.
**Example:**     .. code:: bash

                    org.navit_project.navit.vehicleprofile.attr_iter
================ =====================================================

.. _dbus_method_vehicleprofile_attr_iter_destroy:

attr_iter_destroy
~~~~~~~~~~~~~~~~~

================ ===============================================================================================================================
**Path:**        ``.vehicleprofile``
**Arguments:**   ``o`` (attr_iter)
**Return:**      *none*
**Description:** Destroys a vehicle profile attribute iterator.
**Example:**     .. code:: bash

                    org.navit_project.navit.vehicleprofile.attr_iter_destroy object_path:"/org/navit_project/navit/vehicleprofile/attr_iter/0"
================ ===============================================================================================================================

.. _dbus_method_vehicleprofile_get_attr_wi:

get_attr_wi
~~~~~~~~~~~

================ =======================================================================================================================================
**Path:**        ``.vehicleprofile``
**Arguments:**   string: ``attribute``, ``o`` (attr_iter)
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets a vehicle profile attribute value using an iterator.
**Example:**     .. code:: bash

                    org.navit_project.navit.vehicleprofile.get_attr_wi string:"name" object_path:"/org/navit_project/navit/vehicleprofile/attr_iter/0"
================ =======================================================================================================================================

org.navit_project.navit.vehicle
===============================


.. _methods_2:

Methods
-------

Examples are shown for each command, and when using on the command line
must be prefixed by:

``dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit/default_vehicle``

.. _dbus_method_vehicle_set_attr:

.. _set_attr_2:

set_attr
~~~~~~~~

================ ========================================================================================
**Path:**        ``.vehicle``
**Arguments:**   string: ``attribute``, variant: ``value``
**Return:**      *none*
**Description:** Sets the specified attribute value. The example below
                 changes the current ``vehicleprofile`` to ``"bike"``.
**Example:**     .. code:: bash

                    org.navit_project.navit.vehicle.set_attr string:"profilename" variant:string:"bike"
================ ========================================================================================

org.navit_project.navit.route
=============================


.. _methods_3:

Methods
-------

Examples are shown for each command, and when using on the command line
must be prefixed by:

``dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit/default_route``

.. _dbus_method_route_get_attr:

.. _get_attr_2:

get_attr
~~~~~~~~

================ =======================================================================
**Path:**        ``.route``
**Arguments:**   string: ``attribute``
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets the specified attribute value. The attribute can
                 be anything from route_get_attr() in navit/route.c
**Example:**     .. code:: bash

                    org.navit_project.navit.route.get_attr string:"destination_length"

                 returns:

                 .. code:: bash

                    string "destination_length"
                    variant    int32 338111
================ =======================================================================

.. _dbus_method_route_set_attr:

set_attr
~~~~~~~~

================ =====================================================================================
**Path:**        ``.route``
**Arguments:**   string: ``attribute`` variant: ``value``
**Return:**      *none*
**Description:** Sets a route attribute value.
**Example:**     .. code:: bash

                    org.navit_project.navit.route.set_attr string:"avoid_tolls" variant:boolean:true
================ =====================================================================================

.. _dbus_method_route_add_attr:

add_attr
~~~~~~~~

================ =======================================================================================================================
**Path:**        ``.route``
**Arguments:**   string: ``attribute`` variant: ``value``
**Return:**      *none*
**Description:** Adds a route attribute.
**Example:**     .. code:: bash

                    org.navit_project.navit.route.add_attr string:"waypoint" variant:object_path:"/org/navit_project/navit/waypoint/0"
================ =======================================================================================================================

.. _dbus_method_route_remove_attr:

remove_attr
~~~~~~~~~~~

================ ==========================================================================================================================
**Path:**        ``.route``
**Arguments:**   string: ``attribute`` variant: ``value``
**Return:**      *none*
**Description:** Removes a route attribute.
**Example:**     .. code:: bash

                    org.navit_project.navit.route.remove_attr string:"waypoint" variant:object_path:"/org/navit_project/navit/waypoint/0"
================ ==========================================================================================================================

.. _dbus_method_route_destroy:

destroy
~~~~~~~

================ ================================================
**Path:**        ``.route``
**Arguments:**   *none*
**Return:**      *none*
**Description:** Destroys the route object.
**Example:**     .. code:: bash

                    org.navit_project.navit.route.destroy
================ ================================================

.. _dbus_method_route_dup:

dup
~~~

================ ================================================
**Path:**        ``.route``
**Arguments:**   *none*
**Return:**      ``o`` (object path)
**Description:** Duplicates the route object.
**Example:**     .. code:: bash

                    org.navit_project.navit.route.dup
================ ================================================

org.navit_project.navit.config
==============================


These methods are available on the root config object (path: ``/org/navit_project/navit``).

Methods
-------

.. _dbus_method_config_attr_iter:

attr_iter
~~~~~~~~~

================ ================================================
**Path:**
**Arguments:**   *none*
**Return:**      ``o`` (object path)
**Description:** Returns an iterator for config attributes.
**Example:**     .. code:: bash

                    org.navit_project.navit.config.attr_iter
================ ================================================

.. _dbus_method_config_attr_iter_destroy:

attr_iter_destroy
~~~~~~~~~~~~~~~~~

================ ========================================================================================================
**Path:**
**Arguments:**   ``o`` (attr_iter)
**Return:**      *none*
**Description:** Destroys a config attribute iterator.
**Example:**     .. code:: bash

                    org.navit_project.navit.config.attr_iter_destroy object_path:"/org/navit_project/navit/attr_iter/0"
================ ========================================================================================================

.. _dbus_method_config_get_attr:

get_attr
~~~~~~~~

================ ==========================================================
**Path:**
**Arguments:**   string: ``attrname``
**Return:**      string: ``attrname`` variant: ``value``
**Description:** Gets a config attribute value.
**Example:**     .. code:: bash

                    org.navit_project.navit.config.get_attr string:"zoom"
================ ==========================================================

.. _dbus_method_config_get_attr_wi:

get_attr_wi
~~~~~~~~~~~

================ ================================================================================================================
**Path:**
**Arguments:**   string: ``attrname``, ``o`` (attr_iter)
**Return:**      string: ``attrname`` variant: ``value``
**Description:** Gets a config attribute value using an iterator.
**Example:**     .. code:: bash

                    org.navit_project.navit.config.get_attr_wi string:"zoom" object_path:"/org/navit_project/navit/attr_iter/0"
================ ================================================================================================================

.. _dbus_method_config_callback_new:

callback_new
~~~~~~~~~~~~

================ ===================================================================
**Path:**
**Arguments:**   string: ``signalname``
**Return:**      ``o`` (object path)
**Description:** Creates a new callback for a signal.
**Example:**     .. code:: bash

                    org.navit_project.navit.config.callback_new string:"my_signal"
================ ===================================================================

.. _dbus_method_config_callback_attr_new:

callback_attr_new
~~~~~~~~~~~~~~~~~

================ ======================================================================================
**Path:**
**Arguments:**   string: ``signalname``, string: ``attribute``
**Return:**      ``o`` (object path)
**Description:** Creates a new callback for a signal with attribute filter.
**Example:**     .. code:: bash

                    org.navit_project.navit.config.callback_attr_new string:"my_signal" string:"zoom"
================ ======================================================================================

.. _dbus_method_config_search_list_new:

search_list_new
~~~~~~~~~~~~~~~

================ =========================================================================================================
**Path:**
**Arguments:**   ``o`` (mapset)
**Return:**      ``o`` (object path)
**Description:** Creates a new search list for a mapset.
**Example:**     .. code:: bash

                    org.navit_project.navit.config.search_list_new object_path:"/org/navit_project/navit/default_mapset"
================ =========================================================================================================

org.navit_project.navit.callback
================================

Methods
-------

.. _dbus_method_callback_destroy:

destroy
~~~~~~~

================ ================================================
**Path:**        ``.callback``
**Arguments:**   *none*
**Return:**      *none*
**Description:** Destroys a callback object.
**Example:**     .. code:: bash

                    org.navit_project.navit.callback.destroy
================ ================================================

org.navit_project.navit.graphics
================================

Methods
-------

.. _dbus_method_graphics_get_data:

get_data
~~~~~~~~

================ ==================================================================
**Path:**        ``.graphics``
**Arguments:**   string: ``type``
**Return:**      ``ay`` (byte array)
**Description:** Gets graphics data of the specified type.
**Example:**     .. code:: bash

                    org.navit_project.navit.graphics.get_data string:"screenshot"
================ ==================================================================

.. _dbus_method_graphics_set_attr:

set_attr
~~~~~~~~

================ ===============================================================================
**Path:**        ``.graphics``
**Arguments:**   string: ``attribute`` variant: ``value``
**Return:**      *none*
**Description:** Sets a graphics attribute.
**Example:**     .. code:: bash

                    org.navit_project.navit.graphics.set_attr string:"width" variant:int32:800
================ ===============================================================================

org.navit_project.navit.gui
===========================

Methods
-------

.. _dbus_method_gui_get_attr:

get_attr
~~~~~~~~

================ =========================================================
**Path:**        ``.gui``
**Arguments:**   string: ``attribute``
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets a GUI attribute value.
**Example:**     .. code:: bash

                    org.navit_project.navit.gui.get_attr string:"active"
================ =========================================================

.. _dbus_method_gui_command_parameter:

command_parameter
~~~~~~~~~~~~~~~~~

================ ========================================================================
**Path:**        ``.gui``
**Arguments:**   string: ``command``, dict: ``parameter``
**Return:**      dict: ``return``
**Description:** Executes a GUI command with parameters.
**Example:**     .. code:: bash

                    org.navit_project.navit.gui.command_parameter string:"menu" dict:{}
================ ========================================================================

.. _dbus_method_gui_command:

command
~~~~~~~

================ ======================================================
**Path:**        ``.gui``
**Arguments:**   string: ``command``
**Return:**      dict: ``return``
**Description:** Executes a GUI command.
**Example:**     .. code:: bash

                    org.navit_project.navit.gui.command string:"menu"
================ ======================================================

org.navit_project.navit.map
===========================

Methods
-------

.. _dbus_method_map_get_attr:

get_attr
~~~~~~~~

================ =======================================================
**Path:**        ``.map``
**Arguments:**   string: ``attribute``
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets a map attribute value.
**Example:**     .. code:: bash

                    org.navit_project.navit.map.get_attr string:"name"
================ =======================================================

.. _dbus_method_map_set_attr:

set_attr
~~~~~~~~

================ ===============================================================================
**Path:**        ``.map``
**Arguments:**   string: ``attribute`` variant: ``value``
**Return:**      *none*
**Description:** Sets a map attribute value.
**Example:**     .. code:: bash

                    org.navit_project.navit.map.set_attr string:"enabled" variant:boolean:true
================ ===============================================================================

.. _dbus_method_map_dump:

dump
~~~~

================ ===========================================================
**Path:**        ``.map``
**Arguments:**   string: ``file``
**Return:**      *none*
**Description:** Dumps the map data to a file.
**Example:**     .. code:: bash

                    org.navit_project.navit.map.dump string:"/tmp/map.bin"
================ ===========================================================

org.navit_project.navit.mapset
==============================

Methods
-------

.. _dbus_method_mapset_attr_iter:

attr_iter
~~~~~~~~~

================ ================================================
**Path:**        ``.mapset``
**Arguments:**   *none*
**Return:**      ``o`` (object path)
**Description:** Returns an iterator for mapset attributes.
**Example:**     .. code:: bash

                    org.navit_project.navit.mapset.attr_iter
================ ================================================

.. _dbus_method_mapset_attr_iter_destroy:

attr_iter_destroy
~~~~~~~~~~~~~~~~~

================ ===============================================================================================================
**Path:**        ``.mapset``
**Arguments:**   ``o`` (attr_iter)
**Return:**      *none*
**Description:** Destroys a mapset attribute iterator.
**Example:**     .. code:: bash

                    org.navit_project.navit.mapset.attr_iter_destroy object_path:"/org/navit_project/navit/mapset/attr_iter/0"
================ ===============================================================================================================

.. _dbus_method_mapset_get_attr:

get_attr
~~~~~~~~

================ ==========================================================
**Path:**        ``.mapset``
**Arguments:**   string: ``attribute``
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets a mapset attribute value.
**Example:**     .. code:: bash

                    org.navit_project.navit.mapset.get_attr string:"name"
================ ==========================================================

.. _dbus_method_mapset_get_attr_wi:

get_attr_wi
~~~~~~~~~~~

================ =======================================================================================================================
**Path:**        ``.mapset``
**Arguments:**   string: ``attribute``, ``o`` (attr_iter)
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets a mapset attribute value using an iterator.
**Example:**     .. code:: bash

                    org.navit_project.navit.mapset.get_attr_wi string:"name" object_path:"/org/navit_project/navit/mapset/attr_iter/0"
================ =======================================================================================================================

org.navit_project.navit.navigation
==================================

Methods
-------

.. _dbus_method_navigation_get_attr:

get_attr
~~~~~~~~

================ ============================================================================
**Path:**        ``.navigation``
**Arguments:**   string: ``attribute``
**Return:**      *none* (see description)
**Description:** Gets a navigation attribute value. Return type varies by attribute.
**Example:**     .. code:: bash

                    org.navit_project.navit.navigation.get_attr string:"destination_length"
================ ============================================================================

org.navit_project.navit.search_list
===================================

Methods
-------

.. _dbus_method_search_list_destroy:

destroy
~~~~~~~

================ ================================================
**Path:**        ``.search_list``
**Arguments:**   *none*
**Return:**      *none*
**Description:** Destroys a search list object.
**Example:**     .. code:: bash

                    org.navit_project.navit.search_list.destroy
================ ================================================

.. _dbus_method_search_list_get_result:

get_result
~~~~~~~~~~

================ ===================================================
**Path:**        ``.search_list``
**Arguments:**   *none*
**Return:**      ``i(iii)a{sa{sv}}`` (id, coord, dict)
**Description:** Gets the next search result.
**Example:**     .. code:: bash

                    org.navit_project.navit.search_list.get_result
================ ===================================================

.. _dbus_method_search_list_search:

search
~~~~~~

================ =============================================================================================
**Path:**        ``.search_list``
**Arguments:**   string: ``attribute`` variant: ``value`` int32: ``partial``
**Return:**      *none*
**Description:** Performs a search with the given attribute and value.
**Example:**     .. code:: bash

                    org.navit_project.navit.search_list.search string:"name" variant:string:"Berlin" int32:1
================ =============================================================================================

.. _dbus_method_search_list_select:

select
~~~~~~

================ =============================================================================
**Path:**        ``.search_list``
**Arguments:**   string: ``attribute_type`` int32: ``id`` int32: ``mode``
**Return:**      *none*
**Description:** Selects a search result.
**Example:**     .. code:: bash

                    org.navit_project.navit.search_list.select string:"name" int32:0 int32:1
================ =============================================================================

org.navit_project.navit.tracking
================================

Methods
-------

.. _dbus_method_tracking_get_attr:

get_attr
~~~~~~~~

================ ===================================================================
**Path:**        ``.tracking``
**Arguments:**   string: ``attribute``
**Return:**      *none* (see description)
**Description:** Gets a tracking attribute value. Return type varies by attribute.
**Example:**     .. code:: bash

                    org.navit_project.navit.tracking.get_attr string:"position"
================ ===================================================================

org.navit_project.navit.layout
==============================

Methods
-------

.. _dbus_method_layout_get_attr:

get_attr
~~~~~~~~

================ ==========================================================
**Path:**        ``.layout``
**Arguments:**   string: ``attribute``
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets a layout attribute value.
**Example:**     .. code:: bash

                    org.navit_project.navit.layout.get_attr string:"name"
================ ==========================================================

org.navit_project.navit.osd
===========================

Methods
-------

.. _dbus_method_osd_get_attr:

get_attr
~~~~~~~~

================ ==========================================================
**Path:**        ``.osd``
**Arguments:**   string: ``attribute``
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets an OSD attribute value.
**Example:**     .. code:: bash

                    org.navit_project.navit.osd.get_attr string:"enabled"
================ ==========================================================

.. _dbus_method_osd_set_attr:

set_attr
~~~~~~~~

================ ===============================================================================
**Path:**        ``.osd``
**Arguments:**   string: ``attribute`` variant: ``value``
**Return:**      *none*
**Description:** Sets an OSD attribute value.
**Example:**     .. code:: bash

                    org.navit_project.navit.osd.set_attr string:"enabled" variant:boolean:true
================ ===============================================================================

org.navit_project.navit.roadprofile
===================================

Methods
-------

.. _dbus_method_roadprofile_get_attr:

get_attr
~~~~~~~~

================ ================================================================
**Path:**        ``.roadprofile``
**Arguments:**   string: ``attribute``
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets a road profile attribute value.
**Example:**     .. code:: bash

                    org.navit_project.navit.roadprofile.get_attr string:"speed"
================ ================================================================

.. _dbus_method_roadprofile_get_attr_wi:

get_attr_wi
~~~~~~~~~~~

================ ==================================================================================================================================
**Path:**        ``.roadprofile``
**Arguments:**   string: ``attribute``, ``o`` (attr_iter)
**Return:**      string: ``attribute`` variant: ``value``
**Description:** Gets a road profile attribute value using an iterator.
**Example:**     .. code:: bash

                    org.navit_project.navit.roadprofile.get_attr_wi string:"speed" object_path:"/org/navit_project/navit/roadprofile/attr_iter/0"
================ ==================================================================================================================================

.. _dbus_method_roadprofile_set_attr:

set_attr
~~~~~~~~

================ =================================================================================
**Path:**        ``.roadprofile``
**Arguments:**   string: ``attribute`` variant: ``value``
**Return:**      *none*
**Description:** Sets a road profile attribute value.
**Example:**     .. code:: bash

                    org.navit_project.navit.roadprofile.set_attr string:"speed" variant:int32:50
================ =================================================================================

.. _dbus_method_roadprofile_attr_iter:

attr_iter
~~~~~~~~~

================ ==================================================
**Path:**        ``.roadprofile``
**Arguments:**   *none*
**Return:**      ``o`` (object path)
**Description:** Returns an iterator for road profile attributes.
**Example:**     .. code:: bash

                    org.navit_project.navit.roadprofile.attr_iter
================ ==================================================

.. _dbus_method_roadprofile_attr_iter_destroy:

attr_iter_destroy
~~~~~~~~~~~~~~~~~

================ =========================================================================================================================
**Path:**        ``.roadprofile``
**Arguments:**   ``o`` (attr_iter)
**Return:**      *none*
**Description:** Destroys a road profile attribute iterator.
**Example:**     .. code:: bash

                    org.navit_project.navit.roadprofile.attr_iter_destroy object_path:"/org/navit_project/navit/roadprofile/attr_iter/0"
================ =========================================================================================================================

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


`category: navit dbus <category:_navit_dbus>`__ `category:
Development <category:_Development>`__
