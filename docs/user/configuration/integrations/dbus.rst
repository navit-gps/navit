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

The way navit use dbus is not really the usual way. If you want to
control your navit instance (set_destination, zoom,…), you have first to
do the following (examples are in python) : 1) get the navit general
object

`` object = bus.get_object("org.navit_project.navit","/org/navit_project/navit")``

2) get the interface of this object :

`` iface = dbus.Interface(object,dbus_interface="org.navit_project.navit")``

3) get an iterator through navit instances :

`` iter=iface.attr_iter()``

4) get the object path of the navit instance in your iterator :

`` path = object.get_attr_wi("navit",iter)``

5) get the navit instance you are interested in :

`` navit=bus.get_object('org.navit_project.navit', path[1])``

6) destroy the iter :

`` iface.attr_iter_destroy(iter)``

The following methods have to be applied on the navit object.

Methods
-------

Examples are shown for each command, and when using on the command line
must be prefixed by:

``dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit``

When a coordinate is specified, the coordinate can be formatted like:

``[Proj:]-0xX [-]0xX``

where ``Proj`` can be ``mg`` or ``garmin``. Default: ``mg``. For
example:

``mg: 0x138a4a 0x5d773f``

Or

| ``[Proj:][D][D]Dmm.ss[S][S] N/S [D][D]DMM.ss[S][S]... E/W``
| ``[Proj:][-][D]D.d[d]... [-][D][D]D.d[d]``

where ``Proj`` must be ``geo``. For example:

``geo: 24.0 65.0``

draw
~~~~

================ ==============================================
**Path:**        .navit
**Arguments:**   *none*
**Return:**      *none*
**Description:** Forces a redraw of the current view of the map
**Example:**     org.navit_project.navit.navit.draw
================ ==============================================

add_message
~~~~~~~~~~~

================ =============================================================
**Path:**        .navit
**Arguments:**   string: "message"
**Return:**      *none*
**Description:** **FIXME** Adds a message to the messages label
**Example:**     org.navit_project.navit.navit.add_message string:"My Message"
================ =============================================================

set_center_by_string
~~~~~~~~~~~~~~~~~~~~

================ ==========================================================================
**Path:**        .navit
**Arguments:**   string: ``coordinates``
**Return:**      *none*
**Description:** Centres the map over the specified position.
**Example:**     org.navit_project.navit.navit.set_center_by_string string:"geo: 24.0 65.0"
================ ==========================================================================

set_center
~~~~~~~~~~

================ ==============================================================================
**Path:**        .navit
**Arguments:**   integer: ``projection`` string: ``coordinates``
**Return:**      *none*
**Description:** Centres the map over the specified position.
**Example:**     org.navit_project.navit.navit.set_center int32:1 string:"geo: 24.0 65.0"

                 org.navit_project.navit.navit.set_center int32:1 int32:0x138a4a int32:0x5d773f
================ ==============================================================================

set_center_screen
~~~~~~~~~~~~~~~~~

+------------------+--------------------------------------------------+
| **Path:**        | .navit                                           |
+------------------+--------------------------------------------------+
| **Arguments:**   | integer: ``pixel_x`` integer: ``pixel_y``        |
+------------------+--------------------------------------------------+
| **Return:**      | *none*                                           |
+------------------+--------------------------------------------------+
| **Description:** | Centers the map to a specific position on the    |
|                  | screen.                                          |
+------------------+--------------------------------------------------+
| **Example:**     | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.n                           |
|                  | avit.navit.set_center_screen int32:200 int32:400 |
+------------------+--------------------------------------------------+
|                  |                                                  |
+------------------+--------------------------------------------------+

set_layout
~~~~~~~~~~

+------------------+--------------------------------------------------+
| **Path:**        | .navit                                           |
+------------------+--------------------------------------------------+
| **Arguments:**   | string: ``layoutname``                           |
+------------------+--------------------------------------------------+
| **Return:**      | *none*                                           |
+------------------+--------------------------------------------------+
| **Description:** | Changes the active map layout.                   |
+------------------+--------------------------------------------------+
| **Example:**     | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_                                    |
|                  | project.navit.navit.set_layout string:"car-dark" |
+------------------+--------------------------------------------------+
|                  |                                                  |
+------------------+--------------------------------------------------+

