===================
Windows Development
===================

Compiling using CMake
=====================

At the moment, compiling with [[CMake]] seems to be the only way to create a runnable binary in Windows.


Compiling / debugging using CodeBlocks & mingw compiler
=======================================================

 Up to and including release 0.0.4 the Win32 builds were supported using the CodeBlocks/mingw development environment,
 in combination with the glade for win32 GTK devlopment toolkit. For release 0.1.0 and later use native mingw
 (see below) or cygwin (see below).

Downloads
---------

In order to compile the win32 version of Navit, you need the following software development tools:
 * Glade/gtk+ toolkit for win32 from `Glade 3.43 / Gtk 2.12.9 <http://sourceforge.net/project/showfiles.php?group_id=98754>`_
   and `SourceForgeDownload: Gtk+ 2.10.11 <http://sourceforge.net/project/showfiles.php?group_id=98754&package_id=111411>`_
 * ming compiler from `Website: mingw <http://www.mingw.org>`_ or `SourceForgeDownload: MinGW <http://sourceforge.net/project/showfiles.php?group_id=2435>`_ (select Automated MinGW installer).
 * CodeBlocks IDE from `CodeBlocks download page <http://www.codeblocks.org/downloads.shtml>`_ (select recent development snapshot)
   or `SourceForgeDownload: CodeBlocks <http://sourceforge.net/project/showfiles.php?group_id=126998&package_id=138996>`_

Installation
------------

Install the packages mentioned above. After everything has been installed you can open the navit.workspace file in CodeBlocks:

.. warning::

    Not up to date! Directory projs\CodeBlocks was deleted in 2009

Compiling
---------

.. warning::

    Not up to date! Directory projs\CodeBlocks was deleted in 2009

To compile:

 * Start the CodeBlocks application
 * Open the navit.workspace file (located in projs\CodeBlocks directory)
 * Set the GTK_DIR enviroment variable in CodeBlocks (Setting/Environment, and select environments variables)
 * the GTK_DIR should point to where you have installed the Glade/Gtk toolkit package (e.g. d:\gtk)

Now you should be able to  build/debug the navit project:

Note:

**ZLIB -lzdll message** `Settings> Compiler and Debugger..> Global compiler settings` In the Linker settings TAB (Add) C:\MinGW\lib\libzdll.a

**SAPI** You need to download and install the `Microsoft Speech SDK 5.1 <http://www.microsoft.com/downloads/details.aspx?FamilyID=5e86ec97-40a7-453f-b0ee-6583171b4530&displaylang=en>`_ for this project to build.

Running from debugger
---------------------

In order to run navit from the CodeBlocks debugger, you have to:
 * Copy the navit.xml file from the source directory into the projs\CodeBlocks directory
 * Copy the xpm directory from the toplevel directory into the projs\CodeBlocks directory
 * Modify the navit.xml to set the map (currently only OSM binary files are supported)

Compiling and running using cygwin
==================================

Download cygwin
---------------

Download the cygwin setup.exe from http://cygwin.com/setup.exe

.. note::
    I have been unable to build with cygwin according to these instructions. I suggest you only try it if you are knowledgable
    and can improve the instructions. --[[User:Nop|Nop]] 13:01, 7 November 2010 (UTC)

Dependencies
------------

You will probably need the following packages from cygwin :

 * automake
 * autoconf
 * gtk2-x11-devel
 * libQt4Gui-devel
 * libQtSql4--devel
 * gcc
 * g++ (for qt rendered)
 * gettext-devel
 * diffutils
 * pkgconfig
 * xorg-x11-devel
 * glib2-devel
 * pango-devel
 * atk-devel
 * libtool
 * make
 * rsvg
 * wget
 * cvs because of autopoint

Prepare the build
-----------------

When using cygwin 1.7 you can skip this block and continue at cygwin 1.7

Edit configure.in and add the following to CFLAGS at line 10:

.. code-block:: bash

    -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include

It should look like this :

