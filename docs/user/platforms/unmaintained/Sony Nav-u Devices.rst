.. _sony_nav_u_devices:

Sony Nav-u Devices
==================

This is a tutorial for unlocking Sony Nav-u devices in order to have
them run Navit. For instructions on how to run and configure Navit for
WinCE, please refer to the `Windows CE <WinCE>`__ page. Sony Nav-u
devices run Windows CE, and can be tweaked to execute navit.exe.

In order to run Navit on your Sony nav-u device, follow the steps below:

.. _download_unlock_scripts:

Download unlock scripts
-----------------------

Download the following programs:

-  Mortscript
   `1 <http://www.sto-helit.de/index.php?module=page&entry=ms_overview&action=view&menu=25>`__
-  Tooldevice.exe (this one you will have to find somewhere on the web)

.. _upload_unlock_scripts_to_the_device:

Upload unlock scripts to the device
-----------------------------------

-  Hard-reset your device (reset button on bottom) to gain write-access.
-  Connect your nav-u device to your computer via Activesync and USB and
   start it up.
-  After connecting, click on "Explore" in the ActiveSync window
   (alternatively you can go to "Mobile device" in Explorer).
-  Browse to "``Mounted Volume``".
-  Create a folder called "navit" and copy the contents of the Navit
   package for WinCE (``navit.zip``, see the `Windows CE <WinCE>`__
   page) into that folder.
-  Copy ``sirftech.exe`` into "``Mounted Volume``".
-  Copy the contents of the ``bin/PPC`` from ``mortscript.zip`` into
   "``Mounted Volume\Sony\nav-u``"
-  Copy ``Tooldevice.exe`` into "``Mounted Volume\Sony\nav-u\``"
-  Rename ``nav-u.exe`` to ``nav-u-original.exe`` (or any other name for
   backup)
-  Rename ``Autorun.exe`` to ``nav-u.exe``
-  Create a text file (MortScript command script) named ``nav-u.mscr``
   with the following content:

| ``RunWait("\Mounted Volume\Sony\nav-u\ToolDevice.exe","off 18")``
| ``RunWait("\Mounted Volume\Sony\nav-u\ToolDevice.exe","off 15")``
| ``RunWait("\Mounted Volume\Sony\nav-u\ToolDevice.exe","on Serial")``
| ``Run ("\Windows\TestMode.exe")``
| ``Sleep (50)``
| ``Kill ("TestMode.exe")``
| ``# set volume to zero for system settings``
| ``Set Volume (0)``
| ``RunWait ("\windows\control.exe")``
| ``Runwait ("\Mounted Volume\Sirftech.exe")``
| ``# Set Volume to highest loudness for espeak.``
| ``SetVolume (255)``
| ``# Exit``
| ``While (TRUE)``
| ``RunWait ("\Mounted Volume\navit\navit.exe")``
| ``EndWhile``

-  You can customize the content of "``Mounted Volume\navit\navit.xml``"
   by substituting the relevant lines with the following suggestions for
   this device:

| 
| 
| 
| 
| 
| 
| 
| 
| 

.. _test_the_customized_startup_with_unlock_and_automatic_launch_of_navit:

Test the customized startup with unlock, and automatic launch of navit
----------------------------------------------------------------------

-  Disconnect your device from the computer, reset it again and start it
   up.
-  System Settings will show up, where you can set power options, sound
   etc.
-  Exit System Settings

-  Sirftech will open, where you will have to set your gps chip to NMEA
   mode:

   -  Goto "Com"
   -  Do NOT click "find port and baudrate"!
   -  Select your gps COM port from the dropdown list
   -  Try to open port with different baudrates, starting with 4800
      (and, if that does not work, with 57600 baud). At the right
      baudrate, it will show either NMEA or SIRF mode and the incoming
      data.

      -  If the port is set to NMEA, 4800 baud, everything's fine and
         you can close the port and exit the program.
      -  If the port is set to SIRF, 57600 baud, then leave the port
         open, click OK and open the Sirf menu. The last item on that
         menu lets you change the settings to NMEA mode. Select 4800
         baud, click set and =baudrate. Go back to "Com". Now it should
         show nmea mode working with 4800 baud.
      -  In case the COM port or baudrate that you set above is
         different from the line in ``navit.xml``, you will have to
         update ``navit.xml`` with the correct values as detailed
         `here <https://wiki.navit-project.org/index.php/WinCE#GPS_Receiver>`__

-  Exit SirfTech. Navit should show up. Have fun!

Note: normally the gps chip stays set in NMEA mode even after a reset,
but if you reset twice, the chip is reset to SIRF mode and you have to
switch it to NMEA mode again.
