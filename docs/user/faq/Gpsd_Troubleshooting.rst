.. _gpsd_troubleshooting:

Gpsd Troubleshooting
====================

Common Problems with gpsd include:

-  No support for it compiled in

Please check configure for

``GPSD support: enabled``

to make sure it includes support for gpsd and make sure
libvehicle_gpsd.so is in your plugins directory. An indication that
vehicle_gpsd is missing is also "vehicle_new:invalid type"

-  Include Files and Libraries don't match

Make sure they are from the same version of gpsd

-  Navit crashes when using gpsd

There is currently a bug in libgpsd that leads to crash in navit at
random places. As a workaround, make sure you have no +r in your gpsd
query string. This workaround prevens navit to create an nmea log, use
gpspipe for that.

-  Check if you have the right version of gpsd installed. E.g. gpsdrive
   brings its own version of gpsd that is not compatible with Navit.

-  None from the above, but it still doesn't work

Adding

to your navit.xml below or above the existing debug statement should
give you more information what goes wrong.