zoom
~~~~

+------------------+--------------------------------------------------+
| **Path:**        | .navit                                           |
+------------------+--------------------------------------------------+
| **Arguments:**   | integer: ``factor`` (integer: ``pixel_x``        |
|                  | integer: ``pixel_y``)                            |
+------------------+--------------------------------------------------+
| **Return:**      | *none*                                           |
+------------------+--------------------------------------------------+
| **Description:** | Zooms into the map (positive ``factor``) or out  |
|                  | of the map (negative ``factor``) by the factor   |
|                  | specified. The factor must be integers greater   |
|                  | than 1 (zoom in) or less than -1 (zoom out).     |
|                  | Optionally, a screen position can be specified   |
|                  | in ``pixel_x`` and ``pixel_y`` and the function  |
|                  | will zoom into that particular position. In      |
|                  | order to zoom to a specific zoom-level, use the  |
|                  | ```set_attr`` <#set_attr>`__ method.             |
+------------------+--------------------------------------------------+
| **Example:**     | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.navit.navit.zoom int32:2    |
|                  |                                                  |
|                  | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_proj                                |
|                  | ect.navit.navit.zoom int32:6 int32:200 int32:400 |
+------------------+--------------------------------------------------+
|                  |                                                  |
+------------------+--------------------------------------------------+

block
~~~~~

================ ==============================================
**Path:**        .navit
**Arguments:**   integer: ``mode``
**Return:**      *none*
**Description:** **FIXME**
**Example:**     .. code:: bash
                 
                    org.navit_project.navit.navit.block int32:1
\                
================ ==============================================

set_position
~~~~~~~~~~~~

+------------------+--------------------------------------------------+
| **Path:**        | .navit                                           |
+------------------+--------------------------------------------------+
| **Arguments:**   | | string: ``coordinates``                        |
|                  | | integer: ``projection`` string:                |
|                  |   ``coordinates``                                |
|                  | | integer: ``projection`` integer: ``longitude`` |
|                  |   integer: ``latitude``                          |
+------------------+--------------------------------------------------+
| **Return:**      | *none*                                           |
+------------------+--------------------------------------------------+
| **Description:** | Sets the current position (useful if no gps      |
|                  | position is available).                          |
+------------------+--------------------------------------------------+
| **Example:**     | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.                            |
|                  | navit.navit.set_position string:"geo: 24.0 65.0" |
|                  |                                                  |
|                  | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.navit.na                    |
|                  | vit.set_position int32:1 string:"geo: 24.0 65.0" |
|                  |                                                  |
|                  | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.navit.navit.se              |
|                  | t_position int32:1 int32:0x138a4a int32:0x5d773f |
+------------------+--------------------------------------------------+
|                  |                                                  |
+------------------+--------------------------------------------------+

set_destination
~~~~~~~~~~~~~~~

+------------------+--------------------------------------------------+
| **Path:**        | .navit                                           |
+------------------+--------------------------------------------------+
| **Arguments:**   | | string: ``coordinates`` string:"comment"       |
|                  | | integer: ``projection`` string:                |
|                  |   ``coordinates`` string:"comment"               |
|                  | | integer: ``projection`` integer: ``longitude`` |
|                  |   integer: ``latitude`` string:"comment"         |
+------------------+--------------------------------------------------+
| **Return:**      | *none*                                           |
+------------------+--------------------------------------------------+
| **Description:** | Sets the destination for routing.                |
+------------------+--------------------------------------------------+
| **Example:**     | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.nav                         |
|                  | it.navit.set_destination string:"geo: 24.0 65.0" |
|                  |                                                  |
|                  | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.navit.navit                 |
|                  | .set_destination int32:1 string:"geo: 24.0 65.0" |
|                  |                                                  |
|                  | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.navit.navit.set_d           |
|                  | estination int32:1 int32:0x138a4a int32:0x5d773f |
+------------------+--------------------------------------------------+
|                  |                                                  |
+------------------+--------------------------------------------------+

clear_destination
~~~~~~~~~~~~~~~~~

================ ==================================================
**Path:**        .navit
**Arguments:**   *none*
**Return:**      *none*
**Description:** Removes the destination and stops routing.
**Example:**     .. code:: bash
                 
                    org.navit_project.navit.navit.clear_destination
\                
================ ==================================================

