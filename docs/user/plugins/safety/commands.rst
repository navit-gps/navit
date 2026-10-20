On-screen elements and commands
===============================

This page lists what the Navit Safety plugin exposes on screen and through commands.

Enabling the OSD
----------------

Load the shared object and add an OSD instance in your Navit configuration:

.. code-block:: xml

   <plugin path="$NAVIT_LIBDIR/plugin/${NAVIT_LIBPREFIX}libosd_navit_safety.so" active="yes"/>
   <osd type="navit_safety" enabled="yes" label="mytrip" update_period="30" data="range=600"/>

Optional OSD attributes:

- ``label`` — trip identifier used by the confirmation cache (default ``default``).
- ``update_period`` — minimum seconds between corridor rescans (default 30).
- ``data="range=NNN"`` — full tank or water range in km before buffers (default 600).
- ``data="wbgt=NN.N"`` — wet-bulb globe temperature in degrees Celsius for the foot-travel heat model (default 28).

On-screen display
-----------------

The OSD draws a status panel showing:

- remote mode (Off / Auto / Always) and whether remote planning is active;
- the selected buffer and usable range;
- the worst resupply gap along the route, with **WARN** when it exceeds usable range;
- a desert consumption warning when arid remote planning applies;
- water requirement and heat-stress level when foot-travel mode is enabled.

The panel refreshes when the route changes (``attr_destination``) and on position updates, throttled by ``update_period``. Gap warnings are drawn in red.

Registered commands
-------------------

The plugin registers these Navit commands (usable from OSD buttons, menus, or the command line):

``navit_safety_toggle_remote``
    Cycle remote mode: Off, then Auto, then Always, then Off.

``navit_safety_confirm_poi``
    Mark the nearest unconfirmed resupply POI as operational, store the confirmation in the SQLite cache, and recalculate the plan.

``navit_safety_deny_poi``
    Mark the nearest actionable POI as denied for the current trip and recalculate the plan as if that stop does not exist.

``navit_safety_show_plan``
    Speak the current plan summary through Navit's speech output.

``navit_safety_toggle_foot``
    Toggle foot-travel mode (water planning and heat-stress assessment).

Example OSD button bindings:

.. code-block:: xml

   <osd type="button" command="navit_safety_toggle_remote" label="Remote"/>
   <osd type="button" command="navit_safety_confirm_poi" label="Confirm POI"/>
   <osd type="button" command="navit_safety_show_plan" label="Plan"/>

See :doc:`how_it_works` for the planning pipeline and :doc:`codemap` for source locations.
