Usability
=========

.. warning

   This content is probably out of date


Basic Functions
---------------

Navit has the basic functions:

-  **planning and viewing a route** from a location A to B on the map.
   The location A and B are stored in the bookmarks. And navit
   calculates the route from A to B. Subprocesses are

   -  Selecting location to start (location A -- enter A or select from
      bookmarks)
   -  Selection destination (location B -- enter A or select from
      bookmarks)
   -  Show selected vehicle -- optional: change selected vehicle to get
      the correct distances and travelling time for the trip.
   -  Calculating the route and show user that navit is calculating the
      route.
   -  Showing distance and time of the trip from A to B with selected
      vehicle
   -  View trip on the map.

-  **navigation** with the support of navit from a location A to B.
   Subprocesses are

   -  (unvisible for users) Set location to start as GPS location and do
      not overwrite location A for route planning.
   -  Selection destination (location B -- enter A or select from
      bookmarks)
   -  Press Button (Start) for navigation
   -  Calculating route and users see that navit is calculating
   -  Navigate from A to B
   -  Stop Navigation (Stop Navigation Button - already implemented)

-  **Settings** of:

   -  Select country (maybe not necessary, if ``navit.xml`` has a tag in
      the mapset definition where the country selection of e.g. a
      OpenStreetMap is already defined.
   -  Select vehicle

With the current structure of the navit menu you can do all these things
already, but the menu is not consistent to the main functions of navit.
Furthermore the user does not see at moment on the main menu page what
is the processing status of Navit. The main **Informations on a Main
Menu** are (as options for changing):

-  Main Navit-Mode: *(1) Navigation* or *(2) Route Planning*
-  Starting Point A (e.g. Berlin with GPS Info) or current GPS location
   (depending on route planning or navigation mode)
-  Destination B (if set by user or stored and saved)
-  Distance and Time to destination (if calculated by Navit - like OSD
   in map)
-  Selected Vehicle for the trip.

Clicking on the shown infos will guide the user to that part of menu
where they can change it.

Issues
------

while the engine seems to be fine, in my opinion
(`Mifritscher <User:Mifritscher>`__ 08:46, 4 April 2010 (UTC)) there are
still many flaws in the usability:

-  the internal GUI isn't very usable - no symbols activated (always
   need to go in the menu), zooming etc. is very frustrating -> please
   make the gtk the standard!