get_attr
~~~~~~~~

+------------------+--------------------------------------------------+
| **Path:**        | .navit                                           |
+------------------+--------------------------------------------------+
| **Arguments:**   | string:``attribute``                             |
+------------------+--------------------------------------------------+
| **Return:**      | string:``attribute`` variant:``value``           |
+------------------+--------------------------------------------------+
| **Description:** | Gets the specified attribute value. The          |
|                  | attribute can be anything from attr_def.h.       |
+------------------+--------------------------------------------------+
| **Example:**     | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.                                          |
|                  | navit_project.navit.navit.get_attr string:"zoom" |
|                  |                                                  |
|                  | returns:                                         |
|                  |                                                  |
|                  | .. code:: bash                                   |
|                  |                                                  |
|                  |       string "zoom"                              |
|                  |       variant       int32 10                     |
|                  |                                                  |
|                  | .. raw:: html                                    |
|                  |                                                  |
|                  |    <hr>                                          |
|                  |                                                  |
|                  | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_p                                   |
|                  | roject.navit.navit.get_attr string:"orientation" |
|                  |                                                  |
|                  | returns:                                         |
|                  |                                                  |
|                  | .. code:: bash                                   |
|                  |                                                  |
|                  |       string "orientation"                       |
|                  |       variant       int32 1                      |
+------------------+--------------------------------------------------+
|                  |                                                  |
+------------------+--------------------------------------------------+

set_attr
~~~~~~~~

+------------------+--------------------------------------------------+
| **Path:**        | .navit                                           |
+------------------+--------------------------------------------------+
| **Arguments:**   | string:``attribute`` variant:``value``           |
+------------------+--------------------------------------------------+
| **Return:**      | *none*                                           |
+------------------+--------------------------------------------------+
| **Description:** | Sets the specified attribute value. The          |
|                  | attribute can be anything from attr_def.h. As    |
|                  | shown in the example below, this can be useful   |
|                  | to zoom to a specific zoom level, insted of just |
|                  | zooming by a factor as with the                  |
|                  | ```zoom`` <#zoom>`__ method.                     |
+------------------+--------------------------------------------------+
| **Example:**     | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.navi                        |
|                  | t.navit.set_attr string:"zoom" variant:int32:500 |
+------------------+--------------------------------------------------+
|                  |                                                  |
+------------------+--------------------------------------------------+

export_as_gpx
~~~~~~~~~~~~~

+------------------+--------------------------------------------------+
| **Path:**        | .navit                                           |
+------------------+--------------------------------------------------+
| **Arguments:**   | string:``filename``                              |
+------------------+--------------------------------------------------+
| **Return:**      | *none*                                           |
+------------------+--------------------------------------------------+
| **Description:** | Dump the current route, if any, into the file    |
|                  | specified in the argument in the GPX format.     |
+------------------+--------------------------------------------------+
| **Example:**     | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.n                           |
|                  | avit.navit.export_as_gpx string:"/tmp/route.gpx" |
+------------------+--------------------------------------------------+
|                  |                                                  |
+------------------+--------------------------------------------------+

org.navit_project.navit.vehicleprofile
======================================

.. _methods_1:

Methods
-------

Examples are shown for each command, and when using on the command line
must be prefixed by:

``dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit/default_vehicleprofile``

.. _get_attr_1:

get_attr
~~~~~~~~

+------------------+--------------------------------------------------+
| **Path:**        | .vehicleprofile                                  |
+------------------+--------------------------------------------------+
| **Arguments:**   | string:``attribute``                             |
+------------------+--------------------------------------------------+
| **Return:**      | string:``attribute`` variant:``value``           |
+------------------+--------------------------------------------------+
| **Description:** | Gets the specified attribute value for the       |
|                  | ``vehicleprofile``.                              |
+------------------+--------------------------------------------------+
| **Example:**     | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_pro                                 |
|                  | ject.navit.vehicleprofile.get_attr string:"name" |
|                  |                                                  |
|                  | returns:                                         |
|                  |                                                  |
|                  | .. code:: bash                                   |
|                  |                                                  |
|                  |       string "name"                              |
|                  |       variant       string "car"                 |
|                  |                                                  |
|                  | .. raw:: html                                    |
|                  |                                                  |
|                  |    <hr>                                          |
|                  |                                                  |
|                  | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.nav                         |
|                  | it.vehicleprofile.get_attr string:"static_speed" |
|                  |                                                  |
|                  | returns:                                         |
|                  |                                                  |
|                  | .. code:: bash                                   |
|                  |                                                  |
|                  |       string "static_speed"                      |
|                  |       variant       int32 5                      |
+------------------+--------------------------------------------------+

