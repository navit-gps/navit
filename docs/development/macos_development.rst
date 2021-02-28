=================
MacOS Development
=================

Here are some notes about running navit under Apple Mac OSX.

What you will need
==================

You need Xcode Tools and homebrew in order to install navit.

  * ⚠️ Warning: If you have macports or fink installed create and use a new user account to build navit.

For convinience there is the script prepare_navit_macos.sh available under the navit/scripts directory.

.. code-block:: bash

 $ curl https://raw.githubusercontent.com/OLFDB/navit/macosbuild/scripts/prepare_navit_macos.sh  -o prepare_navit_macos.sh
 
 Then start the installation procedure:
 
 .. code-block:: bash
 
 $ sh prepare_navit_macos.sh
 

What is working
===============
* internal Gui: 	Working, but problems with window refresh
* GTK Gui: Working.
* SDL Gui: Untested yet.

GPSD
====

You have to add the GPS receiver device to gpsd:

GPSD_SOCKET="/usr/local/var/gpsd.sock" /usr/local/opt/gpsd/sbin/gpsdctl add /dev/tty.usbserial-XYZ

Speech
======

If you want (spoken) directions, use the following snippet in your navit.xml:

.. code-block:: xml

           <speech type="cmdline" data="say '%s'"/>

This will use the native say command. You can list all available voices by typing say -v ? in a terminal.
Change the command to say -v <voicename> if you would like a non standard voice to be used. New voices can be added in system preferences->keyboard->dictation


Using xcode
===========

========================================================================================================================================
WARNING: These instructions are currently outdated. Please feel free to submit a PR if you manage to build navit on Mac OSX using Xcode.
========================================================================================================================================

Download one of the `Git sources <https://github.com/navit-gps/navit>`_ that don't contain autogen.sh.

Open X-Code and create a new project. Cocoa will suffice

Add in a new target by clicking the triangle next to "Targets" and selected the location of the navit folder. Delete the previous target.

Delete the default files, and add in the navit files.

In a terminal, go into the navit folder.

.. code-block:: bash

 ./configure --disable-binding-python --disable-sample-map --disable-maptool

xcode can now build the navit


You can also use CMake.

.. code-block:: bash

 cd navit && cmake -G Xcode .

Something went wrong?
=====================

Please let us know by filing an issue on Github or reach out on IRC.
