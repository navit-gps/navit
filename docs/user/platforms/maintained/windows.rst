Windows
=======

`320px|thumb|Navit under Windows, GTK Gui, Dutch
translation <image:Navit_win32_dutch.JPG>`__

============= =======
Version       Support
============= =======
Windows 2000  ?
Windows XP    ?
Windows Vista ?
Windows 7     ?
Windows 8     ?
============= =======

.. _download_the_binary_distribution_package_of_navit:

Download the binary distribution package of Navit
-------------------------------------------------

The Win32 binary distribution of Navit can be downloaded via the
`Download Navit <Download_Navit>`__ page, or download the `Win32
bleeding edge <http://download.navit-project.org/navit/win32/svn/>`__
compiled daily from SVN.

The WinCE binary distribution of Navit can be downloaded via the
`Download Navit <Download_Navit>`__ page, or download the `WinCE
bleeding edge <http://download.navit-project.org/navit/wince/svn/>`__
compiled daily from SVN.

If the created start menu entry does not work, check if a 'bin' is too
many in the link target location.

.. _tips_and_tricks:

Tips and Tricks
---------------

.. raw:: mediawiki

   {{warning|The [[Win32 GUI]] build is broken, so use [[Gui internal]] instead!}}

Compiling, configuring and running Navit on Windows can be a challenge
at times. Below are a few hints, in addition to the more detailed
instructions following.

-  Support for NMEA serial GPS receiver devices (USB/Bluetooth) was
   broken in the win32 port since Navit releases 0.1.0. It is restored
   with the latest builds (3650 and higer). The configuration for serial
   devices has changed: For example, a serial device on Com4 can be
   configured as follows:

.. code:: xml

     <vehicle name="GPSOnCom4" enabled="yes" active="1" source="serial:COM4 baud=115200 parity=N data=8 stop=1" />


See also
--------

-  `Windows development <Windows_development>`__
