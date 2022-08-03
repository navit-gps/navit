WinCE
=====

`320px|thumb|Navit on Windows CE <image:Navit0.2.0-2.png>`__

This is a tutorial for running Navit on a machine with Windows Core
Embedded Systems and Mobile. If you want to build Navit on your machine
please follow the instruction on the page
`WinCE_development <WinCE_development>`__.

=================== =======
Version             Support
=================== =======
Windows CE 4.0      ?
Windows CE 4.1      ?
Windows CE 4.2      ✓
Windows CE 5.0      ✓
Windows CE 6.0      ✓
Windows Mobile 2002 ?
Windows Mobile 2003 ?
Windows Mobile 5.0  ?
Windows Mobile 6    ?
Windows Mobile 6.1  ?
Windows Mobile 6.5  ?
=================== =======

Make sure your device is compatible
-----------------------------------

First, you will have to make sure that your device is compatible, that
is to say, to make sure you can install arbitrary cab files or run
arbitrary exe files on it. For some devices, there are unlock procedures
(`Mio C310 and C210 <Unlocking_Mio_C310_and_C210_for_Navit>`__) that
will allow to escape from the main program and get a Windows CE desktop.

Download cab or zip Navit package
---------------------------------

Download the latest release from http://download.navit-project.org/ and
go to the WinCE section. From there get the CAB or ZIP navit package and
put it on your device (see below). You may use the latest shipped XML
file for you to configure (or for devices with low memory output use the
file from http://ix.io/18Jb).

Installation of Navit
---------------------

SD Card or MicroSD Card
~~~~~~~~~~~~~~~~~~~~~~~

-  Copy the cab-file onto a SD card. Insert it in your mobile device.
   Start the cab-file for installation or (if applicable) use a
   CAB-Install application.

OR

-  Unpack the zip-file onto a ``navit`` folder to save on a SD card.
   Insert it in your mobile device. Navigate to the folder ``navit``
   (probably located in ``\Storage Card``. Start ``navit.exe``.

Direct Download
~~~~~~~~~~~~~~~

If you mobile device with WinCE/WinMobile is directly connected to the
internet. Download the cab file and install it.

Editing navit.xml
~~~~~~~~~~~~~~~~~

For configuration with the navit.xml file it it useful to transfer the
file ``navit.xml`` to your Desktop-Computer (ActiveSync on Win.. or with
`SynCE <http://www.synce.org/moin/>`__ on Linux). Edit the XML-file
``navit.xml`` using Notepad++ or another editor and store it back on
your mobile device.

GPS Receiver
^^^^^^^^^^^^

GPS Receiver communicates with the Operating System OS via COM1, COM2,
... normally. You have to set the COM-interface with the XML-tag in
navit.xml. For example for a bluetooth receiver connected via (virtual)
com port com6 working at baudrate 38400, update you ``navit.xml`` files
for GPS settings ( tag):

**Note** the colon: after COM6. Without it, you'll probably see lots of
"vehicle_wince:initDevice:Waiting to connect to com" in the log file.

**Note**: you will probably have to comment out, or set to disabled all
other entries to keep only this one.

If you have an internal GPS or Bluetooth receiver, then you will have to
determine the com port of the GPS.

There are several ways to do this:

-  On some devices, for internal GPS receiver, go to Settings >
   Connections > GPS. (Sometimes this item is hidden, but you can run
   the program WMGpsShow to unhide it). Tap Hardware and you can see the
   "GPS hardware port" and "Baud rate".
-  You can also run the Windows CE utility SirfTech.exe on your device
   to scan the GPS, it will give you both the com port and baud rate for
   the GPS receiver.
-  For a bluetooth-connected GPS, the virtual com port can be configured
   and / or reviewed in Bluetooth Settings on tab Serial port as value
   of input box *Outbound COM port*.
-  If the above does not help, for internal GPS receiver, have a look at
   ``registry\HKLM\Drivers\BuiltIn\GPSserial\FriendlyName`` to determine
   your GPS-COM-Port.

Once you know the GPS hardware COM port and baud rate, let's say port
COM7 and baud rate 57600, then you have to set these values in
``navit.xml`` using a line similar to:

On some device, it is necessary to use uppercase letters for the COM
port (eg: "COM5") to get it work (added by Killerhippy, hope it helps)

Using Navigation Next Turn OSD item
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To use the `navigation_next_turn <OSD#navigation_next_turn>`__ OSD item
on WinCE, the icon must be explicitely sourced. In ``navit.xml`` it
should something like:

\ `` ``

See http://trac.navit-project.org/ticket/498 for more information.

Speech
^^^^^^

Speech output should work out of the box on WinCE. In navit.xml which
comes with the WinCE Navit-build you can find:

\ `` ``

All files are included and no additional installation/configuration (and
no espeak.exe) is necessary!

Language
^^^^^^^^

Language detection may not work on WinCE devices, so you may have to
manually configure your language by manually setting it in
``navit.xml``. Here is an example for Austrian German:

.. _download_a_map_from_openstreetmap:

Download a Map from OpenStreetmap
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use `Navit pre-processed OSM
maps <http://maps.navit-project.org/download/>`__. With your browser on
desktop:

-  Navigate to the region you want,
-  mark a rectangle for your map (e.g. for Germany) and click select for
   the chosen rectanglular map.
-  then click on download and save the file to ``country.bin`` (e.g.
   ``germany.bin``) on your desktop computer.
-  copy the file to on your WinCE/WinMobile device via ActiveSync or
   SynCE on Linux (or an option that may be faster because of the size
   of the maps, you can also directly copy the map to the (Micro-)SD
   card from your PC and re-insert the card in your mobile device
   later). Create a ``maps`` directory for the maps on your SD card and
   copy the map files to that folder (the examples below assume that the
   maps will be stored in ``\StorageCard\maps``, and that the map will
   be saved as ``\StorageCard\maps\germany.bin``.

**Note** that the separator between directories is on Win-OS the "\" and
Linux, Mac, ... "/".

-  This directory will be used in ``navit.xml`` so that navit is able to
   find and use the map(s):
-  Add and enable the map for the application in navit by changing the
   lines (at approx line number 370)

| 
| ``   ``\ 
| 

| It seems you need to specify the bin name in newer versions of navit.
  I also had to add two backslashes before the bin name running 0.2.0,
  or the log would tell me it couldn't find the map file. This is most
  likely a bug...
| Disable unused mapset sections by setting enabled to ``no``, e.g. the
  pre-installed sample maps at line 370 in ``navit.xml``.

| 
| ``   ``\ 
| 

-  You may want to customize the initial location displayed on the map.
   Example below for 4123 N 1234 E (this string means 41.23 North and
   12.34 East):

Running Navit
-------------

Memory Issues
~~~~~~~~~~~~~

If you have a WinCe device with only 64 MB RAM you may encounter
problems if you try to route to far destinations. Perhaps you can fix
this in WinCe under "Einstellungen/System/Arbeitsspeicher/" and move the
slider to the left to increase your program memory (and decrease your
data memory) assignment. Perhaps you will try about 50000 KB or above
for program memory assignment. This may not fix all crashes but should
stop those immediate crashes after you have selected your destination.
Also, using a specific ``navit.xml`` configuration file like
http://ix.io/18Jb might help (see the reference above).

Vehicle settings for various devices
------------------------------------

+-------------------------------------------------------+-------------+
| Device                                                | Vehicle tag |
+=======================================================+=============+
| Yakumo Delta 300 GPS WM2003 v4.20                     |             |
+-------------------------------------------------------+-------------+
| Navigon 70/71 Easy                                    |             |
+-------------------------------------------------------+-------------+
| `DigiWalker Mio                                       |             |
| C210 <Unlocking_Mio_C310_and_C210_for_Navit>`__       |             |
+-------------------------------------------------------+-------------+
| `Sony Nav-u <Sony_Nav-u_Devices>`__                   |             |
+-------------------------------------------------------+-------------+


See also
--------

-  `WinCE development <WinCE_development>`__
