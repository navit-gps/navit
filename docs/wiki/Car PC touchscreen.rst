.. _car_pc_touchscreen:

Car PC touchscreen
==================

Navit will run on any **Linux-compatible hardware**, from your high-end
desktop to computer to gpe-compatible handhelds like the HTC BlueAngel.

We'll eventually post here some tips about specific hardware.

Touchscreen
===========

If you're planning to use NavIt, then probably you want to take it out
with you on the roads. You can put it on your handheld, but you can also
put a computer in your car, and drive it using a Touchscreen.

.. _the_vm_84_touchscreen:

The VM-84 Touchscreen
---------------------

This is a really cheap TouchScreen, sold from www.short-circuit.com. You
can get it for usually 200$, when a lilliput or equivalent will cost
around 300$. **And, as a NavIt use, you get 5% discount on the shop!**

The only drawback with this touchscreen is its driver. It was a real
pain to get it to work the first time. The device is shipped with some
drivers, which are nothing but relevant. But now, thanks to this page,
it's easy :)

Note : All the credit for this work goes to Martin "cp15" S., who hacked
the elographics driver and made it work.

.. _the_usb_to_uart_bridge.:

The USB to Uart bridge.
~~~~~~~~~~~~~~~~~~~~~~~

This screen use in fact the cp2101 driver module, for the USB to Uart
bridge. So, the first step is to ensure the cp2101 module is loaded.
It's shipped with the kernel :

Symbol: USB_SERIAL_CP2101 [=m]

| ``Location:``
| ``-> Device Drivers``
| ``-> USB support``
| ``-> USB Serial Converter support``
| ``-> USB Serial Converter support (USB_SERIAL [=m])``

Once the module is loaded, and the device is plugged, you should get a
/dev/ttyUSB0 device. It could be a good time to test it before
continuing.

``od -h -w10 </dev/ttyUSB0``

Touch the screen. You should get datas, something like this :

| ``0000000 55 54 02 ec 0c 90 06 90 00 73 55 54 04 ec 0c 90``
| ``0000020 06 90 00 75 55 54 02 60 0b 00 07 00 00 c7 55 54``
| ``0000040 04 60 0b 00 07 00 00 c9 55 54 02 54 0a 98 07 98``
| ``0000060 00 ea 55 54 04 54 0a 98 07 98 00 ec 55 54 02 c8``

**Warning**: if you have no input on the VGA, or if your input is set to
something other than VGA (like RCA1) then the touchscreen functionnality
will be disabled by the screen. This mean that it won't send datas to
your computer. It took me a while to realize this, and i was thinking
that i had a problem with the cp2101 driver.

.. _the_xorg_input_driver:

The Xorg input driver
~~~~~~~~~~~~~~~~~~~~~

Now, you need to get the hacked xorg-input-elographics driver. The patch
has been submitted to freedesktop, but it will probably take some time
(and work) to have it bundled within elographics. The bug entry is here
: https://bugs.freedesktop.org/show_bug.cgi?id=13840

Until then, you can do the following :

Install xorg-server dev kit. Depending on your distro, it can be
xorg-server (Gentoo), xorg-x11-server-sdk (OpenSuse), ...

Get an already patched tarball, extract and build :

| ``wget ``\ ```http://www.kazer.org/navit/xf86-input-elographics-1.1.0-VM84.tgz`` <http://www.kazer.org/navit/xf86-input-elographics-1.1.0-VM84.tgz>`__
| ``tar xvfz xf86-input-elographics-1.1.0-VM84.tgz``
| ``cd xf86-input-elographics-1.1.0``
| ``./configure --prefix=/usr && make``

And install as root :

``make install``

Now, configure your xorg.conf. Add something like this somewhere in your
/etc/X11/xorg.conf :

| ``Section "InputDevice"``
| ``  Driver       "elographics"``
| ``  Identifier   "TouchScreen"``
| ``  Option       "ButtonNumber" "1"``
| ``  Option       "ButtonThreshold" "17"``
| ``  Option       "Device" "/dev/ttyUSB0"``
| ``  Option       "InputFashion" "Touchpanel"``
| ``  Option       "MaxX" "3868"``
| ``  Option       "MaxY" "272"``
| ``  Option       "MinX" "231"``
| ``  Option       "MinY" "3858"``
| ``  Option       "Name" "ELO;2300-TOUCHSCREEN"``
| ``  Option       "ReportingMode" "Scaled"``
| `` Option       "SendCoreEvents" "on"``
| ``# Option "DebugLevel" "9"``
| ``EndSection``

And add the driver to Section "ServerLayout"

`` InputDevice    "TouchScreen" "SendCoreEvents"``

Restart X, and voila!