-  add scrollbars for scrolling, zooming, detail-depth (both for streets
   and text,
-  slowness - why is it so slow on a c2d with Intel x3100 running Ubuntu
   10.4?
-  add sub items for the favorites like in the internal gui - show in
   map, set as position, target etc, delete it.
-  add a ruler for estimating distances etc.
-  add options for default zoom, show whole route, reasonable
   3D-view,...
-  add a much simpler map style: high contrast, streets are only lines,
   no POIs - alternative: make this configurable during runtime (not via
   editing a xml-file...)
-  white streets on a yellow background aren't that readable
-  add items to the scrollbar: set as position, set as target, save as
   favorite, show infos (street name! - alternative: tooltip), which
   operate on the next point which is clicked on (on many cases, you
   have no right mouse key...)
-  add an item for setting the position (like target, but also with the
   option for gps) - useful for navigating without gps
-  sync route commandos and the map: by clicking a commando, show the
   regarding position on the card
-  add the possibility to save current views so they can be reloaded
   later
-  in general: make it configurable via the gui, I don't want to feedle
   with the xml while I'm on the road...
-  add process bar for routing

I know that a part of these points are changeable via the configfile -
but the "normal" user don't want to learn how to change the xml-files,
but simply work with the program. I think navit must have an reasonable
default GUI for novince users using it on "normal" computers.

Btw, my test-computer was an Leovono x61(t) - I used both the pen and
the keyboard+mouse.

seems to be a nice place to add my opinion (`nibbler <User:nibbler>`__
10:56, 6 April 2010 (UTC)) using navit 0.2.0 (ubuntu package linked on
navit-project.org)

-  if current position or destination is not reachable, the engine loops
   in the calculation printing "pos blocked" something in stdout,
   instead of voice/popup warning and giving up, or even looking for
   nearby non-blocked positions
-  images (svg and maybe even png) should be scalable by the engine
-  osd button priority: including graphics in OSDs seems to be working
   via buttons. but the underlaying layers have click-priority, so
   overlayed buttons dont work and underlayed ones are badly or even
   invisible. top buttons should have priority over the underlaying
   ones!
-  osd_configuration allows for text elements to be switched on/off but
   not for buttons, and so not for design elements
-  standard output could be more verbose, like giving speed on change or
   in intervals to be able to work with it (like beeping when to
   slow/fast in a way that is easy to control for users)
-  xml files should support enviroment variables like ~ or $HOME, so OSD
   layouts can be more generic (setting vars in XML files would be nice,
   too)

   -  This *does* work with me (Ubuntu 9.10, SVN r3136). I have (and it
      works): --`Tiiiim <User:Tiiiim>`__ 12:14, 6 April 2010 (UTC)

::

   <map type="binfile" enabled="yes" active="1" data="~/.navit/maps/osm_uk.bin" />

-  the vehicle should be switch off able from gui - to zoom/search on a
   map the vehicle first has to be disabled in xml, or it will
   continously jump/zoom the map to it

   -  This is possible using Settings --> Rules --> Map follows vehicle.
      Switch that off. (again, r3136) --`Tiiiim <User:Tiiiim>`__ 12:14,
      6 April 2010 (UTC)

-  there is no place to look up what differnt map symbols mean like
   border-like lines
-  altitude should be available as gui element like speed etc.

   -  Available as OSD element, if that's what you mean? (again r3136)
      --`Tiiiim <User:Tiiiim>`__ 12:14, 6 April 2010 (UTC)

::

   <osd enabled="yes" type="text" label="${vehicle.position_height}" />

-  vehicle.position_direction should be available in notation like N NNE
   ENE NE E ESE SE SSE S etc, or at least N NE E SE S SW W NW to be more
   human readable
-  routing should support via points
-  OSD layout should be changeable if routing is active/inactive (eg
   dont display distance if there is not destination set)
-  zoom_route() should zoom out further if follow is set
-  in internal gui my vehicle curser is not properly removed with each
   update, so leaving a trace

   -  This is something to do with map refresh update intervals (if I
      remember from the trac ticket)...--`Tiiiim <User:Tiiiim>`__ 12:14,
      6 April 2010 (UTC)

-  switching whole (included) XML files instead of working with
   osd_configuration might make it easier for dynamic menues and save
   the cpu cycles of hidden elements.
-  distance and speed OSD elements come with units, altitude and TL/ETA
   without - without would leave more space for OSD design

   -  With the distance OSD element, you can display the distance only,
      units only or distance and units together. No such option for
      speed. See http://wiki.navit-project.org/index.php/OSD#text.
      --`Tiiiim <User:Tiiiim>`__ 14:14, 7 April 2010 (UTC)

::

   <!-- Distance and units -->
   <osd enabled="yes" type="text" label="${navigation.item.destination_length[named]}" />
   <!-- Distance only -->
   <osd enabled="yes" type="text" label="${navigation.item.destination_length[value]}" />
   <!-- Distance and units -->
   <osd enabled="yes" type="text" label="${navigation.item.destination_length[unit]}" />


Android port specific issues
----------------------------

-  These or the issues I have when using navit on an andriod phone. I
   guess a lot of them are easy to implement, others aren't. I don't
   want to put them in the track because I don't see them as bugs.
   --`Sanderd17 <User:Sanderd17>`__ 14:23, 26 August 2010 (UTC)

   -  The (back) button (a hardware button on most devices) should go
      back in the menu structure.
   -  The favorites should be synchronised with the contacts or there
      should be another type of favorites which is synchronised.
   -  Text font may be a little bigger on the (standard) map. It's
      difficult readable when driving.
   -  Display next streetname in the application title or on an other
      place on the screen.
   -  generate a "navit.xml" in the sd-card home directory at first
      startup or at install. The current location of "navit.xml" is
      almost unfindable.
   -  long lists of POI's or favorites should be scrollable. (can be
      tricky)
   -  type of vehicle should be remembered.

.. _see_also:

See also
--------

-  `Brainstorming <Brainstorming>`__