.. code-block:: bash

         CFLAGS="$CFLAGS -Wall -Wcast-align -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith -Wreturn-type -D_GNU_SOURCE -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include"

Now run `autogen.sh && ./configure`

If you get: `checking for X... no`
try adding the following parameters to ./configure :
`--x-libraries=/usr/X11R6/lib  --x-include=/usr/X11R6/includes`

Cygwin 1.7
''''''''''

With cygwin 1.7 is fairly easy to build navit. Install all the required packages(some has diffrent names now).
Run the autogen script first `./autogen.sh`
and then configure with the following options: `./configure --disable-binding-python --disable-plugins`

Build navit
-----------

Skip for cygwin 1.7

Currently, building navit will fail at this point, because we haven't found an implementation of the wordexp function for cygwin.

Here's a message in that thread from an actual competent Cygwin user: http://www.mail-archive.com/cygwin@cygwin.com/msg16750.html

The implication of that is a "C library". A "C library" is an "implementation" of reusable code. It consists of a library file that contains the compiled object code and a header file with the matching declarations that goes along with it. The library is implemented as a static archive at build time and simply linked into the app binary. There's nothing to include in that case -- it's already in there.


Cygwin 1.7
''''''''''

Just type `make` and `make install`.  You can use stow for easy install and uninstall stuff without using packagemangement.

Configuration GPS
-----------------

.. note::

    If this works at all, it's only when running under cygwin. See above for the proper Win32 configuration. --[[User:Nop|Nop]] 13:04, 7 November 2010 (UTC)

If you have a gps cable device which spits out NMEA data, you can configure it like under unix. Beware of the following enumeration:
 * ComPort1==ttyS0
 * ComPort2==ttyS1
 * ...

Example:

.. code-block:: xml

    <vehicle name="GPSOnCom3" profilename="car" enabled="yes" active="1" source="file:/dev/ttyS2" baudrate="38400" color="#0000ff"/>

Running under Cygwin
--------------------

To run navit under cygwin you need to install the cygwin xorg-server. Than just run navit.



Make a redistributable package
------------------------------


Please read and understand http://cygwin.com/licensing.html so that you don't infringe Cygwin's intellectual property rights (copyleft) when you distribute the package you've built.
Then follows: http://cygwin.com/setup.html

Compiling a native binary using mingw
=====================================

The main advantage of this method is that it will produce a redistributable binary.

Downloads
---------

In order to compile the win32 version of Navit, you need the following software development tools
 * GTK+ toolkit for win32 from `SourceForgeDownload: Glade/GTK+ <http://sourceforge.net/project/showfiles.php?group_id=98754>` (select gtk+-win32-devel)
 * MinGW from `SourceForgeDownload: MinGW <http://sourceforge.net/project/showfiles.php?group_id=2435>`_ (select Automated MinGW installer)
 * MSYS from `SourceForgeDownload: MSYS Base System <http://sourceforge.net/project/showfiles.php?group_id=2435&package_id=2496>`
 * msysCORE from `SourceForgeDownload: MSYS Base System <http://sourceforge.net/project/showfiles.php?group_id=2435&package_id=24963>`
 * diffutils from `SourceForgeDownload: MSYS Base System <http://sourceforge.net/project/showfiles.php?group_id=2435&package_id=24963>`
 * autoconf from `SourceForgeDownload: MSYS Supplementary Tools <http://sourceforge.net/project/showfiles.php?group_id=2435&package_id=67879>`
 * autogen from `SourceForgeDownload: MSYS Supplementary Tools <http://sourceforge.net/project/showfiles.php?group_id=2435&package_id=67879>`
 * automake from `SourceForgeDownload: MSYS Supplementary Tools <http://sourceforge.net/project/showfiles.php?group_id=2435&package_id=67879>`
 * gettext from `SourceForgeDownload: MSYS Supplementary Tools <http://sourceforge.net/project/showfiles.php?group_id=2435&package_id=67879>`
 * libtool from `SourceForgeDownload: MSYS Supplementary Tools <http://sourceforge.net/project/showfiles.php?group_id=2435&package_id=67879>`
 * libiconv from `SourceForgeDownload: MSYS Supplementary Tools <http://sourceforge.net/downloads/mingw/MinGW/libiconv>`

