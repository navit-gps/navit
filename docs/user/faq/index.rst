FAQ
===

There are the most common troubles and solutions for those. Please add
your soluted problem here.

.. _run_time:

Run time
--------

.. _flooded_land:

Flooded land
~~~~~~~~~~~~

Sometimes areas appear to be covered with water, even if there is just
land. This is a result of defective `OpenStreetMap <OpenStreetMap>`__
data, here esp. the `coastline
borders <http://wiki.openstreetmap.org/wiki/Coastline>`__.

Düdingen_flood.png|flooded Düdingen_ok.png|ok

You can help to fix that problem:

-  look at the `OSM coastline error
   checker <http://wiki.openstreetmap.org/wiki/Coastline_error_checker>`__,
   if there are obviously data problems in the surrounding area and try
   to fix them
-  tip at the mapview at the waterarea, menu: actions, curr. position,
   poly_water_tiled,attributes

   -  this reports an OSM way ID, that causes the problem and that you
      can lookup at http://www.openstreetmap.org/browse/way/012345

-  ask in the Forum, e.g.
   `1 <https://forum.navit-project.org/viewtopic.php?f=6&t=446>`__

.. _permanently_wrong_announcements:

Permanently wrong announcements
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A strange bug, where you get every few seconds the direction to turn,
even if there is no obvious reason to do so. This can happen, if you
have (accidently?) multiple `binfile <binfile>`__ maps enabled.

Tickets `#1039 <http://trac.navit-project.org/ticket/1039>`__,
`#1046 <http://trac.navit-project.org/ticket/1046>`__

.. _address_not_found:

Address not found
~~~~~~~~~~~~~~~~~

| Even if can has various reasons that a specific address isn't found
  (not at mapdata, wrong data/data format, internal bugs, ...), it is a
  specific behaviour, if Navit couldn't recognize any of your local
  addresses.
| This is caused how `maptool <maptool>`__ recognizes
  `OpenStreetMap <OpenStreetMap>`__ addresses and how they are modelled
  `2 <http://wiki.openstreetmap.org/wiki/Address>`__
  `3 <http://wiki.openstreetmap.org/wiki/Boundaries>`__
  `4 <http://wiki.openstreetmap.org/wiki/Key:is_in>`__. In short: we do
  a lot of postprocessing, that is country-specific and needs to be
  adapted to your country. Currently there are just a few countries
  enabled that way:

-  Germany
-  Swiss

.. _navit_hangs_at_failed_to_create_gui:

Navit hangs at "failed to create gui"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Example :

| ``o (process:12367): WARNING **: failed to create gui 'sdl'``
| ``Using '/home/kazer/.navit/navit.xml'``

Most likely, your navit.xml file is incomplete / erroneous. The best
thing to try, is to start navit with the default configuration file :

``./navit navit.xml``

if you're running it from the source directory.

If it loads, then your configuration file is broken. First, check if you
have the section at the beginning of the file.

The easiest thing to do to fix this is probably to overwrite your own
config file with the one shipped with Navit. Don't forget to read
Configuring navit for some tips about the config file, especially if you
run the SDL gui (GTK gui should work fine out of the box).

.. _navit_complains_error_parsing_navit.xml:

Navit complains: Error parsing 'navit.xml'
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Example:

``o Error parsing 'navit.xml': Element 'plugins' within unexpected context '(null)'. Expected 'config' at line 4, char 1``

In the example the <config> opening and closing tags were missing from
the navit.xml file.

.. _my_position_is_reported_incorrectly:

My position is reported incorrectly
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Most of the time, this is caused by the decimal separator being
incorrectly set. Navit tries to override the decimal separator at
startup, but the LC_ALL environment variable can block this. You can
check this easily : echo $LC_ALL

If it returns something, you can't override your locale settings. You
should "unset LC_ALL" and try again. If it works, you got it :)

In gentoo, this is set in the /etc/env.d/locales. You may want to remove
or comment the LC_ALL line in that file.

.. _in_the_gtk_gui_accents_are_broken_in_the_buttons_and_i_get_blank_entries_in_the_menus:

In the GTK gui, accents are broken in the buttons, and i get blank entries in the menus
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Same thing as previously. The LC_ALL blocks you from changing the
charset.

.. _navit_routes_me_off_a_highway_and_then_immediately_back_with_osm:

Navit routes me off a highway and then immediately back with OSM
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If OSM has an highway=motorway_link with same speed limit as the highway
then navit uses the ramp if it is technically a few meters shorter
route. You can avoid this by searching for "<vehicleprofile name="car""
in navit.xml and setting "maxspeed_handling="1"" to make navit ignore
the speed limit given in OSM and to default to 40 km/h (or whatever your
navit.xml says) for highway=motorway_link.

