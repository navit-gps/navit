Driver Break DBus API: eco_mode_fuel_enabled
=============================================

The Driver Break plugin exposes the boolean attribute **eco_mode_fuel_enabled** over DBus so external components (e.g. dashboards, eco-routing UIs) can detect whether fuel or eco-related features are active.

When the attribute is **true**, at least one of the following applies:

- An ECU backend is available and running (OBD-II, J1939, or MegaSquirt).
- Adaptive fuel learning is enabled in the plugin configuration.

When **false**, no live ECU data and no adaptive learning are in use; energy-based routing may still be available without fuel data.

DBus service and interface
--------------------------

- **Service:** ``org.navit_project.navit``
- **Object path:** The Navit root object is ``/org/navit_project/navit``. The **navit** object path is obtained by calling ``get_attr_wi("navit", attr_iter)`` on the root; the second element of the return value is the object path of the navit instance.
- **Interface (for get_attr):** ``org.navit_project.navit.navit``

Reading eco_mode_fuel_enabled from the Navit object
--------------------------------------------------

The attribute is available directly on the **navit** object. You do not need to resolve the Driver Break OSD.

**Method:** ``get_attr``
**In parameter:** ``s`` (string) – attribute name: ``"eco_mode_fuel_enabled"``
**Out:** ``sv`` – ``(attrname, value)`` where ``value`` is a DBus boolean (true/false).

Example with dbus-send (replace ``/org/navit_project/navit/navit/0`` with the actual navit object path if different):

.. code-block:: bash

   dbus-send --session --print-reply \
     --dest=org.navit_project.navit \
     /org/navit_project/navit/navit/0 \
     org.navit_project.navit.navit.get_attr \
     string:"eco_mode_fuel_enabled"

Example in Python (using dbus)

.. code-block:: python

   import dbus
   bus = dbus.SessionBus()
   conn = bus.get_object('org.navit_project.navit', '/org/navit_project/navit')
   iface = dbus.Interface(conn, dbus_interface='org.navit_project.navit')
   _iter = iface.attr_iter()
   navit_path = conn.get_attr_wi('navit', _iter)[1]
   iface.attr_iter_destroy(_iter)
   navit = bus.get_object('org.navit_project.navit', navit_path)
   navit_iface = dbus.Interface(navit, dbus_interface='org.navit_project.navit.navit')
   attrname, value = navit_iface.get_attr('eco_mode_fuel_enabled')
   # value is a dbus.Boolean
   eco_fuel_enabled = bool(value)

Reading from the Driver Break OSD (optional)
--------------------------------------------

You can also read ``eco_mode_fuel_enabled`` from the Driver Break OSD object. Obtain OSD object paths by calling ``get_attr_wi("osd", attr_iter)`` on the navit object; then call ``get_attr("eco_mode_fuel_enabled")`` on the OSD interface (``org.navit_project.navit.osd``). Only the Driver Break OSD implements this attribute; other OSDs will return "no data available" for it.

Internal API (C code)
--------------------

- **Navit:** ``navit_get_attr(navit, attr_eco_mode_fuel_enabled, &attr, NULL)`` returns 1 and sets ``attr.u.num`` to 1 or 0 if any OSD (typically the Driver Break OSD) reports the attribute.
- **OSD:** The Driver Break OSD implements ``get_attr`` for ``attr_eco_mode_fuel_enabled`` in its ``osd_methods``; ``osd_get_attr(osd, attr_eco_mode_fuel_enabled, &attr, NULL)`` returns 1 and sets ``attr.u.num`` accordingly.