.. _set_attr_1:

set_attr
~~~~~~~~

+------------------+--------------------------------------------------+
| **Path:**        | .vehicleprofile                                  |
+------------------+--------------------------------------------------+
| **Arguments:**   | string:``attribute`` variant:``value``           |
+------------------+--------------------------------------------------+
| **Return:**      | *none*                                           |
+------------------+--------------------------------------------------+
| **Description:** | Sets the specified attribute value. The example  |
|                  | below *renames* the current ``vehicleprofile``.  |
|                  | To actually *change* the ``vehicleprofile``, use |
|                  | the ``set_attr`` method in the ``.vehicle`` path |
|                  | (see below).                                     |
+------------------+--------------------------------------------------+
| **Example:**     | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.navit.vehicleprof           |
|                  | ile.set_attr string:"name" variant:string:"bike" |
+------------------+--------------------------------------------------+
|                  |                                                  |
+------------------+--------------------------------------------------+

org.navit_project.navit.vehicle
===============================

.. _methods_2:

Methods
-------

Examples are shown for each command, and when using on the command line
must be prefixed by:

``dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit/default_vehicle``

.. _set_attr_2:

set_attr
~~~~~~~~

+------------------+--------------------------------------------------+
| **Path:**        | .vehicle                                         |
+------------------+--------------------------------------------------+
| **Arguments:**   | string:``attribute`` variant:``value``           |
+------------------+--------------------------------------------------+
| **Return:**      | *none*                                           |
+------------------+--------------------------------------------------+
| **Description:** | Sets the specified attribute value. The example  |
|                  | below changes the current ``vehicleprofile`` to  |
|                  | ``"bike"`` .                                     |
+------------------+--------------------------------------------------+
| **Example:**     | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.navit.vehicle.set           |
|                  | _attr string:"profilename" variant:string:"bike" |
+------------------+--------------------------------------------------+
|                  |                                                  |
+------------------+--------------------------------------------------+

org.navit_project.navit.route
=============================

.. _methods_3:

Methods
-------

Examples are shown for each command, and when using on the command line
must be prefixed by:

``dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit/default_route``

.. _get_attr_2:

get_attr
~~~~~~~~

+------------------+--------------------------------------------------+
| **Path:**        | .route                                           |
+------------------+--------------------------------------------------+
| **Arguments:**   | string:``attribute``                             |
+------------------+--------------------------------------------------+
| **Return:**      | string:``attribute`` variant:``value``           |
+------------------+--------------------------------------------------+
| **Description:** | Gets the specified attribute value. The          |
|                  | attribute can be anything from route_get_attr()  |
|                  | in navit/route.c                                 |
+------------------+--------------------------------------------------+
| **Example:**     | .. code:: bash                                   |
|                  |                                                  |
|                  |    org.navit_project.                            |
|                  | navit.route.get_attr string:"destination_length" |
|                  |                                                  |
|                  | returns:                                         |
|                  |                                                  |
|                  | .. code:: bash                                   |
|                  |                                                  |
|                  |      string "destination_length"                 |
|                  |      variant       int32 338111                  |
+------------------+--------------------------------------------------+
|                  |                                                  |
+------------------+--------------------------------------------------+

Signals
=======

.. _add_bookmark_signal:

add bookmark signal
-------------------

Setup the callback:

| `` dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit org.navit_project.navit.callback_attr_new string:my_signal string:bookmark_map ``
| `` object path "/org/navit_project/navit/callback/0"``
| `` dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit org.navit_project.navit.navit.add_attr string:callback ``
| `` variant:objpath:/org/navit_project/navit/callback/0``

Replace "my_signal" with a signal name of your choice.

Now add a bookmark and you should see this in dbus-monitor --session:

`` signal sender=:1.927 -> dest=(null destination) path=/org/navit_project/navit; interface=org.navit_project.navit; member=my_signal``

.. _undocumented_methods:

Undocumented methods
====================

The following code comes from /binding/dbus/binding_dbus.c

.. code:: bash

       {"",        "attr_iter",           "",        "",                                        "o",  "attr_iter",  request_config_attr_iter},
       {"",        "attr_iter_destroy",   "o",       "attr_iter",                               "",   "",      request_config_attr_iter_destroy},
       {"",        "get_attr",            "s",       "attrname",                                "sv", "attrname,value",request_config_get_attr},
       {"",        "get_attr_wi",         "so",      "attrname,attr_iter",                      "sv", "attrname,value",request_config_get_attr},
       {"",        "callback_new",    "s",       "signalname",                              "o",  "callback",request_callback_new},
       {"",        "callback_attr_new",   "ss",       "signalname,attribute",                   "o",  "callback",request_callback_new},
       {"",        "search_list_new",     "o",       "mapset",                                  "o",  "search",request_search_list_new},
       {".callback","destroy",            "",        "",                                        "",   "",      request_callback_destroy},
       {".graphics","get_data",       "s",       "type",                    "ay",  "data", request_graphics_get_data},
       {".graphics","set_attr",           "sv",      "attribute,value",                         "",   "",      request_graphics_set_attr},
       {".gui",     "get_attr",           "s",       "attribute",                               "sv",  "attrname,value", request_gui_get_attr},
       {".gui",     "command_parameter",  "sa{sa{sv}}","command,parameter",                     "a{sa{sv}}",  "return", request_gui_command},
       {".gui",     "command",            "s",       "command",                                 "a{sa{sv}}",  "return", request_gui_command},

       {".navit",  "resize",              "ii",      "upperleft,lowerright",                    "",   "",      request_navit_resize},
       {".navit",  "attr_iter",           "",        "",                                        "o",  "attr_iter",  request_navit_attr_iter},
       {".navit",  "attr_iter_destroy",   "o",       "attr_iter",                               "",   "",      request_navit_attr_iter_destroy},
       {".navit",  "get_attr_wi",         "so",      "attribute,attr_iter",                     "sv",  "attrname,value", request_navit_get_attr},
       {".navit",  "add_attr",            "sv",      "attribute,value",                         "",   "",      request_navit_add_attr},
       {".navit",  "remove_attr",         "sv",      "attribute,value",                         "",   "",      request_navit_remove_attr},
       {".navit",  "evaluate",        "s",       "command",                 "s",  "",      request_navit_evaluate},

       {".map",    "get_attr",            "s",       "attribute",                               "sv",  "attrname,value", request_map_get_attr},
       {".map",    "set_attr",            "sv",      "attribute,value",                         "",   "",      request_map_set_attr},
       {".mapset", "attr_iter",           "",        "",                                        "o",  "attr_iter",  request_mapset_attr_iter},
       {".mapset", "attr_iter_destroy",   "o",       "attr_iter",                               "",   "",      request_mapset_attr_iter_destroy},
       {".mapset", "get_attr",            "s",       "attribute",                               "sv",  "attrname,value", request_mapset_get_attr},
       {".mapset", "get_attr_wi",         "so",      "attribute,attr_iter",                     "sv",  "attrname,value", request_mapset_get_attr},
       {".navigation","get_attr",         "s",       "attribute",                               "",   "",      request_navigation_get_attr},
       {".route",    "set_attr",          "sv",      "attribute,value",                         "",    "",  request_route_set_attr},
       {".route",    "add_attr",          "sv",      "attribute,value",                         "",    "",  request_route_add_attr},
       {".route",    "remove_attr",       "sv",      "attribute,value",                         "",    "",  request_route_remove_attr},
       {".search_list","destroy",         "",        "",                                        "",   "",      request_search_list_destroy},
       {".search_list","get_result",      "",        "",                                        "i(iii)a{sa{sv}}",   "id,coord,dict",      request_search_list_get_result},
       {".search_list","search",          "svi",     "attribute,value,partial",                 "",   "",      request_search_list_search},
       {".search_list","select",          "sii",     "attribute_type,id,mode",                  "",   "",      request_search_list_select},
       {".tracking","get_attr",           "s",       "attribute",                               "",   "",      request_tracking_get_attr},

`category: navit dbus <category:_navit_dbus>`__ `category:
Development <category:_Development>`__