This has been discussed at least at
https://sourceforge.net/projects/navit/forums/forum/512960/topic/3702652

Configuration
-------------

.. raw:: mediawiki

   {{merge|Configuration}}

.. _navit_speaks_but_its_all_english...:

Navit speaks! But it's all English...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

How do you configure Navit to speak one of the other 22 languages? How
do you do that on Windows? How on Linux?

.. _espeak_on_linux:

espeak on Linux
^^^^^^^^^^^^^^^

To get Navit to speak with espeak you can add the following XML to your
navit.xml file:

`` ``\

This tells espeak to output in the English language (-ven) using female
voice #4 (+f4).

To change the spoken language - say to Dutch - there are two things that
need to change: the words that are to be spoken need to be translated to
Dutch, and the speech pronunciation has to change to Dutch too.

For the first, you can change the LANG variable but much better solution
is to use language attribute of the config tag:

`` ``\

Here "nl" tells Navit to display its interface and announce directions
in Dutch and "NL" to default town searching to the Netherlands. Change
"nl_NL" to your preferred locale if you want something else. Note that
the country and language do not need to be the same so "de_FR" is a
perfectly valid combination for German in France.

To change the pronunciation -- I'll use Dutch again as an example -- use
this in your navit.xml instead:

`` ``\

For different eSpeak language codes see `the eSpeak language
documentation <http://espeak.sourceforge.net/languages.html>`__.

.. _weird_coordinates_in_bookmarks.txt_and_centre.txt:

Weird coordinates in bookmarks.txt and centre.txt
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Internally, Navit uses hexadecimal values to represent latitude and
longitude values (see `Coordinate format <Coordinate_format>`__ for
details). These values spill over to the outside world in bookmarks.txt
and centre.txt, where points are saved as hexadecimal values by default.

.. _convert_hex_coordinates_to_decimal_degrees:

Convert hex coordinates to decimal degrees
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following example entry from centre.txt will be used as our example:

`` mg: 0x7c877 0x6496e5``

The first set of numbers/letters is the longitude, the second the
latitude.

| **Step 1: Convert hex to integer**
| The first step is to convert the strings to integer values. This can
  be done in a variety of ways:

| 1) Online
| A variety of converters can be found by Googling "hex to decimal
  converter". One such is Wolfram Alpha, which provides the following
  results (http://www.wolframalpha.com/input/?i=0x7c877+to+decimal):

| `` 0x7c877  -> 510071``
| `` 0x6496e5 -> 6592229``

| 2) Command line
| Using the ``printf`` command, hex can be converted to decimal using
  the following command:

| `` printf "%d" 0x7c877``
| `` 510071``
| `` printf "%d" 0x6496e5``
| `` 6592229``

| **Step 2: Convert integer to decimal degrees**
| The integers are then converted to decimal degrees using the following
  formulae (from ):

| `` Longitude: INT/6371000.0/PI*180``
| `` Latitude: atan(exp(INT/6371000.0))/PI*360-90``

You can drop the calculation straight into Wolfram Alpha, as below:

| `` Longitude: 510071/6371000.0/PI*180 = ``\ ```4.5871787`` <http://www.wolframalpha.com/input/?i=510071/6371000.0/PI*180>`__
| `` Latitude: atan(exp(6592229/6371000.0))/PI*360-90 = [``\ ```http://www.wolframalpha.com/input/?i=atan(exp(6592229/6371000.0`` <http://www.wolframalpha.com/input/?i=atan(exp(6592229/6371000.0>`__\ ``))/PI*360-90 50.877276]``

So there we go:

| `` Longitude: 4.5871787``
| `` Latitude: 50.877276``

`Which is here, just in case you wanted to
know... <http://maps.google.co.uk/maps?hl=en&xhr=t&q=Longitude:+4.5871787+++Latitude:+50.877276&cp=44&um=1&ie=UTF-8&sa=N&tab=wl>`__

**Converting in one commmand**

| `` Longitude: ``\ ```http://www.wolframalpha.com/input/?i=0x7c877/6371000.0/PI*180`` <http://www.wolframalpha.com/input/?i=0x7c877/6371000.0/PI*180>`__
| `` Latitude: ``\ ```http://www.wolframalpha.com/input/?i=atan(exp(0x6496e5/6371000.0`` <http://www.wolframalpha.com/input/?i=atan(exp(0x6496e5/6371000.0>`__\ ``))/PI*360-90``

.. _convert_decimal_degrees_to_hex_coordinates:

Convert decimal degrees to hex coordinates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Using the same location as the previous example:

