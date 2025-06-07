Brainstorming
=============

`400px|thumb <image:NAVIT_platforms.svg>`__
`400px|thumb <image:NAVIT_users.svg>`__ This article is about **internal
brainstorming** for the NAVIT crew. So don't judge about other peoples
entries, dare to be crazy! Ignore the order of items, keep it clear and
show us your mind :)

Presence
--------

How do you feel about the current situation in navit? What works well,
what not?

good
~~~~

| **navit architecture:**
| \*is highly portable!

-  is highly customizable!

| **navit features:**
| \*routing seems to get mostly useful routes

-  TTS
-  offline maps working!

| **dev process:**
| \*a small and nice community :)

| **map process:**
| \*you can get custom Areas e.g. D-A-CH

-  we offer really every part of the world
-  daily update! Jesus!

.. _not_good:

not good
~~~~~~~~

| **navit architecture:**
| \* Usability currently not in mind :(

| **navit features:**
| \* Map rendering issues (generalisation

-  Map style issues (visual clutter, styles not usefull for spec.
   scenarios)
-  Search is not failure-resistant and could be easier to use
-  adding external POIs is hell

| **dev process:**
| \* architecture not specified/autodocs useless

-  no good deployment currently
-  only IRC is used for communication (bad logging and asynchronous
   communication)
-  mostly cp15 seems to be active in development

| **map process:**
| \* map styles hard to develop

-  unclear where to get maps with newest maptool/newest data
-  make use of only few OSM POIs
-  maps need to get installed manually
-  many pages provide OSM Navit binfiles, sometimes they are broken or
   incompatible

Future
------

What do you expect from the upcomming year? What is important to you?

Recommendations
~~~~~~~~~~~~~~~

What kind of (abstract) decisions should we take that should be
reflected in the roadmap?

| **navit architecture:**
| \* should become a general navigation application (so for pedestrians,
  hikers, bikers, motorcyclists, truckers, geocachers, sportsmen,
  sailors, ...?)

-  splitup navit.xml to become easier to maintaine and to customize by
   installers (e.g. settings = deviceclass x platform x defaults)
-  and/or allow overriding global default preferences with users'
   navit.xml defining only the customized bits
-  should allow configuration within the GUI
-  support more sensors (Ant+, ODB2, eCompass, altimeter, TMC, LCD
   brightness, buzzer, ...)
-  allow external speech wavepacks (e.g. german Frederik Freiher von
   Furchensumpf, Das Känguru, Dosenfischer, ...)
-  make use of more standard formats as KML, GPX, WMS, ...
-  let Users help to improve routing, by offering an online webservice,
   where you can start routing and say if the results are good
   enough/suggest better routes
-  make use of terrain data for rendering (hill shading, contour lines,
   ...) and routing (esp. eco-mode for electric vehicles)
-  navit should be ready to use after installation
-  take care of left/right handed traffic (for speech and turns)
-  add internal multilang context help
-  try to detect automatically as much configuration options as possible
   (e.g. GPS port, ...)

| **navit features:**
| \* how to ensure that map fonts are available?

-  OSD rendering = Widgets x Layouts (Container+relative config, ...) x
   Skins x Colouring (decouple OSD layout from UI language, Hotkeys and
   resolution)
-  Antialiased rendering
-  make menues simplier and more useful (e.g. #1 search target, search
   POI, ...)
-  allow routing profiles to be tolerant for rules of the road (e.g.
   bikers can use oneway roads in opposite direction, bikers can use
   footways as well)
-  GUI/interaction should be more l game console style, to be easy to
   use on tap devices and min button devices as well
-  display of online resources, as weather (openweather.org, current
   fuel costs, FON WLAN hotspots, IGO8 traffic surveillance...)
-  predict road conditions (e.g. Ice warning at very low
   temperature+heavy wind+bridges, low fuel / fuel station warning)
-  allow complex addons as drivers logbook, "follow me" mode for
   multiple cars, OSMbugs, Facebook, ...
-  store user profile (age -> usability, weight -> fitness for biking,
   ...)
-  download store for offering custom point/track packages prebuild e.g.
   FON wifi accesspoints, tracks by portals, ...
-  drop propritetary map formats which can't be supported on the long
   term (garmin, marcopolo)
-  allow export/import of the bookmark file without root access, to save
   the bookmarks when changing the device
-  provide a collection of navit.xml folder paths for different
   devices/manufacturers
-  review and optimize the menu structure to have day-to-day tasks on
   the main menu

| **dev process:**
| \* roadmap / release, both long (major releases) and short term (point
  releases)

-  aggressive bug triaging
-  become more friendly to new designers, ... (docs, easy deployment via
   catalogue, ...)
-  make it easier to submit bugs (explain openid usage, trac cleanup,
   ....)
-  deploy ready to use packages/installers (platform settings, basemap,
   ...)
-  establish a testing process (maybe semi-automated and with manual
   checks for specific devices/platforms)
-  use distributed VCS to allow coding/submitting even when offline
-  provide synthetic changelog

| **map process:**
| \* centralize map process, make maps.navit-project.org to the one and
  only download place

-  embedd more metadata in maps (date of build, version of maptool,
   author, who converted, ...)
-  deploy a basemap (show that navit works, even if not ready to use)
-  OSD/rendering/map style resolution/orientation independent
-  rendering = data x local styling (language, ...) x scenario styling
   (car, hiking, ...)
-  map style should be integrated in day light map style and simplified

tasks
~~~~~

You wish specific steps and tasks on the implementation to move forward?

| **navit architecture:**
| \* navit.xml should be splitted up (so parts area easily to
  share/replace, editing becomes less confusing, ...)

-  rename config options to reflect the actual meaning and let them
   differ for users (e.g. vehicle active vs. enabled, profilename->name,
   ...))
-  unify config datatypes (e.g. vehicle follow=yes but active=1 :( )
-  GPS signal quality
-  allow display of multilanguage names e.g. "München (Munich)"
-  navit should display more informations aboud it's installaton, as
   everything changs quickly (svn version, build with, build by, which
   plugins installed/enabled, map details, ...)

| **navit features:**
| \* generic map downloader

-  change to something that explains, that it holds gps config
-  make vehiceprofile flags human-readable
-  routing make use of official international bike route
-  routing make use of turning restrictions

| **dev process:**
| \*splitted repositories (e.g. one for navit, maptool, map styles,
  OSDs, ....)

-  cleanup repository (/xpm -> /svg, seperate icon folders, remove
   old/redundant files, ...)
-  define OSD via a SVG mask and relative alignment
-  make OSD controls more abstract (e.g. a compass) and let them skin
   seperately
-  create platform testsuites

| **map process:**
| \*deploy NAVIT with a very down stripped basemap (only highways,
  country-borders, cities, ...)

-  introduce abstraction level in map design (e.g. text_size,
   local_highway_colour, ...) to make it easy to use on different
   devices in different countries
-  OSM boundaries should be used -> portal to create stable boundary
   poligons
-  OSM streets should be linked to places
-  split binfile and map style in layers (make runtime processing faster
   by searching less material even on low res hardware, different
   generalisation strategies e.g. for buildings, ...)
-  POI icons for brands as superstores or fuel stations
-  introduce platform specific map vars (to finetune map style e.g.
   map_density=countryside)
-  redraw map, even when dragging

**spec. platforms:**

-  flip screen on iPhone/Android

NOT
---

What should NAVIT never support or which way we don't like?

| **simple offline map viewer:**
| Tools like Google Earth or Marble to a better job presenting earth on
  the desktop. We focus on the mobile use and esp. routing/navigation!

| **Multimodal routing:**
| Means to allow routing with multiple vehicles on one single route
  (e.g. taking the bike to get to next busstop and driving by bus). This
  is currently much to complex and the OSM dataset is to inaccurate.
