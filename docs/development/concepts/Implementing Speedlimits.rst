.. _implementing_speedlimits:

Implementing Speedlimits
========================

-  Add a speedlimit attribute to attr_def.h
-  Change osm2navit to write the speedlimit attribute if present (and
   set the appropriate flag as described above)
-  Change routing code to query speedlimit if the "speedlimit present"
   bit is set and add the speed limit to the segment
-  In the graph flooding algorithm, use the speedlimit (together with a
   configurable factor to raise or lower the actual used speed) instead
   of the default speed for this segment

.. _already_done:

Already done:
-------------

-  Assign a new bit to AF\_ for "speedlimit present" (to avoid querying
   the speedlimit attribute if it isn't present)