| `` Longitude: 4.5871787``
| `` Latitude: 50.877276``

| **Step 1: Convert decimal degrees to integer**
| From :

| `` LONG*6371000.0*PI/180``
| `` ln(tan(((LAT+90)*PI)/360))*6371000.0``

Dropping into Wolfram Alpha:

| `` 4.5871787*6371000.0*PI/180 = ``\ ```510071`` <http://www.wolframalpha.com/input/?i=4.5871787*6371000.0*PI/180>`__
| `` ln(tan(((50.877276+90)*PI)/360))*6371000.0 = [``\ ```http://www.wolframalpha.com/input/?i=ln(tan`` <http://www.wolframalpha.com/input/?i=ln(tan>`__\ ``(((50.877276%2B90)*PI)/360))*6371000.0 6592229]``

| **Step 2: Convert integer to hex coordinate**
| Longitude: 510071 ->
  `7c877 <http://www.wolframalpha.com/input/?i=510071+to+hex>`__

`` Latitude: 6592229 -> ``\ ```6496e5`` <http://www.wolframalpha.com/input/?i=6592229+to+hex>`__

Don't forget the leading **0x** to finish it off:

| `` Longitude: 0x7c877``
| `` Latitude: 0x6496e5``

**Converting in one command**

| `` Longitude: ``\ ```http://www.wolframalpha.com/input/?i=round(4.5871787*6371000.0*PI/180)+to+hex`` <http://www.wolframalpha.com/input/?i=round(4.5871787*6371000.0*PI/180)+to+hex>`__
| `` Latitude: ``\ ```http://www.wolframalpha.com/input/?i=round(ln(tan`` <http://www.wolframalpha.com/input/?i=round(ln(tan>`__\ ``(((50.877276%2B90)*PI)/360))*6371000.0)+to+hex``

.. _negative_coordinates:

Negative coordinates
^^^^^^^^^^^^^^^^^^^^

Sometimes the decimal or hex coordinate values have a negative sign in
front of it. This will denote either a southern latitude or a western
longitude. If you perform the transformation in one step using Wolfram
Alpha, you can keep the negative sign in the formula without problems.
If you're transforming by hand, it's probably easiest to forget about
the sign whilst transforming, and just add it back in at the end.

An example in hex is:

``mg: -0x1c766 0x6812ff``

Which can be put straight into Wolfram Alpha as follows:

| `` Longitude: ``\ ```http://www.wolframalpha.com/input/?i=-0x1c766/6371000.0/PI*180`` <http://www.wolframalpha.com/input/?i=-0x1c766/6371000.0/PI*180>`__
| `` Latitude: ``\ ```http://www.wolframalpha.com/input/?i=atan(exp(0x6812ff/6371000.0`` <http://www.wolframalpha.com/input/?i=atan(exp(0x6812ff/6371000.0>`__\ ``))/PI*360-90``

Compilation
-----------

.. raw:: mediawiki

   {{merge|Building}}

.. _build_logs:

Build Logs
~~~~~~~~~~

`Build logs <http://download.navit-project.org/logs/navit>`__ of
released and committed revisions are available.

.. _my_compilation_fails_complaining_about_glglc.h:

My compilation fails, complaining about GL/glc.h
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You are missing the `quesoglc <quesoglc>`__ package. We may switch to
another more common library at some point. It isn't widely used, so
maybe it isn't available via your distro's repository. If you can't find
it, try to compile it from sources, available on the quesoGLC website

If your running Gentoo, you can grab an ebuild for it here :
http://www.kazer.org/navit/quesoglc-0.6.0.ebuild.

.. _cant_exec_autopoint_no_such_file_or_directory:

Can't exec "autopoint": No such file or directory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You're missing gettext-devel.

.. _configure.in21_error_possibly_undefined_macro_ac_disable_static:

configure.in:21: error: possibly undefined macro: AC_DISABLE_STATIC
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You're missing libtool.

.. _configure.in25_error_possibly_undefined_macro_ac_define:

configure.in:25: error: possibly undefined macro: AC_DEFINE
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If autogen.sh complains about missing macro AC_DEFINE or AC_MSG_WARN:
You're missing pkg-config

.. _gui_sdl_window.cpp2319_error_cegui.h_no_such_file_or_directory:

gui_sdl_window.cpp:23:19: error: CEGUI.h: No such file or directory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You need Crazy Eddie's GUI development files. Ususally called
cegui-devel or libcegui-mk2-dev.

.. _navit_cant_find_libgarmin_but_i_installed_it:

Navit can't find libgarmin, but i installed it!
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Try :

``export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig/``

Then re-run navit's configure
