MacOS Development
=================

Here are some notes about running navit under Apple macOS.

What you will need
------------------

You need Xcode Tools and homebrew in order to install navit.

.. code:: shell

If you have macports or fink installed create and use a new user account
to build navit.

For convinience there is the script prepare_navit_macos.sh available
under the navit/scripts directory.

.. code:: shell

   $ curl https://raw.githubusercontent.com/OLFDB/navit/macosbuild/scripts/prepare_navit_macos.sh  -o prepare_navit_macos.sh

   Then start the installation procedure:

   .. code-block:: bash

   $ sh prepare_navit_macos.sh

What is working
---------------

-  internal Gui: Working, but problems with window refresh

-  GTK Gui: Working.

-  SDL Gui: Untested yet.

GPSD
----

You have to add the GPS receiver device to gpsd:

GPSD_SOCKET="/usr/local/var/gpsd.sock" /usr/local/opt/gpsd/sbin/gpsdctl
add /dev/tty.usbserial-XYZ

Speech
------

If you want (spoken) directions, use the following snippet in your
navit.xml:

.. code:: xml

   <speech type="cmdline" data="say '%s'"/>

This will use the native say command. You can list all available voices
by typing say -v ? in a terminal. Change the command to say -v
<voicename> if you would like a non standard voice to be used. New
voices can be added in system preferences->keyboard->dictation

Using Xcode
-----------

cmake -G Xcode ../ -DUSE_PLUGINS=0 -DBUILD_MAPTOOL=1 -DXSLTS=macos
-Dbinding/python=false -DCLANG_ENABLE_OBJC_WEAK=YES

Something went wrong?
---------------------

Please let us know by filing an issue on Github or reach out on IRC.
