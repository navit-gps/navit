.. _macos_development:

MacOS development
=================

.. raw:: mediawiki

   {{warning|1='''This is a page has been migrated to readthedocs:'''https://github.com/navit-gps/navit/pull/883 . It is only kept here for archiving purposes.}}

**Here are some notes about running navit under Apple Mac OSX.**

.. _what_you_will_need:

What you will need
==================

You need Xcode Tools and MacPorts in order to install navit.

MacPorts developers suggest to install Xcode Tools from
http://developer.apple.com/tools/xcode/ and not from the Mac OSX install
disk.

-  `Mac-How <http://www.mac-how.net/>`__

Make sure you don't have fink installed on your system, it can confuse
MacPorts package building and installation.

.. _gtk_gui:

GTK Gui
-------

You should only need gtk2 and glib2 via macPorts

.. _sdl_gui:

SDL Gui
-------

Untested yet.

.. _installation_instruction:

Installation instruction
========================

Download Xcode Tools from http://developer.apple.com/tools/xcode/ and
install it with X11 SDK

Download and Install MacPorts from http://www.macports.org/, or update
your version

`` sudo port -d selfupdate``

Open up a terminal

make sure your PATH variables has /opt/local/bin and /opt/local/sbin in
it:

`` echo $PATH ``

Install automake, wget, libtool, gpsd (if you want gps support), gtk2
and glib2 (for gkt GUI) with

`` sudo port install automake wget gpsd gtk2 glib2 libtool``

Download navit or checkout it from SVN

``  svn co ``\ ```https://svn.code.sf.net/p/navit/code/trunk/navit`` <https://svn.code.sf.net/p/navit/code/trunk/navit>`__

You may also need a header file to handle endian issues (for PPC only)

``wget ``\ ```https://navit.svn.sourceforge.net/svnroot/navit/tags/R0_1_0/navit/projs/CodeBlocks/Win32Extra/byteswap.h`` <https://navit.svn.sourceforge.net/svnroot/navit/tags/R0_1_0/navit/projs/CodeBlocks/Win32Extra/byteswap.h>`__

If you want to install navit along the MacPorts packages, you need to
use the /opt/local directory as prefix:

`` ./autogen.sh && ./configure --prefix=/opt/local --disable-binding-python``

type

`` make``

to build NavIt, and

`` sudo make install``

to install it.

Then, you may edit and adapt your navit.xml file. The XML maptype is not
supported, however normal Navit binfile works perfectly.

Speech
======

If you want (spoken) directions, get espeak from
http://espeak.sourceforge.net, install as advised and use the following
snippet in your navit.xml:

``          ``\ 

This will tell speak to use a female (f) german (de) voice.

.. _using_xcode:

Using xcode
===========

Download one of the `svn
sources <http://download.navit-project.org/navit/src/svn/>`__ that don't
contain autogen.sh.

Open X-Code and create a new project. Cocoa will suffice

Add in a new target by clicking the triangle next to "Targets" and
selected the location of the navit folder. Delete the previous target.

Delete the default files, and add in the navit files.

In a terminal, go into the navit folder.

``./configure --disable-binding-python --disable-sample-map --disable-maptool``

xcode can now build the navit

You can also use `CMake <CMake>`__.

``cd navit && cmake -G Xcode .``

.. _something_went_wrong:

Something went wrong?
=====================

Please `let us know <Contacts>`__.
