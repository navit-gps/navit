.. _configurationadvanced_options:

Configuration/Advanced Options
==============================

.. _advanced_options:

Advanced Options
----------------

The rest of this webpage is meant for advanced/power users who'd like to
fiddle a little more under-the-hood. The average user can safely ignore
this section!

Speech
~~~~~~

Navit can announce driving directions with voice. Navit can use
different mechanisms to play these announcements. Note that not all
tools are available on all platforms.

.. _prerecorded_samples:

Prerecorded samples
^^^^^^^^^^^^^^^^^^^

Navit is able to compose phrases if you give it a set of prerecorded
**samples**. Configuration example:

.. code:: xml

   <speech type="cmdline" data="aplay -q %s"
   sample_dir="/path/to/sampledir" sample_suffix=".wav" flags="1"
   vocabulary_name="0" vocabulary_name_systematic="0" vocabulary_distances="0"/>

The directory *sample_dir* should contain audio files. *sample_suffix*
is the common file type suffix of those files. The names of the files
(without the suffix) must correspond to the text they contain. For each
text it wants to speak, Navit will look for one or more sample files
with corresponding names (ignoring upper/lower case). So for "turn right
in 300 meters" you could use turn.wav, right.wav, in.wav, 300.wav,
meters.wav. Navit will prefer files that contain multiple words: If file
"turn right.wav" is present, it will be used even if you have turn.wav
and right.wav.

Note that Navit internally handles all text in UTF-8 encoding. If you
use a file system where file names are not encoded with UTF-8 (such as
Windows), Navit will only find files for ASCII text. If you use a
language that uses non-ASCII characters, the file name must be the
`percent encoding <http://en.wikipedia.org/wiki/Percent-encoding>`__ of
the UTF-8 representation of the text. For example the filename for "süd"
would be "s%c3%bcd.wav" (because "ü" is encoded as C3BC in UTF-8). For
this feature to work, you must set *flags* to 1.

*data* is the program that can be used to play the sample files. You
should specify the program name along with any necessary parameters. The
placeholder "%s" will be replaced with the file(s) to be played. All
files required for a text will be passed in one go, so the program will
need to support playing multiple files. Note that the %s should *not* be
quoted; the text is not passed through a shell.

Note that if any file that is needed to compose the complete phrase is
missing then Navit will be silent. In that case a warning will be
printed. Unfortunately, there is no complete list of the samples
required. However, all the navigation text is contained in the
translation files (.po files), so you can get a rough list.

By default Navit is trying to announce street names. To disable this
feature you can set *vocabulary_name* and *vocabulary_name_systematic*
to 0 in the speech tag which will specify that the speech synthesizer
isn't capable of speaking names. Also there is *vocabulary_distances*
which you can set to 0 so only the minimum set of
1,2,3,4,5,10,25,50,75,100,150,200,250,300,400,500,750 as numbers is
used.

espeak
^^^^^^

Will use espeak instead, for those who want Navit to speak to them in
English, at 150 words per minute. The *%s* is filled in by Navit when
sent to the speech synthesis software (with something like "Turn left"
or whatever is appropriate at the time). If you need more features, you
should use an external wrapper script which can contain anything
supported by your shell (see `Translations <Translations>`__).

festival
^^^^^^^^

flite
^^^^^

Mbrola
^^^^^^

Android
^^^^^^^

.. code:: xml

    <speech type="android" cps="15"/>

.. _start_up_in_silent_mode:

Start up in silent mode
^^^^^^^^^^^^^^^^^^^^^^^

To have Navit start up in silent mode, insert ``active="0"`` somewhere
in your ``speech`` tag. For example on Android:

.. code:: xml

    <speech type="android" cps="15" active="0"/>

In this case, you should place a ``toggle_announcer`` item in your
`OSD <OSD>`__ configuration, or add a menu item so you can enable speech
output when you need it.

Debugging
~~~~~~~~~

The ``debug`` tag sets the debug level for a particular Navit module:

.. code:: xml

    <debug name="gui_internal" dbg_level="info"/>

Attributes:

* **name** - the module name; optionally a function name in that module
  can be appended with a colon (e.g. ``navit:do_draw``). The module name
  is defined by the ``module_add_library()`` call in each plugin's
  ``CMakeLists.txt`` file. Common examples: ``navit``, ``gui_internal``,
  ``map_binfile``, ``speech_speech_dispatcher``, ``vehicle_gpsd``.
