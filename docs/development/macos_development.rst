=================
MacOS Development
=================

Here are some notes about running navit under Apple Mac OSX.

================================================================================================================================
WARNING: These instructions are currently unmaintained. Please feel free to submit a PR if you manage to build navit on Mac OSX.
================================================================================================================================

What you will need
==================

You need Xcode Tools and homebrew in order to install navit.

For convinience there is the script prepare_navit_macos.sh available under the navit/scripts directory.

.. code-block:: bash

 $ curl https://raw.githubusercontent.com/OLFDB/navit/macosbuild/scripts/prepare_navit_macos.sh  -o prepare_navit_macos.sh 

The steps to create the environment and build are explained below:

If you already have macports, or fink installed create a new user account on your Mac and use that for building navit.

 * Create a new user navituser with admin privileges and restart your machine.
 * Install `homebrew <https://brew.sh/index_de>`
 
.. code-block:: bash
 
 $ /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

Install glib, gtk+, libpng, protobuf-c, cmake, librsvg

.. code-block:: bash

 $ brew install glib, gtk+, libpng, protobuf-c, cmake, librsvg
 
What is working
===============
* internal Gui: 	Working, but problems with window refresh
* GTK Gui: Untested yet.
* SDL Gui: Untested yet.

Installation instruction
========================

Get the navit sources:

.. code-block:: bash

$ git clone https://github.com/navit-gps/navit.git

Change directory to ./navit and create a folder build
.. code-block:: bash
$ cd navit
$ mkdir build
$ cd build

Configure your build using cmake:

.. code-block:: bash

$ cmake -Dbinding/python=false ../

type `make` to build NavIt, and `sudo make install` to install it. Run `sudo make install` twice to have all libraries inside the app bundle

$ make

$ sudo make install

$ sudo make install


Then, you may edit and adapt your `navit.xml` file. The XML maptype is not supported, however normal Navit binfile works perfectly.

Speech
======

If you want (spoken) directions, use the following snippet in your navit.xml:

.. code-block:: xml

           <speech type="cmdline" data="say '%s'"/>

This will use the native say command. You can list all available voices by typing say -v ? in a terminal.
Change the command to say -v <voicename> if you would like a non standard voice to be used. New voices can be added in system preferences->keyboard->dictation


Using xcode
===========

============================================================================================================================
WARNING: These instructions are currently outdated. Please feel free to submit a PR if you manage to build navit on Mac OSX.
============================================================================================================================

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
