.. _tomtom_development:

TomTom development
==================

.. raw:: mediawiki

   {{warning |
   This Page is old, dont use SVN anymore!
   }}

.. _prerequisities_for_building_navit:

Prerequisities for building Navit
---------------------------------

Before we can start, we need to set up a compiler toolchain targetting
TomTom devices. TomTom provides a a pre-compiled compiler toolchain for
Linux and Windows (CygWin). It can be downloaded from TomTom's website:
http://www.tomtom.com/page.php?Page=gpl. This file needs to be unpacked
into /usr/local/cross.

In order to use the TomTom compiler and libraries, we need to set some
environment variables:

.. code:: bash

    export PATH=/usr/local/cross/gcc-3.3.4_glibc-2.3.2/bin:$PATH
    export PREFIX=/usr/local/cross/gcc-3.3.4_glibc-2.3.2/arm-linux/sys-root
    export CFLAGS="-O2 -I$PREFIX/include -I$PREFIX/usr/include"
    export CPPFLAGS="-I$PREFIX/include -I$PREFIX/usr/include"
    export LDFLAGS="-L$PREFIX/lib -L$PREFIX/usr/lib"

Finally, some standard build tools are necessary for building some of
the libraries and the Navit binaries. The most important one is the
autotools suite, including autoconf, automake, autoheader and aclocal.
Install these if you don't already have them.

The toolchain is now ready for use.

.. _porting_libraries:

Porting libraries
-----------------

Several libraries need are needed by Navit and must be ported as well.

zlib
~~~~

Get the zlib source from http://zlib.net/. zlib's configure script does
not support selecting the host system via --host, so we need to set a
few environment variables by hand before we can compile zlib:

.. code:: bash

    export CC=arm-linux-gcc
    export CXX=arm-linux-g++
    export LD=arm-linux-ld
    export NM="arm-linux-nm -B"
    export AR=arm-linux-ar
    export RANLIB=arm-linux-ranlib
    export STRIP=arm-linux-strip
    export OBJCOPY=arm-linux-objcopy
    export LN_S="ln -s"

Now we can configure, make and install:

.. code:: bash

    ./configure --prefix=$PREFIX
    make
    make install

This has been tested with zlib 1.2.5. Older versions require AR to bet
set to "arm-linux-ar -r" instead. Before proceeding, make sure to reset
it to arm-linux-ar because the "-r" switch is needed only by these old
zlib versions and will cause conflicts compiling the other libraries.

libxml2
~~~~~~~

This one is rather straightforward. Simply download the source from
ftp://xmlsoft.org/libxml2/, configure (disable Python as this may cause
problems), make and install:

.. code:: bash

    ./configure --prefix=$PREFIX --host=arm-linux --without-python
    make
    make install

libpng
~~~~~~

The libpng source can be downloaded from
http://sourceforge.net/projects/libpng/files/. I recommend using
libpng-1.2.xx - I have tried 1.4.xx before but that one didn't work.
Configure, make and install:

.. code:: bash

    ./configure --prefix=$PREFIX --host=arm-linux
    make
    make install

libjpeg
~~~~~~~

Get the libjpeg source from
http://sourceforge.net/projects/libjpeg/files/ - the current version
(which worked for me) should be 6b. It seems to rely on the directory
$PREFIX/man/man1 to be present, so we need to create this if it is not
already present. Configure, make and install:

.. code:: bash

    mkdir -p $PREFIX/man/man1
    ./configure --prefix=$PREFIX --host=arm-linux
    make
    make install

libfontconfig
~~~~~~~~~~~~~

fontconfig can be obtained from http://fontconfig.org/release/. This one
is again very simple:

.. code:: bash

    ./configure --prefix=$PREFIX --host=arm-linux --with-arch=arm
    make
    make install

glib
~~~~