Probably the easiest way to obtain and install all the MSYS packages is to follow the instructions `here <http://www.mingw.org/wiki/msys>`

For speech support, one option is to use the "cmdline" speech type (refer to [[Configuration]]) and a utility such as a Windows port of `Say <http://krolik.net/wsvn/wsvn/public/Say%2B%2B/>`_

TroubleShooting
===============

.. code-block::

    /bin/m4: unrecognized option '--gnu'

Wrong version of m4, use 1.4.13


.. code-block::

    Can't locate object method "path" via package "Request (perhaps you forgot to load "Request"?)

Wrong version of Autoconf, make sure the latest version is installed, plus the wrapper (version 1.7). Also delete autom4te.cache.


.. code-block::

    command PKG_MODULE_EXISTS not recognized

For some reason the necessary file "pkg.m4" containing various macros is missing. Find it and put it in ./m4

Cross-Compiling win32 exe using Linux Ubuntu 14.04.1
====================================================

This is a quick walk-thru on compiling a win32 exe using Ubuntu as development machine.

Set up Ubuntu to build Linux version
------------------------------------

First, setup compiling in linux ubuntu explained in https://navit.readthedocs.io/en/trunk/development/linux_development.html
Here is a quick walk-thru:

Get all the dependencies for Ubuntu in one command:

.. code-block:: bash

    sudo apt-get install cmake zlib1g-dev libpng12-dev libgtk2.0-dev librsvg2-bin \
    g++ gpsd gpsd-clients libgps-dev libdbus-glib-1-dev freeglut3-dev libxft-dev \
    libglib2.0-dev libfreeimage-dev gettext

get the latest SVN-source.
First, cd into root: `cd ~`

Now, let's grab the code from SVN. This assumes that you have subversion installed.
This will download the latest SVN source and put in in folder "navit-source".
You can use any location you want for the source, just to keep it simple we place it right in the root.
`svn co  svn://svn.code.sf.net/p/navit/code/trunk/navit/ navit-source`

Create a directory to put the build in and cd into it:

.. code-block:: bash

    mkdir navit-build
    cd navit-build

Start compiling and build navit: `cmake ~/navit-source && make`

At the end of the process navit is built into navit-build/.
You can start navit to see if all worked well:

.. code-block:: bash

    cd ~/navit-build/navit/
    ./navit

Building the win32 exe
----------------------

Now that we have set up the basic building environment we can build a win32 exe using the next walk-thru.

Install ming32 and the dependencies: `sudo apt-get install mingw32 libsaxonb-java librsvg2-bin  mingw32-binutils mingw32-runtime default-jdk`

now cd into the source:

.. code-block:: bash

    cd ~
    cd navit-source

We are going to place the build directory within the source directory.
First, make the build directory and cd into it:

.. code-block:: bash

    mkdir build
    cd build

From within the build directory start compiling and building:

.. code-block:: bash

    cmake -DCMAKE_TOOLCHAIN_FILE=../Toolchain/mingw32.cmake ../

And then make the actual build:

.. code-block:: bash

    make -j4

The -j4 part is used to define the amount of processors the process can use.
So if you have a dual-core pc use -j2
If -j4 fails, try -j2 and if that fails try "make" alone.

Known "bugs"
------------

The "locale" folder is generated one level up.
because of that the languages in navit are not working
Cut and paste (or move) the "locale" folder to the navit folder.
This should be investigated anf fixed so the folder is in the correct place after a build.
So move `navit-source/build/locale/` to `navit-source/build/navit/locale`

You can run `mv navit-source/build/locale/  navit-source/build/navit/`

The country-flags images in the "town" search are not displayed.
This could be due to a conversion error during build, has to be investigated and solved but doesn't inflict with the use of navit.