* **dbg_level** - the level to set for the component named by ``name``:
  ``error``, ``warning``, ``info`` or ``debug`` (default ``error``).
* **level** *(older numeric form)* - equivalent to ``dbg_level``, but
  using a number (0-3) instead of the level's name. Prefer ``dbg_level``.

Some special module names are available:

* Setting a level > 0 for **"timestamps"** enables printing of timestamps
  in debug messages.
* Setting **"segv"** to any value >= 1 installs a segmentation-fault
  handler that prints a backtrace. With a value of 1, gdb is run
  non-interactively just to capture the backtrace and then quit; with a
  value > 1, gdb is started and kept attached for interactive debugging.
* **"global"** sets the global debug level (applies to all modules); this
  is the same as using the command line option ``-d``.

Bookmarks
~~~~~~~~~

See `Add Bookmarks with Dbus <Dbus#add_bookmark_signal>`__

Navigation and routing
~~~~~~~~~~~~~~~~~~~~~~

Tracking
^^^^^^^^

The ``tracking`` tag controls how Navit follows the position reported by
the active vehicle while it is moving:

.. code:: xml

    <tracking cdf_histsize="4" route_pref="1000"/>

Attributes:

* **cdf_histsize** - the number of recent position samples kept for the
  cumulative-displacement (CDF) filter that smooths the reported
  position. A value of 0 disables the filter. See
  `this article <http://julien.cayzac.name/code/gps/>`__ for behind the
  filter.
* **route_pref** - the routing bonus (added to a street's routing value)
  that makes Navit prefer staying on the calculated route. Increasing
  this value helps to stay on track while receiving an inaccurate GPS
  position. The default is 300; use 1000 to 3000 if the device tends to
  skip from the track.

Announcements
^^^^^^^^^^^^^

Voice announcements are defined with ``announce`` tags inside a
``navigation`` tag:

.. code:: xml

    <navigation>
      <announce type="street_0,street_1_city" level0="300" level1="1000" level2="2000"/>
    </navigation>

Attributes:

* **type** - types of ways for which this announcement is valid.
* **level0** *(metres)* - the distance at which the final announcement
  is made (i.e. "turn left now").
* **level1** *(metres)* - the distance at which the intermediate
  announcement is made (i.e. "turn left in 1km").
* **level2** *(metres)* - the distance at which the first announcement
  is made (i.e. "turn left soon").

.. _splitting_navit.xml:

Splitting navit.xml
~~~~~~~~~~~~~~~~~~~

Navit has support for a small subset of **XInclude** / **XPath** for
including parts of external XML files. Supported is a tag like

.. code:: xml

    <xi:include href="some_file" xpointer="xpointer_stuff" />

You can leave out either href (xi:include refers to the same file it is
in then) or xpointer (xi:include then refers the complete file), but not
both. The *href* attribute refers to a file relative to the current
directory. It is suggested to use the complete path, such as
*/home/root/.navit/navit-vehicles.xml*.

href is expanded with wordexp internally, so you can do stuff like:

.. code:: xml

    <xi:include href="$NAVIT_SHAREDIR/maps/*.xml" />

Some examples on the supported syntax:

.. code:: xml

    <xi:include xpointer="xpointer(/config/navit/layout[@name='Car']/layer[@name='points'])" />

references to the XML-Tag "layer" with attribute "name" of value
"points" within an XML-Tag "layout" with attribute "name" of value "Car"
within an XML-Tag "navit" within an XML-Tag "config".

.. code:: xml

    <config xmlns:xi="http://www.w3.org/2001/XInclude">
    <xi:include href="$NAVIT_SHAREDIR/navit.xml" xpointer="xpointer(/config/*[name(.)!='navit'])"/>
    <navit center="4808 N 1134 E" zoom="256" tracking="1" cursor="1" orientation="0">
    <xi:include href="$NAVIT_SHAREDIR/navit.xml" xpointer="xpointer(/config/navit/*[name(.)!='vehicle'])"/>
    </navit>
    </config>

Use this as your $HOME/.navit/navit.xml and you will get everything
under .. except .. (first xi:include), plus as specified plus everything
from navit within config, except the vehicle definitions (second
xi:include).