Cross compiling glib is a bit tricky. First, we need the sources from
http://ftp.gnome.org/pub/gnome/sources/glib/. The configure script tries
to probe some characteristics of the host machine by compiling a few
test programs and running them - which will fail when a cross compiler
is used. We therefore need to set the results of these tests by hand. In
the glib source directory, create a file named tomtom.cache with the
following contents:

.. code:: bash

    glib_cv_long_long_format=ll
    glib_cv_stack_grows=no
    glib_cv_uscore=no
    ac_cv_func_posix_getgrgid_r=yes
    ac_cv_func_posix_getpwuid_r=yes

Make this file read-only to keep configure from overwriting it,
configure, make and install.

.. code:: bash

    chmod a-w tomtom.cache
    ./configure --prefix=$PREFIX --host=arm-linux --cache-file=tomtom.cache
    make
    make install

tslib
~~~~~

These instructions are based on http://www.opentom.org/Tslib. First, we
need the tslib source from the SVN repository:

.. code:: bash

    svn co svn://svn.berlios.de/tslib/trunk/tslib tslib

Before compiling, we need to patch the source a bit: Open the file
plugins/input-raw.c in the tslib source directory and search for
EVIOCGRAB. You will find several occurences of EVIOCGRAB, each of them
within some if-statement. Enclose these if-statements with #ifdef
EVIOCGRAB ... #endif, so that you get something like this:

.. code:: c

    #ifdef EVIOCGRAB
      if(...)
      { ... EVIOCGRAB .... }
    #endif

We also need to edit the config file etc/ts.conf. For older TomToms,
uncomment the line "module_raw h3600", so the file should look like
this:

.. code:: apache

    # Uncomment if you wish to use the linux input layer event interface
    # module_raw input
    
    # Uncomment if you're using a Sharp Zaurus SL-5500/SL-5000d
    # module_raw collie
    
    # Uncomment if you're using a Sharp Zaurus SL-C700/C750/C760/C860
    # module_raw corgi
    
    # Uncomment if you're using a device with a UCB1200/1300/1400 TS interface
    # module_raw ucb1x00
    
    # Uncomment if you're using an HP iPaq h3600 or similar
    module_raw h3600
    
    # Uncomment if you're using a Hitachi Webpad
    # module_raw mk712
    
    # Uncomment if you're using an IBM Arctic II
    # module_raw arctic2
    
    module pthres pmin=1
    module variance delta=30
    module dejitter delta=100
    module linear

The new TomTom versions (ONE, XL) require a different driver. For these
devices, uncomment "module_raw input" instead. Finally, use autogen.sh
to create a configure script, configure, make and install.

.. code:: bash

    ./autogen.sh
    ./configure --prefix=$PREFIX --host=arm-linux
    make
    make install

SDL
~~~

The recipe for compiling libSDL is based mainly on
http://www.opentom.org/LibSDL and http://www.opentom.org/Talk:LibSDL.
First, get the source from http://www.libsdl.org/download-1.2.php.
Unfortunalety we can't compile it out of the box, because the "fbcon"
driver which we will use on TomTom relies on the presence of a virtual
console, which is not present on TomTom Linux. Therefore, we have to
patch the SDL source, using the patch from
http://tracks.yaina.de/source/sdl-fbcon-notty.patch. After applying the
patch, configure (disabling most of the unneeded drivers), make and
install:

.. code:: bash

    ./configure --prefix=$PREFIX --host=arm-linux \
     --disable-esd --disable-joystick --disable-cdrom --disable-video-x11 \
     --disable-x11-vm --disable-dga --disable-video-x11-dgamouse \
     --disable-video-x11-xv --disable-video-x11-xinerama --disable-video-directfb \
     --enable-video-fbcon --disable-audio CFLAGS="$CFLAGS -DFBCON_NOTTY"
    make
    make install

The "-DFBCON_NOTTY" invokes the patch mentioned above and removes
fbcon's dependence on a virtual console. If you like, you can also
compile the test applications, as these are quite useful for testing
whether libSDL works:

