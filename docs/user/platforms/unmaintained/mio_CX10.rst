.. _unlocking_mio_c310_and_c210_for_navit:

Unlocking Mio C310 and C210 for Navit
=====================================

This is a tutorial for unlocking Mio C310 and C210 devices in order to
have them run Navit. For instructions on how to run and configure Navit
for WinCE, please refer to the `Windows CE <WinCE>`__ page. Mio C210 and
Mio C310 devices run Windows CE 4.2, and can be unlocked to execute any
custom Windows CE application, including navit.exe.

First make sure you can start on an SD card
-------------------------------------------

First, you will have to make sure that you have an SD card with
sufficient space to store unlock programs as well as the navit files.
You will also need to test that your device can startup on the SD card.
For C210 (and maybe also C310), SD cards of a capacity above 1 GB are
likely not going to work, unfortunately...

Download unlock scripts
-----------------------

On Mio C210 and C310, you will first need to unlock your device. To do
so, I used the C310Auto.zip package available at:
http://www.gpspassion.com/forumsen/topic.asp?TOPIC_ID=75889 This package
was initially written to unlock C310 devices, and should work
out-of-the-box for C310, but can also be adapted to C210 (see posts by
tweakradje at
http://www.gpspassion.com/forumsen/topic.asp?TOPIC_ID=99520). Download
the C310Auto.zip file and unzip it to your SD card. For C310, just try
to run from the SD card. For C210, you have to adapt it using the
following process (adapted from the procedure writeen by tweakradje on
the gpspassion forum):

-  Rename C310Auto.exe to GOCE.exe as explained by tweakradje. Indeed to
   C210 device will look for GOCE.exe on the SD card when factory reset
   is initiated and run it, so we need this exact GOCE.exe filename for
   the startup script to work.
-  Rename C310Auto.c31 to GOCE.c31 as explained by tweakradje. This file
   is a script file used by GOCE.c31 to perform automatic actions at
   launch.
-  Adapt GOCE.c31 for the C210

Note: on C210, to initiate a factory default reset, insert your SD card
(with exe and c31 scripts) when device has no power, insert the power
supply (USB or batteries) then long press (roughly 5 seconds) the On/off
button until the power LED switches from green to orange. short Rxxxxxxx
message appears (black on white), and the orange welcome screen of Mio
appears. If the screen turns off after the Rxxxxxxx, try again a long
press (often, when changing the SD card, the startup from SD only works
at the second attempt) You should get the desktop showing off, with a
list of icons displayed on the screen. This means the unlock is working!

Unlock directly at factory config reset
---------------------------------------

The .c31 scripts above perform a customization of the device at factory
default, then reboot the device wiht the new config in order to get the
desktop running at each boot. I have no battery in my C210, and restart
does not work well on my C210, so I prefer to avoid the reboot and
directly get the desktop running at factory default reset. Thus, I
adapted the .c31 script, not only to prepare the desktop for the next
reboot, but directly to make it available straight on factory default
config. On the C210, in order not to loose the desktop once the c31
script has finished, we need to kill a pending process called
``Oscar90ST.exe``. We will do this automatically in the .c31 script by
using MortScript utilities (search on the Internet for MortScript.zip
downloads - I used MortScript-4.11b5). I only use the executable called
killproc.exe</tt, located inside the ``bin\PNA`` folder of the
MortScript archive.

Allow use of device unlocked at factory reset on C210
-----------------------------------------------------

I slightly adapted the procedure written by tweakradje, and I started
directly with C310Auto.zip: - First unzip C210Auto.zip to a SD card (the
C210 device does not handle all SD cards, so first make sure it
recognizes yours, SD cards of a capacity above 1 GB are likely not going
to work, unfortunately). - I renamed C310Auto.exe to GOCE.exe as
explained by tweakradje. Indeed to C210 device will look for GOCE.exe on
the SD card when factory reset is initiated and run it. - I renamed
C310Auto.c31 to GOCE.c31 as explained by tweakradje. This file is a
script file used by GOCE.c31 to perform automatic actions at launch. - I
adapted GOCE.c31 by adding the following at the end of GOCE.c31:

``Call "\Storage Card\MortScript\bin\PNA\killproc.exe" Oscar90ST.exe``

I also added automatic language selection and screen calibration:

| ``#Set the Mio nav language & screen calibration``
| ``RegEdit "\Storage Card\Unlock\Registry\MioLang.reg"``
| ``RegEdit "\Storage Card\Unlock\Registry\MioCal.reg"``

To know which value should go in this registry entries, you will need to
run the program ``Regedit`` on the desktop before and after setting the
language, and before and after calibrating the screen. Once this is
done, you can compare the parts that have changed and create the
``MioCal.reg`` and ``MioLang.reg`` files. This will allow to have the
screen calibrated and language selected automatically when performing a
factory default reset. In order to return to the desktop after setting
the language, I used a temporary .c31 script with the following lines at
the end:

| ``Wait 120000``
| ``Beep``
| ``Wait 500``
| ``Beep``
| ``Wait 500``
| ``Call "\Storage Card\MortScript\bin\PNA\killproc.exe" MioMap.exe``

This gives me 120 seconds to export the registry to a file, then run the
calibration and language. After these 120 seconds, the ``MioMap.exe``
process will be kille, the screen will return to the desktop where
``Regedit`` can be run a second time to export the registery after the
changes.

For the C210, my ``MioLang.reg`` file, for french language, looks like:

| ``REGEDIT4``
| ``[HKEY_LOCAL_MACHINE\SOFTWARE\MioMap]``
| ``"StartSuccess"="T"``
| ``"voice"="french_f3"``
| ``"language"="french"``
| ``"AutoRun"=dword:00000000``
| ``"AppDir"="\\My Flash Disk\\MioMap\\MioMap\\"``

For the C210, my ``MioCal.reg`` file looks like:

| ``REGEDIT4``
| ``[HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\TOUCH]``
| ``"CalibrationData"="460,468 652,860 276,852 276,436 652,436"``
| ``"MaxCallError"=dword:00000007``

On the C310, the registry files are already included as an example in
the default C310Auto.c31 file:

| ``RegEdit "\Storage card\Unlock\Registry\Mitac.reg"``
| ``RegEdit "\Storage Card\Unlock\Registry\Time.reg"``

Test factory reset on a SD card with unlock
-------------------------------------------

Try to perform a factory default reset with the SD card inserted in your
device, you should see the desktop and the default Mio navigation should
not start automatically anymore. You will be able to run it using the
Mio Map icon on the desktop.

Install navit on your unlock SD card
------------------------------------

Follow the instructions on the `WinCE <WinCE>`__ page. Once your navit
binary is on the SD card, you can run it by clicking on the CEcmd
utility on the desktop, navigate to the ``\Storage Card\navit`` folder,
and execute ``navit.exe`` Once this is working properly, you can add
navit to the desktop icons for a quicker launch, by adding the following
line in the .c31 script:

``DeskTop   Navit   "\Storage Card\navit\navit.exe"``

My final GOCE.c31 script for the C210 looks like:

| ``ExtRegEdit "\Storage Card\Programs\Utils\RegEdit.exe"``
| ``#Copies the program TaskBar.exe to the \Windows directory.``
| ``#This program simply unhides and enables the taskbar.``
| ``Copy "\Storage Card\Unlock\Windows\TaskBar.exe" \Windows``
| ``#Change the registry so that ST.EXE (the mio startup program) no longer runs``
| ``#and have the TaskBar program run instead``
| ``RegEdit "\Storage Card\Unlock\Registry\TaskBar.reg"``
| ``#Copies additional dlls which may be needed for other programs to run``
| ``Copy "\Storage Card\Unlock\Windows\*.dll" \Windows``
| ``#Here you can create your shortcuts to the desktop (max 12 fit on screen)``
| ``DeskTop   Navit   "\Storage Card\navit\navit.exe"``
| ``DeskTop   RegEdit   "\Storage Card\Programs\Utils\RegEdit.exe"``
| ``DeskTop   CECmd     "\Storage Card\Programs\Utils\cecmd.exe"``
| ``DeskTop   Restart   "\Storage Card\Programs\Utils\Restart.exe"``
| ``DeskTop   Off       "\Storage Card\Programs\Utils\Off.exe"``
| ``DeskTop   Rotate    "\Storage Card\Programs\Utils\Rotate.exe"``
| ``DeskTop   MioMap    "\My Flash Disk\MioMap\MioMap\MioMap.exe"``
| ``DeskTop   Settings  "\Windows\MioUtility.exe"``
| ``DeskTop   TaskMgr   "\Storage Card\Programs\Utils\ITaskMgr.exe"``
| ``DeskTop   ResInfo   "\Storage Card\Programs\Utils\ResInfo.exe"``
| ``DeskTop   Keyboard   "\Storage Card\Programs\Utils\jotkbd.exe"``
| ``#Here you can create your shortcuts to the favorites``
| ``Favorites RegEdit "\Storage Card\Programs\Utils\RegEdit.exe"``
| ``Favorites Restart "\Storage Card\Programs\Utils\Restart.exe"``
| ``Favorites TaskMgr "\Storage Card\Programs\Utils\ITaskMgr.exe"``
| ``#Here you can create your shortcuts to the programs ``
| ``Programs  RegEdit "\Storage Card\Programs\Utils\RegEdit.exe"``
| ``Programs  ResInfo "\Storage Card\Programs\Utils\ResInfo.exe"``
| ``#Set the taskbar to AutoHide``
| ``RegEdit "\Storage Card\Unlock\Registry\Shell.reg"``
| ``#Set the Mio nav language & screen calibration``
| ``RegEdit "\Storage Card\Unlock\Registry\MioLang.reg"``
| ``RegEdit "\Storage Card\Unlock\Registry\MioCal.reg"``
| `` ``
| ``Beep``
| ``Call "\Storage Card\MortScript\bin\PNA\killproc.exe" Oscar90ST.exe``

Finally, you might as well hardcode your language and force fullscreen
mode, by setting the following in ``navit.xml``:

And also

See also
--------

-  `WinCE development <WinCE_development>`__
