NavitTaxiServer
===============

.. _navit_taxi_server:

Navit Taxi Server
-----------------

I'm thinking of commercializing GPS devices with built-in GPRS (or
other) internet access. They will probably be small touchscreen tablets
or something. In this country in south america there is no(?) available
commercial maps, and now is the time to try and sell devices and
solutions :)

I'd like to:

-  Create a "taxi central" server (from now on called NTS), exposing
   https: to the internet. Tours and other info will be put there with
   php or something.
-  Create software/patches for Navit that periodically (or on cabdriver
   request) queries the NTS and sees if this particular taxi has got
   orders. If so, present a ok/cancel dialog and the cabdriver will
   respond.
-  Every time the cabdriver queries the NTS
-  If a tour is accepted, Navit will ask the NTS for directions, and set
   the route according to this.

Local taxi drivers that I've talked to, present their wishlist,
containing among other things:

-  "Mark me as busy" button
-  "Mark me as available" button
-  Panic button - this could cause the client to go wild updating its
   position, stream audio/video if available, etc. And of course notify
   nearby taxi drivers and the central.
-  Possibility for the central to add/remove roadblock information,
   updated over the air.

This functionality could be useful also for delivery agencies, etc.

With time this page hopefully will contain more useful info than just
the plan...
