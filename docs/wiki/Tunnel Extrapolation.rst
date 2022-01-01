.. _tunnel_extrapolation:

Tunnel Extrapolation
====================

Tunnel extrapolation is an extrapolation function which uses the last
available speed data to extrapolate the position of your vehicle inside
a tunnel.

Add ``tunnel_extrapolation="12"`` to ``<tracking ... >``. You will need
to play around with this value if you find that the cursor lags or runs
ahead of your actual position. 12 stands for 1.2s extrapolation per gps
rate.

Note that this function will only work if the particular section of
highway has the
```tunnel=yes`` <http://wiki.openstreetmap.org/wiki/Tunnel>`__ key/value
pair in OSM (and if it hasn't, go ahead and add it!). To process your
own map, see `OpenStreetMap#From_.bz2 <OpenStreetMap#From_.bz2>`__