.. code:: bash

    cd test
    ./configure --prefix=$PREFIX --host=arm-linux
    make
    make install

SDL_Image
~~~~~~~~~

This one is rather easy again. Get the source from
http://www.libsdl.org/projects/SDL_image/, configure, make and install:

.. code:: bash

    ./configure --prefix=$PREFIX --host=arm-linux
    make
    make install

.. _porting_navit:

Porting Navit
-------------

Having all libraries needed by Navit in the right places, we can now
proceed compiling Navit itself. First, get the Navit source either from
http://sourceforge.net/projects/navit/files/ or via SVN from
http://sourceforge.net/scm/?type=svn&group_id=153410 (the latter is
recommended). We can simply run autogen.sh, configure, make and install:

.. code:: bash

    ./autogen.sh
    ./configure --prefix=$PREFIX --host=arm-linux --disable-graphics-gtk-drawing-area --disable-gui-gtk \
     --disable-graphics-qt-qpainter --disable-binding-dbus --disable-fribidi --enable-cache-size=16777216
    make
    make install

Since we have not ported gtk, as we will use libSDL and the internal GUI
only, we have to disable the gtk drivers in the configure command.

.. _installing_navit:

Installing Navit
----------------

.. _creating_directories:

Creating directories
~~~~~~~~~~~~~~~~~~~~

We can now put the compiled libraries and the Navit executable, as well
as some config files and, of course, some maps, to a TomTom device.
Connect the TomTom device to your computer. You should see a new hard
drive. In the following, let's assume this hard drive can be found in
/media/TOMTOM. We then need two directories on the TomTom disk, navit
and SDKRegistry. If they don't exist yet, create them:

.. code:: bash

    cd /media/TOMTOM
    mkdir -p navit SDKRegistry

Within the navit directory, create the following directories: bin, lib,
share, sdl and ts.

.. code:: bash

    cd navit
    mkdir -p bin lib share sdl ts

Now these directories have to be filled with content.

.. _installing_libraries:

Installing libraries
~~~~~~~~~~~~~~~~~~~~

Put all the libraries you just compiled into the lib directory. Be aware
that TomTom uses a FAT file system which isn't aware of symlinks, so you
will have to copy / rename libraries instead of symlinking them. You
should end up with something similar to these files:

-  libSDL-1.2.so
-  libSDL.so
-  libSDL_image-1.2.so
-  libSDL_image.so
-  libfontconfig.so
-  libfreetype.so
-  libgio-2.0.so
-  libglib-2.0.so
-  libgmodule-2.0.so
-  libgobject-2.0.so
-  libgthread-2.0.so
-  libpng.so
-  libpng12.so
-  libts-1.0.so
-  libts.so
-  libxml2.so

It seems that TomTom's compiler toolchain contains some libraries which
are not present on the TomTom devices, so you may need to copy the
following libraries as well:

-  librt.so
-  libthread_db.so

.. _installing_tslib:

Installing tslib
~~~~~~~~~~~~~~~~

We already copied the tslib library into the navit/lib folder in the
previous step. But in order to use tslib, we still need the config file
telling tslib which driver to use. Copy it into navit/ts:

.. code:: bash

    cp $PREFIX/etc/ts.conf /media/TOMTOM/navit/ts/

tslib comes with some utilities for touchscreen devices, such as
calibration and testing tools. When you followed the procedure described
above for compiling and installing tslib, you can find them in
$PREFIX/bin - easily recognizable, because their names start with "ts_".
If you wish to use them on your TomTom, simply copy them into the
navit/ts folder:

.. code:: bash

    cp $PREFIX/bin/ts_* /media/TOMTOM/navit/ts/

In order to run them, you will need a small wrapper script which sets
some environment variables before executing the actual utility. For
example, the following script will run the calibration utility and then
start a simple test program:

.. code:: bash

    #!/bin/sh
    
    cd /mnt/sdcard
    
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/sdcard/navit/lib
    
    export TSLIB_CONSOLEDEVICE=none
    export TSLIB_FBDEVICE=/dev/fb
    export TSLIB_TSDEVICE=/dev/ts
    export TSLIB_CALIBFILE=/mnt/sdcard/navit/ts/pointercal
    export TSLIB_CONFFILE=/mnt/sdcard/navit/ts/ts.conf
    export TSLIB_PLUGINDIR=/mnt/sdcard/navit/lib/ts
    
    /mnt/sdcard/navit/ts/ts_calibrate
    /mnt/sdcard/navit/ts/ts_test

Here, /dev/fb is the frame buffer device, and /dev/ts is the
touchscreen. Newer TomTom versions (ONE, XL) have the touchscreen at
/dev/input/event0 instead. In this case you need to change the
corresponding line to:

.. code:: bash

    export TSLIB_TSDEVICE=/dev/input/event0

To run this script, create a file named **ts.cap** in the SDKRegistry
directory with the following contents:

| ``Version|100|``
| ``AppName|ts-wrapper|``
| ``AppPath|/mnt/sdcard/navit/ts/|``
| ``AppIconFile||``
| ``AppMainTitle|Touchscreen|``
| ``AppPort||``
| ``COMMAND|CMD|hallo||Touchscreen|``

This will create an entry named "Touchscreen" in TomTom's main menu.

.. _installing_sdl_utilities:

Installing SDL utilities
~~~~~~~~~~~~~~~~~~~~~~~~

If you have compiled the SDL test applications along with libSDL, place
them in the navit/sdl folder on your TomTom device, as well as the
\*.bmp, \*.wav , \*.xbm, \*.dat and \*.txt files from the folder named
test in the libSDL source directory. Before running any of these
utilities, we need to set some environment variables. The overall
procedure is very similar to the tslib utilities mentioned above. You
need to create a wrapper script which with the following lines in it:

.. code:: bash

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/sdcard/navit/lib
    
    export SDL_NOMOUSE=1
    export SDL_FBDEV=/dev/fb
    export SDL_VIDEODRIVER=fbcon

Add a line with the command that should be executed by the script. Put
this file into the navit/sdl folder on your TomTom device. Finally, you
need to create an entry in Navit's main menu. Place a file named sdl.cap
in the SDKRegistry with the following contents:

| ``Version|100|``
| ``AppName|sdl-wrapper|``
| ``AppPath|/mnt/sdcard/navit/sdl/|``
| ``AppIconFile||``
| ``AppMainTitle|SDL-Utility|``
| ``AppPort||``
| ``COMMAND|CMD|hallo||SDL-Utility|``

Replace "sdl-wrapper" with the name of your wrapper script, if
necessary.

.. _installing_navit_binaries_plugins_and_icons:

Installing Navit binaries, plugins and icons
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Put the Navit executable into the bin folder:

.. code:: bash

    cp $PREFIX/bin/navit /media/TOMTOM/navit/bin/

In order to run Navit, we need to create a short wrapper script, which
will set a few environment variables before running the actual Navit
executable. Create a file named navit-wrapper in the bin directory with
the following contents:

.. code:: bash

    #!/bin/sh
    
    cd /mnt/sdcard
    
    # Set some paths.
    export PATH=$PATH:/mnt/sdcard/navit/bin
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/sdcard/navit/lib
    export HOME=/mnt/sdcard/
    export NAVIT_LIBDIR=/mnt/sdcard/navit/lib/navit
    export NAVIT_SHAREDIR=/mnt/sdcard/navit/share
    
    # tslib requirements.
    export TSLIB_CONSOLEDEVICE=none
    export TSLIB_FBDEVICE=/dev/fb
    export TSLIB_TSDEVICE=/dev/ts
    export TSLIB_CALIBFILE=/mnt/sdcard/navit/ts/pointercal
    export TSLIB_CONFFILE=/mnt/sdcard/navit/ts/ts.conf
    export TSLIB_PLUGINDIR=/mnt/sdcard/navit/lib/ts
    
    # SDL requirements.
    export SDL_MOUSEDRV=TSLIB
    export SDL_MOUSEDEV=$TSLIB_TSDEVICE
    export SDL_NOMOUSE=1
    export SDL_FBDEV=/dev/fb
    export SDL_VIDEODRIVER=fbcon
    export SDL_AUDIODRIVER=dsp
    
    # fontconfig requirements
    export FC_CONFIG_DIR=/mnt/sdcard/navit/fonts
    export FONTCONFIG_DIR=/mnt/sdcard/navit/fonts
    export FC_CONFIG_FILE=/mnt/sdcard/navit/fonts/fonts.conf
    export FONTCONFIG_FILE=/mnt/sdcard/navit/fonts/fonts.conf
    export FC_DEBUG=0
    
    # Set language.
    export LANG=en_US.utf8
    
    # Run Navit.
    /mnt/sdcard/navit/bin/navit /mnt/sdcard/navit/share/navit.xml 2>/mnt/sdcard/navit/navit.log

The first few lines tell Navit where it can find binaries, libraries,
plugins and map icons. The middle part tells tslib which touchscreen
device to use, and where to place calibration data. For newer TomToms,
the touchscreen device is at /dev/input/event0, so the corresponding
line needs to be changed to:

``export TSLIB_TSDEVICE=/dev/input/event0``

.. raw:: html

   </source>

The following lines tell libSDL to use the touchscreen as mouse input,
to hide the mouse pointer and to use the framebuffer. The last lines set
Navit's language and run the Navit executable. stderr is redirected into
a log file, since there is usually no console available to view debugger
output.

You may have noticed the presence of a "NAVIT_LIBDIR" in the wrapper
script. This is where Navit will look for plugins, so we put the plugins
right there:

.. code:: bash

    cp -r $PREFIX/lib/navit /media/TOMTOM/navit/lib/

Next, copy the images used by Navit to display POIs, as well as the
navit config file into the share folder, and create a folder for
installing maps:

.. code:: bash

    cd /media/TOMTOM/navit/share
    cp -r $PREFIX/share/navit/xpm ./
    cp $PREFIX/share/navit/navit.xml ./
    mkdir -p maps

.. figure:: TomTom-Menu.png
   :alt: TomTom menu with Navit icon.
   :width: 240px

   TomTom menu with Navit icon.

In order to start Navit from TomTom's graphical menu, we finally have to
add a menu button. Create a file named navit.cap with the following
contents in /media/TOMTOM/SDKRegistry:

| ``Version|100|``
| ``AppName|navit-wrapper|``
| ``AppPath|/mnt/sdcard/navit/bin/|``
| ``AppIconFile|navit.bmp|``
| ``AppMainTitle|Navit|``
| ``AppPort||``
| ``COMMAND|CMD|hallo|navit.bmp|Navit|``

In the same directory, place a simple 48 \* 48 pixel, 24 bit bitmap file
named navit.bmp with a fancy Navit icon or whatever you like. If you are
happy without such an icon, you can omit this step and delete the two
"navit.bmp" entries from the file above:

| ``Version|100|``
| ``AppName|navit-wrapper|``
| ``AppPath|/mnt/sdcard/navit/bin/|``
| ``AppIconFile||``
| ``AppMainTitle|Navit|``
| ``AppPort||``
| ``COMMAND|CMD|hallo||Navit|``

.. _installing_maps:

Installing maps
~~~~~~~~~~~~~~~

Any navigation software would be useless without maps. See the
`Maps <Maps>`__ section for maps you can use with Navit. Put them into
the directory /media/TOMTOM/navit/share/maps which you have just
created.

.. _configuring_navit:

Configuring Navit
-----------------