There are a lot of empty folders that are not of use.
Also there are cmake folders and files in every folder.
You can delete those without any problem.

Windows Mobile/Windows CE
=========================

[[WinCE_development]] may have details that are relevant for compilation on WindowsCE / Windows Mobile.

You can download now
[http://download.navit-project.org/navit/wince/svn/ cab or zip file for Windows Mobile and WindowsCE]!
Highest number is the newest version of NavIt.

Download it and save on your Storage Card. Install it.

Now you have NavIt on your PDA or Mobile Phone.

This is a manual for self compiling (navit.exe)


You need to have a Linux (like Ubuntu).
If you didn´t have Linux, start your Linux on Live-CD.

Compiling navit for wince using http://cegcc.sourceforge.net/.
Download latest cegcc release and install it.

In November 2009 versions compiled using arm-cegcc-gcc (both revision 1214 and release 0.59.1) had problems (threw exception_datatype_misalignment and caused access violations).<br />
Using the variant arm-mingw32ce of CeGCC 0.59.1 it was possible to build a working executable which can be debugged (see [[WinCE development]]).

Source [http://www.archlinux.de/?page=PackageDetails;package=4837 cegcc-arm and mingw]  (TODO dead link)

Current installs in /opt/cegcc.
Setup a cross-compile environment:

Example setcegccenv.sh:

.. code-block:: bash

    #! /bin/bash
    export PATH=$PATH:/opt/cegcc/bin/
    export CEGCC_PATH=/opt/cegcc
    export WINCE_PATH=/opt/wince
    export PATH=$CEGCC_PATH/bin:$PATH
    export CPPFLAGS="-I$WINCE_PATH/include"
    export LDFLAGS="-L$WINCE_PATH/lib -L$CEGCC_PATH/lib"
    export LD_LIBRARY_PATH="$WINCE_PATH/bin"
    export PKG_CONFIG_PATH="$WINCE_PATH/lib/pkgconfig"
    export PKG_CONFIG_LIBDIR="$WINCE_PATH/lib/pkgconfig"


For installation, compiling and configuring please see manual for NavIt on Linux.

Then autogen.sh and configure navit. Example configure for wince:

.. code-block:: bash

    ./configure \
    RANLIB=arm-cegcc-ranlib \
    CXX=arm-cegcc-g++ \
    CC=arm-cegcc-gcc \
    --host=arm-pe-wince \
    --disable-readline \
    --disable-dynamic-extensions \
    --disable-largefile \
    --enable-tempstore \
    CFLAGS="-I/opt/wince/include -mwin32 -DWIN32 -D_WIN32_WCE=0x0400 -D_WIN32_IE=0x0400 -Wl,--enable-auto-import" \
    LDFLAGS="-L/opt/wince/lib" \
    --prefix=/opt/wince/  \
    WINDRES=arm-cegcc-windres \
    --disable-vehicle-demo \
    --disable-vehicle-file \
    --disable-speech-cmdline \
    --disable-speech-speech-dispatcher  \
    --disable-postgresql \
    --disable-plugins \
    --prefix=/opt/wince \
    --disable-graphics-qt-qpainter \
    --disable-gui-sdl  \
    --disable-samplemap \
    --disable-gui-gtk \
    --disable-gui-internal \
    --disable-vehicle-gypsy \
    --disable-vehicle-file \
    --disable-vehicle-demo  \
    --disable-binding-dbus \
    --enable-avoid-unaligned \
    --enable-avoid-float

If example did not run, do this:

.. code-block:: bash

    ./configure \
     RANLIB=arm-mingw32ce-ranlib \
     CXX=arm-mingw32ce-g++ \
     CC=arm-mingw32ce-gcc \
     --host=arm-pe-wince \
     --disable-readline \
     --disable-dynamic-extensions \
     --disable-largefile \
     --enable-tempstore ¸\
     CFLAGS="-mwin32 -DWIN32 -D_WIN32_WCE=0x0400 -D_WIN32_IE=0x0400 -Wl,\
     --enable-auto-import" WINDRES=arm-mingw32ce-windres \
     --disable-vehicle-demo  \
     --disable-vehicle-file \
     --disable-speech-cmdline \
     --disable-speech-speech-dispatcher  \
     --disable-postgresql  \
     --disable-plugins \
     --prefix=/opt/wince \
     --disable-graphics-qt-qpainter \
     --disable-gui-sdl  \
     --disable-samplemap \
     --disable-gui-gtk \
     --disable-gui-internal \
     --disable-vehicle-gypsy \
     --disable-vehicle-file \
     --disable-vehicle-demo \
     --disable-binding-dbus \
     --enable-avoid-unaligned \
     --enable-avoid-float \
     --enable-support-libc \
     PKG_CONFIG=arm-mingw32ce-pkgconfig


This is basic just to view the maps. Then: `make`
As usual, osm2navit.exe will fail to compile. `cd navit && make navit.exe`
You find navit.exe under (your directory)/navit/navit/navit.exe

Install sync on your system.


----

For installation you need packages librapi, liprapi2, pyrapi2, libsync.
Package synce-0.9.0-1 contains librapi and libsync.
You do not need to install it again!

Sources: [http://sourceforge.net/project/showfiles.php?group_id=30550 Sync] If link is crashed, use this: [http://rpmfind.net/linux/rpm2html/search.php?query=librapi.so.2 Sync Link2]
libsync: [http://sourceforge.net/project/mirror_picker.php?height=350&width=300&group_id=30550&use_mirror=puzzle&filesize=&filename=libsynce-0.12.tar.gz&abmode= libsync]
pyrapi2: [http://rpmfind.net/linux/rpm2html/search.php?query=pyrapi2.so pyrapi2]
librapi2 [http://repository.slacky.eu/slackware-12.0/libraries/synce-librapi/0.11.0/src/ librapi2]

Once you have navit.exe ready, copy `/opt/cegcc/arm-cegcc/lib/device/*.dll` on your device.

For Debian use:

.. code-block::

    synce-pcp /opt/cegcc/arm-cegcc/lib/device/cegcc.dll ":/windows/cegcc.dll"
    synce-pcp /opt/cegcc/arm-cegcc/lib/device/cegccthrd.dll ":/windows/cegccthrd.dll"

All other Linux/Unix systems use:

.. code-block::

    pcp /opt/cegcc/arm-cegcc/lib/device/cegcc.dll ":/windows/cegcc.dll"
    pcp /opt/cegcc/arm-cegcc/lib/device/cegccthrd.dll ":/windows/cegccthrd.dll"


Synchronisation with a grahic surface, if connection to device failed:

Packages RAKI and RAPIP you can use.

RAKI you have in packages synce-kde (see Synce).

RAKI is like Active Sync, RAPIP is a little bit like fish:// under Konquerror.

Under SuSE Linux you can run kitchensync (not for all PDA).

For synchronisation you can also use kpilot under Suse Linux (runs not with all PDA) or Microsoft Active Sync under Windows (free download at Microsoft homepage).

You can put your memory card in card reader and copy data. Over console you must type in `sync` before you remove memory card.

Install navit.exe.

Debian:

.. code-block::

    synce-pcp navit.exe ":/Storage Card/navit.exe"

All other:

.. code-block::

    pcp navit.exe ":/Storage Card/navit.exe"


Prepare a navit.xml.wince

Change gui to win32 and graphics to win32.

Fix the paths to your maps "/Storage Card/binfilemap.bin"

Debian:

.. code-block::

    synce-pcp binfilemap.bin ":/Storage Card/binfilemap.bin"
    synce-pcp navit.xml.wince ":/Storage Card/navit.xml"

All other:

.. code-block::

    pcp binfilemap.bin ":/Storage Card/binfilemap.bin"
    pcp navit.xml.wince ":/Storage Card/navit.xml"


For a start best use the samplemap.
Now you can launch navit.exe on the device.