Before running Navit, we need to change some settings in the Navit
config file, **navit.xml**, which we have placed in navit/share on the
TomTom disk. Here are some settings which are rather useful:

-  As long as you don't have gdb on your TomTom, set the segfault
   debugging level to 0, i.e. let Navit crash without printing a stack
   trace via gdb. Otherwise Navit will complain that gdb can't be found.
   ::

      <debug name="segv" level="0"/>

-  Use libSDL for graphics output. Set the screen witdh and height as
   well as the bits per pixel fitting to your TomTom device. (Have a
   look at http://www.opentom.org/Hardware_TFT_LCD for a list of screen
   sizes.) Disable the window frame and set SDL flags such that
   "software surfaces" are enabled.
   ::

      <graphics type="sdl" w="480" h="272" bpp="16" frame="0" flags="1"/>

-  Use the internal GUI. This is most suitable for touchscreen devices.
   ::

      <gui type="internal" enabled="yes">

-  GPS data can be fetched from a file named /var/run/gpsfeed. If you
   have gpsd running on your TomTom device, you can simply use gpsd
   instead.
   ::

      <vehicle name="Local GPS" profilename="car" enabled="yes" active="1" source="file:/var/run/gpsfeed">

Finally, enable the mapsets you wish to use. Remember that TomTom's disk
will be mounted at /mnt/sdcard at runtime, so any maps you place in the
navit/share/maps folder will appear in /mnt/sdcard/navit/share/maps
(which is the same as $NAVIT_SHAREDIR/maps, if you have set
$NAVIT_SHAREDIR as shown above).

.. _running_navit:

Running Navit
-------------

Running Navit is as simple as a touch of a button. Disconnect your
device properly from the computer and let it reboot. Touch the screen to
enter the main manu. On the last page of the main menu, a new menu entry
named "Navit" should be visible. Simply press the Navit button and wait
for the Navit screen to appear. Have fun!

.. _debugging_navit:

Debugging Navit
---------------

Sometimes it is useful to obtain debugging output from Navit, for
example to include it in a bug report. Add the following lines to your
navit.xml (if they are not already present):

.. code:: xml

    <debug name="navit" level="0"/>
    <debug name="graphics_sdl" level="0"/>
    <debug name="gui_internal" level="0"/>
    <debug name="map_binfile" level="0"/>
    <debug name="map_textfile" level="0"/>
    <debug name="vehicle_file" level="0"/>
    <debug name="vehicle_demo" level="0"/>
    <debug name="font_freetype" level="0"/>
    <debug name="osd_core" level="0"/>

You can now switch on debugging output for different parts of Navit by
increasing the corresponding debug level. For example, to obtain
standard debug messages from the Navit core application, increase the
debug level in the "navit" entry to 1. For more debug output, you can
use higher numbers, i.e. setting the debug level in the "osd_core" entry
to 5 will give you plenty of debug output from the on screen display
plugin.

Make sure that the debug output is redirected into some file which you
can transfer to your computer after running Navit, so you need an entry
similar to this one in your navit-wrapper script:

.. code:: bash

    # Run Navit.
    /mnt/sdcard/navit/bin/navit /mnt/sdcard/navit/share/navit.xml 2>/mnt/sdcard/navit/navit.log

The debugger output will then be placed in a file named "navit.log" in
the navit folder on your TomTom.

.. _additional_helpful_tools:

Additional helpful tools
------------------------

It is often useful to run programs from a console. For a simple console
running on TomTom devices, have a look at `TomTom
Console <https://web.archive.org/web/20120921131048/http://www-cip.physik.uni-bonn.de/~hoffmann/TTconsole/>`__.
It seems not to run an all TomTom devices and software versions, so you
may try using `Btconsole <http://www.opentom.org/Btconsole>`__ instead,
which allows you to login to your TomTom device via bluetooth.

.. _things_to_be_done:

Things to be done
-----------------

There are quite a few things that still need to be done:

-  porting gpsd
